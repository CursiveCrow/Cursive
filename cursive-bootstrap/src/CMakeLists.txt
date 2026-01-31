add_library(cursive0_core STATIC
  00_core/span.cpp
  00_core/diagnostics.cpp
  00_core/diagnostic_codes.cpp
  00_core/diagnostic_render.cpp
  00_core/diagnostic_messages.cpp
  00_core/host_primitives.cpp
  00_core/ub_model.cpp
  00_core/path.cpp
  00_core/source_load.cpp
  00_core/unicode.cpp
  00_core/symbols.cpp
  00_core/hash.cpp
  00_core/spec_trace.cpp
)

target_include_directories(cursive0_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(cursive0_core PUBLIC ${CURSIVE_ICU_LIBS})

target_compile_features(cursive0_core PUBLIC cxx_std_20)

add_library(cursive0_syntax STATIC
  02_syntax/lexer.cpp
  02_syntax/lexer_ident.cpp
  02_syntax/lexer_literals.cpp
  02_syntax/lexer_security.cpp
  02_syntax/lexer_ws.cpp
  02_syntax/keyword_policy.cpp
  02_syntax/parser.cpp
  02_syntax/parser_docs.cpp
  02_syntax/parser_angle.cpp
  02_syntax/parser_consume.cpp
  02_syntax/parser_recovery.cpp
  02_syntax/parser_items.cpp
  02_syntax/parser_paths.cpp
  02_syntax/parser_placeholders.cpp
  02_syntax/parser_expr.cpp
  02_syntax/parser_patterns.cpp
  02_syntax/parser_types.cpp
  02_syntax/parser_stmt.cpp
  02_syntax/tokenize.cpp
  02_syntax/token.cpp
)

target_link_libraries(cursive0_syntax PUBLIC cursive0_core)

target_include_directories(cursive0_syntax PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_features(cursive0_syntax PUBLIC cxx_std_20)

add_library(cursive0_analysis STATIC
  # resolve/
  03_analysis/resolve/scopes.cpp
  03_analysis/resolve/scopes_intro.cpp
  03_analysis/resolve/scopes_lookup.cpp
  03_analysis/resolve/collect_toplevel.cpp
  03_analysis/resolve/resolve_qual.cpp
  03_analysis/resolve/resolver_expr.cpp
  03_analysis/resolve/resolver_items.cpp
  03_analysis/resolve/resolver_modules.cpp
  03_analysis/resolve/resolver_pat.cpp
  03_analysis/resolve/resolver_types.cpp
  03_analysis/resolve/visibility.cpp
  # types/
  03_analysis/types/types.cpp
  03_analysis/types/type_lower.cpp
  03_analysis/types/type_lookup.cpp
  03_analysis/types/type_expr.cpp
  03_analysis/types/type_predicates.cpp
  03_analysis/types/type_wf.cpp
  03_analysis/types/type_layout.cpp
  # types/expr/
  03_analysis/types/expr/common.cpp
  03_analysis/types/expr/binary.cpp
  03_analysis/types/expr/unary.cpp
  03_analysis/types/expr/if.cpp
  03_analysis/types/expr/range.cpp
  03_analysis/types/expr/cast.cpp
  03_analysis/types/expr/deref.cpp
  03_analysis/types/expr/move.cpp
  03_analysis/types/expr/propagate.cpp
  03_analysis/types/expr/transmute.cpp
  03_analysis/types/expr/addr_of.cpp
  03_analysis/types/expr/field_access.cpp
  03_analysis/types/expr/tuple_access.cpp
  03_analysis/types/expr/alloc.cpp
  03_analysis/types/expr/record_literal.cpp
  03_analysis/types/expr/method_call.cpp
  03_analysis/types/expr/path.cpp
  03_analysis/types/expr/call.cpp
  03_analysis/types/expr/enum_literal.cpp
  03_analysis/types/expr/sizeof.cpp
  03_analysis/types/expr/alignof.cpp
  03_analysis/types/expr/array_repeat.cpp
  03_analysis/types/type_stmt.cpp
  03_analysis/types/type_concurrency.cpp
  03_analysis/types/type_pattern.cpp
  03_analysis/types/type_decls.cpp
  03_analysis/types/type_equiv.cpp
  03_analysis/types/type_match.cpp
  03_analysis/types/type_infer.cpp
  03_analysis/types/subtyping.cpp
  03_analysis/types/typecheck.cpp
  03_analysis/types/conformance.cpp
  03_analysis/types/literals.cpp
  # composite/
  03_analysis/composite/arrays_slices.cpp
  03_analysis/composite/tuples.cpp
  03_analysis/composite/unions.cpp
  03_analysis/composite/records.cpp
  03_analysis/composite/record_methods.cpp
  03_analysis/composite/enums.cpp
  03_analysis/composite/function_types.cpp
  03_analysis/composite/classes.cpp
  03_analysis/composite/class_linearization.cpp
  # modal/
  03_analysis/modal/modal.cpp
  03_analysis/modal/modal_fields.cpp
  03_analysis/modal/modal_transitions.cpp
  03_analysis/modal/modal_widen.cpp
  # memory/
  03_analysis/memory/borrow_bind.cpp
  03_analysis/memory/regions.cpp
  03_analysis/memory/region_type.cpp
  03_analysis/memory/safe_ptr.cpp
  03_analysis/memory/string_bytes.cpp
  03_analysis/memory/calls.cpp
  03_analysis/memory/init_planner.cpp
  # caps/
  03_analysis/caps/cap_system.cpp
  03_analysis/caps/cap_heap.cpp
  03_analysis/caps/cap_filesystem.cpp
  03_analysis/caps/cap_concurrency.cpp
  # keys/
  03_analysis/keys/key_context.cpp
  03_analysis/keys/key_conflict.cpp
  # generics/
  03_analysis/generics/monomorphize.cpp
  03_analysis/types/variance.cpp
  # contracts/
  03_analysis/contracts/contract_check.cpp
  03_analysis/contracts/verification.cpp
  # attributes/
  03_analysis/attributes/attribute_registry.cpp
  # imports/
  03_analysis/resolve/imports.cpp
)

target_link_libraries(cursive0_analysis PUBLIC cursive0_core cursive0_syntax cursive0_project)

target_include_directories(cursive0_analysis PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_features(cursive0_analysis PUBLIC cxx_std_20)

add_library(cursive0_codegen STATIC
  # abi/
  04_codegen/abi/abi_types.cpp
  04_codegen/abi/abi_params.cpp
  04_codegen/abi/abi_calls.cpp
  # layout/
  04_codegen/layout/layout_primitives.cpp
  04_codegen/layout/layout_dispatch.cpp
  04_codegen/layout/layout_records.cpp
  04_codegen/layout/layout_unions.cpp
  04_codegen/layout/layout_aggregates.cpp
  04_codegen/layout/layout_modal.cpp
  04_codegen/layout/layout_dynobj.cpp
  04_codegen/layout/layout_value_bits.cpp
  # lower/
  04_codegen/lower/lower_expr_core.cpp
  04_codegen/lower/lower_expr_calls.cpp
  04_codegen/lower/lower_expr_places.cpp
  04_codegen/lower/lower_stmt.cpp
  04_codegen/lower/lower_pat.cpp
  04_codegen/lower/lower_proc.cpp
  04_codegen/lower/lower_module.cpp
  # llvm/
  04_codegen/llvm/llvm_module.cpp
  04_codegen/llvm/llvm_types.cpp
  04_codegen/llvm/llvm_attrs.cpp
  04_codegen/llvm/llvm_call_abi.cpp
  04_codegen/llvm/llvm_ub_safe.cpp
  04_codegen/llvm/llvm_mem.cpp
  04_codegen/llvm/llvm_ir_utils.cpp
  # intrinsics (runtime interface)
  04_codegen/runtime/runtime_calls.cpp
  04_codegen/runtime/runtime_decls.cpp
  # top-level
  04_codegen/ir_model.cpp
  04_codegen/ir_dump.cpp
  04_codegen/ir_lowering.cpp
  04_codegen/mangle.cpp
  04_codegen/linkage.cpp
  04_codegen/checks.cpp
  04_codegen/cleanup.cpp
  04_codegen/globals.cpp
  04_codegen/init.cpp
  04_codegen/dyn_dispatch.cpp
  04_codegen/vtable_emit.cpp
  04_codegen/literal_emit.cpp
  04_codegen/binding_storage.cpp
  04_codegen/entrypoint.cpp
  04_codegen/poison_instrument.cpp
)

target_link_libraries(cursive0_codegen PUBLIC cursive0_analysis ${llvm_libs})

target_include_directories(cursive0_codegen PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_features(cursive0_codegen PUBLIC cxx_std_20)

add_library(cursive0_project STATIC
  01_project/manifest.cpp
  01_project/load_project.cpp
  01_project/module_discovery.cpp
  01_project/deterministic_order.cpp
  01_project/outputs.cpp
  01_project/ir_assembly.cpp
  01_project/tool_resolution.cpp
  01_project/link.cpp
  01_project/project_validate.cpp
  01_project/ident.cpp
)

target_link_libraries(cursive0_project PUBLIC cursive0_core)

target_include_directories(cursive0_project PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/tomlplusplus/include
)

target_compile_features(cursive0_project PUBLIC cxx_std_20)

add_library(cursive0_frontend STATIC
  02_syntax/parse_modules.cpp
)

target_link_libraries(cursive0_frontend PUBLIC cursive0_core cursive0_project cursive0_syntax cursive0_analysis)

target_include_directories(cursive0_frontend PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_features(cursive0_frontend PUBLIC cxx_std_20)

add_executable(cursivec0
  06_driver/main.cpp
)

target_link_libraries(cursivec0 PRIVATE cursive0_core cursive0_project cursive0_analysis cursive0_codegen cursive0_syntax cursive0_frontend)

target_include_directories(cursivec0 PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_features(cursivec0 PRIVATE cxx_std_20)
