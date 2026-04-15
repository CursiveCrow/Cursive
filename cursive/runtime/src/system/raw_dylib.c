#include "../internal/rt_internal.h"

void* cursive_raw_dylib_resolve(const char* dll_name,
                                const char* symbol_name) {
  if (!dll_name || !symbol_name || dll_name[0] == '\0' ||
      symbol_name[0] == '\0') {
    return NULL;
  }

  HMODULE module = GetModuleHandleA(dll_name);
  if (module == NULL) {
    module = LoadLibraryA(dll_name);
  }
  if (module == NULL) {
    return NULL;
  }

  return (void*)GetProcAddress(module, symbol_name);
}
