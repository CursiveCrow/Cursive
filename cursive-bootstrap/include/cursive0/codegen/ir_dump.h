#pragma once

#include <string>
#include "cursive0/codegen/ir_model.h"

namespace cursive0::codegen {

// DumpIR - Return a string representation of the IR Declarations
std::string DumpIR(const IRDecls& decls);

// DumpIR - Return a string representation of a single IR node
std::string DumpIR(const IRPtr& ir);

}  // namespace cursive0::codegen
