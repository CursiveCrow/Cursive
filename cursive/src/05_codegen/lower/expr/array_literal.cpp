// =============================================================================
// MIGRATION MAPPING: expr/array_literal.cpp
// =============================================================================
//
// SPEC REFERENCE: CursiveSpecification.md Section 6.4 (Expression Lowering)
//   - Lines 16080-16083: (Lower-Expr-Array)
//     Gamma |- LowerList(es) => <IR, vec_v>
//     ------------------------------------------
//     Gamma |- LowerExpr(ArrayExpr(es)) => <IR, [v_1, ..., v_n]>
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/lower/lower_expr_core.cpp
//   - ArrayExpr visitor lowers each element in order via LowerList
//   - ArrayRepeatExpr visitor lowers value and count expressions
//   - Creates DerivedValueInfo of kind ArrayLit or ArrayRepeat
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/ir/ir_model.h (IRValue, IRPtr)
//   - cursive/include/05_codegen/lower/lower_expr.h (LowerCtx, LowerResult, LowerList)
//
// IMPLEMENTATION NOTES:
//   1. Array literals lower each element expression left-to-right via LowerList
//   2. Array repeat expressions lower value and count, storing them in DerivedValueInfo
//   3. The resulting IRValue is a synthetic temp representing the array aggregate
//   4. The elements/repeat info are stored in a DerivedValueInfo
//   5. Materialization happens when the array value is used (stored, returned, etc.)
//
// =============================================================================

#include "05_codegen/lower/expr/array_literal.h"
#include "00_core/assert_spec.h"
#include "04_analysis/typing/type_equiv.h"

namespace cursive::codegen {

// =============================================================================
// LowerArrayLiteral - Lower an array literal expression to IR
// =============================================================================
// SPEC: (Lower-Expr-Array)
//   Gamma |- LowerList(es) => <IR, vec_v>
//   ------------------------------------------
//   Gamma |- LowerExpr(ArrayExpr(es)) => <IR, [v_1, ..., v_n]>
//
// Array literal expressions lower their element expressions left-to-right,
// then produce a synthetic array value that tracks the element values via the
// DerivedValueInfo mechanism. The actual array aggregate is materialized
// when the value is stored or otherwise consumed.
// =============================================================================

LowerResult LowerArrayLiteral(const ast::ArrayExpr& expr, LowerCtx& ctx) {
    SPEC_RULE("Lower-Expr-Array");

    // Lower all element expressions in left-to-right order
    auto [ir, values] = LowerList(expr.elements, ctx);

    // Create a synthetic value to represent the array
    IRValue array_value = ctx.FreshTempValue("array");

    // Register the derived value info so materialization can access elements
    DerivedValueInfo info;
    info.kind = DerivedValueInfo::Kind::ArrayLit;
    info.elements = values;
    ctx.RegisterDerivedValue(array_value, info);

    // Preserve the concrete array type at the array-literal definition site.
    // This keeps LLVM materialization on the typed array path instead of
    // re-inferring an aggregate shape from already-materialized LLVM values.
    analysis::TypeRef element_type;
    bool all_typed = !values.empty();
    for (const auto& value : values) {
        analysis::TypeRef current_type = ctx.LookupValueType(value);
        if (!current_type) {
            all_typed = false;
            break;
        }
        if (!element_type) {
            element_type = current_type;
            continue;
        }
        const auto equiv = analysis::TypeEquiv(element_type, current_type);
        if (!equiv.ok || !equiv.equiv) {
            all_typed = false;
            break;
        }
    }
    if (all_typed && element_type) {
        ctx.RegisterValueType(
            array_value,
            analysis::MakeTypeArray(
                element_type,
                static_cast<std::uint64_t>(values.size())));
    }

    return LowerResult{ir, array_value};
}

// =============================================================================
// LowerArrayRepeat - Lower an array repeat expression to IR
// =============================================================================
// SPEC: (Lower-Expr-ArrayRepeat)
//   Gamma |- LowerExpr(value) => <IR_v, v>
//   Gamma |- LowerExpr(count) => <IR_c, c>
//   ------------------------------------------
//   Gamma |- LowerExpr(ArrayRepeatExpr(value, count)) => <SeqIR(IR_v, IR_c), [v; c]>
//
// Array repeat expressions lower the value expression and count expression,
// then produce a synthetic array value that tracks these via DerivedValueInfo
// with kind ArrayRepeat. The actual array is materialized when used.
// =============================================================================

LowerResult LowerArrayRepeat(const ast::ArrayRepeatExpr& expr, LowerCtx& ctx) {
    SPEC_RULE("Lower-Expr-ArrayRepeat");

    // Lower value and count expressions
    auto value_result = LowerExpr(*expr.value, ctx);
    auto count_result = LowerExpr(*expr.count, ctx);

    // Create a synthetic value to represent the array
    IRValue array_value = ctx.FreshTempValue("array_repeat");

    // Register the derived value info with repeat value and count
    DerivedValueInfo info;
    info.kind = DerivedValueInfo::Kind::ArrayRepeat;
    info.repeat_value = value_result.value;
    info.repeat_count = count_result.value;
    ctx.RegisterDerivedValue(array_value, info);

    return LowerResult{SeqIR({value_result.ir, count_result.ir}), array_value};
}

}  // namespace cursive::codegen
