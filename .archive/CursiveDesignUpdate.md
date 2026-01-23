# **Cursive Language Specification (Design 2.0)**

**Editor's Note:** This document constitutes the second major draft of the Cursive Language Specification. It formally supersedes Cursive-Draft01.md and integrates the proposals from all supplemental design documents.

The following core systems from Draft 1.0 have been **ABOLISHED AND REPLACED**:

* **The \[\[grants\]\] System (Draft 1.0, Clause 12):** Replaced by a first-class **Object-Capability (O-Cap) System** (see Clause 12).  
* **behavior and contract (Draft 1.0, Clauses 10, 12):** Replaced by a single, unified **trait System** FOR DEFINING INTERFACES (see Clause 10).  
* **The witness\<T\> Type (Draft 1.0, Clause 13):** Functionality is replaced by dynamic "trait-as-type" polymorphism (item: Trait) FOR DYNAMIC_TYPES. WITNESSES ARE STILL USED AS RUNTIME CONTRACT VALIDTATION. (see Clause 10).  
* **The ProcedureSpec API (Draft 1.0, Clause 15):** Replaced by a declarative, LLM-friendly **quote/emit Metaprogramming System** (see Clause 15).  
* **Non-Responsible Bindings (\<-):** The \<- operator is **forbidden** in safe code. Its functionality is replaced by the Ptr\<T\>@State and Permission systems (see Clause 11).

This document is the new single source of truth.

## **Clause 1 — Introduction and Conformance**

### **§1.1 Scope \[intro.scope\]**

\[1\] This clause establishes the boundaries of the Cursive language specification, defines the actors it serves, and identifies complementary material external to the core standard.

\[2\] The specification governs every program element required to compile and execute portable, deterministic Cursive code. The governed areas are:

* lexical structure and translation phases (Clauses 2-4);  
* declarations, names and scopes, types, and expressions (Clauses 5-8);  
* statements and control flow (Clause 9);  
* the traits, permissions, and memory model systems that ensure memory safety and semantic guarantees (Clauses 10–11);  
* the object-capability, concurrency, interoperability, and compile-time evaluation systems (Clauses 12–15).

\[3\] The specification intentionally excludes topics whose evolution is decoupled from language conformance. Excluded topics include:

* the normative standard library catalogue;  
* platform-specific ABI and calling convention details left to implementation policy (§14.6 \[interop.abi\]);  
* tooling, build systems, or project layout conventions.

\[4\] Certain behaviors are implementation-defined but remain within the specification's boundary.

\[5\] Undefined behavior is catalogued centrally in Annex B.2 \[behavior.undefined\].

### **§1.2 Normative References \[intro.refs\]**

\[1\] The documents listed here contain provisions that, through citation, become requirements of the Cursive specification.

\[2\] ISO/IEC 10646:2020 — Universal Coded Character Set (UCS).

\[3\] ISO/IEC 60559:2020 (IEEE 754-2019) — Floating-Point Arithmetic.

\[4\] The Unicode Standard, Version 15.0.0 or later.

\[5\] Unicode Standard Annex \#31 — Unicode Identifier and Pattern Syntax.

### **§1.3 Terms and Definitions \[intro.terms\]**

*(This section is revised to reflect the new 2.0 design.)*

#### **§1.3.1 General Language Terms \[intro.terms.general\]**

\[2\] binding — the association between an identifier and an entity within a particular scope.

\[3\] compilation unit — the smallest source artifact processed as a whole.

\[4\] conforming implementation — any compiler or tool that meets the requirements of §1.5.

\[5\] conforming program — a program that satisfies the grammar, static semantics, and behavioral requirements.

\[6\] declaration — a syntactic form that introduces a name.

\[7\] diagnostic — a message issued when a program violates a rule.

\[8\] entity — any value, type, module, procedure, or trait that may be named.

\[9\] expression — a syntactic form that yields a value or place.

\[10\] statement — a syntactic form that executes for effects.

\[11\] scope — the syntactic region where a binding is visible.

#### **§1.3.2 Cursive-Specific Concepts \[intro.terms.language\]**

\[12\] permission — a type qualifier (const, partitioned, unique) describing how a value may be accessed (§11.3).

\[13\] capability — A first-class object representing the authority to perform an observable effect (e.g., I/O, allocation) (§12.1).

\[14\] Context — The root capability object (e.g., ctx: Context) passed to main, which holds all available system capabilities (§12.1).

\[15\] sequent — a formal specification of procedure requirements and guarantees, using the form \[\[ must \=\> will \]\] (§12.5).

\[16\] trait — a unified declaration that defines an abstract interface, provides concrete implementations, or both. Replaces behavior and contracts AS INTERFACES. Contracts are still in the spec as procedure and type-level constraints. (Formerly 'sequents' and 'where' clause) (§10.1).

\[17\] polymorphism (static) — Zero-cost generic polymorphism using constrained parameters: \<T \<: Trait\> (§10.3).

\[18\] polymorphism (dynamic) — Runtime polymorphism using trait objects as types: item: Trait (§10.4).

\[19\] polymorphism (opaque) — Zero-cost encapsulation of return types: (): MyType \<: Trait (§10.5).

\[20\] modal type — a type whose values transition through named states validated at compile time (e.g., Ptr\<T\>@State) (§7.5).

\[21\] static invalidation — The compile-time process where a binding is rendered unusable after its ownership is transferred via move (§11.2).

\[22\] partitioning — The aliasing-safety system for partitioned data (§11.4).

\[23\] region — a lexically-scoped allocation arena (region { ... }) for O(1) bulk allocation (§11.6).

\[24\] ^ (caret) — The region allocation operator (§11.6).

\[25\] parallel / fork — The zero-cost static concurrency model for CREW patterns (§13.2).

\[26\] quote / emit — The declarative metaprogramming system for compile-time code generation (§15.2, §15.4).

#### **§1.3.3 Behavioral Classifications \[intro.terms.behaviour\]**

\[27\] implementation-defined behavior — behavior where the specification provides a set of allowed outcomes and requires documentation.

\[28\] unspecified behavior — behavior for which multiple outcomes are permitted without documentation.

\[29\] undefined behavior (UB) — behavior for which the specification imposes no requirements.

\[30\] ill-formed program — a program that violates a static semantic rule requiring diagnosis.

### **§1.4 Notation and Conventions \[intro.notation\]**

\[1\] This subclause formalises the notational systems used throughout the specification.

\[2\] Grammar fragments appear in ebnf blocks. Productions use ::= for definition, | for alternative, \* for zero-or-more, \+ for one-or-more, and ? for optional.

\[3\] Metavariables: $x, y, z$ for variables; $T, U, V$ for types; $e$ for expressions; $s$ for statements; $\\Gamma$ for contexts.

\[4\] Inference rules:

$$\\frac{\\text{premise}\_1 \\quad \\cdots \\quad \\text{premise}\_n}{\\text{conclusion}}\\tag{\\text{Rule-Name}}$$

### **§1.5 Conformance \[intro.conformance\]**

\[1\] This subclause defines what it means to conform to the Cursive specification.

\[2\] The keywords **shall** and **shall not** express absolute requirements. **Should** and **should not** express strong recommendations. **May** describes permissions.

\[3\] A conforming implementation shall accept all conforming programs and shall issue a diagnostic for any ill-formed program.

### **§1.8 Design Decisions and Absent Features \[intro.absent\]**

\[1\] Cursive achieves memory safety through explicit mechanisms that differ from other systems programming languages.

#### **§1.8.1 No Garbage Collection \[intro.absent.gc\]**

\[2\] Cursive does not employ garbage collection. Resource cleanup is deterministic via RAII (§11.2).

#### **§1.8.2 No Borrow Checker \[intro.absent.borrow\]**

\[5\] Cursive achieves memory safety without borrow checking or lifetime annotations. The design uses a combination of:

* **RAII and Static Invalidation** (§11.2)  
* **Modal Pointers** for liveness (Ptr\<T\>@State) (§7.5)  
* **Permissions** (const, unique, partitioned) (§11.3)  
* **The Partitioning System** for partitioned aliasing (§11.4)

\[ Rationale: This stack of explicit, composable, and local-reasoning-friendly mechanisms provides memory safety without the complexity of a full borrow-checking system. \]

#### **§1.8.3 No Send/Sync Markers \[intro.absent.markers\]**

\[6\] Thread safety is encoded through the permission system, not separate marker traits.

* **const types** are safe to share across threads (immutable).  
* **unique types** are safe to transfer between threads via move.  
* **partitioned types** are *not* thread-safe and must be wrapped in Mutex\<T\> or accessed only within a parallel block.

## **Clause 2 — Lexical Structure and Translation**

### **§2.1 Source Text and Encoding \[lex.source\]**

\[1\] Cursive source input is a sequence of Unicode scalar values encoded as UTF-8.

\[2\] Implementations shall normalize \\n, \\r, and \\r\\n sequences to a single \\n.

\[3\] The byte order mark (U+FEFF) is stripped if present at the start of the file, and is an error otherwise.

### **§2.2 Translation Phases \[lex.phases\]**

\[1\] Compilation proceeds through a deterministic pipeline:

1. **Parsing:** Source text is converted to an AST. All declarations are recorded.  
2. **Compile-Time Execution:** comptime blocks and procedures are executed (§15.1).  
3. **Type Checking:** The AST is semantically validated against type, memory, and capability rules.  
4. **Code Generation:** The validated AST is lowered to an intermediate representation and then to target machine code.

\[2\] Cursive uses a two-phase model: parsing completes before semantic analysis (type checking, etc.), allowing for declarations to appear in any order within a file.

### **§2.3 Lexical Elements \[lex.tokens\]**

\[1\] Lexical analysis transforms the character stream into tokens: identifier, keyword, literal, operator, punctuator, or newline.

\[2\] Whitespace and comments (except documentation comments) are discarded.

\[3\] Keywords (Reserved):

abstract, as, comptime, const, continue, else, enum, extern, false, fork, if, import, internal, let, loop, match, modal, module, move, override, parallel, private, procedure, protected, public, quote, record, region, result, return, Self, self, shadow, partitioned, static, true, trait, type, unique, unsafe, use, var, where

\[4\] **Literals:**

* **Integers:** 123, 0xABC, 0o755, 0b1010\_0101. Suffixes like u8, i64, usize are supported.  
* **Floats:** 1.23, 1.0e-5, 2.f32.  
* **Booleans:** true, false.  
* **Characters:** 'a', '\\n', '\\u{1F4A1}'.  
* **Strings:** "Hello, world".

### **§2.4 Tokenization and Statement Termination \[lex.terminators\]**

\[1\] A newline token terminates a statement unless one of the following continuation conditions is met:

* An unclosed delimiter ((, \[, {) exists.  
* The line ends with a binary operator (e.g., \+, \-, \==, \=\>).  
* The next line begins with a leading . (for field access).

\[2\] Semicolons (;) are optional terminators and may be used to separate multiple statements on a single line.

### **§2.5 Compilation Units and Top-Level Forms \[lex.units\]**

\[1\] A compilation unit (a single source file) consists of zero or more top-level items.

\[2\] **Top-Level Items:**

top\_level\_item

    ::= import\_declaration

     | use\_declaration

     | variable\_declaration

     | procedure\_declaration

     | type\_declaration

     | trait\_declaration

\[3\] Expression statements, control-flow constructs, and local bindings are not permitted at module scope.

\[4\] **Example (Valid Compilation Unit):**

use std::io::println // 'use' is hypothetical, O-Cap model (§12) is preferred

public record Point { x: f64, y: f64 }

let ORIGIN: const \= Point { x: 0.0, y: 0.0 }

public procedure distance(a: Point, b: Point): f64

    \[\[ \]\] // Empty sequent (pure)

{

    let dx \= b.x \- a.x

    let dy \= b.y \- a.y

    result (dx \* dx \+ dy \* dy).sqrt()

}

## **Clause 3 — Modules (Overview)**

### **§3.1 Module Overview and File-Based Organization \[module.overview\]**

\[1\] A *module* is the fundamental namespace unit in Cursive. Each compilation unit (§2.5) defines exactly one module.

\[2\] Module paths are derived from the filesystem. A file at source/math/geometry.cursive relative to a source root source/ defines the module math::geometry.

## **Clause 4 — Module (Syntax and Semantics)**

### **§4.1 Import Declarations \[module.import\]**

\[1\] import declarations make modules from external packages available for use.

\[2\] import is not used for modules within the same project/assembly; intra-project modules are always available via their qualified paths.

import\_declaration ::= "import" module\_path ("as" identifier)?

module\_path ::= identifier ("::" identifier)\*

import external\_crypto::aes

import external\_logging as log

### **§4.2 Use Declarations \[module.use\]**

\[1\] use declarations bring symbols from other modules into the current scope, permitting unqualified access.

use\_declaration ::= visibility\_modifier? "use" use\_clause

use\_clause

    ::= qualified\_path

     | qualified\_path "as" identifier

     | qualified\_path "::" "{" use\_list "}"

     | qualified\_path "::" "\*"

use math::geometry::Point

use math::geometry::Vector as Vec3

use math::constants::\*

let p \= Point{...} // OK

let v \= Vec3{...}  // OK

let pi \= PI        // OK

### **§4.3 Qualified Names and Resolution \[module.qualified\]**

\[1\] All items in a project are accessible via their fully qualified path (e.g., my\_project::utils::helper()) without an import.

\[2\] import is only required to introduce the root name of an external assembly/package.

## **Clause 5 — Declarations**

*(This clause is revised to reflect the new memory and capability models.)*

### **§5.1 Overview \[decl.overview\]**

\[1\] Declarations bind identifiers to entities (values, types, callables, etc.). Cursive maintains a single unified namespace per scope.

### **§5.2 Variable Bindings and Initializers \[decl.variable\]**

\[1\] Variable bindings introduce identifiers with explicit mutability (let, var) and cleanup responsibility (\=).

\[2\] Responsible Binding (=):

The \= operator creates a responsible binding. The binding owns the object and is responsible for its cleanup (RAII).

// 'owner' is responsible for cleaning up the Buffer.

let owner \= Buffer::new()

\[3\] Non-Responsible Binding (\<-):

The \<- operator is forbidden in safe Cursive code. Its use-after-free and aliasing risks are superseded by the Permission and Partitioning systems (Clause 11). It is re-enabled only in unsafe blocks (§11.8).

\[4\] move Keyword:

The move keyword is not a binding operator. It is an expression operator used to transfer ownership from a responsible let binding to a new context (see §11.2).

\[5\] Mutability:

Mutability is a property of a type's permission, not the binding. To create a mutable binding, use a permission qualifier.

// 'data' is a non-rebindable binding to a mutable, exclusive Buffer.

let data: unique \= Buffer::new()

data.mutate() // OK

// 'var' is used for re-binding.

var count \= 10

count \= 20 // OK

### **§5.3 Procedure Declarations \[decl.function\]**

\[1\] Procedure declarations define callable entities.

\[2\] **Full Syntax:**

procedure\_declaration ::=

    visibility? "comptime"? "procedure" identifier ("\<" generic\_params? "\>")?

    "(" param\_list? ")" (":" type)?

    sequent\_clause?

    callable\_body

sequent\_clause ::= "\[\[" must\_clause? ("=\>" will\_clause?)? "\]\]"

callable\_body ::= block\_stmt | ("=" expression ";") | ";"

\[3\] **Contract:** The \[\[ ... \]\] block is the procedure's contract (see §12.5).

* must\_clause: Preconditions that the *caller* must guarantee.  
* will\_clause: Postconditions that the *callee* guarantees.  
* **Abolished Syntax:** The grants: list and |- (turnstile) operator are **abolished**. Capabilities are handled by passing objects.

\[4\] Parameter Responsibility (move):

Parameters are non-responsible by default. To accept ownership, a parameter must be marked with move.

// 'data' is a non-responsible reference.

procedure inspect(data: Buffer) {

    // ...

}

// 'move' signifies this procedure takes ownership.

procedure consume(move data: Buffer) {

    // ...

} // 'data' is dropped here.

See §11.2 for move semantics at the call site.

### **§5.4 Type Declarations \[decl.type\]**

\[1\] Type declarations introduce nominal types: record, enum, and modal.

\[2\] Trait Implementation:

Types implement traits using the \<: operator (see §10.2).

// 'record Point is-a Drawable'

record Point \<: Drawable {

    x: f64,

    y: f64,

    // ... trait procedure implementations ...

}

### **§5.8 Program Entry and Execution Model \[decl.entry\]**

\[1\] The entry point for an executable **must** have the following signature, which receives all system capabilities via the Context object (see §12.1).

public procedure main(ctx: Context): i32 {

    // ... application logic ...

    result 0

}

\[2\] Any other signature (e.g., no parameters, incorrect return type) is a compile-time error (E12-001).

### **§5.9 Grant Declarations \[decl.grant\]**

\[1\] This section is **abolished**. The grant keyword is removed. User-defined capabilities are now regular records (see §12.5).

## **Clause 6 — Names, Scopes, and Resolution**

### **§6.1 Identifiers \[name.identifier\]**

\[1\] Identifiers name entities and follow Unicode XID standards, as specified in §2.3. Identifiers are case-sensitive.

### **§6.2 Scope Formation \[name.scope\]**

\[1\] Cursive uses lexical scoping. Scopes are created by:

* Modules (outermost)  
* Procedure bodies  
* Block expressions ({ ... })  
* loop, if, and match expressions.

\[2\] **Example (Scope hierarchy):**

let x \= 10 // Module scope

procedure test(p: i32) { // Procedure scope (p, x visible)

    let y \= 20 // Local scope (p, x, y visible)

    {

        let z \= 30 // Inner block scope (p, x, y, z visible)

    }

    // z is out of scope here

}

// p, y are out of scope here

### **§6.3 Name Introduction and Shadowing \[name.shadow\]**

\[1\] Redeclaring an identifier in the same scope is an error.

\[2\] A declaration in a nested scope shadows an identifier from an outer scope. This requires the shadow keyword.

let x \= 10

{

    // 'shadow' is required to redeclare 'x'

    shadow let x \= "hello"

    // 'x' here refers to the string

    // let y \= x // This 'x' is the string

}

// 'x' here refers to the integer 10

\[3\] Failing to use shadow when shadowing is intended is a compile-time error.

### **§6.4 Name Lookup \[name.lookup\]**

\[1\] Unqualified Lookup: my\_var

Search proceeds from the innermost scope outward:

1. Current block scope.  
2. Enclosing block scopes.  
3. Procedure scope (parameters).  
4. Module scope (file-level declarations and use imports).  
5. Universe scope (built-in types like i32, bool).  
   If not found, it is an error (E06-401).

\[2\] Qualified Lookup: my\_module::my\_item

The path to the left of :: must resolve to a module or type. The item on the right must be a visible member of that module/type.

## **Clause 7 — Type System**

### **§7.1 Type System Overview \[type.overview\]**

\[1\] Cursive has a static, nominal type system. Bidirectional inference is used to reduce the need for annotations, but all procedure boundaries must have explicit types.

### **§7.2 Primitive Types \[type.primitive\]**

\[1\] Integer Types: i8, i16, i32, i64, i128, isize, u8, u16, u32, u64, u128, usize.

\[2\] Floating-Point Types: f32, f64. Conform to IEEE 754\.

\[3\] Boolean Type: bool. Values are true and false.

\[4\] Character Type: char. Represents a Unicode Scalar Value.

\[5\] Unit Type: (). Represents an empty value. Procedures with no result implicitly return ().

\[6\] Never Type: \!. Represents a computation that never returns (e.t., panic() or sys.exit()).

### **§7.3 Composite Types \[type.composite\]**

\[1\] Tuples: Anonymous, structural products.

let point: (f64, f64) \= (1.0, 2.0)

\[2\] Records: Nominal products with named fields.

record Point { x: f64, y: f64 }

\[3\] Arrays: Fixed-size collections.

let buffer: \[u8; 1024\] \= \[0; 1024\]

\[4\] Slices: A view over a contiguous sequence.

let view: \[u8\] \= buffer\[0..10\]

\[5\] Strings: Built-in modal types for UTF-8 text.

\* string@Managed: Owned, heap-allocated, mutable string.

\* string@View: An immutable, non-owning view (slice) of a string.

\[6\] Enums: Nominal discriminated sums (tagged unions).

cursive enum Message { Quit, Text(string@View), Move { x: i32, y: i32 }, }

\[7\] Union Types (\\/): Anonymous, structural sum types.

let result: i32 \\/ ParseError \= parse("123")

### **§7.4 Function Types \[type.function\]**

\[1\] Function types define the signature of a callable.

\[2\] Syntax: (ParamType1, ParamType2) \-\> ReturnType

\[3\] Note: The grants (\! G) and sequent (\[\[...\]\]) components are not part of the function type itself. Capabilities are part of the signature via parameters (O-Cap), and sequents are metadata about the procedure, not part of its type.

### **§7.5 Modal Pointers (Liveness & Nullness) \[type.pointer\]**

\[1\] Cursive's primary pointer type, Ptr\<T\>@State, is a modal type that statically tracks pointer liveness and nullness.

\[2\] **Built-in Modal Definition:**

modal Ptr\<T\> {

    // The pointer is valid, non-null, and points to live data.

    // This is the ONLY state that can be dereferenced (\*ptr).

    @Valid { ... }

    // The pointer is null.

    // This is the safe, explicit replacement for \`nullptr\`.

    @Null { ... }

    // The pointer \*was\* valid, but its data (from a Region) was freed.

    // Access is a compile-time error. Solves use-after-region-free.

    @Expired { ... }

}

\[3\] Creation: The address-of operator & creates a Ptr\<T\>@Valid.

let value \= 42

let ptr: Ptr\<i32\>@Valid \= \&value

\[4\] Dereference: The \* operator is a compile-time error unless the pointer is in the @Valid state.

let x \= \*ptr // OK

let null\_ptr: Ptr\<i32\>@Null \= Ptr::null()

// let y \= \*null\_ptr // COMPILE-TIME ERROR (E07-301): Cannot dereference @Null

\[5\] **Raw Pointers:** \*const T and \*mut T are provided for unsafe code and FFI. They have no safety guarantees.

### **§7.6 Modal Types \[type.modal\]**

\[1\] modal declarations define compile-time state machines. Ptr\<T\>@State is a built-in modal type.

\[2\] See §12.3 for an example (FileHandle).

## **Clause 8 — Expressions**

### **§8.1 Expression Fundamentals \[expr.fundamental\]**

\[1\] Expressions produce values. Evaluation order is strictly left-to-right, with the exception of logical short-circuiting (&&, ||).

### **§8.2 Primary and Postfix Expressions \[expr.primary\]**

\[1\] Primary: Literals, identifiers, (expr).

\[2\] Postfix:

* **Field Access:** expr.field  
* **Procedure Call:** expr(arg1, arg2)  
* **Method Call:** expr.method(arg1) (Syntactic sugar for Trait::method(expr, arg1))  
* **Indexing:** expr\[index\]  
* **Pipeline:** expr \=\> procedure (Sugar for procedure(expr))

### **§8.3 Unary and Binary Operators \[expr.operator\]**

\[1\] Unary: \! (logical not), \- (negation), & (address-of), \* (dereference).

\[2\] Binary (by precedence):

* \*\* (power)  
* \*, /, % (multiplicative)  
* \+, \- (additive)  
* \<\<, \>\> (shift)  
* & (bitwise AND)  
* ^ (bitwise XOR)  
* | (bitwise OR)  
* \==, \!=, \<, \<=, \>, \>= (comparison)  
* && (logical AND, short-circuits)  
* || (logical OR, short-circuits)  
* \= (assignment), \+=, \-=, etc. (compound assignment)  
* move (move expression, §11.2)

### **§8.4 Structured Expressions \[expr.structured\]**

\[1\] **if expressions:**

let value \= if condition {

    result 10

} else {

    result 20

}

\[2\] **match expressions:**

let text \= match value {

    Message::Quit \=\> "quit",

    Message::Text(s) \=\> s,

    Message::Move { x, y } \=\> "move",

}

\[3\] **loop expressions:**

* **Infinite loop:** loop { ... }  
* **Iterator loop:** loop item: Type in iterator { ... }  
* **Conditional loop:** loop condition { ... } (Note: while is not a keyword)

\[4\] region blocks: See §11.6.

\[5\] parallel blocks: See §13.2.

\[6\] unsafe blocks: See §11.8.

## **Clause 9 — Statements**

### **§9.1 Statement Fundamentals \[stmt.fundamental\]**

\[1\] Statements execute for side effects and do not produce a value. They are terminated by newlines or semicolons.

### **§9.2 Simple Statements \[stmt.simple\]**

* **Variable Declaration:** let x \= 10  
* **Assignment:** x \= 20  
* **Expression Statement:** procedure\_call()  
* **defer Statement:** defer { resource.close() } (Executes at scope exit, in LIFO order).

### **§9.3 Control Flow Statements \[stmt.control\]**

* **return:** Early exit from a procedure.  
* **break:** Exit from a loop.  
* **continue:** Skip to the next iteration of a loop.  
* **Labeled Statements:** break 'my\_label and continue 'my\_label are supported.

## **Clause 10\. Traits and Polymorphism**

*(This clause is a full replacement, superseding old Clauses 10 and 12 from Draft 01.0.)*

### **§10.1 Trait Declarations**

\[1\] A trait defines an interface. It may contain:

1\. Abstract Procedures: A signature without a body (a requirement).

2\. Concrete Procedures: A signature with a body (a default implementation).

\[2\] **Syntax:**

public trait Drawable {

    // 1\. Abstract procedure (implementers MUST provide this)

    procedure draw(self: const, ctx: Context);

    // 2\. Concrete procedure (implementers INHERIT this)

    procedure get\_id(self: const): u32 {

        result 0 // Default implementation

    }

}

### **§10.2 Trait Implementation**

\[1\] A type implements a trait using the \<: ("implements" or "is-a") operator.

\[2\] **Rules:**

* Implementers **must** provide all abstract procedures.  
* Implementers **may** replace concrete procedures using the override keyword.

\[3\] **Syntax:**

// 'record Point is-a Drawable'

record Point \<: Drawable {

    x: f64, y: f64,

    // 1\. Fulfilling the abstract requirement (no 'override')

    procedure draw(self: const, ctx: Context) {

        // ... implementation ...

    }

    // 2\. Replacing the concrete default (MUST use 'override')

    override procedure get\_id(self: const): u32 {

        result 123

    }

}

record Circle \<: Drawable {

    center: Point, radius: f64,

    // 1\. Fulfilling the abstract requirement

    procedure draw(self: const, ctx: Context) {

        // ... implementation ...

    }

    // 2\. Inheriting the default 'get\_id' (no override provided)

}

\[4\] **Diagnostics:**

* E10-001: Using override on an abstract procedure (e.g., draw).  
* E10-002: Failing to use override on a concrete procedure (e.g., get\_id).  
* E10-003: Failing to implement an abstract procedure.

### **§10.3 Path 1: Static Polymorphism (Generics)**

\[1\] This is the **zero-cost, default** path for polymorphism on *inputs*. It uses generic parameters constrained by traits.

\[2\] **Syntax:**

// This is STATIC and MONOMORPHIZED.

// \<T \<: Drawable\> is a compile-time constraint.

procedure draw\_static\<T \<: Drawable\>(item: T, ctx: Context) {

    item.draw(ctx) // This is a direct, inlined call.

}

// Usage:

let p \= Point{ x: 10.0, y: 10.0 }

draw\_static(p, ctx) // Compiles to a specialized draw\_static\<Point\>

### **§10.4 Path 2: Dynamic Polymorphism (Trait Objects)**

\[1\] This is the **opt-in, runtime-cost** path for *inputs and storage*. It uses the trait name directly as a type, which creates a "fat pointer" (data pointer \+ vtable).

\[2\] **Syntax:**

// This is DYNAMIC and uses a VTABLE.

// 'item: Drawable' means 'item' is a fat pointer.

procedure draw\_dynamic(item: Drawable, ctx: Context) {

    item.draw(ctx) // This is a vtable lookup.

}

// Usage (Solves the heterogeneous list problem):

let p \= Point{ x: 10.0, y: 10.0 }

let c \= Circle{ center: p, radius: 5.0 }

// Coercion from concrete type to trait object:

let drawable\_p: Drawable \= p

let drawable\_c: Drawable \= c

// Store different concrete types in one list:

let items: \[Drawable\] \= \[drawable\_p, drawable\_c\]

loop item: Drawable in items {

    draw\_dynamic(item, ctx) // Dispatches to correct implementation

}

### **§10.5 Path 3: Opaque Polymorphism (Opaque Types)**

\[1\] This is the **zero-cost, encapsulation** path for *outputs*. It uses the \<: operator on a *return type* to hide implementation details.

\[2\] **Syntax:**

// This is STATIC, zero-cost, and OPAQUE.

// The caller knows \*only\* that they get \*some\* type

// that implements Drawable. They don't know the concrete type.

// \--- Library's Public API \---

// Returns an opaque 'Drawable'

public procedure make\_widget(): internal::Widget \<: Drawable;

// \--- Library's Internal Implementation \---

internal record Widget \<: Drawable {

    id: u32,

    procedure draw(self: const, ctx: Context) { ... }

}

public procedure make\_widget(): internal::Widget \<: Drawable {

    result internal::Widget{ id: 42 }

}

// \--- User's Code \---

let widget \= make\_widget()

// 'widget' \*is\* a concrete 'internal::Widget' in memory,

// but the type system only allows Drawable methods.

widget.draw(ctx) // OK

// widget.id // COMPILE-TIME ERROR: 'id' is not a member of trait 'Drawable'

## **Clause 11\. The Cursive Memory Model**

*(This clause is a full replacement for Draft 1.0 Clause 11, based on "The Cursive Memory Model".)*

### **§11.1 Principles: Liveness vs. Aliasing**

\[1\] Cursive achieves memory safety without a borrow checker, using three composable mechanisms:

1. **RAII \+ Static Invalidation** (Ownership)  
2. **Permissions \+ Partitioning** (Aliasing)  
3. **Modal Pointers** (Liveness, from §7.5)

\[2\] **Liveness:** Ensures a pointer is valid, non-null, and points to live memory. Solved by **RAII** and **Modal Pointers** (Ptr\<T\>@State).

\[3\] **Aliasing:** Ensures no two pointers can cause a data race. Solved by **Permissions** and the **Partitioning System**.

### **§11.2 RAII and move (Ownership)**

\[1\] **RAII:** Bindings created with \= are *responsible* for their object. Cleanup is deterministic at scope exit.

\[2\] **move Keyword:** move is the *only* way to transfer ownership (cleanup responsibility) in safe code.

\[3\] Static Invalidation: The move keyword performs two actions at compile-time:

1\. Transfers ownership (e.g., to a procedure parameter).

2\. Statically invalidates the source binding.

\[4\] **Example:**

procedure consume(move data: Buffer) {

    // 'data' is now responsible

} // 'data' is dropped, Buffer is freed

let owner \= Buffer::new()

consume(move owner) // 1\. Ownership transferred

                    // 2\. 'owner' is now statically invalid

// owner.read() // COMPILE-TIME ERROR (E11-001):

             // Use of moved value 'owner'.

\[5\] This rule statically prevents all use-after-free and double-free errors with zero runtime cost.

### **§11.3 Permissions (Aliasing Gateway)**

\[1\] Permissions are type qualifiers that control aliasing rules.

\[2\] **const (Default):** Read-only, aliased. Allows any number of Ptr\<const T\> pointers. Does not participate in partitioning.

\[3\] **unique (Static Exclusion):** Read-write, no aliasing. The compiler *statically forbids* creating any mutable pointer to a unique binding. This is the zero-cost path for exclusive mutation.

let data: unique \= Buffer::new()

data.mutate() // OK

// let ptr: Ptr\<\>@Valid \= \&data // COMPILE-TIME ERROR (E11-101):

                // Cannot create a reference to a 'unique' binding.

\[4\] **partitioned (Aliased Mutability):** Read-write, aliased. This is the explicit **opt-in** for aliased mutability. All partitioned bindings are protected by the **Partitioning System**.

§11.4 The Partitioning System (Aliasing Safety) \[DRAFTING NOTE: This section supersedes the implicit proof system from §11.4.2 of the previous draft. The implicit "Static Partitioning Proof" is ABOLISHED and replaced by the explicit `partition` contract block. This resolves the ergonomic failure identified in the design review.\]

\[1\] The Partitioning System is Cursive's static, compile-time alternative to a borrow checker. It provides data-race freedom for partitioned data by enforcing alias exclusion rules at compile time. All checks are static and incur zero runtime cost.

\[2\] Partition Definitions (Record Fields): Fields in a record can be grouped into explicit partitions using the `partition name { ... }` syntax. Fields *not* in an explicit partition are each considered to be in their own, implicit, anonymous partition. This mechanism remains unchanged.

record GameWorld {

    // 'players' is in its own implicit partition

    players: \[Player\],

    

    // 'enemies' and 'projectiles' are grouped in the 'physics' partition

    partition physics {

        enemies: \[Enemy\],

        projectiles: \[Projectile\],

    }

}

\[3\] Rule 1: Static Record Partitioning. Accessing fields in different partitions is always permitted. Accessing multiple fields in the *same* explicit partition via separate pointers is a **Compile-Time Error (E11-310)**. This rule remains unchanged.

let world: partitioned \= GameWorld::new()

let e: Ptr\<\[Enemy\]\>@Valid \= \&world.enemies; // OK, acquires 'physics' partition lock

let p: Ptr\<\[Player\]\>@Valid \= \&world.players; // OK, acquires 'players' partition lock

// COMPILE-TIME ERROR (E11-310): Partition 'physics' is already locked

// let proj \= \&world.projectiles; 

\[4\] Rule 2: Static Collection Partitioning (The `partition` Block). The implicit Static Partitioning Proof (§11.4.2 of the prior draft) is **abolished**. It was implicit, non-local, and unusable.

It is replaced by the `partition` contract block: an explicit, zero-cost, compile-time assertion that makes the proof of non-overlap a syntactic requirement. This aligns with **Goal 7 (Local Reasoning)** and **Goal 8 (LLM-Friendly Syntax)**.

§11.4.1 The `partition` Contract Block

\[1\] **Syntax:** The `partition` block is a new statement form.

partition\_statement ::=

    "partition" \<collection\> "by" "(" \<index\_list\> ")"

    "where" "(" \<proof\_expression\> ")"

    \<block\_stmt\>

\<collection\> ::= \<expression\> // Must resolve to a 'partitioned' collection

\<index\_list\> ::= \<identifier\> ("," \<identifier\>)\*

\<proof\_expression\> ::= \<expression\> // Must be a compile-time boolean

\[2\] **Semantics:**

1. **Zero-Cost:** The `partition` block is a **compile-time-only contract**. It generates **zero runtime code,** satisfying **Goal 9 (Zero-Cost Abstractions)**.  
2. **Explicit Proof:** The `where` clause is the programmer's explicit proof of non-overlap. The compiler's *only* job is to prove that this expression is `true` at compile time, using its standard `const` evaluation rules.  
3. **Compiler Guarantee:** *Inside* the `block_stmt`, the compiler *assumes* the proof is valid. All Static Partitioning errors (E11-311, "Potential alias conflict") for the collection specified in `<collection>` *when accessed by an index in the `by` list* are **disabled**.  
4. **Error Condition:** If the `where` clause is *not* provably `true` at compile time, the `partition` block itself fails compilation with a new error:  
   * **E11-312: "Partition contract proof failed."**

#### **2.3.4. \[FORMALIZED\] Specification: The Partition Proof Verifier (v2.1)**

This section provides the complete normative specification for the Partition Proof Verifier, as referenced in the language design (§11.4.1). This text is now considered normative for implementation.

##### **V.1 Overview and Guiding Principles**

This document defines the normative requirements for the **Partition Proof Verifier**, the compile-time mechanism responsible for validating the `where` clause of a `partition` block.

The verifier's primary goal is **predictability, not power**. It must be a simple, rules-based, flow-sensitive verifier, not a general-purpose SMT solver. This ensures that a human developer or an LLM can easily determine why a proof has failed and how to correct it by manipulating the local control flow.

* **Zero-Cost:** The verifier is a compile-time-only mechanism. It generates **zero** runtime code, satisfying Goal 9 (Zero-Cost Abstractions).  
* **Flow-Sensitive:** The verifier gathers **Known Facts** by analyzing the control-flow graph (CFG) leading up to the `partition` block.  
* **Syntactically-Restricted:** The verifier will only evaluate expressions that conform to the **Verifiable Expression Grammar** defined below (V.2.1).  
* **No "Black Boxes":** Implementations are **forbidden** from using general-purpose SMT solvers or other "black box" theorem provers, as their failure modes are unpredictable.

##### **V.2 Verifier Capabilities**

The verifier operates in two stages:

1. **Fact Generation:** Analyze the CFG of the enclosing procedure to build a set of **Known Facts** valid at the `partition` block.  
2. **Proof Evaluation:** Evaluate the `where` clause expression against the set of Known Facts.

If the `where` clause expression cannot be proven to be **true**, the compiler **shall** issue a compile-time error:

* **E11-312:** "Partition contract proof failed. The verifier could not statically prove the `where` clause is true given the current context."

###### **V.2.1 The Verifiable Expression Grammar**

The `proof_expression` in a `partition ... where (proof_expression)` clause, as well as any conditional expressions used for Fact Generation (see V.2.2), **must** conform to the following restricted grammar.

An expression is **verifiable** if it contains only:

* **Allowed Entities:**  
  * Procedure parameters (e.g., `i`, `j`).  
  * `comptime` constants.  
  * Local `let` bindings (non-`shadow`) whose values are derived *only* from other verifiable entities.  
* **Allowed Types:**  
  * All primitive integer types (`i*`, `u*`, `isize`, `usize`).  
  * `bool`.  
* **Allowed Operators:**  
  * Integer Arithmetic: `+`, `-`, `*`.  
  * Comparison: `==`, `!=`, `<`, `<=`, `>`, `>=`.  
  * Logical: `&&`, `||`, `!`.  
* **Forbidden Operations:**  
  * **All Procedure Calls:** This is critical. The verifier **shall not** call any procedure, including other `comptime` procedures, to avoid unpredictable side effects or unbounded complexity.  
  * **Mutation:** No assignment (`=`), `var` bindings, or mutating operators.  
  * **Complex Arithmetic:** `/` and `%` are forbidden.  
  * **Pointers & References:** No `&`, `*`, `Ptr`, or `region` operations.  
  * **Allocation:** No `^` or heap operations.

###### **V.2.2 The Control-Flow Fact Calculus**

The verifier **shall** generate a set of **Known Facts** by analyzing the control flow leading to the `partition` block.

* **Rule V.2.2.1: `if` / `else` Facts**  
  * Given: `if (V_COND) { BLOCK_A } else { BLOCK_B }`  
  * Facts for `BLOCK_A`: `{ V_COND }`  
  * Facts for `BLOCK_B`: `{ !(V_COND) }`  
* **Rule V.2.2.2: `if` with Early Exit Fact**  
  * Given: `if (V_COND) { ...; return; } ... CODE_AFTER`  
  * Facts for `CODE_AFTER`: `{ !(V_COND) }`  
  * This is the fundamental rule that enables the "swap pattern."  
* **Rule V.2.2.3: `match` Facts**  
  * Given: `match (V_EXPR) { case V_PATTERN => { BLOCK_A } }`  
  * Facts for `BLOCK_A`: `{ V_EXPR == V_PATTERN }`  
  * (This applies only to verifiable patterns, e.g., integer literals).  
* **Rule V.2.2.4: `loop` Facts**  
  * Given: `loop (V_COND) { BLOCK_A } ... CODE_AFTER`  
  * Facts for `BLOCK_A`: `{ V_COND }`  
  * Facts for `CODE_AFTER`: `{ !(V_COND) }`  
* **Rule V.2.2.5: `let` Binding Facts**  
  * Given: `let V_IDENT = V_EXPR`  
  * Facts for subsequent code: `{ V_IDENT == V_EXPR }`  
* **Rule V.2.2.6: Iterator Range Fact**  
  * Given: `loop i: T in START..END { BLOCK_A }`  
  * Facts for `BLOCK_A`: `{ i >= START && i < END }`  
  * (This applies to verifiable range expressions, e.g., literals, consts, or parameters).  
* **Rule V.2.2.7: Nested Iterator Fact**  
  * Given: `loop i in 0..N { loop j in (i+1)..N { BLOCK_A } }`  
  * Facts for `BLOCK_A`: `{ i >= 0 && i < N && j > i && j < N }`  
  * (This rule specifically provides the fact `j > i`, which implies `i != j`, solving the nested loop verbosity issue).  
* **Rule V.2.2.8: Strided Iterator Fact**  
  * Given: `loop i in iterator.strided(STEP) { BLOCK_A }`  
  * Facts for `BLOCK_A`: `{ i % STEP == 0 }`  
  * (This is a conceptual rule; a concrete implementation depends on the standard library's iterator definition but follows this principle).

###### **V.2.3 Proof Evaluation**

The verifier proves the `where` clause by:

1. Substituting all known facts from the CFG.  
2. Applying constant folding and basic integer arithmetic identities.  
* **Identity V.2.3.1: Arithmetic Identity**  
  * The verifier **shall** be able to prove simple, non-overflowing arithmetic identities involving `+`, `-`, and `*` with `comptime` constants.  
  * `i != i + 1` (must prove `true`)  
  * `i != i - 1` (must prove `true`)  
  * Given `const S > 0`, `i != i + S` (must prove `true`)

##### **V.3 Normative Conformance Suite**

An implementation's Partition Proof Verifier is **conforming** if and only if it produces the "Required Outcome" for all test cases in this suite.

| Test Case            | Code Snippet                                    | `where` Clause             | Required Outcome | Rationale                                                         |
| -------------------- | ----------------------------------------------- | -------------------------- | ---------------- | ----------------------------------------------------------------- |
| Pass: Constant       | ...                                             | `where (1 != 2)`           | PASS             | Constant Folding.                                                 |
| Pass: Simple Fact    | `if i == j { return; }`                         | `where (i != j)`           | PASS             | "Swap Pattern". Rule V.2.2.2.                                     |
| Pass: Arithmetic     | ...                                             | `where (i != i + 1)`       | PASS             | "Window Pattern". Identity V.2.3.1.                               |
| Pass: `let` Fact     | `let k = i + 1;`                                | `where (k != i)`           | PASS             | Rules V.2.2.5 \+ V.2.3.1.                                         |
| Pass: `else` Fact    | `if i > j { ... } else { ... }`                 | `where (i <= j)`           | PASS             | Rule V.2\`\`\`\`\`\`\`\`\`\`\`\`\`\`\`.2.1 (Inside `else` block). |
| Pass: Loop Range     | `loop i in 0..10 { ... }`                       | `where (i >= 0 && i < 10)` | PASS             | Rule V.2.2.6. Solves verbosity.                                   |
| Pass: Nested Loop    | `loop i in 0..N { loop j in (i+1)..N { ... } }` | `where (i != j)`           | PASS             | Rule V.2.2.7. Solves verbosity.                                   |
| Pass: Strided Access | `const STRIDE=4; ...`                           | `where (i != i + STRIDE)`  | PASS             | Identity V.2.3.1.                                                 |
| Fail: No Fact        | ...                                             | `where (i != j)`           | FAIL (E11-312)   | `i != j` is not universally true.                                 |
| Fail: Complex Arith  | ...                                             | `where (i / 2 > i)`        | FAIL (E11-312)   | Verifier does not support `/`.                                    |
| Fail: Proc Call      | `comptime procedure p() { result true; }`       | `where (p())`              | FAIL (E11-312)   | Verifier does not support procedure calls.                        |

§11.6 Regions and `^` (Liveness)

\[1\] A **region block** is a fundamental language feature that creates a temporary, stack-like arena for O(1) bulk allocation. It is a primitive memory management tool, distinct from the O-Cap system's heap allocator.

\[2\] The `^` operator is a primitive allocation operator. It allocates an object within the innermost `region` block.

\[3\] **Example:**

region temp {

    let x \= ^Buffer::new() // Allocated in 'temp' arena

    let y \= ^Buffer::new() // Allocated in 'temp' arena

    let ptr: Ptr\<Buffer\>@Valid \= \&x

    // ... use ptr

} // 'temp' block exits. x, y, and the data 'ptr' points to

  // are ALL freed in O(1).

\[4\] The compiler uses the `Ptr<T>@State` system (§7.5) to statically prevent pointers to region-allocated data from escaping their region. An escaped pointer would transition to `Ptr<T>@Expired`, which cannot be dereferenced.

§11.7 The `Drop` Trait and Static Invalidation

\[1\] The `Drop` trait provides custom, compiler-invoked cleanup logic for a type when its lifetime ends.

\[2\] The trait is defined (per Annex B) as:

public trait Drop {

    // 'drop' is invoked by the compiler, not user-callable.

    // It is called on a responsible binding at the end of its

    // scope. The compiler provides temporary exclusive ('unique')

    // access for the duration of the call.

    procedure drop(self);

}

\[3\] The `Drop::drop(self)` procedure is **not** user-callable. It is called *only* by the compiler. Attempting to call `my_obj.drop()` directly in source code shall result in a compile-time error (E11-003).

\[4\] The compiler shall call `Drop::drop` on a binding `b` if and only if both of the following conditions are met: a) `b` is a **responsible binding** (created with the `=` operator, §5.2) or had responsibility transferred to it. b) `b` is at the end of its lexical scope (e.g., end of block, `return`, or procedure end).

\[5\] **Static Invalidation Rule:** When `Drop::drop` is called on a responsible binding `b` (the "owner"), the compiler performs two actions simultaneously: 1\. **Execution:** The `drop` procedure is executed. The binding `b` is granted temporary, exclusive (`unique`) access for the duration of the call, ensuring safe mutation during cleanup. 2\. **Static Invalidation:** All `partitioned` aliases and `Ptr` references derived from `b` are **statically invalidated**. This is the same compile-time mechanism used by the `move` keyword (§11.2).

\[6\] Any subsequent use of these invalidated aliases shall result in a compile-time error (E11-001): "Use of invalidated value."

\[7\] **Rationale:** This mechanism replaces the "proof of non-aliasing" rule (formerly E11-002). Instead of requiring a complex, non-local proof that *fails* compilation, this rule *triggers* invalidation, which always succeeds. It makes `Drop` a well-defined lexical event, aligning with **Goal 7 (Local Reasoning)** and **Goal 5 (Memory Safety Without Complexity)**. The rule is simple and local: aliases are valid only as long as their responsible owner is in scope.

\[8\] This creates a clear parallel:

* `move b`: Transfers ownership *and* invalidates `b` \+ all aliases.  
* `drop(b)` (at scope-end): Ends ownership (calls `Drop`) *and* invalidates all aliases.

\[9\] **Example 11.7.1: Drop and Static Invalidation**

procedure partitioned\_drop\_semantics() {

    // 'world' is responsible and partitioned

    let world: partitioned \= GameWorld::new()

    // 'players' is a partitioned alias of 'world'

    let players: Ptr\<\[Player\]\>@Valid \= \&world.players;

    if some\_condition {

        // OK: 'players' is valid while 'world' is in scope

        process(players);

    }

} // 'world' goes out of scope here.

  // 1\. Compiler calls 'world.drop()'.

  // 2\. Compiler \*statically invalidates\* 'players'.

  // 3\. No error occurs.

procedure use\_after\_drop() {

    let world: partitioned \= GameWorld::new()

    let players: Ptr\<\[Player\]\>@Valid \= \&world.players;

    // ... (code that causes 'world' to drop) ...

    // COMPILE-TIME ERROR (E11-001):

    // "Use of invalidated value 'players'."

    process(players);

}

### **§11.8 unsafe Blocks**

\[1\] The unsafe keyword disables Cursive's static guarantees. Inside an unsafe block:

2\. All Static (E11-310) and Dynamic (E11-311) partition checks are disabled.

3\. Raw pointer (\*T) dereferencing is permitted.

\[2\] The programmer is now 100% responsible for both Liveness and Aliasing.

**§12.1 Overview and Principles**

**§12.1.1 Scope**

\[1\] This clause specifies the Cursive capability system, which governs all procedures that produce observable **external effects** (e.g., I/O, networking, threading, heap allocation). \[2\] Cursive is an **Object-Capability (O-Cap)** language. It does not have global functions for effects. All capabilities are represented by **first-class objects** that are passed as parameters. \[3\] A procedure can only perform an effect if it has been passed a capability object that provides a method for that effect. This system ensures all capabilities are explicit, lexically scoped, and locally verifiable, adhering to the design principles of Local Reasoning (G7) and LLM-Friendliness (G8).

**§12.1.2 Abolished Mechanisms**

\[1\] The `[[grants]]` attribute system (from Cursive-Draft01.md, §12) is **abolished** and removed from the language. \[2\] The `grant` declaration keyword (from Cursive-Draft01.md, §5.9) is **abolished**. \[3\] The `grant_clause` and turnstile (`|-`) are **removed** from procedure sequent syntax (see §12.6).

**§12.2 The Root of Capability: `main` and `Context`**

\[1\] All system-level capabilities originate from the Cursive runtime and are passed to the program's entry point. \[2\] The `main` procedure (defined in §5.8) **shall** have a signature that accepts exactly one parameter of type `Context`.

// Required signature for the program entry point

public procedure main(ctx: Context): i32 {

    // ...

    result 0

}

\[3\] Any `main` procedure not matching this signature shall produce a compile-time error **(E12-001)**.

**§12.2.1 The `Context` Record**

\[1\] The `Context` is a built-in `record` type. It is the "root of power" for the application, bundling the root implementations of all available system-level capabilities. \[2\] The Cursive runtime shall construct and pass this object to `main`. The `Context` record has the following structure:

// Built-in record definition provided by the runtime

public record Context {

    // Filesystem capabilities (§12.3.1)

    fs: RootFileSystem,

    // Network capabilities (§12.3.2)

    net: RootNetwork,

    // System capabilities (§12.3.3)

    sys: System,

    // Dynamic memory capabilities (§12.3.4)

    heap: RootHeapAllocator,

    // \[Editor's Note: \`arena: RootArena\` has been removed.

    // Regions (§11.6) are a primitive language feature

    // and are not part of the O-Cap system.\]

}

**§12.3 System Capability Traits and Attenuation**

\[1\] System capabilities are defined as **trait**s (per Clause 10\) to define their interfaces. This allows for **attenuation** (restricting power) by enabling different concrete types (e.g., `RootFileSystem` and `RestrictedFileSystem`) to implement the same interface. \[2\] Procedures that require a capability accept a parameter of the **trait type** (e.g., `fs: FileSystem`), allowing them to accept *any* object that implements the interface. This uses "Path 2: Dynamic Polymorphism" (§10.4).

**§12.3.1 FileSystem Capability**

\[1\] The `FileSystem` trait defines the interface for filesystem operations.

// 1\. The Capability Trait (Interface)

public trait FileSystem {

    // \--- Operations \---

    procedure read\_to\_string(self: const, path: string@View): string

        \[\[ ... \]\]; // Sequent omitted

    procedure open(self: const, path: string@View): FileHandle@Open

        \[\[ ... \]\];

    // \--- Attenuation Method \---

    // Returns a \*new\* object that ALSO implements FileSystem,

    // but is restricted to a sub-path.

    // This uses "Path 3: Opaque Polymorphism" for its return type.

    procedure restrict(self: const, sub\_path: string@View): RestrictedFileSystem \<: FileSystem

        \[\[ ... \]\]; 

}

// 2\. The Root Implementation (Held by Context)

internal record RootFileSystem \<: FileSystem {

    // Implements all FileSystem methods, operating on "/"

    

    override procedure read\_to\_string(self: const, path: string@View): string {

        // ... implementation for root filesystem ...

    }

    override procedure restrict(self: const, sub\_path: string@View): RestrictedFileSystem \<: FileSystem {

        let safe\_path \= self.resolve\_safe\_path(sub\_path)

        result RestrictedFileSystem { root: safe\_path }

    }

}

// 3\. The Restricted Implementation

internal record RestrictedFileSystem \<: FileSystem {

    root: string@Managed, // e.g., "/app/data"

    // Override all operations to be relative to 'self.root'

    // and to forbid ".." escapes.

    override procedure read\_to\_string(self: const, path: string@View): string {

        let safe\_path \= self.resolve\_within\_root(path)

        // ... perform read relative to self.root ...

    }

    

    // Can be further restricted

    override procedure restrict(self: const, sub\_path: string@View): RestrictedFileSystem \<: FileSystem {

        let new\_root \= self.resolve\_within\_root(sub\_path)

        result RestrictedFileSystem { root: new\_root }

    }

}

**§12.3.2 Network Capability**

\[1\] The `Network` trait defines the interface for networking. It follows the same pattern as `FileSystem`, with a `RootNetwork` object (in `Context`) and a `restrict_to_host(...)` attenuation method.

public trait Network {

    procedure connect(self: const, addr: string@View): Socket@Connected;

    procedure restrict\_to\_host(self: const, host: string@View): RestrictedNetwork \<: Network;

}

**§12.3.3 System Capability**

\[1\] The `System` record provides methods for system-level queries (env, time) and concurrency primitives (threading, mutexes).

public record System {

    // \--- Operations \---

    procedure get\_env(self: const, key: string@View): string;

    procedure time(self: const): Timestamp;

    procedure exit(self: const, code: i32): \!;

    // \--- Concurrency \---

    procedure spawn\<T\>(self: const, action: () \-\> T): Thread\<T\>@Spawned;

    procedure create\_mutex\<T\>(self: const, value: T): Mutex\<T\>@Unlocked;

}

\[2\] **Mutex Capability:** The `Mutex<T>` type is a modal capability for managing shared, mutable state. It is provided by `ctx.sys.create_mutex()`.

public modal Mutex\<T\> {

    @Unlocked { ... }

    @Locked { data: unique T }

    // 'lock' takes 'const' self, allowing many threads to \*try\* to lock

    procedure lock(self: const): Mutex\<T\>@Locked { ... }

    // 'unlock' takes 'unique' self, only the holder can unlock

    procedure unlock(self: unique): Mutex\<T\>@Unlocked { ... }

}

// MutexGuard provides RAII-style unlocking.

public record MutexGuard\<T\> {

    // This binding is responsible for the Locked state

    mutex: unique Mutex\<T\>@Locked

}

// The guard's drop implementation calls unlock, returning the

// mutex to the @Unlocked state.

public trait Drop for MutexGuard\<T\> {

    procedure drop(self) {

        // 'self' has temporary 'unique' access

        // This move transitions the mutex state and consumes the guard

        move self.mutex.unlock()

    }

}

**§12.3.4 Memory Allocators**

\[1\] The `alloc::` grant namespace is **abolished**. All dynamic allocation capabilities are provided by explicit allocator objects.

\[2\] **Heap Allocator:** Replaces `alloc::heap`. This is the only O-Cap-governed allocator. It originates from `ctx.heap`.

public trait HeapAllocator {

    procedure alloc\<T\>(self: const, layout: Layout): Ptr\<T\>@Valid;

    procedure free\<T\>(self: const, ptr: Ptr\<T\>@Valid);

    procedure with\_quota(self: const, bytes: usize): QuotaAllocator \<: HeapAllocator;

}

// Context contains 'heap: RootHeapAllocator'

internal record RootHeapAllocator \<: HeapAllocator { ... }

internal record QuotaAllocator \<: HeapAllocator { ... } // Attenuated version

\[3\] **Arena Allocator:** Replaces `alloc::region`. The `alloc::region` grant is **abolished**. The `region` keyword and `^` operator are now primitive language features (see §11.6) and do not require an O-Cap capability.

**§12.4 Capability Propagation**

\[1\] Capabilities are propagated **only** by passing capability objects as parameters. A procedure that does not receive a capability object cannot perform that capability's effects.

\[2\] There are two valid, non-viral patterns for this.

**§12.4.1 Path 1: Explicit Capability Passing**

\[1\] Low-level, reusable procedures shall ask only for the **specific** capabilities they need. The parameter type is the **trait**, allowing the caller to pass *any* implementation (root or restricted).

// This procedure is 100% local and testable.

// It can \*only\* read files.

procedure read\_config(fs: FileSystem): Config {

    // 'fs' is a trait object (Path 2 Polymorphism)

    let data \= fs.read\_to\_string("config.toml")

    result parse(data)

}

// The caller is responsible for passing the capability.

procedure load\_services(fs: FileSystem) {

    let config \= read\_config(fs); // 'fs' is passed down

    // ...

}

**§12.4.2 Path 2: Capability Bundle Passing (Optional)**

\[1\] High-level "manager" procedures may accept a "bundle" of capabilities to avoid long parameter lists and stabilize signatures against change. This is a **design pattern**, not a language feature.

// 1\. Define a custom bundle record

record AppContext {

    fs: FileSystem,

    net: Network,

    heap: HeapAllocator

}

// 2\. High-level procedures accept the bundle

procedure run\_app\_server(ctx: const AppContext) {

    // 3\. Delegate capabilities to callees

    let config \= read\_config(ctx.fs);

    let listener \= ctx.net.listen("0.0.0.0:8080");

    // ...

}

// 4\. 'read\_config' from §12.4.1 is called with 'ctx.fs'

\[2\] This pattern solves the "annotation nightmare" (virality). If `read_config` *also* needs a `HeapAllocator`, its signature changes to `read_config(fs: FileSystem, heap: HeapAllocator)`.

* In the "Explicit Passing" model, `load_services` must also be updated to pass the heap.  
* In the "Bundling" model, `run_app_server`'s signature **does not change**. Only its implementation changes to pass `ctx.heap`.

**§12.5 User-Defined Capabilities**

\[1\] A "user-defined capability" (formerly "user-defined grant") is a design pattern, not a language feature. It is a user-defined `record` or `trait` that encapsulates an application-level permission.

\[2\] **Example: User-Defined `BillingService`**

// 1\. DEFINE the capability as a trait

public trait BillingService {

    procedure charge(self: const, user\_id: u64, amount: u64): bool;

}

// 2\. IMPLEMENT the trait, injecting system capabilities

internal record StripeBillingService \<: BillingService {

    api\_key: string@Managed,

    net: Network, // Requires a Network capability to work

    override procedure charge(self: const, user\_id: u64, amount: u64): bool {

        // ... uses self.net to make an API call to Stripe ...

    }

}

// 3\. PASS the capability as a parameter

public procedure main(ctx: Context): i32 {

    let key \= ctx.sys.get\_env("STRIPE\_KEY");

    

    // Construct the capability, injecting the root network capability

    let billing\_svc \= StripeBillingService {

        api\_key: key,

        net: ctx.net

    };

    

    // Pass the \*individual\* capability to the app

    run\_app(ctx.fs, billing\_svc);

    result 0

}

procedure run\_app(fs: FileSystem, billing: BillingService) {

    // This procedure can read files and charge cards,

    // but has no other network access.

}

**§12.6 Sequent Simplification**

\[1\] With the removal of the `[[grants]]` system, the procedure sequent syntax (§5.3) is simplified. \[2\] The `grant_clause` and `|-` (turnstile) productions are **abolished** from the `sequent_clause` grammar. \[3\] The new, simplified syntax is:

sequent\_clause    ::= "\[\[" sequent\_spec "\]\]"

sequent\_spec    ::= must\_clause "=\>" will\_clause

                  | must\_clause

                  | "=\>" will\_clause

                  | ""

\[4\] An empty sequent `[[ ]]` is equivalent to `[[ true => true ]]` (a pure procedure with no logical constraints). \[5\] **Examples:**

// Precondition only:

procedure divide(a: i32, b: i32): i32

    \[\[ b \!= 0 \]\]

{ result a / b }

// Postcondition only:

procedure add(a: i32, b: i32): i32

    \[\[ \=\> result \== a \+ b \]\]

{ result a \+ b }

// Both:

procedure clamp(val: i32, min: i32, max: i32): i32

    \[\[ min \<= max \=\> result \>= min && result \<= max \]\]

{ ... }

**§12.7 Integration with Other Systems**

**§12.7.1 Integration with Memory Systems**

\[1\] The `alloc::heap` grant is governed by the O-Cap system. The `alloc::region` grant is **abolished**.

\[2\] **Heap Allocation:** Procedures that must allocate on the heap shall take a `HeapAllocator` object as a parameter (or a bundle containing one), which originates from `ctx.heap`.

\[3\] **Region Allocation:** The `region` keyword and `^` operator are primitive language features for temporary, stack-like allocation (see **Clause 11, §11.6**). They are available in any function (like stack variables) and do **not** require a capability grant.

**§12.7.2 `region` and `^` Desugaring**

\[1\] This section is **\[ABOLISHED\]**. The `region` keyword and `^` operator are fundamental memory management primitives (see §11.6) and are no longer syntactic sugar for an `Arena` capability.

**§12.7.3 `unsafe::` Grants (Abolished)**

\[1\] The `unsafe::` grant namespace is **abolished**. \[2\] `unsafe` is not a "grant" a caller gives; it is a property of the callee. \[3\] Procedures that perform unsafe operations (e.g., raw pointer dereferencing) shall be enclosed in an `unsafe` block (§11.9). Calls to such procedures do not require any special annotation, but the procedures themselves are clearly marked.

**§12.8 Diagnostics**

\[1\] Diagnostics for Clause 12\.

| Code    | Severity | Description                                                                                   | Section |
| ------- | -------- | --------------------------------------------------------------------------------------------- | ------- |
| E12-001 | Error    | `main` procedure has incorrect signature. Must be `public procedure main(ctx: Context): i32`. | §12.2   |

## **Clause 13\. Concurrency**

*(This clause is a full replacement, resolving the conflict in the supplemental docs.)*

### **§13.1 Two-Path Concurrency Model**

\[1\] Cursive provides two distinct models for managing concurrency, chosen based on the use case.

1. **Path 1 (Zero-Cost CREW):** The parallel block. This is the preferred, high-performance model for data-parallel, "read-mostly" tasks. It provides *static* data-race-freedom.  
2. **Path 2 (Capability Locking):** The Mutex\<T\> object. This is the model for managing partitioned, mutable *state*, especially for IO, FFI, or partitioned counters.

### **§13.2 Path 1: The parallel Epoch**

\[1\] The parallel block provides statically-guaranteed, zero-cost data-race-freedom for Concurrent-Read, Exclusive-Write (CREW) patterns.

\[2\] **Syntax:**

parallel\_block ::= "parallel" "(" binding\_list ")" "{" statement\* "}"

fork\_expr      ::= "fork" "{" closure\_body "}"

\[3\] **Semantics:**

1. **Enter Epoch:** parallel (world) takes one or more unique or partitioned bindings.  
2. The original world binding is *statically invalidated* outside the block.  
3. world is re-introduced *inside* the block with its permission tightened to const. This is a zero-cost, compile-time change.  
4. **Fork:** The fork keyword is now valid. It spawns a closure on a worker thread. The closure can *only* capture the const world, making data races statically impossible.  
5. **Join:** The compiler *statically requires* that all Thread handles returned by fork are join()ed before the block exits. Failure to join is a compile-time error (E13-001).  
6. **Exit Epoch:** Once all threads are joined, the block exits. The original unique permission on world is restored.

\[4\] **Example:**

let world: unique \= GameWorld::new()

loop {

    // 1\. ENTER EPOCH: 'world' is now 'const' inside.

    parallel (world) {

        // \--- CONCURRENT READ (SAFE) \---

        var threads: \[Thread\] \= \[\] // 'var' needed to rebind/push

        // 2\. FORK: Captures 'const world' (zero-cost share)

        let t1 \= fork { world.query\_physics() }

        let t2 \= fork { world.query\_ai() }

        threads.push(t1)

        threads.push(t2)

        // 3\. JOIN (STATICALLY CHECKED)

        loop t: Thread in threads { t.join() }

    }

    // 4\. EXIT EPOCH: 'world' is 'unique' again.

    // \--- EXCLUSIVE WRITE (SAFE) \---

    world.update\_positions()

}

### **§13.3 Path 2: Mutex\<T\> Capability**

\[1\] For stateful, partitioned mutability (like counters or IO), the Mutex\<T\> capability (provided by ctx.sys from §12.3.4) is used.

\[2\] **Modal Definition:**

public modal Mutex\<T\> {

    @Unlocked { ... }

    @Locked { data: unique T }

    // 'lock' takes 'const' self, allowing many threads to \*try\* to lock

    procedure lock(self: const): Mutex\<T\>@Locked { ... }

    // 'unlock' takes 'unique' self, only the holder can unlock

    procedure unlock(self: unique): Mutex\<T\>@Unlocked { ... }

}

*(Assumption: Mutex\<T\>@Locked implements the Drop trait, which calls self.unlock() for RAII-style unlocking.)*

\[3\] **Example:**

// This procedure is thread-safe because it requires the Mutex capability

procedure increment(lock: const Mutex\<i32\>) {

    // 1\. Lock to transition from @Unlocked to @Locked

    let guard: unique \= lock.lock()

    

    // 2\. We now have 'unique' access to the data

    guard.data \= guard.data \+ 1

    

    // 3\. 'guard' is dropped, its Drop trait calls 'unlock'

}

public procedure main(ctx: Context): i32 {

    // 1\. Create the capability

    let counter: unique \= ctx.sys.create\_mutex(0)

    // 2\. Spawn threads, passing the 'const' Mutex (which is safe)

    let t1 \= ctx.sys.spawn(|| increment(counter))

    let t2 \= ctx.sys.spawn(|| increment(counter))

    t1.join()

    t2.join()

    result 0

}

### **§13.4 partitioned Permission and Threading**

\[1\] The partitioned permission and Partitioning System (§11.4) are for single-threaded aliasing only.

\[2\] partitioned provides no multi-threaded safety.

\[3\] partitioned data cannot be captured by fork (E13-101) or ctx.sys.spawn() unless it is wrapped in a Mutex\<T\> or other synchronization primitive.

## **Clause 14\. Interoperability (FFI)**

*(This clause is revised to use the O-Cap and unsafe block model, removing all grants.)*

### **§14.1 FFI Declarations**

\[1\] FFI is declared using extern blocks. Calls to extern procedures are only permitted inside unsafe blocks.

extern "C" procedure C\_function(arg: i32): i32;

procedure my\_func(a: i32): i32 {

    let b \= unsafe {

        // Programmer is 100% responsible for safety

        C\_function(a)

    }

    result b

}

### **§14.2 FFI-Safe Types**

\[1\] FFI-safe types are primitives (i32, f64), raw pointers (\*const T, \*mut T), and records/enums marked \[\[repr(C)\]\].

\[2\] Cursive types with complex state (e.g., Ptr\<T\>@State, partitioned T with dynamic partitioning, Mutex\<T\>, FileSystem) are **not** FFI-safe and shall be rejected by the compiler if used in an extern signature (E14-001).

### **§14.3 \[\[repr(C)\]\]**

\[1\] The \[\[repr(C)\]\] attribute guarantees C-compatible layout for records and enums, making them FFI-safe.

## **Clause 15\. Metaprogramming (Codegen)**

*(This clause is a full replacement, based on the quote/emit model.)*

### **§15.1 comptime Procedures**

\[1\] Procedures marked comptime are executed by the compiler. They can call other comptime procedures and intrinsics, but cannot call runtime procedures.

### **§15.2 quote Expressions**

\[1\] The quote keyword captures Cursive code as an AST data structure (QuotedBlock or QuotedExpr).

comptime {

    let body \= quote {

        result a \+ b

    }

    // 'body' is now a QuotedBlock data structure

}

### **§15.3 $(...) Interpolation**

\[1\] The $(...) syntax splices compile-time values into a quote block.

* $(value): Splices a value as a literal (e.g., 1024, "hello").  
* $(string\_as\_ident): Splices a string as an *identifier*.

\[2\] **Example:**

comptime {

    let type\_name \= "i32"

    let const\_val \= 100

    let code \= quote {

        public procedure add\_$(type\_name)(a: $(type\_name), b: $(type\_name)): $(type\_name) {

            result a \+ b \+ $(const\_val)

        }

    }

    // 'code' AST now represents:

    // public procedure add\_i32(a: i32, b: i32): i32 {

    //     result a \+ b \+ 100

    // }

}

### **§15.4 codegen::emit Intrinsic**

\[1\] The codegen::emit intrinsic is the *only* way to perform code generation. It is a capability object (ComptimeCodegen) that must be available in the comptime scope. It takes a QuotedBlock and adds its declarations to the current module's scope.

\[2\] **Example:**

// This procedure generates code

comptime procedure generate\_adder(codegen: ComptimeCodegen, ty: string)

    \[\[ codegen.is\_available() \=\> true \]\]

{

    let code\_to\_emit \= quote {

        public procedure add\_$(ty)(a: $(ty), b: $(ty)): $(ty) {

            result a \+ b

        }

    };

    codegen.emit(code\_to\_emit)

}

// This comptime block calls the generator

// (Assume 'compiler' provides the capability)

comptime {

    let cg \= compiler.get\_codegen()

    generate\_adder(cg, "i32")

    generate\_adder(cg, "f64")

}

// These procedures are now available \*in the same file\*:

let x \= add\_i32(10, 20\)

let y \= add\_f64(1.5, 2.5)

## **Annex A. Grammar**

**Editor's Note:** The grammar in Draft 1.0 is now **obsolete**. It does not reflect the syntax for trait, \<, parallel, fork, quote, $(...), or the O-Cap model. This annex is pending a full rewrite for Cursive 2.0.

## **Annex B. Trait Catalogs**

*(This section, formerly "Behavior Catalogs," is carried over. It defines standard library traits like Drop.)*

