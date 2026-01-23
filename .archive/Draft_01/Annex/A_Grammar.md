# Cursive Language Specification

## Annex A — Grammar

**Part**: Annex
**Section**: Annex A - Grammar
**Stable label**: [grammar]
**Forward references**: None

---

This appendix is the single normative source for all Cursive grammar productions in ANTLR-style notation. Every other chapter references these productions instead of re-declaring them. For readability, individual clauses may use simplified notation, but this annex contains the complete, authoritative grammar suitable for parser generation.

[ Note: This grammar uses ANTLR-style EBNF notation. For simplified notation used in prose, see individual clause grammar sections. The grammar here is suitable for direct use with ANTLR parser generators and similar tools.
— end note ]

## A.1 Lexical Grammar

```antlr
identifier
    : ident_start ident_continue*
    ;

ident
    : identifier
    ;

ident_start
    : [a-zA-Z_]
    ;

ident_continue
    : [a-zA-Z0-9_]
    ;

NEWLINE
    : '\n' | '\r\n'
    ;

separator
    : NEWLINE | ';'
    ;

line_comment
    : '//' ~[\n\r]* (NEWLINE | EOF)
    ;

block_comment
    : '/*' (~'*/' | block_comment)* '*/'
    ;

doc_comment
    : '///' ~[\n\r]*
    ;

module_doc
    : '//!' ~[\n\r]*
    ;

attribute
    : '[[' attribute_body ']]'
    ;

attribute_body
    : ident ('(' attr_args? ')')?
    ;

attr_args
    : attr_arg (',' attr_arg)*
    ;

attr_arg
    : ident '(' attr_args? ')'
    | ident '=' literal
    | literal
    | ident
    ;

literal
    : integer_literal
    | float_literal
    | boolean_literal
    | char_literal
    | string_literal
    ;

nat
    : digit (digit | '_')*
    ;

integer_literal
    : dec_literal integer_suffix?
    | hex_literal integer_suffix?
    | oct_literal integer_suffix?
    | bin_literal integer_suffix?
    ;

dec_literal
    : digit (digit | '_')*
    ;

hex_literal
    : '0x' hex_digit (hex_digit | '_')*
    ;

oct_literal
    : '0o' oct_digit (oct_digit | '_')*
    ;

bin_literal
    : '0b' bin_digit (bin_digit | '_')*
    ;

integer_suffix
    : 'i8' | 'i16' | 'i32' | 'i64' | 'i128' | 'isize'
    | 'u8' | 'u16' | 'u32' | 'u64' | 'u128' | 'usize'
    ;

float_literal
    : dec_literal '.' dec_literal? exponent? float_suffix?
    | dec_literal exponent float_suffix?
    | '.' dec_literal exponent? float_suffix?
    ;

exponent
    : ('e' | 'E') ('+' | '-')? dec_literal
    ;

float_suffix
    : 'f32' | 'f64'
    ;

boolean_literal
    : 'true' | 'false'
    ;

char_literal
    : '\'' char_body '\''
    ;

char_body
    : char_escape
    | ~['\\\n\r]
    ;

char_escape
    : '\\' [nrt\\'\"0]
    | '\\x' hex_digit hex_digit
    | '\\u{' hex_digit+ '}'
    ;

string_literal
    : '"' string_char* '"'
    ;

string_char
    : string_escape
    | ~["\\]
    ;

string_escape
    : '\\' [nrt\\'\"0]
    | '\\x' hex_digit hex_digit
    | '\\u{' hex_digit+ '}'
    ;

digit
    : [0-9]
    ;

hex_digit
    : [0-9a-fA-F]
    ;

oct_digit
    : [0-7]
    ;

bin_digit
    : [0-1]
    ;
```

[ Note: Newline-sensitive statement termination rules are specified in Clause 2 §2.4.
— end note ]

## A.2 Type Grammar

```antlr
type
    : primitive_type
    | compound_type
    | permission_type
    | pointer_type
    | safe_ptr_type
    | modal_type
    | witness_type
    | function_type
    | transition_type
    | grant_poly_type
    | generic_type
    | union_type
    | self_type
    | permission_hole_type
    | type_hole
    ;

primitive_type
    : 'i8' | 'i16' | 'i32' | 'i64' | 'i128' | 'isize'
    | 'u8' | 'u16' | 'u32' | 'u64' | 'u128' | 'usize'
    | 'f32' | 'f64'
    | 'bool' | 'char' | 'string' | '!' | '()'
    ;

compound_type
    : array_type
    | slice_type
    | tuple_type
    ;

array_type
    : '[' type ';' integer_literal ']'
    ;

slice_type
    : '[' type ']'
    ;

tuple_type
    : '(' type (',' type)+ ')'
    ;

permission_type
    : 'const' type
    | 'unique' type
    | 'shared' type
    ;

// Context-sensitive permission defaults:
// - Bindings (let/var): bare T means const T (default immutable)
// - Parameters: bare T means const T (default immutable)
// - Returns: bare T means const T (default immutable)
// - Fields: bare T means const T (default immutable)

pointer_type
    : '*' type
    | '*mut' type
    ;

safe_ptr_type
    : 'Ptr' '<' type '>'
    | 'Ptr' '<' type '>' '@' ident
    ;
    // Standard modal states for Ptr<T>: @Null, @Valid, @Weak, @Expired

modal_type
    : ident '@' modal_state
    ;

modal_state
    : ident
    | '_?'
    ;

witness_type
    : 'witness' '<' witness_property '>' allocation_state?
    ;

witness_property
    : ident
    | ident '@' ident
    ;

allocation_state
    : '@' ('Stack' | 'Region' | 'Heap')
    ;

function_type
    : '(' type_list? ')' '->' type grant_annotation?
    ;

transition_type
    : '@' ident '->' '@' ident
    ;

grant_poly_type
    : 'forall' ident grant_bound? '.' function_type
    ;

grant_bound
    : 'where' ident '<:' '{' grant_set '}'
    ;

generic_type
    : ident '<' type_args '>'
    ;

type_args
    : type (',' type)*
    ;

type_list
    : type (',' type)*
    ;

union_type
    : type '\\/' type
    ;

self_type
    : 'Self'
    ;

grant_annotation
    : '!' grant_set
    ;

type_hole
    : '_?'
    ;

permission_hole_type
    : '_?' type
    ;
```

[ Note (normative; see Clause 8 §8.2 and §8.7):

- `permission_hole_type` appears only where a permission wrapper is expected; elaboration chooses `perm⋆ ∈ {readonly, unique, shared, owned}` and yields `perm⋆ Type`.
- When `_?` is immediately followed by a `type` token in a position that admits permission wrappers, the parser recognizes `permission_hole_type` rather than `type_hole`.
  — end note ]

[ Note: Range type constructors (`Range<T>`, `RangeInclusive<T>`, etc.) are defined in Clause 2 §2.4.
— end note ]

## A.3 Pattern Grammar

```antlr
pattern
    : wildcard_pattern
    | literal_pattern
    | ident_pattern
    | tuple_pattern
    | record_pattern
    | enum_pattern
    | modal_pattern
    | pattern 'as' ident
    ;

wildcard_pattern
    : '_'
    ;

literal_pattern
    : literal
    ;

ident_pattern
    : identifier
    ;

tuple_pattern
    : '(' pattern (',' pattern)+ ')'
    ;

record_pattern
    : ident '{' field_pattern_list? '}'
    ;

field_pattern_list
    : field_pattern (',' field_pattern)* ','?
    ;

field_pattern
    : ident (':' pattern)?
    ;

enum_pattern
    : ident ('::' ident)? pattern_payload?
    ;

pattern_payload
    : '(' pattern_list? ')'
    | '{' field_pattern_list '}'
    ;

pattern_list
    : pattern (',' pattern)* ','?
    ;

modal_pattern
    : ident '@' modal_pattern_state pattern_payload?
    ;

modal_pattern_state
    : ident
    | '_?'
    ;
```

## A.4 Expression Grammar

```antlr
expr
    : seq_expr
    ;

seq_expr
    : assignment_expr
    | assignment_expr separator expr
    ;

assignment_expr
    : pipeline_expr (assign_op pipeline_expr)*
    ;

pipeline_expr
    : range_expr ('=>' range_expr ':' type)*
    ;

range_expr
    : logical_or_expr range_suffix
    | range_prefix logical_or_expr
    | '..'
    | logical_or_expr
    ;

range_suffix
    : '..' logical_or_expr?
    | '..=' logical_or_expr
    ;

range_prefix
    : '..'
    | '..='
    ;

logical_or_expr
    : logical_and_expr ('||' logical_and_expr)*
    ;

logical_and_expr
    : bitwise_or_expr ('&&' bitwise_or_expr)*
    ;

bitwise_or_expr
    : bitwise_xor_expr ('|' bitwise_xor_expr)*
    ;

bitwise_xor_expr
    : bitwise_and_expr ('^' bitwise_and_expr)*
    ;

bitwise_and_expr
    : equality_expr ('&' equality_expr)*
    ;

equality_expr
    : relational_expr (eq_op relational_expr)?
    ;

relational_expr
    : shift_expr (rel_op shift_expr)?
    ;

shift_expr
    : additive_expr (shift_op additive_expr)*
    ;

additive_expr
    : multiplicative_expr (add_op multiplicative_expr)*
    ;

multiplicative_expr
    : power_expr (mul_op power_expr)*
    ;

power_expr
    : unary_expr ('**' unary_expr)*
    ;

unary_expr
    : caret_prefix unary_expr
    | unary_op unary_expr
    | postfix_expr
    ;

caret_prefix
    : '^'+
    ;

postfix_expr
    : primary_expr postfix_suffix*
    ;

postfix_suffix
    : '.' ident
    | '::' ident '(' argument_list? ')'
    | '.' integer_literal
    | '[' expr ']'
    | '[' range_suffix ']'
    | '(' argument_list? ')'
    | 'as' type
    | '?'
    ;

primary_expr
    : literal
    | identifier
    | expr_hole
    | tuple_expr
    | record_expr
    | enum_expr
    | array_expr
    | unit_expr
    | '(' expr ')'
    | block_expr
    | if_expr
    | match_expr
    | loop_expr
    | closure_expr
    | region_expr
    | comptime_expr
    | 'move' expr
    | 'contract' '(' expr ',' expr ',' expr ')'
    | 'transition' '(' expr ',' '@' ident ')'
    | 'return' expr?
    | 'defer' block_expr
    | 'continue' '(' expr ')'
    | 'abort' '(' expr ')'
    ;

unit_expr
    : '()'
    ;

tuple_expr
    : '(' expr ',' expr (',' expr)* ','? ')'
    ;

record_expr
    : ident '{' field_init_list? '}'
    ;

field_init_list
    : field_init (',' field_init)* ','?
    ;

field_init
    : ident ':' expr
    | ident
    ;

tuple_struct_expr
    : ident '(' expr (',' expr)* ','? ')'
    ;

enum_expr
    : ident '::' ident enum_payload?
    ;

enum_payload
    : '(' argument_list? ')'
    | '{' field_init_list '}'
    ;

array_expr
    : '[' expr (',' expr)* ','? ']'
    | '[' expr ';' expr ']'
    ;

slice_expr
    : expr '[' range ']'
    ;

range
    : expr '..' expr          // exclusive end
    | expr '..=' expr         // inclusive end
    | '..' expr               // from start
    | '..=' expr              // from start, inclusive end
    | expr '..'               // to end
    | '..'                    // full slice
    ;

argument_list
    : expr (',' expr)* ','?
    ;

if_expr
    : 'if' if_head block_expr ('else' (if_expr | block_expr))?
    ;

if_head
    : expr
    | 'let' pattern '=' expr
    ;

match_expr
    : 'match' expr '{' match_arm+ '}'
    ;

match_arm
    : pattern ('if' expr)? '=>' expr ','
    ;

loop_expr
    : 'loop' loop_head? loop_verif? loop_block
    ;

loop_head
    : pattern ':' type 'in' expr
    | expr
    ;

loop_verif
    : ('with' '{' assertion_list '}')? ('by' expr)?
    ;

loop_block
    : block_expr
    ;

block_expr
    : block_stmt
    ;

closure_expr
    : '|' param_list? '|' '->' expr
    | '|' param_list? '|' block_expr
    ;

region_expr
    : 'region' ident ('as' ident)? block_expr
    ;

comptime_expr
    : 'comptime' block_expr
    ;

assign_op
    : '=' | '+=' | '-=' | '*=' | '/=' | '%=' | '&=' | '|=' | '^=' | '<<=' | '>>='
    ;

unary_op
    : '-' | '!' | '*' | '&'
    ;
    // Note: ^, &, * are position-disambiguated:
    //  - Prefix position: unary (region alloc, address-of, deref)
    //  - Infix position: binary (XOR, AND, multiply)

add_op
    : '+' | '-'
    ;

mul_op
    : '*' | '/' | '%'
    ;

shift_op
    : '<<' | '>>'
    ;

eq_op
    : '==' | '!='
    ;

rel_op
    : '<' | '>' | '<=' | '>='
    ;

// Hole production for expressions (Clause 8 §8.5)
expr_hole
    : '_?' (':' type)?
    ;
```


## A.5 Statement Grammar

```antlr
statement
    : block_stmt
    | var_decl_stmt
    | assign_stmt
    | use_decl
    | expr_stmt
    | labeled_stmt
    | return_stmt
    | break_stmt
    | continue_stmt
    | defer_stmt
    | empty_stmt
    | grant_gated_branch
    | loop_with_region
    | with_block
    ;

block_stmt
    : '{' statement* '}'
    | '{' statement* 'result' expr '}'
    ;

var_decl_stmt
    : 'let' pattern (':' type)? binding_op expr
    | 'var' pattern (':' type)? binding_op expr
    | 'shadow' 'let' pattern (':' type)? binding_op expr
    | 'shadow' 'var' pattern (':' type)? binding_op expr
    ;

binding_op
    : '='
    | '<-'
    ;

assign_stmt
    : l_value assign_op expr
    ;

l_value
    : identifier
    | expr '.' ident
    | expr '.' integer_literal
    | expr '[' expr ']'
    | '*' expr
    ;

expr_stmt
    : expr
    ;

labeled_stmt
    : '\'' ident ':' statement
    ;

return_stmt
    : 'return' expr?
    ;

break_stmt
    : 'break' label_ref? expr?
    ;

continue_stmt
    : 'continue' label_ref?
    ;

label_ref
    : '\'' ident
    ;

defer_stmt
    : 'defer' block_stmt
    ;

empty_stmt
    : ';'
    ;

grant_gated_branch
    : 'comptime' 'if' grant_predicate block_stmt ('else' block_stmt)?
    ;

grant_predicate
    : 'grants_include' '(' grant_set ')'
    | 'grants_exclude' '(' grant_set ')'
    ;

loop_with_region
    : 'loop' pattern ':' type 'in' expr 'region' ident block_stmt
    ;

with_block
    : 'with' grant_list '{' implementation_list '}' block_stmt
    ;

grant_list
    : grant_path (',' grant_path)*
    ;

implementation_list
    : implementation (',' implementation)*
    ;

implementation
    : grant_path '.' ident '(' param_list? ')' '=>' expr
    ;
```

[ Note: `loop_with_region` is syntactic sugar for iterator loops with region allocation as described in Clause 11 §11.3 [memory.region].
— end note ]

[ Note: `with_block` provides custom implementations for grant operations within a specific scope. Complete grant semantics are specified in Clause 12 §12.3 [contract.grant].
— end note ]

## A.6 Declaration Grammar

```antlr
top_level
    : (decl | use_decl)*
    ;

use_decl
    : visibility_modifier? 'use' use_clause
    | 'import' module_path ('as' ident)?
    ;

decl
    : var_decl_stmt
    | type_decl
    | procedure_decl
    | contract_decl
    | behavior_decl
    | grant_decl
    ;

type_decl
    : 'type' ident generic_params? '=' type where_clause?
    | record_decl
    | tuple_struct_decl
    | enum_decl
    | union_decl
    | modal_decl
    ;

record_decl
    : attribute* visibility? 'record' ident generic_params?
      contract_clause_list?
      behavior_clause?
      where_clause?
      record_body
    ;

contract_clause_list
    : ':' contract_ref (',' contract_ref)*
    ;

behavior_clause
    : 'with' behavior_ref (',' behavior_ref)*
    ;

invariant
    : assertion
    ;

record_body
    : '{' record_item* '}'
    ;

record_item
    : partition_directive
    | record_member
    ;

record_member
    : record_field
    | procedure_decl
    ;

record_field
    : visibility? ident ':' type
    ;

partition_directive
    : '<<' (ident | '_') '>>'
    ;

tuple_struct_decl
    : attribute* visibility? 'record' ident '(' field_type (',' field_type)* ','? ')'
      ('{' procedure_decl* '}')?
    ;

field_type
    : visibility? type
    ;

enum_decl
    : attribute* visibility? 'enum' ident generic_params? where_clause? enum_body
    ;

enum_body
    : '{' enum_variant (',' enum_variant)* ','? '}'
    ;

enum_variant
    : ident pattern_payload?
    ;

union_decl
    : attribute* visibility? 'union' ident generic_params? where_clause? union_body
    ;

union_body
    : '{' union_field+ '}'
    ;

union_field
    : ident ':' type
    ;

modal_decl
    : attribute* visibility? 'modal' ident generic_params? where_clause? modal_body
    ;

modal_body
    : '{' modal_state+ state_coercion* procedure_decl* '}'
    ;

// Built-in Arena modal type (informative - actual declaration is internal)
// modal Arena {
//     @Active { ptr: Ptr<u8>, capacity: usize, allocated: usize }
//     @Active::alloc<T>(~!, value: T) -> @Active
//     @Active::alloc_array<T>(~!, count: usize) -> @Active
//     @Active::reset(~!) -> @Active
//     @Active::freeze(~) -> @Frozen
//     @Active::free(~!) -> @Freed
//
//     @Frozen { ptr: Ptr<u8>, allocated: usize }
//     @Frozen::thaw(~!) -> @Active
//     @Frozen::free(~!) -> @Freed
//
//     @Freed { }
// }

modal_state
    : '@' ident record_body where_clause?
    ;

state_coercion
    : 'coerce' '@' ident '<:' '@' ident coercion_constraint?
    ;

coercion_constraint
    : '{'
      ('cost' ':' cost ','?)?
      ('requires' ':' permission ','?)?
      ('grants' ':' grant_set ','?)?         // Canonical form
      '}'
    ;

cost
    : 'O(1)' | 'O(n)' | integer_literal
    ;

procedure_decl
    : attribute* visibility? procedure_kind transition_spec?
      ident generic_params?
      '(' param_list? ')' (':' type)?
      where_clause?
      contract_clause?
      callable_body
    ;

procedure_kind
    : 'procedure'
    | 'comptime' 'procedure'
    | 'extern' string_literal? 'procedure'
    ;

callable_body
    : block_stmt
    | '=' expression ';'
    | ';'
    ;

transition_spec
    : '@' ident '->' '@' ident
    ;

param_list
    : param (',' param)*
    ;

param
    : param_modifier? ident ':' type
    | self_param
    ;

param_modifier
    : 'move'
    ;

self_param
    : '~'            // Shorthand for self: const Self
    | '~%'           // Shorthand for self: shared Self
    | '~!'           // Shorthand for self: unique Self
    | 'self' ':' permission? self_type
    ;

contract_decl
    : attribute* visibility? 'contract' ident generic_params?
      contract_extends_clause? where_clause? contract_body
    ;

contract_extends_clause
    : ':' contract_ref (',' contract_ref)*
    ;

contract_ref
    : ident ('<' type_args '>')?
    ;

contract_body
    : '{' contract_item* '}'
    ;

contract_item
    : associated_type_decl
    | procedure_signature
    ;

associated_type_decl
    : 'type' ident type_bound?
    ;

type_bound
    : ':' behavior_bounds
    ;

procedure_signature
    : 'procedure' ident '(' param_list? ')' (':' type)?
      where_clause?
      contract_clause*
    ;

type_projection
    : ident '::' ident
    | 'Self' '::' ident
    ;

behavior_decl
    : attribute* visibility? 'behavior' ident generic_params?
      behavior_extension?
      where_clause?
      '{' behavior_item* '}'
    ;

grant_decl
    : visibility? 'grant' ident
    ;

behavior_extension
    : 'with' behavior_ref (',' behavior_ref)*
    ;

behavior_item
    : procedure_decl
    | type_decl
    ;

qualified_name
    : module_path '::' ident          // module_path::identifier
    | ident '::' ident                // alias::identifier (alias is an ident introduced by import)
    ;

module_path
    : ident ('::' ident)*
    ;

visibility_modifier
    : 'public'                    // Limited to 'public' for re-export intent in use declarations
    ;

qualified_path
    : module_path
    | module_path '::' ident
    ;

use_clause
    : qualified_path
    | qualified_path 'as' ident
    | qualified_path '::' '{' use_list '}'
    | qualified_path '::' '*'
    ;

use_list
    : use_specifier (',' use_specifier)*
    ;

use_specifier
    : ident
    | ident 'as' ident
    ;

ident_list
    : ident (',' ident)* ','?
    ;

visibility
    : 'public' | 'internal' | 'private' | 'protected'
    ;

generic_params
    : '<' generic_param (',' generic_param)* '>'
    ;

generic_param
    : type_param
    | const_param
    | grant_param
    ;

type_param
    : ident (':' behavior_bounds)?
    ;

const_param
    : 'const' ident ':' type
    ;

grant_param
    : ident                             // Grant parameter, e.g., G
    ;

behavior_bounds
    : behavior_ref ('+' behavior_ref)*
    ;

behavior_ref
    : ident ('<' type_args '>')?
    ;

where_clause
    : 'where' where_predicate (',' where_predicate)*
    ;

where_predicate
    : ident ':' behavior_bounds
    | type ':' behavior_bounds
    | ident '<:' '{' grant_set '}'        // Grant parameter bounds
    ;

permission
    : 'const' | 'unique' | 'shared'
    ;
```

## A.7 Contract Grammar

```antlr
contract_clause
    : sequent_clause
    ;

sequent_clause
    : '[[' sequent_spec ']]'
    ;

sequent_spec
    : grant_set '|-' antecedent '=>' consequent
    | grant_set '|-' antecedent
    | grant_set '|-' '=>' consequent
    | grant_set '|-'
    | '|-' antecedent '=>' consequent
    | '|-' antecedent '=>'
    | '|-' '=>' consequent
    | '|-' antecedent
    | antecedent '=>' consequent
    | grant_set
    ;

antecedent
    : predicate_block
    ;

consequent
    : predicate_block
    ;

predicate_block
    : assertion
    | assertion_list
    ;

assertion_list
    : assertion ('&&' assertion)*
    ;
```

[ Note: Explanation:

- `sequent_clause` — Contractual sequent in sequent calculus form, delimited by semantic brackets `⟦ ⟧` (Unicode U+27E6, U+27E7) or ASCII equivalent `[[ ]]`
- `sequent_spec` — The sequent proper: `grants ⊢ P ⇒ Q`
  - `grants` — Grant context (capability assumptions) as comma-separated list
  - `|-` — Turnstile (entailment), ASCII representation of `⊢`
  - `P` — Antecedent (preconditions)
  - `=>` — Implication, ASCII representation of `⇒`
  - `Q` — Consequent (postconditions)
- Sequent components may be omitted with smart defaulting:
  - Grant-only: `[[ io::write ]]` expands to `[[ io::write |- true => true ]]`
  - Precondition-only: `[[ x > 0 ]]` expands to `[[ x > 0 => true ]]` (canonical: `[[ |- x > 0 => true ]]`)
  - Postcondition-only: `[[ => result > 0 ]]` expands to `[[ true => result > 0 ]]` (canonical: `[[ |- true => result > 0 ]]`)
  - No grants: `[[ P => Q ]]` (preferred form, canonical: `[[ |- P => Q ]]`)
  - Pure function: entire sequent clause may be omitted, defaults to `[[ ∅ |- true => true ]]` (empty grant set, canonical form)

Conjunction within antecedents and consequents uses explicit `&&` operator, not comma separation.

The semantic brackets `⟦ ⟧` / `[[ ]]` are consistent with their use for type value sets (e.g., `⟦ bool ⟧ = {true, false}`) throughout this specification, creating a unified notation for formal semantic content.
— end note ]


## A.8 Assertion Grammar

```antlr
assertion
    : logic_expr
    ;

logic_expr
    : logic_expr '&&' logic_expr
    | logic_expr '||' logic_expr
    | '!' logic_expr
    | logic_term
    ;

logic_term
    : expr compare_op expr
    | 'forall' '(' ident 'in' expr ')' '{' assertion '}'
    | 'exists' '(' ident 'in' expr ')' '{' assertion '}'
    | '@old' '(' expr ')'
    | 'result'
    | expr
    ;

compare_op
    : '==' | '!=' | '<' | '>' | '<=' | '>='
    ;
```

## A.9 Grant Grammar

[ Note (normative): This section defines the grammar for grant specifications, which track compile-time capabilities and restrictions on procedure operations.
— end note ]

```antlr
grant_set
    : grant_ref (',' grant_ref)* ','?
    ;

grant_ref
    : grant_path
    | ident                       // Grant parameter reference (simple identifier)
    | '_?'                        // Grant inference hole
    ;

grant_path
    : ident ('::' ident)*       // e.g., alloc::heap, fs::write
    ;

// Built-in grant categories
alloc_grant
    : 'alloc' '::' ('heap' | 'region' | 'stack' | 'temp' | '*')
    ;

file_system_grant
    : 'fs' '::' ('read' | 'write' | 'delete' | 'metadata' | '*')
    ;

network_grant
    : 'net' '::' ('read' | 'write' | 'connect' | 'listen' | '*')
    ;

time_grant
    : 'time' '::' ('read' | 'sleep' | 'monotonic' | '*')
    ;

thread_grant
    : 'thread' '::' ('spawn' | 'join' | 'atomic' | '*')
    ;

ffi_grant
    : 'ffi' '::' ('call' | '*')
    ;

unsafe_grant
    : 'unsafe' '::' ('ptr' | 'transmute' | '*')
    ;

panic_grant
    : 'panic'
    ;
```

[ Note: Grant parameters are declared in generic parameter lists (§A.6) and referenced in grant sets using `grants<G>` syntax. — end note ]

**Example A.9.1 (Grant polymorphism):**

```cursive
procedure process<T, G>(data: T): Result<T>
    grants<G>, alloc::heap
    where G <: {fs::read, net::read}
{
    // Procedure body can use grants<G> and alloc::heap
}
```

[ Note: Grant Inference: The `_?` hole allows the compiler to infer required grants based on procedure body (see Clause 8 §8.x and Clause 9 §9.x).
— end note ]

[ Note: Forward Reference: Complete grant semantics, propagation rules, and verification are specified in Clause 12 [contract] (Contracts and Grants).
— end note ]

---

## A.10 Consolidated Grammar Reference

[1] This section provides a consolidated reference grouping all grammar productions by syntactic category for quick lookup. The authoritative definitions appear in §§A.1–A.9 above; this section provides navigation only.

### A.10.1 Lexical Productions

**Identifiers**: `identifier`, `ident_start`, `ident_continue`  
**Literals**: `literal`, `integer_literal`, `float_literal`, `boolean_literal`, `char_literal`, `string_literal`  
**Numeric**: `dec_literal`, `hex_literal`, `oct_literal`, `bin_literal`, `integer_suffix`, `float_suffix`  
**Comments**: `line_comment`, `block_comment`, `doc_comment`, `module_doc`  
**Separators**: `NEWLINE`, `separator`  
**Attributes**: `attribute`, `attribute_body`, `attr_args`, `attr_arg`

### A.10.2 Type Productions

**Primitive types**: `primitive_type` (integers, floats, bool, char, string, unit, never)  
**Compound types**: `array_type`, `slice_type`, `tuple_type`  
**Permission types**: `permission_type` (const, unique, shared)  
**Pointer types**: `pointer_type` (raw), `safe_ptr_type` (modal)  
**Modal types**: `modal_type`, `modal_state`  
**Witness types**: `witness_type`, `witness_property`, `allocation_state`  
**Function types**: `function_type`, `transition_type`  
**Generic types**: `generic_type`, `type_args`  
**Union types**: `union_type`  
**Special**: `self_type`, `type_hole`, `permission_hole_type`

### A.10.3 Pattern Productions

**Patterns**: `pattern`, `wildcard_pattern`, `literal_pattern`, `ident_pattern`, `tuple_pattern`, `record_pattern`, `enum_pattern`, `modal_pattern`  
**Pattern components**: `field_pattern_list`, `pattern_payload`, `pattern_list`

### A.10.4 Expression Productions

**Expression hierarchy**: `expr`, `seq_expr`, `assignment_expr`, `pipeline_expr`, `range_expr`  
**Logical operators**: `logical_or_expr`, `logical_and_expr`  
**Bitwise operators**: `bitwise_or_expr`, `bitwise_xor_expr`, `bitwise_and_expr`  
**Comparison**: `equality_expr`, `relational_expr`  
**Arithmetic**: `additive_expr`, `multiplicative_expr`, `power_expr`, `shift_expr`  
**Unary**: `unary_expr`, `unary_op`, `caret_prefix`  
**Postfix**: `postfix_expr`, `postfix_suffix` (includes `'as' type` cast operator)  
**Primary**: `primary_expr`, `literal`, `identifier`, `tuple_expr`, `record_expr`, `enum_expr`, `array_expr`, `unit_expr`  
**Control flow**: `if_expr`, `match_expr`, `loop_expr`, `block_expr`  
**Advanced**: `closure_expr`, `region_expr`, `comptime_expr`  
**Operators**: `assign_op`, `add_op`, `mul_op`, `shift_op`, `eq_op`, `rel_op`, `'as'` (type cast)

### A.10.5 Statement Productions

**Statements**: `statement`, `block_stmt`, `var_decl_stmt`, `assign_stmt`, `expr_stmt`, `labeled_stmt`, `return_stmt`, `break_stmt`, `continue_stmt`, `defer_stmt`, `empty_stmt`  
**L-values**: `l_value`  
**Labels**: `label_ref`  
**Special**: `grant_gated_branch`, `loop_with_region`, `with_block`

### A.10.6 Declaration Productions

**Top level**: `top_level`, `use_decl`, `decl`  
**Module syntax**: `visibility_modifier`, `qualified_path`, `use_clause`, `use_list`, `use_specifier`, `module_path`, `qualified_name`  
**Types**: `type_decl`, `record_decl`, `tuple_struct_decl`, `enum_decl`, `union_decl`, `modal_decl`  
**Procedures**: `procedure_decl`, `procedure_kind`, `callable_body`, `param_list`, `param`, `param_modifier`, `self_param`  
**Contracts**: `contract_decl`, `contract_extends_clause`, `contract_body`, `contract_item`, `procedure_signature`  
**Behaviors**: `behavior_decl`, `behavior_extension`, `behavior_item`  
**Grants**: `grant_decl`  
**Components**: `record_body`, `record_item`, `record_member`, `record_field`, `partition_directive`, `enum_body`, `enum_variant`, `modal_body`, `modal_state`  
**Generics**: `generic_params`, `generic_param`, `type_param`, `const_param`, `grant_param`, `behavior_bounds`, `where_clause`, `where_predicate`  
**Visibility**: `visibility`, `visibility_modifier`  
**Associated types**: `associated_type_decl`, `type_projection`  
**Qualifiers**: `permission`, `binding_op`

### A.10.7 Contract Grammar Productions

**Sequents**: `contract_clause`, `sequent_clause`, `sequent_spec`  
**Components**: `antecedent`, `consequent`, `predicate_block`, `assertion_list`  
**Reference**: Complete sequent syntax specified in §A.7

### A.10.8 Assertion Grammar Productions

**Assertions**: `assertion`, `logic_expr`, `logic_term`  
**Operators**: `compare_op`  
**Quantifiers**: `forall`, `exists` (in logic_term)  
**Special**: `@old` operator, `result` identifier

### A.10.9 Grant Grammar Productions

**Grants**: `grant_set`, `grant_ref`, `grant_path`  
**Built-in categories**: `alloc_grant`, `file_system_grant`, `network_grant`, `time_grant`, `thread_grant`, `ffi_grant`, `unsafe_grant`, `panic_grant`  
**Reference**: Complete grant semantics in Clause 12 §12.3

### A.10.10 Grammar Production Cross-Reference Table

[2] Table A.10 provides a complete index of all grammar productions defined in this annex:

| Production     | Section | Category    | Used In                   |
| -------------- | ------- | ----------- | ------------------------- |
| identifier     | A.1     | Lexical     | All constructs            |
| type           | A.2     | Type        | Declarations, annotations |
| pattern        | A.3     | Pattern     | Match, destructuring      |
| expr           | A.4     | Expression  | All executable contexts   |
| statement      | A.5     | Statement   | Procedure bodies          |
| decl           | A.6     | Declaration | Module scope              |
| grant_decl     | A.6     | Declaration | Grant declarations        |
| sequent_clause | A.7     | Contract    | Procedure signatures      |
| assertion      | A.8     | Assertion   | Contracts, invariants     |
| grant_set      | A.9     | Grant       | Sequent grants clause     |

[3] The grammar is complete and unambiguous. All productions are defined with clear precedence and associativity. Implementations should be able to generate parsers directly from these ANTLR-style productions.

---

**Previous**: Annex A §A.9 Grant Grammar | **Next**: Annex B — Behavior Classification ([behavior])
