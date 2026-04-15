#include "../internal/rt_internal.h"

static C0StringView c0_empty_string_view(void) {
  C0StringView out;
  out.data = NULL;
  out.len = 0;
  return out;
}

static C0StringView c0_system_get_env_none(void) {
  c0_trace_emit_rule("System-GetEnv-None");
  return c0_empty_string_view();
}

void cursive_x3a_x3aruntime_x3a_x3asystem_x3a_x3aexit(int32_t code) {
  c0_trace_emit_rule("System-Exit");
  c0_trace_emit_rule("Prim-System-Exit");
  ExitProcess((UINT)code);
  for (;;) {
  }
}

C0StringView cursive_x3a_x3aruntime_x3a_x3asystem_x3a_x3aget_x5fenv(
    const C0StringView* key) {
  if (!key || !key->data || key->len == 0) {
    return c0_system_get_env_none();
  }

  uint32_t key_wide_len = 0;
  wchar_t* key_wide = c0_utf8_to_wide(key->data, key->len, &key_wide_len);
  if (!key_wide) {
    c0_trace_emit_rule("Prim-System-GetEnv");
    return c0_empty_string_view();
  }

  SetLastError(ERROR_SUCCESS);
  DWORD required = GetEnvironmentVariableW(key_wide, NULL, 0);
  if (required == 0) {
    DWORD env_error = GetLastError();
    c0_heap_free_raw(key_wide);
    if (env_error == ERROR_ENVVAR_NOT_FOUND) {
      return c0_system_get_env_none();
    }
    if (env_error == ERROR_SUCCESS) {
      c0_trace_emit_rule("System-GetEnv-Ok");
      return c0_empty_string_view();
    }
    c0_trace_emit_rule("Prim-System-GetEnv");
    return c0_empty_string_view();
  }

  wchar_t* value_wide = (wchar_t*)c0_heap_alloc_raw(
      ((size_t)required) * sizeof(wchar_t));
  if (!value_wide) {
    c0_heap_free_raw(key_wide);
    return c0_empty_string_view();
  }

  DWORD written = GetEnvironmentVariableW(key_wide, value_wide, required);
  c0_heap_free_raw(key_wide);
  if (written == 0 || written >= required) {
    c0_trace_emit_rule("Prim-System-GetEnv");
    c0_heap_free_raw(value_wide);
    return c0_empty_string_view();
  }

  uint32_t value_utf8_len = 0;
  uint8_t* value_utf8 = c0_wide_to_utf8(value_wide, (uint32_t)written, &value_utf8_len);
  c0_heap_free_raw(value_wide);
  if (!value_utf8) {
    c0_trace_emit_rule("Prim-System-GetEnv");
    return c0_empty_string_view();
  }

  C0StringView out;
  out.data = value_utf8;
  out.len = value_utf8_len;
  c0_trace_emit_rule("System-GetEnv-Ok");
  return out;
}

int32_t cursive_x3a_x3aruntime_x3a_x3asystem_x3a_x3arun(
    const C0StringView* command) {
  if (!command || !command->data || command->len == 0) {
    return -1;
  }

  uint32_t command_wide_len = 0;
  wchar_t* command_wide =
      c0_utf8_to_wide(command->data, command->len, &command_wide_len);
  if (!command_wide) {
    return -1;
  }

  STARTUPINFOW si;
  PROCESS_INFORMATION pi;
  c0_memset(&si, 0, sizeof(si));
  c0_memset(&pi, 0, sizeof(pi));
  si.cb = sizeof(si);

  BOOL ok = CreateProcessW(
      NULL,
      command_wide,
      NULL,
      NULL,
      FALSE,
      0,
      NULL,
      NULL,
      &si,
      &pi);
  c0_heap_free_raw(command_wide);
  if (!ok) {
    return -1;
  }

  DWORD wait_result = WaitForSingleObject(pi.hProcess, INFINITE);
  DWORD exit_code = 0;
  BOOL got_exit_code = GetExitCodeProcess(pi.hProcess, &exit_code);

  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);

  if (wait_result != WAIT_OBJECT_0 || !got_exit_code) {
    return -1;
  }

  c0_trace_emit_rule("System-Run");
  c0_trace_emit_rule("Prim-System-Run");
  return (int32_t)exit_code;
}
