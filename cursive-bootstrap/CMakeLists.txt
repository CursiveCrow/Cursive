cmake_minimum_required(VERSION 3.20)

project(cursive LANGUAGES C CXX)

include(CTest)

# T-LLVM-001: Integration
include(cmake/LLVM21.cmake)


option(CURSIVE_USE_BUNDLED_ICU "Use bundled ICU 72.1 from third_party/icu" ON)
option(CURSIVE_ENABLE_FUZZING "Build fuzz targets (libFuzzer)" OFF)

set(CURSIVE_ICU_LIBS "")
if(CURSIVE_USE_BUNDLED_ICU)
  if(WIN32)
    # Windows: Use pre-built MSVC binaries from third_party/icu/win64
    set(ICU_WIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/icu/win64")
    set(ICU_INCLUDE_DIR "${ICU_WIN_DIR}/include")
    set(ICU_LIB_DIR "${ICU_WIN_DIR}/lib64")
    set(ICU_BIN_DIR "${ICU_WIN_DIR}/bin64")

    set(ICU_WIN_PREBUILT TRUE)
    if(NOT EXISTS "${ICU_INCLUDE_DIR}/unicode/utypes.h")
      set(ICU_WIN_PREBUILT FALSE)
    endif()
    if(NOT EXISTS "${ICU_LIB_DIR}/icuuc.lib")
      set(ICU_WIN_PREBUILT FALSE)
    endif()
    if(NOT EXISTS "${ICU_LIB_DIR}/icuin.lib")
      set(ICU_WIN_PREBUILT FALSE)
    endif()
    if(NOT EXISTS "${ICU_LIB_DIR}/icudt.lib")
      set(ICU_WIN_PREBUILT FALSE)
    endif()

    if(NOT ICU_WIN_PREBUILT)
      message(FATAL_ERROR "Bundled ICU Windows binaries not found. Extract icu4c-72_1-Win64-MSVC2019.zip into third_party/icu/win64/")
    endif()

    add_library(ICU::uc SHARED IMPORTED)
    add_library(ICU::i18n SHARED IMPORTED)
    add_library(ICU::data SHARED IMPORTED)
    set_target_properties(ICU::uc PROPERTIES
      IMPORTED_LOCATION "${ICU_BIN_DIR}/icuuc72.dll"
      IMPORTED_IMPLIB "${ICU_LIB_DIR}/icuuc.lib"
      INTERFACE_INCLUDE_DIRECTORIES "${ICU_INCLUDE_DIR}"
    )
    set_target_properties(ICU::i18n PROPERTIES
      IMPORTED_LOCATION "${ICU_BIN_DIR}/icuin72.dll"
      IMPORTED_IMPLIB "${ICU_LIB_DIR}/icuin.lib"
      INTERFACE_INCLUDE_DIRECTORIES "${ICU_INCLUDE_DIR}"
    )
    set_target_properties(ICU::data PROPERTIES
      IMPORTED_LOCATION "${ICU_BIN_DIR}/icudt72.dll"
      IMPORTED_IMPLIB "${ICU_LIB_DIR}/icudt.lib"
      INTERFACE_INCLUDE_DIRECTORIES "${ICU_INCLUDE_DIR}"
    )
    set(CURSIVE_ICU_LIBS ICU::i18n ICU::uc ICU::data)

    # Copy DLLs to output directory for runtime
    set(CURSIVE_ICU_DLLS
      "${ICU_BIN_DIR}/icuuc72.dll"
      "${ICU_BIN_DIR}/icuin72.dll"
      "${ICU_BIN_DIR}/icudt72.dll"
    )
  else()
    # Unix/Linux: Use pre-built static libraries or build from source
    set(ICU_INSTALL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/icu/install")
    set(ICU_INCLUDE_DIR "${ICU_INSTALL_DIR}/include")
    set(ICU_LIB_DIR "${ICU_INSTALL_DIR}/lib")
    set(ICU_PREBUILT TRUE)
    if(NOT EXISTS "${ICU_INCLUDE_DIR}/unicode/utypes.h")
      set(ICU_PREBUILT FALSE)
    endif()
    if(NOT EXISTS "${ICU_LIB_DIR}/libicuuc.a")
      set(ICU_PREBUILT FALSE)
    endif()
    if(NOT EXISTS "${ICU_LIB_DIR}/libicui18n.a")
      set(ICU_PREBUILT FALSE)
    endif()
    if(NOT EXISTS "${ICU_LIB_DIR}/libicudata.a")
      set(ICU_PREBUILT FALSE)
    endif()

    if(ICU_PREBUILT)
      add_library(ICU::uc STATIC IMPORTED)
      add_library(ICU::i18n STATIC IMPORTED)
      add_library(ICU::data STATIC IMPORTED)
      set_target_properties(ICU::uc PROPERTIES
        IMPORTED_LOCATION "${ICU_LIB_DIR}/libicuuc.a"
        INTERFACE_INCLUDE_DIRECTORIES "${ICU_INCLUDE_DIR}"
      )
      set_target_properties(ICU::i18n PROPERTIES
        IMPORTED_LOCATION "${ICU_LIB_DIR}/libicui18n.a"
        INTERFACE_INCLUDE_DIRECTORIES "${ICU_INCLUDE_DIR}"
      )
      set_target_properties(ICU::data PROPERTIES
        IMPORTED_LOCATION "${ICU_LIB_DIR}/libicudata.a"
        INTERFACE_INCLUDE_DIRECTORIES "${ICU_INCLUDE_DIR}"
      )
      set(CURSIVE_ICU_LIBS ICU::i18n ICU::uc ICU::data)
    else()
      include(ExternalProject)
      set(ICU_TARBALL "${CMAKE_CURRENT_SOURCE_DIR}/third_party/icu/icu4c-72_1-src.tgz")
      if(NOT EXISTS "${ICU_TARBALL}")
        message(FATAL_ERROR "Missing bundled ICU tarball: ${ICU_TARBALL}")
      endif()
      file(MAKE_DIRECTORY "${ICU_INSTALL_DIR}/include")
      file(MAKE_DIRECTORY "${ICU_INSTALL_DIR}/lib")
      ExternalProject_Add(icu_external
        URL "${ICU_TARBALL}"
        SOURCE_SUBDIR "icu/source"
        BUILD_IN_SOURCE 1
        CONFIGURE_COMMAND <SOURCE_DIR>/runConfigureICU Linux --disable-tests --disable-samples --disable-extras --disable-icuio --enable-static --disable-shared --with-data-packaging=static --prefix=${ICU_INSTALL_DIR}
        BUILD_COMMAND ${CMAKE_MAKE_PROGRAM}
        INSTALL_COMMAND ${CMAKE_MAKE_PROGRAM} install
      )

      add_library(ICU::uc STATIC IMPORTED)
      add_library(ICU::i18n STATIC IMPORTED)
      add_library(ICU::data STATIC IMPORTED)
      set_target_properties(ICU::uc PROPERTIES
        IMPORTED_LOCATION "${ICU_LIB_DIR}/libicuuc.a"
        INTERFACE_INCLUDE_DIRECTORIES "${ICU_INCLUDE_DIR}"
      )
      set_target_properties(ICU::i18n PROPERTIES
        IMPORTED_LOCATION "${ICU_LIB_DIR}/libicui18n.a"
        INTERFACE_INCLUDE_DIRECTORIES "${ICU_INCLUDE_DIR}"
      )
      set_target_properties(ICU::data PROPERTIES
        IMPORTED_LOCATION "${ICU_LIB_DIR}/libicudata.a"
        INTERFACE_INCLUDE_DIRECTORIES "${ICU_INCLUDE_DIR}"
      )
      add_dependencies(ICU::uc icu_external)
      add_dependencies(ICU::i18n icu_external)
      add_dependencies(ICU::data icu_external)
      set(CURSIVE_ICU_LIBS ICU::i18n ICU::uc ICU::data)
    endif()
  endif()
else()
  find_package(ICU REQUIRED COMPONENTS uc i18n)
  set(CURSIVE_ICU_LIBS ICU::uc ICU::i18n)
endif()

if(WIN32)
  if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(FATAL_ERROR "Windows builds require MSVC (cl.exe). Use a Visual Studio generator (e.g. \"Visual Studio 17 2022\") and toolset v143.")
  else()
    # Fix PDB locking issues with parallel builds
    add_compile_options(/FS)
  endif()
endif()

add_subdirectory(runtime)


add_library(cursive0_core STATIC
  src/core/span.cpp
  src/core/diagnostics.cpp
  src/core/diagnostic_codes.cpp
  src/core/diagnostic_render.cpp
  src/core/diagnostic_messages.cpp
  src/core/host_primitives.cpp
  src/core/ub_model.cpp
  src/core/path.cpp
  src/core/source_load.cpp
  src/core/unicode.cpp
  src/core/symbols.cpp
  src/core/hash.cpp
  src/core/spec_trace.cpp
)

target_include_directories(cursive0_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(cursive0_core PUBLIC ${CURSIVE_ICU_LIBS})

target_compile_features(cursive0_core PUBLIC cxx_std_20)

add_library(cursive0_syntax STATIC
  src/syntax/lexer.cpp
  src/syntax/lexer_ident.cpp
  src/syntax/lexer_literals.cpp
  src/syntax/lexer_security.cpp
  src/syntax/lexer_ws.cpp
  src/syntax/keyword_policy.cpp
  src/syntax/parser.cpp
  src/syntax/parser_docs.cpp
  src/syntax/parser_angle.cpp
  src/syntax/parser_consume.cpp
  src/syntax/parser_recovery.cpp
  src/syntax/parser_items.cpp
  src/syntax/parser_paths.cpp
  src/syntax/parser_placeholders.cpp
  src/syntax/parser_expr.cpp
  src/syntax/parser_patterns.cpp
  src/syntax/parser_types.cpp
  src/syntax/parser_stmt.cpp
  src/syntax/tokenize.cpp
  src/syntax/token.cpp
)

target_link_libraries(cursive0_syntax PUBLIC cursive0_core)

target_include_directories(cursive0_syntax PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(cursive0_syntax PUBLIC cxx_std_20)


add_library(cursive0_analysis STATIC
  # resolve/
  src/analysis/resolve/scopes.cpp
  src/analysis/resolve/scopes_intro.cpp
  src/analysis/resolve/scopes_lookup.cpp
  src/analysis/resolve/collect_toplevel.cpp
  src/analysis/resolve/resolve_qual.cpp
  src/analysis/resolve/resolver_expr.cpp
  src/analysis/resolve/resolver_items.cpp
  src/analysis/resolve/resolver_modules.cpp
  src/analysis/resolve/resolver_pat.cpp
  src/analysis/resolve/resolver_types.cpp
  src/analysis/resolve/visibility.cpp
  # types/
  src/analysis/types/types.cpp
  src/analysis/types/type_expr.cpp
  src/analysis/types/type_stmt.cpp
  src/analysis/types/type_pattern.cpp
  src/analysis/types/type_decls.cpp
  src/analysis/types/type_equiv.cpp
  src/analysis/types/type_match.cpp
  src/analysis/types/type_infer.cpp
  src/analysis/types/subtyping.cpp
  src/analysis/types/typecheck.cpp
  src/analysis/types/conformance.cpp
  src/analysis/types/literals.cpp
  # composite/
  src/analysis/composite/arrays_slices.cpp
  src/analysis/composite/tuples.cpp
  src/analysis/composite/unions.cpp
  src/analysis/composite/records.cpp
  src/analysis/composite/record_methods.cpp
  src/analysis/composite/enums.cpp
  src/analysis/composite/function_types.cpp
  src/analysis/composite/classes.cpp
  src/analysis/composite/class_linearization.cpp
  # modal/
  src/analysis/modal/modal.cpp
  src/analysis/modal/modal_fields.cpp
  src/analysis/modal/modal_transitions.cpp
  src/analysis/modal/modal_widen.cpp
  # memory/
  src/analysis/memory/borrow_bind.cpp
  src/analysis/memory/regions.cpp
  src/analysis/memory/region_type.cpp
  src/analysis/memory/safe_ptr.cpp
  src/analysis/memory/string_bytes.cpp
  src/analysis/memory/calls.cpp
  src/analysis/memory/init_planner.cpp
  # caps/
  src/analysis/caps/cap_system.cpp
  src/analysis/caps/cap_heap.cpp
  src/analysis/caps/cap_filesystem.cpp
)

target_link_libraries(cursive0_analysis PUBLIC cursive0_core cursive0_syntax cursive0_project)

target_include_directories(cursive0_analysis PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(cursive0_analysis PUBLIC cxx_std_20)

add_library(cursive0_codegen STATIC
  # abi/
  src/codegen/abi/abi_types.cpp
  src/codegen/abi/abi_params.cpp
  src/codegen/abi/abi_calls.cpp
  # layout/
  src/codegen/layout/layout_primitives.cpp
  src/codegen/layout/layout_dispatch.cpp
  src/codegen/layout/layout_records.cpp
  src/codegen/layout/layout_unions.cpp
  src/codegen/layout/layout_aggregates.cpp
  src/codegen/layout/layout_modal.cpp
  src/codegen/layout/layout_dynobj.cpp
  src/codegen/layout/layout_value_bits.cpp
  # lower/
  src/codegen/lower/lower_expr_core.cpp
  src/codegen/lower/lower_expr_calls.cpp
  src/codegen/lower/lower_expr_places.cpp
  src/codegen/lower/lower_stmt.cpp
  src/codegen/lower/lower_pat.cpp
  src/codegen/lower/lower_proc.cpp
  src/codegen/lower/lower_module.cpp
  # llvm/
  src/codegen/llvm/llvm_module.cpp
  src/codegen/llvm/llvm_types.cpp
  src/codegen/llvm/llvm_attrs.cpp
  src/codegen/llvm/llvm_call_abi.cpp
  src/codegen/llvm/llvm_ub_safe.cpp
  src/codegen/llvm/llvm_mem.cpp
  # runtime/
  src/codegen/runtime/runtime_calls.cpp
  src/codegen/runtime/runtime_decls.cpp
  # top-level
  src/codegen/ir_model.cpp
  src/codegen/ir_dump.cpp
  src/codegen/ir_lowering.cpp
  src/codegen/mangle.cpp
  src/codegen/linkage.cpp
  src/codegen/checks.cpp
  src/codegen/cleanup.cpp
  src/codegen/globals.cpp
  src/codegen/init.cpp
  src/codegen/dyn_dispatch.cpp
  src/codegen/vtable_emit.cpp
  src/codegen/literal_emit.cpp
  src/codegen/binding_storage.cpp
  src/codegen/entrypoint.cpp
  src/codegen/poison_instrument.cpp
)






target_link_libraries(cursive0_codegen PUBLIC cursive0_analysis ${llvm_libs})


target_include_directories(cursive0_codegen PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(cursive0_codegen PUBLIC cxx_std_20)


add_library(cursive0_project STATIC
  src/project/manifest.cpp
  src/project/load_project.cpp
  src/project/module_discovery.cpp
  src/project/deterministic_order.cpp
  src/project/outputs.cpp
  src/project/ir_assembly.cpp
  src/project/tool_resolution.cpp
  src/project/link.cpp
  src/project/project_validate.cpp
  src/project/ident.cpp
)

target_link_libraries(cursive0_project PUBLIC cursive0_core)

target_include_directories(cursive0_project PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tomlplusplus/include
)

target_compile_features(cursive0_project PUBLIC cxx_std_20)

add_library(cursive0_eval STATIC
  src/eval/value.cpp
  src/eval/control.cpp
  src/eval/state.cpp
  src/eval/address.cpp
  src/eval/match.cpp
  src/eval/eval.cpp
  src/eval/exec.cpp
  src/eval/cleanup.cpp
  src/eval/init.cpp
  src/eval/apply.cpp
  src/eval/builtins.cpp
)

target_link_libraries(cursive0_eval PUBLIC
  cursive0_core
  cursive0_syntax
  cursive0_analysis
  cursive0_project
)

target_include_directories(cursive0_eval PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(cursive0_eval PUBLIC cxx_std_20)

add_library(cursive0_interpreter STATIC
  src/eval/interpreter.cpp
)

target_link_libraries(cursive0_interpreter PUBLIC
  cursive0_eval
  cursive0_analysis
  cursive0_project
  cursive0_syntax
  cursive0_core
)

target_include_directories(cursive0_interpreter PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(cursive0_interpreter PUBLIC cxx_std_20)

add_library(cursive0_frontend STATIC
  src/syntax/parse_modules.cpp
)

target_link_libraries(cursive0_frontend PUBLIC cursive0_core cursive0_project cursive0_syntax cursive0_analysis)

target_include_directories(cursive0_frontend PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(cursive0_frontend PUBLIC cxx_std_20)

add_executable(cursive0_interpreter_cli
  tools/cursive0_interpreter_main.cpp
)

target_link_libraries(cursive0_interpreter_cli PRIVATE
  cursive0_interpreter
  cursive0_frontend
)

target_include_directories(cursive0_interpreter_cli PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(cursive0_interpreter_cli PRIVATE cxx_std_20)


add_executable(cursivec0
  src/driver/main.cpp
)

target_link_libraries(cursivec0 PRIVATE cursive0_core cursive0_project cursive0_analysis cursive0_codegen cursive0_syntax cursive0_frontend cursive0_eval)

target_include_directories(cursivec0 PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(cursivec0 PRIVATE cxx_std_20)

# Copy ICU DLLs to output directory on Windows
if(WIN32 AND CURSIVE_USE_BUNDLED_ICU AND DEFINED CURSIVE_ICU_DLLS)
  foreach(dll ${CURSIVE_ICU_DLLS})
    add_custom_command(TARGET cursivec0 POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${dll}"
        $<TARGET_FILE_DIR:cursivec0>
      COMMENT "Copying ICU DLL: ${dll}"
    )
  endforeach()
endif()

set(SPEC_VERIFIER_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/spec_verifier")
set(SPEC_VERIFIER_BOOTSTRAP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bootstrap")
set(SPEC_VERIFIER_BOOTSTRAP_COMPILER "${SPEC_VERIFIER_BOOTSTRAP_DIR}/cursivec0.exe")
set(SPEC_VERIFIER_BOOTSTRAP_RUNTIME "${SPEC_VERIFIER_BOOTSTRAP_DIR}/cursive0_rt.lib")
set(SPEC_VERIFIER_OUTPUT
  "${SPEC_VERIFIER_ROOT}/build/bin/spec_verifier${CMAKE_EXECUTABLE_SUFFIX}"
)

add_custom_command(
  OUTPUT "${SPEC_VERIFIER_OUTPUT}"
  COMMAND ${CMAKE_COMMAND} -E make_directory
          "${SPEC_VERIFIER_ROOT}/runtime"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${SPEC_VERIFIER_BOOTSTRAP_RUNTIME}"
          "${SPEC_VERIFIER_ROOT}/runtime/cursive0_rt.lib"
  COMMAND "${SPEC_VERIFIER_BOOTSTRAP_COMPILER}" build "src/main.cursive"
  WORKING_DIRECTORY "${SPEC_VERIFIER_ROOT}"
  DEPENDS
          "${SPEC_VERIFIER_BOOTSTRAP_COMPILER}"
          "${SPEC_VERIFIER_BOOTSTRAP_RUNTIME}"
          "${SPEC_VERIFIER_ROOT}/Cursive.toml"
          "${SPEC_VERIFIER_ROOT}/src/main.cursive"
)

add_custom_target(spec_verifier_build DEPENDS "${SPEC_VERIFIER_OUTPUT}")

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

if(CURSIVE_ENABLE_FUZZING)
  include(cmake/Fuzzing.cmake)
endif()
