#include "cursive0/eval/control.h"

namespace cursive0::eval {

Outcome MakeVal(Value value) {
  Outcome out;
  out.node = std::move(value);
  return out;
}

Outcome MakeCtrl(Control ctrl) {
  Outcome out;
  out.node = std::move(ctrl);
  return out;
}

StmtOut MakeOk() {
  StmtOut out;
  out.node = std::monostate{};
  return out;
}

StmtOut MakeCtrlOut(Control ctrl) {
  StmtOut out;
  out.node = std::move(ctrl);
  return out;
}

bool IsVal(const Outcome& out) {
  return std::holds_alternative<Value>(out.node);
}

bool IsCtrl(const Outcome& out) {
  return std::holds_alternative<Control>(out.node);
}

Value BreakVal(const std::optional<Value>& value_opt) {
  if (value_opt.has_value()) {
    return *value_opt;
  }
  return Value{UnitVal{}};
}

std::string ControlToString(const Control& ctrl) {
  std::string kind;
  switch (ctrl.kind) {
    case ControlKind::Return:
      kind = "return";
      break;
    case ControlKind::Result:
      kind = "result";
      break;
    case ControlKind::Break:
      kind = "break";
      break;
    case ControlKind::Continue:
      kind = "continue";
      break;
    case ControlKind::Panic:
      kind = "panic";
      break;
    case ControlKind::Abort:
      kind = "abort";
      break;
  }
  if (ctrl.value.has_value()) {
    return kind + "(" + ValueToString(*ctrl.value) + ")";
  }
  return kind;
}

}  // namespace cursive0::eval
