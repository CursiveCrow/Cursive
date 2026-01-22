# C0X Parsing Test Coverage Manifest

This directory contains parsing (Phase 1) tests for C0X extension syntax.
Tests verify that the grammar for each C0X feature parses correctly.

## Coverage by Spec Section

### §6 - Generics Syntax

| Test File | Spec Rule | Description |
|-----------|-----------|-------------|
| parse_generic_6_1_single_param_ok.cursive | C0X-6.1 | Single type parameter `<T>` |
| parse_generic_6_1_multi_param_ok.cursive | C0X-6.1 | Multiple params `<K; V>` |
| parse_generic_6_2_default_ok.cursive | C0X-6.2 | Default type `<T = i32>` |
| parse_generic_6_3_where_ok.cursive | C0X-6.3 | where clause `where T <: Bound` |
| parse_generic_6_3_where_multi_ok.cursive | C0X-6.3 | Multiple bounds `where T <: A + B` |
| parse_generic_proc_ok.cursive | C0X-6 | Generic procedure |
| parse_generic_call_ok.cursive | C0X-6 | Generic call with turbofish |

### §7 - Key System Syntax

| Test File | Spec Rule | Description |
|-----------|-----------|-------------|
| parse_key_7_6_block_read_ok.cursive | C0X-7.6 | Key block `# path { }` |
| parse_key_7_6_block_write_ok.cursive | C0X-7.6 | Key block `# path write { }` |
| parse_key_7_6_multi_path_ok.cursive | C0X-7.6 | Multiple paths `# a, b { }` |
| parse_key_7_6_dynamic_ok.cursive | C0X-7.6.2 | Dynamic modifier `# path dynamic { }` |
| parse_key_7_6_speculative_ok.cursive | C0X-7.6 | Speculative `# path speculative { }` |
| parse_key_7_6_3_release_ok.cursive | C0X-7.6.3 | Release `# path release { }` |
| parse_key_7_2_path_field_ok.cursive | C0X-7.2 | Field path `o.inner.val` |
| parse_key_7_2_path_index_ok.cursive | C0X-7.2 | Index path `arr[1]` |
| parse_key_7_2_coarsen_field_ok.cursive | C0X-7.2 | Coarsen field `o.#inner` |
| parse_key_7_2_coarsen_index_ok.cursive | C0X-7.2 | Coarsen index `arr[#0]` |
| parse_key_7_8_boundary_record_ok.cursive | C0X-7.8 | Record boundary `#field: T` |
| parse_key_7_8_boundary_enum_ok.cursive | C0X-7.8 | Enum boundary `#payload: T` |

### §8 - Class Syntax

| Test File | Spec Rule | Description |
|-----------|-----------|-------------|
| parse_class_8_abstract_method_ok.cursive | C0X-8 | Abstract method (no body) |
| parse_class_8_concrete_method_ok.cursive | C0X-8 | Concrete method (with body) |
| parse_class_8_associated_type_ok.cursive | C0X-8 | Associated type `type Element` |
| parse_class_8_associated_type_default_ok.cursive | C0X-8 | Assoc type default `type E = ()` |
| parse_class_8_superclass_ok.cursive | C0X-8.1 | Superclass `class D <: B` |
| parse_class_8_multi_superclass_ok.cursive | C0X-8.1 | Multiple `class C <: A + B` |
| parse_class_8_modal_ok.cursive | C0X-8.1 | Modal class with states |
| parse_class_8_impl_ok.cursive | C0X-8.2 | Implementation `T <: Cl { }` |
| parse_class_8_override_ok.cursive | C0X-8.3 | Override keyword |

### §9 - Import/Using Syntax

| Test File | Spec Rule | Description |
|-----------|-----------|-------------|
| parse_import_9_2_ok.cursive | C0X-9.2 | import declaration |
| parse_import_9_2_as_ok.cursive | C0X-9.2 | import with alias |
| parse_using_9_3_item_ok.cursive | C0X-9.3 | using item |
| parse_using_9_3_list_ok.cursive | C0X-9.3 | using list `{a, b}` |
| parse_using_9_3_self_ok.cursive | C0X-9.3 | using self |
| parse_using_9_3_wildcard_ok.cursive | C0X-9.3 | using wildcard `*` |
| parse_using_9_4_public_ok.cursive | C0X-9.4 | public using re-export |

### §14 - Contract Syntax

| Test File | Spec Rule | Description |
|-----------|-----------|-------------|
| parse_contract_14_4_precond_ok.cursive | C0X-14.4 | Precondition only `|= P` |
| parse_contract_14_4_full_ok.cursive | C0X-14.4 | Full contract `|= P => Q` |
| parse_contract_14_4_post_only_ok.cursive | C0X-14.4 | Postcondition only `|= => Q` |
| parse_contract_14_5_4_entry_ok.cursive | C0X-14.5.4 | @entry intrinsic |
| parse_invariant_14_6_1_type_ok.cursive | C0X-14.6.1 | Type invariant `where { }` |
| parse_invariant_14_6_2_loop_ok.cursive | C0X-14.6.2 | Loop invariant `where { }` |

### Attributes Syntax

| Test File | Spec Rule | Description |
|-----------|-----------|-------------|
| parse_attr_dynamic_ok.cursive | C0X-Attr | [[dynamic]] attribute |
| parse_attr_static_dispatch_only_ok.cursive | C0X-8.5 | [[static_dispatch_only]] |
| parse_attr_multiple_ok.cursive | C0X-Attr | Multiple attributes |
| parse_attr_memory_ordering_ok.cursive | C0X-7.7 | Memory ordering [[seqcst]] |

### Shared Permission Syntax

| Test File | Spec Rule | Description |
|-----------|-----------|-------------|
| parse_shared_receiver_ok.cursive | C0X-Perm | Shared receiver `~%` |
| parse_shared_type_ok.cursive | C0X-Perm | Shared type `shared T` |
