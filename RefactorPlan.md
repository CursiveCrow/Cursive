cursive/src/
|-- 00_core/
|   |-- diagnostic_codes.cpp
|   |-- diagnostic_messages.cpp
|   |-- diagnostic_render.cpp
|   |-- diagnostics.cpp
|   |-- hash.cpp
|   |-- host_primitives.cpp
|   |-- ident.cpp
|   |-- path.cpp
|   |-- source_load.cpp
|   |-- span.cpp
|   |-- spec_trace.cpp
|   |-- symbols.cpp
|   |-- ub_model.cpp
|   |-- unicode.cpp
|-- 01_project/
|   |-- assemblies.cpp
|   |-- deterministic_order.cpp
|   |-- load_project.cpp
|   |-- manifest.cpp
|   |-- module_discovery.cpp
|   |-- outputs.cpp
|   |-- link.cpp
|   |-- ir_assembly.cpp
|   |-- tool_resolution.cpp
|   |-- unwind_ffi_surface.cpp
|   |-- project_validate.cpp
|-- 02_source/
|   |-- source/
|   |   |-- source_normalize.cpp
|   |   |-- source_map.cpp
|   |-- lexer/
|   |   |-- token.cpp
|   |   |-- tokenize.cpp
|   |   |-- lexer.cpp
|   |   |-- lexer_ident.cpp
|   |   |-- lexer_literals.cpp
|   |   |-- lexer_security.cpp
|   |   |-- lexer_ws.cpp
|   |   |-- keyword_policy.cpp
|   |-- parser/
|   |   |-- parser.cpp
|   |   |-- parser_state.cpp
|   |   |-- parser_consume.cpp
|   |   |-- parser_angle.cpp
|   |   |-- parser_paths.cpp
|   |   |-- parser_docs.cpp
|   |   |-- parser_recovery.cpp
|   |   |-- parse_modules.cpp
|   |   |-- expr/
|   |   |   |-- literal.cpp
|   |   |   |-- null_ptr.cpp
|   |   |   |-- identifier.cpp
|   |   |   |-- qualified_name.cpp
|   |   |   |-- qualified_apply.cpp
|   |   |   |-- path.cpp
|   |   |   |-- error_expr.cpp
|   |   |   |-- tuple_literal.cpp
|   |   |   |-- array_literal.cpp
|   |   |   |-- record_literal.cpp
|   |   |   |-- enum_literal.cpp
|   |   |   |-- field_access.cpp
|   |   |   |-- tuple_access.cpp
|   |   |   |-- index_access.cpp
|   |   |   |-- call.cpp
|   |   |   |-- call_type_args.cpp
|   |   |   |-- method_call.cpp
|   |   |   |-- unary.cpp
|   |   |   |-- binary.cpp
|   |   |   |-- cast.cpp
|   |   |   |-- range.cpp
|   |   |   |-- if_expr.cpp
|   |   |   |-- match_expr.cpp
|   |   |   |-- loop_infinite.cpp
|   |   |   |-- loop_conditional.cpp
|   |   |   |-- loop_iter.cpp
|   |   |   |-- block_expr.cpp
|   |   |   |-- unsafe_block_expr.cpp
|   |   |   |-- move_expr.cpp
|   |   |   |-- transmute_expr.cpp
|   |   |   |-- alloc_expr.cpp
|   |   |   |-- propagate_expr.cpp
|   |   |   |-- addr_of.cpp
|   |   |   |-- deref.cpp
|   |   |   |-- contract_result.cpp
|   |   |   |-- contract_entry.cpp
|   |   |   |-- parallel_expr.cpp
|   |   |   |-- spawn_expr.cpp
|   |   |   |-- dispatch_expr.cpp
|   |   |   |-- wait_expr.cpp
|   |   |   |-- yield_expr.cpp
|   |   |   |-- yield_from_expr.cpp
|   |   |   |-- sync_expr.cpp
|   |   |   |-- race_expr.cpp
|   |   |   |-- all_expr.cpp
|   |   |   |-- closure_expr.cpp
|   |   |   |-- pipeline_expr.cpp
|   |   |   |-- expr_common.cpp
|   |   |-- stmt/
|   |   |   |-- let_stmt.cpp
|   |   |   |-- var_stmt.cpp
|   |   |   |-- shadow_let_stmt.cpp
|   |   |   |-- shadow_var_stmt.cpp
|   |   |   |-- assign_stmt.cpp
|   |   |   |-- compound_assign_stmt.cpp
|   |   |   |-- expr_stmt.cpp
|   |   |   |-- defer_stmt.cpp
|   |   |   |-- region_stmt.cpp
|   |   |   |-- frame_stmt.cpp
|   |   |   |-- key_block_stmt.cpp
|   |   |   |-- return_stmt.cpp
|   |   |   |-- break_stmt.cpp
|   |   |   |-- continue_stmt.cpp
|   |   |   |-- unsafe_block_stmt.cpp
|   |   |   |-- error_stmt.cpp
|   |   |   |-- stmt_common.cpp
|   |   |-- item/
|   |   |   |-- import_decl.cpp
|   |   |   |-- using_decl.cpp
|   |   |   |-- extern_block.cpp
|   |   |   |-- static_decl.cpp
|   |   |   |-- procedure_decl.cpp
|   |   |   |-- record_decl.cpp
|   |   |   |-- enum_decl.cpp
|   |   |   |-- modal_decl.cpp
|   |   |   |-- class_decl.cpp
|   |   |   |-- type_alias_decl.cpp
|   |   |   |-- visibility.cpp
|   |   |   |-- attribute_list.cpp
|   |   |   |-- signature.cpp
|   |   |   |-- contract_clause.cpp
|   |   |   |-- foreign_contract_clause.cpp
|   |   |   |-- generic_params.cpp
|   |   |   |-- where_clause.cpp
|   |   |   |-- item_common.cpp
|   |   |-- type/
|   |   |   |-- primitive_type.cpp
|   |   |   |-- tuple_type.cpp
|   |   |   |-- function_type.cpp
|   |   |   |-- closure_type.cpp
|   |   |   |-- array_type.cpp
|   |   |   |-- slice_type.cpp
|   |   |   |-- safe_ptr_type.cpp
|   |   |   |-- raw_ptr_type.cpp
|   |   |   |-- string_type.cpp
|   |   |   |-- bytes_type.cpp
|   |   |   |-- dynamic_type.cpp
|   |   |   |-- state_specific_type.cpp
|   |   |   |-- type_path.cpp
|   |   |   |-- opaque_type.cpp
|   |   |   |-- union_type.cpp
|   |   |   |-- refinement_clause.cpp
|   |   |   |-- permission.cpp
|   |   |   |-- type_args.cpp
|   |   |   |-- type_bounds.cpp
|   |   |   |-- type_common.cpp
|   |   |-- pattern/
|   |   |   |-- literal_pattern.cpp
|   |   |   |-- wildcard_pattern.cpp
|   |   |   |-- identifier_pattern.cpp
|   |   |   |-- typed_pattern.cpp
|   |   |   |-- tuple_pattern.cpp
|   |   |   |-- record_pattern.cpp
|   |   |   |-- enum_pattern.cpp
|   |   |   |-- modal_pattern.cpp
|   |   |   |-- range_pattern.cpp
|   |   |   |-- pattern_common.cpp
|   |-- ast/
|   |   |-- ast_nodes.cpp
|   |   |-- ast_builder.cpp
|   |   |-- ast_dump.cpp
|   |   |-- ast_validate.cpp
|   |-- module/
|   |   |-- module_aggregate.cpp
|   |   |-- module_docs.cpp
|-- 03_comptime/
|   |-- comptime_gate.cpp
|   |-- deferred_attributes.cpp
|-- 04_analysis/
|   |-- conformance/
|   |   |-- conformance.cpp
|   |   |-- subset_checks.cpp
|   |-- resolve/
|   |   |-- scopes.cpp
|   |   |-- scopes_intro.cpp
|   |   |-- scopes_lookup.cpp
|   |   |-- resolve_module.cpp
|   |   |-- resolve_items.cpp
|   |   |-- resolve_types.cpp
|   |   |-- resolve_expr.cpp
|   |   |-- resolve_stmt.cpp
|   |   |-- resolve_pattern.cpp
|   |   |-- resolve_path.cpp
|   |   |-- resolve_qual.cpp
|   |   |-- resolve_expr_list.cpp
|   |   |-- resolve_enum_payload.cpp
|   |   |-- resolve_callee.cpp
|   |   |-- resolve_arm.cpp
|   |   |-- resolve_stmt_seq.cpp
|   |   |-- resolve_imports.cpp
|   |   |-- resolve_using.cpp
|   |   |-- resolve_extern.cpp
|   |   |-- resolve_attributes.cpp
|   |   |-- resolve_contracts.cpp
|   |   |-- resolve_generics.cpp
|   |-- typing/
|   |   |-- typecheck.cpp
|   |   |-- type_equiv.cpp
|   |   |-- subtyping.cpp
|   |   |-- type_infer.cpp
|   |   |-- type_wf.cpp
|   |   |-- const_len.cpp
|   |   |-- type_refs.cpp
|   |   |-- literals.cpp
|   |   |-- args_ok.cpp
|   |   |-- arg_pass.cpp
|   |   |-- match_check.cpp
|   |   |-- expr/
|   |   |   |-- literal.cpp
|   |   |   |-- null_ptr.cpp
|   |   |   |-- identifier.cpp
|   |   |   |-- qualified_name.cpp
|   |   |   |-- qualified_apply.cpp
|   |   |   |-- path.cpp
|   |   |   |-- error_expr.cpp
|   |   |   |-- tuple_literal.cpp
|   |   |   |-- array_literal.cpp
|   |   |   |-- record_literal.cpp
|   |   |   |-- enum_literal.cpp
|   |   |   |-- field_access.cpp
|   |   |   |-- tuple_access.cpp
|   |   |   |-- index_access.cpp
|   |   |   |-- call.cpp
|   |   |   |-- call_type_args.cpp
|   |   |   |-- method_call.cpp
|   |   |   |-- unary.cpp
|   |   |   |-- binary.cpp
|   |   |   |-- cast.cpp
|   |   |   |-- range.cpp
|   |   |   |-- if_expr.cpp
|   |   |   |-- match_expr.cpp
|   |   |   |-- loop_infinite.cpp
|   |   |   |-- loop_conditional.cpp
|   |   |   |-- loop_iter.cpp
|   |   |   |-- block_expr.cpp
|   |   |   |-- unsafe_block_expr.cpp
|   |   |   |-- move_expr.cpp
|   |   |   |-- transmute_expr.cpp
|   |   |   |-- alloc_expr.cpp
|   |   |   |-- propagate_expr.cpp
|   |   |   |-- addr_of.cpp
|   |   |   |-- deref.cpp
|   |   |   |-- contract_result.cpp
|   |   |   |-- contract_entry.cpp
|   |   |   |-- parallel_expr.cpp
|   |   |   |-- spawn_expr.cpp
|   |   |   |-- dispatch_expr.cpp
|   |   |   |-- wait_expr.cpp
|   |   |   |-- yield_expr.cpp
|   |   |   |-- yield_from_expr.cpp
|   |   |   |-- sync_expr.cpp
|   |   |   |-- race_expr.cpp
|   |   |   |-- all_expr.cpp
|   |   |   |-- closure_expr.cpp
|   |   |   |-- pipeline_expr.cpp
|   |   |   |-- expr_common.cpp
|   |   |-- stmt/
|   |   |   |-- let_stmt.cpp
|   |   |   |-- var_stmt.cpp
|   |   |   |-- shadow_let_stmt.cpp
|   |   |   |-- shadow_var_stmt.cpp
|   |   |   |-- assign_stmt.cpp
|   |   |   |-- compound_assign_stmt.cpp
|   |   |   |-- expr_stmt.cpp
|   |   |   |-- defer_stmt.cpp
|   |   |   |-- region_stmt.cpp
|   |   |   |-- frame_stmt.cpp
|   |   |   |-- key_block_stmt.cpp
|   |   |   |-- return_stmt.cpp
|   |   |   |-- break_stmt.cpp
|   |   |   |-- continue_stmt.cpp
|   |   |   |-- unsafe_block_stmt.cpp
|   |   |   |-- error_stmt.cpp
|   |   |   |-- stmt_common.cpp
|   |   |-- item/
|   |   |   |-- import_decl.cpp
|   |   |   |-- using_decl.cpp
|   |   |   |-- extern_block.cpp
|   |   |   |-- static_decl.cpp
|   |   |   |-- procedure_decl.cpp
|   |   |   |-- record_decl.cpp
|   |   |   |-- enum_decl.cpp
|   |   |   |-- modal_decl.cpp
|   |   |   |-- class_decl.cpp
|   |   |   |-- type_alias_decl.cpp
|   |   |   |-- visibility.cpp
|   |   |   |-- attribute_list.cpp
|   |   |   |-- signature.cpp
|   |   |   |-- contract_clause.cpp
|   |   |   |-- foreign_contract_clause.cpp
|   |   |   |-- generic_params.cpp
|   |   |   |-- where_clause.cpp
|   |   |   |-- item_common.cpp
|   |   |-- pattern/
|   |   |   |-- literal_pattern.cpp
|   |   |   |-- wildcard_pattern.cpp
|   |   |   |-- identifier_pattern.cpp
|   |   |   |-- typed_pattern.cpp
|   |   |   |-- tuple_pattern.cpp
|   |   |   |-- record_pattern.cpp
|   |   |   |-- enum_pattern.cpp
|   |   |   |-- modal_pattern.cpp
|   |   |   |-- range_pattern.cpp
|   |   |   |-- pattern_common.cpp
|   |-- composite/
|   |   |-- arrays_slices.cpp
|   |   |-- tuples.cpp
|   |   |-- unions.cpp
|   |   |-- records.cpp
|   |   |-- record_methods.cpp
|   |   |-- enums.cpp
|   |   |-- classes.cpp
|   |   |-- class_linearization.cpp
|   |   |-- function_types.cpp
|   |-- modal/
|   |   |-- modal.cpp
|   |   |-- modal_fields.cpp
|   |   |-- modal_transitions.cpp
|   |   |-- modal_widen.cpp
|   |-- memory/
|   |   |-- init_planner.cpp
|   |   |-- regions.cpp
|   |   |-- region_type.cpp
|   |   |-- safe_ptr.cpp
|   |   |-- string_bytes.cpp
|   |   |-- borrow_bind.cpp
|   |   |-- calls.cpp
|   |-- caps/
|   |   |-- authority_model.cpp
|   |   |-- context_caps.cpp
|   |   |-- cap_requirements.cpp
|   |   |-- callgraph_caps.cpp
|   |-- contracts/
|   |   |-- contract_check.cpp
|   |   |-- verification.cpp
|   |   |-- contract_purity.cpp
|   |   |-- contract_intrinsics.cpp
|   |-- attributes/
|   |   |-- attribute_registry.cpp
|   |   |-- attribute_targets.cpp
|   |   |-- attribute_validate.cpp
|   |-- keys/
|   |   |-- key_context.cpp
|   |   |-- key_conflict.cpp
|   |   |-- key_paths.cpp
|   |   |-- key_capture.cpp
|   |   |-- key_lifetimes.cpp
|   |-- concurrency/
|   |   |-- parallel.cpp
|   |   |-- spawn.cpp
|   |   |-- dispatch.cpp
|   |   |-- wait.cpp
|   |   |-- yield.cpp
|   |   |-- yield_from.cpp
|   |   |-- sync.cpp
|   |   |-- race.cpp
|   |   |-- all.cpp
|   |   |-- async.cpp
|   |-- provenance/
|   |   |-- prov_place.cpp
|   |   |-- prov_expr.cpp
|   |   |-- prov_stmt.cpp
|   |   |-- prov_block.cpp
|   |   |-- prov_args.cpp
|   |-- generics/
|   |   |-- generic_params.cpp
|   |   |-- where_bounds.cpp
|   |   |-- monomorphize.cpp
|-- 05_codegen/
|   |-- ir/
|   |   |-- ir_model.cpp
|   |   |-- ir_builder.cpp
|   |   |-- ir_dump.cpp
|   |-- lower/
|   |   |-- lower_module.cpp
|   |   |-- lower_proc.cpp
|   |   |-- lower_bind.cpp
|   |   |-- expr/
|   |   |   |-- literal.cpp
|   |   |   |-- null_ptr.cpp
|   |   |   |-- identifier.cpp
|   |   |   |-- qualified_name.cpp
|   |   |   |-- qualified_apply.cpp
|   |   |   |-- path.cpp
|   |   |   |-- error_expr.cpp
|   |   |   |-- tuple_literal.cpp
|   |   |   |-- array_literal.cpp
|   |   |   |-- record_literal.cpp
|   |   |   |-- enum_literal.cpp
|   |   |   |-- field_access.cpp
|   |   |   |-- tuple_access.cpp
|   |   |   |-- index_access.cpp
|   |   |   |-- call.cpp
|   |   |   |-- call_type_args.cpp
|   |   |   |-- method_call.cpp
|   |   |   |-- unary.cpp
|   |   |   |-- binary.cpp
|   |   |   |-- cast.cpp
|   |   |   |-- range.cpp
|   |   |   |-- if_expr.cpp
|   |   |   |-- match_expr.cpp
|   |   |   |-- loop_infinite.cpp
|   |   |   |-- loop_conditional.cpp
|   |   |   |-- loop_iter.cpp
|   |   |   |-- block_expr.cpp
|   |   |   |-- unsafe_block_expr.cpp
|   |   |   |-- move_expr.cpp
|   |   |   |-- transmute_expr.cpp
|   |   |   |-- alloc_expr.cpp
|   |   |   |-- propagate_expr.cpp
|   |   |   |-- addr_of.cpp
|   |   |   |-- deref.cpp
|   |   |   |-- contract_result.cpp
|   |   |   |-- contract_entry.cpp
|   |   |   |-- parallel_expr.cpp
|   |   |   |-- spawn_expr.cpp
|   |   |   |-- dispatch_expr.cpp
|   |   |   |-- wait_expr.cpp
|   |   |   |-- yield_expr.cpp
|   |   |   |-- yield_from_expr.cpp
|   |   |   |-- sync_expr.cpp
|   |   |   |-- race_expr.cpp
|   |   |   |-- all_expr.cpp
|   |   |   |-- closure_expr.cpp
|   |   |   |-- pipeline_expr.cpp
|   |   |   |-- expr_common.cpp
|   |   |-- stmt/
|   |   |   |-- let_stmt.cpp
|   |   |   |-- var_stmt.cpp
|   |   |   |-- shadow_let_stmt.cpp
|   |   |   |-- shadow_var_stmt.cpp
|   |   |   |-- assign_stmt.cpp
|   |   |   |-- compound_assign_stmt.cpp
|   |   |   |-- expr_stmt.cpp
|   |   |   |-- defer_stmt.cpp
|   |   |   |-- region_stmt.cpp
|   |   |   |-- frame_stmt.cpp
|   |   |   |-- key_block_stmt.cpp
|   |   |   |-- return_stmt.cpp
|   |   |   |-- break_stmt.cpp
|   |   |   |-- continue_stmt.cpp
|   |   |   |-- unsafe_block_stmt.cpp
|   |   |   |-- error_stmt.cpp
|   |   |   |-- stmt_common.cpp
|   |   |-- pattern/
|   |   |   |-- literal_pattern.cpp
|   |   |   |-- wildcard_pattern.cpp
|   |   |   |-- identifier_pattern.cpp
|   |   |   |-- typed_pattern.cpp
|   |   |   |-- tuple_pattern.cpp
|   |   |   |-- record_pattern.cpp
|   |   |   |-- enum_pattern.cpp
|   |   |   |-- modal_pattern.cpp
|   |   |   |-- range_pattern.cpp
|   |   |   |-- pattern_common.cpp
|   |-- layout/
|   |   |-- layout_primitives.cpp
|   |   |-- layout_records.cpp
|   |   |-- layout_unions.cpp
|   |   |-- layout_enums.cpp
|   |   |-- layout_tuples.cpp
|   |   |-- layout_ranges.cpp
|   |   |-- layout_modal.cpp
|   |   |-- layout_dynobj.cpp
|   |   |-- layout_aggregates.cpp
|   |   |-- layout_dispatch.cpp
|   |   |-- layout_value_bits.cpp
|   |-- abi/
|   |   |-- abi_types.cpp
|   |   |-- abi_params.cpp
|   |   |-- abi_returns.cpp
|   |   |-- abi_calls.cpp
|   |   |-- abi_lowering.cpp
|   |   |-- calling_conventions.cpp
|   |-- symbols/
|   |   |-- mangle.cpp
|   |   |-- linkage.cpp
|   |   |-- visibility.cpp
|   |   |-- symbol_table.cpp
|   |-- globals/
|   |   |-- globals.cpp
|   |   |-- init.cpp
|   |   |-- entrypoint.cpp
|   |   |-- literal_emit.cpp
|   |   |-- binding_storage.cpp
|   |-- cleanup/
|   |   |-- cleanup.cpp
|   |   |-- drop_hooks.cpp
|   |   |-- unwind.cpp
|   |-- intrinsics/
|   |   |-- intrinsics_decls.cpp
|   |   |-- intrinsics_calls.cpp
|   |   |-- intrinsics_interface.cpp
|   |   |-- builtins.cpp
|   |-- dyn_dispatch/
|   |   |-- dyn_dispatch.cpp
|   |   |-- vtable_emit.cpp
|   |-- checks/
|   |   |-- checks.cpp
|   |   |-- panic.cpp
|   |   |-- poison_instrument.cpp
|   |-- llvm/
|   |   |-- llvm_module.cpp
|   |   |-- llvm_types.cpp
|   |   |-- llvm_call.cpp
|   |   |-- llvm_emit.cpp
|   |   |-- llvm_attr.cpp
|   |   |-- llvm_ub_safe.cpp
|   |   |-- llvm_backend.cpp
|   |   |-- llvm_passes.cpp
|-- 06_driver/
|   |-- main.cpp
|   |-- cli.cpp
|   |-- options.cpp
|   |-- pipeline.cpp
|   |-- version.cpp

# Mapping: current files and content -> proposed organization

Notes:
- "SPLIT" indicates one source file distributes content across multiple target files.
- "NEW" indicates a target file that does not exist yet and must be created.
- "ADD TO PLAN" indicates the target file is required to house existing content but is missing from the hierarchy above.

## 00_core
- 00_core/diagnostic_codes.cpp -> 00_core/diagnostic_codes.cpp
- 00_core/diagnostic_messages.cpp -> 00_core/diagnostic_messages.cpp
- 00_core/diagnostic_render.cpp -> 00_core/diagnostic_render.cpp
- 00_core/diagnostics.cpp -> 00_core/diagnostics.cpp
- 00_core/hash.cpp -> 00_core/hash.cpp
- 00_core/host_primitives.cpp -> 00_core/host_primitives.cpp
- 00_core/path.cpp -> 00_core/path.cpp
- 00_core/source_load.cpp -> 00_core/source_load.cpp (optionally SPLIT into 02_source/source/source_normalize.cpp + 02_source/source/source_map.cpp)
- 00_core/span.cpp -> 00_core/span.cpp
- 00_core/spec_trace.cpp -> 00_core/spec_trace.cpp
- 00_core/symbols.cpp -> 00_core/symbols.cpp
- 00_core/ub_model.cpp -> 00_core/ub_model.cpp
- 00_core/unicode.cpp -> 00_core/unicode.cpp
- 01_project/ident.cpp -> 00_core/ident.cpp (move)

## 01_project
- 01_project/deterministic_order.cpp -> 01_project/deterministic_order.cpp
- 01_project/ir_assembly.cpp -> 01_project/ir_assembly.cpp
- 01_project/link.cpp -> 01_project/link.cpp (optionally SPLIT to NEW: assemblies.cpp, unwind_ffi_surface.cpp)
- 01_project/load_project.cpp -> 01_project/load_project.cpp
- 01_project/manifest.cpp -> 01_project/manifest.cpp
- 01_project/module_discovery.cpp -> 01_project/module_discovery.cpp
- 01_project/outputs.cpp -> 01_project/outputs.cpp
- 01_project/project_validate.cpp -> 01_project/project_validate.cpp
- 01_project/tool_resolution.cpp -> 01_project/tool_resolution.cpp
- NEW: 01_project/assemblies.cpp (extract assembly model from load_project.cpp/link.cpp if needed)
- NEW: 01_project/unwind_ffi_surface.cpp (extract FFI/unwind checks from link.cpp if needed)

## 02_source (from 02_syntax)
### Lexer/tokenization
- 02_syntax/token.cpp -> 02_source/lexer/token.cpp
- 02_syntax/tokenize.cpp -> 02_source/lexer/tokenize.cpp
- 02_syntax/lexer.cpp -> 02_source/lexer/lexer.cpp
- 02_syntax/lexer_ident.cpp -> 02_source/lexer/lexer_ident.cpp
- 02_syntax/lexer_literals.cpp -> 02_source/lexer/lexer_literals.cpp
- 02_syntax/lexer_security.cpp -> 02_source/lexer/lexer_security.cpp
- 02_syntax/lexer_ws.cpp -> 02_source/lexer/lexer_ws.cpp
- 02_syntax/keyword_policy.cpp -> 02_source/lexer/keyword_policy.cpp

### Parser core
- 02_syntax/parser.cpp -> 02_source/parser/parser.cpp (SPLIT common state into NEW: parser_state.cpp if desired)
- 02_syntax/parser_consume.cpp -> 02_source/parser/parser_consume.cpp
- 02_syntax/parser_angle.cpp -> 02_source/parser/parser_angle.cpp
- 02_syntax/parser_paths.cpp -> 02_source/parser/parser_paths.cpp
- 02_syntax/parser_docs.cpp -> 02_source/parser/parser_docs.cpp
- 02_syntax/parser_recovery.cpp -> 02_source/parser/parser_recovery.cpp
- 02_syntax/parser_placeholders.cpp -> 02_source/parser/parser_placeholders.cpp (or merge into parser_recovery.cpp)
- 02_syntax/parse_modules.cpp -> 02_source/parser/parse_modules.cpp
- NEW: 02_source/parser/parser_state.cpp (extract parser state helpers from parser.cpp/parser_consume.cpp)

### Parser: expressions (SPLIT from parser_expr.cpp)
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/literal.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/null_ptr.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/identifier.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/qualified_name.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/qualified_apply.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/path.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/error_expr.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/tuple_literal.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/array_literal.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/record_literal.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/enum_literal.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/field_access.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/tuple_access.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/index_access.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/call.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/call_type_args.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/method_call.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/unary.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/binary.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/cast.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/range.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/if_expr.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/match_expr.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/loop_infinite.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/loop_conditional.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/loop_iter.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/block_expr.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/unsafe_block_expr.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/move_expr.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/transmute_expr.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/alloc_expr.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/propagate_expr.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/addr_of.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/deref.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/contract_result.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/contract_entry.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/parallel_expr.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/spawn_expr.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/dispatch_expr.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/wait_expr.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/yield_expr.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/yield_from_expr.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/sync_expr.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/race_expr.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/all_expr.cpp
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/closure_expr.cpp (if supported)
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/pipeline_expr.cpp (if supported)
- 02_syntax/parser_expr.cpp -> 02_source/parser/expr/expr_common.cpp (precedence, shared helpers)

### Parser: statements (SPLIT from parser_stmt.cpp)
- 02_syntax/parser_stmt.cpp -> 02_source/parser/stmt/let_stmt.cpp
- 02_syntax/parser_stmt.cpp -> 02_source/parser/stmt/var_stmt.cpp
- 02_syntax/parser_stmt.cpp -> 02_source/parser/stmt/shadow_let_stmt.cpp
- 02_syntax/parser_stmt.cpp -> 02_source/parser/stmt/shadow_var_stmt.cpp
- 02_syntax/parser_stmt.cpp -> 02_source/parser/stmt/assign_stmt.cpp
- 02_syntax/parser_stmt.cpp -> 02_source/parser/stmt/compound_assign_stmt.cpp
- 02_syntax/parser_stmt.cpp -> 02_source/parser/stmt/expr_stmt.cpp
- 02_syntax/parser_stmt.cpp -> 02_source/parser/stmt/defer_stmt.cpp
- 02_syntax/parser_stmt.cpp -> 02_source/parser/stmt/region_stmt.cpp
- 02_syntax/parser_stmt.cpp -> 02_source/parser/stmt/frame_stmt.cpp
- 02_syntax/parser_stmt.cpp -> 02_source/parser/stmt/key_block_stmt.cpp
- 02_syntax/parser_stmt.cpp -> 02_source/parser/stmt/return_stmt.cpp
- 02_syntax/parser_stmt.cpp -> 02_source/parser/stmt/break_stmt.cpp
- 02_syntax/parser_stmt.cpp -> 02_source/parser/stmt/continue_stmt.cpp
- 02_syntax/parser_stmt.cpp -> 02_source/parser/stmt/unsafe_block_stmt.cpp
- 02_syntax/parser_stmt.cpp -> 02_source/parser/stmt/error_stmt.cpp
- 02_syntax/parser_stmt.cpp -> 02_source/parser/stmt/stmt_common.cpp

### Parser: items (SPLIT from parser_items.cpp)
- 02_syntax/parser_items.cpp -> 02_source/parser/item/import_decl.cpp
- 02_syntax/parser_items.cpp -> 02_source/parser/item/using_decl.cpp
- 02_syntax/parser_items.cpp -> 02_source/parser/item/extern_block.cpp
- 02_syntax/parser_items.cpp -> 02_source/parser/item/static_decl.cpp
- 02_syntax/parser_items.cpp -> 02_source/parser/item/procedure_decl.cpp
- 02_syntax/parser_items.cpp -> 02_source/parser/item/record_decl.cpp
- 02_syntax/parser_items.cpp -> 02_source/parser/item/enum_decl.cpp
- 02_syntax/parser_items.cpp -> 02_source/parser/item/modal_decl.cpp
- 02_syntax/parser_items.cpp -> 02_source/parser/item/class_decl.cpp
- 02_syntax/parser_items.cpp -> 02_source/parser/item/type_alias_decl.cpp
- 02_syntax/parser_items.cpp -> 02_source/parser/item/visibility.cpp
- 02_syntax/parser_items.cpp -> 02_source/parser/item/attribute_list.cpp
- 02_syntax/parser_items.cpp -> 02_source/parser/item/signature.cpp
- 02_syntax/parser_items.cpp -> 02_source/parser/item/contract_clause.cpp
- 02_syntax/parser_items.cpp -> 02_source/parser/item/foreign_contract_clause.cpp
- 02_syntax/parser_items.cpp -> 02_source/parser/item/generic_params.cpp
- 02_syntax/parser_items.cpp -> 02_source/parser/item/where_clause.cpp
- 02_syntax/parser_items.cpp -> 02_source/parser/item/item_common.cpp

### Parser: types (SPLIT from parser_types.cpp)
- 02_syntax/parser_types.cpp -> 02_source/parser/type/primitive_type.cpp
- 02_syntax/parser_types.cpp -> 02_source/parser/type/tuple_type.cpp
- 02_syntax/parser_types.cpp -> 02_source/parser/type/function_type.cpp
- 02_syntax/parser_types.cpp -> 02_source/parser/type/closure_type.cpp
- 02_syntax/parser_types.cpp -> 02_source/parser/type/array_type.cpp
- 02_syntax/parser_types.cpp -> 02_source/parser/type/slice_type.cpp
- 02_syntax/parser_types.cpp -> 02_source/parser/type/safe_ptr_type.cpp
- 02_syntax/parser_types.cpp -> 02_source/parser/type/raw_ptr_type.cpp
- 02_syntax/parser_types.cpp -> 02_source/parser/type/string_type.cpp
- 02_syntax/parser_types.cpp -> 02_source/parser/type/bytes_type.cpp
- 02_syntax/parser_types.cpp -> 02_source/parser/type/dynamic_type.cpp
- 02_syntax/parser_types.cpp -> 02_source/parser/type/state_specific_type.cpp
- 02_syntax/parser_types.cpp -> 02_source/parser/type/type_path.cpp
- 02_syntax/parser_types.cpp -> 02_source/parser/type/opaque_type.cpp
- 02_syntax/parser_types.cpp -> 02_source/parser/type/union_type.cpp
- 02_syntax/parser_types.cpp -> 02_source/parser/type/refinement_clause.cpp
- 02_syntax/parser_types.cpp -> 02_source/parser/type/permission.cpp
- 02_syntax/parser_types.cpp -> 02_source/parser/type/type_args.cpp
- 02_syntax/parser_types.cpp -> 02_source/parser/type/type_bounds.cpp
- 02_syntax/parser_types.cpp -> 02_source/parser/type/type_common.cpp

### Parser: patterns (SPLIT from parser_patterns.cpp)
- 02_syntax/parser_patterns.cpp -> 02_source/parser/pattern/literal_pattern.cpp
- 02_syntax/parser_patterns.cpp -> 02_source/parser/pattern/wildcard_pattern.cpp
- 02_syntax/parser_patterns.cpp -> 02_source/parser/pattern/identifier_pattern.cpp
- 02_syntax/parser_patterns.cpp -> 02_source/parser/pattern/typed_pattern.cpp
- 02_syntax/parser_patterns.cpp -> 02_source/parser/pattern/tuple_pattern.cpp
- 02_syntax/parser_patterns.cpp -> 02_source/parser/pattern/record_pattern.cpp
- 02_syntax/parser_patterns.cpp -> 02_source/parser/pattern/enum_pattern.cpp
- 02_syntax/parser_patterns.cpp -> 02_source/parser/pattern/modal_pattern.cpp
- 02_syntax/parser_patterns.cpp -> 02_source/parser/pattern/range_pattern.cpp
- 02_syntax/parser_patterns.cpp -> 02_source/parser/pattern/pattern_common.cpp

## 03_comptime
- NEW: 03_comptime/comptime_gate.cpp (already in plan)
- NEW: 03_comptime/deferred_attributes.cpp (already in plan)

## 04_analysis (from 03_analysis)
### attributes
- 03_analysis/attributes/attribute_registry.cpp -> 04_analysis/attributes/attribute_registry.cpp
  - SPLIT: attribute target tables -> NEW: 04_analysis/attributes/attribute_targets.cpp
  - SPLIT: validation rules -> NEW: 04_analysis/attributes/attribute_validate.cpp

### caps
- 03_analysis/caps/cap_concurrency.cpp -> 04_analysis/caps/cap_concurrency.cpp (ADD TO PLAN)
- 03_analysis/caps/cap_filesystem.cpp -> 04_analysis/caps/cap_filesystem.cpp (ADD TO PLAN)
- 03_analysis/caps/cap_heap.cpp -> 04_analysis/caps/cap_heap.cpp (ADD TO PLAN)
- 03_analysis/caps/cap_system.cpp -> 04_analysis/caps/cap_system.cpp (ADD TO PLAN)
- NEW: 04_analysis/caps/authority_model.cpp (extract shared rules from cap_* if needed)
- NEW: 04_analysis/caps/context_caps.cpp (extract Context shape/fields)
- NEW: 04_analysis/caps/cap_requirements.cpp (callsite capability analysis)
- NEW: 04_analysis/caps/callgraph_caps.cpp (cap requirement propagation)

### composite
- 03_analysis/composite/arrays_slices.cpp -> 04_analysis/composite/arrays_slices.cpp
- 03_analysis/composite/tuples.cpp -> 04_analysis/composite/tuples.cpp
- 03_analysis/composite/unions.cpp -> 04_analysis/composite/unions.cpp
- 03_analysis/composite/records.cpp -> 04_analysis/composite/records.cpp
- 03_analysis/composite/record_methods.cpp -> 04_analysis/composite/record_methods.cpp
- 03_analysis/composite/enums.cpp -> 04_analysis/composite/enums.cpp
- 03_analysis/composite/classes.cpp -> 04_analysis/composite/classes.cpp
- 03_analysis/composite/class_linearization.cpp -> 04_analysis/composite/class_linearization.cpp
- 03_analysis/composite/function_types.cpp -> 04_analysis/composite/function_types.cpp

### contracts
- 03_analysis/contracts/contract_check.cpp -> 04_analysis/contracts/contract_check.cpp
- 03_analysis/contracts/verification.cpp -> 04_analysis/contracts/verification.cpp
- NEW: 04_analysis/contracts/contract_purity.cpp (split from contract_check.cpp if present)
- NEW: 04_analysis/contracts/contract_intrinsics.cpp (split from contract_check.cpp if present)

### generics
- 03_analysis/generics/monomorphize.cpp -> 04_analysis/generics/monomorphize.cpp
- NEW: 04_analysis/generics/generic_params.cpp (extract from type lowering or parser if needed)
- NEW: 04_analysis/generics/where_bounds.cpp (extract bounds validation)

### keys
- 03_analysis/keys/key_context.cpp -> 04_analysis/keys/key_context.cpp
- 03_analysis/keys/key_conflict.cpp -> 04_analysis/keys/key_conflict.cpp
- NEW: 04_analysis/keys/key_paths.cpp (split from key_context.cpp if needed)
- NEW: 04_analysis/keys/key_capture.cpp (split from key_context.cpp or concurrency checks)
- NEW: 04_analysis/keys/key_lifetimes.cpp (split from key_context.cpp if needed)

### memory
- 03_analysis/memory/init_planner.cpp -> 04_analysis/memory/init_planner.cpp
- 03_analysis/memory/regions.cpp -> 04_analysis/memory/regions.cpp
- 03_analysis/memory/region_type.cpp -> 04_analysis/memory/region_type.cpp
- 03_analysis/memory/safe_ptr.cpp -> 04_analysis/memory/safe_ptr.cpp
- 03_analysis/memory/string_bytes.cpp -> 04_analysis/memory/string_bytes.cpp
- 03_analysis/memory/borrow_bind.cpp -> 04_analysis/memory/borrow_bind.cpp
- 03_analysis/memory/calls.cpp -> 04_analysis/memory/calls.cpp

### modal
- 03_analysis/modal/modal.cpp -> 04_analysis/modal/modal.cpp
- 03_analysis/modal/modal_fields.cpp -> 04_analysis/modal/modal_fields.cpp
- 03_analysis/modal/modal_transitions.cpp -> 04_analysis/modal/modal_transitions.cpp
- 03_analysis/modal/modal_widen.cpp -> 04_analysis/modal/modal_widen.cpp

### resolve (renames/splits)
- 03_analysis/resolve/collect_toplevel.cpp -> 04_analysis/resolve/resolve_items.cpp (merge with resolver_items.cpp)
- 03_analysis/resolve/imports.cpp -> 04_analysis/resolve/resolve_imports.cpp
- 03_analysis/resolve/resolve_qual.cpp -> 04_analysis/resolve/resolve_qual.cpp
- 03_analysis/resolve/resolver_expr.cpp -> 04_analysis/resolve/resolve_expr.cpp
- 03_analysis/resolve/resolver_items.cpp -> 04_analysis/resolve/resolve_items.cpp
- 03_analysis/resolve/resolver_modules.cpp -> 04_analysis/resolve/resolve_module.cpp
- 03_analysis/resolve/resolver_pat.cpp -> 04_analysis/resolve/resolve_pattern.cpp
- 03_analysis/resolve/resolver_types.cpp -> 04_analysis/resolve/resolve_types.cpp
- 03_analysis/resolve/scopes.cpp -> 04_analysis/resolve/scopes.cpp
- 03_analysis/resolve/scopes_intro.cpp -> 04_analysis/resolve/scopes_intro.cpp
- 03_analysis/resolve/scopes_lookup.cpp -> 04_analysis/resolve/scopes_lookup.cpp
- 03_analysis/resolve/visibility.cpp -> 04_analysis/resolve/visibility.cpp (ADD TO PLAN)
- NEW: 04_analysis/resolve/resolve_stmt.cpp (split from resolve_expr/resolve_items as needed)
- NEW: 04_analysis/resolve/resolve_stmt_seq.cpp (split from resolve_stmt.cpp)
- NEW: 04_analysis/resolve/resolve_expr_list.cpp (split from resolve_expr.cpp)
- NEW: 04_analysis/resolve/resolve_enum_payload.cpp (split from resolve_items.cpp)
- NEW: 04_analysis/resolve/resolve_callee.cpp (split from resolve_expr.cpp)
- NEW: 04_analysis/resolve/resolve_arm.cpp (split from resolve_expr.cpp)
- NEW: 04_analysis/resolve/resolve_using.cpp (split from resolve_imports.cpp)
- NEW: 04_analysis/resolve/resolve_extern.cpp (split from resolve_items.cpp)
- NEW: 04_analysis/resolve/resolve_attributes.cpp (split from resolve_items.cpp)
- NEW: 04_analysis/resolve/resolve_contracts.cpp (split from resolve_items.cpp)
- NEW: 04_analysis/resolve/resolve_generics.cpp (split from resolve_types.cpp)

### typing
- 03_analysis/types/conformance.cpp -> 04_analysis/conformance/conformance.cpp
- 03_analysis/types/typecheck.cpp -> 04_analysis/typing/typecheck.cpp
- 03_analysis/types/type_equiv.cpp -> 04_analysis/typing/type_equiv.cpp
- 03_analysis/types/subtyping.cpp -> 04_analysis/typing/subtyping.cpp
- 03_analysis/types/type_infer.cpp -> 04_analysis/typing/type_infer.cpp
- 03_analysis/types/type_wf.cpp -> 04_analysis/typing/type_wf.cpp
- 03_analysis/types/type_predicates.cpp -> 04_analysis/typing/type_predicates.cpp (ADD TO PLAN)
- 03_analysis/types/type_lower.cpp -> 04_analysis/typing/type_lower.cpp (ADD TO PLAN)
- 03_analysis/types/type_lookup.cpp -> 04_analysis/typing/type_lookup.cpp (ADD TO PLAN)
- 03_analysis/types/types.cpp -> 04_analysis/typing/types.cpp (ADD TO PLAN)
- 03_analysis/types/variance.cpp -> 04_analysis/typing/variance.cpp (ADD TO PLAN)
- 03_analysis/types/literals.cpp -> 04_analysis/typing/literals.cpp
- 03_analysis/types/type_match.cpp -> 04_analysis/typing/match_check.cpp
- 03_analysis/types/type_pattern.cpp -> SPLIT into 04_analysis/typing/pattern/* (see below)
- 03_analysis/types/type_stmt.cpp -> SPLIT into 04_analysis/typing/stmt/* (see below)
- 03_analysis/types/type_decls.cpp -> SPLIT into 04_analysis/typing/item/* (see below)
- 03_analysis/types/type_expr.cpp -> SPLIT into 04_analysis/typing/expr/* + 04_analysis/typing/typecheck.cpp (dispatcher) + 04_analysis/typing/expr/expr_common.cpp
- 03_analysis/types/type_concurrency.cpp -> 04_analysis/concurrency/*.cpp (parallel/spawn/dispatch/wait/yield/sync/race/all) and/or typing/expr equivalents
- 03_analysis/types/type_layout.cpp -> move to 05_codegen/layout/* (preferred) or ADD 04_analysis/typing/type_layout.cpp if kept in analysis

### typing/expr (from 03_analysis/types/expr)
- 03_analysis/types/expr/addr_of.cpp -> 04_analysis/typing/expr/addr_of.cpp
- 03_analysis/types/expr/alignof.cpp -> 04_analysis/typing/expr/alignof.cpp (ADD TO PLAN)
- 03_analysis/types/expr/alloc.cpp -> 04_analysis/typing/expr/alloc_expr.cpp (rename)
- 03_analysis/types/expr/array_repeat.cpp -> 04_analysis/typing/expr/array_repeat.cpp (ADD TO PLAN)
- 03_analysis/types/expr/binary.cpp -> 04_analysis/typing/expr/binary.cpp
- 03_analysis/types/expr/call.cpp -> 04_analysis/typing/expr/call.cpp
- 03_analysis/types/expr/cast.cpp -> 04_analysis/typing/expr/cast.cpp
- 03_analysis/types/expr/common.cpp -> 04_analysis/typing/expr/expr_common.cpp
- 03_analysis/types/expr/deref.cpp -> 04_analysis/typing/expr/deref.cpp
- 03_analysis/types/expr/enum_literal.cpp -> 04_analysis/typing/expr/enum_literal.cpp
- 03_analysis/types/expr/field_access.cpp -> 04_analysis/typing/expr/field_access.cpp
- 03_analysis/types/expr/if.cpp -> 04_analysis/typing/expr/if_expr.cpp (rename)
- 03_analysis/types/expr/method_call.cpp -> 04_analysis/typing/expr/method_call.cpp
- 03_analysis/types/expr/move.cpp -> 04_analysis/typing/expr/move_expr.cpp (rename)
- 03_analysis/types/expr/path.cpp -> 04_analysis/typing/expr/path.cpp
- 03_analysis/types/expr/propagate.cpp -> 04_analysis/typing/expr/propagate_expr.cpp (rename)
- 03_analysis/types/expr/range.cpp -> 04_analysis/typing/expr/range.cpp
- 03_analysis/types/expr/record_literal.cpp -> 04_analysis/typing/expr/record_literal.cpp
- 03_analysis/types/expr/sizeof.cpp -> 04_analysis/typing/expr/sizeof.cpp (ADD TO PLAN)
- 03_analysis/types/expr/transmute.cpp -> 04_analysis/typing/expr/transmute_expr.cpp (rename)
- 03_analysis/types/expr/tuple_access.cpp -> 04_analysis/typing/expr/tuple_access.cpp
- 03_analysis/types/expr/unary.cpp -> 04_analysis/typing/expr/unary.cpp

### typing/pattern (from type_pattern.cpp)
- 03_analysis/types/type_pattern.cpp -> 04_analysis/typing/pattern/literal_pattern.cpp
- 03_analysis/types/type_pattern.cpp -> 04_analysis/typing/pattern/wildcard_pattern.cpp
- 03_analysis/types/type_pattern.cpp -> 04_analysis/typing/pattern/identifier_pattern.cpp
- 03_analysis/types/type_pattern.cpp -> 04_analysis/typing/pattern/typed_pattern.cpp
- 03_analysis/types/type_pattern.cpp -> 04_analysis/typing/pattern/tuple_pattern.cpp
- 03_analysis/types/type_pattern.cpp -> 04_analysis/typing/pattern/record_pattern.cpp
- 03_analysis/types/type_pattern.cpp -> 04_analysis/typing/pattern/enum_pattern.cpp
- 03_analysis/types/type_pattern.cpp -> 04_analysis/typing/pattern/modal_pattern.cpp
- 03_analysis/types/type_pattern.cpp -> 04_analysis/typing/pattern/range_pattern.cpp
- 03_analysis/types/type_pattern.cpp -> 04_analysis/typing/pattern/pattern_common.cpp

### typing/stmt (from type_stmt.cpp)
- 03_analysis/types/type_stmt.cpp -> 04_analysis/typing/stmt/let_stmt.cpp
- 03_analysis/types/type_stmt.cpp -> 04_analysis/typing/stmt/var_stmt.cpp
- 03_analysis/types/type_stmt.cpp -> 04_analysis/typing/stmt/shadow_let_stmt.cpp
- 03_analysis/types/type_stmt.cpp -> 04_analysis/typing/stmt/shadow_var_stmt.cpp
- 03_analysis/types/type_stmt.cpp -> 04_analysis/typing/stmt/assign_stmt.cpp
- 03_analysis/types/type_stmt.cpp -> 04_analysis/typing/stmt/compound_assign_stmt.cpp
- 03_analysis/types/type_stmt.cpp -> 04_analysis/typing/stmt/expr_stmt.cpp
- 03_analysis/types/type_stmt.cpp -> 04_analysis/typing/stmt/defer_stmt.cpp
- 03_analysis/types/type_stmt.cpp -> 04_analysis/typing/stmt/region_stmt.cpp
- 03_analysis/types/type_stmt.cpp -> 04_analysis/typing/stmt/frame_stmt.cpp
- 03_analysis/types/type_stmt.cpp -> 04_analysis/typing/stmt/key_block_stmt.cpp
- 03_analysis/types/type_stmt.cpp -> 04_analysis/typing/stmt/return_stmt.cpp
- 03_analysis/types/type_stmt.cpp -> 04_analysis/typing/stmt/break_stmt.cpp
- 03_analysis/types/type_stmt.cpp -> 04_analysis/typing/stmt/continue_stmt.cpp
- 03_analysis/types/type_stmt.cpp -> 04_analysis/typing/stmt/unsafe_block_stmt.cpp
- 03_analysis/types/type_stmt.cpp -> 04_analysis/typing/stmt/error_stmt.cpp
- 03_analysis/types/type_stmt.cpp -> 04_analysis/typing/stmt/stmt_common.cpp

### typing/item (from type_decls.cpp)
- 03_analysis/types/type_decls.cpp -> 04_analysis/typing/item/import_decl.cpp
- 03_analysis/types/type_decls.cpp -> 04_analysis/typing/item/using_decl.cpp
- 03_analysis/types/type_decls.cpp -> 04_analysis/typing/item/extern_block.cpp
- 03_analysis/types/type_decls.cpp -> 04_analysis/typing/item/static_decl.cpp
- 03_analysis/types/type_decls.cpp -> 04_analysis/typing/item/procedure_decl.cpp
- 03_analysis/types/type_decls.cpp -> 04_analysis/typing/item/record_decl.cpp
- 03_analysis/types/type_decls.cpp -> 04_analysis/typing/item/enum_decl.cpp
- 03_analysis/types/type_decls.cpp -> 04_analysis/typing/item/modal_decl.cpp
- 03_analysis/types/type_decls.cpp -> 04_analysis/typing/item/class_decl.cpp
- 03_analysis/types/type_decls.cpp -> 04_analysis/typing/item/type_alias_decl.cpp
- 03_analysis/types/type_decls.cpp -> 04_analysis/typing/item/visibility.cpp
- 03_analysis/types/type_decls.cpp -> 04_analysis/typing/item/attribute_list.cpp
- 03_analysis/types/type_decls.cpp -> 04_analysis/typing/item/signature.cpp
- 03_analysis/types/type_decls.cpp -> 04_analysis/typing/item/contract_clause.cpp
- 03_analysis/types/type_decls.cpp -> 04_analysis/typing/item/foreign_contract_clause.cpp
- 03_analysis/types/type_decls.cpp -> 04_analysis/typing/item/generic_params.cpp
- 03_analysis/types/type_decls.cpp -> 04_analysis/typing/item/where_clause.cpp
- 03_analysis/types/type_decls.cpp -> 04_analysis/typing/item/item_common.cpp

## 05_codegen (from 04_codegen)
### abi
- 04_codegen/abi/abi_types.cpp -> 05_codegen/abi/abi_types.cpp
- 04_codegen/abi/abi_params.cpp -> 05_codegen/abi/abi_params.cpp
- 04_codegen/abi/abi_calls.cpp -> 05_codegen/abi/abi_calls.cpp
- NEW: 05_codegen/abi/abi_returns.cpp (split from abi_calls if needed)
- NEW: 05_codegen/abi/abi_lowering.cpp (split from abi_calls if needed)
- NEW: 05_codegen/abi/calling_conventions.cpp (extract from linkage/abi if needed)

### ir
- 04_codegen/ir_model.cpp -> 05_codegen/ir/ir_model.cpp
- 04_codegen/ir_dump.cpp -> 05_codegen/ir/ir_dump.cpp
- 04_codegen/ir_lowering.cpp -> SPLIT into 05_codegen/lower/* + NEW: 05_codegen/ir/ir_builder.cpp

### lower
- 04_codegen/lower/lower_module.cpp -> 05_codegen/lower/lower_module.cpp
- 04_codegen/lower/lower_proc.cpp -> 05_codegen/lower/lower_proc.cpp
- 04_codegen/lower/lower_expr_core.cpp -> SPLIT into 05_codegen/lower/expr/* + expr_common.cpp
- 04_codegen/lower/lower_expr_calls.cpp -> SPLIT into 05_codegen/lower/expr/call.cpp, method_call.cpp, call_type_args.cpp
- 04_codegen/lower/lower_expr_places.cpp -> SPLIT into 05_codegen/lower/expr/field_access.cpp, tuple_access.cpp, index_access.cpp, addr_of.cpp, deref.cpp
- 04_codegen/lower/lower_stmt.cpp -> SPLIT into 05_codegen/lower/stmt/* + stmt_common.cpp
- 04_codegen/lower/lower_pat.cpp -> SPLIT into 05_codegen/lower/pattern/* + pattern_common.cpp

### layout
- 04_codegen/layout/layout_primitives.cpp -> 05_codegen/layout/layout_primitives.cpp
- 04_codegen/layout/layout_records.cpp -> 05_codegen/layout/layout_records.cpp
- 04_codegen/layout/layout_unions.cpp -> 05_codegen/layout/layout_unions.cpp
- 04_codegen/layout/layout_modal.cpp -> 05_codegen/layout/layout_modal.cpp
- 04_codegen/layout/layout_dynobj.cpp -> 05_codegen/layout/layout_dynobj.cpp
- 04_codegen/layout/layout_aggregates.cpp -> SPLIT into NEW: layout_tuples.cpp, layout_enums.cpp, layout_ranges.cpp (keep shared helpers in layout_aggregates.cpp)
- 04_codegen/layout/layout_dispatch.cpp -> 05_codegen/layout/layout_dispatch.cpp
- 04_codegen/layout/layout_value_bits.cpp -> 05_codegen/layout/layout_value_bits.cpp

### symbols/globals/intrinsics/cleanup/checks
- 04_codegen/linkage.cpp -> 05_codegen/symbols/linkage.cpp
- 04_codegen/mangle.cpp -> 05_codegen/symbols/mangle.cpp
- 04_codegen/binding_storage.cpp -> 05_codegen/globals/binding_storage.cpp
- 04_codegen/globals.cpp -> 05_codegen/globals/globals.cpp
- 04_codegen/init.cpp -> 05_codegen/globals/init.cpp
- 04_codegen/entrypoint.cpp -> 05_codegen/globals/entrypoint.cpp
- 04_codegen/literal_emit.cpp -> 05_codegen/globals/literal_emit.cpp
- 04_codegen/cleanup.cpp -> 05_codegen/cleanup/cleanup.cpp
- 04_codegen/checks.cpp -> 05_codegen/checks/checks.cpp
- 04_codegen/poison_instrument.cpp -> 05_codegen/checks/poison_instrument.cpp
- 04_codegen/dyn_dispatch.cpp -> 05_codegen/dyn_dispatch/dyn_dispatch.cpp
- 04_codegen/vtable_emit.cpp -> 05_codegen/dyn_dispatch/vtable_emit.cpp
- 04_codegen/runtime/runtime_calls.cpp -> 05_codegen/intrinsics/intrinsics_calls.cpp (rename)
- 04_codegen/runtime/runtime_decls.cpp -> 05_codegen/intrinsics/intrinsics_decls.cpp (rename)
- NEW: 05_codegen/intrinsics/intrinsics_interface.cpp (split from intrinsics_decls.cpp if needed)
- NEW: 05_codegen/intrinsics/builtins.cpp (split from intrinsics_decls.cpp if needed)
- NEW: 05_codegen/checks/panic.cpp (split from checks.cpp if needed)
- NEW: 05_codegen/cleanup/drop_hooks.cpp (split from cleanup.cpp if needed)
- NEW: 05_codegen/cleanup/unwind.cpp (split from cleanup.cpp if needed)

### llvm
- 04_codegen/llvm/llvm_module.cpp -> 05_codegen/llvm/llvm_module.cpp
- 04_codegen/llvm/llvm_types.cpp -> 05_codegen/llvm/llvm_types.cpp
- 04_codegen/llvm/llvm_attrs.cpp -> 05_codegen/llvm/llvm_attr.cpp (rename)
- 04_codegen/llvm/llvm_call_abi.cpp -> 05_codegen/llvm/llvm_call.cpp (rename)
- 04_codegen/llvm/llvm_ir_utils.cpp -> 05_codegen/llvm/llvm_backend.cpp (rename)
- 04_codegen/llvm/llvm_mem.cpp -> 05_codegen/llvm/llvm_emit.cpp (rename)
- 04_codegen/llvm/llvm_ub_safe.cpp -> 05_codegen/llvm/llvm_ub_safe.cpp
- NEW: 05_codegen/llvm/llvm_passes.cpp (split from llvm_backend.cpp if needed)

## 06_driver
- 06_driver/main.cpp -> 06_driver/main.cpp (SPLIT into cli.cpp, options.cpp, pipeline.cpp, version.cpp if desired)
- NEW: 06_driver/cli.cpp (split from main.cpp)
- NEW: 06_driver/options.cpp (split from main.cpp)
- NEW: 06_driver/pipeline.cpp (split from main.cpp)
- NEW: 06_driver/version.cpp (split from main.cpp)

# Header mapping (public include/)
Policy: public headers live under include mirroring the new src hierarchy; private headers (if introduced) live next to their .cpp in src.

## include/00_core
- include/00_core/assert_spec.h -> include/00_core/assert_spec.h
- include/00_core/diagnostic_codes.h -> include/00_core/diagnostic_codes.h
- include/00_core/diagnostic_messages.h -> include/00_core/diagnostic_messages.h
- include/00_core/diagnostic_render.h -> include/00_core/diagnostic_render.h
- include/00_core/diagnostics.h -> include/00_core/diagnostics.h
- include/00_core/hash.h -> include/00_core/hash.h
- include/00_core/host_primitives.h -> include/00_core/host_primitives.h
- include/00_core/int128.h -> include/00_core/int128.h
- include/00_core/keywords.h -> include/00_core/keywords.h
- include/00_core/path.h -> include/00_core/path.h
- include/00_core/source_load.h -> include/00_core/source_load.h
- include/00_core/source_text.h -> include/00_core/source_text.h
- include/00_core/span.h -> include/00_core/span.h
- include/00_core/spec_trace.h -> include/00_core/spec_trace.h
- include/00_core/symbols.h -> include/00_core/symbols.h
- include/00_core/ub_model.h -> include/00_core/ub_model.h
- include/00_core/unicode.h -> include/00_core/unicode.h
- include/01_project/ident.h -> include/00_core/ident.h (move with ident.cpp)

## include/01_project
- include/01_project/deterministic_order.h -> include/01_project/deterministic_order.h
- include/01_project/ir_assembly.h -> include/01_project/ir_assembly.h
- include/01_project/link.h -> include/01_project/link.h
- include/01_project/manifest.h -> include/01_project/manifest.h
- include/01_project/module_discovery.h -> include/01_project/module_discovery.h
- include/01_project/outputs.h -> include/01_project/outputs.h
- include/01_project/project.h -> include/01_project/project.h
- include/01_project/tool_resolution.h -> include/01_project/tool_resolution.h

## include/02_source (from 02_syntax)
- include/02_syntax/ast.h -> include/02_source/ast/ast.h
- include/02_syntax/keyword_policy.h -> include/02_source/lexer/keyword_policy.h
- include/02_syntax/lexer.h -> include/02_source/lexer/lexer.h
- include/02_syntax/parse_modules.h -> include/02_source/parser/parse_modules.h
- include/02_syntax/parser.h -> include/02_source/parser/parser.h
- include/02_syntax/token.h -> include/02_source/lexer/token.h

## include/03_comptime
- NEW: include/03_comptime/comptime_gate.h (if you expose the gate)
- NEW: include/03_comptime/deferred_attributes.h (if you expose the gate)

## include/04_analysis (from 03_analysis)
### attributes
- include/03_analysis/attributes/attribute_registry.h -> include/04_analysis/attributes/attribute_registry.h
- NEW: include/04_analysis/attributes/attribute_targets.h (if split)
- NEW: include/04_analysis/attributes/attribute_validate.h (if split)

### caps
- include/03_analysis/caps/cap_concurrency.h -> include/04_analysis/caps/cap_concurrency.h
- include/03_analysis/caps/cap_filesystem.h -> include/04_analysis/caps/cap_filesystem.h
- include/03_analysis/caps/cap_heap.h -> include/04_analysis/caps/cap_heap.h
- include/03_analysis/caps/cap_system.h -> include/04_analysis/caps/cap_system.h
- NEW: include/04_analysis/caps/authority_model.h (if added)
- NEW: include/04_analysis/caps/context_caps.h (if added)
- NEW: include/04_analysis/caps/cap_requirements.h (if added)
- NEW: include/04_analysis/caps/callgraph_caps.h (if added)

### composite
- include/03_analysis/composite/arrays_slices.h -> include/04_analysis/composite/arrays_slices.h
- include/03_analysis/composite/classes.h -> include/04_analysis/composite/classes.h
- include/03_analysis/composite/enums.h -> include/04_analysis/composite/enums.h
- include/03_analysis/composite/function_types.h -> include/04_analysis/composite/function_types.h
- include/03_analysis/composite/record_methods.h -> include/04_analysis/composite/record_methods.h
- include/03_analysis/composite/records.h -> include/04_analysis/composite/records.h
- include/03_analysis/composite/tuples.h -> include/04_analysis/composite/tuples.h
- include/03_analysis/composite/unions.h -> include/04_analysis/composite/unions.h

### contracts
- include/03_analysis/contracts/contract_check.h -> include/04_analysis/contracts/contract_check.h
- include/03_analysis/contracts/verification.h -> include/04_analysis/contracts/verification.h
- NEW: include/04_analysis/contracts/contract_purity.h (if split)
- NEW: include/04_analysis/contracts/contract_intrinsics.h (if split)

### generics
- include/03_analysis/generics/monomorphize.h -> include/04_analysis/generics/monomorphize.h
- NEW: include/04_analysis/generics/generic_params.h (if split)
- NEW: include/04_analysis/generics/where_bounds.h (if split)

### keys
- include/03_analysis/keys/key_context.h -> include/04_analysis/keys/key_context.h
- include/03_analysis/keys/key_conflict.h -> include/04_analysis/keys/key_conflict.h
- NEW: include/04_analysis/keys/key_paths.h (if split)
- NEW: include/04_analysis/keys/key_capture.h (if split)
- NEW: include/04_analysis/keys/key_lifetimes.h (if split)

### memory
- include/03_analysis/memory/borrow_bind.h -> include/04_analysis/memory/borrow_bind.h
- include/03_analysis/memory/calls.h -> include/04_analysis/memory/calls.h
- include/03_analysis/memory/init_planner.h -> include/04_analysis/memory/init_planner.h
- include/03_analysis/memory/region_type.h -> include/04_analysis/memory/region_type.h
- include/03_analysis/memory/regions.h -> include/04_analysis/memory/regions.h
- include/03_analysis/memory/safe_ptr.h -> include/04_analysis/memory/safe_ptr.h
- include/03_analysis/memory/string_bytes.h -> include/04_analysis/memory/string_bytes.h

### modal
- include/03_analysis/modal/modal.h -> include/04_analysis/modal/modal.h
- include/03_analysis/modal/modal_fields.h -> include/04_analysis/modal/modal_fields.h
- include/03_analysis/modal/modal_transitions.h -> include/04_analysis/modal/modal_transitions.h
- include/03_analysis/modal/modal_widen.h -> include/04_analysis/modal/modal_widen.h

### resolve
- include/03_analysis/resolve/collect_toplevel.h -> include/04_analysis/resolve/resolve_items.h (rename)
- include/03_analysis/resolve/imports.h -> include/04_analysis/resolve/resolve_imports.h (rename)
- include/03_analysis/resolve/resolve_qual.h -> include/04_analysis/resolve/resolve_qual.h
- include/03_analysis/resolve/resolver.h -> include/04_analysis/resolve/resolver.h
- include/03_analysis/resolve/scopes.h -> include/04_analysis/resolve/scopes.h
- include/03_analysis/resolve/scopes_intro.h -> include/04_analysis/resolve/scopes_intro.h
- include/03_analysis/resolve/scopes_lookup.h -> include/04_analysis/resolve/scopes_lookup.h
- include/03_analysis/resolve/visibility.h -> include/04_analysis/resolve/visibility.h

### typing (from 03_analysis/types)
- include/03_analysis/types/conformance.h -> include/04_analysis/conformance/conformance.h
- include/03_analysis/types/context.h -> include/04_analysis/typing/context.h
- include/03_analysis/types/literals.h -> include/04_analysis/typing/literals.h
- include/03_analysis/types/place_types.h -> include/04_analysis/typing/place_types.h
- include/03_analysis/types/subtyping.h -> include/04_analysis/typing/subtyping.h
- include/03_analysis/types/type_decls.h -> include/04_analysis/typing/type_decls.h
- include/03_analysis/types/type_equiv.h -> include/04_analysis/typing/type_equiv.h
- include/03_analysis/types/type_expr.h -> include/04_analysis/typing/type_expr.h
- include/03_analysis/types/type_infer.h -> include/04_analysis/typing/type_infer.h
- include/03_analysis/types/type_layout.h -> include/04_analysis/typing/type_layout.h (or move with layout to include/05_codegen/layout/type_layout.h)
- include/03_analysis/types/type_lookup.h -> include/04_analysis/typing/type_lookup.h
- include/03_analysis/types/type_lower.h -> include/04_analysis/typing/type_lower.h
- include/03_analysis/types/type_match.h -> include/04_analysis/typing/match_check.h (rename)
- include/03_analysis/types/type_pattern.h -> include/04_analysis/typing/type_pattern.h
- include/03_analysis/types/type_predicates.h -> include/04_analysis/typing/type_predicates.h
- include/03_analysis/types/type_stmt.h -> include/04_analysis/typing/type_stmt.h
- include/03_analysis/types/type_wf.h -> include/04_analysis/typing/type_wf.h
- include/03_analysis/types/typecheck.h -> include/04_analysis/typing/typecheck.h
- include/03_analysis/types/types.h -> include/04_analysis/typing/types.h
- include/03_analysis/types/variance.h -> include/04_analysis/typing/variance.h

#### typing/expr
- include/03_analysis/types/expr/addr_of.h -> include/04_analysis/typing/expr/addr_of.h
- include/03_analysis/types/expr/alignof.h -> include/04_analysis/typing/expr/alignof.h
- include/03_analysis/types/expr/alloc.h -> include/04_analysis/typing/expr/alloc_expr.h (rename)
- include/03_analysis/types/expr/array_repeat.h -> include/04_analysis/typing/expr/array_repeat.h
- include/03_analysis/types/expr/binary.h -> include/04_analysis/typing/expr/binary.h
- include/03_analysis/types/expr/call.h -> include/04_analysis/typing/expr/call.h
- include/03_analysis/types/expr/cast.h -> include/04_analysis/typing/expr/cast.h
- include/03_analysis/types/expr/common.h -> include/04_analysis/typing/expr/expr_common.h (rename)
- include/03_analysis/types/expr/deref.h -> include/04_analysis/typing/expr/deref.h
- include/03_analysis/types/expr/enum_literal.h -> include/04_analysis/typing/expr/enum_literal.h
- include/03_analysis/types/expr/field_access.h -> include/04_analysis/typing/expr/field_access.h
- include/03_analysis/types/expr/if.h -> include/04_analysis/typing/expr/if_expr.h (rename)
- include/03_analysis/types/expr/method_call.h -> include/04_analysis/typing/expr/method_call.h
- include/03_analysis/types/expr/move.h -> include/04_analysis/typing/expr/move_expr.h (rename)
- include/03_analysis/types/expr/path.h -> include/04_analysis/typing/expr/path.h
- include/03_analysis/types/expr/propagate.h -> include/04_analysis/typing/expr/propagate_expr.h (rename)
- include/03_analysis/types/expr/range.h -> include/04_analysis/typing/expr/range.h
- include/03_analysis/types/expr/record_literal.h -> include/04_analysis/typing/expr/record_literal.h
- include/03_analysis/types/expr/sizeof.h -> include/04_analysis/typing/expr/sizeof.h
- include/03_analysis/types/expr/transmute.h -> include/04_analysis/typing/expr/transmute_expr.h (rename)
- include/03_analysis/types/expr/tuple_access.h -> include/04_analysis/typing/expr/tuple_access.h
- include/03_analysis/types/expr/unary.h -> include/04_analysis/typing/expr/unary.h

## include/05_codegen (from 04_codegen + intrinsics)
### root/abi/ir/lower/layout/llvm
- include/04_codegen/abi/abi.h -> include/05_codegen/abi/abi.h
- include/04_codegen/async_frame.h -> include/05_codegen/intrinsics/async_frame.h (move)
- include/04_codegen/checks.h -> include/05_codegen/checks/checks.h
- include/04_codegen/cleanup.h -> include/05_codegen/cleanup/cleanup.h
- include/04_codegen/dyn_dispatch.h -> include/05_codegen/dyn_dispatch/dyn_dispatch.h
- include/04_codegen/globals.h -> include/05_codegen/globals/globals.h
- include/04_codegen/ir_dump.h -> include/05_codegen/ir/ir_dump.h
- include/04_codegen/ir_model.h -> include/05_codegen/ir/ir_model.h
- include/04_codegen/layout/layout.h -> include/05_codegen/layout/layout.h
- include/04_codegen/linkage.h -> include/05_codegen/symbols/linkage.h
- include/04_codegen/llvm/llvm_emit.h -> include/05_codegen/llvm/llvm_emit.h
- include/04_codegen/llvm/llvm_ir_panic.h -> include/05_codegen/llvm/llvm_ir_panic.h
- include/04_codegen/llvm/llvm_ir_utils.h -> include/05_codegen/llvm/llvm_backend.h (rename)
- include/04_codegen/lower/lower_expr.h -> include/05_codegen/lower/lower_expr.h
- include/04_codegen/lower/lower_module.h -> include/05_codegen/lower/lower_module.h
- include/04_codegen/lower/lower_pat.h -> include/05_codegen/lower/lower_pat.h
- include/04_codegen/lower/lower_proc.h -> include/05_codegen/lower/lower_proc.h
- include/04_codegen/lower/lower_stmt.h -> include/05_codegen/lower/lower_stmt.h
- include/04_codegen/mangle.h -> include/05_codegen/symbols/mangle.h
- include/runtime/runtime_interface.h -> include/05_codegen/intrinsics/intrinsics_interface.h (rename)
