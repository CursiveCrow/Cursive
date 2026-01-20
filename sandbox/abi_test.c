#include <stdint.h>

typedef struct { uint8_t disc; uint8_t payload; } U;

__declspec(noinline) U ret_u(void) {
  U u;
  u.disc = 0;
  u.payload = 1;
  return u;
}

__declspec(noinline) void caller(void) {
  U u = ret_u();
  volatile uint8_t d = u.disc;
  volatile uint8_t p = u.payload;
  (void)d; (void)p;
}
