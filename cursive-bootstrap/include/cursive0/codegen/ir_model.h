#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "cursive0/analysis/types/types.h"
#include "cursive0/analysis/memory/regions.h"
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
  IRValue result;
};

struct IRCallVTable {
  IRValue base;
  std::size_t slot = 0;
  std::vector<IRValue> args;
  IRValue result;
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
  analysis::TypeRef type;
  analysis::ProvenanceKind prov = analysis::ProvenanceKind::Bottom;
  std::optional<std::string> prov_region;
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
  IRValue result;
};

struct IRReadPtr {
  IRValue ptr;
  IRValue result;
};

struct IRWritePtr {
  IRValue ptr;
  IRValue value;
};

struct IRUnaryOp {
  std::string op;
  IRValue operand;
  IRValue result;
};

struct IRBinaryOp {
  std::string op;
  IRValue lhs;
  IRValue rhs;
  IRValue result;
};

struct IRCast {
  analysis::TypeRef target;
  IRValue value;
  IRValue result;
};

struct IRTransmute {
  analysis::TypeRef from;
  analysis::TypeRef to;
  IRValue value;
  IRValue result;
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
  analysis::TypeRef target;
  IRValue value;
};

struct IRAlloc {
  std::optional<IRValue> region;
  IRValue value;
  IRValue result;
  analysis::TypeRef type;
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
  IRValue result;
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
  IRValue result;
};

struct IRMatchArm {
  std::shared_ptr<cursive0::syntax::Pattern> pattern;
  IRPtr body;
  IRValue value;
};

struct IRMatch {
  IRValue scrutinee;
  analysis::TypeRef scrutinee_type;
  std::vector<IRMatchArm> arms;
  IRValue result;
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
  cursive0::analysis::TypeRef type;
  std::vector<IRIncoming> incoming;
  IRValue value;
};

struct IRClearPanic {};
struct IRPanicCheck {
  // Optional cleanup IR to run when panic is detected.
  IRPtr cleanup_ir;
};

struct IRInitPanicHandle {
  std::string module;
  std::vector<std::string> poison_modules;
};

struct IRCheckPoison {
  std::string module;
};

struct IRLowerPanic {
  std::string reason;
  // Optional cleanup IR to run before returning.
  IRPtr cleanup_ir;
};

// C0X Extension: Structured Concurrency IR nodes (§11.3)

// §18.1 Parallel block IR
struct IRParallel {
  IRValue domain;                    // $ExecutionDomain
  IRPtr body;                        // Block body
  IRValue result;                    // Result value (tuple of spawn results or unit)
  std::optional<IRValue> cancel_token;  // Optional cancellation token
  std::string name;                  // Optional debug name
};

// §18.4 Spawn expression IR
struct IRSpawn {
  IRPtr captured_env;                // Captured environment setup
  IRPtr body;                        // Spawn body
  IRValue body_result;               // Result value from body execution (T)
  IRValue result;                    // Spawned<T> handle
  IRValue env_ptr;                   // Pointer to captured environment
  IRValue env_size;                  // Size of captured environment (usize)
  IRValue body_fn;                   // Wrapper function symbol
  IRValue result_size;               // Result size (usize)
  std::string name;                  // Optional debug name
};

// §10.3 Wait expression IR
struct IRWait {
  IRValue handle;                    // Spawned<T>
  IRValue result;                    // T (extracted from Spawned)
};

// §18.5 Dispatch expression IR
struct IRDispatch {
  std::shared_ptr<cursive0::syntax::Pattern> pattern;  // Iteration variable
  IRValue range;                     // Range<I>
  IRPtr body;                        // Iteration body
  IRValue body_result;               // Result value from each iteration
  IRPtr captured_env;                // Captured environment setup
  IRValue env_ptr;                   // Pointer to captured environment
  IRValue body_fn;                   // Wrapper function symbol
  IRValue elem_size;                 // Iteration element size (usize)
  IRValue result_size;               // Body result size (usize)
  IRValue result_ptr;                // Pointer to reduction result
  std::optional<std::string> reduce_op;  // Reduction operator if present
  std::optional<IRValue> reduce_fn;  // Custom reducer wrapper if present
  IRValue result;                    // T if reduce, () otherwise
  bool ordered = false;              // [ordered] flag
  std::optional<IRValue> chunk_size; // [chunk:] value
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
               IRInitPanicHandle,
               IRCheckPoison,
               IRLowerPanic,
               // C0X Extension: Structured Concurrency
               IRParallel,
               IRSpawn,
               IRWait,
               IRDispatch>
      node;
};

struct IRParam {
  std::optional<cursive0::analysis::ParamMode> mode;
  std::string name;
  cursive0::analysis::TypeRef type;
};

struct ProcIR {
  std::string symbol;
  std::vector<IRParam> params;
  cursive0::analysis::TypeRef ret;
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

// §6.3.1 External procedure declaration (from extern block)
struct ExternProcIR {
  std::string symbol;
  std::vector<IRParam> params;
  cursive0::analysis::TypeRef ret;
  // Optional ABI specifier (e.g., "C")
  std::optional<std::string> abi;
};

using IRDecl = std::variant<ProcIR, GlobalConst, GlobalZero, GlobalVTable, ExternProcIR>;
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
