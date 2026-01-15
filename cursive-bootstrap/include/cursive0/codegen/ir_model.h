#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "cursive0/sema/types.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::codegen {

// Internal IR model for Cursive0 codegen.

struct IR;
using IRPtr = std::shared_ptr<IR>;

struct IRValue {
  enum class Kind {
    Opaque,
    Local,
    Symbol,
    Immediate,
  };

  Kind kind = Kind::Opaque;
  std::string name;
  std::vector<std::uint8_t> bytes;
};

struct IRPlace {
  std::string repr;
};

struct IRRange {
  syntax::RangeKind kind = syntax::RangeKind::Full;
  std::optional<IRValue> lo;
  std::optional<IRValue> hi;
};

struct IRSeq {
  std::vector<IRPtr> items;
};

struct IRCall {
  IRValue callee;
  std::vector<IRValue> args;
};

struct IRCallVTable {
  IRValue base;
  std::size_t slot = 0;
  std::vector<IRValue> args;
};

struct IRStoreGlobal {
  std::string symbol;
  IRValue value;
};

struct IRReadVar {
  std::string name;
};

struct IRReadPath {
  std::vector<std::string> path;
  std::string name;
};

struct IRBindVar {
  std::string name;
  IRValue value;
};

struct IRStoreVar {
  std::string name;
  IRValue value;
};

struct IRStoreVarNoDrop {
  std::string name;
  IRValue value;
};

struct IRReadPlace {
  IRPlace place;
};

struct IRWritePlace {
  IRPlace place;
  IRValue value;
};

struct IRAddrOf {
  IRPlace place;
};

struct IRReadPtr {
  IRValue ptr;
};

struct IRWritePtr {
  IRValue ptr;
  IRValue value;
};

struct IRUnaryOp {
  std::string op;
  IRValue operand;
};

struct IRBinaryOp {
  std::string op;
  IRValue lhs;
  IRValue rhs;
};

struct IRCast {
  sema::TypeRef target;
  IRValue value;
};

struct IRTransmute {
  sema::TypeRef from;
  sema::TypeRef to;
  IRValue value;
};

struct IRCheckIndex {
  IRValue base;
  IRValue index;
};

struct IRCheckRange {
  IRValue base;
  IRRange range;
};

struct IRCheckSliceLen {
  IRValue base;
  IRRange range;
  IRValue value;
};

struct IRCheckOp {
  std::string op;
  std::string reason;
  IRValue lhs;
  std::optional<IRValue> rhs;
};

struct IRCheckCast {
  sema::TypeRef target;
  IRValue value;
};

struct IRAlloc {
  std::optional<IRValue> region;
  IRValue value;
};

struct IRReturn {
  IRValue value;
};

struct IRResult {
  IRValue value;
};

struct IRBreak {
  std::optional<IRValue> value;
};

struct IRContinue {};

struct IRDefer {
  std::shared_ptr<cursive0::syntax::Block> block;
};

struct IRMoveState {
  IRPlace place;
};

struct IRIf {
  IRValue cond;
  IRPtr then_ir;
  IRValue then_value;
  IRPtr else_ir;
  IRValue else_value;
};

struct IRBlock {
  IRPtr setup;
  IRPtr body;
  IRValue value;
};

enum class IRLoopKind {
  Infinite,
  Conditional,
  Iter,
};

struct IRLoop {
  IRLoopKind kind = IRLoopKind::Infinite;
  std::shared_ptr<cursive0::syntax::Pattern> pattern;
  std::shared_ptr<cursive0::syntax::Type> type_opt;
  IRPtr iter_ir;
  std::optional<IRValue> iter_value;
  IRPtr cond_ir;
  std::optional<IRValue> cond_value;
  IRPtr body_ir;
  IRValue body_value;
};

struct IRMatchArm {
  std::shared_ptr<cursive0::syntax::Pattern> pattern;
  IRPtr body;
  IRValue value;
};

struct IRMatch {
  IRValue scrutinee;
  std::vector<IRMatchArm> arms;
};

struct IRRegion {
  IRValue owner;
  std::optional<std::string> alias;
  IRPtr body;
  IRValue value;
};

struct IRFrame {
  std::optional<IRValue> region;
  IRPtr body;
  IRValue value;
};

struct IRBranch {
  std::optional<IRValue> cond;
  std::string true_label;
  std::string false_label;
};

struct IRIncoming {
  std::string label;
  IRValue value;
};

struct IRPhi {
  cursive0::sema::TypeRef type;
  std::vector<IRIncoming> incoming;
  IRValue value;
};

struct IRClearPanic {};
struct IRPanicCheck {};

struct IRCheckPoison {
  std::string module;
};

struct IRLowerPanic {
  std::string reason;
};

struct IROpaque {};

struct IR {
  std::variant<IROpaque,
               IRSeq,
               IRCall,
               IRCallVTable,
               IRStoreGlobal,
               IRReadVar,
               IRReadPath,
               IRBindVar,
               IRStoreVar,
               IRStoreVarNoDrop,
               IRReadPlace,
               IRWritePlace,
               IRAddrOf,
               IRReadPtr,
               IRWritePtr,
               IRUnaryOp,
               IRBinaryOp,
               IRCast,
               IRTransmute,
               IRCheckIndex,
               IRCheckRange,
               IRCheckSliceLen,
               IRCheckOp,
               IRCheckCast,
               IRAlloc,
               IRReturn,
               IRResult,
               IRBreak,
               IRContinue,
               IRDefer,
               IRMoveState,
               IRIf,
               IRBlock,
               IRLoop,
               IRMatch,
               IRRegion,
               IRFrame,
               IRBranch,
               IRPhi,
               IRClearPanic,
               IRPanicCheck,
               IRCheckPoison,
               IRLowerPanic>
      node;
};

struct IRParam {
  std::optional<cursive0::sema::ParamMode> mode;
  std::string name;
  cursive0::sema::TypeRef type;
};

struct ProcIR {
  std::string symbol;
  std::vector<IRParam> params;
  cursive0::sema::TypeRef ret;
  IRPtr body;
};

struct GlobalConst {
  std::string symbol;
  std::vector<std::uint8_t> bytes;
};

struct GlobalZero {
  std::string symbol;
  std::uint64_t size = 0;
};

struct VTableHeader {
  std::uint64_t size = 0;
  std::uint64_t align = 1;
  std::string drop_sym;
};

struct GlobalVTable {
  std::string symbol;
  VTableHeader header;
  std::vector<std::string> slots;
};

using IRDecl = std::variant<ProcIR, GlobalConst, GlobalZero, GlobalVTable>;
using IRDecls = std::vector<IRDecl>;

bool ValidateIR(const IR& ir);
bool ValidateDecl(const IRDecl& decl);
bool ValidateModuleIR(const IRDecls& decls);

// Emits SPEC_RULE anchors for §6.0 codegen model rules.
void AnchorCodegenModelRules();

// Emits SPEC_RULE anchors for §6.4 ExecIR semantics rules.
void AnchorIRExecRules();

// Helper to create a sequence of IR nodes (flattens and removes nulls)
IRPtr SeqIR(std::vector<IRPtr> items);
IRPtr SeqIR();
IRPtr SeqIR(IRPtr ir);
IRPtr SeqIR(IRPtr ir1, IRPtr ir2);

}  // namespace cursive0::codegen
