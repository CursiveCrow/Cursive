#pragma once

#include <memory>
#include <vector>

#include "02_source/ast/ast.h"
#include "02_source/module_paths.h"

namespace cursive::analysis {

bool ReflectImplementsForm(
    const std::vector<const ast::ASTModule*>& available_modules,
    const source::ModuleNames& available_module_names,
    const ast::ModulePath& current_module,
    const std::shared_ptr<ast::Type>& type_syntax,
    const ast::ClassPath& form_path);

}  // namespace cursive::analysis
