# Set MSVC runtime library to match prebuilt LLVM (static release CRT).
# The shipped LLVM binaries in extern/llvm are built against /MT with
# _ITERATOR_DEBUG_LEVEL=0, so compiler targets must use the same runtime model
# across all configs to avoid link-time CRT/iterator mismatch diagnostics.
if(MSVC)
  set(CURSIVE_MSVC_RUNTIME "MultiThreaded")
  set(CMAKE_MSVC_RUNTIME_LIBRARY "${CURSIVE_MSVC_RUNTIME}" CACHE STRING "" FORCE)
endif()

add_library(cursive_core STATIC
  00_core/diagnostic_codes.cpp
  00_core/diagnostic_messages.cpp
  00_core/diagnostic_render.cpp
  00_core/diagnostics.cpp
  00_core/build_log_policy.cpp
  00_core/crash_debug.cpp
  00_core/hash.cpp
  00_core/host_primitives.cpp
  00_core/ident.cpp
  00_core/numeric_literals.cpp
  00_core/path.cpp
  00_core/process_config.cpp
  00_core/source_load.cpp
  00_core/span.cpp
  00_core/spec_trace.cpp
  00_core/symbols.cpp
  00_core/behavior_model.cpp
  00_core/terminal.cpp
  00_core/unicode.cpp
  00_core/windows_bundle.cpp
)

get_filename_component(CURSIVE_REPO_ROOT "${CMAKE_SOURCE_DIR}/.." REALPATH)
set(CURSIVE_EXTERN_DIR "${CURSIVE_REPO_ROOT}/extern")
set(CURSIVE_TOOLS_DIR "${CMAKE_SOURCE_DIR}/tools")
set(CURSIVE_STATIC_RULE_MAPPING "${CURSIVE_TOOLS_DIR}/static_rule_mapping.json")
set(CURSIVE_STATIC_RULE_REGISTRY_INC "${CMAKE_SOURCE_DIR}/src/00_core/generated/static_rule_registry.inc")
set(CURSIVE_STATIC_RULE_REGISTRY_REPORT "${CURSIVE_BUILD_DIR}/logs/conformance/static_rule_registry_report.json")
set(CURSIVE_DIAG_REGISTRY_INC "${CMAKE_SOURCE_DIR}/src/00_core/generated/diag_registry.inc")
set(CURSIVE_TYPECHECK_DIAG_MAP_INC "${CMAKE_SOURCE_DIR}/src/04_analysis/typing/item/typecheck_diag_map.inc")

if(WIN32)
  find_program(CURSIVE_POWERSHELL_EXECUTABLE powershell.exe)
else()
  find_program(CURSIVE_POWERSHELL_EXECUTABLE pwsh powershell)
endif()

if(NOT CURSIVE_POWERSHELL_EXECUTABLE)
  message(FATAL_ERROR "PowerShell executable not found; required for static rule registry generation.")
endif()

set(CURSIVE_DIAG_SPEC_PATH "" CACHE FILEPATH "Path to the canonical Cursive language specification")
if(NOT CURSIVE_DIAG_SPEC_PATH)
  execute_process(
    COMMAND ${CURSIVE_POWERSHELL_EXECUTABLE}
            -NoProfile
            -ExecutionPolicy Bypass
            -File "${CURSIVE_TOOLS_DIR}/resolve_spec_path.ps1"
            -RepoRoot "${CURSIVE_REPO_ROOT}"
    RESULT_VARIABLE CURSIVE_DIAG_SPEC_RESULT
    OUTPUT_VARIABLE CURSIVE_DIAG_SPEC_PATH
    ERROR_VARIABLE CURSIVE_DIAG_SPEC_ERROR
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if(NOT CURSIVE_DIAG_SPEC_RESULT EQUAL 0)
    message(FATAL_ERROR "Unable to resolve canonical language spec path: ${CURSIVE_DIAG_SPEC_ERROR}")
  endif()
  set(CURSIVE_DIAG_SPEC_PATH "${CURSIVE_DIAG_SPEC_PATH}" CACHE FILEPATH "Path to the canonical Cursive language specification" FORCE)
endif()

if(NOT EXISTS "${CURSIVE_DIAG_SPEC_PATH}")
  message(FATAL_ERROR "Resolved language spec path does not exist: ${CURSIVE_DIAG_SPEC_PATH}")
endif()

add_custom_target(generate_static_rule_registry
  COMMAND ${CMAKE_COMMAND} -E make_directory "${CURSIVE_BUILD_DIR}/logs/conformance"
  COMMAND ${CURSIVE_POWERSHELL_EXECUTABLE}
          -NoProfile
          -ExecutionPolicy Bypass
          -File "${CURSIVE_TOOLS_DIR}/generate_static_rule_registry.ps1"
          -RepoRoot "${CURSIVE_REPO_ROOT}"
          -MappingPath "${CURSIVE_STATIC_RULE_MAPPING}"
          -OutputPath "${CURSIVE_STATIC_RULE_REGISTRY_INC}"
          -ReportPath "${CURSIVE_STATIC_RULE_REGISTRY_REPORT}"
          -Strict
  BYPRODUCTS
          "${CURSIVE_STATIC_RULE_REGISTRY_INC}"
          "${CURSIVE_STATIC_RULE_REGISTRY_REPORT}"
  DEPENDS
          "${CURSIVE_TOOLS_DIR}/generate_static_rule_registry.ps1"
          "${CURSIVE_STATIC_RULE_MAPPING}"
  COMMENT "Generating static rule registry for §1.2 conformance"
  VERBATIM
)

add_custom_target(generate_diagnostic_registry
  COMMAND ${CURSIVE_POWERSHELL_EXECUTABLE}
          -NoProfile
          -ExecutionPolicy Bypass
          -File "${CURSIVE_TOOLS_DIR}/generate_diagnostic_registry.ps1"
          -RepoRoot "${CURSIVE_REPO_ROOT}"
          -SpecPath "${CURSIVE_DIAG_SPEC_PATH}"
          -OutputRegistryPath "${CURSIVE_DIAG_REGISTRY_INC}"
          -OutputTypecheckMapPath "${CURSIVE_TYPECHECK_DIAG_MAP_INC}"
  BYPRODUCTS
          "${CURSIVE_DIAG_REGISTRY_INC}"
          "${CURSIVE_TYPECHECK_DIAG_MAP_INC}"
  DEPENDS
          "${CURSIVE_TOOLS_DIR}/generate_diagnostic_registry.ps1"
          "${CURSIVE_TOOLS_DIR}/resolve_spec_path.ps1"
          "${CURSIVE_DIAG_SPEC_PATH}"
  COMMENT "Generating diagnostic registry from canonical language spec"
  VERBATIM
)

add_custom_target(validate_diagnostic_spec_sync
  COMMAND ${CURSIVE_POWERSHELL_EXECUTABLE}
          -NoProfile
          -ExecutionPolicy Bypass
          -File "${CURSIVE_TOOLS_DIR}/validate_diagnostic_spec_sync.ps1"
          -RepoRoot "${CURSIVE_REPO_ROOT}"
          -SpecPath "${CURSIVE_DIAG_SPEC_PATH}"
          -RegistryPath "${CURSIVE_DIAG_REGISTRY_INC}"
  DEPENDS
          generate_diagnostic_registry
          "${CURSIVE_TOOLS_DIR}/validate_diagnostic_spec_sync.ps1"
          "${CURSIVE_TOOLS_DIR}/resolve_spec_path.ps1"
          "${CURSIVE_DIAG_SPEC_PATH}"
  COMMENT "Validating diagnostic spec sync against canonical language spec"
  VERBATIM
)

add_dependencies(cursive_core generate_static_rule_registry generate_diagnostic_registry validate_diagnostic_spec_sync)

if(WIN32)
  set(ICU_ROOT "${CURSIVE_EXTERN_DIR}/icu/win64")
  set(ICU_INCLUDE_DIR "${ICU_ROOT}/include")
  set(ICU_LIB_DIR "${ICU_ROOT}/lib64")
  set(ICU_BIN_DIR "${ICU_ROOT}/bin64")
  set(ICU_LIBS
    "${ICU_LIB_DIR}/icuuc.lib"
    "${ICU_LIB_DIR}/icuin.lib"
    "${ICU_LIB_DIR}/icudt.lib"
  )
  # ICU DLLs needed at runtime
  set(ICU_DLLS
    "${ICU_BIN_DIR}/icuuc72.dll"
    "${ICU_BIN_DIR}/icuin72.dll"
    "${ICU_BIN_DIR}/icudt72.dll"
  )
else()
  set(ICU_ROOT "${CURSIVE_EXTERN_DIR}/icu/install")
  set(ICU_INCLUDE_DIR "${ICU_ROOT}/include")
  set(ICU_LIB_DIR "${ICU_ROOT}/lib")
  set(ICU_LIBS
    "${ICU_LIB_DIR}/libicuuc.a"
    "${ICU_LIB_DIR}/libicui18n.a"
    "${ICU_LIB_DIR}/libicudata.a"
  )
endif()

if(NOT EXISTS "${ICU_INCLUDE_DIR}")
  message(FATAL_ERROR "ICU include directory not found: ${ICU_INCLUDE_DIR}")
endif()

foreach(icu_lib IN LISTS ICU_LIBS)
  if(NOT EXISTS "${icu_lib}")
    message(FATAL_ERROR "ICU library not found: ${icu_lib}")
  endif()
endforeach()

target_include_directories(cursive_core PUBLIC
  ${CMAKE_SOURCE_DIR}/include
  ${CURSIVE_REPO_ROOT}
)

target_compile_features(cursive_core PUBLIC cxx_std_20)
target_include_directories(cursive_core PRIVATE ${ICU_INCLUDE_DIR})
target_link_libraries(cursive_core PRIVATE ${ICU_LIBS})
if(WIN32)
  target_link_libraries(cursive_core PRIVATE dbghelp)
endif()

add_library(cursive_project STATIC
  01_project/assemblies.cpp
  01_project/assembly_graph.cpp
  01_project/deterministic_order.cpp
  01_project/ir_assembly.cpp
  01_project/link.cpp
  01_project/load_project.cpp
  01_project/manifest.cpp
  01_project/module_discovery.cpp
  01_project/outputs.cpp
  01_project/project_validate.cpp
  01_project/target_profile.cpp
  01_project/tool_resolution.cpp
  01_project/ffi_library.cpp
  01_project/unwind_ffi_surface.cpp
)

target_include_directories(cursive_project PUBLIC
  ${CMAKE_SOURCE_DIR}/include
  ${CURSIVE_REPO_ROOT}
  ${CURSIVE_EXTERN_DIR}/tomlplusplus/include
)

target_compile_features(cursive_project PUBLIC cxx_std_20)

add_library(cursive_source STATIC
  02_source/module_paths.cpp
  02_source/ast/builder/builder_common.cpp
  02_source/ast/ast_dump.cpp
  02_source/ast/nodes/ast_nodes.cpp
  02_source/ast/validate/validate_common.cpp
  02_source/lexer/keyword_policy.cpp
  02_source/lexer/lexer.cpp
  02_source/lexer/lexer_ident.cpp
  02_source/lexer/lexer_literals.cpp
  02_source/lexer/lexer_security.cpp
  02_source/lexer/lexer_ws.cpp
  02_source/lexer/token.cpp
  02_source/lexer/tokenize.cpp
  02_source/parser/expr/all_expr.cpp
  02_source/parser/expr/alloc_expr.cpp
  02_source/parser/expr/array_literal.cpp
  02_source/parser/expr/addr_of.cpp
  02_source/parser/expr/binary.cpp
  02_source/parser/expr/block_expr.cpp
  02_source/parser/expr/call.cpp
  02_source/parser/expr/call_type_args.cpp
  02_source/parser/expr/cast.cpp
  02_source/parser/expr/closure_expr.cpp
  02_source/parser/expr/comptime_expr.cpp
  02_source/parser/expr/contract_entry.cpp
  02_source/parser/expr/contract_result.cpp
  02_source/parser/expr/deref.cpp
  02_source/parser/expr/dispatch_expr.cpp
  02_source/parser/expr/enum_literal.cpp
  02_source/parser/expr/error_expr.cpp
  02_source/parser/expr/expr_common.cpp
  02_source/parser/expr/field_access.cpp
  02_source/parser/expr/identifier.cpp
  02_source/parser/expr/if_expr.cpp
  02_source/parser/expr/index_access.cpp
  02_source/parser/expr/literal.cpp
  02_source/parser/expr/loop_conditional.cpp
  02_source/parser/expr/loop_infinite.cpp
  02_source/parser/expr/loop_iter.cpp
  02_source/parser/expr/method_call.cpp
  02_source/parser/expr/move_expr.cpp
  02_source/parser/expr/null_ptr.cpp
  02_source/parser/expr/parallel_expr.cpp
  02_source/parser/expr/path.cpp
  02_source/parser/expr/pipeline_expr.cpp
  02_source/parser/expr/postfix.cpp
  02_source/parser/expr/primary.cpp
  02_source/parser/expr/propagate_expr.cpp
  02_source/parser/expr/qualified_apply.cpp
  02_source/parser/expr/qualified_name.cpp
  02_source/parser/expr/quote_expr.cpp
  02_source/parser/expr/race_expr.cpp
  02_source/parser/expr/range.cpp
  02_source/parser/expr/record_literal.cpp
  02_source/parser/expr/spawn_expr.cpp
  02_source/parser/expr/sync_expr.cpp
  02_source/parser/expr/transmute_expr.cpp
  02_source/parser/expr/type_literal.cpp
  02_source/parser/expr/tuple_access.cpp
  02_source/parser/expr/tuple_literal.cpp
  02_source/parser/expr/unary.cpp
  02_source/parser/expr/unsafe_block_expr.cpp
  02_source/parser/expr/wait_expr.cpp
  02_source/parser/expr/yield_expr.cpp
  02_source/parser/expr/yield_from_expr.cpp
  02_source/parser/item/attribute_list.cpp
  02_source/parser/item/class_decl.cpp
  02_source/parser/item/comptime_procedure_decl.cpp
  02_source/parser/item/contract_clause.cpp
  02_source/parser/item/derive_target_decl.cpp
  02_source/parser/item/enum_decl.cpp
  02_source/parser/item/extern_block.cpp
  02_source/parser/item/foreign_contract_clause.cpp
  02_source/parser/item/generic_params.cpp
  02_source/parser/item/import_decl.cpp
  02_source/parser/item/item_common.cpp
  02_source/parser/item/modal_decl.cpp
  02_source/parser/item/parse_item.cpp
  02_source/parser/item/procedure_decl.cpp
  02_source/parser/item/record_decl.cpp
  02_source/parser/item/signature.cpp
  02_source/parser/item/static_decl.cpp
  02_source/parser/item/type_alias_decl.cpp
  02_source/parser/item/using_decl.cpp
  02_source/parser/item/visibility.cpp
  02_source/parser/item/where_clause.cpp
  02_source/parser/parse_modules.cpp
  02_source/parser/parser.cpp
  02_source/parser/parser_angle.cpp
  02_source/parser/parser_consume.cpp
  02_source/parser/parser_docs.cpp
  02_source/parser/parser_paths.cpp
  02_source/parser/parser_recovery.cpp
  02_source/parser/parser_state.cpp
  02_source/parser/parser_terminator.cpp
  02_source/parser/pattern/enum_pattern.cpp
  02_source/parser/pattern/identifier_pattern.cpp
  02_source/parser/pattern/literal_pattern.cpp
  02_source/parser/pattern/modal_pattern.cpp
  02_source/parser/pattern/pattern_common.cpp
  02_source/parser/pattern/range_pattern.cpp
  02_source/parser/pattern/record_pattern.cpp
  02_source/parser/pattern/tuple_pattern.cpp
  02_source/parser/pattern/typed_pattern.cpp
  02_source/parser/pattern/wildcard_pattern.cpp
  02_source/parser/stmt/assign_stmt.cpp
  02_source/parser/stmt/break_stmt.cpp
  02_source/parser/stmt/comptime_stmt.cpp
  02_source/parser/stmt/compound_assign_stmt.cpp
  02_source/parser/stmt/continue_stmt.cpp
  02_source/parser/stmt/defer_stmt.cpp
  02_source/parser/stmt/error_stmt.cpp
  02_source/parser/stmt/expr_stmt.cpp
  02_source/parser/stmt/frame_stmt.cpp
  02_source/parser/stmt/key_block_stmt.cpp
  02_source/parser/stmt/let_stmt.cpp
  02_source/parser/stmt/parse_stmt.cpp
  02_source/parser/stmt/region_stmt.cpp
  02_source/parser/stmt/return_stmt.cpp
  02_source/parser/stmt/shadow_let_stmt.cpp
  02_source/parser/stmt/shadow_var_stmt.cpp
  02_source/parser/stmt/stmt_common.cpp
  02_source/parser/stmt/unsafe_block_stmt.cpp
  02_source/parser/stmt/var_stmt.cpp
  02_source/parser/type/array_type.cpp
  02_source/parser/type/bytes_type.cpp
  02_source/parser/type/closure_type.cpp
  02_source/parser/type/dynamic_type.cpp
  02_source/parser/type/function_type.cpp
  02_source/parser/type/opaque_type.cpp
  02_source/parser/type/permission.cpp
  02_source/parser/type/primitive_type.cpp
  02_source/parser/type/raw_ptr_type.cpp
  02_source/parser/type/refinement_clause.cpp
  02_source/parser/type/safe_ptr_type.cpp
  02_source/parser/type/slice_type.cpp
  02_source/parser/type/state_specific_type.cpp
  02_source/parser/type/string_type.cpp
  02_source/parser/type/tuple_type.cpp
  02_source/parser/type/type_args.cpp
  02_source/parser/type/type_bounds.cpp
  02_source/parser/type/type_common.cpp
  02_source/parser/type/type_path.cpp
  02_source/parser/type/union_type.cpp
  03_comptime/common.cpp
  03_comptime/derive.cpp
  03_comptime/eval.cpp
  03_comptime/files.cpp
  03_comptime/pass.cpp
  03_comptime/quote.cpp
  03_comptime/reflect.cpp
  03_comptime/rewrite.cpp
)

target_include_directories(cursive_source PUBLIC
  ${CMAKE_SOURCE_DIR}/include
  ${CURSIVE_REPO_ROOT}
)

target_compile_features(cursive_source PUBLIC cxx_std_20)

add_library(cursive_analysis STATIC
  04_analysis/attributes/attribute_registry.cpp
  04_analysis/attributes/attribute_targets.cpp
  04_analysis/attributes/attribute_validate.cpp
  04_analysis/caps/authority_model.cpp
  04_analysis/caps/callgraph_caps.cpp
  04_analysis/caps/cap_comptime.cpp
  04_analysis/caps/cap_concurrency.cpp
  04_analysis/caps/cap_filesystem.cpp
  04_analysis/caps/cap_heap.cpp
  04_analysis/caps/cap_network.cpp
  04_analysis/caps/cap_requirements.cpp
  04_analysis/caps/cap_system.cpp
  04_analysis/caps/context_caps.cpp
  04_analysis/composite/arrays_slices.cpp
  04_analysis/composite/class_linearization.cpp
  04_analysis/composite/classes.cpp
  04_analysis/composite/enums.cpp
  04_analysis/composite/function_types.cpp
  04_analysis/composite/reflect_bridge.cpp
  04_analysis/composite/record_methods.cpp
  04_analysis/composite/records.cpp
  04_analysis/composite/tuples.cpp
  04_analysis/composite/unions.cpp
  04_analysis/conformance/conformance.cpp
  04_analysis/conformance/subset_checks.cpp
  04_analysis/contracts/contract_check.cpp
  04_analysis/contracts/contract_intrinsics.cpp
  04_analysis/contracts/verification.cpp
  04_analysis/generics/generic_params.cpp
  04_analysis/generics/monomorphize.cpp
  04_analysis/keys/key_conflict.cpp
  04_analysis/keys/key_context.cpp
  04_analysis/keys/key_capture.cpp
  04_analysis/keys/key_lifetimes.cpp
  04_analysis/keys/key_paths.cpp
  04_analysis/memory/borrow_bind.cpp
  04_analysis/memory/calls.cpp
  04_analysis/memory/init_planner.cpp
  04_analysis/memory/region_type.cpp
  04_analysis/memory/regions.cpp
  04_analysis/memory/safe_ptr.cpp
  04_analysis/memory/string_bytes.cpp
  04_analysis/modal/builtin_modal_intrinsics.cpp
  04_analysis/modal/modal.cpp
  04_analysis/modal/modal_fields.cpp
  04_analysis/modal/modal_transitions.cpp
  04_analysis/modal/modal_widen.cpp
  04_analysis/provenance/prov_args.cpp
  04_analysis/provenance/prov_block.cpp
  04_analysis/provenance/prov_expr.cpp
  04_analysis/provenance/prov_place.cpp
  04_analysis/provenance/prov_stmt.cpp
  04_analysis/resolve/resolve_attributes.cpp
  04_analysis/resolve/resolve_callee.cpp
  04_analysis/resolve/resolve_contracts.cpp
  04_analysis/resolve/resolve_enum_payload.cpp
  04_analysis/resolve/resolve_expr.cpp
  04_analysis/resolve/resolve_expr_list.cpp
  04_analysis/resolve/resolve_extern.cpp
  04_analysis/resolve/resolve_generics.cpp
  04_analysis/resolve/resolve_items.cpp
  04_analysis/resolve/resolve_module.cpp
  # resolve_path.cpp removed - functionality is in scopes_lookup.cpp
  04_analysis/resolve/resolve_pattern.cpp
  04_analysis/resolve/resolve_qual.cpp
  04_analysis/resolve/resolve_stmt_seq.cpp
  04_analysis/resolve/resolve_types.cpp
  04_analysis/resolve/resolve_using.cpp
  04_analysis/resolve/collect_toplevel.cpp
  04_analysis/resolve/scopes.cpp
  04_analysis/resolve/scopes_intro.cpp
  04_analysis/resolve/scopes_lookup.cpp
  04_analysis/resolve/visibility.cpp
  04_analysis/typing/arg_pass.cpp
  04_analysis/typing/args_ok.cpp
  04_analysis/typing/const_len.cpp
  04_analysis/typing/dynamic_context.cpp
  04_analysis/typing/deprecation_warnings.cpp
  04_analysis/typing/ffi_by_value.cpp
  04_analysis/typing/type_layout.cpp
  04_analysis/typing/type_lower.cpp
  04_analysis/typing/type_expr.cpp
  04_analysis/typing/type_lookup.cpp
  04_analysis/typing/type_predicates.cpp
  04_analysis/typing/expr/addr_of.cpp
  04_analysis/typing/expr/alignof.cpp
  04_analysis/typing/expr/all_expr.cpp
  04_analysis/typing/expr/alloc_expr.cpp
  04_analysis/typing/expr/array_literal.cpp
  04_analysis/typing/expr/array_repeat.cpp
  04_analysis/typing/expr/binary.cpp
  04_analysis/typing/expr/block_expr.cpp
  04_analysis/typing/expr/call.cpp
  04_analysis/typing/expr/call_type_args.cpp
  04_analysis/typing/expr/cast.cpp
  04_analysis/typing/expr/closure_expr.cpp
  04_analysis/typing/expr/contract_entry.cpp
  04_analysis/typing/expr/contract_result.cpp
  04_analysis/typing/expr/deref.cpp
  04_analysis/typing/expr/dispatch_expr.cpp
  04_analysis/typing/expr/enum_literal.cpp
  04_analysis/typing/expr/error_expr.cpp
  04_analysis/typing/expr/expr_common.cpp
  04_analysis/typing/expr/field_access.cpp
  04_analysis/typing/expr/identifier.cpp
  04_analysis/typing/expr/if_expr.cpp
  04_analysis/typing/expr/index_access.cpp
  # literal.cpp removed - functionality is in typing/literals.cpp
  04_analysis/typing/expr/loop_conditional.cpp
  04_analysis/typing/expr/loop_infinite.cpp
  04_analysis/typing/expr/loop_iter.cpp
  04_analysis/typing/expr/method_call.cpp
  04_analysis/typing/expr/move_expr.cpp
  04_analysis/typing/expr/null_ptr.cpp
  04_analysis/typing/expr/parallel_expr.cpp
  04_analysis/typing/expr/path.cpp
  04_analysis/typing/expr/pipeline_expr.cpp
  04_analysis/typing/expr/propagate_expr.cpp
  04_analysis/typing/expr/qualified_apply.cpp
  04_analysis/typing/expr/qualified_name.cpp
  04_analysis/typing/expr/race_expr.cpp
  04_analysis/typing/expr/range.cpp
  04_analysis/typing/expr/record_literal.cpp
  04_analysis/typing/expr/spawn_expr.cpp
  04_analysis/typing/expr/sizeof.cpp
  04_analysis/typing/expr/sync_expr.cpp
  04_analysis/typing/expr/transmute_expr.cpp
  04_analysis/typing/expr/tuple_access.cpp
  04_analysis/typing/expr/tuple_literal.cpp
  04_analysis/typing/expr/unary.cpp
  04_analysis/typing/expr/unsafe_block_expr.cpp
  04_analysis/typing/expr/wait_expr.cpp
  04_analysis/typing/expr/yield_expr.cpp
  04_analysis/typing/expr/yield_from_expr.cpp
  04_analysis/typing/item/attribute_list.cpp
  04_analysis/typing/item/class_decl.cpp
  04_analysis/typing/item/enum_decl.cpp
  04_analysis/typing/item/extern_block.cpp
  04_analysis/typing/item/generic_params.cpp
  04_analysis/typing/item/import_decl.cpp
  04_analysis/typing/item/item_common.cpp
  04_analysis/typing/item/modal_decl.cpp
  04_analysis/typing/item/procedure_decl.cpp
  04_analysis/typing/item/record_decl.cpp
  04_analysis/typing/item/signature.cpp
  04_analysis/typing/item/static_decl.cpp
  04_analysis/typing/item/type_alias_decl.cpp
  04_analysis/typing/item/using_decl.cpp
  04_analysis/typing/item/visibility.cpp
  04_analysis/typing/item/where_clause.cpp
  04_analysis/typing/literals.cpp
  04_analysis/typing/if_case_check.cpp
  04_analysis/typing/stmt/assign_stmt.cpp
  04_analysis/typing/stmt/break_stmt.cpp
  04_analysis/typing/stmt/compound_assign_stmt.cpp
  04_analysis/typing/stmt/continue_stmt.cpp
  04_analysis/typing/stmt/defer_stmt.cpp
  04_analysis/typing/stmt/error_stmt.cpp
  04_analysis/typing/stmt/expr_stmt.cpp
  04_analysis/typing/stmt/frame_stmt.cpp
  04_analysis/typing/stmt/key_block_stmt.cpp
  04_analysis/typing/stmt/let_stmt.cpp
  04_analysis/typing/stmt/region_stmt.cpp
  04_analysis/typing/stmt/return_stmt.cpp
  04_analysis/typing/stmt/shadow_let_stmt.cpp
  04_analysis/typing/stmt/shadow_var_stmt.cpp
  04_analysis/typing/stmt/stmt_common.cpp
  04_analysis/typing/stmt/unsafe_block_stmt.cpp
  04_analysis/typing/stmt/var_stmt.cpp
  04_analysis/typing/subtyping.cpp
  04_analysis/typing/type_equiv.cpp
  04_analysis/typing/type_infer.cpp
  04_analysis/typing/type_refs.cpp
  04_analysis/typing/type_wf.cpp
  04_analysis/typing/typecheck.cpp
  04_analysis/typing/variance.cpp
  04_analysis/typing/pattern/pattern_common.cpp
)

target_include_directories(cursive_analysis PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)

target_compile_features(cursive_analysis PUBLIC cxx_std_20)

add_library(cursive_codegen STATIC
  05_codegen/abi/abi_calls.cpp
  05_codegen/abi/abi_lowering.cpp
  05_codegen/abi/abi_panic.cpp
  05_codegen/abi/abi_params.cpp
  05_codegen/abi/abi_returns.cpp
  05_codegen/abi/abi_types.cpp
  05_codegen/abi/types/abi_type_array.cpp
  05_codegen/abi/types/abi_type_dynamic.cpp
  05_codegen/abi/types/abi_type_func.cpp
  05_codegen/abi/types/abi_type_modal.cpp
  05_codegen/abi/types/abi_type_path.cpp
  05_codegen/abi/types/abi_type_prim.cpp
  05_codegen/abi/types/abi_type_ptr.cpp
  05_codegen/abi/types/abi_type_range.cpp
  05_codegen/abi/types/abi_type_rawptr.cpp
  05_codegen/abi/types/abi_type_slice.cpp
  05_codegen/abi/types/abi_type_string.cpp
  05_codegen/abi/types/abi_type_tuple.cpp
  05_codegen/abi/types/abi_type_union.cpp
  05_codegen/abi/calling_conventions.cpp
  05_codegen/checks/checks.cpp
  05_codegen/checks/panic.cpp
  05_codegen/checks/poison_instrument.cpp
  05_codegen/cleanup/cleanup.cpp
  05_codegen/cleanup/drop_hooks.cpp
  05_codegen/cleanup/unwind.cpp
  05_codegen/dyn_dispatch/dyn_dispatch.cpp
  05_codegen/globals/binding_storage.cpp
  05_codegen/globals/globals.cpp
  05_codegen/globals/init.cpp
  05_codegen/globals/literal_emit.cpp
  05_codegen/intrinsics/builtins.cpp
  05_codegen/intrinsics/intrinsics_calls.cpp
  05_codegen/intrinsics/intrinsics_decls.cpp
  05_codegen/intrinsics/intrinsics_interface.cpp
  05_codegen/ir/ir_dump.cpp
  05_codegen/ir/ir_model.cpp
  05_codegen/llvm/llvm_attr.cpp
  05_codegen/llvm/llvm_backend.cpp
  05_codegen/llvm/llvm_call.cpp
  05_codegen/llvm/llvm_emit.cpp
  05_codegen/llvm/llvm_ir_panic.cpp
  05_codegen/llvm/llvm_module.cpp
  05_codegen/llvm/llvm_passes.cpp
  05_codegen/llvm/llvm_types.cpp
  05_codegen/llvm/llvm_ub_safe.cpp
  05_codegen/layout/layout_aggregates.cpp
  05_codegen/layout/layout_dispatch.cpp
  05_codegen/layout/layout_dynobj.cpp
  05_codegen/layout/layout_modal.cpp
  05_codegen/layout/layout_primitives.cpp
  05_codegen/layout/layout_records.cpp
  05_codegen/layout/layout_tuples.cpp
  05_codegen/layout/layout_unions.cpp
  05_codegen/layout/layout_value_bits.cpp
  05_codegen/lower/expr/addr_of.cpp
  05_codegen/lower/expr/all_expr.cpp
  05_codegen/lower/expr/alloc_expr.cpp
  05_codegen/lower/expr/array_literal.cpp
  05_codegen/lower/expr/binary.cpp
  05_codegen/lower/expr/block_expr.cpp
  05_codegen/lower/expr/call.cpp
  05_codegen/lower/expr/cast.cpp
  05_codegen/lower/expr/closure_expr.cpp
  05_codegen/lower/expr/contract_entry.cpp
  05_codegen/lower/expr/contract_result.cpp
  05_codegen/lower/expr/deref.cpp
  05_codegen/lower/expr/dispatch_expr.cpp
  05_codegen/lower/expr/enum_literal.cpp
  05_codegen/lower/expr/error_expr.cpp
  05_codegen/lower/expr/expr_common.cpp
  05_codegen/lower/expr/field_access.cpp
  05_codegen/lower/expr/identifier.cpp
  05_codegen/lower/expr/if_expr.cpp
  05_codegen/lower/expr/index_access.cpp
  05_codegen/lower/expr/literal.cpp
  05_codegen/lower/expr/loop_conditional.cpp
  05_codegen/lower/expr/loop_infinite.cpp
  05_codegen/lower/expr/loop_iter.cpp
  05_codegen/lower/expr/if_case_expr.cpp
  05_codegen/lower/expr/method_call.cpp
  05_codegen/lower/expr/move_expr.cpp
  05_codegen/lower/expr/null_ptr.cpp
  05_codegen/lower/expr/parallel_expr.cpp
  05_codegen/lower/expr/path.cpp
  05_codegen/lower/expr/pipeline_expr.cpp
  05_codegen/lower/expr/propagate_expr.cpp
  05_codegen/lower/expr/qualified_apply.cpp
  05_codegen/lower/expr/qualified_name.cpp
  05_codegen/lower/expr/race_expr.cpp
  05_codegen/lower/expr/range.cpp
  05_codegen/lower/expr/record_literal.cpp
  05_codegen/lower/expr/spawn_expr.cpp
  05_codegen/lower/expr/sync_expr.cpp
  05_codegen/lower/expr/transmute_expr.cpp
  05_codegen/lower/expr/tuple_access.cpp
  05_codegen/lower/expr/tuple_literal.cpp
  05_codegen/lower/expr/unary.cpp
  05_codegen/lower/expr/unsafe_block_expr.cpp
  05_codegen/lower/expr/wait_expr.cpp
  05_codegen/lower/expr/yield_expr.cpp
  05_codegen/lower/expr/yield_from_expr.cpp
  05_codegen/lower/lower_bind.cpp
  05_codegen/lower/lower_module.cpp
  05_codegen/lower/lower_proc.cpp
  05_codegen/lower/pattern/enum_pattern.cpp
  05_codegen/lower/pattern/identifier_pattern.cpp
  05_codegen/lower/pattern/literal_pattern.cpp
  05_codegen/lower/pattern/modal_pattern.cpp
  05_codegen/lower/pattern/pattern_common.cpp
  05_codegen/lower/pattern/range_pattern.cpp
  05_codegen/lower/pattern/record_pattern.cpp
  05_codegen/lower/pattern/tuple_pattern.cpp
  05_codegen/lower/pattern/typed_pattern.cpp
  05_codegen/lower/pattern/wildcard_pattern.cpp
  05_codegen/lower/stmt/assign_stmt.cpp
  05_codegen/lower/stmt/break_stmt.cpp
  05_codegen/lower/stmt/compound_assign_stmt.cpp
  05_codegen/lower/stmt/continue_stmt.cpp
  05_codegen/lower/stmt/defer_stmt.cpp
  05_codegen/lower/stmt/error_stmt.cpp
  05_codegen/lower/stmt/expr_stmt.cpp
  05_codegen/lower/stmt/frame_stmt.cpp
  05_codegen/lower/stmt/key_block_stmt.cpp
  05_codegen/lower/stmt/let_stmt.cpp
  05_codegen/lower/stmt/region_stmt.cpp
  05_codegen/lower/stmt/return_stmt.cpp
  05_codegen/lower/stmt/shadow_let_stmt.cpp
  05_codegen/lower/stmt/shadow_var_stmt.cpp
  05_codegen/lower/stmt/stmt_common.cpp
  05_codegen/lower/stmt/unsafe_block_stmt.cpp
  05_codegen/lower/stmt/var_stmt.cpp
  05_codegen/symbols/linkage.cpp
  05_codegen/symbols/mangle.cpp
  05_codegen/symbols/symbol_table.cpp
  05_codegen/symbols/visibility.cpp
)

# Set up LLVM
set(LLVM_ARCH_DIR "llvm-21.1.8-x86_64")
set(LLVM_ROOT "${CURSIVE_EXTERN_DIR}/llvm/${LLVM_ARCH_DIR}")

if(EXISTS "${LLVM_ROOT}")
  message(STATUS "Using LLVM from: ${LLVM_ROOT}")
  set(LLVM_DIR "${LLVM_ROOT}/lib/cmake/llvm")
  find_package(LLVM REQUIRED CONFIG)
  message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
else()
  message(WARNING "LLVM not found at ${LLVM_ROOT}")
endif()

target_include_directories(cursive_codegen PUBLIC
  ${CMAKE_SOURCE_DIR}/include
  ${LLVM_INCLUDE_DIRS}
)

target_compile_features(cursive_codegen PUBLIC cxx_std_20)

if(LLVM_FOUND)
  target_compile_definitions(cursive_codegen PRIVATE ${LLVM_DEFINITIONS})

  # Map LLVM components to library names
  llvm_map_components_to_libnames(LLVM_LIBS
    Core
    Support
    Target
    MC
    TargetParser
    CodeGen
    X86CodeGen
    X86AsmParser
    X86Desc
    X86Info
    Passes
    Analysis
    TransformUtils
  )

  target_link_libraries(cursive_codegen PUBLIC ${LLVM_LIBS})

  target_include_directories(cursive_project PUBLIC ${LLVM_INCLUDE_DIRS})
  target_compile_definitions(cursive_project PRIVATE ${LLVM_DEFINITIONS})

  if(WIN32)
    set(CURSIVE_PROJECT_LLVM_LIBS
      "${LLVM_ROOT}/lib/LLVMAsmParser.lib"
      "${LLVM_ROOT}/lib/LLVMBitWriter.lib"
      "${LLVM_ROOT}/lib/LLVMObject.lib"
      "${LLVM_ROOT}/lib/LLVMBinaryFormat.lib"
    )
    foreach(llvm_extra_lib IN LISTS CURSIVE_PROJECT_LLVM_LIBS)
      if(NOT EXISTS "${llvm_extra_lib}")
        message(FATAL_ERROR "Required LLVM/lld support library not found: ${llvm_extra_lib}")
      endif()
    endforeach()
  else()
    set(CURSIVE_PROJECT_LLVM_LIBS
      LLVMAsmParser
      LLVMBitWriter
    )
  endif()

  target_link_libraries(cursive_project PUBLIC ${CURSIVE_PROJECT_LLVM_LIBS})
endif()

target_link_libraries(cursive_project PUBLIC cursive_core)
target_link_libraries(cursive_project PUBLIC cursive_source)
target_link_libraries(cursive_source PUBLIC cursive_core)
target_link_libraries(cursive_analysis PUBLIC cursive_core cursive_source cursive_project)
target_link_libraries(cursive_codegen PUBLIC cursive_analysis)

add_executable(cursive-debug
  06_driver/debugger_main.cpp
  06_driver/version.cpp
)

set_target_properties(cursive-debug PROPERTIES OUTPUT_NAME "cursive-debug")

target_link_libraries(cursive-debug PRIVATE cursive_core)

target_include_directories(cursive-debug PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

target_compile_features(cursive-debug PRIVATE cxx_std_20)

add_executable(cursive
  06_driver/cli.cpp
  06_driver/main.cpp
  06_driver/options.cpp
  06_driver/pipeline.cpp
  06_driver/shared_library_exports.cpp
  06_driver/version.cpp
)

target_link_libraries(cursive PRIVATE cursive_core cursive_project cursive_analysis cursive_codegen cursive_source)

target_include_directories(cursive PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${LLVM_INCLUDE_DIRS}
)

target_compile_features(cursive PRIVATE cxx_std_20)

if(LLVM_FOUND)
  target_compile_definitions(cursive PRIVATE ${LLVM_DEFINITIONS})
endif()

if(MSVC)
  # Re-assert runtime model after LLVM package loading and pin it on the
  # executable target to prevent Debug/Release CRT mixing at link time.
  set(CMAKE_MSVC_RUNTIME_LIBRARY "${CURSIVE_MSVC_RUNTIME}" CACHE STRING "" FORCE)
  set_property(TARGET cursive PROPERTY
    MSVC_RUNTIME_LIBRARY "${CURSIVE_MSVC_RUNTIME}")

  target_link_libraries(cursive PRIVATE delayimp)
  target_link_options(cursive PRIVATE
    "/DELAYLOAD:icuuc72.dll"
    "/DELAYLOAD:icuin72.dll"
  )
endif()

# Ensure runtime library is built alongside compiler
# The runtime is linked with compiled Cursive programs, not the compiler itself,
# but we want it built as part of the standard build process
add_dependencies(cursive cursive0_rt)

if(WIN32)
  find_library(CURSIVE_DELAYIMP_LIB_PATH NAMES delayimp)
  if(NOT CURSIVE_DELAYIMP_LIB_PATH)
    message(FATAL_ERROR "Required Windows compiler sidecar asset not found: delayimp.lib")
  endif()

  set(CURSIVE_LLD_LINK_PATH "${LLVM_ROOT}/bin/lld-link.exe")
  set(CURSIVE_LLVM_LIB_PATH "${LLVM_ROOT}/bin/llvm-lib.exe")
  set(CURSIVE_LLVM_AS_PATH "${LLVM_ROOT}/bin/llvm-as.exe")
  set(CURSIVE_ICUDT_DLL_PATH "${ICU_BIN_DIR}/icudt72.dll")
  set(CURSIVE_ICUUC_DLL_PATH "${ICU_BIN_DIR}/icuuc72.dll")
  set(CURSIVE_ICUIN_DLL_PATH "${ICU_BIN_DIR}/icuin72.dll")

  foreach(sidecar_input IN ITEMS
      "${CURSIVE_DELAYIMP_LIB_PATH}"
      "${CURSIVE_LLD_LINK_PATH}"
      "${CURSIVE_LLVM_LIB_PATH}"
      "${CURSIVE_LLVM_AS_PATH}"
      "${CURSIVE_ICUDT_DLL_PATH}"
      "${CURSIVE_ICUUC_DLL_PATH}"
      "${CURSIVE_ICUIN_DLL_PATH}")
    if(NOT EXISTS "${sidecar_input}")
      message(FATAL_ERROR "Required Windows compiler sidecar asset not found: ${sidecar_input}")
    endif()
  endforeach()

  set(CURSIVE_COMPILER_SIDECAR_BIN_DIR "${CMAKE_BINARY_DIR}/bin")
  set(CURSIVE_COMPILER_SIDECAR_LIB_DIR "${CMAKE_BINARY_DIR}/lib")
  set(CURSIVE_COMPILER_SIDECAR_TOOLS_DIR "${CMAKE_BINARY_DIR}/tools")

  add_custom_command(TARGET cursive POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CURSIVE_COMPILER_SIDECAR_BIN_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CURSIVE_COMPILER_SIDECAR_LIB_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CURSIVE_COMPILER_SIDECAR_TOOLS_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CURSIVE_DELAYIMP_LIB_PATH}"
            "${CURSIVE_COMPILER_SIDECAR_LIB_DIR}/delayimp.lib"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CURSIVE_LLD_LINK_PATH}"
            "${CURSIVE_COMPILER_SIDECAR_TOOLS_DIR}/lld-link.exe"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CURSIVE_LLVM_LIB_PATH}"
            "${CURSIVE_COMPILER_SIDECAR_TOOLS_DIR}/llvm-lib.exe"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CURSIVE_LLVM_AS_PATH}"
            "${CURSIVE_COMPILER_SIDECAR_TOOLS_DIR}/llvm-as.exe"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CURSIVE_ICUDT_DLL_PATH}"
            "${CURSIVE_COMPILER_SIDECAR_BIN_DIR}/icudt72.dll"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CURSIVE_ICUUC_DLL_PATH}"
            "${CURSIVE_COMPILER_SIDECAR_BIN_DIR}/icuuc72.dll"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CURSIVE_ICUIN_DLL_PATH}"
            "${CURSIVE_COMPILER_SIDECAR_BIN_DIR}/icuin72.dll"
    COMMENT "Staging build-tree compiler sidecar assets"
  )
endif()

set(CURSIVE_OUT_DIR "${CMAKE_BINARY_DIR}/out")

if(WIN32)
  set(CURSIVE_OUT_RUNTIME_PATH "${CMAKE_BINARY_DIR}/runtime/cursive0_rt.lib")
  add_custom_target(cursive_out ALL
    DEPENDS cursive cursive0_rt
    COMMAND "${CMAKE_COMMAND}" -E rm -rf "${CURSIVE_OUT_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CURSIVE_OUT_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CURSIVE_OUT_DIR}/windows/bin"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CURSIVE_OUT_DIR}/windows/lib"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CURSIVE_OUT_DIR}/windows/tools"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:cursive>"
            "${CURSIVE_OUT_DIR}/$<TARGET_FILE_NAME:cursive>"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CURSIVE_OUT_RUNTIME_PATH}"
            "${CURSIVE_OUT_DIR}/cursive0_rt.lib"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CURSIVE_COMPILER_SIDECAR_BIN_DIR}/icudt72.dll"
            "${CURSIVE_OUT_DIR}/windows/bin/icudt72.dll"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CURSIVE_COMPILER_SIDECAR_BIN_DIR}/icuuc72.dll"
            "${CURSIVE_OUT_DIR}/windows/bin/icuuc72.dll"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CURSIVE_COMPILER_SIDECAR_BIN_DIR}/icuin72.dll"
            "${CURSIVE_OUT_DIR}/windows/bin/icuin72.dll"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CURSIVE_COMPILER_SIDECAR_LIB_DIR}/delayimp.lib"
            "${CURSIVE_OUT_DIR}/windows/lib/delayimp.lib"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CURSIVE_COMPILER_SIDECAR_TOOLS_DIR}/lld-link.exe"
            "${CURSIVE_OUT_DIR}/windows/tools/lld-link.exe"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CURSIVE_COMPILER_SIDECAR_TOOLS_DIR}/llvm-lib.exe"
            "${CURSIVE_OUT_DIR}/windows/tools/llvm-lib.exe"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CURSIVE_COMPILER_SIDECAR_TOOLS_DIR}/llvm-as.exe"
            "${CURSIVE_OUT_DIR}/windows/tools/llvm-as.exe"
    COMMENT "Packaging compiler distribution into build/out"
  )
else()
  add_custom_target(cursive_out ALL
    DEPENDS cursive cursive0_rt
    COMMAND "${CMAKE_COMMAND}" -E rm -rf "${CURSIVE_OUT_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CURSIVE_OUT_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:cursive>"
            "${CURSIVE_OUT_DIR}/$<TARGET_FILE_NAME:cursive>"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_BINARY_DIR}/runtime/libcursive0_rt.a"
            "${CURSIVE_OUT_DIR}/libcursive0_rt.a"
    COMMENT "Packaging compiler distribution into build/out"
  )
endif()
