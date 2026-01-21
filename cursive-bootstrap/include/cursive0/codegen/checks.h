#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "cursive0/codegen/ir_model.h"
#include "cursive0/analysis/types/types.h"
#include "cursive0/syntax/ast.h"

namespace cursive0::codegen {

// Forward declarations for types defined in lower_expr.h
struct LowerCtx;
struct LowerResult;

// §6.8 PanicReason enumeration with panic codes
enum class PanicReason {
  ErrorExpr,     // 0x0001
  ErrorStmt,     // 0x0002
  DivZero,       // 0x0003
  Overflow,      // 0x0004
  Shift,         // 0x0005
  Bounds,        // 0x0006
  Cast,          // 0x0007
  NullDeref,     // 0x0008
  ExpiredDeref,  // 0x0009
  InitPanic,     // 0x000A
  Other,         // 0x00FF
};

// Returns the panic code for a given PanicReason per §6.8
std::uint16_t PanicCode(PanicReason reason);

// Returns the reason string for IR representation
std::string PanicReasonString(PanicReason reason);

// §6.11 Range value representation
struct RangeVal {
  syntax::RangeKind kind;
  std::optional<IRValue> lo;
  std::optional<IRValue> hi;
};

// §6.11 LowerRangeExpr result
struct LowerRangeResult {
  IRPtr ir;
  RangeVal value;
};

// Forward declaration - LowerResult is defined in lower_expr.h
struct LowerResult;

// §6.11 Check-Index-Ok / Check-Index-Err
// Returns true if index is within bounds [0, len)
bool CheckIndex(std::uint64_t len, std::uint64_t idx);

// §6.11 Check-Range-Ok / Check-Range-Err
// Returns true if range bounds are valid for the given length
bool CheckRange(std::uint64_t len, const RangeVal& range);

// §6.11 SliceBounds - compute (start, end) from a range and length
// Returns nullopt if range is out of bounds
std::optional<std::pair<std::uint64_t, std::uint64_t>> SliceBounds(
    const RangeVal& range, std::uint64_t len);

// §6.11 Lower-Range-* rules
// Lowers a range expression to IR and a RangeVal
LowerRangeResult LowerRangeExpr(const syntax::RangeExpr& expr, LowerCtx& ctx);

// §6.11 Lower-Transmute / Lower-Transmute-Err
// Lowers a transmute expression, checking size compatibility
LowerResult LowerTransmute(analysis::TypeRef from_type,
                           analysis::TypeRef to_type,
                           const syntax::Expr& expr,
                           LowerCtx& ctx);

// §6.11 Lower-RawDeref-Safe/Raw/Null/Expired
// Lowers a raw pointer dereference with appropriate checks
LowerResult LowerRawDeref(const IRValue& ptr_value,
                          analysis::TypeRef ptr_type,
                          LowerCtx& ctx);

// §6.8 LowerPanic - emit panic IR for a given reason
IRPtr LowerPanic(PanicReason reason, LowerCtx& ctx);

// §6.8 PanicSym - the runtime panic handler symbol
std::string PanicSym();

// §6.8 ClearPanic - emit IR to clear the panic flag
IRPtr ClearPanic(LowerCtx& ctx);

// §6.8 PanicCheck - emit IR to check if panic occurred after a call
IRPtr PanicCheck(LowerCtx& ctx);

// §6.8 InitPanicHandle - emit IR for module init panic handling
IRPtr InitPanicHandle(const std::string& module_path, LowerCtx& ctx);

// Emit SPEC_RULE anchors for §6.11 and §6.8 rules
void AnchorChecksAndPanicRules();

}  // namespace cursive0::codegen
