cmake_minimum_required(VERSION 3.20)

project(cursive LANGUAGES C CXX)

include(CTest)

option(CURSIVE_USE_BUNDLED_ICU "Use bundled ICU 72.1 from third_party/icu" ON)

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
  endif()
endif()

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


add_library(cursive0_sema STATIC
  src/sema/conformance.cpp
  src/sema/class_linearization.cpp
  src/sema/classes.cpp
  src/sema/modal.cpp
  src/sema/modal_fields.cpp
  src/sema/modal_transitions.cpp
  src/sema/modal_widen.cpp
  src/sema/string_bytes.cpp
  src/sema/cap_filesystem.cpp
  src/sema/cap_heap.cpp
  src/sema/cap_system.cpp
  src/sema/enums.cpp
  src/sema/region_type.cpp
  src/sema/types.cpp
  src/sema/safe_ptr.cpp
  src/sema/scopes.cpp
  src/sema/scopes_intro.cpp
  src/sema/scopes_lookup.cpp
  src/sema/collect_toplevel.cpp
  src/sema/resolve_qual.cpp
  src/sema/resolver_expr.cpp
  src/sema/resolver_items.cpp
  src/sema/resolver_modules.cpp
  src/sema/resolver_pat.cpp
  src/sema/resolver_types.cpp
  src/sema/type_equiv.cpp
  src/sema/subtyping.cpp
  src/sema/visibility.cpp
  src/sema/function_types.cpp
  src/sema/calls.cpp
  src/sema/tuples.cpp
  src/sema/arrays_slices.cpp
  src/sema/records.cpp
  src/sema/record_methods.cpp
  src/sema/unions.cpp
  src/sema/literals.cpp
  src/sema/type_infer.cpp
  src/sema/type_stmt.cpp
  src/sema/regions.cpp
  src/sema/type_expr.cpp
  src/sema/type_pattern.cpp
  src/sema/type_match.cpp
  src/sema/borrow_bind.cpp
  src/sema/type_decls.cpp
  src/sema/typecheck.cpp
  src/sema/init_planner.cpp
)

target_link_libraries(cursive0_sema PUBLIC cursive0_core cursive0_syntax cursive0_project)

target_include_directories(cursive0_sema PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(cursive0_sema PUBLIC cxx_std_20)

add_library(cursive0_codegen STATIC
  src/codegen/ir_model.cpp
  src/codegen/layout_primitives.cpp
  src/codegen/layout_dispatch.cpp
  src/codegen/layout_records.cpp
  src/codegen/layout_unions.cpp
  src/codegen/layout_aggregates.cpp
  src/codegen/layout_modal.cpp
  src/codegen/layout_dynobj.cpp
  src/codegen/layout_value_bits.cpp
  src/codegen/abi_types.cpp
  src/codegen/abi_params.cpp
  src/codegen/abi_calls.cpp
  src/codegen/mangle.cpp
  src/codegen/linkage.cpp
  src/codegen/checks.cpp
  src/codegen/lower_expr_core.cpp
  src/codegen/lower_expr_calls.cpp
  src/codegen/lower_expr_places.cpp
  src/codegen/lower_stmt.cpp
  src/codegen/lower_pat.cpp
  src/codegen/cleanup.cpp
  src/codegen/globals.cpp
  src/codegen/init.cpp
  src/codegen/runtime_calls.cpp
  src/codegen/dyn_dispatch.cpp
  src/codegen/lower_proc.cpp
  src/codegen/lower_module.cpp
  src/codegen/ir_dump.cpp
)

target_link_libraries(cursive0_codegen PUBLIC cursive0_sema)

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

add_library(cursive0_frontend STATIC
  src/frontend/parse_modules.cpp
)

target_link_libraries(cursive0_frontend PUBLIC cursive0_core cursive0_project cursive0_syntax cursive0_sema)

target_include_directories(cursive0_frontend PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(cursive0_frontend PUBLIC cxx_std_20)


add_executable(cursivec0
  src/driver/main.cpp
)

target_link_libraries(cursivec0 PRIVATE cursive0_core cursive0_project cursive0_sema cursive0_codegen cursive0_syntax cursive0_frontend)

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

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
