#include "cursive_rt.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TASK104_THREAD_COUNT 4u
#define TASK104_RECORDS_PER_THREAD 12u

typedef struct Task104Record {
  uint64_t seq;
  uint64_t tid;
  char* line;
} Task104Record;

typedef struct Task104RecordList {
  Task104Record* data;
  size_t count;
} Task104RecordList;

typedef struct Task104ThreadArgs {
  uint32_t thread_index;
  DWORD thread_id;
  uint32_t emit_count;
} Task104ThreadArgs;

static int report_failure(const char* label) {
  fprintf(stderr, "TASK-104 runtime regression failed: %s\n", label);
  return 1;
}

static C0StringView sv_from_cstr(const char* text) {
  C0StringView out;
  out.data = (const uint8_t*)text;
  out.len = text ? (uint64_t)strlen(text) : 0u;
  return out;
}

static char* utf8_from_wide(const wchar_t* wide) {
  int needed;
  char* utf8;
  if (!wide) {
    return NULL;
  }
  needed = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
  if (needed <= 0) {
    return NULL;
  }
  utf8 = (char*)malloc((size_t)needed);
  if (!utf8) {
    return NULL;
  }
  if (WideCharToMultiByte(CP_UTF8, 0, wide, -1, utf8, needed, NULL, NULL) <= 0) {
    free(utf8);
    return NULL;
  }
  return utf8;
}

static char* make_temp_file_utf8(const wchar_t* prefix) {
  wchar_t temp_root[MAX_PATH];
  wchar_t temp_path[MAX_PATH];
  if (GetTempPathW(MAX_PATH, temp_root) == 0) {
    return NULL;
  }
  if (GetTempFileNameW(temp_root, prefix, 0, temp_path) == 0) {
    return NULL;
  }
  return utf8_from_wide(temp_path);
}

static char* read_file_utf8(const char* path_utf8) {
  wchar_t* wide_path;
  HANDLE file;
  LARGE_INTEGER size;
  DWORD read = 0;
  char* bytes;
  int needed;
  if (!path_utf8) {
    return NULL;
  }
  needed = MultiByteToWideChar(CP_UTF8, 0, path_utf8, -1, NULL, 0);
  if (needed <= 0) {
    return NULL;
  }
  wide_path = (wchar_t*)malloc(sizeof(wchar_t) * (size_t)needed);
  if (!wide_path) {
    return NULL;
  }
  if (MultiByteToWideChar(CP_UTF8, 0, path_utf8, -1, wide_path, needed) <= 0) {
    free(wide_path);
    return NULL;
  }
  file = CreateFileW(wide_path,
                     GENERIC_READ,
                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                     NULL,
                     OPEN_EXISTING,
                     FILE_ATTRIBUTE_NORMAL,
                     NULL);
  free(wide_path);
  if (file == INVALID_HANDLE_VALUE) {
    return NULL;
  }
  if (!GetFileSizeEx(file, &size) || size.QuadPart < 0) {
    CloseHandle(file);
    return NULL;
  }
  if ((uint64_t)size.QuadPart > (uint64_t)SIZE_MAX - 1u) {
    CloseHandle(file);
    return NULL;
  }
  bytes = (char*)malloc((size_t)size.QuadPart + 1u);
  if (!bytes) {
    CloseHandle(file);
    return NULL;
  }
  if (size.QuadPart != 0) {
    if (!ReadFile(file, bytes, (DWORD)size.QuadPart, &read, NULL)) {
      free(bytes);
      CloseHandle(file);
      return NULL;
    }
  }
  bytes[read] = '\0';
  CloseHandle(file);
  return bytes;
}

static void delete_file_utf8(const char* path_utf8) {
  int needed;
  wchar_t* wide_path;
  if (!path_utf8) {
    return;
  }
  needed = MultiByteToWideChar(CP_UTF8, 0, path_utf8, -1, NULL, 0);
  if (needed <= 0) {
    return;
  }
  wide_path = (wchar_t*)malloc(sizeof(wchar_t) * (size_t)needed);
  if (!wide_path) {
    return;
  }
  if (MultiByteToWideChar(CP_UTF8, 0, path_utf8, -1, wide_path, needed) > 0) {
    DeleteFileW(wide_path);
  }
  free(wide_path);
}

static int parse_uint64_field(const char* text,
                              const char* key,
                              uint64_t* out_value) {
  const char* cursor;
  uint64_t value = 0u;
  if (!text || !key || !out_value) {
    return 0;
  }
  cursor = strstr(text, key);
  if (!cursor) {
    return 0;
  }
  cursor += strlen(key);
  if (*cursor < '0' || *cursor > '9') {
    return 0;
  }
  while (*cursor >= '0' && *cursor <= '9') {
    value = (value * 10u) + (uint64_t)(*cursor - '0');
    ++cursor;
  }
  *out_value = value;
  return 1;
}

static int collect_records(char* text, Task104RecordList* out_records) {
  size_t capacity = 0u;
  size_t count = 0u;
  Task104Record* records = NULL;
  char* line = text;
  if (!text || !out_records) {
    return 0;
  }
  while (*line != '\0') {
    char* next = strchr(line, '\n');
    Task104Record record;
    if (next) {
      *next = '\0';
    }
    if (*line != '\0' && strcmp(line, "runtime_trace_v1") != 0) {
      if (!parse_uint64_field(line, "seq=", &record.seq) ||
          !parse_uint64_field(line, "tid=", &record.tid)) {
        free(records);
        return 0;
      }
      record.line = line;
      if (count == capacity) {
        size_t new_capacity = capacity == 0u ? 16u : capacity * 2u;
        Task104Record* grown =
            (Task104Record*)realloc(records, sizeof(Task104Record) * new_capacity);
        if (!grown) {
          free(records);
          return 0;
        }
        records = grown;
        capacity = new_capacity;
      }
      records[count++] = record;
    }
    if (!next) {
      break;
    }
    line = next + 1;
  }
  out_records->data = records;
  out_records->count = count;
  return 1;
}

static int record_line_contains(const Task104RecordList* records,
                                const char* needle,
                                size_t min_count) {
  size_t count = 0u;
  size_t i;
  if (!records || !needle) {
    return 0;
  }
  for (i = 0u; i < records->count; ++i) {
    if (strstr(records->data[i].line, needle)) {
      ++count;
    }
  }
  return count >= min_count;
}

static int sequence_is_strictly_ascending(const Task104RecordList* records) {
  size_t i;
  if (!records || records->count == 0u) {
    return 0;
  }
  for (i = 1u; i < records->count; ++i) {
    if (records->data[i - 1u].seq >= records->data[i].seq) {
      return 0;
    }
  }
  return 1;
}

static int every_tid_matches(const Task104RecordList* records, uint64_t tid) {
  size_t i;
  if (!records || records->count == 0u) {
    return 0;
  }
  for (i = 0u; i < records->count; ++i) {
    if (records->data[i].tid != tid) {
      return 0;
    }
  }
  return 1;
}

static int every_tid_known(const Task104RecordList* records,
                           const DWORD* tids,
                           size_t tid_count) {
  size_t i;
  if (!records || !tids || tid_count == 0u) {
    return 0;
  }
  for (i = 0u; i < records->count; ++i) {
    size_t j;
    int matched = 0;
    for (j = 0u; j < tid_count; ++j) {
      if (records->data[i].tid == (uint64_t)tids[j]) {
        matched = 1;
        break;
      }
    }
    if (!matched) {
      return 0;
    }
  }
  return 1;
}

static void free_record_list(Task104RecordList* records) {
  if (!records) {
    return;
  }
  free(records->data);
  records->data = NULL;
  records->count = 0u;
}

static DWORD WINAPI task104_emit_thread(LPVOID param) {
  Task104ThreadArgs* args = (Task104ThreadArgs*)param;
  uint32_t i;
  args->thread_id = GetCurrentThreadId();
  for (i = 0u; i < args->emit_count; ++i) {
    char payload[128];
    C0StringView rule_id = sv_from_cstr("Log-Thread");
    C0StringView file = sv_from_cstr("task104_runtime_thread");
    C0StringView payload_view;
    int written = snprintf(payload,
                           sizeof(payload),
                           "category=log;level=info;label=thread-%u-%u",
                           (unsigned)args->thread_index,
                           (unsigned)i);
    if (written < 0 || (size_t)written >= sizeof(payload)) {
      return 1u;
    }
    payload_view = sv_from_cstr(payload);
    cursive_x3a_x3aruntime_x3a_x3aconformance_x3a_x3aemit(
        &rule_id, &file, 1u, (uint64_t)(i + 1u), 1u, (uint64_t)(i + 1u),
        &payload_view);
  }
  return 0u;
}

static int test_file_sequence_and_no_reset(void) {
  char* path_a = make_temp_file_utf8(L"c14");
  char* path_b = make_temp_file_utf8(L"c14");
  char* text_a = NULL;
  char* text_b = NULL;
  Task104RecordList records_a;
  Task104RecordList records_b;
  C0StringView rule = sv_from_cstr("Log-Test");
  C0StringView file = sv_from_cstr("task104_runtime_file");
  C0StringView payload_a = sv_from_cstr("category=log;level=info;label=file-a");
  C0StringView payload_b = sv_from_cstr("category=log;level=info;label=file-b");
  DWORD current_tid = GetCurrentThreadId();
  uint64_t last_seq_a = 0u;

  memset(&records_a, 0, sizeof(records_a));
  memset(&records_b, 0, sizeof(records_b));

  if (!path_a || !path_b) {
    free(path_a);
    free(path_b);
    return report_failure("temp file path allocation");
  }

  cursive_x3a_x3aruntime_x3a_x3aconformance_x3a_x3aset_x5fsink(
      1u, (const uint8_t*)path_a, (uint64_t)strlen(path_a));
  cursive_x3a_x3aruntime_x3a_x3aconformance_x3a_x3aemit(
      &rule, &file, 10u, 1u, 10u, 4u, &payload_a);
  cursive_x3a_x3aruntime_x3a_x3aconformance_x3a_x3aemit_x5fint(
      &rule, &file, 11u, 1u, 11u, 4u, &payload_a, 17u, 32u, 0u);

  text_a = read_file_utf8(path_a);
  if (!text_a) {
    delete_file_utf8(path_a);
    delete_file_utf8(path_b);
    free(path_a);
    free(path_b);
    return report_failure("read first sink file");
  }
  if (!collect_records(text_a, &records_a)) {
    free(text_a);
    delete_file_utf8(path_a);
    delete_file_utf8(path_b);
    free(path_a);
    free(path_b);
    return report_failure("parse first sink file");
  }
  if (records_a.count != 2u) {
    free_record_list(&records_a);
    free(text_a);
    delete_file_utf8(path_a);
    delete_file_utf8(path_b);
    free(path_a);
    free(path_b);
    return report_failure("first sink record count");
  }
  if (!sequence_is_strictly_ascending(&records_a)) {
    free_record_list(&records_a);
    free(text_a);
    delete_file_utf8(path_a);
    delete_file_utf8(path_b);
    free(path_a);
    free(path_b);
    return report_failure("first sink sequence order");
  }
  if (!every_tid_matches(&records_a, (uint64_t)current_tid)) {
    free_record_list(&records_a);
    free(text_a);
    delete_file_utf8(path_a);
    delete_file_utf8(path_b);
    free(path_a);
    free(path_b);
    return report_failure("first sink tid");
  }
  if (!record_line_contains(&records_a, "label%3Dfile-a", 2u)) {
    free_record_list(&records_a);
    free(text_a);
    delete_file_utf8(path_a);
    delete_file_utf8(path_b);
    free(path_a);
    free(path_b);
    return report_failure("first sink label");
  }
  last_seq_a = records_a.data[records_a.count - 1u].seq;

  cursive_x3a_x3aruntime_x3a_x3aconformance_x3a_x3aset_x5fsink(
      1u, (const uint8_t*)path_b, (uint64_t)strlen(path_b));
  cursive_x3a_x3aruntime_x3a_x3aconformance_x3a_x3aemit(
      &rule, &file, 12u, 1u, 12u, 4u, &payload_b);

  text_b = read_file_utf8(path_b);
  if (!text_b) {
    free_record_list(&records_a);
    free(text_a);
    delete_file_utf8(path_a);
    delete_file_utf8(path_b);
    free(path_a);
    free(path_b);
    return report_failure("read second sink file");
  }
  if (!collect_records(text_b, &records_b)) {
    free(text_b);
    free_record_list(&records_a);
    free(text_a);
    delete_file_utf8(path_a);
    delete_file_utf8(path_b);
    free(path_a);
    free(path_b);
    return report_failure("parse second sink file");
  }
  if (records_b.count != 1u) {
    free_record_list(&records_b);
    free(text_b);
    free_record_list(&records_a);
    free(text_a);
    delete_file_utf8(path_a);
    delete_file_utf8(path_b);
    free(path_a);
    free(path_b);
    return report_failure("second sink record count");
  }
  if (records_b.data[0].seq <= last_seq_a) {
    free_record_list(&records_b);
    free(text_b);
    free_record_list(&records_a);
    free(text_a);
    delete_file_utf8(path_a);
    delete_file_utf8(path_b);
    free(path_a);
    free(path_b);
    return report_failure("sequence reset across sink change");
  }
  if (!every_tid_matches(&records_b, (uint64_t)current_tid)) {
    free_record_list(&records_b);
    free(text_b);
    free_record_list(&records_a);
    free(text_a);
    delete_file_utf8(path_a);
    delete_file_utf8(path_b);
    free(path_a);
    free(path_b);
    return report_failure("second sink tid");
  }
  if (!record_line_contains(&records_b, "label%3Dfile-b", 1u)) {
    free_record_list(&records_b);
    free(text_b);
    free_record_list(&records_a);
    free(text_a);
    delete_file_utf8(path_a);
    delete_file_utf8(path_b);
    free(path_a);
    free(path_b);
    return report_failure("second sink label");
  }

  free_record_list(&records_b);
  free(text_b);
  free_record_list(&records_a);
  free(text_a);
  delete_file_utf8(path_a);
  delete_file_utf8(path_b);
  free(path_a);
  free(path_b);
  return 0;
}

static int test_multithreaded_sequence_order(void) {
  char* path = make_temp_file_utf8(L"c14");
  char* text = NULL;
  Task104RecordList records;
  HANDLE threads[TASK104_THREAD_COUNT];
  Task104ThreadArgs args[TASK104_THREAD_COUNT];
  DWORD tids[TASK104_THREAD_COUNT];
  DWORD waits;
  size_t i;

  memset(&records, 0, sizeof(records));
  memset(threads, 0, sizeof(threads));
  memset(args, 0, sizeof(args));
  memset(tids, 0, sizeof(tids));

  if (!path) {
    return report_failure("multithread sink path");
  }

  cursive_x3a_x3aruntime_x3a_x3aconformance_x3a_x3aset_x5fsink(
      1u, (const uint8_t*)path, (uint64_t)strlen(path));

  for (i = 0u; i < TASK104_THREAD_COUNT; ++i) {
    args[i].thread_index = (uint32_t)i;
    args[i].emit_count = TASK104_RECORDS_PER_THREAD;
    threads[i] = CreateThread(NULL, 0u, task104_emit_thread, &args[i], 0u, NULL);
    if (!threads[i]) {
      size_t j;
      for (j = 0u; j < i; ++j) {
        if (threads[j]) {
          WaitForSingleObject(threads[j], INFINITE);
          CloseHandle(threads[j]);
        }
      }
      delete_file_utf8(path);
      free(path);
      return report_failure("thread creation");
    }
  }

  waits = WaitForMultipleObjects(TASK104_THREAD_COUNT, threads, TRUE, INFINITE);
  if (waits == WAIT_FAILED) {
    size_t j;
    for (j = 0u; j < TASK104_THREAD_COUNT; ++j) {
      CloseHandle(threads[j]);
    }
    delete_file_utf8(path);
    free(path);
    return report_failure("thread wait");
  }

  for (i = 0u; i < TASK104_THREAD_COUNT; ++i) {
    DWORD exit_code = 0u;
    if (!GetExitCodeThread(threads[i], &exit_code) || exit_code != 0u) {
      size_t j;
      for (j = 0u; j < TASK104_THREAD_COUNT; ++j) {
        CloseHandle(threads[j]);
      }
      delete_file_utf8(path);
      free(path);
      return report_failure("thread exit");
    }
    tids[i] = args[i].thread_id;
    CloseHandle(threads[i]);
  }

  text = read_file_utf8(path);
  if (!text) {
    delete_file_utf8(path);
    free(path);
    return report_failure("read multithread sink file");
  }
  if (!collect_records(text, &records)) {
    free(text);
    delete_file_utf8(path);
    free(path);
    return report_failure("parse multithread sink file");
  }
  if (records.count != (size_t)(TASK104_THREAD_COUNT * TASK104_RECORDS_PER_THREAD)) {
    free_record_list(&records);
    free(text);
    delete_file_utf8(path);
    free(path);
    return report_failure("multithread record count");
  }
  if (!sequence_is_strictly_ascending(&records)) {
    free_record_list(&records);
    free(text);
    delete_file_utf8(path);
    free(path);
    return report_failure("multithread ascending sequence");
  }
  if (!every_tid_known(&records, tids, TASK104_THREAD_COUNT)) {
    free_record_list(&records);
    free(text);
    delete_file_utf8(path);
    free(path);
    return report_failure("multithread known tids");
  }
  if (!record_line_contains(&records, "label%3Dthread-", TASK104_THREAD_COUNT)) {
    free_record_list(&records);
    free(text);
    delete_file_utf8(path);
    free(path);
    return report_failure("multithread labels");
  }

  free_record_list(&records);
  free(text);
  delete_file_utf8(path);
  free(path);
  return 0;
}

static int test_console_sink_metadata(void) {
  char* capture_path = make_temp_file_utf8(L"c14");
  HANDLE saved_stderr = GetStdHandle(STD_ERROR_HANDLE);
  HANDLE capture = INVALID_HANDLE_VALUE;
  char* text = NULL;
  int needed;
  wchar_t* wide_path = NULL;
  Task104RecordList records;
  C0StringView rule = sv_from_cstr("Log-Console");
  C0StringView file = sv_from_cstr("task104_runtime_console");
  C0StringView payload = sv_from_cstr("category=log;level=info;label=console");
  DWORD current_tid = GetCurrentThreadId();

  memset(&records, 0, sizeof(records));

  if (!capture_path) {
    return report_failure("console capture path");
  }

  needed = MultiByteToWideChar(CP_UTF8, 0, capture_path, -1, NULL, 0);
  if (needed <= 0) {
    free(capture_path);
    return report_failure("console capture path conversion");
  }
  wide_path = (wchar_t*)malloc(sizeof(wchar_t) * (size_t)needed);
  if (!wide_path) {
    free(capture_path);
    return report_failure("console capture wide allocation");
  }
  if (MultiByteToWideChar(CP_UTF8, 0, capture_path, -1, wide_path, needed) <= 0) {
    free(wide_path);
    free(capture_path);
    return report_failure("console capture wide conversion");
  }

  capture = CreateFileW(wide_path,
                        GENERIC_WRITE | GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
  free(wide_path);
  if (capture == INVALID_HANDLE_VALUE) {
    free(capture_path);
    return report_failure("console capture open");
  }

  if (!SetStdHandle(STD_ERROR_HANDLE, capture)) {
    CloseHandle(capture);
    free(capture_path);
    return report_failure("console redirect stderr");
  }

  cursive_x3a_x3aruntime_x3a_x3aconformance_x3a_x3aset_x5fsink(0u, NULL, 0u);
  cursive_x3a_x3aruntime_x3a_x3aconformance_x3a_x3aemit(
      &rule, &file, 20u, 1u, 20u, 4u, &payload);
  FlushFileBuffers(capture);

  if (!SetStdHandle(STD_ERROR_HANDLE, saved_stderr)) {
    CloseHandle(capture);
    delete_file_utf8(capture_path);
    free(capture_path);
    return report_failure("console restore stderr");
  }
  CloseHandle(capture);

  text = read_file_utf8(capture_path);
  if (!text) {
    delete_file_utf8(capture_path);
    free(capture_path);
    return report_failure("read console capture");
  }
  if (!collect_records(text, &records)) {
    free(text);
    delete_file_utf8(capture_path);
    free(capture_path);
    return report_failure("parse console capture");
  }
  if (records.count != 1u) {
    free_record_list(&records);
    free(text);
    delete_file_utf8(capture_path);
    free(capture_path);
    return report_failure("console record count");
  }
  if (!every_tid_matches(&records, (uint64_t)current_tid)) {
    free_record_list(&records);
    free(text);
    delete_file_utf8(capture_path);
    free(capture_path);
    return report_failure("console tid");
  }
  if (!record_line_contains(&records, "console", 1u)) {
    free_record_list(&records);
    free(text);
    delete_file_utf8(capture_path);
    free(capture_path);
    return report_failure("console label");
  }

  free_record_list(&records);
  free(text);
  delete_file_utf8(capture_path);
  free(capture_path);
  return 0;
}

int main(void) {
  if (test_file_sequence_and_no_reset() != 0) {
    return 1;
  }
  if (test_multithreaded_sequence_order() != 0) {
    return 1;
  }
  if (test_console_sink_metadata() != 0) {
    return 1;
  }
  return 0;
}
