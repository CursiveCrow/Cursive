#include "cursive0/04_codegen/ir_model.h"

#include <algorithm>
#include <type_traits>

#include "cursive0/00_core/assert_spec.h"

namespace cursive0::codegen {
namespace {

bool ValidateValue(const IRValue& value) {
  switch (value.kind) {
    case IRValue::Kind::Local:
    case IRValue::Kind::Symbol:
      return !value.name.empty();
    case IRValue::Kind::Opaque:
    case IRValue::Kind::Immediate:
      return true;
  }
  return false;
}

bool ValidatePlace(const IRPlace& place) {
  return !place.repr.empty();
}


bool ValidateRange(const IRRange& range) {
  if (range.lo.has_value() && !ValidateValue(*range.lo)) {
    return false;
  }
  if (range.hi.has_value() && !ValidateValue(*range.hi)) {
    return false;
  }
  return true;
}

bool ValidateIRPtr(const IRPtr& ptr) {
  if (!ptr) {
    return false;
  }
  return ValidateIR(*ptr);
}

bool ValidateMatchArm(const IRMatchArm& arm) {
  if (!arm.pattern || !arm.body) {
    return false;
  }
  if (!ValidateIRPtr(arm.body)) {
    return false;
  }
  return ValidateValue(arm.value);
}

bool ValidateIncoming(const IRIncoming& incoming) {
  if (incoming.label.empty()) {
    return false;
  }
  return ValidateValue(incoming.value);
}

}  // namespace

bool ValidateIR(const IR& ir) {
  return std::visit(
      [&](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, IROpaque>) {
          return true;
        } else if constexpr (std::is_same_v<T, IRSeq>) {
          for (const auto& item : node.items) {
            if (!ValidateIRPtr(item)) {
              return false;
            }
          }
          return true;
        } else if constexpr (std::is_same_v<T, IRCall>) {
          if (!ValidateValue(node.callee)) {
            return false;
          }
          for (const auto& arg : node.args) {
            if (!ValidateValue(arg)) {
              return false;
            }
          }
          return ValidateValue(node.result);
        } else if constexpr (std::is_same_v<T, IRCallVTable>) {
          if (!ValidateValue(node.base)) {
            return false;
          }
          for (const auto& arg : node.args) {
            if (!ValidateValue(arg)) {
              return false;
            }
          }
          return ValidateValue(node.result);
        } else if constexpr (std::is_same_v<T, IRStoreGlobal>) {
          return !node.symbol.empty() && ValidateValue(node.value);
        } else if constexpr (std::is_same_v<T, IRReadVar>) {
          return !node.name.empty();
        } else if constexpr (std::is_same_v<T, IRReadPath>) {
          if (node.name.empty()) {
            return false;
          }
          for (const auto& part : node.path) {
            if (part.empty()) {
              return false;
            }
          }
          return true;
        } else if constexpr (std::is_same_v<T, IRBindVar>) {
          return !node.name.empty() && ValidateValue(node.value);
        } else if constexpr (std::is_same_v<T, IRStoreVar>) {
          return !node.name.empty() && ValidateValue(node.value);
        } else if constexpr (std::is_same_v<T, IRStoreVarNoDrop>) {
          return !node.name.empty() && ValidateValue(node.value);
        } else if constexpr (std::is_same_v<T, IRReadPlace>) {
          return ValidatePlace(node.place);
        } else if constexpr (std::is_same_v<T, IRWritePlace>) {
          return ValidatePlace(node.place) && ValidateValue(node.value);
        } else if constexpr (std::is_same_v<T, IRAddrOf>) {
          return ValidatePlace(node.place) && ValidateValue(node.result);
        } else if constexpr (std::is_same_v<T, IRReadPtr>) {
          return ValidateValue(node.ptr) && ValidateValue(node.result);
        } else if constexpr (std::is_same_v<T, IRWritePtr>) {
          return ValidateValue(node.ptr) && ValidateValue(node.value);
        } else if constexpr (std::is_same_v<T, IRUnaryOp>) {
          return !node.op.empty() && ValidateValue(node.operand) &&
                 ValidateValue(node.result);
        } else if constexpr (std::is_same_v<T, IRBinaryOp>) {
          return !node.op.empty() && ValidateValue(node.lhs) &&
                 ValidateValue(node.rhs) && ValidateValue(node.result);
        } else if constexpr (std::is_same_v<T, IRCast>) {
          return node.target != nullptr && ValidateValue(node.value) &&
                 ValidateValue(node.result);
        } else if constexpr (std::is_same_v<T, IRTransmute>) {
          return node.from != nullptr && node.to != nullptr &&
                 ValidateValue(node.value) && ValidateValue(node.result);
        } else if constexpr (std::is_same_v<T, IRCheckIndex>) {
          return ValidateValue(node.base) && ValidateValue(node.index);
        } else if constexpr (std::is_same_v<T, IRCheckRange>) {
          return ValidateValue(node.base) && ValidateRange(node.range);
        } else if constexpr (std::is_same_v<T, IRCheckSliceLen>) {
          return ValidateValue(node.base) && ValidateRange(node.range) &&
                 ValidateValue(node.value);
        } else if constexpr (std::is_same_v<T, IRCheckOp>) {
          if (node.op.empty() || node.reason.empty()) {
            return false;
          }
          if (!ValidateValue(node.lhs)) {
            return false;
          }
          if (node.rhs.has_value() && !ValidateValue(*node.rhs)) {
            return false;
          }
          return true;
        } else if constexpr (std::is_same_v<T, IRCheckCast>) {
          return node.target != nullptr && ValidateValue(node.value);
        } else if constexpr (std::is_same_v<T, IRAlloc>) {
          if (node.region.has_value() && !ValidateValue(*node.region)) {
            return false;
          }
          return ValidateValue(node.value) && ValidateValue(node.result);
        } else if constexpr (std::is_same_v<T, IRReturn>) {
          return ValidateValue(node.value);
        } else if constexpr (std::is_same_v<T, IRResult>) {
          return ValidateValue(node.value);
        } else if constexpr (std::is_same_v<T, IRBreak>) {
          return !node.value.has_value() || ValidateValue(*node.value);
        } else if constexpr (std::is_same_v<T, IRContinue>) {
          return true;
        } else if constexpr (std::is_same_v<T, IRDefer>) {
          return node.block != nullptr;
        } else if constexpr (std::is_same_v<T, IRMoveState>) {
          return ValidatePlace(node.place);
        } else if constexpr (std::is_same_v<T, IRIf>) {
          return ValidateValue(node.cond) && ValidateIRPtr(node.then_ir) &&
                 ValidateValue(node.then_value) && ValidateIRPtr(node.else_ir) &&
                 ValidateValue(node.else_value) && ValidateValue(node.result);
        } else if constexpr (std::is_same_v<T, IRBlock>) {
          return ValidateIRPtr(node.setup) && ValidateIRPtr(node.body) &&
                 ValidateValue(node.value);
        } else if constexpr (std::is_same_v<T, IRLoop>) {
          if (!node.body_ir) {
            return false;
          }
          if (!ValidateIRPtr(node.body_ir) || !ValidateValue(node.body_value) ||
              !ValidateValue(node.result)) {
            return false;
          }
          switch (node.kind) {
            case IRLoopKind::Infinite:
              return true;
            case IRLoopKind::Conditional:
              if (!node.cond_ir || !node.cond_value.has_value()) {
                return false;
              }
              return ValidateIRPtr(node.cond_ir) &&
                     ValidateValue(*node.cond_value);
            case IRLoopKind::Iter:
              if (!node.pattern || !node.iter_ir ||
                  !node.iter_value.has_value()) {
                return false;
              }
              return ValidateIRPtr(node.iter_ir) &&
                     ValidateValue(*node.iter_value);
          }
          return false;
        } else if constexpr (std::is_same_v<T, IRMatch>) {
          if (!ValidateValue(node.scrutinee)) {
            return false;
          }
          for (const auto& arm : node.arms) {
            if (!ValidateMatchArm(arm)) {
              return false;
            }
          }
          return ValidateValue(node.result);
        } else if constexpr (std::is_same_v<T, IRRegion>) {
          return ValidateValue(node.owner) && ValidateIRPtr(node.body) &&
                 ValidateValue(node.value);
        } else if constexpr (std::is_same_v<T, IRFrame>) {
          if (node.region.has_value() && !ValidateValue(*node.region)) {
            return false;
          }
          return ValidateIRPtr(node.body) && ValidateValue(node.value);
        } else if constexpr (std::is_same_v<T, IRBranch>) {
          if (node.cond.has_value()) {
            return !node.true_label.empty() && !node.false_label.empty() &&
                   ValidateValue(*node.cond);
          }
          return !node.true_label.empty();
        } else if constexpr (std::is_same_v<T, IRPhi>) {
          if (!node.type) {
            return false;
          }
          for (const auto& inc : node.incoming) {
            if (!ValidateIncoming(inc)) {
              return false;
            }
          }
          return ValidateValue(node.value);
        } else if constexpr (std::is_same_v<T, IRClearPanic>) {
          return true;
        } else if constexpr (std::is_same_v<T, IRPanicCheck>) {
          if (node.cleanup_ir) {
            return ValidateIRPtr(node.cleanup_ir);
          }
          return true;
        } else if constexpr (std::is_same_v<T, IRInitPanicHandle>) {
          return true;
        } else if constexpr (std::is_same_v<T, IRCheckPoison>) {
          return true;
        } else if constexpr (std::is_same_v<T, IRLowerPanic>) {
          if (node.cleanup_ir) {
            return ValidateIRPtr(node.cleanup_ir);
          }
          return true;
        } else {
          return false;
        }
      },
      ir.node);
}

bool ValidateDecl(const IRDecl& decl) {
  return std::visit(
      [&](const auto& node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, ProcIR>) {
          if (node.symbol.empty() || !node.ret || !node.body) {
            return false;
          }
          for (const auto& param : node.params) {
            if (param.name.empty() || !param.type) {
              return false;
            }
          }
          return ValidateIR(*node.body);
        } else if constexpr (std::is_same_v<T, GlobalConst>) {
          return !node.symbol.empty();
        } else if constexpr (std::is_same_v<T, GlobalZero>) {
          return !node.symbol.empty();
        } else if constexpr (std::is_same_v<T, GlobalVTable>) {
          if (node.symbol.empty() || node.header.drop_sym.empty()) {
            return false;
          }
          for (const auto& slot : node.slots) {
            if (slot.empty()) {
              return false;
            }
          }
          return true;
        } else {
          return false;
        }
      },
      decl);
}

bool ValidateModuleIR(const IRDecls& decls) {
  for (const auto& decl : decls) {
    if (!ValidateDecl(decl)) {
      return false;
    }
  }
  return true;
}

void AnchorCodegenModelRules() {
  SPEC_RULE("CG-Project");
  SPEC_RULE("CG-Module");
  SPEC_RULE("CG-Item-Using");
  SPEC_RULE("CG-Item-TypeAlias");
  SPEC_RULE("CG-Item-Procedure-Main");
  SPEC_RULE("CG-Item-Procedure");
  SPEC_RULE("CG-Item-Static");
  SPEC_RULE("CG-Item-Record");
  SPEC_RULE("CG-Item-Method");
  SPEC_RULE("CG-Item-Modal");
  SPEC_RULE("CG-Item-StateMethod");
  SPEC_RULE("CG-Item-Transition");
  SPEC_RULE("CG-Item-Class");
  SPEC_RULE("CG-Item-ClassMethod-Abstract");
  SPEC_RULE("CG-Item-ClassMethod-Body");
  SPEC_RULE("CG-Item-Enum");
  SPEC_RULE("CG-Item-ErrorItem");
  SPEC_RULE("CG-Expr");
  SPEC_RULE("CG-Stmt");
  SPEC_RULE("CG-Block");
  SPEC_RULE("CG-Place");
  SPEC_RULE("CG-Place");
}

void AnchorIRExecRules() {
  // §6.4 ExecIR semantics
  SPEC_RULE("ExecIR-Alloc");
  SPEC_RULE("ExecIR-BindVar");
  SPEC_RULE("ExecIR-Block");
  SPEC_RULE("ExecIR-Break");
  SPEC_RULE("ExecIR-Continue");
  SPEC_RULE("ExecIR-Defer");
  SPEC_RULE("ExecIR-Frame-Explicit");
  SPEC_RULE("ExecIR-Frame-Implicit");
  SPEC_RULE("ExecIR-If-False");
  SPEC_RULE("ExecIR-If-True");
  SPEC_RULE("ExecIR-Loop-Cond-Body-Ctrl");
  SPEC_RULE("ExecIR-Loop-Cond-Break");
  SPEC_RULE("ExecIR-Loop-Cond-Continue");
  SPEC_RULE("ExecIR-Loop-Cond-Ctrl");
  SPEC_RULE("ExecIR-Loop-Cond-False");
  SPEC_RULE("ExecIR-Loop-Cond-True-Step");
  SPEC_RULE("ExecIR-Loop-Infinite-Break");
  SPEC_RULE("ExecIR-Loop-Infinite-Continue");
  SPEC_RULE("ExecIR-Loop-Infinite-Ctrl");
  SPEC_RULE("ExecIR-Loop-Infinite-Step");
  SPEC_RULE("ExecIR-Loop-Iter");
  SPEC_RULE("ExecIR-Loop-Iter-Ctrl");
  SPEC_RULE("ExecIR-Match");
  SPEC_RULE("ExecIR-MoveState");
  SPEC_RULE("ExecIR-ReadPath");
  SPEC_RULE("ExecIR-ReadPtr");
  SPEC_RULE("ExecIR-ReadVar");
  SPEC_RULE("ExecIR-Region");
  SPEC_RULE("ExecIR-Result");
  SPEC_RULE("ExecIR-Return");
  SPEC_RULE("ExecIR-StoreVar");
  SPEC_RULE("ExecIR-StoreVarNoDrop");
  SPEC_RULE("ExecIR-WritePtr");
  SPEC_RULE("LoopIterIR-Done");
  SPEC_RULE("LoopIterIR-Step-Break");
  SPEC_RULE("LoopIterIR-Step-Continue");
  SPEC_RULE("LoopIterIR-Step-Ctrl");
  SPEC_RULE("LoopIterIR-Step-Val");
  SPEC_RULE("MoveState-Field");
  SPEC_RULE("MoveState-Root");
}

IRPtr SeqIR(std::vector<IRPtr> items) {
  // Remove null items
  items.erase(std::remove_if(items.begin(), items.end(),
                             [](const IRPtr& p) { return p == nullptr; }),
              items.end());

  if (items.empty()) {
    return std::make_shared<IR>(IR{IROpaque{}});
  }
  if (items.size() == 1) {
    return items[0];
  }
  return std::make_shared<IR>(IR{IRSeq{std::move(items)}});
}

IRPtr SeqIR() {
  return std::make_shared<IR>(IR{IRSeq{{}}});
}

IRPtr SeqIR(IRPtr ir) {
  if (!ir) return SeqIR();
  return ir;
}

IRPtr SeqIR(IRPtr ir1, IRPtr ir2) {
  if (!ir1 && !ir2) return SeqIR();
  if (!ir1) return ir2;
  if (!ir2) return ir1;
  
  auto result = std::make_shared<IR>(IR{IRSeq{{}}});
  auto& seq = std::get<IRSeq>(result->node);
  seq.items.push_back(ir1);
  seq.items.push_back(ir2);
  return result;
}

}  // namespace cursive0::codegen
