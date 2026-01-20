#include <stdint.h>

int _RTC_InitBase(void) {
  return 0;
}

int _RTC_Shutdown(void) {
  return 0;
}

void _RTC_CheckStackVars(void* frame, void* desc) {
  (void)frame;
  (void)desc;
}

void _RTC_CheckStackVars2(void* frame, void* desc) {
  (void)frame;
  (void)desc;
}
