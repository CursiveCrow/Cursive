#pragma once

#include <vector>

#include "cursive0/core/diagnostics.h"
#include "cursive0/core/source_text.h"
#include "cursive0/syntax/ast.h"
#include "cursive0/syntax/token.h"

namespace cursive0::analysis {

struct PhaseOrderResult {
  bool phase1_ok = false;
  bool phase3_ok = false;
  bool phase4_ok = false;
};

struct SubsetResult {
  bool subset_ok = true;
  cursive0::core::DiagnosticStream diags;
};

struct ConformanceInput {
  PhaseOrderResult phase_orders;
  bool subset_ok = false;
};

bool WF(const PhaseOrderResult& phases);

bool C0Conforming(const ConformanceInput& input);

bool RejectIllFormed(const ConformanceInput& input);

SubsetResult CheckC0SubsetPermTokens(const std::vector<syntax::Token>& tokens);
SubsetResult CheckC0UnwindAttrUnsupportedTokens(const std::vector<syntax::Token>& tokens);
SubsetResult CheckC0AttrSyntaxUnsupportedTokens(const std::vector<syntax::Token>& tokens);
SubsetResult CheckC0UnsupportedConstructsTokens(const std::vector<syntax::Token>& tokens);


SubsetResult CheckC0SubsetPermSyntax(const core::SourceFile& source);

SubsetResult CheckC0UnwindAttrUnsupported(const core::SourceFile& source);
SubsetResult CheckC0AttrSyntaxUnsupported(const core::SourceFile& source);
SubsetResult CheckC0UnsupportedConstructs(const core::SourceFile& source);
SubsetResult CheckC0UnsupportedFormsAst(const syntax::ASTFile& ast,
                                        const core::SourceFile& source);

}  // namespace cursive0::analysis
