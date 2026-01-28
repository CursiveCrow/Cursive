#pragma once

#include <optional>
#include <variant>

#include "cursive0/eval/value.h"

namespace cursive0::eval {

enum class ControlKind {
  Return,
  Break,
  Continue,
  Panic,
  Abort,
};

struct Control {
  ControlKind kind = ControlKind::Panic;
  std::optional<Value> value;
};

struct Outcome {
  std::variant<Value, Control> node;
};

struct StmtOut {
  std::variant<std::monostate, Control> node;
};

Outcome MakeVal(Value value);
Outcome MakeCtrl(Control ctrl);
StmtOut MakeOk();
StmtOut MakeCtrlOut(Control ctrl);

bool IsVal(const Outcome& out);
bool IsCtrl(const Outcome& out);
Value BreakVal(const std::optional<Value>& value_opt);
std::string ControlToString(const Control& ctrl);

}  // namespace cursive0::eval
