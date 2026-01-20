#include <cassert>
#include <cstdint>
#include <string>

#include "cursive0/core/int128.h"
#include "cursive0/semantics/control.h"
#include "cursive0/semantics/value.h"
#include "cursive0/sema/types.h"

int main() {
  using namespace cursive0::semantics;

  IntVal int_val;
  int_val.type = "i32";
  int_val.negative = false;
  int_val.magnitude = cursive0::core::UInt128FromU64(42);
  Value int_value{int_val};

  Value same_int{int_val};
  assert(ValueEqual(int_value, same_int));

  Value bool_value{BoolVal{true}};
  assert(!ValueEqual(int_value, bool_value));

  assert(ValueToString(int_value) == "i32(42)");

  StringVal str_val;
  str_val.state = cursive0::sema::StringState::View;
  str_val.bytes = {0x48, 0x69};
  Value str_value{str_val};
  assert(ValueToString(str_value) == "string@View(4869)");

  Control ctrl;
  ctrl.kind = ControlKind::Return;
  ctrl.value = int_value;
  assert(ControlToString(ctrl) == "return(i32(42))");

  return 0;
}
