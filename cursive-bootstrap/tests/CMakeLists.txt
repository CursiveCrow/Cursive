# Copy ICU DLLs to test output directory on Windows
if(WIN32 AND CURSIVE_USE_BUNDLED_ICU AND DEFINED CURSIVE_ICU_DLLS)
  set(TESTS_DLL_COPIED FALSE)
endif()

# Helper function to ensure DLLs are copied (only once per test target)
function(copy_icu_dlls_for_test target)
  if(WIN32 AND CURSIVE_USE_BUNDLED_ICU AND DEFINED CURSIVE_ICU_DLLS)
    foreach(dll ${CURSIVE_ICU_DLLS})
      add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${dll}"
          $<TARGET_FILE_DIR:${target}>
        COMMENT "Copying ICU DLL for test: ${dll}"
      )
    endforeach()
  endif()
endfunction()

add_executable(core_span_test
  unit/core_span_test.cpp
)

target_link_libraries(core_span_test PRIVATE cursive0_core)
copy_icu_dlls_for_test(core_span_test)

add_test(NAME core_span_test COMMAND core_span_test)

add_executable(syntax_token_span_test
  unit/syntax_token_span_test.cpp
)

target_link_libraries(syntax_token_span_test PRIVATE cursive0_syntax)

add_test(NAME syntax_token_span_test COMMAND syntax_token_span_test)

add_executable(syntax_newline_termination_test
  unit/syntax_newline_termination_test.cpp
)

target_link_libraries(syntax_newline_termination_test PRIVATE cursive0_syntax)

add_test(NAME syntax_newline_termination_test COMMAND syntax_newline_termination_test)

add_executable(syntax_lexer_ws_test
  unit/syntax_lexer_ws_test.cpp
)

target_link_libraries(syntax_lexer_ws_test PRIVATE cursive0_syntax)

add_test(NAME syntax_lexer_ws_test COMMAND syntax_lexer_ws_test)

add_executable(syntax_lexer_literals_test
  unit/syntax_lexer_literals_test.cpp
)

target_link_libraries(syntax_lexer_literals_test PRIVATE cursive0_syntax)

add_test(NAME syntax_lexer_literals_test COMMAND syntax_lexer_literals_test)

add_executable(syntax_lexer_ident_kw_test
  unit/syntax_lexer_ident_kw_test.cpp
)

target_link_libraries(syntax_lexer_ident_kw_test PRIVATE cursive0_syntax)

add_test(NAME syntax_lexer_ident_kw_test COMMAND syntax_lexer_ident_kw_test)

add_executable(syntax_lexer_max_munch_test
  unit/syntax_lexer_max_munch_test.cpp
)

target_link_libraries(syntax_lexer_max_munch_test PRIVATE cursive0_syntax)

add_test(NAME syntax_lexer_max_munch_test COMMAND syntax_lexer_max_munch_test)

add_executable(syntax_lexer_security_test
  unit/syntax_lexer_security_test.cpp
)

target_link_libraries(syntax_lexer_security_test PRIVATE cursive0_syntax)

add_test(NAME syntax_lexer_security_test COMMAND syntax_lexer_security_test)

add_executable(syntax_tokenize_test
  unit/syntax_tokenize_test.cpp
)

target_link_libraries(syntax_tokenize_test PRIVATE cursive0_syntax)

add_test(NAME syntax_tokenize_test COMMAND syntax_tokenize_test)

add_executable(syntax_parser_core_test
  unit/syntax_parser_core_test.cpp
)

target_link_libraries(syntax_parser_core_test PRIVATE cursive0_syntax)

add_test(NAME syntax_parser_core_test COMMAND syntax_parser_core_test)

add_executable(syntax_parser_items_test
  unit/syntax_parser_items_test.cpp
)

target_link_libraries(syntax_parser_items_test PRIVATE cursive0_syntax)

add_test(NAME syntax_parser_items_test COMMAND syntax_parser_items_test)

add_executable(syntax_parser_stmt_test
  unit/syntax_parser_stmt_test.cpp
)

target_link_libraries(syntax_parser_stmt_test PRIVATE cursive0_syntax)

add_test(NAME syntax_parser_stmt_test COMMAND syntax_parser_stmt_test)

add_executable(syntax_parser_expr_test
  unit/syntax_parser_expr_test.cpp
)

target_link_libraries(syntax_parser_expr_test PRIVATE cursive0_syntax)

add_test(NAME syntax_parser_expr_test COMMAND syntax_parser_expr_test)

add_executable(syntax_parser_pattern_test
  unit/syntax_parser_pattern_test.cpp
)

target_link_libraries(syntax_parser_pattern_test PRIVATE cursive0_syntax)

add_test(NAME syntax_parser_pattern_test COMMAND syntax_parser_pattern_test)

add_executable(syntax_parser_type_test
  unit/syntax_parser_type_test.cpp
)

target_link_libraries(syntax_parser_type_test PRIVATE cursive0_syntax)

add_test(NAME syntax_parser_type_test COMMAND syntax_parser_type_test)

add_executable(syntax_parser_docs_test
  unit/syntax_parser_docs_test.cpp
)

target_link_libraries(syntax_parser_docs_test PRIVATE cursive0_syntax)

add_test(NAME syntax_parser_docs_test COMMAND syntax_parser_docs_test)

add_executable(syntax_parser_consume_test
  unit/syntax_parser_consume_test.cpp
)

target_link_libraries(syntax_parser_consume_test PRIVATE cursive0_syntax)

add_test(NAME syntax_parser_consume_test COMMAND syntax_parser_consume_test)

add_executable(syntax_parser_angle_test
  unit/syntax_parser_angle_test.cpp
)

target_link_libraries(syntax_parser_angle_test PRIVATE cursive0_syntax)

add_test(NAME syntax_parser_angle_test COMMAND syntax_parser_angle_test)

add_executable(syntax_keyword_policy_test
  unit/syntax_keyword_policy_test.cpp
)

target_link_libraries(syntax_keyword_policy_test PRIVATE cursive0_syntax)

add_test(NAME syntax_keyword_policy_test COMMAND syntax_keyword_policy_test)

add_executable(analysis_scopes_test
  unit/analysis_scopes_test.cpp
)

target_link_libraries(analysis_scopes_test PRIVATE cursive0_analysis)

add_test(NAME analysis_scopes_test COMMAND analysis_scopes_test)

add_executable(analysis_scopes_intro_test
  unit/analysis_scopes_intro_test.cpp
)

target_link_libraries(analysis_scopes_intro_test PRIVATE cursive0_analysis)

add_test(NAME analysis_scopes_intro_test COMMAND analysis_scopes_intro_test)

add_executable(analysis_types_test
  unit/analysis_types_test.cpp
)

target_link_libraries(analysis_types_test PRIVATE cursive0_analysis)

add_test(NAME analysis_types_test COMMAND analysis_types_test)

add_executable(analysis_type_equiv_test
  unit/analysis_type_equiv_test.cpp
)

add_executable(analysis_subtyping_test
  unit/analysis_subtyping_test.cpp
)

target_link_libraries(analysis_type_equiv_test PRIVATE cursive0_analysis)

add_test(NAME analysis_type_equiv_test COMMAND analysis_type_equiv_test)

target_link_libraries(analysis_subtyping_test PRIVATE cursive0_analysis)

add_test(NAME analysis_subtyping_test COMMAND analysis_subtyping_test)

add_executable(analysis_borrow_bind_test
  unit/analysis_borrow_bind_test.cpp
)

target_link_libraries(analysis_borrow_bind_test PRIVATE cursive0_analysis)

add_test(NAME analysis_borrow_bind_test COMMAND analysis_borrow_bind_test)

add_executable(analysis_calls_test
  unit/analysis_calls_test.cpp
)

target_link_libraries(analysis_calls_test PRIVATE cursive0_analysis)

add_test(NAME analysis_calls_test COMMAND analysis_calls_test)

add_executable(analysis_tuples_test
  unit/analysis_tuples_test.cpp
)

target_link_libraries(analysis_tuples_test PRIVATE cursive0_analysis)

add_test(NAME analysis_tuples_test COMMAND analysis_tuples_test)

add_executable(analysis_arrays_slices_test
  unit/analysis_arrays_slices_test.cpp
)

target_link_libraries(analysis_arrays_slices_test PRIVATE cursive0_analysis)

add_test(NAME analysis_arrays_slices_test COMMAND analysis_arrays_slices_test)

add_executable(analysis_enum_disc_test
  unit/analysis_enum_disc_test.cpp
)

target_link_libraries(analysis_enum_disc_test PRIVATE cursive0_analysis)

add_test(NAME analysis_enum_disc_test COMMAND analysis_enum_disc_test)

add_executable(analysis_unions_test
  unit/analysis_unions_test.cpp
)

target_link_libraries(analysis_unions_test PRIVATE cursive0_analysis)

add_test(NAME analysis_unions_test COMMAND analysis_unions_test)


add_executable(analysis_records_test
  unit/analysis_records_test.cpp
)

target_link_libraries(analysis_records_test PRIVATE cursive0_analysis)

add_test(NAME analysis_records_test COMMAND analysis_records_test)

add_executable(analysis_type_infer_test
  unit/analysis_type_infer_test.cpp
)

add_executable(analysis_type_stmt_test
  unit/analysis_type_stmt_test.cpp
)

target_link_libraries(analysis_type_infer_test PRIVATE cursive0_analysis)

add_test(NAME analysis_type_infer_test COMMAND analysis_type_infer_test)

target_link_libraries(analysis_type_stmt_test PRIVATE cursive0_analysis)

add_test(NAME analysis_type_stmt_test COMMAND analysis_type_stmt_test)

add_executable(analysis_collect_toplevel_test
  unit/analysis_collect_toplevel_test.cpp
)

target_link_libraries(analysis_collect_toplevel_test PRIVATE cursive0_analysis)

add_test(NAME analysis_collect_toplevel_test COMMAND analysis_collect_toplevel_test)

add_executable(analysis_resolve_qual_test
  unit/analysis_resolve_qual_test.cpp
)

target_link_libraries(analysis_resolve_qual_test PRIVATE cursive0_analysis)

add_test(NAME analysis_resolve_qual_test COMMAND analysis_resolve_qual_test)

add_executable(core_diagnostics_emit_test
  unit/core_diagnostics_emit_test.cpp
)

target_link_libraries(core_diagnostics_emit_test PRIVATE cursive0_core)

add_test(NAME core_diagnostics_emit_test COMMAND core_diagnostics_emit_test)

add_executable(core_diag_code_selection_test
  unit/core_diag_code_selection_test.cpp
)

target_link_libraries(core_diag_code_selection_test PRIVATE cursive0_core)

add_test(NAME core_diag_code_selection_test COMMAND core_diag_code_selection_test)

add_executable(core_diag_order_test
  unit/core_diag_order_test.cpp
)

target_link_libraries(core_diag_order_test PRIVATE cursive0_core)

add_test(NAME core_diag_order_test COMMAND core_diag_order_test)

add_executable(core_diag_external_test
  unit/core_diag_external_test.cpp
)

target_link_libraries(core_diag_external_test PRIVATE cursive0_core)

add_test(NAME core_diag_external_test COMMAND core_diag_external_test)

add_executable(core_diag_render_test
  unit/core_diag_render_test.cpp
)

target_link_libraries(core_diag_render_test PRIVATE cursive0_core)

add_test(NAME core_diag_render_test COMMAND core_diag_render_test)


add_executable(core_diag_messages_test
  unit/core_diag_messages_test.cpp
)

target_link_libraries(core_diag_messages_test PRIVATE cursive0_core)

add_test(NAME core_diag_messages_test COMMAND core_diag_messages_test)

add_executable(core_ub_model_test
  unit/core_ub_model_test.cpp
)

target_link_libraries(core_ub_model_test PRIVATE cursive0_core)

add_test(NAME core_ub_model_test COMMAND core_ub_model_test)

add_executable(core_host_primitives_test
  unit/core_host_primitives_test.cpp
)

target_link_libraries(core_host_primitives_test PRIVATE cursive0_core)

add_test(NAME core_host_primitives_test COMMAND core_host_primitives_test)


add_executable(core_source_load_test
  unit/core_source_load_test.cpp
)

target_link_libraries(core_source_load_test PRIVATE cursive0_core)

add_test(NAME core_source_load_test COMMAND core_source_load_test)


add_executable(core_line_normalization_test
  unit/core_line_normalization_test.cpp
)

target_link_libraries(core_line_normalization_test PRIVATE cursive0_core)

add_test(NAME core_line_normalization_test COMMAND core_line_normalization_test)

add_executable(core_prohibited_codepoints_test
  unit/core_prohibited_codepoints_test.cpp
)

target_link_libraries(core_prohibited_codepoints_test PRIVATE cursive0_core)

add_test(NAME core_prohibited_codepoints_test COMMAND core_prohibited_codepoints_test)


add_executable(project_manifest_parse_test
  unit/project_manifest_parse_test.cpp
)

target_link_libraries(project_manifest_parse_test PRIVATE cursive0_project)

add_test(NAME project_manifest_parse_test COMMAND project_manifest_parse_test)


add_executable(project_manifest_validate_test
  unit/project_manifest_validate_test.cpp
)

target_link_libraries(project_manifest_validate_test PRIVATE cursive0_project)

add_test(NAME project_manifest_validate_test COMMAND project_manifest_validate_test)

add_executable(project_load_pipeline_test
  unit/project_load_pipeline_test.cpp
)

target_link_libraries(project_load_pipeline_test PRIVATE cursive0_project)

add_test(NAME project_load_pipeline_test COMMAND project_load_pipeline_test)

add_executable(project_deterministic_order_test
  unit/project_deterministic_order_test.cpp
)

target_link_libraries(project_deterministic_order_test PRIVATE cursive0_project)

add_test(NAME project_deterministic_order_test COMMAND project_deterministic_order_test)

add_executable(project_module_discovery_test
  unit/project_module_discovery_test.cpp
)

target_link_libraries(project_module_discovery_test PRIVATE cursive0_project)

add_test(NAME project_module_discovery_test COMMAND project_module_discovery_test)

add_executable(project_outputs_test
  unit/project_outputs_test.cpp
)

target_link_libraries(project_outputs_test PRIVATE cursive0_project)

add_test(NAME project_outputs_test COMMAND project_outputs_test)

add_executable(project_link_test
  unit/project_link_test.cpp
)

target_link_libraries(project_link_test PRIVATE cursive0_project)

add_test(NAME project_link_test COMMAND project_link_test)

add_executable(project_tool_resolution_test
  unit/project_tool_resolution_test.cpp
)

target_link_libraries(project_tool_resolution_test PRIVATE cursive0_project)

add_test(NAME project_tool_resolution_test COMMAND project_tool_resolution_test)

add_executable(project_ir_assembly_test
  unit/project_ir_assembly_test.cpp
)

target_link_libraries(project_ir_assembly_test PRIVATE cursive0_project)

add_test(NAME project_ir_assembly_test COMMAND project_ir_assembly_test)

add_executable(syntax_parse_modules_test
  unit/syntax_parse_modules_test.cpp
)

target_link_libraries(syntax_parse_modules_test PRIVATE cursive0_frontend)

add_test(NAME syntax_parse_modules_test COMMAND syntax_parse_modules_test)


add_executable(analysis_conformance_test
  unit/analysis_conformance_test.cpp
)

target_link_libraries(analysis_conformance_test PRIVATE cursive0_analysis)

add_test(NAME analysis_conformance_test COMMAND analysis_conformance_test)

add_executable(analysis_conformance_subset_unicode_test
  unit/analysis_conformance_subset_unicode_test.cpp
)

target_link_libraries(analysis_conformance_subset_unicode_test PRIVATE cursive0_analysis)

add_test(NAME analysis_conformance_subset_unicode_test COMMAND analysis_conformance_subset_unicode_test)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

add_test(NAME compile_tests
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/harness/run_compiler.py
    --compiler $<TARGET_FILE:cursivec0>
    --tests-root ${CMAKE_CURRENT_SOURCE_DIR}/compile
)

if(WIN32)
  add_test(NAME eval_oracle_tests
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/harness/run_eval_oracle.py
      --compiler $<TARGET_FILE:cursivec0>
      --interpreter $<TARGET_FILE:cursive0_interpreter_cli>
      --tests-root ${CMAKE_CURRENT_SOURCE_DIR}/e2e
  )
endif()

add_test(NAME spec_coverage_report
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../tools/coverage_report.py
)

add_test(NAME spec_diag_report
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../tools/diag_check_report.py
)

add_executable(analysis_scopes_lookup_test
  unit/analysis_scopes_lookup_test.cpp
)

target_link_libraries(analysis_scopes_lookup_test PRIVATE cursive0_analysis)

add_test(NAME analysis_scopes_lookup_test COMMAND analysis_scopes_lookup_test)

add_executable(analysis_resolver_expr_test
  unit/analysis_resolver_expr_test.cpp
)

target_link_libraries(analysis_resolver_expr_test PRIVATE cursive0_analysis)

add_test(NAME analysis_resolver_expr_test COMMAND analysis_resolver_expr_test)

add_executable(analysis_resolver_items_test
  unit/analysis_resolver_items_test.cpp
)

target_link_libraries(analysis_resolver_items_test PRIVATE cursive0_analysis)

add_test(NAME analysis_resolver_items_test COMMAND analysis_resolver_items_test)

add_executable(analysis_visibility_test
  unit/analysis_visibility_test.cpp
)

target_link_libraries(analysis_visibility_test PRIVATE cursive0_analysis)

add_test(NAME analysis_visibility_test COMMAND analysis_visibility_test)

add_executable(analysis_literals_test
  unit/analysis_literals_test.cpp
)

target_link_libraries(analysis_literals_test PRIVATE cursive0_analysis)

add_test(NAME analysis_literals_test COMMAND analysis_literals_test)

add_executable(analysis_type_expr_test
  unit/analysis_type_expr_test.cpp
)

target_link_libraries(analysis_type_expr_test PRIVATE cursive0_analysis)

add_test(NAME analysis_type_expr_test COMMAND analysis_type_expr_test)

add_executable(analysis_type_wf_test
  unit/analysis_type_wf_test.cpp
)

target_link_libraries(analysis_type_wf_test PRIVATE cursive0_analysis)

add_test(NAME analysis_type_wf_test COMMAND analysis_type_wf_test)

add_executable(analysis_type_match_test
  unit/analysis_type_match_test.cpp
)

target_link_libraries(analysis_type_match_test PRIVATE cursive0_analysis)

add_test(NAME analysis_type_match_test COMMAND analysis_type_match_test)
add_executable(analysis_init_planner_test
  unit/analysis_init_planner_test.cpp
)

target_link_libraries(analysis_init_planner_test PRIVATE cursive0_analysis)

add_test(NAME analysis_init_planner_test COMMAND analysis_init_planner_test)

add_executable(codegen_ir_model_test
  unit/codegen_ir_model_test.cpp
)

add_executable(codegen_phase4_lower_missing_cov_test
  unit/codegen_phase4_lower_missing_cov_test.cpp
)

target_link_libraries(codegen_phase4_lower_missing_cov_test PRIVATE cursive0_codegen)

add_test(NAME codegen_phase4_lower_missing_cov_test COMMAND codegen_phase4_lower_missing_cov_test)

add_executable(codegen_init_poison_test
  unit/codegen_init_poison_test.cpp
)

target_link_libraries(codegen_init_poison_test PRIVATE cursive0_codegen)

add_test(NAME codegen_init_poison_test COMMAND codegen_init_poison_test)

add_executable(codegen_panic_cleanup_test
  unit/codegen_panic_cleanup_test.cpp
)

target_link_libraries(codegen_panic_cleanup_test PRIVATE cursive0_codegen)

add_test(NAME codegen_panic_cleanup_test COMMAND codegen_panic_cleanup_test)

target_link_libraries(codegen_ir_model_test PRIVATE cursive0_codegen)

add_test(NAME codegen_ir_model_test COMMAND codegen_ir_model_test)

add_executable(codegen_layout_test
  unit/codegen_layout_test.cpp
)

target_link_libraries(codegen_layout_test PRIVATE cursive0_codegen)

add_test(NAME codegen_layout_test COMMAND codegen_layout_test)

add_executable(codegen_globals_test
  unit/codegen_globals_test.cpp
)

target_link_libraries(codegen_globals_test PRIVATE cursive0_codegen)
copy_icu_dlls_for_test(codegen_globals_test)

add_test(NAME codegen_globals_test COMMAND codegen_globals_test)

add_executable(codegen_abi_test
  unit/codegen_abi_test.cpp
)

target_link_libraries(codegen_abi_test PRIVATE cursive0_codegen)

add_test(NAME codegen_abi_test COMMAND codegen_abi_test)

add_executable(codegen_runtime_interface_phase4_test
  unit/codegen_runtime_interface_phase4_test.cpp
)

target_link_libraries(codegen_runtime_interface_phase4_test PRIVATE cursive0_codegen)
copy_icu_dlls_for_test(codegen_runtime_interface_phase4_test)

add_test(NAME codegen_runtime_interface_phase4_test COMMAND codegen_runtime_interface_phase4_test)

add_executable(core_hash_test
  unit/core_hash_test.cpp
)

target_link_libraries(core_hash_test PRIVATE cursive0_core)
copy_icu_dlls_for_test(core_hash_test)

add_test(NAME core_hash_test COMMAND core_hash_test)

add_executable(codegen_mangle_test
  unit/codegen_mangle_test.cpp
)

target_link_libraries(codegen_mangle_test PRIVATE cursive0_codegen)
copy_icu_dlls_for_test(codegen_mangle_test)

add_test(NAME codegen_mangle_test COMMAND codegen_mangle_test)

add_executable(codegen_linkage_test
  unit/codegen_linkage_test.cpp
)

target_link_libraries(codegen_linkage_test PRIVATE cursive0_codegen)
copy_icu_dlls_for_test(codegen_linkage_test)

add_test(NAME codegen_linkage_test COMMAND codegen_linkage_test)

add_executable(codegen_llvm_test
  unit/codegen_llvm_test.cpp
)

target_link_libraries(codegen_llvm_test PRIVATE cursive0_codegen)


copy_icu_dlls_for_test(codegen_llvm_test)

add_test(NAME codegen_llvm_test COMMAND codegen_llvm_test)

add_executable(codegen_string_literal_test
  unit/codegen_string_literal_test.cpp
)

target_link_libraries(codegen_string_literal_test PRIVATE cursive0_codegen)
copy_icu_dlls_for_test(codegen_string_literal_test)

add_test(NAME codegen_string_literal_test COMMAND codegen_string_literal_test)

add_executable(eval_smoke_test
  unit/eval_smoke_test.cpp
)

target_link_libraries(eval_smoke_test PRIVATE cursive0_interpreter)
copy_icu_dlls_for_test(eval_smoke_test)

add_test(NAME eval_smoke_test COMMAND eval_smoke_test)

add_executable(eval_state_test
  unit/eval_state_test.cpp
)

target_link_libraries(eval_state_test PRIVATE cursive0_eval)
copy_icu_dlls_for_test(eval_state_test)

add_test(NAME eval_state_test COMMAND eval_state_test)

add_executable(eval_match_test
  unit/eval_match_test.cpp
)

target_link_libraries(eval_match_test PRIVATE cursive0_eval)
copy_icu_dlls_for_test(eval_match_test)

add_test(NAME eval_match_test COMMAND eval_match_test)

add_executable(eval_eval_test
  unit/eval_eval_test.cpp
)

target_link_libraries(eval_eval_test PRIVATE cursive0_eval cursive0_analysis)
copy_icu_dlls_for_test(eval_eval_test)

add_test(NAME eval_eval_test COMMAND eval_eval_test)

add_executable(eval_apply_cov_test
  unit/eval_apply_cov_test.cpp
)

target_link_libraries(eval_apply_cov_test PRIVATE cursive0_eval cursive0_analysis)
copy_icu_dlls_for_test(eval_apply_cov_test)

add_test(NAME eval_apply_cov_test COMMAND eval_apply_cov_test)

add_executable(eval_cast_cov_test
  unit/eval_cast_cov_test.cpp
)

target_link_libraries(eval_cast_cov_test PRIVATE cursive0_eval cursive0_analysis)
copy_icu_dlls_for_test(eval_cast_cov_test)

add_test(NAME eval_cast_cov_test COMMAND eval_cast_cov_test)

add_executable(eval_exec_cov_test
  unit/eval_exec_cov_test.cpp
)

target_link_libraries(eval_exec_cov_test PRIVATE cursive0_eval)
copy_icu_dlls_for_test(eval_exec_cov_test)

add_test(NAME eval_exec_cov_test COMMAND eval_exec_cov_test)

add_executable(eval_cleanup_cov_test
  unit/eval_cleanup_cov_test.cpp
)

target_link_libraries(eval_cleanup_cov_test PRIVATE cursive0_eval cursive0_analysis)
copy_icu_dlls_for_test(eval_cleanup_cov_test)

add_test(NAME eval_cleanup_cov_test COMMAND eval_cleanup_cov_test)

add_executable(eval_interpreter_cov_test
  unit/eval_interpreter_cov_test.cpp
)

target_link_libraries(eval_interpreter_cov_test PRIVATE cursive0_interpreter cursive0_analysis)
copy_icu_dlls_for_test(eval_interpreter_cov_test)

add_test(NAME eval_interpreter_cov_test COMMAND eval_interpreter_cov_test)

add_executable(eval_eval_call_cov_test
  unit/eval_eval_call_cov_test.cpp
)

target_link_libraries(eval_eval_call_cov_test PRIVATE cursive0_eval cursive0_analysis)
copy_icu_dlls_for_test(eval_eval_call_cov_test)

add_test(NAME eval_eval_call_cov_test COMMAND eval_eval_call_cov_test)

add_executable(eval_control_flow_cov_test
  unit/eval_control_flow_cov_test.cpp
)

target_link_libraries(eval_control_flow_cov_test PRIVATE cursive0_eval cursive0_analysis)
copy_icu_dlls_for_test(eval_control_flow_cov_test)

add_test(NAME eval_control_flow_cov_test COMMAND eval_control_flow_cov_test)

add_executable(eval_exec_flow_cov_test
  unit/eval_exec_flow_cov_test.cpp
)

target_link_libraries(eval_exec_flow_cov_test PRIVATE cursive0_eval cursive0_analysis)
copy_icu_dlls_for_test(eval_exec_flow_cov_test)

add_test(NAME eval_exec_flow_cov_test COMMAND eval_exec_flow_cov_test)

add_executable(eval_lookup_cov_test
  unit/eval_lookup_cov_test.cpp
)

target_link_libraries(eval_lookup_cov_test PRIVATE cursive0_eval cursive0_analysis)
copy_icu_dlls_for_test(eval_lookup_cov_test)

add_test(NAME eval_lookup_cov_test COMMAND eval_lookup_cov_test)

add_executable(eval_write_cov_test
  unit/eval_write_cov_test.cpp
)

target_link_libraries(eval_write_cov_test PRIVATE cursive0_eval cursive0_analysis)
copy_icu_dlls_for_test(eval_write_cov_test)

add_test(NAME eval_write_cov_test COMMAND eval_write_cov_test)

add_executable(eval_unwind_cov_test
  unit/eval_unwind_cov_test.cpp
)

target_link_libraries(eval_unwind_cov_test PRIVATE cursive0_eval cursive0_analysis)
copy_icu_dlls_for_test(eval_unwind_cov_test)

add_test(NAME eval_unwind_cov_test COMMAND eval_unwind_cov_test)


add_executable(eval_value_test
  unit/eval_value_test.cpp
)

target_link_libraries(eval_value_test PRIVATE cursive0_eval)
copy_icu_dlls_for_test(eval_value_test)

add_test(NAME eval_value_test COMMAND eval_value_test)

add_executable(eval_init_test
  unit/eval_init_test.cpp
)

target_link_libraries(eval_init_test PRIVATE cursive0_eval cursive0_analysis)
copy_icu_dlls_for_test(eval_init_test)

add_test(NAME eval_init_test COMMAND eval_init_test)

add_executable(eval_builtins_test
  unit/eval_builtins_test.cpp
)

target_link_libraries(eval_builtins_test PRIVATE cursive0_eval)
copy_icu_dlls_for_test(eval_builtins_test)

add_test(NAME eval_builtins_test COMMAND eval_builtins_test)

add_executable(eval_interpreter_test
  unit/eval_interpreter_test.cpp
)

target_link_libraries(eval_interpreter_test PRIVATE cursive0_interpreter cursive0_analysis)
copy_icu_dlls_for_test(eval_interpreter_test)

add_test(NAME eval_interpreter_test COMMAND eval_interpreter_test)
