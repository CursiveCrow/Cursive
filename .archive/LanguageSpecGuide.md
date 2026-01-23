# Guide to Writing a Systems Programming Language Specification

Designing a clear and thorough language specification is crucial. By examining well-regarded systems language specs (C, C++, Go, Zig, Odin), we can identify common organizational patterns. Typically, a language specification is divided into logical sections covering everything from lexical syntax to runtime semantics. Below we outline the overall structure and section organization that are commonly used, with examples from popular languages.

## 1. Introduction and Notation

Start with a brief introduction describing the language’s purpose, paradigm, and any design goals. This sets context for readers. For example, the Go spec begins by stating Go is “designed with systems programming in mind”[1]. Similarly, a spec often mentions its intended audience and scope (e.g., “this is the reference manual for language X”[2]).
If the spec uses formal grammar, include a Notation subsection explaining the BNF/EBNF syntax used for grammar rules. The Go spec, for instance, has a notation section explaining its EBNF conventions[3]. This ensures readers understand how grammar productions are expressed.
• Tip: Keep the introduction concise. Include any relevant version or standard information and references to related documentation (e.g., a separate standard library spec).

## 2. Lexical Structure

After the intro, lexical conventions are usually the first major content section. This defines how source code is composed of characters and tokens. Common subsections under lexical structure include:
• Characters and source text encoding: Define the character set and encoding (e.g., Unicode UTF-8 in Go[4]). Mention any normalization or restrictions (Go notes that NUL bytes may be disallowed[5]).
• Comments and Whitespace: Specify comment syntax and that whitespace (spaces, newlines) is generally ignored. For example, Odin’s docs state, “Comments can be anywhere… Single line comments begin with //”[6]. Zig’s spec notes it only has // single-line comments (no C-style /\* \*/ blocks)[7][8].
• Tokens: List the classes of lexical tokens. Typically: identifiers, keywords, operators/punctuation, literals, and how they are formed[9]. It’s common to provide a table or list of reserved keywords[10] and symbols. For example, the Go spec enumerates all keywords that cannot be used as identifiers[10].
• Literals: Describe literal notation for numbers, characters, strings, etc. Often subdivided by type:
• Numeric literals: syntax for integers (decimal, hex, binary, octal) and floating-point literals (including exponent, imaginary parts, etc)[11][12]. Many specs allow underscores in numeric literals for readability (as in Odin: 1_000_000 is valid[13]). Define default types for unsuffixed literals and any implicit conversions for constants[14].
• String literals: notation for quoted strings, escape sequences, raw string syntax, concatenation rules, etc[15][16]. Provide tables of escape characters if needed[17].
• Character (rune) literals: notation for character constants (e.g., 'A', '\xFF')[18].
• Semicolon/line termination rules: If the language has automatic semicolon insertion or significant newlines, document the rules. (Go’s spec has a “Semicolons” section explaining its automatic semicolon insertion at line breaks[19].)
This lexical section essentially defines how to turn source text into tokens. It is usually very structured, often with formal grammar productions for each element (e.g., the grammar for comments, identifiers, literals, etc.). By defining the lexical syntax up front, the spec lays the foundation for the higher-level grammar that follows.

## 3. Basic Concepts and Definitions (Optional)

Some specifications include a short “Basic Concepts” or “General” section after lexical conventions and before diving into detailed syntax. This section (present in the C/C++ specs) defines fundamental ideas used throughout the spec. It might cover terms like “program,” “implementation,” “undefined behavior,” “object,” “value,” etc., and broad topics such as the memory model or evaluation strategy.
For instance, the ISO C++ spec has a clause for “General concepts” and “Terms and definitions” where it explains what objects, storage duration, execution environment etc. mean in the context of the language. C’s spec also defines concepts like object lifetime, sequence points (or their modern equivalent) early on. If your language has any such fundamental concepts (e.g., behavioral guarantees, memory safety assumptions, concurrency model basics), it’s helpful to define them here before the detailed syntax sections. Keep this section high-level and concise – detailed rules will come later, but this provides a conceptual framework.
(In many modern language docs, this section may be minimal or merged into later sections. If your language is straightforward, you can sometimes skip an explicit “basic concepts” chapter and introduce terms as needed in relevant sections.)

## 4. Types and Values

Every systems language spec needs a section describing its type system and the universe of values. Common practice is to break this into subsections by category of types:
• Built-in Primitive Types: List the fundamental scalar types (integers, floating-point, boolean, characters, etc.) and their properties. For example, Zig’s reference lists its integer and float types in a table with their sizes and C equivalents[20]. You should specify things like size/range (or at least whether they are architecture-dependent) and behavior (e.g., overflow rules or undefined behavior). Mention special primitive types like byte/char, and boolean (true/false as predefined constants[21]).
• Composite Types: Discuss how to form compound types:
• Arrays: Define array types and their properties (length, indexing, whether length is part of type). E.g., “An array is a sequence of elements of a single type; the number of elements is the array’s length”[22].
• Slices or pointers to arrays: If the language has pointer or reference types (common in systems languages), describe pointer types. Go and Zig both have slice/pointer concepts: “A slice is a descriptor for a segment of an underlying array”[23], whereas Zig explicitly distinguishes pointers (single-item vs many-item)[24].
• Structures (Records): Define struct types as aggregates of named fields. The spec should explain how struct declarations work and how fields are accessed. Odin’s documentation, for instance, states: “A struct is a record type… a collection of fields. Struct fields are accessed with a dot syntax.”[25]. In the spec, you might provide grammar for struct type definitions and clarify aspects like default alignment or padding rules (if any, for system languages this can be important to mention or leave to implementation with a note).
• Unions: If the language supports unions (data types where only one of many fields is valid at a time), describe their syntax and semantics (e.g., “only one field can be active at a time”[26]). Not all modern languages have C-style unions, but Zig does, and C/C++ do (with well-defined or implementation-defined behavior).
• Enumerations: If there are enum types, explain how they are declared and what their underlying values are. Zig and Odin have enums; Zig’s spec for example shows how enum fields can be used and enum literals without specifying the type[27].
• Other Type Constructs: Cover any additional type categories:
• Function (Procedure) Types: If functions are first-class or if function pointers exist, describe the type representing a function signature. For instance, Go’s spec has Function types as a subsection[28], and Odin’s docs cover procedure types/literals in its “Procedures” section.
• Pointer Types / References: Already touched on in composite types; ensure rules for pointer usage are covered (e.g., pointer arithmetic if allowed, null pointer constant, etc.).
• Interface or Trait Types: If the language has interface types (like Go’s interfaces or a concept of abstract base classes in C++), explain how those work. Go dedicates subsections to Interface types[29] including how a type implements an interface[30].
• Generics/Parametric types: If the language supports generic types or templates, you may not cover it in the “Types” chapter directly, but you might just allude to it and have a separate section later. For example, C++ defers templates to a later chapter; Odin has a section called Parametric Polymorphism (generics)[31] to explain generics after covering basic type and procedure syntax.
• Type Properties: Some specs include a subsection on general properties of types and values:
• Zero value / default initialization: If your language defines a default “zero” or “nil” value for types, state it. Go’s spec explicitly defines the zero value for each type (e.g., numeric types default to 0, pointers to nil, etc.)[32][33].
• Type identity and equivalence: Rules for when two types are considered the same. C++ and Go both have detailed rules for type identity or assignment compatibility[34][35]. If your language has name equivalence or structural equivalence, clarify it here.
• Representation and memory model: You might briefly note how values are represented (especially if relevant to systems programming, e.g., endianness is usually not in spec, but aliasing rules and alignment might be). Go’s spec has a subsection “Properties of types and values” discussing how values are stored (self-contained vs references)[32][36]. C++ and C have the concept of object representation and value representation that could be mentioned.
This Types section tends to be one of the longest, as it enumerates the building blocks of data in the language. It’s often helpful to include examples or tables (like Zig’s primitive type table[20]) for clarity. Ensure each type form (array, struct, etc.) has a grammar rule and an explanation of its semantics.

## 5. Variables and Declarations

Next, specify how variables are declared and how names bind to types. Many specs cover constants and variables in separate subsections:
• Constant Declarations: Describe how to declare constants (compile-time constants or immutable values). Odin’s spec notes, “Constants are entities which have an assigned value that cannot be changed and must be evaluable at compile-time.”[37]. Include grammar for constant declarations if different from variables. If the language has enumerated constants or defines, mention those too.
• Variable Declarations: Explain syntax for variable declarations, both local and global. State rules like definite assignment or default initialization. For example, Odin’s overview shows simple declarations: “x: int declares x to have type int… Variables are initialized to zero by default unless specified otherwise.”[38]. Go’s spec similarly has a Variables section describing what a variable is and how it’s declared[39].
• Cover various forms (if any): e.g., type inference in declarations (:= in Go/Odin)[40], multiple declarations in one line, etc.
• Storage Class / Lifetime (if applicable): Low-level languages like C/C++ have storage classes (static, auto) and lifetime (automatic, static, dynamic). If relevant, describe how variables are allocated and how long they exist. C’s spec has “storage-class specifiers” and rules for static duration, etc., whereas in Go/Odin such details are simpler (all global variables are static-duration, locals are automatic, etc., which you can mention).
• Type Declarations: If your language allows defining new type names (aliases or typedef-like constructs), include that. Go, for instance, allows type aliases and new named types; Odin supports type aliases[41]. Mention how naming a type works (does it create a distinct type or just an alias?).
• Declarations and Scope: It’s common to have a combined section on Scope rules alongside declarations. Specify where an identifier is visible. Go’s spec explicitly has “Declarations and scope” as a section[42], which defines how and where each declared name is valid. It enumerates block scope, file scope, package scope, etc., and special rules (like blank identifier \_ in Go which isn’t a declaration[43][44]). C and C++ also define scopes (block, file, class, namespace, etc.) in their specs.
• Include any unusual scope rules: e.g., Go’s rule that an unused label is an error and that labels have function scope (not block scoped)[45], or C++’s rule that variable declarations in for loops have limited scope. Odin’s docs likely rely on simpler block scoping similar to C.
• Name resolution and Linkage: In systems languages, especially those that compile to binaries, you might need to mention how global names are linked across modules (C/C++ have linkage specifications). If your language has modules or packages (discussed later), you can defer cross-file naming to that section. But do cover how the same identifier can or cannot be reused in overlapping scopes (most specs say no shadowing in the same scope[46], but inner scopes may shadow outer ones[47]).
• Predeclared and Reserved Identifiers: Many specs list names that are always declared by the language. For example, Go predeclares certain built-in types and functions in a “universe” scope (e.g., bool, true, false, nil, iota, append, len, cap, etc.)[48][49]. It’s helpful to document these magic predefined names so users know they don’t need to declare them (and cannot redeclare them).
In summary, the Declarations section defines how to introduce names (of variables, constants, types, functions, etc.) and what regions of the program those names cover. It bridges the gap between the type system and actual program code. Clear scoping rules are particularly important for a precise spec.

## 6. Expressions and Operators

The Expressions part of the specification explains how values and calls are combined and evaluated. This is typically a large section, often organized by operator precedence or by expression category. A common approach is:
• Start with a general statement: “An expression specifies the computation of a value by applying operators and functions to operands.”[50]. This gives the reader a high-level idea that expressions produce values and may have side effects.
• Value categories (if applicable): Some specs (especially C++’s) define categories like lvalue/rvalue at the start of this section. If your language needs to distinguish them (for assignment or passing by reference), clarify here. Otherwise, you might not need this if such distinctions are not made explicit.
• Then detail specific kinds of expressions in order of precedence (highest-precedence operators/operands first, down to lowest). For example:
• Primary expressions: Usually covers operands that don’t involve operators: e.g., literals, variable references, parenthesized expressions, perhaps lambda or compound literals. (C++ spec has “Primary expressions” as a subsection, including literals[51], this pointer, lambda, etc.)
• Postfix/Call expressions: Function calls, array indexing, struct field access (a.b), type‑scope procedure calls (obj::proc(...)), etc. Define the syntax and order of evaluation for each. Go’s spec, for example, would describe selector expressions (for field access) and index expressions under expressions.
• Unary expressions: Negation (-x), logical NOT, bitwise NOT, address-of, dereference, type conversion casts, increment/decrement (++/--), etc. Each operator is specified for what it does and its operand types.
• Binary expressions: Cover arithmetic operators (+ - \* / %), bitwise ops, shifts, and their semantics (including integer overflow behavior, which in C is undefined, in Zig causes a runtime panic in debug or wraps in release, etc. – these details should be specified or referenced). Relational and equality operators (==, !=, <, >, <=, >=) and the types they apply to should be covered.
• Logical operators: AND, OR (&&, || in C/Java, and, or in some languages) and short-circuit behavior.
• Combined assignment: If +=, -= etc. are distinct in the grammar, mention them (usually as part of binary operators).
• Ternary conditional (?:) if the language has it (C/C++ do; Go, Zig, Odin do not – Odin uses when and if instead, which are statements).
• Other special expressions: This includes:
o Type conversions / casts: e.g., C-style cast or function-like casts, or cast(type, value) forms. Odin and Zig have explicit casts (T(v) in Odin[52], @intCast etc. in Zig). The spec should define how conversions happen, which are implicit vs explicit.
o Compile-time or constant expressions: If your language has a notion of compile-time evaluation (like C’s constexpr, Zig’s comptime), describe what constitutes a constant expression.
o Pointer arithmetic: if relevant (C/C++ allow arithmetic on pointers within arrays). Document rules like scaling by element size, etc.
o Short-circuit behaviors and order of evaluation: For instance, many languages guarantee left-to-right evaluation for certain operators or function arguments, while C/C++ only fully specify some aspects. If your language defines a strict evaluation order, state it. If not (like C’s unspecified order for function arguments), note that too.
o Error handling expressions: Zig has try and catch expressions for error unions, which are specific to its error-handling mechanism[53]. If your language has something like exception throw expressions or await in expressions, include them here.
Many specifications include an operator precedence table to summarize the hierarchy. Zig’s documentation, for example, provides a table of operators with descriptions[54][55] and even shows example usage and edge cases (like wrapping arithmetic) in context[56][57]. You may either incorporate such a table or clearly structure the subsections in decreasing precedence. The Go spec lists grammar productions in a way that inherently defines precedence (since higher-precedence productions occur earlier in the grammar).
Also, mention associativity and order of evaluation rules. For instance, describe that multiplication/division are evaluated left-to-right. In languages like C++17 and later, many order-of-evaluation aspects are defined (e.g., operands of + are evaluated left-to-right, etc.), whereas older C/C++ left some unspecified. Make a conscious choice and document it to avoid ambiguity.
By the end of this section, the reader should know exactly how any complex expression will be parsed and what it will do at runtime. For a systems language, this includes the gritty details (like integer overflow, sequence points or sequence of evaluation, etc.), all spelled out or referenced.
Example from Go: The spec explains that certain operands like a function call or receive operation are not guaranteed to happen in any specific order relative to each other, except where the grammar forces it – these sorts of notes belong in the expressions section. Go’s spec formalizes that with simple language since it has simpler rules than C++.
Example from Zig: Zig explicitly disallows undefined order by having well-defined behavior for all expressions, which the documentation illustrates (Zig catches undefined overflow in debug builds[55]). If your language has similar safety checks, note them.

## 7. Statements and Control Flow

After expressions, specify the syntax and semantics of statements – how the language performs actions, controls flow, and groups instructions. Common subsections under Statements include:
• General Syntax of Statements: Many specs first present a grammar for Statement (often covering simple statements and control flow structures).
• Expression Statements: Using an expression (usually for its side effects) as a statement. E.g., calling a function or using the ++ operator as a statement.
• Declaration Statements: If your language allows declarations inside code (like C’s local variable declarations or constants), clarify that they are also statements or separate. (Go treats short variable declarations as statements, for instance.)
• Compound (Block) Statements: Define how statements can be grouped, typically with { ... } forming a block (and that a block is a new lexical scope). Go’s spec defines Block as “a sequence of declarations and statements within matching braces”[58].
• Control Flow Statements: Each major control keyword usually gets a subsection:
• Conditional: if (and possibly else). Specify the syntax (if condition then block else block…) and any special rules (e.g., in Odin/Go, parentheses around the condition are optional[59], but braces are mandatory). Note scoping if the language allows an initializer (like C++/Go if(init; condition) or the new variable in an if).
• Switch/Match: Most systems languages have a multi-way branch: switch in C/C++/Go/Odin or match in others. Document how cases are evaluated (fall-through behavior or not, how patterns or constants are matched). For example, Odin’s spec notes that in its switch, each case executes without needing an explicit break (no implicit fall-through)[60][61], whereas C’s switch by default falls through unless break is used. If your language has pattern matching (like Rust’s match or a more advanced switch), detail that syntax and semantics.
• Loops: for, while, do/while as applicable.
o For loops: If multiple forms exist, clarify each. Go and Odin have a single for that can act like a while or C-style loop. Odin’s documentation, for example, illustrates a basic for with init/cond/post clauses[62], as well as range-based loops[63] and special directives like #reverse for reverse iteration[64] or #unroll for loop unrolling[65]. Include such details if your language has them.
o While loops: If present, grammar and meaning (simple condition checked before each iteration).
o Do/While: If present, note that it checks the condition after each iteration.
o Foreach/range loops: If provided (like for x in collection), explain iteration order and any constraints (e.g., if collection can be modified during loop).
• Break/Continue: Explain loop control statements. Most specs say “break terminates the innermost loop or switch”, “continue skips to the next iteration of the loop”, etc[66].
• Return: Syntax of returning from functions, with or without a value. Mention if multiple return values are allowed (Odin and Go allow multiple return values; Odin’s spec has “Multiple results” as a subsection[67]).
• Defer/Defer-like: Languages like Go and Odin have defer (execute a statement at function end)[68] or defer { } blocks (C++ uses RAII instead of a keyword, but if your language has a construct for deferral, include it).
• Goto and Labels: If the language has goto, specify label syntax and scope (e.g., Go has them; Odin might not). Document restrictions (can’t jump into an inner scope in many languages, etc.). Go’s spec covers label scope and the requirement that label must exist and be used[45].
• Exception handling: If the language uses try/throw/catch for error handling (like C++ exceptions), those are statements (or expression in throw’s case) to document. For example, explain try-block syntax, catch clauses, and unwinding behavior. (Zig and Odin don’t have exceptions; they use error unions or multiple returns.)
• Concurrency statements: If the language has built-in concurrency support (like Go’s go statement to spawn goroutines, or spawn in some languages), document them here. The Go spec’s “Go statements” section defines: “A go statement starts the execution of a function call as an independent goroutine…”[69]. Also, Go’s select statement (for waiting on channel operations) would be described in this section.
• Other statements: e.g., return, yield (if generators), assert (if treated specially by the language), etc.
• Blocks and Scope Reminder: You might reiterate how blocks create scopes for variables (if not already covered under declarations). Go’s spec enumerates implicit blocks like each if or loop having its own implicit block[70].
Typically, each statement form’s syntax is given (in grammar) and then an explanation of its runtime effect is provided. For example, the spec might say: "if Statement – The syntax is if [InitStmt]; Condition Block [else Block]. The condition is evaluated; if true, the first block executes, otherwise the else block (if present) executes. The condition expression must be a boolean (or equivalent)...", and so on. Provide any special rules (like in C/C++, the condition is contextually converted to bool; or in your language, perhaps only booleans are allowed, not integers).
Example (Odin when): Odin has a compile-time conditional when (for constant expressions) in addition to runtime if. If your language has something similar, describe it either under statements or in a compile-time execution section. Odin’s when is documented as “almost identical to if but with some differences”[71] – such differences would be important to note in the spec.
By covering statements, you define the imperative control structures available to programmers. This section, combined with Expressions, defines most of the language’s execution logic. Make sure to cover edge cases (like falling off the end of a function is equivalent to return in some languages, or that a switch with no cases simply does nothing, etc. – these are often noted in passing).

## 8. Functions and Procedures

Next, describe how to declare and use functions (procedures) in your language. This overlaps with declarations and statements but usually merits its own focused section because functions are a core unit of program structure and behavior. Key points to include:
• Function Declarations: Syntax for defining a function/procedure. This was probably introduced in the grammar earlier (for example, under “Top-level declarations” you may have included FunctionDecl). Here you elaborate on it. For instance, you explain the components: optional modifiers (like pub or export), the name, parameter list, return type, and function body (block of statements). If your language supports forward-declarations or separate prototypes (like C’s declarations vs definitions), explain that. In Go, function declarations are simple and must include bodies except for interface methods. Odin uses :: proc() syntax for procedures[72]; C/C++ use a more complex declarator syntax.
• Parameters and Return Types: Discuss parameter passing (by value or reference), evaluation order (most languages evaluate arguments left-to-right, but check if any deviation). Odin’s docs have a subsection “Parameters” with examples[73]. Note if default parameter values or named arguments are supported (Odin allows both named arguments[74] and default values[75]; C++ has default parameters but no naming; Go has neither).
• Multiple Return Values: If supported, specify how they work (both Go and Odin allow multiple returns; the spec should clarify how they are syntax-wise and how assignment from a multi-valued call works).
• Calling Convention and Evaluation: Typically, the spec might not delve deeply into calling conventions (that’s more ABI-specific), but you should define how a function call expression works in terms of the language semantics (e.g., it evaluates arguments, then transfers control, etc., and what happens on return).
• Anonymous functions or Lambdas: If the language allows literal function values (like function closures), describe their syntax and scoping rules. (Odin doesn’t have lambdas as far as I know; C++ does with [=](int x){ ... }, Go has function literals func(x int){ ... }.)
• Type‑scope procedures on types: If your language supports procedures attached to types (e.g., functions with an explicit `self`/`$` receiver), describe that. Prefer `Type::proc(self, ...)` style resolution and specify permission requirements on `self`.
• Overloading/Generics: If the language allows function overloading (same name, different parameters) or generic functions (templates), mention how that is handled. C++ covers overloading and function template specializations in depth; Odin explicitly allows procedure overloading and even provides a keyword for it[77]. If your language has ad-hoc polymorphism (overloading or multimethods), specify the resolution rules (at compile time by most specific signature, etc.). If not, simply state that function names must be unique in a scope (like Go requires).
• Inline Assembly or Intrinsics: Some systems languages permit embedding assembly or special intrinsics in functions. If your spec will cover that, you might allocate a subsection (with all necessary caveats about being implementation-defined or non-portable).
• Coroutines/Generators: If functions can yield (produce sequence of values) or pause (as in coroutines), document the semantics of yield or await and how it affects function return types, etc.
An example from Odin: The spec’s “Procedures” section describes what a procedure is (they use the term instead of function), with examples of defining and calling them[78]. It covers advanced features like “explicit procedure overloading”[77], “variadic parameters”[79], and “named results” (named return values treated as variables in the function)[80]. If your language has analogous features, follow a similar approach: dedicate sub-sections to each feature.
In summary, this section should tell the reader how to define a subroutine, how parameters and return values work, and how such units integrate into the language (as first-class values or not). It often ties together with earlier sections: for example, you might refer back to the Declarations section for the rule that a function name can be declared only once in a scope, etc.

## 9. Modules, Packages, and Imports

Most systems languages have a concept of compilation units or modules beyond individual functions. Depending on the language, this could be simple (C has files and declarations with external/internal linkage) or more high-level (Go has packages, Rust has modules, Odin organizes code in packages). Your specification should include a section describing how code is organized across files and libraries:
• Source Files and Modules: Define what constitutes a module or package. For instance, Go states: “Go programs are constructed by linking together packages. A package in turn is constructed from one or more source files...”[81]. Odin’s docs note “Odin programs consist of packages. A package is a directory of Odin code files, all of which have the same package declaration at the top.”[82]. Similarly, mention how a program is composed (in C, a program is a set of translation units linked together; in your language perhaps a set of modules).
• Module Declaration: If applicable, describe how a file declares its module or package name (e.g., package foo; at the top in Odin/Go[72], or module name in others). State any rules about module naming and file system mapping (many languages require directory structure to match package names – Odin/Go do).
• Import/Include mechanism: Specify how one module uses definitions from another:
• If using an import keyword (like Go’s import "pkg/path" or Odin’s import), describe its syntax and when it executes (compile-time) and how it affects scope (usually it brings identifiers from the imported package into a namespace or requires qualified access). Odin’s “import statement” subsection explains how to import packages from its core library[83], and notes that all declarations in a package are public by default[84] unless otherwise specified.
• If using header files (as in C/C++ #include), you might discuss the preprocessor in a separate appendix or earlier in lexical (C’s preprocessor is a substantial topic outside the pure grammar).
• If the language supports importing specific symbols vs the whole module, detail that (e.g., Python/Rust allow import x as y or such).
• Module initialization and entry point: Explain any rules for initialization order. For example, Go’s spec has a section “Program initialization and execution” that defines how package init functions run and that the main package’s main() function is the entry point[85]. If your language requires a special main function (like C, C++, and Go do) or allows any top-level code execution, clarify that. Odin presumably expects a main procedure in the main package to start execution (similar to Go).
• Visibility and Exports: Clarify how identifiers become visible outside their module. Many languages have a notion of exported vs private members (e.g., in Go any name starting with capital letter is exported from a package[86]; in C, using static vs non-static at file scope controls linkage; in C++, everything in a namespace is visible to other files unless in an anonymous namespace). Document the rules for what is accessible across module boundaries. Odin’s docs mention that all package-level declarations are public by default[84], which is a design choice to note.
• Module Dependencies and Order: If relevant, mention that the language or its implementation handles dependency ordering. For instance, you might note that circular imports are or aren’t allowed, or that all imports are resolved before program start (Go does this with deterministic initialization order: imports first, then package-level vars, then init functions).
• Conditional compilation or configuration: If the language has any mechanism to include/exclude code depending on conditions (like C’s preprocessor #if or Zig’s compile-time if), mention how that interacts with modules or files. Often, this is an advanced topic possibly deferred to an appendix or a “Miscellaneous” section.
This section essentially defines the granularity of compilation and linking. In a systems programming language, it’s important for the spec to specify how separate compilation units interact. For example, in C++ the one-definition rule (ODR) and linkage specifications would be explained (though those are quite complex in detail; you might not need such complexity if your language has simpler module rules).
By covering modules/packages, you ensure that someone reading the spec understands how to build a complete program from parts, not just how to write individual functions.

## 10. Error Handling and Exceptions

    Systems languages vary in how they handle errors or exceptional conditions. Dedicate a section to whatever mechanism your language provides, whether it’s exceptions, error codes, or other constructs:
    • Exceptions: If your language has try/throw/catch exceptions (like C++ or Java), describe:
    • How to throw an exception (syntax of throw).
    • What types can be thrown (in C++ any object copyable, in other languages only certain types).
    • The propagation of exceptions: unwinding, stack cleanup, etc., possibly referencing language runtime behavior.
    • Try-catch syntax and semantics, including matching rules for handlers (by type or value).
    • Possibly a finally or defer-equivalent to ensure cleanup, if present.
    • Error codes / unions: If no exceptions, perhaps errors are represented as return values or special unions. Zig is a good example: it doesn’t have stack unwinding exceptions, but uses an “error set type” and “error union type” to propagate errors[87]. The Zig spec provides a section on Errors describing how an error value can be carried alongside a normal value using the ! operator in a function’s return type[88]. It also covers how try expressions work to propagate errors by returning from the function if an error occurs[89]. If your language uses a similar pattern, explain the syntax (! or Result<T, E> types, etc.) and semantics of checking and propagating errors.
    • For instance, mention if there’s a keyword to raise an error or to match on error results (Zig’s try and catch inside expressions).
    • Odin largely relies on multiple return values or optional types for errors, which you might cover in its Types or Statements (defer for cleanup) sections. (Odin doesn’t have exceptions; it might encourage using an optional type or an or_return mechanism).
    • Assertions: If the language defines an assert statement or built-in for debugging that might terminate the program on failure, mention it and whether it’s always on or can be disabled.
    • Resource management (Defer/Using): We touched on defer earlier in statements. If not already detailed, ensure that any idiomatic error-handling cleanup constructs are clearly explained (like defer closing a file or a RAII object’s destructor in C++ being called on scope exit – though C++ deals with that in object semantics, not a keyword).
    • Termination and Panics: Define what happens if an unhandled error/exception occurs. Does the program abort (like a panic in Go or an uncaught C++ exception calling std::terminate)? The spec should clarify the behavior in such edge cases (e.g., “If an exception is thrown and not caught, program execution is terminated.”).
    Essentially, this section covers how the language deviates from the normal flow of control in the face of errors or unusual conditions. It is very important in systems programming, as these languages often need to clearly specify which errors must be checked by the programmer and which are fatal or UB (undefined behavior). For example, C’s spec says that integer overflow is undefined behavior (so the spec must explicitly mention that where relevant in Expressions), whereas Zig’s spec guarantees catching overflow in debug mode[55], or saturating arithmetic if explicitly used[90] – these are error-related aspects to articulate.

## 11. Concurrency and Memory Model

    Modern systems languages usually define a memory model (especially if the language allows multi-threading or atomic operations). Depending on the complexity, you might include:
    • Memory Model: If your language is low-level like C/C++, include a subsection describing the memory model (e.g., sequence consistency, atomic operations, what data races are, etc.). The C++ spec has an entire chapter on the memory model, but for a simpler spec you might just state basic guarantees (for example, “reads and writes to different variables can happen in any order unless synchronized; a data race produces undefined behavior” – if that’s your stance, or the opposite if you guarantee data-race safety like Java’s memory model does in some ways).
    • Threads and Concurrency Primitives: If the core language includes threads (like Go’s goroutines and channels), detail them. The Go spec incorporates concurrency by describing the behavior of go statements (which we cited above) and channel types/operations:
    • Channel send/receive syntax and that they may block[91].
    • The select statement semantics (if applicable).
    • That goroutines run concurrently with no guaranteed ordering except synchronization via channels or other primitives.
    • Atomic operations / Volatile: If applicable, specify any atomic or volatile keywords semantics. C11/C++11 have <stdatomic.h> and atomic<> which are partly library, partly core memory model – you might not need this unless your language design explicitly includes it.
    • Memory Safety: If your language has specific guarantees (like Rust’s borrow checker rules, or a GC that avoids use-after-free, or Zig’s approach to dangling pointers), summarize those rules. e.g., “It is undefined behavior to access memory through a pointer that is not valid (dangling or null). The compiler does not automatically check this, so programmers must ensure pointers are valid.” In contrast, if you have a GC, state that objects are freed only when unreachable, so dangling pointers are not possible (but maybe you then disallow pointer arithmetic, etc.).
    • Pointer aliasing rules: C++ has strict aliasing rule (no accessing an object of type T through a pointer of unrelated type U, with some exceptions). If your language has or doesn’t have such a rule, specify it to aid optimizer expectations.
    • Implementation limits: Sometimes, specs mention minimum limits (like C mandates at least 32767 nested blocks, etc. in annex). This might not be needed, but if you have constraints like max stack size or recursion depth, you could note them or leave it to implementations.
    This topic can become very deep. For a new language, you might simplify by either adopting an existing well-known model (e.g., “Our language’s concurrency model is similar to C++20’s, with sequential consistency for data-race-free programs[92]”), or if you have a managed runtime (like Go’s, which doesn’t require the programmer to define memory barriers explicitly), describe the programmer’s view (channels synchronize memory, etc.).
    Example: The Go spec doesn’t present a formal memory model in the main spec, but the Memory Model is described in a separate document. Instead, the spec simply states that “The behavior of programs that contain data races is undefined.” It relies on the reader to infer the rest or read the separate memory model doc. You might do similarly: mention data races and refer to an appendix or separate spec if needed.
    If your language is single-threaded by design (like Zig has no built-in threads, concurrency is via libraries), you can note that and omit a detailed memory model. But given systems programming often involves concurrency, it’s wise to address it even briefly.

## 12. Standard Library and Built-in Functions

    While the language specification usually focuses on core language syntax and semantics, many specifications at least mention built-in functions or the standard library basics, especially if certain library features are tied to the language semantics.
    • Built-in functions/keywords: Some languages have built-in functions that are treated specially by the compiler. For example, Go defines that len(), cap(), append(), make(), new(), panic(), etc., are built-in functions that are available without import[48][49]. The Go spec has a section enumerating these and explaining their behavior[93]. If your language has similar built-ins (e.g., memory allocation primitives, or special keywords like sizeof that act like operators but look like functions), document them here.
    • Standard Library Structure: Although a full standard library is usually documented separately, you might provide a brief overview of how the library is organized (especially for system languages where some library parts might blur with the language, like I/O might require language support). C and C++ specs actually include the entire standard library in the specification document. For a new language spec, you might not integrate library spec, but you can mention the existence of a core library and how it is imported.
    • Environmental interaction: Define if the language has any assumptions about the runtime environment (for instance, C++ main can have command-line arguments from the environment; or a language might specify that there’s a standard output stream available by default, etc.). This could be part of either the library or the program execution section.
    • Intrinsic types or magic constants: e.g., if null/nil is not just an ordinary constant, mention its nature (Go treats nil as a predeclared identifier of a builtin zero-value constant for pointers, interfaces, etc.).
    • If your language has attributes or annotations that affect optimization or interfacing (like #[inline] or dllimport in others), and they are part of the language syntax, document those in either a separate section or alongside the constructs they modify.
    It’s acceptable for the spec to defer to a library reference for details of library calls. But any library functionality that could affect the compiler or language semantics should be mentioned. For example, if calling a certain library function has special semantics (like exit() terminating the program, or abort() not running destructors in C++), the spec or a note might mention it.

## 13. Appendices (Grammar Summary, etc.)

    Finally, many language specs include appendices or reference tables for quick lookup and formal reference:
    • Complete Grammar: A full grammar of the language in BNF/EBNF form is often given in an appendix or at the end. This is extremely useful for implementers and advanced users. For instance, the C++ draft has an appendix with the grammar productions for everything, and the C spec includes a Yacc grammar in an annex.
    • Summary of Differences: If this is not the first version of the spec, sometimes differences from previous versions or from related languages are highlighted.
    • Reserved words list: Though likely already in Lexical section, a summary table of keywords can be repeated for convenience.
    • Operator Precedence Table: Again, if not given inline in the Expressions section, an appendix table can summarize all operators and their associativity/precedences in one place.
    • Library summary: C/C++ have library headers summarized in tables[94][95]. You may not need this in a first spec, but it’s a consideration.
    • Undefined/Implementation-defined Behaviors: C and C++ specifications have appendices listing all the undefined, unspecified, and implementation-defined behaviors in the language. This is a “deep spec” feature that might be overkill for a new language initially. But it’s a good practice to document anything that is not fully specified and left to implementations, either inline or in a summary list.
    • Index: A thorough spec often has an index of terms and where they’re defined. While not part of the spec’s content, it’s immensely helpful.

---

With the above sections, your specification will cover the overall structure of the language (from basic lexemes up to program execution) as well as section organization within each area (e.g. types broken down into primitives/arrays/structs, statements broken down by each keyword, etc.). The key is to maintain a logical flow: lexical syntax → types and basic declarations → how those are used in expressions → how expressions form statements → how statements form functions → how functions and types form programs. This mirrors the structure found in the C spec (which is organized in parallel to these concepts[96]) and the Go spec (which flows from lexical to grammar to types to statements to runtime aspects in a similar order).
To ensure clarity, follow the common practices observed:
• Give precise definitions (often using normative wording like “must”, “shall”, “undefined behavior” for constraints).
• Provide examples where the spec might be tricky. Zig’s documentation is very example-heavy, which helps readability for programmers (e.g., showing sample code for loop unrolling or error handling). Go’s spec is lighter on examples but still includes some (like illustrating rune literal or slice behavior in text).
• Be consistent in section ordering so that earlier sections don’t refer to concepts not yet introduced (or if they do, forward-reference and explain later). For instance, define what an identifier is in the lexical section before talking about declarations that use identifiers.
• Use headings and subheadings logically so readers can find what they need. For example, if someone wants to know about the switch statement, they should see a “switch statement” subsection under Statements. If they want to know about enums, it should be under Types, etc.

## Typical Specification Outline Template

To summarize, here is a template outline for a systems language specification, reflecting common structure and section breakdown:

1. Introduction – Purpose of the language, scope of the spec, notation (EBNF grammar conventions, etc.)[3].
2. Lexical Structure – Character set and encoding[4]; tokens and keywords[10]; literals (numeric[11], string[15], char, etc.); comments and whitespace rules[6]; any preprocessing (for languages that have a separate lexical preprocessing phase).
3. Basic Concepts (if needed) – Definitions of fundamental terms and concepts (undefined behavior, memory model overview, program structure, etc.), if not covered elsewhere.
4. Types – All types in the language and their semantics:
5. Primitive types (integers, floats, boolean, etc., with ranges or sizes)[20].
6. Composite types: arrays[22], slices/pointers[97][98], structs[25], unions[26], enums[27], etc.
7. Reference/Pointer types and null/nil behavior.
8. Function types (if functions are first-class)[28].
9. Interface/trait types (if the language has them)[29].
10. Type definitions/aliases[41] and type identity rules[34].
11. Properties of types: default “zero” values, type inference rules, or compile-time evaluable constant types (if applicable).
12. Declarations – Syntax for declaring:
13. Constants (compile-time constants)[37].
14. Variables (with or without initialization, type inference)[38].
15. Type declarations (new type names or aliases).
16. Scope and Lifetime: blocks, visibility, shadowing rules[46][47]; static vs local lifetime (if applicable); module vs local scope.
17. Linkage/Exports: how declarations in one file/module are made available to others (e.g., export keyword or public by default, etc.)[84].
18. Predeclared identifiers: list of names that are always defined (keywords, built-in types like int, built-in functions like len in Go[48]).
19. Expressions – Definition and evaluation of expressions:
20. Value literals (mostly covered in lexical, but constant expressions might be discussed here).
21. Operators: unary, binary, and ternary operators with their precedence and associativity[54][56].
22. Operator semantics: arithmetic (including overflow or division by zero handling), comparisons, logical ops (short-circuit rules), bitwise ops, pointer arithmetic (if allowed).
23. Function calls and order of evaluation of arguments.
24. Member access: struct field access a.b, array indexing a[i], slicing a[i:j], etc., with any bounds-checking rules[99].
25. Type conversion casts: rules for implicit vs explicit conversion[52].
26. Miscellaneous: conditional (?: if present), compound assignments, comma operator (C), lambda expressions (if present), etc.
27. Constant expressions: which expressions can be evaluated at compile time (if your language has constexpr-like facilities or a comptime context[100]).
28. Statements – Control flow and other statements:
29. Block statements ({ } blocks and scoping)[101].
30. Expression statement: using an expression (like a function call) as a statement.
31. Declaration statement: if declaring variables in code is allowed (e.g., var x = 5 as a statement).
32. If statement (syntax, including optional else)[59].
33. Switch statement (or match) – including constant vs pattern matching, fall-through rules[60][61].
34. Loop statements: for (with its variants: C-style, range loops[62], etc.), while, do/while as applicable. Mention iteration order and loop scoping.
35. Break/Continue – loop control behavior[66].
36. Return – returning from functions (and effect on function flow).
37. Defer/Defer-like – if present, execution of deferred statements at end of scope[68].
38. Goto and Labels – if present, with scope of labels and restrictions[45].
39. Exception handling statements: try/catch/throw or equivalent, if the language has exceptions.
40. Concurrency statements: e.g., go func() in Go launching a goroutine[69], or other parallelism constructs like parallel for, etc.
41. Other: assert, yield (if generators), etc., as applicable.
42. Functions and Procedures – How to define and use functions/procedures:
43. Function declaration syntax: parameters (with types and default values if any)[73][75], return types (single or multiple)[67], variadic parameters[79].
44. Procedure syntax on types: if procedures can be attached to types, explain receiver forms (`$`, `self: T`, `self: mut T`, `self: own T`) and the `::` call syntax.
45. Function overloading: if allowed, how resolution works (by number/types of args)[77]. If not allowed (like Go), state that each name can only have one function in a scope.
46. Anonymous functions/lambdas: syntax and scoping rules for function literals, if supported.
47. Closures: if lambdas capture variables from enclosing scopes, describe capture semantics (by value or reference).
48. Coroutines/Generators: if supported, how yield works or how function state is preserved across calls.
49. Built-in special functions: constructor/destructor if an OO language, or special lifecycle callbacks if any.
50. Modules/Packages – Organization of code:
51. Source file structure: how to declare a module or package in a file (e.g., package name; at top).
52. Import/include mechanism: syntax and semantics of importing other modules[83]. Whether it’s compile-time include (as in C) or module system (as in Go/Odin).
53. Visibility: what is exported vs private by default (e.g., capitalized names exported in Go[102], everything public in Odin unless in a sub-package).
54. Initialization order: if modules have initialization code (like Go’s init functions), specify the order (imported packages initialize before importers[103], etc.).
55. Program entry point: define how the program starts (e.g., main function in an executable module).
56. Linking rules: if applicable, how multiple modules are linked (for lower-level languages, mention that unresolved symbols are link-time errors, etc.; for higher-level, this might be implicit in imports).
57. Memory Model and Concurrency – (if not fully covered earlier)
    o Memory model: rules for memory access in multithreaded contexts (data races and their effect – undefined or otherwise).
    o Atomic operations: if language provides atomic types or operations, their guarantees.
    o Thread creation and synchronization: if not part of core syntax (maybe it’s via a standard library or intrinsic). For Go, this is partly covered by go and channels; for C++ it’s in the library (std::thread).
    o Volatile/low-level: if a volatile qualifier exists (C/C++), define its semantics (e.g., prevents certain optimizations, etc.).
    o Pointer safety: any guarantees or lack thereof (dangling pointers lead to undefined behavior, etc., as discussed in the spec text).
    o Garbage Collection: if the language is garbage-collected (like Go, Odin, Zig are not GC except Go is; Go’s spec actually doesn’t detail the GC beyond implying one exists), you might just mention that the implementation manages memory automatically, and perhaps note that finalizers or destructors are handled by the GC if applicable.
58. Built-in Functions and Standard Library Overview –
    o List of built-in functions or keywords that act like functions (len, sizeof, etc.) with their description[93].
    o Mention of core library facilities (e.g., memory allocation, I/O) as a bridge to the library documentation. You might say “The standard library provides I/O routines, concurrency primitives, etc., which are outside the core language spec.”
    o Environment interaction: command-line arguments, environment variables, or other system interactions provided (often via library).
59. Appendices – Additional reference material:
    o Complete formal grammar of the language.
    o Summary of syntax or tables (operator precedence, keyword list, etc.).
    o Definition of undefined/implementation-defined behaviors (if any) for clarity (especially important in C/C++ style languages).
    o Example programs or patterns (some specs or reference manuals include non-normative examples as an appendix or within the text for illustration).
    Each of these sections should follow one another in a logical progression, as shown above. This outline is consistent with how languages like C and C++ present their standards (though C++ intermixes some topics due to its complexity), and how newer languages like Go and Zig structure their documentation. By adhering to this structure, you ensure that your language specification is comprehensive and aligns with readers’ expectations from prior art.
    References to Existing Specs for Guidance:
    Throughout this guide, we’ve cited examples from existing language documentation to illustrate the recommended structure. The Go specification is a great model for a clean, accessible spec: it introduces notation[3], then covers lexical matters[104], then moves through constants[105], types[106], statements, and so on, finishing with packages and the runtime[107]. Odin’s documentation (while more tutorial-like) shows an order of presenting variables, then literals, then control flow, then procedures, etc., which aligns with our outline[108][78]. Zig’s language reference, being one large document, still follows a similar logical sequence: basic syntax (comments, values) first[7][109], then types (ints, floats, arrays)[110][111], then more advanced concepts (structs, enums, error handling)[112][87].
    By studying and borrowing the common structure of these successful language specs, your own specification will be easier to navigate and will cover all necessary aspects of a systems programming language in a familiar, well-organized way. Following these common patterns and practices ensures that readers — whether they are compiler implementers, experienced programmers, or language enthusiasts — can quickly find information and understand the rules of your language.

---

[1] [2] [3] [4] [5] [9] [10] [11] [12] [15] [18] [19] [21] [22] [23] [28] [29] [30] [32] [33] [34] [35] [36] [39] [42] [43] [44] [45] [46] [47] [48] [49] [50] [58] [69] [70] [76] [81] [85] [86] [91] [92] [93] [101] [102] [103] [104] [105] [106] [107] The Go Programming Language Specification - The Go Programming Language
https://go.dev/ref/spec
[6] [13] [14] [16] [17] [25] [31] [37] [38] [40] [41] [52] [59] [60] [61] [62] [63] [64] [65] [66] [67] [68] [71] [72] [73] [74] [75] [77] [78] [79] [80] [82] [83] [84] [108] Overview | Odin Programming Language
https://odin-lang.org/docs/overview/
[7] [8] [20] [24] [26] [27] [53] [54] [55] [56] [57] [87] [88] [89] [90] [97] [98] [99] [100] [109] [110] [111] [112] Zig Documentation - W3cubDocs
http://docs.w3cub.com/zig/
[51] [94] [95] 14882: Contents
https://timsong-cpp.github.io/cppwp/std17/
[96] ANSI C Specification
https://eli-project.sourceforge.net/c_html/c.html
