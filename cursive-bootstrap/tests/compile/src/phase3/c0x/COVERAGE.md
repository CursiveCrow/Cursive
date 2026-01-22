# C0X Test Coverage Manifest

This directory contains semantic analysis tests for C0X extension features.
Tests are named according to the spec section they verify.

## Coverage by Spec Section

### §6 - Generics

| Test File | Spec Rule | Description |
|-----------|-----------|-------------|
| generic_6_1_type_param_ok.cursive | C0X-6.1 | Single type parameter declaration |
| generic_6_1_multi_param_ok.cursive | C0X-6.1 | Multiple type parameters with semicolon |
| generic_6_2_default_ok.cursive | C0X-6.2 | Type parameter with default value |
| generic_6_3_where_clause_ok.cursive | C0X-6.3 | where clause with bounds |
| generic_6_4_bound_satisfaction_ok.cursive | C0X-6.4 | Type argument satisfies bound |
| generic_6_4_bound_violation_err.cursive | C0X-6.4, E-SEM-2601 | Type argument violates bound |
| generic_6_4_multi_bound_ok.cursive | C0X-6.4 | Multiple bounds combined with + |
| generic_6_5_covariant_ok.cursive | C0X-6.5 | Covariant type parameter |
| generic_6_5_contravariant_ok.cursive | C0X-6.5 | Contravariant type parameter |
| generic_6_5_invariant_ok.cursive | C0X-6.5 | Invariant type parameter |
| generic_6_6_monomorphize_ok.cursive | C0X-6.6 | Monomorphization of generics |
| generic_6_6_recursive_instantiation_err.cursive | C0X-6.6, E-SEM-2602 | Non-terminating type recursion |

### §7 - Key System

| Test File | Spec Rule | Description |
|-----------|-----------|-------------|
| key_7_1_key_triple_ok.cursive | C0X-7.1 | Key as (P, M, S) triple |
| key_7_2_path_field_ok.cursive | C0X-7.2 | Key path with field segments |
| key_7_2_path_index_ok.cursive | C0X-7.2 | Key path with index segments |
| key_7_2_coarsen_marker_ok.cursive | C0X-7.2 | Coarsening marker # on segment |
| key_7_3_covers_read_ok.cursive | C0X-7.3 | Covers predicate for Read mode |
| key_7_3_covers_write_ok.cursive | C0X-7.3 | Covers predicate for Write mode |
| key_7_4_implicit_acquire_ok.cursive | C0X-7.4 | Implicit key acquisition |
| key_7_5_conflict_overlap_write_ok.cursive | C0X-7.5 | Non-overlapping paths OK |
| key_7_5_conflict_write_write_err.cursive | C0X-7.5, E-SEM-2701 | Write-Write conflict |
| key_7_5_conflict_read_write_err.cursive | C0X-7.5, E-SEM-2701 | Read-Write conflict |
| key_7_5_no_conflict_read_read_ok.cursive | C0X-7.5 | Read-Read no conflict |
| key_7_6_block_mode_default_read_ok.cursive | C0X-7.6 | Default Read mode |
| key_7_6_block_mode_write_ok.cursive | C0X-7.6 | Explicit Write mode |
| key_7_6_write_in_read_err.cursive | C0X-7.6, E-SEM-2702 | Write in Read mode error |
| key_7_6_1_acquire_order_ok.cursive | C0X-7.6.1 | Canonical acquisition order |
| key_7_6_2_dynamic_ok.cursive | C0X-7.6.2 | Dynamic modifier |
| key_7_6_2_non_constant_index_err.cursive | C0X-7.6.2, E-SEM-2703 | Non-constant index without dynamic |
| key_7_6_3_release_ok.cursive | C0X-7.6.3 | Release block |
| key_7_6_3_release_mode_mismatch_err.cursive | C0X-7.6.3, E-SEM-2704 | Release mode mismatch |
| key_7_6_speculative_ok.cursive | C0X-7.6 | Speculative modifier |
| key_7_8_boundary_field_ok.cursive | C0X-7.8 | Type-level boundary on field |
| key_7_8_boundary_stops_traversal_ok.cursive | C0X-7.8 | Boundary stops path derivation |
| key_7_8_boundary_enum_ok.cursive | C0X-7.8 | Boundary on enum payload |
| key_7_9_shared_class_receiver_only_ok.cursive | C0X-7.9 | shared $Cl with ~ only methods |
| key_7_9_shared_class_mut_receiver_err.cursive | C0X-7.9, E-SEM-2705 | shared $Cl with ~! method |

### §8 - Full Classes

| Test File | Spec Rule | Description |
|-----------|-----------|-------------|
| class_8_1_wf_generic_ok.cursive | C0X-8.1 | Class with generic params |
| class_8_1_acyclic_superclass_ok.cursive | C0X-8.1 | Acyclic superclass bounds |
| class_8_1_cyclic_superclass_err.cursive | C0X-8.1, E-SEM-2611 | Cyclic superclass |
| class_8_1_modal_abstract_state_ok.cursive | C0X-8.1 | Modal class with abstract states |
| class_8_1_nonmodal_abstract_state_err.cursive | C0X-8.1, E-SEM-2612 | Non-modal with abstract states |
| class_8_2_impl_complete_ok.cursive | C0X-8.2 | Complete implementation |
| class_8_2_impl_incomplete_err.cursive | C0X-8.2, E-SEM-2613 | Missing method |
| class_8_3_override_concrete_ok.cursive | C0X-8.3 | Override concrete method |
| class_8_3_override_abstract_err.cursive | C0X-8.3, E-SEM-2614 | Override on abstract |
| class_8_3_missing_override_err.cursive | C0X-8.3, E-SEM-2615 | Missing override keyword |
| class_8_4_coherence_ok.cursive | C0X-8.4 | Coherent implementation |
| class_8_4_orphan_err.cursive | C0X-8.4, E-SEM-2616 | Orphan rule violation |
| class_8_5_vtable_eligible_ok.cursive | C0X-8.5 | Vtable-eligible methods |
| class_8_5_not_dispatchable_generic_err.cursive | C0X-8.5, E-SEM-2617 | Generic method not vtable-eligible |
| class_8_5_static_dispatch_only_ok.cursive | C0X-8.5 | [[static_dispatch_only]] attribute |
| class_8_5_self_by_value_err.cursive | C0X-8.5, E-SEM-2618 | Self by value not vtable-eligible |
| class_8_associated_type_ok.cursive | C0X-8 | Associated types |

### §9 - Module Importing

| Test File | Spec Rule | Description |
|-----------|-----------|-------------|
| import_9_2_ok.cursive | C0X-9.2 | import declaration |
| import_9_2_as_alias_ok.cursive | C0X-9.2 | import with alias |
| using_9_3_item_ok.cursive | C0X-9.3 | using single item |
| using_9_3_list_ok.cursive | C0X-9.3 | using list |
| using_9_3_self_ok.cursive | C0X-9.3 | using self |
| using_9_3_wildcard_ok.cursive | C0X-9.3 | using wildcard |
| using_9_3_conflict_err.cursive | C0X-9.3, E-SEM-2901 | Name conflict |
| using_9_4_reexport_ok.cursive | C0X-9.4 | public using re-export |

### §14 - Contracts

| Test File | Spec Rule | Description |
|-----------|-----------|-------------|
| contract_14_4_1_clause_position_ok.cursive | C0X-14.4.1 | Contract clause position |
| contract_14_4_2_purity_ok.cursive | C0X-14.4.2 | Pure predicates |
| contract_14_4_2_purity_err.cursive | C0X-14.4.2, E-SEM-2803 | Impure predicate |
| contract_14_4_3_pre_context_ok.cursive | C0X-14.4.3 | Precondition context |
| contract_14_4_3_post_context_ok.cursive | C0X-14.4.3 | Postcondition context |
| contract_14_5_1_pre_elision_ok.cursive | C0X-14.5.1 | Precondition elision |
| contract_14_5_3_result_postcond_only_ok.cursive | C0X-14.5.3 | @result in postcondition |
| contract_14_5_3_result_in_pre_err.cursive | C0X-14.5.3, E-SEM-2806 | @result in precondition |
| contract_14_5_4_entry_ok.cursive | C0X-14.5.4 | @entry operator |
| contract_14_5_4_entry_non_copyable_err.cursive | C0X-14.5.4, E-SEM-2805 | @entry non-copyable |
| contract_14_6_1_type_invariant_ok.cursive | C0X-14.6.1 | Type invariant |
| contract_14_6_1_invariant_public_field_err.cursive | C0X-14.6.1, E-SEM-2821 | Invariant with public field |
| contract_14_6_2_loop_invariant_ok.cursive | C0X-14.6.2 | Loop invariant |
| contract_14_7_1_static_verification_ok.cursive | C0X-14.7.1 | Static verification |
| contract_14_7_1_static_fail_err.cursive | C0X-14.7.1, E-SEM-2801 | Static verification failure |
| contract_14_7_1_dynamic_ok.cursive | C0X-14.7.1 | [[dynamic]] verification |

## Diagnostic Error Codes Tested

| Code | Description | Test Files |
|------|-------------|------------|
| E-SEM-2601 | Generic bound violation | generic_6_4_bound_violation_err |
| E-SEM-2602 | Non-terminating type recursion | generic_6_6_recursive_instantiation_err |
| E-SEM-2611 | Cyclic superclass | class_8_1_cyclic_superclass_err |
| E-SEM-2612 | Abstract states in non-modal | class_8_1_nonmodal_abstract_state_err |
| E-SEM-2613 | Incomplete implementation | class_8_2_impl_incomplete_err |
| E-SEM-2614 | Override on abstract | class_8_3_override_abstract_err |
| E-SEM-2615 | Missing override keyword | class_8_3_missing_override_err |
| E-SEM-2616 | Orphan rule violation | class_8_4_orphan_err |
| E-SEM-2617 | Non-dispatchable generic | class_8_5_not_dispatchable_generic_err |
| E-SEM-2618 | Self by value | class_8_5_self_by_value_err |
| E-SEM-2701 | Key conflict | key_7_5_conflict_* |
| E-SEM-2702 | Write in read mode | key_7_6_write_in_read_err |
| E-SEM-2703 | Non-constant index | key_7_6_2_non_constant_index_err |
| E-SEM-2704 | Release mode mismatch | key_7_6_3_release_mode_mismatch_err |
| E-SEM-2705 | shared $Cl with mut receiver | key_7_9_shared_class_mut_receiver_err |
| E-SEM-2801 | Static verification failure | contract_14_7_1_static_fail_err |
| E-SEM-2803 | Impure contract predicate | contract_14_4_2_purity_err |
| E-SEM-2805 | @entry non-copyable type | contract_14_5_4_entry_non_copyable_err |
| E-SEM-2806 | @result outside postcondition | contract_14_5_3_result_in_pre_err |
| E-SEM-2821 | Invariant with public field | contract_14_6_1_invariant_public_field_err |
| E-SEM-2901 | Using name conflict | using_9_3_conflict_err |
