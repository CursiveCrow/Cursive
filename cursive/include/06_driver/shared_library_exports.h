#pragma once

#include <optional>

#include "01_project/outputs.h"
#include "06_driver/pipeline.h"

namespace cursive::driver {

std::optional<project::SharedLibraryExports> ResolveSharedLibraryExports(
    const project::Project& project,
    const CodegenCache& cache);

bool PrepareSharedLibraryCodegenContext(
    const project::Project& project,
    CodegenCache& cache,
    const project::SharedLibraryExports& exports);

}  // namespace cursive::driver
