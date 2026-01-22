# Cursive 0

- [Cursive 0](#cursive-0)
  - [0. Front Matter](#0-front-matter)
    - [0.1. Status and Scope (Bootstrap, Informative)](#01-status-and-scope-bootstrap-informative)
    - [0.2. Deviations from the Root Specification (Cursive0)](#02-deviations-from-the-root-specification-cursive0)
    - [0.3. Bootstrap Milestones and Equivalence](#03-bootstrap-milestones-and-equivalence)
    - [0.4. Document Conventions](#04-document-conventions)
  - [1. Foundations](#1-foundations)
    - [1.1. Conformance](#11-conformance)
    - [1.2. Behavior Types](#12-behavior-types)
    - [1.4. Unsupported Constructs Policy](#14-unsupported-constructs-policy)
    - [1.5. Target and ABI Assumptions](#15-target-and-abi-assumptions)
    - [1.6. Diagnostics Infrastructure](#16-diagnostics-infrastructure)
    - [1.7. Host Primitives](#17-host-primitives)
  - [2. Phase 0: Build/Project Model](#2-phase-0-buildproject-model)
    - [2.1. Project Root and Manifest](#21-project-root-and-manifest)
    - [2.2. Assemblies](#22-assemblies)
    - [2.3. Deterministic Ordering and Case Folding](#23-deterministic-ordering-and-case-folding)
    - [2.4. Module Discovery](#24-module-discovery)
    - [2.5. Output Artifacts and Linking](#25-output-artifacts-and-linking)
    - [2.6. Tool Resolution and IR Assembly Inputs](#26-tool-resolution-and-ir-assembly-inputs)
    - [2.7. Unwind and FFI Surface](#27-unwind-and-ffi-surface)
  - [3. Phase 1: Source Loading, Lexing, Parsing](#3-phase-1-source-loading-lexing-parsing)
    - [3.1. Source Loading and Normalization](#31-source-loading-and-normalization)
    - [3.2. Lexical Analysis](#32-lexical-analysis)
    - [3.3. Grammar, Parsing, and AST Construction](#33-grammar-parsing-and-ast-construction)
    - [3.4. Module Aggregation](#34-module-aggregation)
  - [4. Phase 2: Compile-Time Execution (Deferred in Cursive0)](#4-phase-2-compile-time-execution-deferred-in-cursive0)
  - [5. Phase 3: Name Resolution + Type Checking](#5-phase-3-name-resolution--type-checking)
    - [5.1. Name Resolution and Scopes (Cursive0)](#51-name-resolution-and-scopes-cursive0)
    - [5.2. Type System Core (Cursive0)](#52-type-system-core-cursive0)
    - [5.3. Classes and Record Methods (Cursive0)](#53-classes-and-record-methods-cursive0)
    - [5.4. Modal Types (Definitions)](#54-modal-types-definitions)
    - [5.5. State-Specific Fields](#55-state-specific-fields)
    - [5.6. Transitions and State-Specific Methods](#56-transitions-and-state-specific-methods)
    - [5.7. Modal Widening (`widen`)](#57-modal-widening-widen)
    - [5.8. String and Bytes Types and States](#58-string-and-bytes-types-and-states)
    - [5.9. Capabilities and Context (Cursive0)](#59-capabilities-and-context-cursive0)
    - [5.10. Enum Discriminant Defaults](#510-enum-discriminant-defaults)
    - [5.11. Foundational Classes (Cursive0)](#511-foundational-classes-cursive0)
    - [5.12. Initialization Planning](#512-initialization-planning)
  - [6. Phase 4: Code Generation](#6-phase-4-code-generation)
    - [6.0. Codegen Model and Judgments](#60-codegen-model-and-judgments)
    - [6.1. Layout and Representation](#61-layout-and-representation)
    - [6.2. ABI Lowering (Cursive0)](#62-abi-lowering-cursive0)
    - [6.3. Symbols, Mangling, and Linkage](#63-symbols-mangling-and-linkage)
    - [6.4. Expression Lowering and Evaluation Order](#64-expression-lowering-and-evaluation-order)
    - [6.5. Statement and Block Lowering](#65-statement-and-block-lowering)
    - [6.6. Pattern Matching Lowering](#66-pattern-matching-lowering)
    - [6.7. Globals and Initialization](#67-globals-and-initialization)
    - [6.8. Cleanup, Drop, and Unwinding](#68-cleanup-drop-and-unwinding)
    - [6.9. Built-ins Runtime Interface](#69-built-ins-runtime-interface)
    - [6.10. Dynamic Dispatch](#610-dynamic-dispatch)
    - [6.11. Checks and Panic](#611-checks-and-panic)
    - [6.12. LLVM 21 Backend Requirements](#612-llvm-21-backend-requirements)
  - [7. Dynamic Semantics](#7-dynamic-semantics)
    - [7.1. Initialization Order and Poisoning](#71-initialization-order-and-poisoning)
    - [7.2. Modal Layout (Dynamic Semantics)](#72-modal-layout-dynamic-semantics)
    - [7.3. Modal Pattern Matching](#73-modal-pattern-matching)
    - [7.4. Deterministic Destruction and Unwinding (Cursive0)](#74-deterministic-destruction-and-unwinding-cursive0)
    - [7.5. String Literal Semantics](#75-string-literal-semantics)
    - [7.6. Dynamic Class Objects](#76-dynamic-class-objects)
    - [7.7. FileSystem and File Operations](#77-filesystem-and-file-operations)
    - [7.8. Interpreter Entrypoint (Project-Level)](#78-interpreter-entrypoint-project-level)
  - [8. Appendix A - Diagnostic Codes](#8-appendix-a---diagnostic-codes)
    - [8.0. DiagIdâ€“Code Map](#80-diagidâcode-map)
    - [8.1. E-PRJ (Project)](#81-e-prj-project)
    - [8.2. E-MOD (Module)](#82-e-mod-module)
    - [8.3. E-OUT (Output and Linking)](#83-e-out-output-and-linking)
    - [8.4. E-SRC (Source)](#84-e-src-source)
    - [8.5. E-CNF (Conformance / Limits)](#85-e-cnf-conformance--limits)
    - [8.6. E-UNS (Unsupported Constructs)](#86-e-uns-unsupported-constructs)
    - [8.7. E-MEM (Memory)](#87-e-mem-memory)
    - [8.8. W-MOD (Module Warnings)](#88-w-mod-module-warnings)
    - [8.9. W-SRC (Source Warnings)](#89-w-src-source-warnings)
    - [8.10. E-TYP (Types)](#810-e-typ-types)
    - [8.11. W-SYS (System Warnings)](#811-w-sys-system-warnings)
    - [8.12. E-SEM (Semantics)](#812-e-sem-semantics)
    - [8.13. W-SEM (Semantic Warnings)](#813-w-sem-semantic-warnings)
  - [9. Appendix B - Notation and Glossary](#9-appendix-b---notation-and-glossary)
    - [9.1. Notation Conventions](#91-notation-conventions)
    - [9.2. Helper Functions and Relations](#92-helper-functions-and-relations)

## 0. Front Matter

### 0.1. Status and Scope (Bootstrap, Informative)

**Phase.**
Phase = {Phase0, Phase1, Phase2, Phase3, Phase4}

**PhaseStatus.**
PhaseStatus = {Implemented, Deferred, InProgress}

**PhaseStatusMap.**
PhaseStatus(Phase0) = Implemented
PhaseStatus(Phase1) = Implemented
PhaseStatus(Phase2) = Deferred
PhaseStatus(Phase3) = InProgress
PhaseStatus(Phase4) = InProgress

**PhaseSection.**
PhaseSection(Phase0) = 2
PhaseSection(Phase1) = 3
PhaseSection(Phase2) = 4
PhaseSection(Phase3) = 5
PhaseSection(Phase4) = 6

### 0.2. Deviations from the Root Specification (Cursive0)

**DeviationId.**
DeviationId = {D_BootstrapEquivalence, D_SourceNormalization, D_ModuleOrdering, D_KeywordReservation, D_GenericTokenization, D_UnsafeSpanClassification, D_GroupingTrailingCommas, D_UnsupportedGrammarFamilies, D_SingleAssemblyVisibility, D_OverloadingScope, D_Permissions, D_ParamPassing, D_PointerAddressOf, D_RegionOptionsSyntax, D_TypeInference, D_RecordUpdate, D_RangeExpressions, D_RangePatterns, D_FieldVisibilityDefault, D_EnumDiscriminantControls, D_UnionLayout, D_LayoutAttributes, D_CallingConventionToolchain, D_SymbolVisibilityMechanism, D_FileSystemSemantics}

- `System` in Cursive0 omits `time()` and `get_env` returns `string | ()`; this is a bootstrap restriction.

**DeviationRef.**
DeviationRef(D_BootstrapEquivalence) = {"0.3.2"}
DeviationRef(D_SourceNormalization) = {"3.1.1"}
DeviationRef(D_ModuleOrdering) = {"2.3.1", "2.3.2", "2.3.3"}
DeviationRef(D_KeywordReservation) = {"3.2.3"}
DeviationRef(D_GenericTokenization) = {"3.2.9"}
DeviationRef(D_UnsafeSpanClassification) = {"3.2.12"}
DeviationRef(D_GroupingTrailingCommas) = {"3.3.4"}
DeviationRef(D_UnsupportedGrammarFamilies) = {"3.3.2.7"}
DeviationRef(D_SingleAssemblyVisibility) = {"5.1.4"}
DeviationRef(D_OverloadingScope) = {"5.1.5", "5.3"}
DeviationRef(D_Permissions) = {"1.1.1", "5.2.2"}
DeviationRef(D_ParamPassing) = {"5.2.4", "5.3.2", "5.2.15"}
DeviationRef(D_PointerAddressOf) = {"1.1.1", "5.2.12", "5.2.16"}
DeviationRef(D_RegionOptionsSyntax) = {"3.3.4", "5.2.17"}
DeviationRef(D_TypeInference) = {"5.2.9"}
DeviationRef(D_RecordUpdate) = {"3.3.4"}
DeviationRef(D_RangeExpressions) = {"5.2.12"}
DeviationRef(D_RangePatterns) = {"5.2.13"}
DeviationRef(D_FieldVisibilityDefault) = {"5.2.2", "5.3.2"}
DeviationRef(D_EnumDiscriminantControls) = {"5.10", "6.1.4"}
DeviationRef(D_UnionLayout) = {"6.1.4", "6.1.4.1"}
DeviationRef(D_LayoutAttributes) = {"6.1.3"}
DeviationRef(D_CallingConventionToolchain) = {"6.2.1", "6.2.3"}
DeviationRef(D_SymbolVisibilityMechanism) = {"6.3.4"}
DeviationRef(D_FileSystemSemantics) = {"7.7"}

### 0.3. Bootstrap Milestones and Equivalence

#### 0.3.1. Bootstrap Milestones and Invariants

**BootstrapCompiler.**
BootstrapCompiler = {cursivec0, cursivec1, cursivec2}

**BootstrapName.**
BootstrapName(cursivec0) = "cursivec0"
BootstrapName(cursivec1) = "cursivec1"
BootstrapName(cursivec2) = "cursivec2"

**BootstrapImpl.**
BootstrapImpl(cursivec0) = {"C++", "LLVM"}
BootstrapImpl(cursivec1) = {"Cursive"}
BootstrapImpl(cursivec2) = {"Cursive"}

**CompilerSourceProject.**
CompilerSource ∈ Project

**ProducesCompiler.**
ProducesCompiler : BootstrapCompiler × Project ⇀ BootstrapCompiler

**Milestone Invariants.**
M0 ⇔ Status(cursivec0, CompilerSource) = ok ∧ Subset(CompilerSource, S0)
M1 ⇔ Status(cursivec1, CompilerSource) = ok ∧ ProducesCompiler(cursivec1, CompilerSource) = cursivec2
M2 ⇔ ∀ P ∈ Project. BootstrapEq(cursivec1, cursivec2, P)

#### 0.3.2. Observable Behavior Equivalence for Bootstrap

**Compiler Observable Behavior.**

Under(p, O) ⇔ prefix(Normalize(p), Normalize(O))
IsFile(p) ⇔ FSKind(p) = File

DiagObs(d) = ⟨d.code, d.severity, d.message, d.span⟩
DiagStream(C, P) = [DiagObs(d_1), …, DiagObs(d_k)]
Status(C, P) = ok ⇔ ∀ d ∈ DiagStream(C, P). d.severity ≠ Error
Status(C, P) = fail ⇔ ∃ d ∈ DiagStream(C, P). d.severity = Error
ExitCode(C, P) = 0 ⇔ Status(C, P) = ok
ExitCode(C, P) = 1 ⇔ Status(C, P) = fail
Executable(P) ⇔ P.assembly.kind = `executable`
IRSet(P) = {IRPath(P, m, e) | m ∈ ModuleList(P)} if P.assembly.emit_ir = e ∈ {"ll", "bc"}
IRSet(P) = ∅ if P.assembly.emit_ir ∉ {"ll", "bc"}
ExeSet(P) =
 {ExePath(P)}  if Executable(P)
 ∅             otherwise
RequiredOutputs(P) = {ObjPath(P, m) | m ∈ ModuleList(P)} ∪ IRSet(P) ∪ ExeSet(P)
Artifacts(C, P) = RequiredOutputs(P) ⇔ Status(C, P) = ok
Artifacts(C, P) = ∅ ⇔ Status(C, P) = fail

ObsComp(C, P) = ⟨Status(C, P), ExitCode(C, P), DiagStream(C, P), Artifacts(C, P)⟩

**Bootstrap Equivalence.**
BootstrapEq(C_a, C_b, P) ⇔ ObsComp(C_a, P) = ObsComp(C_b, P) ∧ (Status(C_a, P) = ok ⇒ Artifacts(C_a, P) = RequiredOutputs(P))

### 0.4. Document Conventions

**NormativeKeywords.**
NormativeKeywords = {`MUST`, `MUST NOT`, `SHOULD`, `SHOULD NOT`, `MAY`}

**DocScope.**
DocScope = {ConformanceTarget, SupportedSubset, RequiredBehavior(`cursivec0`)}

**DiagnosticCodeFormat.**
DiagPrefix = {E, W, I}
DiagCategory = [A-Z]^3
DiagDigits = [0-9]^4
DiagCode = DiagPrefix ++ "-" ++ DiagCategory ++ "-" ++ DiagDigits
Bucket(Digits) = Digits[0..1]
Seq(Digits) = Digits[2..3]

## 1. Foundations

### 1.1. Conformance

**C0Conforming.**
C0Conforming(P) ⇔ WF(P) ∧ Subset(P, S0)

**WF.**
WF(P) ⇔ ∃ Γ. Project(Γ) = P ∧ ∀ j ∈ ReqJudgments(P). Γ ⊢ j ⇓ ok

**ReqJudgments.**
ReqJudgments(P) = [Phase1Order(P), Phase3Order(P), Phase4Order(P)]

**Phase1Order.**
Γ ⊢ Phase1Order(P) ⇓ ok ⇔ ∃ Ms. Γ ⊢ ParseModules(P) ⇓ Ms

**Phase4Order.**
Γ ⊢ Phase4Order(P) ⇓ ok ⇔ ∃ Objs, IRs, Exe. Γ ⊢ OutputPipeline(P) ⇓ (Objs, IRs, Exe)

**Constructs.**
TypeNodes(P, m) = { t | t ∈ Type ∧ Subnode(ASTModule(P, m), t) }
StmtNodes(P, m) = { s | s ∈ Stmt ∧ Subnode(ASTModule(P, m), s) }

ItemKind(UsingDecl(_)) = `using_decl`
ItemKind(ProcedureDecl(_, _, _, _, _, _, _)) = `procedure`
ItemKind(RecordDecl(_, _, _, _, _, _)) = `record`
ItemKind(EnumDecl(_, _, _, _, _, _)) = `enum`
ItemKind(ModalDecl(_, _, _, _, _, _)) = `modal`
ItemKind(ClassDecl(_, _, _, _, _)) = `class`
ItemKind(TypeAliasDecl(_, _, _, _, _)) = `type_alias`
ItemKind(StaticDecl(_, _, _, _, _)) = `static_decl`
ItemKind(_) = ⊥

TopDeclConstructs(P) = { ItemKind(it) | m ∈ P.modules ∧ it ∈ ASTModule(P, m).items ∧ ItemKind(it) ≠ ⊥ }

TypeCtor(TypePerm(_, base)) = TypeCtor(base)
TypeCtor(TypePrim(name)) = {name}
TypeCtor(TypeTuple(elems)) = {`tuple`} ∪ ⋃_{t ∈ elems} TypeCtor(t)
TypeCtor(TypeArray(elem, _)) = {`array`} ∪ TypeCtor(elem)
TypeCtor(TypeSlice(elem)) = {`slice`} ∪ TypeCtor(elem)
TypeCtor(TypeUnion(members)) = {`union`} ∪ ⋃_{t ∈ members} TypeCtor(t)
TypeCtor(TypeFunc(params, ret)) = {`function`} ∪ ⋃_{⟨_, t⟩ ∈ params} TypeCtor(t) ∪ TypeCtor(ret)
TypeCtor(TypePtr(elem, _)) = {`ptr`} ∪ TypeCtor(elem)
TypeCtor(TypeRawPtr(_, elem)) = {`rawptr`} ∪ TypeCtor(elem)
TypeCtor(TypeString(_)) = {`string`}
TypeCtor(TypeBytes(_)) = {`bytes`}
TypeCtor(TypeDynamic(_)) = {`dyn_class`}
TypeCtor(TypeModalState(_, _)) = {`modal`}
TypeCtor(TypePath(["Region"])) = {`region`}
TypeCtor(TypePath(["RegionOptions"])) = {`region_options`}
TypeCtor(TypePath(p)) = {`record`} if RecordDecl(p) defined
TypeCtor(TypePath(p)) = {`enum`} if EnumDecl(p) defined
TypeCtor(_) = ∅

TypeConstructs(P) = ⋃_{m ∈ P.modules} ⋃_{t ∈ TypeNodes(P, m)} TypeCtor(t)

PermOfType(TypePerm(p, _)) = {p}
PermOfType(_) = ∅
RecvPerms(members) = { p | ∃ vis, ov, name, recv, params, ret, body, span, doc. MethodDecl(vis, ov, name, recv, params, ret, body, span, doc) ∈ members ∧ recv = ReceiverShorthand(p) }
ClassRecvPerms(items) = { p | ∃ vis, name, recv, params, ret, body, span, doc. ClassMethodDecl(vis, name, recv, params, ret, body, span, doc) ∈ items ∧ recv = ReceiverShorthand(p) }
PermConstructs(P) = ⋃_{m ∈ P.modules} ⋃_{t ∈ TypeNodes(P, m)} PermOfType(t) ∪ ⋃_{m ∈ P.modules} ⋃_{RecordDecl(_, _, _, members, _, _) ∈ ASTModule(P, m).items} RecvPerms(members) ∪ ⋃_{m ∈ P.modules} ⋃_{ClassDecl(_, _, _, items, _) ∈ ASTModule(P, m).items} ClassRecvPerms(items)

ExprKind(Literal(_)) = `literal`
ExprKind(Identifier(_)) = `identifier`
ExprKind(FieldAccess(_, _)) = `field_access`
ExprKind(TupleAccess(_, _)) = `tuple_index`
ExprKind(IndexAccess(_, _)) = `index`
ExprKind(IfExpr(_, _, _)) = `if`
ExprKind(MatchExpr(_, _)) = `match`
ExprKind(LoopInfinite(_)) = `loop`
ExprKind(LoopConditional(_, _)) = `loop`
ExprKind(LoopIter(_, _, _, _)) = `loop`
ExprKind(MoveExpr(_)) = `move`
ExprKind(Unary(`"widen"`, _)) = `widen`
ExprKind(TransmuteExpr(_, _, _)) = `transmute`
ExprKind(UnsafeBlockExpr(_)) = `unsafe`
ExprKind(AllocExpr(_, _)) = `region_alloc`
ExprKind(MethodCall(_, _, _)) = `method_call`
ExprKind(Propagate(_)) = `union_propagate`
ExprKind(_) = ⊥

StmtKind(LetStmt(_)) = `let`
StmtKind(VarStmt(_)) = `var`
StmtKind(ShadowLetStmt(_, _, _)) = `shadow`
StmtKind(ShadowVarStmt(_, _, _)) = `shadow`
StmtKind(AssignStmt(_, _)) = `assign`
StmtKind(CompoundAssignStmt(_, _, _)) = `compound_assign`
StmtKind(DeferStmt(_)) = `defer`
StmtKind(RegionStmt(_, _, _)) = `region`
StmtKind(FrameStmt(_, _)) = `frame`
StmtKind(ReturnStmt(_)) = `return`
StmtKind(ResultStmt(_)) = `result`
StmtKind(BreakStmt(_)) = `break`
StmtKind(ContinueStmt) = `continue`
StmtKind(UnsafeBlockStmt(_)) = `unsafe`
StmtKind(_) = ⊥

ExprStmtConstructs(P) = { ExprKind(e) | m ∈ P.modules ∧ e ∈ ExprNodes(P, m) ∧ ExprKind(e) ≠ ⊥ } ∪ { StmtKind(s) | m ∈ P.modules ∧ s ∈ StmtNodes(P, m) ∧ StmtKind(s) ≠ ⊥ }

CapConstructs(P) = { c | c ∈ {`Context`, `FileSystem`, `HeapAllocator`} ∧ ∃ m, t. m ∈ P.modules ∧ t ∈ TypeNodes(P, m) ∧ t = TypePath([c]) }

Constructs(P) = TopDeclConstructs(P) ∪ TypeConstructs(P) ∪ PermConstructs(P) ∪ ExprStmtConstructs(P) ∪ CapConstructs(P)

**Subset.**
Subset(P, S0) ⇔ Constructs(P) ⊆ S0

**(Reject-IllFormed)**
¬ C0Conforming(P)
─────────────────
Γ ⊢ Reject(P)

**TranslationPhases.**

TranslationPhases = [Phase1, Phase2, Phase3, Phase4]

#### 1.1.1. Cursive0 Subset (S0)

S0 = S_Lex ∪ S_Modules ∪ S_TopDecl ∪ S_Types ∪ S_Perms ∪ S_ExprStmt ∪ S_Caps

**S_Lex.**
S_Lex = RulesIn({"3.1", "3.2", "3.3"})

**S_Modules.**
S_Modules = RulesIn({"2", "3.3.6.3", "5.1"})

**S_TopDecl.**
S_TopDecl = {`using_decl`, `procedure`, `record`, `enum`, `modal`, `class`, `type_alias`, `static_decl`}

**S_Types.**
PrimTypes_C0 = IntTypes ∪ FloatTypes ∪ {`bool`, `char`, `()`, `!`}
TypeCtors_C0 = {`tuple`, `array`, `slice`, `record`, `enum`, `union`, `function`, `ptr`, `rawptr`, `string`, `bytes`, `region_options`, `region`, `dyn_class`}
S_Types = PrimTypes_C0 ∪ TypeCtors_C0

**S_Perms.**
S_Perms = PermSet_C0

**S_ExprStmt.**
S_ExprStmt = {`literal`, `identifier`, `field_access`, `tuple_index`, `index`, `if`, `loop`, `match`, `break`, `continue`, `return`, `result`, `defer`, `region`, `frame`, `union_propagate`, `let`, `var`, `shadow`, `assign`, `compound_assign`, `move`, `widen`, `transmute`, `unsafe`, `region_alloc`, `method_call`}

**S_Caps.**
S_Caps = {`Context`, `FileSystem`, `HeapAllocator`}
  
**PermSet (Cursive0).**
PermSet_C0 = {`const`, `unique`}

**(Perm-Shared-Unsupported)**
PermSyntax ∈ {`shared`, `~%`}    c = Code(Perm-Shared-Unsupported)
───────────────────────────────────────────────────────────────
Γ ⊢ Emit(c)

**Subset Lexeme Basis.**
Let S be a source file and let K satisfy Γ ⊢ Tokenize(S) ⇓ (K, _).
Any use of PermSyntax or UnsupportedForm MUST be based on token lexemes in K (and, where specified elsewhere, the AST produced by ParseFile(S)); implementations MUST NOT match substrings inside identifiers.
See Â§3.2.2 and Â§3.2.7.

S0Unsupported = {`derive`, `extern`, `attribute`, `import`, `opaque_type`, `refinement_type`, `closure`, `pipeline`, `async`, `parallel`, `dispatch`, `spawn`, `metaprogramming`, `Network`, `Reactor`, `GPUFactory`, `CPUFactory`, `AsyncRuntime`}

### 1.2. Behavior Types

**BehaviorClass.**
BehaviorClass = {Specified, UVB}

UVBRel = {ReadPtrSigma(RawPtr(q, addr), σ), WritePtrSigma(RawPtr(q, addr), v, σ)}

**IllFormed.**
StaticJudgSet = WFModulePathJudg ∪ LinkJudg ∪ ParseJudgment ∪ ResolvePathJudg ∪ ResolveExprListJudg ∪ ResolveEnumPayloadJudg ∪ ResolveCalleeJudg ∪ ResolveArmJudg ∪ ResolveStmtSeqJudg ∪ TypeEqJudg ∪ ConstLenJudg ∪ SubtypingJudg ∪ PermSubJudg ∪ ArgsOkTJudg ∪ TypeInfJudg ∪ StmtJudg ∪ PatJudg ∪ ExprJudg ∪ MatchJudg ∪ DeclJudg ∪ BJudgment ∪ ArgPassJudg ∪ ProvPlaceJudg ∪ ProvExprJudg ∪ ProvStmtJudg ∪ BlockProvJudg ∪ ArgsOkJudg ∪ TypeWFJudg ∪ StringBytesJudg ∪ BitcopyDropJudg ∪ TypeRefsJudg ∪ ValueRefsJudg ∪ CodegenJudg ∪ LayoutJudg ∪ EncodeConstJudg ∪ ValidValueJudg ∪ RecordLayoutJudg ∪ UnionLayoutJudg ∪ TupleLayoutJudg ∪ RangeLayoutJudg ∪ EnumLayoutJudg ∪ ModalLayoutJudg ∪ DynLayoutJudg ∪ ABITyJudg ∪ ABIParamJudg ∪ ABIRetJudg ∪ ABICallJudg ∪ LowerCallJudg ∪ MangleJudg ∪ LinkageJudg ∪ EvalOrderJudg ∪ LowerExprJudg ∪ LowerStmtJudg ∪ PatternLowerJudg ∪ LowerBindJudg ∪ GlobalsJudg ∪ ConstInitJudg ∪ CleanupJudg ∪ RuntimeIfcJudg ∪ DynDispatchJudg ∪ ChecksJudg ∪ LLVMAttrJudg ∪ RuntimeDeclJudg ∪ LLVMTyJudg ∪ LLVMEmitJudg ∪ LowerIRJudg ∪ BindStorageJudg ∪ LLVMCallJudg ∪ VTableJudg ∪ LiteralEmitJudg ∪ BuiltinSymJudg ∪ DropHookJudg ∪ EntryJudg ∪ PoisonJudg
StaticRuleSet = { r | Conclusion(r) ∈ StaticJudgSet }
Conclusion(r) = J    (r is written (π_1 … π_k) / J)
Premises(r) = [π_1, …, π_k]    (r is written (π_1 … π_k) / _)
Subject(Γ ⊢ j) = j_0 where j_0 is the leftmost term to the right of ⊢
EnvOf(Γ ⊢ j) = Γ
θ ranges over substitutions of metavariables in r
Applies(r, x) ⇔ ∃ θ. Subject(Conclusion(r)[θ]) = x
PremisesHold(r, x) ⇔ ∃ θ. Subject(Conclusion(r)[θ]) = x ∧ Γ_r = EnvOf(Conclusion(r)[θ]) ∧ ∀ π ∈ Premises(r)[θ]. π ≠ ⊥ ∧ (π is a judgment ⇒ Γ_r ⊢ π)
IllFormed(x) ⇔ ∃ r ∈ StaticRuleSet. Applies(r, x) ∧ ¬ PremisesHold(r, x)

**Undefinedness Policy.**
StaticUndefined(J) ⇔ ∃ r. Conclusion(r) = J ∧ ∃ π ∈ Premises(r). π = ⊥
DynamicUndefined(R) ⇔ R ∈ UVBRel ∧ R undefined
Behavior(R) = Specified ⇔ ¬ DynamicUndefined(R)
Behavior(R) = UVB ⇔ DynamicUndefined(R)

RuleId(r) = id ⇔ r is labeled (id)
DiagIdOf(J) = id ⇔ ∃ r. Conclusion(r) = J ∧ RuleId(r) = id
DiagIdOf(J) = ⊥ ⇔ ¬ ∃ r. Conclusion(r) = J ∧ RuleId(r) defined

**(Static-Undefined)**
StaticUndefined(J)    Code(DiagIdOf(J)) = c
───────────────────────────────────────
Γ ⊢ J ⇑ c

**(Static-Undefined-NoCode)**
StaticUndefined(J)    Code(DiagIdOf(J)) = ⊥
────────────────────────────────────────
Γ ⊢ J ⇑

**(Dynamic-Undefined-UVB)**
DynamicUndefined(R)
────────────────────────────
Γ ⊢ Behavior(R) = UVB

**Static vs. Runtime Checks.**

CheckKind = {PatternExhaustiveness, TypeCompatibility, PermissionViolations, ProvenanceEscape, ArrayBounds, SafePointerValidity, IntegerOverflow, SliceBounds, IntDivisionByZero}

StaticCheck = {PatternExhaustiveness, TypeCompatibility, PermissionViolations, ProvenanceEscape, ArrayBounds, SafePointerValidity}
RuntimeCheck = {IntegerOverflow, SliceBounds, IntDivisionByZero}

RuntimeBehavior(IntegerOverflow) = Panic
RuntimeBehavior(SliceBounds) = Panic
RuntimeBehavior(IntDivisionByZero) = Panic

ResourceExhaustion ⇒ OutsideConformance

**Error Recovery (Cursive0).**
LexRecovery = SkipToNextTokenStart
ParseRecovery = SyncSet({`;`, `}`, `EOF`})
TypeRecovery = ContinueDecls
MaxErrorCount ∈ ℕ ∪ {∞}
SuggestedMaxErrorCount = 100
AbortOnErrorCount(n) ⇔ n ≥ MaxErrorCount

### 1.4. Unsupported Constructs Policy

**UnsupportedConstruct.**
UnsupportedConstruct = {`key_system`, `attribute_syntax`, `extern_block`, `foreign_decl`, `class_generics`, `class_where_clause`, `associated_type`, `modal_class`, `class_contract`}

**(WF-Attr-Unsupported)**
`[[...]]` ∈ M
───────────────────────────────────────────────
Γ ⊢ Emit(Code(WF-Attr-Unsupported))

UnsupportedForm = UnsupportedConstruct ∪ S0Unsupported ∪ UnsupportedGrammarFamily ∪ UnsupportedClassItem ∪ UnsupportedWhereClause ∪ ComptimeForm

**(Unsupported-Construct)**
f ∈ UnsupportedForm
──────────────────────────────────────────────
Γ ⊢ Emit(Code(Unsupported-Construct))

### 1.5. Target and ABI Assumptions

TargetArch = Win64
Endianness = Little
PtrSizeBytes = PtrSize
LayoutSpec = RulesIn({"6.1.1", "6.1.2", "6.1.3", "6.1.4", "6.1.5", "6.1.6"})

Target = "x86_64-pc-windows-msvc"
Win64 = Target

### 1.6. Diagnostics Infrastructure

#### 1.6.1. Source Locations and Spans

**SourceLocation.**

SourceLocation = ⟨file, offset, line, column⟩

**Span.**

Span = ⟨file, start_offset, end_offset, start_line, start_col, end_line, end_col⟩

SpanRange(sp) = [sp.start_offset, sp.end_offset)

**(WF-Location)**
0 ≤ o    Γ ⊢ Locate(S, o) ⇓ ℓ_loc
────────────────────────────────
Γ ⊢ ℓ_loc : LocationOk

**(WF-Span)**
0 ≤ s ≤ e ≤ S.byte_len    Γ ⊢ Locate(S, s) ⇓ ℓ_s    Γ ⊢ Locate(S, e) ⇓ ℓ_e
──────────────────────────────────────────────────────────────────────────
Γ ⊢ ⟨S.path, s, e, ℓ_s.line, ℓ_s.column, ℓ_e.line, ℓ_e.column⟩ : SpanOk

**Span Construction**

ClampSpan(S, s, e) = (s', e')
s' = min(s, S.byte_len)
e' = min(max(e, s'), S.byte_len)

**(Span-Of)**
Γ ⊢ ClampSpan(S, s, e) ⇓ (s', e')    Γ ⊢ ⟨S.path, s', e', line_s, col_s, line_e, col_e⟩ : SpanOk
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ SpanOf(S, s, e) ⇓ ⟨S.path, s', e', line_s, col_s, line_e, col_e⟩

#### 1.6.2. Token Spans

**TokenKind.**

TokenKind_C0 = TokenKind_(§ 3.2.4) ∪ {Unknown}

**(No-Unknown-Ok)**
∀ t ∈ K. t.kind ≠ Unknown
─────────────────────────
Γ ⊢ K : TokenStreamOk

**RawToken.**

RawToken = ⟨kind, lexeme, s, e⟩

**Token.**

Token = ⟨kind, lexeme, span⟩

**(Attach-Token-Ok)**
Γ ⊢ SpanOf(S, s, e) ⇓ sp
───────────────────────────────────────────────
Γ ⊢ AttachSpan(S, ⟨k, ℓ, s, e⟩) ⇓ ⟨k, ℓ, sp⟩

**Token Stream Attachment (Bigâ€‘Step)**

**(Attach-Tokens-Ok)**
∀ r ∈ rs, Γ ⊢ AttachSpan(S, r) ⇓ t    ts = [t | r ∈ rs]
──────────────────────────────────────────────────────────────
Γ ⊢ AttachSpans(S, rs) ⇓ ts

#### 1.6.3. Diagnostics: Records and Emission

**Diagnostic.**

Severity = {Error, Warning}

**Diagnostic Stream.**
Δ = [d_1, …, d_n]

**(Emit-Append)**
────────────────────────────────────────────
Γ ⊢ Emit(Δ, d) ⇓ (Δ ++ [d])

**Emit (Implicit).**
Emit(c) = Emit(Δ, ⟨c, Severity(c), Message(c), ⊥⟩)
Emit(c, sp) = Emit(Δ, ⟨c, Severity(c), Message(c), sp⟩)

Severity(c) = SeverityColumn(c)
Message(c) = ConditionColumn(c)

CompileStatus(Δ) =
 fail  if HasError(Δ)
 ok    otherwise

#### 1.6.4. Diagnostic Code Selection

SpecCode : DiagId ⇀ DiagCode
SpecCode(id) = ⊥
C0Code : DiagId ⇀ DiagCode

**(Code-Spec)**
SpecCode(id) = c
────────────────────────────
Γ ⊢ Code(id) ⇓ c

**(Code-C0)**
SpecCode(id) = ⊥    C0Code(id) = c
────────────────────────────────
Γ ⊢ Code(id) ⇓ c

**DiagIdâ€“Code Mapping.**
id emits a diagnostic
────────────────────────────
Γ ⊢ Emit(Code(id))
⇑ ≡ ⇑ Code(id)

**Resolution Failure.**
NoDiag(↑)

#### 1.6.5. Diagnostic Ordering

**(Order)**
Δ = [d_1, d_2, …, d_n]
──────────────────────────────────────────────────
Γ ⊢ Order(Δ) ⇓ [d_1, d_2, …, d_n]

#### 1.6.6. Diagnostic Rendering

Render(d) =
 code ++ " (" ++ sev ++ ")" ++ msg ++ " @" ++ loc  if d.span ≠ ⊥
 code ++ " (" ++ sev ++ ")" ++ msg                if d.span = ⊥

code = d.code
sev =
 "error"   if d.severity = Error
 "warning" if d.severity = Warning
msg =
 "\""      if d.message = "\""
 ": " ++ d.message  otherwise
loc = d.span.file ++ ":" ++ d.span.start_line ++ ":" ++ d.span.start_col
PermLexeme(const) = "const"
PermLexeme(unique) = "unique"
PermLexeme(shared) = "shared"
QualLexeme(imm) = "imm"
QualLexeme(mut) = "mut"
PtrStateSuffix(⊥) = ""
PtrStateSuffix(Valid) = "@Valid"
PtrStateSuffix(Null) = "@Null"
PtrStateSuffix(Expired) = "@Expired"
StringStateSuffix(⊥) = ""
StringStateSuffix(View) = "@View"
StringStateSuffix(Managed) = "@Managed"
BytesStateSuffix(⊥) = ""
BytesStateSuffix(View) = "@View"
BytesStateSuffix(Managed) = "@Managed"
ParamRender(⟨⊥, T⟩) = TypeRender(T)
ParamRender(⟨move, T⟩) = "move " ++ TypeRender(T)
TypeRender(TypePrim(name)) = name
TypeRender(TypeRange) = "TypeRange"
TypeRender(TypePerm(p, T)) = PermLexeme(p) ++ " " ++ TypeRender(T)
TypeRender(TypeUnion([T_1, …, T_n])) = Join(" | ", [TypeRender(T_1), …, TypeRender(T_n)])
TypeRender(TypeFunc([⟨m_1, T_1⟩, …, ⟨m_n, T_n⟩], R)) = "(" ++ Join(", ", [ParamRender(⟨m_1, T_1⟩), …, ParamRender(⟨m_n, T_n⟩)]) ++ ") -> " ++ TypeRender(R)
TypeRender(TypeTuple([])) = "()"
TypeRender(TypeTuple([T])) = "(" ++ TypeRender(T) ++ ";)"
TypeRender(TypeTuple([T_1, …, T_n])) = "(" ++ Join(", ", [TypeRender(T_1), …, TypeRender(T_n)]) ++ ")"
TypeRender(TypeArray(T, e)) = "[" ++ TypeRender(T) ++ "; " ++ ArrayLen(e) ++ "]"
TypeRender(TypeSlice(T)) = "[" ++ TypeRender(T) ++ "]"
TypeRender(TypePtr(T, s)) = "Ptr<" ++ TypeRender(T) ++ ">" ++ PtrStateSuffix(s)
TypeRender(TypeRawPtr(q, T)) = "* " ++ QualLexeme(q) ++ " " ++ TypeRender(T)
TypeRender(TypeString(st)) = "string" ++ StringStateSuffix(st)
TypeRender(TypeBytes(st)) = "bytes" ++ BytesStateSuffix(st)
TypeRender(TypeDynamic(p)) = "$" ++ StringOfPath(p)
TypeRender(TypeModalState(p, S)) = StringOfPath(p) ++ "@" ++ S
TypeRender(TypePath(p)) = StringOfPath(p)

#### 1.6.7. Diagnostics without Source Spans

**(NoSpan-External)**
Origin(d) = External
──────────────────────
Γ ⊢ d.span = ⊥

### 1.7. Host Primitives

FSPrim = {FSOpenRead, FSOpenWrite, FSOpenAppend, FSCreateWrite, FSReadFile, FSReadBytes, FSWriteFile, FSWriteStdout, FSWriteStderr, FSExists, FSRemove, FSOpenDir, FSCreateDir, FSEnsureDir, FSKind, FSRestrict}
FilePrim = {FileReadAll, FileReadAllBytes, FileWrite, FileFlush, FileClose}
DirPrim = {DirNext, DirClose}

HostPrim = {ParseTOML, ReadBytes, WriteFile, ResolveTool, ResolveRuntimeLib, Invoke, AssembleIR, InvokeLinker} ∪ FSPrim ∪ FilePrim ∪ DirPrim
HostPrimDiag = {ParseTOML, ReadBytes, WriteFile, ResolveTool, ResolveRuntimeLib, Invoke, AssembleIR, InvokeLinker}
HostPrimRuntime = FSPrim ∪ FilePrim ∪ DirPrim
MapsToDiagOrRuntime(p) ⇔ p ∈ HostPrimDiag ∪ HostPrimRuntime
HostPrimFail(p) ⇔ p ∈ HostPrim ∧ ∃ args. Γ ⊢ p(args) ⇑

HostPrimFail(p) ∧ ¬ MapsToDiagOrRuntime(p) ⇒ IllFormed(p)

## 2. Phase 0: Build/Project Model

**Assembly Kind.**
AssemblyKind = {`executable`, `library`}

**Assembly Record.**
Assembly = ⟨name, kind, root, out_dir, emit_ir, source_root, outputs, modules⟩

**Project Record.**
Project = ⟨root, assemblies, assembly, source_root, outputs, modules⟩
Assemblies(P) = P.assemblies
Assembly(P) = P.assembly
AsmNames(P) = [A.name | A ∈ Assemblies(P)]
AsmByName(P, n) = A ⇔ A ∈ Assemblies(P) ∧ A.name = n ∧ (∀ B ∈ Assemblies(P). B.name = n ⇒ B = A)

**Build/Project Validation Scope.**
Phase0Checks = RulesIn({"2"})
SourceChecks = RulesIn({"3", "4", "5", "6"})
Phase0Checks ∩ SourceChecks = ∅

**Command-Line Output.**
DumpProject(P, dump) =
 ProjectSummary(P) ++ OutputSummary(P)  if dump = false
 ProjectSummary(P) ++ OutputSummary(P) ++ ["file:" ++ f | d ∈ Modules(P.source_root), f ∈ CompilationUnit(d)]  if dump = true

ProjectSummary(P) = [⟨`project_root`, P.root⟩, ⟨`assemblies`, AsmNames(P)⟩, ⟨`assembly_name`, P.assembly.name⟩, ⟨`source_root`, P.source_root⟩, ⟨`output_root`, OutputRoot(P)⟩, ⟨`module_list`, ModuleList(P)⟩]

OutputSummary(P) = [⟨`module`, m, `obj`, ObjPath(P, m), `ir`, IROpt(P, m)⟩ | m ∈ ModuleList(P)]

IROpt(P, m) =
 IRPath(P, m, P.assembly.emit_ir)  if P.assembly.emit_ir ≠ `none`
 ⊥                                if P.assembly.emit_ir = `none`

### 2.1. Project Root and Manifest

**Manifest Parsing (Big-Step)**

ParseTOML : Path ⇀ TOMLTable

**(Parse-Manifest-Ok)**
ParseTOML(R/`Cursive.toml`) ⇓ T
────────────────────────────────
Γ ⊢ ParseManifest(R) ⇓ T

**(Parse-Manifest-Missing)**
¬ exists(R/`Cursive.toml`)    c = Code(Parse-Manifest-Missing)
──────────────────────────────────────────────────────────────
Γ ⊢ ParseManifest(R) ⇑ c

**(Parse-Manifest-Err)**
ParseTOML(R/`Cursive.toml`) ⇑    c = Code(Parse-Manifest-Err)
──────────────────────────────────────────────────────────────
Γ ⊢ ParseManifest(R) ⇑ c

**Manifest Required (No Single-File Fallback).**
If Γ ⊢ ParseManifest(R) ⇑ c, then Γ ⊢ LoadProject(R, target) ⇑ c and the implementation MUST NOT attempt any single-file or heuristic fallback project construction.

**Manifest Path Resolution.**
Manifest lookup MUST use host filesystem path resolution semantics for R/`Cursive.toml` and MUST NOT perform additional case verification.

**Manifest Schema (Cursive0)**

n = t.name
k = t.kind
r = t.root
o = t.out_dir
e = t.emit_ir

**(WF-Assembly-Name)**
Γ ⊢ n : Identifier    Γ ⊢ n : NotKeyword
────────────────────────────────────────
Γ ⊢ n : Name

**(WF-Assembly-Name-Err)**
¬(Γ ⊢ n : Identifier ∧ Γ ⊢ n : NotKeyword)    c = Code(WF-Assembly-Name-Err)
────────────────────────────────────────────────────────────────────────────
Γ ⊢ n : Name ⇑ c

**(WF-Assembly-Kind)**
k ∈ AssemblyKind
──────────────────
Γ ⊢ k : Kind

**(WF-Assembly-Kind-Err)**
k ∉ AssemblyKind    c = Code(WF-Assembly-Kind-Err)
──────────────────────────────────────────────────
Γ ⊢ k : Kind ⇑ c

**(WF-Assembly-Root-Path)**
Γ ⊢ r : RelPath
────────────────
Γ ⊢ r : RootPath

**(WF-Assembly-Root-Path-Err)**
¬(Γ ⊢ r : RelPath)    c = Code(WF-Assembly-Root-Path-Err)
────────────────────────────────────────────────────────
Γ ⊢ r : RootPath ⇑ c

**(WF-Assembly-OutDir-Path)**
o = ⊥ ∨ Γ ⊢ o : RelPath
────────────────────────
Γ ⊢ o : OutDirPath

**(WF-Assembly-OutDir-Path-Err)**
o ≠ ⊥    ¬(Γ ⊢ o : RelPath)    c = Code(WF-Assembly-OutDir-Path-Err)
────────────────────────────────────────────────────────────────────
Γ ⊢ o : OutDirPath ⇑ c

**(WF-Assembly-EmitIR)**
e ∈ {⊥, `none`, `ll`, `bc`}
───────────────────────────
Γ ⊢ e : EmitIR

**(WF-Assembly-EmitIR-Err)**
e ∉ {⊥, `none`, `ll`, `bc`}    c = Code(WF-Assembly-EmitIR-Err)
─────────────────────────────────────────────────────────────────
Γ ⊢ e : EmitIR ⇑ c

**Manifest Validation (Big-Step)**

Keys(T) = Dom(T)
AsmField(T) = T[`assembly`]
AsmTables(T) =
 [AsmField(T)]  if IsTable(AsmField(T))
 AsmField(T)    if IsArrayTable(AsmField(T))
 ⊥              otherwise

**(WF-TopKeys)**
Keys(T) ⊆ {`assembly`}
────────────────────────
Γ ⊢ T : TopKeys

**(WF-TopKeys-Err)**
¬(Keys(T) ⊆ {`assembly`})    c = Code(WF-TopKeys-Err)
──────────────────────────────────────────────────────
Γ ⊢ T : TopKeys ⇑ c

**(WF-Assembly-Table)**
AsmTables(T) ≠ ⊥
────────────────────────
Γ ⊢ T : AssemblyTable

**(WF-Assembly-Table-Err)**
AsmTables(T) = ⊥    c = Code(WF-Assembly-Table-Err)
────────────────────────────────────────────────────
Γ ⊢ T : AssemblyTable ⇑ c

**(WF-Assembly-Count)**
AsmTables(T) = Ts    |Ts| ≥ 1
────────────────────────────────
Γ ⊢ T : AssemblyCount

**(WF-Assembly-Count-Err)**
AsmTables(T) = Ts    |Ts| = 0    c = Code(WF-Assembly-Count-Err)
───────────────────────────────────────────────────────────────
Γ ⊢ T : AssemblyCount ⇑ c

**(WF-Assembly-Name-Dup)**
AsmTables(T) = Ts    Distinct([t.name | t ∈ Ts])
────────────────────────────────────────────────
Γ ⊢ T : AssemblyNames

**(WF-Assembly-Name-Dup-Err)**
AsmTables(T) = Ts    ¬ Distinct([t.name | t ∈ Ts])    c = Code(WF-Assembly-Name-Dup)
────────────────────────────────────────────────────────────────────────────
Γ ⊢ T : AssemblyNames ⇑ c

Req = {`name`, `kind`, `root`}
Opt = {`out_dir`, `emit_ir`}

**(WF-Assembly-Keys)**
Keys(t) ⊆ (Req ∪ Opt)
──────────────────────
Γ ⊢ t : KnownKeys

**(WF-Assembly-Keys-Err)**
¬(Keys(t) ⊆ (Req ∪ Opt))    c = Code(WF-Assembly-Keys-Err)
───────────────────────────────────────────────────────────
Γ ⊢ t : KnownKeys ⇑ c

**(WF-Assembly-Required-Types)**
∀ k ∈ Req. IsString(t[k])
──────────────────────────
Γ ⊢ t : ReqTypes

**(WF-Assembly-Required-Types-Err)**
∃ k ∈ Req. t[k] = ⊥ ∨ ¬ IsString(t[k])    c = Code(WF-Assembly-Required-Types-Err)
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ t : ReqTypes ⇑ c

**(WF-Assembly-Optional-Types)**
t[`out_dir`] ∈ {string, ⊥}
──────────────────────────
Γ ⊢ t : OutDirType

**(WF-Assembly-OutDirType-Err)**
t[`out_dir`] ∉ {string, ⊥}    c = Code(WF-Assembly-OutDirType-Err)
──────────────────────────────────────────────────────────────────
Γ ⊢ t : OutDirType ⇑ c

t[`emit_ir`] ∈ {string, ⊥}
──────────────────────────
Γ ⊢ t : EmitIRType

**(WF-Assembly-EmitIRType-Err)**
t[`emit_ir`] ∉ {string, ⊥}    c = Code(WF-Assembly-EmitIRType-Err)
──────────────────────────────────────────────────────────────────
Γ ⊢ t : EmitIRType ⇑ c

**Path Resolution**
WinSep = {"\\", "/"}
AsciiLetter(c) ⇔ (c ∈ {"A", …, "Z"} ∨ c ∈ {"a", …, "z"})
DriveRooted(p) ⇔ |p| ≥ 3 ∧ AsciiLetter(At(p, 0)) ∧ At(p, 1) = ":" ∧ At(p, 2) ∈ WinSep
UNC(p) ⇔ StartsWith(p, "//") ∨ StartsWith(p, "\\\\")
RootRelative(p) ⇔ (StartsWith(p, "/") ∨ StartsWith(p, "\\")) ∧ ¬ UNC(p) ∧ ¬ DriveRooted(p)
RootTag(p) =
 p[0..2)  if DriveRooted(p)
 "//"     if UNC(p)
 "/"      if RootRelative(p)
 "\""     otherwise
Tail(p) =
 p[3..|p|)  if DriveRooted(p)
 p[2..|p|)  if UNC(p)
 p[1..|p|)  if RootRelative(p)
 p         otherwise
Segs(p) = [ p[i..j) | 0 ≤ i < j ≤ |p| ∧ (∀ k ∈ [i, j). At(p, k) ∉ WinSep) ∧ (i = 0 ∨ At(p, i-1) ∈ WinSep) ∧ (j = |p| ∨ At(p, j) ∈ WinSep) ]
PathComps(p) =
 Segs(p)  if RootTag(p) = "\""
 [RootTag(p)] ++ Segs(Tail(p))  otherwise
JoinComp([]) = "\""
JoinComp([c]) = c
JoinComp(c::cs) =
 c ++ JoinComp(cs)          if c ∈ {"/", "//"}
 c ++ "/" ++ JoinComp(cs)   if DriveRooted(c ++ "/")
 c ++ "/" ++ JoinComp(cs)   otherwise
Join(a, b) =
 b  if AbsPath(b)
 JoinComp(PathComps(a) ++ PathComps(b))  otherwise

AbsPath(p) ⇔ DriveRooted(p) ∨ UNC(p) ∨ RootRelative(p)
is_relative(p) ⇔ ¬ AbsPath(p)
Join : Path × Path → Path
Normalize : Path → Path
Canon : Path ⇀ Path
prefix(p, q) ⇔ PathPrefix(PathComps(q), PathComps(p))
Normalize(p) = JoinComp([ c | c ∈ PathComps(p) ∧ c ≠ "." ])
Canon(p) = ⊥ ⇔ ∃ c ∈ PathComps(Normalize(p)). c = ".."
Canon(p) = Normalize(p) ⇔ ¬ ∃ c ∈ PathComps(Normalize(p)). c = ".."
Drop(0, xs) = xs    Drop(n, []) = []    Drop(n, x::xs) = Drop(n-1, xs) (n > 0)
relative(p, base) = rel ⇔ Canon(p) = p' ∧ Canon(base) = b' ∧ PathPrefix(PathComps(b'), PathComps(p')) ∧ rel = JoinComp(Drop(|PathComps(b')|, PathComps(p')))
Basename(p) =
 "\""  if |PathComps(p)| = 0
 last(PathComps(p))  otherwise
last([x]) = x    last(x::xs) = last(xs) (|xs| > 0)

b = Basename(p)
D = { j | 0 ≤ j < |b| ∧ b[j] = "." }
FileExt(p) =
 "\""  if D = ∅
 "\""  if D ≠ ∅ ∧ max(D) = 0
 b[max(D)..|b|)  if D ≠ ∅ ∧ max(D) > 0

**(Resolve-Canonical)**
p' = Normalize(Join(R, p))    Canon(R) = R'    Canon(p') = p''
─────────────────────────────────────────────────────────────
Γ ⊢ Resolve(R, p) ⇓ (R', p'')

**(Resolve-Canonical-Err)**
p' = Normalize(Join(R, p))    (Canon(R) = ⊥ ∨ Canon(p') = ⊥)    c = Code(Resolve-Canonical-Err)
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Resolve(R, p) ⇑ c

**(WF-RelPath)**
is_relative(p)    Γ ⊢ Resolve(R, p) ⇓ (R', p'')    prefix(p'', R')
──────────────────────────────────────────────────────────────────────
Γ ⊢ p : RelPath

**(WF-RelPath-Err)**
¬ is_relative(p) ∨ (Γ ⊢ Resolve(R, p) ⇓ (R', p'') ∧ ¬ prefix(p'', R'))    c = Code(WF-RelPath-Err)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ p : RelPath ⇑ c

Project(Γ) = P ⇔ Γ.project = P

**Project Load (Small-Step)**

AssemblyTarget = Name ∪ {⊥}
ProjLoadState = {Start(R, target), Parsed(R, target, T), Validated(R, target, T), ProjAsmScan(R, target, T, Ts, As), Discovered(P), Error(code)}

**(Step-Parse)**
Γ ⊢ ParseManifest(R) ⇓ T
────────────────────────────────────────────────────────
⟨Start(R, target)⟩ → ⟨Parsed(R, target, T)⟩

**(Step-Parse-Err)**
Γ ⊢ ParseManifest(R) ⇑ c
──────────────────────────────────────────────
⟨Start(R, target)⟩ → ⟨Error(c)⟩

**(Step-Validate)**
Γ ⊢ ValidateManifest(T) ⇓ ok
──────────────────────────────────────────────────────────
⟨Parsed(R, target, T)⟩ → ⟨Validated(R, target, T)⟩

**(Step-Validate-Err)**
Γ ⊢ ValidateManifest(T) ⇑ c
───────────────────────────────────────────────
⟨Parsed(R, target, T)⟩ → ⟨Error(c)⟩

**Manifest Validation (Deterministic).**

ChecksAsm(t) = [Γ ⊢ t : KnownKeys, Γ ⊢ t : ReqTypes, Γ ⊢ t : OutDirType, Γ ⊢ t : EmitIRType, Γ ⊢ t.name : Name, Γ ⊢ t.kind : Kind, Γ ⊢ t.emit_ir : EmitIR, Γ ⊢ t.root : RootPath, Γ ⊢ t.out_dir : OutDirPath]
BaseChecks(T) = [Γ ⊢ T : TopKeys, Γ ⊢ T : AssemblyTable, Γ ⊢ T : AssemblyCount, Γ ⊢ T : AssemblyNames]
AsmChecks(T) =
 []  if AsmTables(T) = ⊥
 ++_{t ∈ AsmTables(T)} ChecksAsm(t)  otherwise
Checks(T) = BaseChecks(T) ++ AsmChecks(T)

FirstFail([]) = ⊥
FirstFail(J::Js) = c ⇔ Γ ⊢ J ⇑ c
FirstFail(J::Js) = FirstFail(Js) ⇔ Γ ⊢ J ⇓ ok

**(ValidateManifest-Ok)**
FirstFail(Checks(T)) = ⊥
────────────────────────────────────
Γ ⊢ ValidateManifest(T) ⇓ ok

**(ValidateManifest-Err)**
FirstFail(Checks(T)) = c
───────────────────────────────────
Γ ⊢ ValidateManifest(T) ⇑ c

**(Step-Asm-Init)**
Ts = AsmTables(T)
──────────────────────────────────────────────────────────
⟨Validated(R, target, T)⟩ → ⟨ProjAsmScan(R, target, T, Ts, [])⟩

**(Step-Asm-Cons)**
Γ ⊢ BuildAssembly(R, t_0) ⇓ A
────────────────────────────────────────────────────────────────────────────────────────────
⟨ProjAsmScan(R, target, T, t_0::ts, As)⟩ → ⟨ProjAsmScan(R, target, T, ts, As ++ [A])⟩

**(Step-Asm-Err)**
Γ ⊢ BuildAssembly(R, t_0) ⇑ c
──────────────────────────────────────────────────────────────
⟨ProjAsmScan(R, target, T, t_0::ts, As)⟩ → ⟨Error(c)⟩

**(Step-Asm-Done)**
Γ ⊢ SelectAssembly(As, target) ⇓ A_0    P = ⟨root = R, assemblies = As, assembly = A_0, source_root = A_0.source_root, outputs = A_0.outputs, modules = A_0.modules⟩
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨ProjAsmScan(R, target, T, [], As)⟩ → ⟨Discovered(P)⟩

**(Step-Asm-Done-Err)**
Γ ⊢ SelectAssembly(As, target) ⇑ c
───────────────────────────────────────────────
⟨ProjAsmScan(R, target, T, [], As)⟩ → ⟨Error(c)⟩

**Assembly Selection**

**(Select-Only)**
|As| = 1    target = ⊥    As = [A_0]
──────────────────────────────────────────────
Γ ⊢ SelectAssembly(As, target) ⇓ A_0

**(Select-By-Name)**
target ≠ ⊥    A ∈ As    A.name = target
──────────────────────────────────────────────
Γ ⊢ SelectAssembly(As, target) ⇓ A

**(Select-Err)**
(target = ⊥ ∧ |As| ≠ 1) ∨ (target ≠ ⊥ ∧ ¬ ∃ A ∈ As. A.name = target)    c = Code(Assembly-Select-Err)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ SelectAssembly(As, target) ⇑ c

**Assembly Build (Big-Step)**

**(BuildAssembly-Ok)**
Γ ⊢ Resolve(R, t.root) ⇓ (R', S)    Γ ⊢ S : SourceRoot    Γ ⊢ Modules(S, t.name) ⇓ M    L = sort_{≺_mod}(M)    A = ⟨name = t.name, kind = t.kind, root = t.root, out_dir = t.out_dir, emit_ir = t.emit_ir, source_root = S, outputs = OutputPaths(R, t), modules = L⟩
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BuildAssembly(R, t) ⇓ A

**(BuildAssembly-Err-Resolve)**
Γ ⊢ Resolve(R, t.root) ⇑ c
─────────────────────────────────
Γ ⊢ BuildAssembly(R, t) ⇑ c

**(BuildAssembly-Err-Root)**
Γ ⊢ Resolve(R, t.root) ⇓ (R', S)    Γ ⊢ S : SourceRoot ⇑ c
─────────────────────────────────────────────────────────────
Γ ⊢ BuildAssembly(R, t) ⇑ c

**(BuildAssembly-Err-Modules)**
Γ ⊢ Resolve(R, t.root) ⇓ (R', S)    Γ ⊢ S : SourceRoot    Γ ⊢ Modules(S, t.name) ⇑ c
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BuildAssembly(R, t) ⇑ c

**Project Load (Big-Step)**

**(LoadProject-Ok)**
Γ ⊢ ParseManifest(R) ⇓ T    Γ ⊢ ValidateManifest(T) ⇓ ok    AsmTables(T) = [t_1, …, t_n]    ∀ i, Γ ⊢ BuildAssembly(R, t_i) ⇓ A_i    As = [A_1, …, A_n]    Γ ⊢ SelectAssembly(As, target) ⇓ A_0    P = ⟨root = R, assemblies = As, assembly = A_0, source_root = A_0.source_root, outputs = A_0.outputs, modules = A_0.modules⟩
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LoadProject(R, target) ⇓ P

**(LoadProject-Err)**
Γ ⊢ LoadProject(R, target) →* ⟨Error(c)⟩
────────────────────────────────────────────
Γ ⊢ LoadProject(R, target) ⇑ c

**Well-Formed Project Root**

**(WF-Project-Root)**
exists(`Cursive.toml` at R)
───────────────────────────
⊢ R : ProjectRoot

### 2.2. Assemblies

A_0.kind ∈ AssemblyKind
────────────────────────
Γ ⊢ A_0 : Assembly


### 2.3. Deterministic Ordering and Case Folding

#### 2.3.1. Module File Processing Order
FoldPath(r) = JoinComp([CaseFold(NFC(c)) | c ∈ PathComps(r)])

FileKey(f, d) =
 ⟨FoldPath(rel), rel⟩  if relative(f, d) ⇓ rel
 ⟨⊥, Basename(f)⟩      if relative(f, d) ⇑

f_1 ≺_file f_2 ⇔ Utf8LexLess(FileKey(f_1, d), FileKey(f_2, d))

**(FileOrder-Rel-Fail)**
relative(f, d) ⇑    c = Code(FileOrder-Rel-Fail)
────────────────────────────────────────────────
Γ ⊢ Emit(c)

#### 2.3.2. Module Path Case-Folding Algorithm

**Fold.**
Fold(p) = [CaseFold(NFC(c)) | c ∈ p]

#### 2.3.3. Directory Enumeration Order

DirKey(d, S) =
 ⟨FoldPath(rel), rel⟩  if relative(d, S) ⇓ rel
 ⟨⊥, Basename(d)⟩      if relative(d, S) ⇑

d_1 ≺_dir d_2 ⇔ Utf8LexLess(DirKey(d_1, S), DirKey(d_2, S))

DirSeq(S) = sort_{≺_dir}(Dirs(S))

**(DirSeq-Read-Err)**
Dirs(S) ⇑    c = Code(DirSeq-Read-Err)
──────────────────────────────────────
Γ ⊢ Emit(c)

**(DirSeq-Rel-Fail)**
relative(d, S) ⇑    c = Code(DirSeq-Rel-Fail)
──────────────────────────────────────────────
Γ ⊢ Emit(c)

### 2.4. Module Discovery

**Dirs.**
Dirs(S) = { d | is_dir(d) ∧ relative(d, S) ⇓ r }
S ∈ Dirs(S)

**(WF-Source-Root)**
is_dir(S)
──────────────
Γ ⊢ S : SourceRoot

**(WF-Source-Root-Err)**
¬ is_dir(S)    c = Code(WF-Source-Root-Err)
───────────────────────────────────────────
Γ ⊢ S : SourceRoot ⇑ c

**(Module-Dir)**
∃ f ∈ Files(d) : FileExt(f) = ".cursive"
─────────────────────────────────────────
Γ ⊢ d : ModuleDir

Modules(S) = { d ∈ Dirs(S) | Γ ⊢ d : ModuleDir }

**Module Discovery (Big-Step)**

**(Modules-Ok)**
⟨DiscStart(S, A)⟩ →* ⟨DiscDone(M)⟩
──────────────────────────────────
Γ ⊢ Modules(S, A) ⇓ M

**(Modules-Err)**
⟨DiscStart(S, A)⟩ →* ⟨Error(c)⟩
─────────────────────────────────
Γ ⊢ Modules(S, A) ⇑ c

Files(d) = { f | f ∈ d ∧ FileExt(f) = ".cursive" }

CompilationUnit(d) = sort_{≺_file}(Files(d))

**(CompilationUnit-Rel-Fail)**
∃ f ∈ Files(d). relative(f, d) ⇑    c = Code(FileOrder-Rel-Fail)
─────────────────────────────────────────────────────────────────
Γ ⊢ CompilationUnit(d) ⇑ c


**Module Path.**

**(Module-Path-Root)**
relative(d, S) = ε
────────────────────────────
Γ ⊢ ModulePath(d, S, A) = A

**(Module-Path-Rel)**
relative(d, S) = c_1 / … / c_n
──────────────────────────────────────────────
Γ ⊢ ModulePath(d, S, A) = c_1 :: … :: c_n

**(Module-Path-Rel-Fail)**
relative(d, S) ⇑
────────────────────────────────
Γ ⊢ ModulePath(d, S, A) ⇑

WFModulePathJudg = {WF-Module-Path}

**(WF-Module-Path-Ok)**
∀ comp ∈ p, (Γ ⊢ comp : Identifier) ∧ ¬ Keyword(comp)
────────────────────────────────────────────────────────
Γ ⊢ WF-Module-Path(p) ⇓ ok

**(WF-Module-Path-Reserved)**
∃ comp ∈ p. Keyword(comp)    c = Code(WF-Module-Path-Reserved)
────────────────────────────────────────────────────────────────
Γ ⊢ WF-Module-Path(p) ⇑ c

**(WF-Module-Path-Ident-Err)**
∃ comp ∈ p. ¬(Γ ⊢ comp : Identifier)    c = Code(WF-Module-Path-Ident-Err)
──────────────────────────────────────────────────────────────────────────
Γ ⊢ WF-Module-Path(p) ⇑ c

**(WF-Module-Path-Collision)**
p_1 ≠ p_2    Fold(p_1) = Fold(p_2)
────────────────────────────────────────────────────────────────────────────
Γ ⊢ Emit(Code(WF-Module-Path-Collision))    Γ ⊢ Emit(W-MOD-1101, ⊥)

**Module Discovery (Small-Step)**
DiscState = {DiscStart(S, A), DiscScan(S, A, Pending, M, Seen), DiscDone(M), Error(code)}

**(Disc-Start)**
────────────────────────────────────────────────────────────────────────────
⟨DiscStart(S, A)⟩ → ⟨DiscScan(S, A, DirSeq(S), [], ∅)⟩

**(Disc-Skip)**
Γ ⊬ d : ModuleDir
────────────────────────────────────────────────────────────
⟨DiscScan(S, A, d::ds, M, Seen)⟩ → ⟨DiscScan(S, A, ds, M, Seen)⟩

**(Disc-Add)**
Γ ⊢ d : ModuleDir    Γ ⊢ ModulePath(d, S, A) = p    Γ ⊢ WF-Module-Path(p) ⇓ ok    Fold(p) ∉ dom(Seen)
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨DiscScan(S, A, d::ds, M, Seen)⟩ → ⟨DiscScan(S, A, ds, M ++ [p], Seen ∪ {Fold(p) ↦ p})⟩

**(Disc-Collision)**
Γ ⊢ d : ModuleDir    Γ ⊢ ModulePath(d, S, A) = p    Γ ⊢ WF-Module-Path(p) ⇓ ok    Fold(p) ∈ dom(Seen)    Seen[Fold(p)] ≠ p
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨DiscScan(S, A, d::ds, M, Seen)⟩ → ⟨Error(Code(Disc-Collision))⟩

**(Disc-Invalid-Component)**
Γ ⊢ d : ModuleDir    Γ ⊢ ModulePath(d, S, A) = p    Γ ⊢ WF-Module-Path(p) ⇑ c
────────────────────────────────────────────────────────────────────────────────────────────────
⟨DiscScan(S, A, d::ds, M, Seen)⟩ → ⟨Error(c)⟩

**(Disc-Rel-Fail)**
Γ ⊢ d : ModuleDir    relative(d, S) ⇑
────────────────────────────────────────────────────────────────────────────
⟨DiscScan(S, A, d::ds, M, Seen)⟩ → ⟨Error(Code(Disc-Rel-Fail))⟩

**(Disc-Done)**
──────────────────────────────────────────────────────────────
⟨DiscScan(S, A, [], M, Seen)⟩ → ⟨DiscDone(M)⟩

**(WF-Import-Unsupported)**
import_decl ∈ M
────────────────────────────────────────────────
Γ ⊢ Emit(Code(WF-Import-Unsupported))

### 2.5. Output Artifacts and Linking

**Output Root.**

O = OutputRoot(P) =
 P.root/P.assembly.out_dir  if provided
 P.root/`build`             otherwise

**Output Hygiene (Cursive0).**
OutputHygiene(P) ⇔ ∀ p ∈ RequiredOutputs(P). Under(p, OutputRoot(P))

OutputPaths(R, A).root =
 R/A.out_dir  if provided
 R/`build`    otherwise
OutputPaths(R, A).obj_dir = OutputPaths(R, A).root/`obj`
OutputPaths(R, A).ir_dir = OutputPaths(R, A).root/`ir`
OutputPaths(R, A).bin_dir = OutputPaths(R, A).root/`bin`

P.outputs = P.assembly.outputs

**Object File Naming**

PathToPrefix(s) = Concat([BMap(b) | b ∈ Utf8(NFC(s))])
BMap(b) =
 chr(b)           if b ∈ [0-9A-Za-z]
 "_x" ++ Hex2(b)  otherwise

mangle(s) = PathToPrefix(s)
MangleModulePath(p) = mangle(PathString(PathKey(p)))

obj(m) = O / `obj` / (MangleModulePath(p) ++ ".obj")

**Executable Naming**

exe = O / `bin` / (assembly_name ++ ".exe")

**Output and Linking Semantics (Formal Rules)**

path(m) = m.path
S = P.source_root

**Module Emission Order.**

m_1 ≺_mod m_2 ⇔ Utf8LexLess(Fold(path(m_1)), Fold(path(m_2))) ∨ (Fold(path(m_1)) = Fold(path(m_2)) ∧ Utf8LexLess(path(m_1), path(m_2)))

Utf8LexLess(a, b) ⇔ LexBytes(Utf8(a), Utf8(b))

**(ModuleList-Ok)**
Γ ⊢ Modules(S, P.assembly.name) ⇓ M    L = sort_{≺_mod}(M)
───────────────────────────────────────────────────────────
Γ ⊢ ModuleList(P) ⇓ L

**(ModuleList-Err)**
Γ ⊢ Modules(S, P.assembly.name) ⇑ c
────────────────────────────────────
Γ ⊢ ModuleList(P) ⇑ c

**Output Paths.**
O = OutputRoot(P)
assembly_name = P.assembly.name
ext(e) =
 ".ll"  if e = `ll`
 ".bc"  if e = `bc`

ObjPath(P, m) = O / `obj` / (MangleModulePath(path(m)) ++ ".obj")
IRPath(P, m, e) = O / `ir` / (MangleModulePath(path(m)) ++ ext(e))
ExePath(P) =
 O / `bin` / (assembly_name ++ ".exe")  if Executable(P)
 ⊥                                     otherwise

ObjPaths(P, ms) = [ObjPath(P, m) | m ∈ ms]
IRPaths(P, ms, e) = [IRPath(P, m, e) | m ∈ ms]

**Module Index and Symbol Name.**
ModuleList(P) = [m_1, …, m_n]
Index(P, m_i) = i
pad4(i) = PadLeft(Decimal(i), '0', 4)
SymbolName(P, m) =
 "main"  if path(m) = P.assembly.name
 "mod" ++ pad4(Index(P, m))  otherwise

trunc8(s) = PadRight(Take(Utf8(s), 8), 8, 0x00)

**LLVM Target Constants.**
LLVMTriple = "x86_64-pc-windows-msvc"
LLVMDataLayout = "e-m:w-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"

IsRootModule(P, m) ⇔ path(m) = P.assembly.name

WithEntry(P, m, IR) =
 IR ++ [EntryStub(P)]  if Executable(P) ∧ IsRootModule(P, m)
 IR                    otherwise

**(CodegenObj-LLVM)**
Project(Γ) = P    Γ ⊢ CodegenModule(m) ⇓ IR    IR' = WithEntry(P, m, IR)    Γ ⊢ LowerIR(IR') ⇓ L    Γ ⊢ EmitObj(L) ⇓ b
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ CodegenObj(m) ⇓ b

**(CodegenIR-LLVM)**
Project(Γ) = P    e ∈ {`ll`, `bc`}    Γ ⊢ CodegenModule(m) ⇓ IR    IR' = WithEntry(P, m, IR)    Γ ⊢ LowerIR(IR') ⇓ L    Γ ⊢ EmitLLVM(L) ⇓ b
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ CodegenIR(m, e) ⇓ b

**File Emission.**
WriteFileOk(p, b) ⇒ Overwrites(p, b)

**Directory Creation.**
EnsureDir(p) ⇓ ok ⇒ IsDir(p)

**Emit Objects**

**(Emit-Objects-Empty)**
────────────────────────────────────────
Γ ⊢ EmitObjects([], P) ⇓ []

**(Emit-Objects-Cons)**
Γ ⊢ CodegenObj(m) ⇓ b    Γ ⊢ WriteFile(ObjPath(P, m), b) ⇓ ok    Γ ⊢ EmitObjects(ms, P) ⇓ L
────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EmitObjects(m::ms, P) ⇓ ObjPath(P, m)::L

**Emit IR**

**(Emit-IR-None)**
e = `none`
───────────────────────────────
Γ ⊢ EmitIR(ms, P, e) ⇓ []

**(Emit-IR-Cons-LL)**
e = `ll`    Γ ⊢ CodegenIR(m, e) ⇓ b    Γ ⊢ WriteFile(IRPath(P, m, e), b) ⇓ ok    Γ ⊢ EmitIR(ms, P, e) ⇓ L
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EmitIR(m::ms, P, e) ⇓ IRPath(P, m, e)::L

**(Emit-IR-Cons-BC)**
e = `bc`    Γ ⊢ CodegenIR(m, `ll`) ⇓ t    Γ ⊢ ResolveTool(`llvm-as`) ⇓ a    Γ ⊢ AssembleIR(a, t) ⇓ b    Γ ⊢ WriteFile(IRPath(P, m, e), b) ⇓ ok    Γ ⊢ EmitIR(ms, P, e) ⇓ L
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EmitIR(m::ms, P, e) ⇓ IRPath(P, m, e)::L

EmitIRFail(m, P, `ll`) ⇔ Γ ⊢ CodegenIR(m, `ll`) ⇑ ∨ (∃ b. Γ ⊢ CodegenIR(m, `ll`) ⇓ b ∧ Γ ⊢ WriteFile(IRPath(P, m, `ll`), b) ⇑)
EmitIRFail(m, P, `bc`) ⇔
 Γ ⊢ CodegenIR(m, `ll`) ⇑ ∨
 (∃ t. Γ ⊢ CodegenIR(m, `ll`) ⇓ t ∧ Γ ⊢ ResolveTool(`llvm-as`) ⇑) ∨
 (∃ t, a. Γ ⊢ CodegenIR(m, `ll`) ⇓ t ∧ Γ ⊢ ResolveTool(`llvm-as`) ⇓ a ∧ Γ ⊢ AssembleIR(a, t) ⇑) ∨
 (∃ t, a, b. Γ ⊢ CodegenIR(m, `ll`) ⇓ t ∧ Γ ⊢ ResolveTool(`llvm-as`) ⇓ a ∧ Γ ⊢ AssembleIR(a, t) ⇓ b ∧ Γ ⊢ WriteFile(IRPath(P, m, `bc`), b) ⇑)

**(Emit-IR-Err)**
EmitIRFail(m, P, e)    c = Code(Out-IR-Err)
────────────────────────────────────────────
Γ ⊢ EmitIR(m::ms, P, e) ⇑ c

LinkJudg = {ResolveRuntimeLib, Link}
RuntimeLibName = "cursive0_rt.lib"
RuntimeLibPath(P) = P.root / `runtime` / RuntimeLibName

**(ResolveRuntimeLib-Ok)**
Γ ⊢ ReadBytes(RuntimeLibPath(P)) ⇓ _
──────────────────────────────────────────────
Γ ⊢ ResolveRuntimeLib(P) ⇓ RuntimeLibPath(P)

**(ResolveRuntimeLib-Err)**
Γ ⊢ ReadBytes(RuntimeLibPath(P)) ⇑
─────────────────────────────────
Γ ⊢ ResolveRuntimeLib(P) ⇑

LinkerSyms : Path × List(Path) × Path ⇀ ℘(Symbol)
RuntimeRequiredSyms = RuntimeSyms
MissingRuntimeSym(t, L, exe) ⇔ RuntimeRequiredSyms ⊈ LinkerSyms(t, L, exe)

LinkObjs(P) = ObjPaths(P, ModuleList(P))
LinkInputs(P) = LinkObjs(P) ++ [RuntimeLibPath(P)]
LinkFlags(P) = ["/OUT:" ++ ExePath(P), "/ENTRY:main", "/SUBSYSTEM:CONSOLE", "/NODEFAULTLIB"]
LinkArgsOk(P, L, exe) ⇔ L = LinkInputs(P) ∧ exe = ExePath(P) ∧ LinkFlags(P) = ["/OUT:" ++ ExePath(P), "/ENTRY:main", "/SUBSYSTEM:CONSOLE", "/NODEFAULTLIB"]

**(Link-Ok)**
Executable(P)    Γ ⊢ ResolveTool(`lld-link`) ⇓ t    Γ ⊢ ResolveRuntimeLib(P) ⇓ lib    LinkArgsOk(P, Objs ++ [lib], ExePath(P))    Γ ⊢ InvokeLinker(t, Objs ++ [lib], ExePath(P)) ⇓ ok
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Link(Objs, P) ⇓ ok

**(Link-NotFound)**
Executable(P)    Γ ⊢ ResolveTool(`lld-link`) ⇑    c = Code(Out-Link-NotFound)
─────────────────────────────────────────────────────────────────────────────
Γ ⊢ Link(Objs, P) ⇑ c

**(Link-Runtime-Missing)**
Executable(P)    Γ ⊢ ResolveTool(`lld-link`) ⇓ t    Γ ⊢ ResolveRuntimeLib(P) ⇑    c = Code(Out-Link-Runtime-Missing)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Link(Objs, P) ⇑ c

**(Link-Runtime-Incompatible)**
Executable(P)    Γ ⊢ ResolveTool(`lld-link`) ⇓ t    Γ ⊢ ResolveRuntimeLib(P) ⇓ lib    LinkArgsOk(P, Objs ++ [lib], ExePath(P))    MissingRuntimeSym(t, Objs ++ [lib], ExePath(P))    c = Code(Out-Link-Runtime-Incompatible)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Link(Objs, P) ⇑ c

**(Link-Fail)**
Executable(P)    Γ ⊢ ResolveTool(`lld-link`) ⇓ t    Γ ⊢ ResolveRuntimeLib(P) ⇓ lib    LinkArgsOk(P, Objs ++ [lib], ExePath(P))    ¬ MissingRuntimeSym(t, Objs ++ [lib], ExePath(P))    Γ ⊢ InvokeLinker(t, Objs ++ [lib], ExePath(P)) ⇑    c = Code(Out-Link-Fail)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Link(Objs, P) ⇑ c

**Output Pipeline (Big-Step)**
O = OutputRoot(P)
ms = ModuleList(P)
e = P.assembly.emit_ir

**(Output-Pipeline)**
Executable(P)    Γ ⊢ EnsureDir(O) ⇓ ok    Γ ⊢ EnsureDir(O / `obj`) ⇓ ok    Γ ⊢ EnsureDir(O / `bin`) ⇓ ok    (e = `none` ∨ Γ ⊢ EnsureDir(O / `ir`) ⇓ ok)    Γ ⊢ EmitObjects(ms, P) ⇓ Objs    Γ ⊢ EmitIR(ms, P, e) ⇓ IRs    Γ ⊢ Link(Objs, P) ⇓ ok
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ OutputPipeline(P) ⇓ (Objs, IRs, ExePath(P))

**(Output-Pipeline-Lib)**
¬ Executable(P)    Γ ⊢ EnsureDir(O) ⇓ ok    Γ ⊢ EnsureDir(O / `obj`) ⇓ ok    (e = `none` ∨ Γ ⊢ EnsureDir(O / `ir`) ⇓ ok)    Γ ⊢ EmitObjects(ms, P) ⇓ Objs    Γ ⊢ EmitIR(ms, P, e) ⇓ IRs
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ OutputPipeline(P) ⇓ (Objs, IRs, ⊥)

**(Output-Pipeline-Err)**
⟨OutStart(P)⟩ →* ⟨Error(c)⟩
────────────────────────────────
Γ ⊢ OutputPipeline(P) ⇑ c

**Output Pipeline (Small-Step)**
OutState = {OutStart(P), OutDirs(P), OutObjs(P, ms, Objs), OutIR(P, ms, Objs, IRs, e), OutLink(P, Objs, IRs), OutDone(Objs, IRs, Exe), Error(code)}
O = OutputRoot(P)
ms = ModuleList(P)
e = P.assembly.emit_ir

**(Out-Start)**
────────────────────────────────────────
⟨OutStart(P)⟩ → ⟨OutDirs(P)⟩

**(Out-Dirs-Ok)**
Γ ⊢ EnsureDir(O) ⇓ ok    Γ ⊢ EnsureDir(O / `obj`) ⇓ ok    (¬ Executable(P) ∨ Γ ⊢ EnsureDir(O / `bin`) ⇓ ok)    (e = `none` ∨ Γ ⊢ EnsureDir(O / `ir`) ⇓ ok)
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨OutDirs(P)⟩ → ⟨OutObjs(P, ms, [])⟩

**(Out-Dirs-Err)**
Γ ⊢ EnsureDir(O) ⇑ ∨ Γ ⊢ EnsureDir(O / `obj`) ⇑ ∨ (Executable(P) ∧ Γ ⊢ EnsureDir(O / `bin`) ⇑) ∨ (e ∈ {`ll`, `bc`} ∧ Γ ⊢ EnsureDir(O / `ir`) ⇑)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨OutDirs(P)⟩ → ⟨Error(Code(Out-Dirs-Err))⟩

**(Out-Obj-Collision)**
¬ Distinct(L ++ ObjPaths(P, ms))
──────────────────────────────────────────────
⟨OutObjs(P, ms, L)⟩ → ⟨Error(Code(Out-Obj-Collision))⟩

**(Out-Obj-Cons)**
Γ ⊢ CodegenObj(m) ⇓ b    Γ ⊢ WriteFile(ObjPath(P, m), b) ⇓ ok
─────────────────────────────────────────────────────────────────────────────
⟨OutObjs(P, m::ms, L)⟩ → ⟨OutObjs(P, ms, L ++ [ObjPath(P, m)])⟩

**(Out-Obj-Err)**
Γ ⊢ CodegenObj(m) ⇑ ∨ (Γ ⊢ CodegenObj(m) ⇓ b ∧ Γ ⊢ WriteFile(ObjPath(P, m), b) ⇑)
──────────────────────────────────────────────────────────────────────────────────────────────
⟨OutObjs(P, m::ms, L)⟩ → ⟨Error(Code(Out-Obj-Err))⟩

**(Out-Obj-Done)**
────────────────────────────────────────────────────────────────────
⟨OutObjs(P, [], L)⟩ → ⟨OutIR(P, ModuleList(P), L, [], e)⟩

**(Out-IR-None)**
e = `none`    Executable(P)
─────────────────────────────────────────────────────────────────────
⟨OutIR(P, ms, Objs, IRs, e)⟩ → ⟨OutLink(P, Objs, IRs)⟩

**(Out-IR-None-NoLink)**
e = `none`    ¬ Executable(P)
────────────────────────────────────────────────────────────────────────
⟨OutIR(P, ms, Objs, IRs, e)⟩ → ⟨OutDone(Objs, IRs, ⊥)⟩

**(Out-IR-Collision)**
e ∈ {`ll`, `bc`}    ¬ Distinct(IRs ++ IRPaths(P, ms, e))
──────────────────────────────────────────────────────────────────────────────
⟨OutIR(P, ms, Objs, IRs, e)⟩ → ⟨Error(Code(Out-IR-Collision))⟩

**(Out-IR-Cons-LL)**
e = `ll`    Γ ⊢ CodegenIR(m, e) ⇓ b    Γ ⊢ WriteFile(IRPath(P, m, e), b) ⇓ ok
────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨OutIR(P, m::ms, Objs, IRs, e)⟩ → ⟨OutIR(P, ms, Objs, IRs ++ [IRPath(P, m, e)], e)⟩

**(Out-IR-Cons-BC)**
e = `bc`    Γ ⊢ CodegenIR(m, `ll`) ⇓ t    Γ ⊢ ResolveTool(`llvm-as`) ⇓ a    Γ ⊢ AssembleIR(a, t) ⇓ b    Γ ⊢ WriteFile(IRPath(P, m, e), b) ⇓ ok
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨OutIR(P, m::ms, Objs, IRs, e)⟩ → ⟨OutIR(P, ms, Objs, IRs ++ [IRPath(P, m, e)], e)⟩

**(Out-IR-Err)**
(e = `ll` ∧ (Γ ⊢ CodegenIR(m, e) ⇑ ∨ (Γ ⊢ CodegenIR(m, e) ⇓ b ∧ Γ ⊢ WriteFile(IRPath(P, m, e), b) ⇑))) ∨
(e = `bc` ∧ (Γ ⊢ CodegenIR(m, `ll`) ⇑ ∨ (Γ ⊢ CodegenIR(m, `ll`) ⇓ t ∧ Γ ⊢ ResolveTool(`llvm-as`) ⇑) ∨ (Γ ⊢ CodegenIR(m, `ll`) ⇓ t ∧ Γ ⊢ ResolveTool(`llvm-as`) ⇓ a ∧ Γ ⊢ AssembleIR(a, t) ⇑) ∨ (Γ ⊢ CodegenIR(m, `ll`) ⇓ t ∧ Γ ⊢ ResolveTool(`llvm-as`) ⇓ a ∧ Γ ⊢ AssembleIR(a, t) ⇓ b ∧ Γ ⊢ WriteFile(IRPath(P, m, e), b) ⇑)))
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨OutIR(P, m::ms, Objs, IRs, e)⟩ → ⟨Error(Code(Out-IR-Err))⟩

**(Out-IR-Done)**
e ∈ {`ll`, `bc`}    ms = []    Executable(P)
─────────────────────────────────────────────────────────────────────────────
⟨OutIR(P, ms, Objs, IRs, e)⟩ → ⟨OutLink(P, Objs, IRs)⟩

**(Out-IR-Done-NoLink)**
e ∈ {`ll`, `bc`}    ms = []    ¬ Executable(P)
────────────────────────────────────────────────────────────────────────────────
⟨OutIR(P, ms, Objs, IRs, e)⟩ → ⟨OutDone(Objs, IRs, ⊥)⟩

**(Out-Link-Ok)**
Executable(P)    Γ ⊢ ResolveTool(`lld-link`) ⇓ t    Γ ⊢ ResolveRuntimeLib(P) ⇓ lib    Γ ⊢ InvokeLinker(t, Objs ++ [lib], ExePath(P)) ⇓ ok
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨OutLink(P, Objs, IRs)⟩ → ⟨OutDone(Objs, IRs, ExePath(P))⟩

**(Out-Link-NotFound)**
Executable(P)    Γ ⊢ ResolveTool(`lld-link`) ⇑
────────────────────────────────────────────────────────────────────────────
⟨OutLink(P, Objs, IRs)⟩ → ⟨Error(Code(Out-Link-NotFound))⟩

**(Out-Link-Runtime-Missing)**
Executable(P)    Γ ⊢ ResolveRuntimeLib(P) ⇑
──────────────────────────────────────────────────────────────────────────────
⟨OutLink(P, Objs, IRs)⟩ → ⟨Error(Code(Out-Link-Runtime-Missing))⟩

**(Out-Link-Runtime-Incompatible)**
Executable(P)    Γ ⊢ ResolveTool(`lld-link`) ⇓ t    Γ ⊢ ResolveRuntimeLib(P) ⇓ lib    MissingRuntimeSym(t, Objs ++ [lib], ExePath(P))
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨OutLink(P, Objs, IRs)⟩ → ⟨Error(Code(Out-Link-Runtime-Incompatible))⟩

**(Out-Link-Fail)**
Executable(P)    Γ ⊢ ResolveTool(`lld-link`) ⇓ t    Γ ⊢ ResolveRuntimeLib(P) ⇓ lib    ¬ MissingRuntimeSym(t, Objs ++ [lib], ExePath(P))    Γ ⊢ InvokeLinker(t, Objs ++ [lib], ExePath(P)) ⇑
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨OutLink(P, Objs, IRs)⟩ → ⟨Error(Code(Out-Link-Fail))⟩

### 2.6. Tool Resolution and IR Assembly Inputs

SearchDirs(P) =
 [Env(`C0_LLVM_BIN`)]  if Env(`C0_LLVM_BIN`) ≠ ⊥ ∧ Env(`C0_LLVM_BIN`) ≠ ""
 [P.root/`llvm/llvm-21.1.8-x86_64/bin`]  if RepoLLVM(P)
 PATHDirs  otherwise

**(ResolveTool-Ok)**
Project(Γ) = P    SearchDirs(P) contains x at t
───────────────────────────────────────────────
Γ ⊢ ResolveTool(x) ⇓ t

**(ResolveTool-Err-Linker)**
Project(Γ) = P    x = `lld-link`    SearchDirs(P) does not contain x
─────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveTool(x) ⇑

**(ResolveTool-Err-IR)**
Project(Γ) = P    x = `llvm-as`    SearchDirs(P) does not contain x
────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveTool(x) ⇑

**(AssembleIR-Ok)**
Invoke(a, t) ⇓ b
────────────────────────
Γ ⊢ AssembleIR(a, t) ⇓ b

**(AssembleIR-Err)**
Invoke(a, t) ⇑
────────────────────────
Γ ⊢ AssembleIR(a, t) ⇑

### 2.7. Unwind and FFI Surface

**(WF-Unwind-Unsupported)**
`[[unwind]]` ∈ M
──────────────────────────────────────────────
Γ ⊢ Emit(Code(WF-Unwind-Unsupported))

## 3. Phase 1: Source Loading, Lexing, Parsing

### 3.1. Source Loading and Normalization

**Source File Record.**

SourceFile = ⟨path, bytes, scalars, text, byte_len, line_starts, line_count⟩

S.text = EncodeUTF8(S.scalars)
S.byte_len = ByteLen(S.text)
S.line_count = |S.line_starts|

**Unicode Scalars and UTF-8.**
Byte = { n ∈ ℕ | 0 ≤ n ≤ 255 }
Bytes = [Byte]
UnicodeScalar = { u ∈ ℕ | 0 ≤ u ≤ 0x10FFFF ∧ u ∉ [0xD800, 0xDFFF] }
Scalars = [UnicodeScalar]
String = Scalars
Utf8Len(u) =
 1  if 0 ≤ u ≤ 0x7F
 2  if 0x80 ≤ u ≤ 0x7FF
 3  if 0x800 ≤ u ≤ 0xFFFF
 4  if 0x10000 ≤ u ≤ 0x10FFFF
EncodeUTF8(u) =
 [u]  if 0 ≤ u ≤ 0x7F
 [0xC0 ∨ (u >> 6), 0x80 ∨ (u & 0x3F)]  if 0x80 ≤ u ≤ 0x7FF
 [0xE0 ∨ (u >> 12), 0x80 ∨ ((u >> 6) & 0x3F), 0x80 ∨ (u & 0x3F)]  if 0x800 ≤ u ≤ 0xFFFF
 [0xF0 ∨ (u >> 18), 0x80 ∨ ((u >> 12) & 0x3F), 0x80 ∨ ((u >> 6) & 0x3F), 0x80 ∨ (u & 0x3F)]  if 0x10000 ≤ u ≤ 0x10FFFF
EncodeUTF8([]) = []
EncodeUTF8(u::U) = EncodeUTF8(u) ++ EncodeUTF8(U)
DecodeUTF8(B) = U ⇔ EncodeUTF8(U) = B
Utf8Valid(B) ⇔ ∃ U. DecodeUTF8(B) = U
Utf8(s) = EncodeUTF8(s)


#### 3.1.1. Unicode Normalization Outside Identifiers

NormalizeOutsideIdentifiers : Scalars → Scalars
NormalizeOutsideIdentifiers(T) = T

#### 3.1.2. Lexically Sensitive Unicode Enforcement
T = S.scalars
LexSensitivePos(S) = [ p | 0 ≤ p < |T| ∧ Sensitive(T[p]) ∧ ¬ InsideLiteralOrComment(p) ]
Γ ⊢ LexSecure(S, K, LexSensitivePos(S)) ⇓ ok
#### 3.1.3. UTF-8 Decoding and BOM Handling


**(Decode-Ok)**
DecodeUTF8(B) ⇓ U
────────────────────────
Γ ⊢ Decode(B) ⇓ U

**(Decode-Err)**
DecodeUTF8(B) ⇑
────────────────────────
Γ ⊢ Decode(B) ⇑

StripLeadBOM([]) = []
StripLeadBOM(U+FEFF::U) = U
StripLeadBOM(u::U) = u::U  if u ≠ U+FEFF

**(StripBOM-Empty)**
────────────────────────────────────────
Γ ⊢ StripBOM([]) ⇓ ([], false, ⊥)

**(StripBOM-None)**
U = u_0::u_1::…    u_0 ≠ U+FEFF    ∀ i > 0, u_i ≠ U+FEFF
────────────────────────────────────────────────────────────
Γ ⊢ StripBOM(U) ⇓ (U, false, ⊥)

**(StripBOM-Start)**
U = U+FEFF::U_1    ∀ i, U_1[i] ≠ U+FEFF
────────────────────────────────────────────
Γ ⊢ StripBOM(U) ⇓ (U_1, true, ⊥)

**(StripBOM-Embedded)**
U' = StripLeadBOM(U)    b = (U ≠ [] ∧ U[0] = U+FEFF)    i = min{ p | 0 ≤ p < |U'| ∧ U'[p] = U+FEFF }
──────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ StripBOM(U) ⇓ (U', b, i)

#### 3.1.4. Line Ending Normalization and Logical Lines

CR = U+000D
LF = U+000A

**(Norm-Empty)**
────────────────────────────────────────
Γ ⊢ NormalizeLF([]) ⇓ []

**(Norm-CRLF)**
Γ ⊢ NormalizeLF(U) ⇓ V
────────────────────────────────────────────────────────
Γ ⊢ NormalizeLF([CR, LF] ++ U) ⇓ [LF] ++ V

**(Norm-CR)**
U = [] ∨ U[0] ≠ LF    Γ ⊢ NormalizeLF(U) ⇓ V
──────────────────────────────────────────────────────────
Γ ⊢ NormalizeLF([CR] ++ U) ⇓ [LF] ++ V

**(Norm-LF)**
Γ ⊢ NormalizeLF(U) ⇓ V
────────────────────────────────────────────
Γ ⊢ NormalizeLF([LF] ++ U) ⇓ [LF] ++ V

**(Norm-Other)**
c ≠ CR    c ≠ LF    Γ ⊢ NormalizeLF(U) ⇓ V
────────────────────────────────────────────
Γ ⊢ NormalizeLF([c] ++ U) ⇓ [c] ++ V

**Logical Line Map.**

Utf8Offsets([]) = [0]
Utf8Offsets(c::cs) = [0] ++ [o + Utf8Len(c) | o ∈ Utf8Offsets(cs)]

LineStarts(T) = [0] ++ [Utf8Offsets(T)[i] + 1 | 0 ≤ i < |T| ∧ T[i] = LF]
LineCount(T) = |LineStarts(T)|

**Locate (Line/Column).**
L = S.line_starts
o' = min(o, S.byte_len)
k = max{ j | L[j] ≤ o' }

Γ ⊢ Locate(S, o) ⇓ ⟨file = S.path, offset = o', line = k + 1, col = o' - L[k] + 1⟩

#### 3.1.5. Prohibited Code Points

Prohibited(c) ⇔ General_Category(c) = Cc ∧ c ∉ {U+0009, U+000A, U+000C, U+000D}

LiteralSpan(T) = ⋃ { [ByteOf(T, i), ByteOf(T, j)) | StringRange(T, i, j) ∨ CharRange(T, i, j) }

**(WF-Prohibited)**
∀ i, Prohibited(T[i]) ⇒ ByteOf(T, i) ∈ LiteralSpan(T)
───────────────────────────────────────────────────────
Γ ⊢ T : NoProhibited

#### 3.1.6. NFC Normalization for Identifiers and Module Paths

NFC(s) = UnicodeNFC_{15.0.0}(s)

CaseFold(s) = UnicodeCaseFold_{15.0.0}(s)

**Totality.**
The functions NFC and CaseFold are total on sequences of Unicode scalar values. All inputs to IdKey and PathKey MUST be Unicode scalar sequences; inputs are produced by LoadSource, which rejects invalid UTF-8.

IdKey(s) = NFC(s)
IdEq(s_1, s_2) ⇔ IdKey(s_1) = IdKey(s_2)

PathKey(p) = [NFC(c_1), …, NFC(c_n)]
PathEq(p, q) ⇔ PathKey(p) = PathKey(q)

#### 3.1.7. Newline Tokens and Statement Termination

Tokenize : SourceFile ⇀ (Token* × DocComment*)
Tokenize(S) = (K, D) ⇒ LexNewline(K, S) ∧ LexNoComments(K, S)

Depth(K, 0) = 0
Depth(K, i+1) = Depth(K, i) + δ(K[i])
δ(t) =
 1   if t ∈ {Punctuator("("), Punctuator("["), Punctuator("{")}
 -1  if t ∈ {Punctuator(")"), Punctuator("]"), Punctuator("}")}
 0   otherwise

Prev(K, i) = ⊥ ⇔ { j | j < i ∧ K[j].kind ≠ newline ∧ ∀ k. j < k < i ⇒ K[k].kind ≠ newline } = ∅
Prev(K, i) = K[j] ⇔ j = max{ j | j < i ∧ K[j].kind ≠ newline ∧ ∀ k. j < k < i ⇒ K[k].kind ≠ newline }
Next(K, i) = ⊥ ⇔ { j | j > i ∧ K[j].kind ≠ newline } = ∅
Next(K, i) = K[j] ⇔ j = min{ j | j > i ∧ K[j].kind ≠ newline }

Ambig = {"+", "-", "*", "&", "|"}
BeginsOperand(t) ⇔ t.kind ∈ {Identifier, IntLiteral, FloatLiteral, StringLiteral, CharLiteral, BoolLiteral, NullLiteral} ∨ (t.kind = Punctuator ∧ t.lexeme ∈ {"(", "[", "{"}) ∨ (t.kind = Operator ∧ t.lexeme ∈ {"!", "-", "&", "*", "^"}) ∨ (t.kind = Keyword ∧ t.lexeme ∈ {"if", "match", "loop", "unsafe", "move", "transmute", "widen"})
UnaryOnly = {"!", "~", "?"}

Continue(K, i) ⇔ Depth(K, i) > 0 ∨ (∃ t. Prev(K, i) = t ∧ (t.lexeme = "," ∨ (t.kind = Operator ∧ ((t.lexeme ∈ Ambig ∧ ∃ u. Next(K, i) = u ∧ BeginsOperand(u)) ∨ t.lexeme ∉ UnaryOnly)))) ∨ (∃ u. Next(K, i) = u ∧ u.lexeme ∈ {".", "::", "~>"})

Filter(K) = [ K[i] | K[i].kind ≠ newline ∨ ¬ Continue(K, i) ]

IsTerminator(t) ⇔ t = Punctuator(";") ∨ t.kind = newline
BoundaryTokens(K, i) = { t | t = K[i] ∨ t = Prev(K, i) ∨ t = Next(K, i) } \ {⊥}
HasTerminator(F, i) ⇔ ∃ t ∈ BoundaryTokens(F, i). IsTerminator(t)
RequiredTerminator : Token* × ℕ → Bool
ContinuesLine : Token* × ℕ → Bool
ContinuesLine(K, i) ⇔ K[i].kind = newline ∧ Continue(K, i)
RequiredTerminator(K, i) ⇔ K[i].kind = newline ∧ ¬ ContinuesLine(K, i)

**(Missing-Terminator-Err)**
RequiredTerminator(K, i)    ¬ HasTerminator(Filter(K), i)    c = Code(Missing-Terminator-Err)
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Emit(c)


#### 3.1.8. Source Loading Pipeline (Small-Step and Big-Step)

SourceLoadState = {Start(f, B), Sized(f, B), Decoded(f, B, U), BomStripped(f, B, U, b, j), Normalized(f, B, T, j), LineMapped(f, B, T, L), Validated(S), Error(code)}
B ∈ Bytes
U ∈ Scalars
T ∈ Scalars
L = LineStarts(T)
j ∈ ℕ ∪ {⊥}

**(Step-Size)**
────────────────────────────────────────
⟨Start(f, B)⟩ → ⟨Sized(f, B)⟩

**(Step-Decode)**
Γ ⊢ Decode(B) ⇓ U
────────────────────────────────────────────
⟨Sized(f, B)⟩ → ⟨Decoded(f, B, U)⟩

**(Step-Decode-Err)**
Γ ⊢ Decode(B) ⇑
───────────────────────────────────────────────────────────
⟨Sized(f, B)⟩ → ⟨Error(Code(Step-Decode-Err))⟩

**(Step-BOM)**
Γ ⊢ StripBOM(U) ⇓ (U', b, j)
────────────────────────────────────────────────
⟨Decoded(f, B, U)⟩ → ⟨BomStripped(f, B, U', b, j)⟩

**(Step-Norm)**
T = NormalizeOutsideIdentifiers(U)    Γ ⊢ NormalizeLF(T) ⇓ V
─────────────────────────────────────────────────────────────
⟨BomStripped(f, B, U, b, j)⟩ → ⟨Normalized(f, B, V, j)⟩

**(Step-EmbeddedBOM-Err)**
j ≠ ⊥
───────────────────────────────────────────────────────────
⟨Normalized(f, B, T, j)⟩ → ⟨Error(Code(Step-EmbeddedBOM-Err))⟩

**(Step-LineMap)**
j = ⊥    L = LineStarts(T)
────────────────────────────────────────────────────────────
⟨Normalized(f, B, T, j)⟩ → ⟨LineMapped(f, B, T, L)⟩

**(Step-Prohibited)**
Γ ⊢ T : NoProhibited    S = ⟨path = f, bytes = B, scalars = T, text = EncodeUTF8(T), byte_len = ByteLen(T), line_starts = L, line_count = |L|⟩
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨LineMapped(f, B, T, L)⟩ → ⟨Validated(S)⟩

**(Step-Prohibited-Err)**
Γ ⊬ T : NoProhibited
────────────────────────────────────────────────────────────
⟨LineMapped(f, B, T, L)⟩ → ⟨Error(Code(Step-Prohibited-Err))⟩

**Source Load (Big-Step)**

**(LoadSource-Ok)**
Γ ⊢ Decode(B) ⇓ U    Γ ⊢ StripBOM(U) ⇓ (U', b, ⊥)    Γ ⊢ NormalizeLF(NormalizeOutsideIdentifiers(U')) ⇓ T    L = LineStarts(T)    Γ ⊢ T : NoProhibited    S = ⟨path = f, bytes = B, scalars = T, text = EncodeUTF8(T), byte_len = ByteLen(T), line_starts = L, line_count = |L|⟩
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LoadSource(f, B) ⇓ S

**(LoadSource-Err)**
Γ ⊢ LoadSource(f, B) →* ⟨Error(c)⟩
──────────────────────────────────────
Γ ⊢ LoadSource(f, B) ⇑ c

#### 3.1.9. Diagnostic Spans for Source Loading

S_tmp = ⟨path = f, bytes = B, text = EncodeUTF8(T), byte_len = ByteLen(T), line_starts = LineStarts(T), line_count = |LineStarts(T)|⟩

O = Utf8Offsets(T)
O[|T|] = ByteLen(T)

SpanAtIndex(T, i) = SpanOf(S_tmp, O[i], O[i+1])

SpanAtLineStart(T, k) = SpanOf(S_tmp, s, e)
s =
 LineStarts(T)[k]  if k < |LineStarts(T)|
 ByteLen(T)        otherwise
e = min(s + 1, ByteLen(T))

If b = true, the warning W-SRC-0101 MUST be emitted even if LoadSource ultimately fails.


**(Span-BOM-Warn)**
b = true    e = min(1, ByteLen(T))
──────────────────────────────────────────────
Γ ⊢ Emit(W-SRC-0101, SpanOf(S_tmp, 0, e))

**(Span-BOM-Embedded)**
j ≠ ⊥    i = min{ p | 0 ≤ p < |T| ∧ T[p] = U+FEFF }
───────────────────────────────────────────────────────────────────
Γ ⊢ Emit(E-SRC-0103, SpanAtIndex(T, i))

**(Span-Prohibited)**
i = min{ p | 0 ≤ p < |T| ∧ Prohibited(T[p]) ∧ O[p] ∉ LiteralSpan(T) }
─────────────────────────────────────────────────────────────────────────────
Γ ⊢ Emit(E-SRC-0104, SpanAtIndex(T, i))

**(NoSpan-Decode)**
────────────────────────────────────────
Γ ⊢ Emit(E-SRC-0101, ⊥)

### 3.2. Lexical Analysis

#### 3.2.1. Inputs, Outputs, and Records

**LexerInput.**
T = S.scalars
B = S.text
n = S.byte_len
LexerInput(S) = ⟨T, B, n⟩

**LexerOutput.**
LexerOutput(S) = ⟨K, D⟩
K ∈ Token*    D ∈ DocComment*

**EOF Token.**
EOFSpan(S) = SpanOfText(S, |T|, |T|)
TokenEOF(S) = ⟨EOF, ε, EOFSpan(S)⟩

**LexemeBinding.**
TokenRange(S, t) = (i, j) ⇔ t.span = SpanOfText(S, i, j)
LexemeBinding(S, T, K) ⇔ ∀ t ∈ K. ∃ i, j. TokenRange(S, t) = (i, j) ∧ t.lexeme = Lexeme(T, i, j)

**DocComment.**

DocComment = ⟨kind, text, span⟩

DocKind = {LineDoc, ModuleDoc}
StripLeadingSpace(s) =
 s[1..|s|)  if |s| > 0 ∧ At(s, 0) = U+0020
 s          otherwise
DocBody(T, i, j) = StripLeadingSpace(T[i+3..j))
DocMarker(T, i) =
 LineDoc    if T[i..i+3] = "///"
 ModuleDoc  if T[i..i+3] = "//!"
 ⊥          otherwise

**Newline Tokens.**
NewlineTokenAt(S, T, i) ⇔ T[i] = LF ∧ ¬ InsideLiteralOrComment(i)
LexNewline(K, S) ⇔ ∀ i. NewlineTokenAt(S, T, i) ⇒ ∃ t ∈ K. t.kind = Newline ∧ TokenRange(S, t) = (i, i+1)
TokenInComment(S, t) ⇔ ∃ i, j, a, b. TokenRange(S, t) = (i, j) ∧ (LineCommentRange(T, a, b) ∨ BlockCommentRange(T, a, b)) ∧ a ≤ i ∧ j ≤ b
LexNoComments(K, S) ⇔ ∀ t ∈ K. ¬ TokenInComment(S, t)

**Indices and Lexemes.**
T = S.scalars
O = Utf8Offsets(T)
ScalarIndex(T) = { i | 0 ≤ i ≤ |T| }

ByteOf(T, i) = O[i]

SpanOfText(S, i, j) = SpanOf(S, ByteOf(T, i), ByteOf(T, j))

Lexeme(T, i, j) = T[i..j)

#### 3.2.2. Character Classes

c ∈ UnicodeScalar

**Whitespace.**

Whitespace(c) ⇔ c ∈ {U+0020, U+0009, U+000C}

**Line Feed.**

LineFeed(c) ⇔ c = U+000A

**Identifier Characters.**
XID_Start : UnicodeScalar → Bool
XID_Continue : UnicodeScalar → Bool

IdentStart(c) ⇔ c = '_' ∨ XID_Start(c)
IdentContinue(c) ⇔ c = '_' ∨ XID_Continue(c)

XIDVersion = "15.0.0"
XID_Start(c) ⇔ c ∈ UAX31_XID_Start_{15.0.0}
XID_Continue(c) ⇔ c ∈ UAX31_XID_Continue_{15.0.0}

**Non-Characters.**

NonCharacter(c) ⇔ c ∈ [U+FDD0, U+FDEF] ∨ (c & 0xFFFF) ∈ {0xFFFE, 0xFFFF}

**Digits.**

DecDigit(c) ⇔ c ∈ {'0' … '9'}
HexDigit(c) ⇔ DecDigit(c) ∨ c ∈ {'a' … 'f', 'A' … 'F'}
OctDigit(c) ⇔ c ∈ {'0' … '7'}
BinDigit(c) ⇔ c ∈ {'0', '1'}

**Lexically Sensitive Characters.**

Sensitive(c) ⇔ c ∈ {U+202A … U+202E, U+2066 … U+2069, U+200C, U+200D}

#### 3.2.3. Reserved Lexemes

**Reserved.**
Reserved = {`as`, `break`, `class`, `continue`, `else`, `enum`, `false`, `defer`, `frame`, `if`, `imm`, `import`, `internal`, `let`, `loop`, `match`, `modal`, `move`, `mut`, `null`, `private`, `procedure`, `protected`, `public`, `record`, `region`, `result`, `return`, `shadow`, `shared`, `transition`, `transmute`, `true`, `type`, `unique`, `unsafe`, `var`, `widen`, `using`, `const`, `override`}

FutureReserved = ∅

**Keyword Predicate.**
Keyword(s) ⇔ s ∈ Reserved

**Reserved Namespaces.**
ReservedNamespacePrefix = {`cursive.`}
ReservedIdentPrefix = {`gen_`}
ReservedNamespacePhase = Phase3

**Universe-Protected Bindings.**
UniverseProtected = {`i8`, `i16`, `i32`, `i64`, `i128`, `u8`, `u16`, `u32`, `u64`, `u128`, `f16`, `f32`, `f64`, `bool`, `char`, `usize`, `isize`, `Self`, `Drop`, `Bitcopy`, `Clone`, `string`, `bytes`, `Modal`, `Region`, `RegionOptions`, `Context`, `System`, `Async`, `Future`, `Sequence`, `Stream`, `Pipe`, `Exchange`}
UniverseProtectedPhase = Phase3

#### 3.2.4. Token Kinds

TokenKind ∈ {Identifier, Keyword(k), IntLiteral, FloatLiteral, StringLiteral, CharLiteral, BoolLiteral, NullLiteral, Operator(o), Punctuator(p), Newline, Unknown}

**Operator Set.**
OperatorSet = {"+", "-", "*", "/", "%", "**", "==", "!=", "<", "<=", ">", ">=", "&&", "||", "!", "&", "|", "^", "<<", ">>", "=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>=", ":=", "<:", "..", "..=", "=>", "->", "::", "~", "~>", "~!", "?", "#", "@", "$"}

**Punctuator Set.**
PunctuatorSet = {"(", ")", "[", "]", "{", "}", ",", ":", ";", "."}

OperatorSet ∩ PunctuatorSet = ∅

#### 3.2.5. Comment and Whitespace Scanning

T = S.scalars

**Line Comment Scan.**

**(Scan-Line-Comment)**
T[i] = '/'    T[i+1] = '/'    j = min{ p | i ≤ p ∧ (p = |T| ∨ T[p] = LF) }
────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ScanLineComment(T, i) ⇓ j

**Doc Comment Classification.**
kind = DocMarker(T, i)
body = DocBody(T, i, j)

**(Doc-Comment)**
Γ ⊢ ScanLineComment(T, i) ⇓ j    kind ≠ ⊥
──────────────────────────────────────────────────────────────
Γ ⊢ DocComment(T, i) ⇓ ⟨kind, body, SpanOfText(S, i, j)⟩

LineCommentTokens(T, i) = []
LineCommentNext(T, i) = j where Γ ⊢ ScanLineComment(T, i) ⇓ j

**Block Comment Scan (Nested).**
BlockState = { BlockScan(T, i, d, i_0) | 0 ≤ i_0 ≤ i ≤ |T| ∧ d ∈ ℕ } ∪ { BlockDone(j) | 0 ≤ j ≤ |T| }

**(Block-Start)**
T[i] = '/'    T[i+1] = '*'
───────────────────────────────────────────────────────────
⟨BlockScan(T, i, d, i_0)⟩ → ⟨BlockScan(T, i+2, d+1, i_0)⟩

**(Block-End)**
T[i] = '*'    T[i+1] = '/'    d > 1
───────────────────────────────────────────────────────────
⟨BlockScan(T, i, d, i_0)⟩ → ⟨BlockScan(T, i+2, d-1, i_0)⟩

**(Block-Done)**
T[i] = '*'    T[i+1] = '/'    d = 1
───────────────────────────────────────────────────────
⟨BlockScan(T, i, d, i_0)⟩ → ⟨BlockDone(i+2)⟩

**(Block-Step)**
T[i..i+2] ≠ "/*"    T[i..i+2] ≠ "*/"
───────────────────────────────────────────────────────────
⟨BlockScan(T, i, d, i_0)⟩ → ⟨BlockScan(T, i+1, d, i_0)⟩

**(Block-Comment-Unterminated)**
⟨BlockScan(T, i, d, i_0)⟩ →* ⟨BlockScan(T, |T|, d, i_0)⟩    d > 0    c = Code(Block-Comment-Unterminated)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Emit(c, SpanOfText(S, i_0, i_0+2))

#### 3.2.6. Literal Lexing

T = S.scalars

**Syntax.**

```ebnf
integer_literal  ::= (decimal_integer | hex_integer | octal_integer | binary_integer) int_suffix?
decimal_integer  ::= dec_digit ("_"* dec_digit)*
hex_integer      ::= "0x" hex_digit ("_"* hex_digit)*
octal_integer    ::= "0o" oct_digit ("_"* oct_digit)*
binary_integer   ::= "0b" bin_digit ("_"* bin_digit)*
int_suffix       ::= "i8" | "i16" | "i32" | "i64" | "i128" | "u8" | "u16" | "u32" | "u64" | "u128" | "isize" | "usize"

float_literal ::= decimal_integer "." decimal_integer? exponent? float_suffix?
exponent      ::= ("e" | "E") ("+" | "-")? decimal_integer
float_suffix  ::= "f16" | "f32" | "f64"

string_literal   ::= '"' (string_char | escape_sequence)* '"'
string_char      ::= string_char_unit
escape_sequence  ::= "\n" | "\r" | "\t" | "\\" | "\"" | "\'" | "\0" | "\x" hex_digit hex_digit | "\u{" hex_digit+ "}"

char_literal ::= "'" (char_content | escape_sequence) "'"
char_content ::= char_content_unit

bool_literal ::= "true" | "false"
null_literal ::= "null"
```

**Escape Validity.**
SimpleEscape = {`\\`, `\"`, `\'`, `\n`, `\r`, `\t`, `\0`}
EscapeOk(`\\`) ∧ EscapeOk(`\"`) ∧ EscapeOk(`\'`) ∧ EscapeOk(`\n`) ∧ EscapeOk(`\r`) ∧ EscapeOk(`\t`) ∧ EscapeOk(`\0`)
EscapeOk("\\x" h_1 h_2) ⇔ HexDigit(h_1) ∧ HexDigit(h_2)
EscapeOk("\\u{" h_1 … h_n "}") ⇔ 1 ≤ n ≤ 6 ∧ UnicodeScalar(HexValue(h_1 … h_n))

StringChar(c) ⇔ UnicodeScalar(c) ∧ c ≠ "\"" ∧ c ≠ "\\" ∧ c ≠ U+000A
CharContent(c) ⇔ UnicodeScalar(c) ∧ c ≠ "'" ∧ c ≠ "\\" ∧ c ≠ U+000A
string_char_unit = { c | StringChar(c) }
char_content_unit = { c | CharContent(c) }

**Underscore Constraints.**
BasePrefix = {"0x", "0o", "0b"}
IntSuffixSet = {"i8", "i16", "i32", "i64", "i128", "u8", "u16", "u32", "u64", "u128", "isize", "usize"}
FloatSuffixSet = {"f16", "f32", "f64"}
NumSuffix = IntSuffixSet ∪ FloatSuffixSet
StartsWithUnderscore(s) ⇔ At(s, 0) = "_"
EndsWithUnderscore(s) ⇔ At(s, |s|-1) = "_"
AfterBasePrefixUnderscore(s) ⇔ ∃ p ∈ BasePrefix. StartsWith(s, Concat(p, "_"))
AdjacentExponentUnderscore(s) ⇔ ∃ i. At(s, i) = "_" ∧ ((i > 0 ∧ At(s, i-1) ∈ {"e", "E"}) ∨ (i+1 < |s| ∧ At(s, i+1) ∈ {"e", "E"}))
BeforeSuffixUnderscore(s) ⇔ ∃ suf ∈ NumSuffix. EndsWith(s, Concat("_", suf))
NumericUnderscoreOk(s) ⇔ ¬ StartsWithUnderscore(s) ∧ ¬ EndsWithUnderscore(s) ∧ ¬ AfterBasePrefixUnderscore(s) ∧ ¬ AdjacentExponentUnderscore(s) ∧ ¬ BeforeSuffixUnderscore(s)

**Numeric Scan (Maximal Prefix).**

DecRun(T, i) = max({i} ∪ { j | i < j ≤ |T| ∧ ∀ k ∈ [i, j). (DecDigit(T[k]) ∨ T[k] = "_") })
HexRun(T, i) = max({i} ∪ { j | i < j ≤ |T| ∧ ∀ k ∈ [i, j). (HexDigit(T[k]) ∨ T[k] = "_") })
OctRun(T, i) = max({i} ∪ { j | i < j ≤ |T| ∧ ∀ k ∈ [i, j). (OctDigit(T[k]) ∨ T[k] = "_") })
BinRun(T, i) = max({i} ∪ { j | i < j ≤ |T| ∧ ∀ k ∈ [i, j). (BinDigit(T[k]) ∨ T[k] = "_") })

SuffixMatch(T, i, U) = max({i} ∪ { j | i < j ≤ |T| ∧ Lexeme(T, i, j) ∈ U })

ExpSignEnd(T, i) =
 i+1  if i < |T| ∧ T[i] ∈ {"+", "-"}
 i    otherwise

ExpEnd(T, i) =
 DecRun(T, ExpSignEnd(T, i+1))  if i < |T| ∧ T[i] ∈ {"e", "E"}
 i                              otherwise

DecCoreEnd(T, i) =
 ExpEnd(T, q)  if p = DecRun(T, i) ∧ p < |T| ∧ T[p] = "." ∧ q = DecRun(T, p+1)
 ExpEnd(T, p)  if p = DecRun(T, i) ∧ (p ≥ |T| ∨ T[p] ≠ ".")

NumericCoreEnd(T, i) =
 HexRun(T, i+2)  if T[i..i+2] = "0x"
 OctRun(T, i+2)  if T[i..i+2] = "0o"
 BinRun(T, i+2)  if T[i..i+2] = "0b"
 DecCoreEnd(T, i)  otherwise

NumericScanEnd(T, i) = SuffixMatch(T, NumericCoreEnd(T, i), NumSuffix)

HasDot(T, i, j) ⇔ ∃ p. i ≤ p < j ∧ T[p] = "."
HasExp(T, i, j) ⇔ ∃ p. i ≤ p < j ∧ T[p] ∈ {"e", "E"}

NumericKind(T, i) =
 FloatLiteral  if SuffixMatch(T, NumericCoreEnd(T, i), FloatSuffixSet) > NumericCoreEnd(T, i)
 FloatLiteral  if HasDot(T, i, NumericCoreEnd(T, i)) ∨ HasExp(T, i, NumericCoreEnd(T, i))
 IntLiteral    otherwise

**(Lex-Int)**
DecDigit(T[i])    j = NumericScanEnd(T, i)    NumericKind(T, i) = IntLiteral
───────────────────────────────────────────────────────────────────────────────
Γ ⊢ IntLiteral(T, i) ⇓ j

**(Lex-Float)**
DecDigit(T[i])    j = NumericScanEnd(T, i)    NumericKind(T, i) = FloatLiteral
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ FloatLiteral(T, i) ⇓ j

NumericLexemeOk(T, i, j) ⇔ (Lexeme(T, i, j) matches integer_literal ∨ Lexeme(T, i, j) matches float_literal) ∧ NumericUnderscoreOk(Lexeme(T, i, j))
NumericLexemeBad(T, i, j) ⇔ ¬ NumericLexemeOk(T, i, j)

**(Lex-Numeric-Err)**
DecDigit(T[i])    j = NumericScanEnd(T, i)    NumericLexemeBad(T, i, j)    c = Code(Lex-Numeric-Err)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Emit(c, SpanOfText(S, i, j))

**Leading Zeros.**
Digits(s) = Remove(s, "_")
DecimalLeadingZero(T, i, j) ⇔ Lexeme(T, i, j) matches decimal_integer ∧ |Digits(Lexeme(T, i, j))| > 1 ∧ At(Digits(Lexeme(T, i, j)), 0) = '0'

DecimalLeadingZero(T, i, j)
──────────────────────────────────────────────
Γ ⊢ Emit(W-SRC-0301, SpanOfText(S, i, j))

**EscapeSequences.**
EscapeValue(`\\`) = 0x5C
EscapeValue(`\"`) = 0x22
EscapeValue(`\'`) = 0x27
EscapeValue(`\n`) = 0x0A
EscapeValue(`\r`) = 0x0D
EscapeValue(`\t`) = 0x09
EscapeValue(`\0`) = 0x00
EscapeValue("\\x" h_1 h_2) = HexValue(h_1 h_2)
EscapeValue("\\u{" h_1 … h_n "}") = EncodeUTF8(HexValue(h_1 … h_n))

**(Lex-String)**
Lexeme(T, i, j) matches string_literal
────────────────────────────────────────────
Γ ⊢ StringLiteral(T, i) ⇓ j

BackslashCount(T, p) = max{ k | 0 ≤ k ≤ p ∧ ∀ r ∈ [p-k, p). T[r] = "\\" }
UnescapedQuote(T, p) ⇔ T[p] = "\"" ∧ BackslashCount(T, p) mod 2 = 0
StringTerminator(T, i) = min{ q | q > i ∧ (UnescapedQuote(T, q) ∨ T[q] = LF ∨ q = |T|) }
LineFeedOrEOFBeforeClose(T, i) ⇔ StringTerminator(T, i) = |T| ∨ T[StringTerminator(T, i)] = LF
EscapeMatch(T, p, q) ⇔ Lexeme(T, p, q) matches escape_sequence ∧ EscapeOk(Lexeme(T, p, q))
BadEscapeAt(T, p) ⇔ T[p] = "\\" ∧ ¬ ∃ q. EscapeMatch(T, p, q)
FirstBadEscape(T, i) = min{ p | i < p < StringTerminator(T, i) ∧ BadEscapeAt(T, p) }

**(Lex-String-Unterminated)**
LineFeedOrEOFBeforeClose(T, i)    c = Code(Lex-String-Unterminated)
────────────────────────────────────────────────────────────────────
Γ ⊢ Emit(c, SpanOfText(S, i, i+1))

**(Lex-String-BadEscape)**
FirstBadEscape(T, i) = p    c = Code(Lex-String-BadEscape)
───────────────────────────────────────────────────────────────
Γ ⊢ Emit(c, SpanOfText(S, p, p+1))

**(Lex-Char)**
Lexeme(T, i, j) matches char_literal
─────────────────────────────────────────
Γ ⊢ CharLiteral(T, i) ⇓ j

**Character Literal Encoding.**
CharValueRange = { x | 0 ≤ x ≤ 0x10FFFF ∧ x ∉ [0xD800, 0xDFFF] }
CharRepr = `u32`
SizeOf(`char`) = 4
AlignOf(`char`) = 4

UnescapedApostrophe(T, p) ⇔ T[p] = "'" ∧ BackslashCount(T, p) mod 2 = 0
CharTerminator(T, i) = min{ q | q > i ∧ (UnescapedApostrophe(T, q) ∨ T[q] = LF ∨ q = |T|) }
CharLiteralInvalid(T, i) ⇔ CharScalarCount(T, i) ≠ 1
CharScalarCountFrom(T, p, q) = 0 ⇔ p ≥ q
CharScalarCountFrom(T, p, q) = 1 + CharScalarCountFrom(T, p+1, q) ⇔ p < q ∧ T[p] ≠ "\\"
CharScalarCountFrom(T, p, q) = 1 + CharScalarCountFrom(T, r, q) ⇔ p < q ∧ T[p] = "\\" ∧ EscapeMatch(T, p, r)
CharScalarCountFrom(T, p, q) = 1 + CharScalarCountFrom(T, p+1, q) ⇔ p < q ∧ T[p] = "\\" ∧ ¬ ∃ r. EscapeMatch(T, p, r)
CharScalarCount(T, i) = CharScalarCountFrom(T, i+1, CharTerminator(T, i))

**(Lex-Char-Unterminated)**
LineFeedOrEOFBeforeClose(T, i)    c = Code(Lex-Char-Unterminated)
─────────────────────────────────────────────────────────────────
Γ ⊢ Emit(c, SpanOfText(S, i, i+1))

**(Lex-Char-BadEscape)**
FirstBadEscape(T, i) = p    c = Code(Lex-Char-BadEscape)
─────────────────────────────────────────────────────────
Γ ⊢ Emit(c, SpanOfText(S, p, p+1))

**(Lex-Char-Invalid)**
CharLiteralInvalid(T, i)    c = Code(Lex-Char-Invalid)
────────────────────────────────────────────────────────
Γ ⊢ Emit(c, SpanOfText(S, i, i+1))

**Literal Tokenization Helpers.**

StringTok(T, i) = { (StringLiteral, j) | StringLiteral(T, i) ⇓ j }
CharTok(T, i) = { (CharLiteral, j) | CharLiteral(T, i) ⇓ j }
IntTok(T, i) = { (IntLiteral, j) | IntLiteral(T, i) ⇓ j }
FloatTok(T, i) = { (FloatLiteral, j) | FloatLiteral(T, i) ⇓ j }

#### 3.2.7. Identifier and Keyword Lexing

T = S.scalars

**Identifier Scan.**
IdentScanEnd(T, i) = min{ j | j > i ∧ (¬ IdentContinue(T[j]) ∨ j = |T|) ∧ ∀ k ∈ (i, j). IdentContinue(T[k]) }

**(Lex-Identifier)**
IdentStart(T[i])    j = IdentScanEnd(T, i)    s = Lexeme(T, i, j)
────────────────────────────────────────────────────────────────
Γ ⊢ Ident(T, i) ⇓ (s, j)

**(Lex-Ident-InvalidUnicode)**
k = min{ p | i ≤ p < j ∧ NonCharacter(T[p]) }    c = Code(Lex-Ident-InvalidUnicode)
────────────────────────────────────────────────────────────────────────────
Γ ⊢ Emit(c, SpanOfText(S, k, k+1))

**(Lex-Ident-Token)**
Γ ⊢ Ident(T, i) ⇓ (s, j)    Γ ⊢ ClassifyIdent(s) ⇓ k
──────────────────────────────────────────────────────────────
Γ ⊢ IdentToken(T, i) ⇓ (k, j)

**Keyword Classification.**
ClassifyIdent(s) =
 BoolLiteral  if s ∈ {"true", "false"}
 NullLiteral  if s = "null"
 Keyword(s)   if Keyword(s)
 Identifier   otherwise

#### 3.2.8. Operator and Punctuator Lexing

OpSet = OperatorSet
PuncSet = PunctuatorSet

OpMatch(T, i) = { (o, j) | o ∈ OpSet ∧ Lexeme(T, i, j) = o }
PuncMatch(T, i) = { (p, j) | p ∈ PuncSet ∧ Lexeme(T, i, j) = p }

OpTok(T, i) = { (Operator(o), j) | (o, j) ∈ OpMatch(T, i) }
PuncTok(T, i) = { (Punctuator(p), j) | (p, j) ∈ PuncMatch(T, i) }

#### 3.2.9. Maximal-Munch Rule

T = S.scalars

IsQuote(c) ⇔ c ∈ {"\"", "'"}
Candidates(T, i) =
 StringTok(T, i) ∪ CharTok(T, i)  if IsQuote(T[i])
 FloatTok(T, i) ∪ IntTok(T, i)    if DecDigit(T[i])
 IdentToken(T, i)                 if IdentStart(T[i])
 OpTok(T, i) ∪ PuncTok(T, i)       if OpTok(T, i) ≠ ∅ ∨ PuncTok(T, i) ≠ ∅
 ∅                                otherwise

Longest(C) = { (k, j) ∈ C | ∀ (k', j') ∈ C, j ≥ j' }

KindPriority(IntLiteral) = 3
KindPriority(FloatLiteral) = 3
KindPriority(StringLiteral) = 3
KindPriority(CharLiteral) = 3
KindPriority(BoolLiteral) = 3
KindPriority(NullLiteral) = 3
KindPriority(Identifier) = 2
KindPriority(Keyword(_)) = 2
KindPriority(Operator(_)) = 1
KindPriority(Punctuator(_)) = 0

PickLongest(C) = argmax_{(k, j) ∈ C} ⟨j, KindPriority(k)⟩

**(Max-Munch)**
PickLongest(C) = (k, j)
──────────────────────────────
Γ ⊢ NextToken(T, i) ⇓ (k, j)

**(Max-Munch-Err)**
Candidates(T, i) = ∅    c = Code(Max-Munch-Err)
────────────────────────────────────────────────
Γ ⊢ NextToken(T, i) ⇑ c

SpanOfErr(Max-Munch-Err, S, i) = SpanOfText(S, i, i+1)

GenericCloseException = false

#### 3.2.10. Lexical Security

T = S.scalars
O = Utf8Offsets(T)

**Literal/Comment Ranges.**
LineCommentRange(T, i, j) ⇔ Γ ⊢ ScanLineComment(T, i) ⇓ j
BlockCommentRange(T, i, j) ⇔ T[i..i+2] = "/*" ∧ ⟨BlockScan(T, i, 0, i)⟩ →* ⟨BlockDone(j)⟩
StringRange(T, i, j) ⇔ Γ ⊢ StringLiteral(T, i) ⇓ j
CharRange(T, i, j) ⇔ Γ ⊢ CharLiteral(T, i) ⇓ j
InsideLiteralOrComment(i) ⇔ ∃ a, b. a ≤ i < b ∧ (LineCommentRange(T, a, b) ∨ BlockCommentRange(T, a, b) ∨ StringRange(T, a, b) ∨ CharRange(T, a, b))

**Sensitive Positions in a Span.**

SensitiveInSpan(T, i, j) = [ p | i ≤ p < j ∧ Sensitive(T[p]) ]

**Unsafe Spans (Token-Only).**

IsLBrace(t) ⇔ t.kind = Punctuator("{")
IsRBrace(t) ⇔ t.kind = Punctuator("}")

NextNonNewline(K, i) = ⊥ ⇔ { j | j ≥ i ∧ K[j].kind ≠ Newline } = ∅
NextNonNewline(K, i) = j ⇔ j = min{ j | j ≥ i ∧ K[j].kind ≠ Newline }

MatchBrace(K, j) = min{ k | k > j ∧ Balance(j, k) = 0 ∧ ∀ m ∈ (j, k), Balance(j, m) > 0 }

Balance(K, j, m) = |{ x | j ≤ x ≤ m ∧ IsLBrace(K[x]) }| - |{ x | j ≤ x ≤ m ∧ IsRBrace(K[x]) }|
MatchBrace(K, j) = ⊥ ⇔ { k | k > j ∧ Balance(K, j, k) = 0 ∧ ∀ m ∈ (j, k). Balance(K, j, m) > 0 } = ∅

SpanFrom(t_a, t_b) = ⟨t_a.span.file, t_a.span.start_offset, t_b.span.end_offset, t_a.span.start_line, t_a.span.start_col, t_b.span.end_line, t_b.span.end_col⟩

UnsafeSpans(K) = { SpanFrom(K[j], K[k]) | K[i].kind = Keyword("unsafe"), j = NextNonNewline(K, i+1), K[j].kind = Punctuator("{"), k = MatchBrace(K, j), k ≠ ⊥ }

UnsafeAtByte(b) ⇔ ∃ sp ∈ UnsafeSpans(K). b ∈ SpanRange(sp)

UnsafeSpanMode = TokenOnly

**Lexical Security Check.**
Sens = [ p | Sensitive(T[p]) ∧ ¬ InsideLiteralOrComment(p) ]

**(LexSecure-Err)**
i = min{ p | p ∈ Sens ∧ ¬ UnsafeAtByte(ByteOf(T, p)) }    c = Code(LexSecure-Err)
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LexSecure(S, K, Sens) ⇑ c

**(LexSecure-Warn)**
∀ p ∈ Sens, UnsafeAtByte(ByteOf(T, p))    Γ ⊢ EmitList(LexSecureWarns(S, Sens))
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LexSecure(S, K, Sens) ⇓ ok

LexSecureWarns(S, Sens) = [ ⟨W-SRC-0308, SpanOfText(S, p, p+1)⟩ | p ∈ Sens ]
LexSecureErrSpan(S, i) = SpanOfText(S, i, i+1)

#### 3.2.11. Tokenization (Small-Step)

LexState = {LexStart(S), LexScan(S, i, K, D, Sens), LexDone(K, D, Sens), LexError(code)}
T = S.scalars
|T| = len(T)

**(Lex-Start)**
────────────────────────────────────────────
⟨LexStart(S)⟩ → ⟨LexScan(S, 0, [], [], [])⟩

**(Lex-End)**
i ≥ |T|
──────────────────────────────────────────────
⟨LexScan(S, i, K, D, Sens)⟩ → ⟨LexDone(K, D, Sens)⟩

**(Lex-Whitespace)**
Whitespace(T[i])
───────────────────────────────────────────────
⟨LexScan(S, i, K, D, Sens)⟩ → ⟨LexScan(S, i+1, K, D, Sens)⟩

**(Lex-Newline)**
T[i] = LF
──────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨LexScan(S, i, K, D, Sens)⟩ → ⟨LexScan(S, i+1, K ++ [⟨newline, Lexeme(T, i, i+1), SpanOfText(S, i, i+1)⟩], D, Sens)⟩

**(Lex-Line-Comment)**
T[i..i+2] = "//"    T[i..i+3] ∉ {"///", "//!"}    Γ ⊢ ScanLineComment(T, i) ⇓ j
──────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨LexScan(S, i, K, D, Sens)⟩ → ⟨LexScan(S, j, K, D, Sens)⟩

**(Lex-Doc-Comment)**
T[i..i+3] ∈ {"///", "//!"}    Γ ⊢ ScanLineComment(T, i) ⇓ j    Γ ⊢ DocComment(T, i) ⇓ d
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨LexScan(S, i, K, D, Sens)⟩ → ⟨LexScan(S, j, K, D ++ [d], Sens)⟩

**(Lex-Block-Comment)**
T[i..i+2] = "/*"    ⟨BlockScan(T, i, 0, i)⟩ →* ⟨BlockDone(j)⟩
──────────────────────────────────────────────────────────────────────────────
⟨LexScan(S, i, K, D, Sens)⟩ → ⟨LexScan(S, j, K, D, Sens)⟩

**(Lex-String-Unterminated-Recover)**
T[i] = "\""    LineFeedOrEOFBeforeClose(T, i)    c = Code(Lex-String-Unterminated)    Γ ⊢ Emit(c, SpanOfText(S, i, i+1))    j = StringTerminator(T, i)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨LexScan(S, i, K, D, Sens)⟩ → ⟨LexScan(S, j, K, D, Sens)⟩

**(Lex-Char-Unterminated-Recover)**
T[i] = "'"    LineFeedOrEOFBeforeClose(T, i)    c = Code(Lex-Char-Unterminated)    Γ ⊢ Emit(c, SpanOfText(S, i, i+1))    j = CharTerminator(T, i)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨LexScan(S, i, K, D, Sens)⟩ → ⟨LexScan(S, j, K, D, Sens)⟩

**(Lex-Sensitive)**
Sensitive(T[i])
────────────────────────────────────────────────────────────────────
⟨LexScan(S, i, K, D, Sens)⟩ → ⟨LexScan(S, i+1, K, D, Sens ++ [i])⟩

SensitiveTok(T, i, j, k) =
 []                    if k ∈ {StringLiteral, CharLiteral}
 SensitiveInSpan(T, i, j)  otherwise

**(Lex-Token)**
¬ Whitespace(T[i])    T[i] ≠ LF    T[i..i+2] ∉ {"//", "/*"}    ¬ Sensitive(T[i])    Γ ⊢ NextToken(T, i) ⇓ (k, j)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨LexScan(S, i, K, D, Sens)⟩ → ⟨LexScan(S, j, K ++ [⟨k, Lexeme(T, i, j), SpanOfText(S, i, j)⟩], D, Sens ++ SensitiveTok(T, i, j, k))⟩

**(Lex-Token-Err)**
¬ Whitespace(T[i])    T[i] ≠ LF    T[i..i+2] ∉ {"//", "/*"}    ¬ (T[i] = "\"" ∧ LineFeedOrEOFBeforeClose(T, i))    ¬ (T[i] = "'" ∧ LineFeedOrEOFBeforeClose(T, i))    ¬ Sensitive(T[i])    Γ ⊢ NextToken(T, i) ⇑ c
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨LexScan(S, i, K, D, Sens)⟩ → ⟨LexError(c)⟩

#### 3.2.12. Tokenize (Big-Step)

**(Tokenize-Ok)**
⟨LexStart(S)⟩ →* ⟨LexDone(K, D, Sens)⟩    Γ ⊢ LexSecure(S, K, Sens) ⇓ ok
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ Tokenize(S) ⇓ (K, D)

**(Tokenize-Secure-Err)**
⟨LexStart(S)⟩ →* ⟨LexDone(K, D, Sens)⟩    Γ ⊢ LexSecure(S, K, Sens) ⇑ c
─────────────────────────────────────────────────────────────────────────────
Γ ⊢ Tokenize(S) ⇑ c

**(Tokenize-Err)**
⟨LexStart(S)⟩ →* ⟨LexError(c)⟩
────────────────────────────────────
Γ ⊢ Tokenize(S) ⇑ c

Phase1LexDiagRefs = {"8.4", "8.5", "8.8"}

### 3.3. Grammar, Parsing, and AST Construction

#### 3.3.1. Inputs, Outputs, and Invariants

**Inputs.**
ParseInputs(U, K, D, K') ⇔ U = [S_1, …, S_n] ∧ K = [K_1, …, K_n] ∧ D = [D_1, …, D_n] ∧ (∀ i. Γ ⊢ Tokenize(S_i) ⇓ (K_i, D_i)) ∧ K' = [Filter(K_i) | 1 ≤ i ≤ n]
ParseUnitSources(U) ⇔ ∃ d, f_1, …, f_n, B_1, …, B_n. CompilationUnit(d) = [f_1, …, f_n] ∧ U = [S_1, …, S_n] ∧ ∧_{i=1}^n (Γ ⊢ ReadBytes(f_i) ⇓ B_i ∧ Γ ⊢ LoadSource(f_i, B_i) ⇓ S_i)

**Outputs.**
ParseOutputs(U, F) ⇔ Γ ⊢ ParseUnit(U) ⇓ F
F = [F_1, …, F_n]

ModuleAggregationRef = {"3.4.1", "3.4.2"}

**Parsing Phase Invariants.**

**(Phase1-Complete)**
∀ i, Γ ⊢ ParseFile(S_i) ⇓ F_i
────────────────────────────────────────
Γ ⊢ ParseUnit(U) ⇓ [F_1, …, F_n]

PhaseExec(Phase1, U) ⇔ Γ ⊢ ParseUnit(U) ⇓ F
∀ p ∈ {Phase2, Phase3, Phase4}. PhaseExec(p, U) ⇒ PhaseExec(Phase1, U)

**(Phase1-Declarations)**
Γ ⊢ ParseUnit(U) ⇓ [F_1, …, F_n]    ∀ i, F_i.items = I_i
──────────────────────────────────────────────────────────────
Γ ⊢ AllDecls(U) = I_1 ++ … ++ I_n

**(Phase1-Forward-Refs)**
──────────────────────────────────────────────────────────────
Γ ⊢ ParsePhase(U) ⇓ NoResolutionConstraints

#### 3.3.2. AST Node Catalog (Cursive0 Subset)

ASTNode = ASTItem ∪ ASTExpr ∪ ASTPattern ∪ ASTType ∪ ASTStmt
SpanOfNode : ASTNode → Span
DocOf : ASTNode → (DocList ∪ {⊥})

SpanDefault(P, P') = SpanBetween(P, P')
DocDefault = []
DocOptDefault = ⊥
FillSpan(n, P, P') =
 n[span := SpanDefault(P, P')]  if SpanMissing(n)
 n                             otherwise
FillDoc(n) =
 n[doc := DocDefault]  if DocMissing(n)
 n                     otherwise
FillDocOpt(n) =
 n[doc_opt := DocOptDefault]  if DocOptMissing(n)
 n                            otherwise
ParseCtor(n, P, P') = FillDocOpt(FillDoc(FillSpan(n, P, P')))
DocAssociationRef = {"3.3.11"}

**Doc Lists.**

DocList = [DocComment]
DocCommentRef = {"3.2.1"}

**Paths.**

Path = [identifier]
ModulePath = Path
TypePath = Path
ClassPath = Path
PathString(p) = StringOfPath(p)
StringOfPathRef = {"3.4.1"}

##### 3.3.2.1. Module and Files

**ASTModule.**

ASTModule = ⟨path, items, module_doc⟩
ASTModule.path ∈ Path
ASTModule.items ∈ [ASTItem]
ASTModule.module_doc ∈ DocList

**ASTFile.**

ASTFile = ⟨path, items, module_doc⟩
ASTFile.path ∈ Path
ASTFile.items ∈ [ASTItem]
ASTFile.module_doc ∈ DocList

##### 3.3.2.2. Items

ASTItem ∈ {UsingDecl, StaticDecl, ProcedureDecl, RecordDecl, EnumDecl, ModalDecl, ClassDecl, TypeAliasDecl, ErrorItem}

**UsingDecl.**

UsingDecl = ⟨vis, clause, span, doc⟩

**UsingClause.**

UsingClause ∈ {UsingPath = ⟨path, alias_opt⟩, UsingList = ⟨module_path, specs⟩}

UsingSpec = ⟨name, alias_opt⟩

**StaticDecl.**

StaticDecl = ⟨vis, mut, binding, span, doc⟩

mut ∈ {`let`, `var`}

**ProcedureDecl.**

ProcedureDecl = ⟨vis, name, params, return_type_opt, body, span, doc⟩

**RecordDecl.**

RecordDecl = ⟨vis, name, implements, members, span, doc⟩

RecordDecl.implements ∈ [ClassPath]

RecordMember ∈ {FieldDecl = ⟨vis, name, type, init_opt, span, doc_opt⟩, MethodDecl = ⟨vis, override, name, receiver, params, return_type_opt, body, span, doc_opt⟩}
Receiver ∈ {ReceiverShorthand(perm), ReceiverExplicit(mode_opt, type)}
perm ∈ {`const`, `unique`, `shared`}
mode_opt ∈ {`move`, ⊥}

**EnumDecl.**

EnumDecl = ⟨vis, name, implements, variants, span, doc⟩

EnumDecl.implements ∈ [ClassPath]

VariantDecl = ⟨name, payload_opt, discriminant_opt, span, doc_opt⟩

VariantPayload ∈ {TuplePayload = [Type], RecordPayload = [FieldDecl]}
∀ f ∈ RecordPayload. f.init_opt = ⊥

**ModalDecl.**

ModalDecl = ⟨vis, name, implements, states, span, doc⟩

ModalDecl.implements ∈ [ClassPath]

StateBlock = ⟨name, members, span, doc_opt⟩

StateMember ∈ {StateFieldDecl = ⟨vis, name, type, span, doc_opt⟩, StateMethodDecl = ⟨vis, name, params, return_type_opt, body, span, doc_opt⟩, TransitionDecl = ⟨vis, name, params, target_state, body, span, doc_opt⟩}

**ClassDecl.**

ClassDecl = ⟨vis, name, supers, items, span, doc⟩

ClassDecl.supers ∈ [ClassPath]

ClassItem ∈ {ClassFieldDecl = ⟨vis, name, type, span, doc_opt⟩, ClassMethodDecl = ⟨vis, name, receiver, params, return_type_opt, body_opt, span, doc_opt⟩}
AbstractClassMethod(m) ⇔ ∃ vis, name, recv, params, ret, span, doc. m = ClassMethodDecl(vis, name, recv, params, ret, ⊥, span, doc)
ConcreteClassMethod(m) ⇔ ∃ vis, name, recv, params, ret, body, span, doc. m = ClassMethodDecl(vis, name, recv, params, ret, body, span, doc) ∧ body ≠ ⊥

**TypeAliasDecl.**

TypeAliasDecl = ⟨vis, name, type, span, doc⟩
AliasBodyRef = {"6.1.4"}
AliasStep(TypePath(p)) = AliasBody(p) if defined; otherwise TypePath(p)
AliasStep(T) = T if T ∉ {TypePath(p)}
AliasNorm(T) =
 TypePerm(perm, AliasNorm(base))  if T = TypePerm(perm, base)
 TypeTuple([AliasNorm(t) | t ∈ elems])  if T = TypeTuple(elems)
 TypeArray(AliasNorm(elem), size_expr)  if T = TypeArray(elem, size_expr)
 TypeSlice(AliasNorm(elem))  if T = TypeSlice(elem)
 TypeUnion([AliasNorm(t) | t ∈ members])  if T = TypeUnion(members)
 TypeFunc([⟨m, AliasNorm(t)⟩ | ⟨m, t⟩ ∈ params], AliasNorm(ret))  if T = TypeFunc(params, ret)
 TypeDynamic(AliasPath(path))  if T = TypeDynamic(path)
 TypeModalState(AliasPath(path), state)  if T = TypeModalState(path, state)
 TypePtr(AliasNorm(elem), ptr_state_opt)  if T = TypePtr(elem, ptr_state_opt)
 TypeRawPtr(qual, AliasNorm(elem))  if T = TypeRawPtr(qual, elem)
 AliasNorm(AliasStep(T))  if T = TypePath(p)
 T  otherwise
AliasPath(p) = p if AliasBody(p) undefined
AliasPath(p) = AliasPath(p') if AliasBody(p) = TypePath(p')
AliasTransparent(T, U) ⇔ AliasNorm(T) = AliasNorm(U)
AliasGraph = { ⟨p, q⟩ | AliasBody(p) = T ∧ q ∈ TypePaths(T) }
TypePaths(TypePrim(_)) = ∅
TypePaths(TypeRange) = ∅
TypePaths(TypePerm(_, T)) = TypePaths(T)
TypePaths(TypeTuple([T_1, …, T_n])) = ⋃_{i=1}^n TypePaths(T_i)
TypePaths(TypeArray(T, _)) = TypePaths(T)
TypePaths(TypeSlice(T)) = TypePaths(T)
TypePaths(TypeUnion([T_1, …, T_n])) = ⋃_{i=1}^n TypePaths(T_i)
TypePaths(TypeFunc([⟨_, T_1⟩, …, ⟨_, T_n⟩], R)) = (⋃_{i=1}^n TypePaths(T_i)) ∪ TypePaths(R)
TypePaths(TypePtr(T, _)) = TypePaths(T)
TypePaths(TypeRawPtr(_, T)) = TypePaths(T)
TypePaths(TypeString(_)) = ∅
TypePaths(TypeBytes(_)) = ∅
TypePaths(TypeDynamic(p)) = {p}
TypePaths(TypeModalState(p, _)) = {p}
TypePaths(TypePath(p)) = {p}
AliasCycle(p) ⇔ p ∈ Reach^+(AliasGraph, p)
**(TypeAlias-Recursive-Err)**
AliasCycle(p)    c = Code(TypeAlias-Recursive-Err)
────────────────────────────────────────────────────
Γ ⊢ p : TypeAliasOk ⇑ c
**(TypeAlias-Ok)**
¬ AliasCycle(p)
────────────────────────
Γ ⊢ p : TypeAliasOk

**ErrorItem.**

ErrorItem = ⟨span⟩
IsDecl(ErrorItem(_)) = false

##### 3.3.2.3. Types

**Type.**
Type = {TypePerm(perm, base), TypePrim(name), TypeTuple(elems), TypeArray(elem, size_expr), TypeSlice(elem), TypeUnion(members), TypeFunc(params, ret), TypePath(path), TypeDynamic(path), TypeString(string_state_opt), TypeBytes(bytes_state_opt), TypeModalState(path, state), TypePtr(elem, ptr_state_opt), TypeRawPtr(qual, elem), TypeRange}

Perm = {`const`, `unique`, `shared`}
Qual = {`imm`, `mut`}
PtrStateOpt = {⊥, `Valid`, `Null`, `Expired`}
StringStateOpt = {⊥, `@Managed`, `@View`}
BytesStateOpt = {⊥, `@Managed`, `@View`}
Name ∈ PrimTypes_C0

ParamMode = {`move`, ⊥}
ParamType = ⟨mode, type⟩ where mode ∈ ParamMode ∧ type ∈ Type

TypeRangeSyntax = ⊥

**Range Record.**
RangeFieldType(`kind`) = `u8`
RangeFieldType(`lo`) = `usize`
RangeFieldType(`hi`) = `usize`

##### 3.3.2.4. Expressions

**Literal Tokens.**
LiteralKind = {IntLiteral, FloatLiteral, StringLiteral, CharLiteral, BoolLiteral, NullLiteral}
LiteralToken = { t ∈ Token | t.kind ∈ LiteralKind }

**Expr.**
RangeKind = {`To`, `ToInclusive`, `Full`, `From`, `Exclusive`, `Inclusive`}
Expr = {Literal(lit), PtrNullExpr, Identifier(name), QualifiedName(path, name), QualifiedApply(path, name, form), Path(path, name), ErrorExpr(span), TupleExpr(elems), ArrayExpr(elems), RecordExpr(type_ref, fields), EnumLiteral(path, payload_opt), FieldAccess(base, name), TupleAccess(base, index), IndexAccess(base, index_expr), Call(callee, args), MethodCall(base, name, args), Unary(op, expr), Binary(op, left, right), Cast(expr, type), Range(kind, lo_opt, hi_opt), IfExpr(cond, then_block, else_opt), MatchExpr(scrutinee, arms), LoopInfinite(body), LoopConditional(cond, body), LoopIter(pattern, type_opt, iter, body), BlockExpr(stmts, tail_opt), UnsafeBlockExpr(body), MoveExpr(place), TransmuteExpr(src_type, dst_type, expr), AllocExpr(region_opt, expr), Propagate(expr), AddressOf(place), Deref(expr)}
ExprSpan : Expr → Span

TypeRef = {TypePath(path), ModalStateRef(path, state)}

**Qualified Expressions.**
ParenForm = {Paren(args) | args ∈ Arg*}
BraceForm = {Brace(fields) | fields ∈ FieldInit*}
QualForm = ParenForm ∪ BraceForm
∀ P, P', e. (Γ ⊢ ParseExpr(P) ⇓ (P', e)) ⇒ e ∉ {Path(_, _), EnumLiteral(_, _)}

**Argument.**
Arg = ⟨moved, expr, span⟩ where moved ∈ {true, false}

MovedArg(moved, e) =
 MoveExpr(e)  if moved = true ∧ IsPlace(e)
 e            otherwise

**Field Initializer.**
FieldInit = ⟨name, expr⟩

##### 3.3.2.5. Patterns

Pattern = {LiteralPattern(lit), WildcardPattern, IdentifierPattern(name), TypedPattern(name, type), TuplePattern(elems), RecordPattern(type_path, fields), EnumPattern(type_path, name, payload_opt), ModalPattern(state_name, fields_opt), RangePattern(kind, lo, hi)}
PatternSpan : Pattern → Span

FieldPattern = ⟨name, pattern_opt, span⟩

EnumPayloadPattern = {TuplePayloadPattern([Pattern]), RecordPayloadPattern([FieldPattern])}

ModalPayloadPattern = {ModalRecordPayload([FieldPattern])}

##### 3.3.2.6. Statements

Stmt = {LetStmt(binding), VarStmt(binding), ErrorStmt(span), ShadowLetStmt(name, type_opt, init), ShadowVarStmt(name, type_opt, init), AssignStmt(place, expr), CompoundAssignStmt(place, op, expr), ExprStmt(expr), DeferStmt(block), RegionStmt(opts_opt, alias_opt, block), FrameStmt(target_opt, block), ReturnStmt(expr_opt), ResultStmt(expr), BreakStmt(expr_opt), ContinueStmt, UnsafeBlockStmt(block)}

binding = ⟨pattern, type_opt, op, init, span⟩
opts_opt ∈ {⊥} ∪ Expr    alias_opt ∈ {⊥} ∪ Identifier
target_opt ∈ {⊥} ∪ Identifier

##### 3.3.2.7. Unsupported Grammar Families (Cursive0 Decision)

UnsupportedGrammarFamily = {`attributes`, `extern_ffi`, `generics`, `contracts`, `keys`, `concurrency`, `async`, `metaprogramming`}
UnsupportedGrammarFamily ⊆ UnsupportedForm

#### 3.3.3. Parser State and Judgments

**Parser State.**

PState = ⟨K, i, D, j, d, Δ⟩

TokStream(P) = K
TokIndex(P) = i
DocStream(P) = D
DocIndex(P) = j
Depth(P) = d
DiagStream(P) = Δ

**Helper Functions.**

Tok(P) =
 K[i]                        if i < |K|
 ⟨EOF, ε, EOFSpan(K)⟩         if i = |K|

SourceOf(K) = S ⇔ Γ ⊢ Tokenize(S) ⇓ (K_raw, D) ∧ K = Filter(K_raw)
EOFSpan(K) = EOFSpan(SourceOf(K))

Advance(P) = ⟨K, i+1, D, j, d, Δ⟩
Clone(P) = ⟨K, i, D, j, d, []⟩
MergeDiag(P_b, P_d, P_s) = ⟨TokStream(P_s), TokIndex(P_s), DocStream(P_s), DocIndex(P_s), Depth(P_s), DiagStream(P_b) ++ DiagStream(P_d)⟩

**Parser Index Invariant.**
PStateOk(P) ⇔ 0 ≤ i ≤ |K|

AdvanceOrEOF(P) =
 Advance(P)  if i < |K|
 P           if i = |K|

LastConsumed(P, P') =
 K[i'-1]  if i' > i
 Tok(P)   if i' = i
SpanBetween(P, P') = SpanFrom(Tok(P), LastConsumed(P, P'))

SplitSpan2(sp) = (sp_L, sp_R) where
 sp_L.file = sp.file ∧ sp_R.file = sp.file
 sp_L.start_offset = sp.start_offset ∧ sp_L.end_offset = sp.start_offset + 1
 sp_R.start_offset = sp.start_offset + 1 ∧ sp_R.end_offset = sp.start_offset + 2
 sp_L.start_line = sp.start_line ∧ sp_L.end_line = sp.start_line
 sp_R.start_line = sp.start_line ∧ sp_R.end_line = sp.start_line
 sp_L.start_col = sp.start_col ∧ sp_L.end_col = sp.start_col + 1
 sp_R.start_col = sp.start_col + 1 ∧ sp_R.end_col = sp.start_col + 2

SplitShiftR(P) = ⟨K', i, D, j, d, Δ⟩
where Tok(P) = ⟨Operator(">>"), ">>", sp⟩ ∧ (sp_L, sp_R) = SplitSpan2(sp)
K' = K[0..i) ++ [⟨Operator(">"), ">", sp_L⟩, ⟨Operator(">"), ">", sp_R⟩] ++ K[i+1..]

**Judgments (Big-Step).**
ParseJudgment = {ParseFile, ParseModule, ParseItem, ParseStmt, ParseExpr, ParsePattern, ParseType}

#### 3.3.4. Grammar Subset and Cursive0 Lexeme Policy

**Lexeme Predicates.**
IsIdent(t) ⇔ t.kind = Identifier
IsKw(t, s) ⇔ t.kind = Keyword(s)
IsOp(t, s) ⇔ t.kind = Operator(s)
IsPunc(t, s) ⇔ t.kind = Punctuator(s)
Lexeme(t) = t.lexeme

**Contextual Keywords.**
CtxKeyword = {"in"}
Ctx(t, s) ⇔ IsIdent(t) ∧ Lexeme(t) = s ∧ s ∈ CtxKeyword
¬ Ctx(t, "as") ∧ ¬ Ctx(t, "move")

**Cursive0 Declaration Keywords.**
UsingKeyword = "using"
Keyword("use") = false

**Union Propagation.**
UnionPropTok(t) ⇔ IsOp(t, "?")
UnionPropForm(e) ⇔ ∃ e_0. e = Propagate(e_0)

**Cursive0 Type Restrictions.**
TypeWhereTok(t) ⇔ IsIdent(t) ∧ Lexeme(t) = "where"
OpaqueTypeTok(t) ⇔ IsIdent(t) ∧ Lexeme(t) = "opaque"
TypeArgsUnsupported(P) ⇔ Γ ⊢ ParseTypePath(P) ⇓ (P_1, path) ∧ IsOp(Tok(P_1), "<") ∧ path ≠ [`Ptr`]
C0TypeRestricted(P) ⇔ TypeArgsUnsupported(P) ∨ TypeWhereTok(Tok(P)) ∨ OpaqueTypeTok(Tok(P))

**Syntax (Cursive0 Subset).**

```ebnf
module            ::= item*
item              ::= using_decl | static_decl | procedure_decl | record_decl | enum_decl | modal_decl | class_decl | type_alias_decl
using_decl         ::= visibility? "using" using_clause
using_clause       ::= using_path ("as" identifier)? | using_list
path               ::= identifier ("::" identifier)*
module_path        ::= path
using_path         ::= path
type_path          ::= path
using_list         ::= module_path "{" using_specifier ("," using_specifier)* "}"
using_specifier    ::= identifier ("as" identifier)?

static_decl        ::= visibility? ("let" | "var") binding_decl
binding_decl       ::= pattern (":" type)? binding_op expression
procedure_decl     ::= visibility? "procedure" identifier signature block_expr
signature          ::= "(" param_list? ")" ("->" type)?
param_list         ::= param ("," param)*
param              ::= param_mode? identifier ":" type
param_mode         ::= "move"
record_decl        ::= visibility? "record" identifier implements_clause? "{" record_body? "}"
enum_decl          ::= visibility? "enum" identifier implements_clause? "{" variant_list? "}"
modal_decl         ::= visibility? "modal" identifier implements_clause? "{" state_block+ "}"
class_decl         ::= visibility? "class" identifier ("<:" superclass_bounds)? "{" class_body? "}"
type_alias_decl    ::= visibility? "type" identifier "=" type

record_body        ::= record_member ("," record_member)*
record_member      ::= record_field_decl | method_def
method_def         ::= visibility? "override"? "procedure" identifier "(" receiver ("," param_list)? ")" ("->" type)? block_expr
receiver           ::= receiver_shorthand | explicit_receiver
receiver_shorthand ::= "~" | "~!"
explicit_receiver  ::= param_mode? "self" ":" type

implements_clause  ::= "<:" class_list
class_list         ::= class_path ("," class_path)*
class_path         ::= type_path

state_block        ::= "@" state_name "{" state_member* "}"
state_name         ::= identifier
state_member       ::= state_field_decl | state_method_def | transition_def
state_field_decl   ::= visibility? identifier ":" type
state_method_def   ::= visibility? "procedure" identifier "(" param_list ")" ("->" type)? block_expr
transition_def     ::= visibility? "transition" identifier "(" param_list ")" "->" "@" target_state block_expr
target_state       ::= identifier

superclass_bounds  ::= class_path ("+" class_path)*
class_body         ::= class_item*
class_item         ::= class_method_decl | class_field_decl
class_method_decl  ::= visibility? "procedure" identifier "(" receiver ("," param_list)? ")" ("->" type)? class_method_body
class_method_body  ::= block_expr | terminator
class_field_decl   ::= visibility? identifier ":" type terminator

visibility         ::= "public" | "internal" | "private" | "protected"

record_field_decl_list ::= record_field_decl ("," record_field_decl)*
record_field_decl  ::= visibility? identifier ":" type record_field_init_opt?
record_field_init_opt ::= "=" expression
field_decl_list    ::= field_decl ("," field_decl)*
field_decl         ::= visibility? identifier ":" type
variant_list       ::= variant ("," variant)*
variant            ::= identifier variant_payload? ("=" integer_literal)?
variant_payload    ::= "(" type_list? ")" | "{" field_decl_list? "}"
type_list          ::= type ("," type)*

statement          ::= binding_stmt | shadow_binding | assignment_stmt | expr_stmt | defer_stmt | region_stmt | frame_stmt | return_stmt | result_stmt | break_stmt | continue_stmt | unsafe_block
binding_stmt       ::= ("let" | "var") pattern (":" type)? binding_op expression terminator
shadow_binding     ::= "shadow" ("let" | "var") identifier (":" type)? "=" expression terminator
assignment_stmt    ::= place_expr "=" expression terminator
compound_assign    ::= place_expr compound_op expression terminator
expr_stmt          ::= expression terminator
defer_stmt         ::= "defer" block_expr
region_stmt        ::= "region" region_opts? region_alias? block_expr
region_opts        ::= "(" expression ")"
region_alias       ::= "as" identifier
frame_stmt         ::= "frame" block_expr | identifier "." "frame" block_expr
return_stmt        ::= "return" expression? terminator?
result_stmt        ::= "result" expression terminator?
break_stmt         ::= "break" expression? terminator?
continue_stmt      ::= "continue" terminator?
unsafe_block       ::= "unsafe" block_expr

binding_op         ::= "=" | ":="
compound_op        ::= "+=" | "-=" | "*=" | "/=" | "%="
terminator         ::= ";" | newline
newline            ::= "\n"

expression         ::= range_expression | logical_or_expr
range_expression   ::= exclusive_range | inclusive_range | from_range | to_range | to_inclusive_range | full_range
exclusive_range    ::= logical_or_expr ".." logical_or_expr
inclusive_range    ::= logical_or_expr "..=" logical_or_expr
from_range         ::= logical_or_expr ".."
to_range           ::= ".." logical_or_expr
to_inclusive_range ::= "..=" logical_or_expr
full_range         ::= ".."
logical_or_expr    ::= logical_and_expr ("||" logical_and_expr)*
logical_and_expr   ::= comparison_expr ("&&" comparison_expr)*
comparison_expr    ::= bitwise_or_expr (("==" | "!=" | "<" | "<=" | ">" | ">=") bitwise_or_expr)*
bitwise_or_expr    ::= bitwise_xor_expr ("|" bitwise_xor_expr)*
bitwise_xor_expr   ::= bitwise_and_expr ("^" bitwise_and_expr)*
bitwise_and_expr   ::= shift_expr ("&" shift_expr)*
shift_expr         ::= additive_expr (("<<" | ">>") additive_expr)*
additive_expr      ::= multiplicative_expr (("+" | "-") multiplicative_expr)*
multiplicative_expr ::= power_expr (("*" | "/" | "%") power_expr)*
power_expr         ::= cast_expr ("**" power_expr)?
cast_expr          ::= unary_expr ("as" type)?

unary_expr          ::= unary_operator unary_expr | address_of_expr | move_expr | postfix_expr
unary_operator      ::= "!" | "-" | "*" | "widen"
address_of_expr     ::= "&" place_expr
move_expr           ::= "move" place_expr

postfix_expr        ::= primary_expr postfix_suffix*
postfix_suffix      ::= "." identifier | "." decimal_literal | "[" expression "]" | "~>" identifier "(" argument_list? ")" | "(" argument_list? ")" | "?"

primary_expr        ::= literal | null_ptr_expr | identifier_expr | qualified_expr | tuple_literal | array_literal | record_literal | if_expr | match_expr | loop_expr | block_expr | unsafe_expr | transmute_expr
                   | alloc_expr
unsafe_expr         ::= "unsafe" block_expr
transmute_expr      ::= "transmute" "::" "<" type "," type ">" "(" expression ")"
identifier_expr     ::= identifier
qualified_expr      ::= type_path "::" identifier qualified_suffix?
qualified_suffix    ::= "(" argument_list? ")" | "{" field_init_list "}"
null_ptr_expr       ::= "Ptr" "::" "null" "(" ")"
alloc_expr          ::= "^" expression

parenthesized_expr  ::= "(" expression ")"

tuple_literal       ::= "(" tuple_expr_elements? ")"
tuple_expr_elements ::= expression ";" | expression ("," expression)+
array_literal       ::= "[" expression_list "]"
expression_list     ::= expression ("," expression)*
record_literal      ::= (type_path | state_specific_type) "{" field_init_list "}"
field_init_list     ::= field_init ("," field_init)*
field_init          ::= identifier ":" expression | identifier


argument_list       ::= argument ("," argument)*
argument            ::= "move"? expression

if_expr             ::= "if" expression block_expr ("else" (block_expr | if_expr))?
match_expr          ::= "match" expression "{" match_arm ("," match_arm)* "}"
match_arm           ::= pattern ("if" expression)? "=>" arm_body
arm_body            ::= expression | block_expr

loop_expr           ::= "loop" block_expr | "loop" expression block_expr | "loop" pattern (":" type)? "in" expression block_expr

block_expr          ::= "{" statement* expression? "}"

place_expr          ::= "*" place_expr | postfix_expr

pattern             ::= literal_pattern | wildcard_pattern | identifier_pattern | typed_pattern | tuple_pattern | record_pattern | enum_pattern | modal_pattern | range_pattern
literal_pattern     ::= literal
wildcard_pattern    ::= "_"
identifier_pattern  ::= identifier
typed_pattern       ::= identifier ":" type

tuple_pattern       ::= "(" tuple_pattern_elements? ")"
tuple_pattern_elements ::= pattern ";" | pattern ("," pattern)+
record_pattern      ::= type_path "{" field_pattern_list? "}"
field_pattern_list  ::= field_pattern ("," field_pattern)*
field_pattern       ::= identifier (":" pattern)?

enum_pattern        ::= type_path "::" identifier enum_payload_pattern?
enum_payload_pattern ::= "(" tuple_pattern_elements? ")" | "{" field_pattern_list? "}"

modal_pattern       ::= "@" identifier ("{" field_pattern_list? "}")?

range_pattern       ::= pattern (".." | "..=") pattern

type                ::= permission? non_permission_type
permission          ::= "const" | "unique" | "shared"
non_permission_type ::= union_type | non_union_type
union_type          ::= non_union_type ("|" non_union_type)+
non_union_type      ::= primitive_type | tuple_type | function_type | array_type | slice_type | safe_pointer_type | raw_pointer_type | string_type | bytes_type | dynamic_type | state_specific_type | type_path

primitive_type      ::= integer_type | float_type | bool_type | char_type | unit_type | never_type
integer_type        ::= "i8" | "i16" | "i32" | "i64" | "i128" | "u8" | "u16" | "u32" | "u64" | "u128" | "isize" | "usize"
float_type          ::= "f16" | "f32" | "f64"
bool_type           ::= "bool"
char_type           ::= "char"
unit_type           ::= "(" ")"
never_type          ::= "!"

tuple_type          ::= "(" tuple_type_elements? ")"
tuple_type_elements ::= type ";" | type ("," type)+
function_type       ::= "(" param_type_list? ")" "->" type
param_type_list     ::= param_type ("," param_type)*
param_type          ::= "move"? type
array_type          ::= "[" type ";" const_expression "]"
slice_type          ::= "[" type "]"

string_type         ::= "string" ("@" ("Managed" | "View"))?
bytes_type          ::= "bytes" ("@" ("Managed" | "View"))?

state_specific_type ::= type_path "@" state_name

safe_pointer_type   ::= "Ptr" "<" type ">" ("@" pointer_state)?
pointer_state       ::= "Valid" | "Null" | "Expired"
raw_pointer_type    ::= "*" ("imm" | "mut") type

dynamic_type        ::= "$" type_path

const_expression    ::= expression
```

**Method Context (Cursive0).**

RecordMembers(M) = { m | m occurs as RecordMember in M }
ClassItems(M) = { m | m occurs as ClassItem in M }
MethodDecls(M) = { m | m = MethodDecl(…) ∧ m occurs in M }
ClassMethodDecls(M) = { m | m = ClassMethodDecl(…) ∧ m occurs in M }
MethodContextOk(M) ⇔ MethodDecls(M) ⊆ RecordMembers(M) ∧ ClassMethodDecls(M) ⊆ ClassItems(M)

**(Method-Context-Err)**
¬ MethodContextOk(M)    c = Code(Method-Context-Err)
────────────────────────────────────────────────────
Γ ⊢ Emit(c)

#### 3.3.5. Token Consumption (Small-Step, Success-Only)

ConsumeState = {Consume(P, k), ConsumeDone(P)}
ParseRejectRules = ∅

**(Tok-Consume-Kind)**
Tok(P).kind = k
────────────────────────────────────────────────
⟨Consume(P, k)⟩ → ⟨ConsumeDone(Advance(P))⟩

**(Tok-Consume-Keyword)**
IsKw(Tok(P), s)
────────────────────────────────────────────────────────────
⟨Consume(P, Keyword(s))⟩ → ⟨ConsumeDone(Advance(P))⟩

**(Tok-Consume-Operator)**
IsOp(Tok(P), s)
────────────────────────────────────────────────────────────
⟨Consume(P, Operator(s))⟩ → ⟨ConsumeDone(Advance(P))⟩

**(Tok-Consume-Punct)**
IsPunc(Tok(P), s)
────────────────────────────────────────────────────────────
⟨Consume(P, Punctuator(s))⟩ → ⟨ConsumeDone(Advance(P))⟩

**List Parsing (Small-Step)**

ListState = {ListStart(P), ListScan(P, xs), ListDone(P, xs)}

**(List-Start)**
────────────────────────────────────────
⟨ListStart(P)⟩ → ⟨ListScan(P, [])⟩

**(List-Cons)**
Γ ⊢ ParseElem(P) ⇓ (P', x)
──────────────────────────────────────────────────────
⟨ListScan(P, xs)⟩ → ⟨ListScan(P', xs ++ [x])⟩

**(List-Done)**
Tok(P) ∈ EndSet
──────────────────────────────────────
⟨ListScan(P, xs)⟩ → ⟨ListDone(P, xs)⟩

EndSet ⊆ TokenKind
TrailingComma(P, EndSet) ⇔ IsPunc(Tok(P), ",") ∧ Tok(Advance(P)) ∈ EndSet

**(Trailing-Comma-Err)**
TrailingComma(P, EndSet)    c = Code(Unsupported-Construct)
─────────────────────────────────────────────────────────────
Γ ⊢ Emit(c, Tok(P).span)

#### 3.3.6. Module and Item Parsing

**ParseFile (Big-Step).**
Γ ⊢ Tokenize(S) ⇓ (K_raw, D)
K = Filter(K_raw)
P_0 = ⟨K, 0, D, 0, 0, []⟩

**(ParseFile-Ok)**
Γ ⊢ ParseItems(P_0) ⇓ (P_1, I, MDoc)
────────────────────────────────────────────────
Γ ⊢ ParseFile(S) ⇓ ⟨S.path, I, MDoc⟩

ParseModule ∈ RulesIn({"3.4.1", "3.4.2"})

**Item Sequence (Big-Step).**

**(ParseItems-Empty)**
Tok(P) = EOF
──────────────────────────────────────
Γ ⊢ ParseItems(P) ⇓ (P, [], [])

**(ParseItems-Cons)**
Tok(P) ≠ EOF    Γ ⊢ ParseItem(P) ⇓ (P_1, it)    Γ ⊢ ParseItems(P_1) ⇓ (P_2, I, M)
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseItems(P) ⇓ (P_2, [it] ++ I, M)

TopLevelItem ⊆ item

##### 3.3.6.1. Identifiers and Paths

**(Parse-Ident)**
IsIdent(Tok(P))
──────────────────────────────────────────────────────────────
Γ ⊢ ParseIdent(P) ⇓ (Advance(P), Lexeme(Tok(P)))

**(Parse-Ident-Err)**
¬ IsIdent(Tok(P))    c = Code(Parse-Syntax-Err)    Γ ⊢ Emit(c, Tok(P).span)
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseIdent(P) ⇓ (P, "_")

**(Parse-ModulePath)**
Γ ⊢ ParseIdent(P) ⇓ (P_1, id)    Γ ⊢ ParseModulePathTail(P_1, [id]) ⇓ (P_2, path)
───────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseModulePath(P) ⇓ (P_2, path)

**(Parse-ModulePathTail-End)**
¬ IsOp(Tok(P), "::")
──────────────────────────────────────────────
Γ ⊢ ParseModulePathTail(P, xs) ⇓ (P, xs)

**(Parse-ModulePathTail-Cons)**
IsOp(Tok(P), "::")    Γ ⊢ ParseIdent(Advance(P)) ⇓ (P_1, id)    Γ ⊢ ParseModulePathTail(P_1, xs ++ [id]) ⇓ (P_2, ys)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseModulePathTail(P, xs) ⇓ (P_2, ys)

##### 3.3.6.2. Visibility Parsing

**(Parse-Vis-Opt)**
IsKw(Tok(P), v)    v ∈ {`public`, `internal`, `private`, `protected`}
────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseVis(P) ⇓ (Advance(P), v)

**(Parse-Vis-Default)**
¬ IsKw(Tok(P), v)
──────────────────────────────────────────
Γ ⊢ ParseVis(P) ⇓ (P, `internal`)

##### 3.3.6.3. Using Declarations

**(Parse-Using-Path)**
Γ ⊢ ParseVis(P) ⇓ (P_1, vis)    IsKw(Tok(P_1), `using`)    Γ ⊢ ParseModulePath(Advance(P_1)) ⇓ (P_2, path)    Γ ⊢ ParseAliasOpt(P_2) ⇓ (P_3, alias_opt)
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseItem(P) ⇓ (P_3, ⟨UsingDecl, vis, ⟨UsingPath, path, alias_opt⟩, SpanBetween(P, P_3), []⟩)

**(Parse-Using-List)**
Γ ⊢ ParseVis(P) ⇓ (P_1, vis)    IsKw(Tok(P_1), `using`)    Γ ⊢ ParseModulePath(Advance(P_1)) ⇓ (P_2, mp)    IsPunc(Tok(P_2), "{")    Γ ⊢ ParseUsingList(Advance(P_2)) ⇓ (P_3, specs)
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseItem(P) ⇓ (P_3, ⟨UsingDecl, vis, ⟨UsingList, mp, specs⟩, SpanBetween(P, P_3), []⟩)

##### 3.3.6.4. Static Declarations

**(Parse-Static-Decl)**
Γ ⊢ ParseVis(P) ⇓ (P_1, vis)    Tok(P_1) = Keyword(kw)    kw ∈ {`let`, `var`}    mut = kw    Γ ⊢ ParseBindingAfterLetVar(P_1) ⇓ (P_2, bind)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseItem(P) ⇓ (P_2, ⟨StaticDecl, vis, mut, bind, SpanBetween(P, P_2), []⟩)

##### 3.3.6.5. Procedure Declarations

**(Parse-Procedure)**
Γ ⊢ ParseVis(P) ⇓ (P_1, vis)    IsKw(Tok(P_1), `procedure`)    Γ ⊢ ParseIdent(Advance(P_1)) ⇓ (P_2, name)    Γ ⊢ ParseSignature(P_2) ⇓ (P_3, params, ret_opt)    Γ ⊢ ParseBlock(P_3) ⇓ (P_4, body)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseItem(P) ⇓ (P_4, ⟨ProcedureDecl, vis, name, params, ret_opt, body, SpanBetween(P, P_4), []⟩)

##### 3.3.6.6. Record Declarations

**(Parse-Record)**
Γ ⊢ ParseVis(P) ⇓ (P_1, vis)    IsKw(Tok(P_1), `record`)    Γ ⊢ ParseIdent(Advance(P_1)) ⇓ (P_2, name)    Γ ⊢ ParseImplementsOpt(P_2) ⇓ (P_3, impls)    Γ ⊢ ParseRecordBody(P_3) ⇓ (P_4, members)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseItem(P) ⇓ (P_4, ⟨RecordDecl, vis, name, impls, members, SpanBetween(P, P_4), []⟩)

##### 3.3.6.7. Enum Declarations

**(Parse-Enum)**
Γ ⊢ ParseVis(P) ⇓ (P_1, vis)    IsKw(Tok(P_1), `enum`)    Γ ⊢ ParseIdent(Advance(P_1)) ⇓ (P_2, name)    Γ ⊢ ParseImplementsOpt(P_2) ⇓ (P_3, impls)    Γ ⊢ ParseEnumBody(P_3) ⇓ (P_4, variants)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseItem(P) ⇓ (P_4, ⟨EnumDecl, vis, name, impls, variants, SpanBetween(P, P_4), []⟩)

##### 3.3.6.8. Modal Declarations

**(Parse-Modal)**
Γ ⊢ ParseVis(P) ⇓ (P_1, vis)    IsKw(Tok(P_1), `modal`)    Γ ⊢ ParseIdent(Advance(P_1)) ⇓ (P_2, name)    Γ ⊢ ParseImplementsOpt(P_2) ⇓ (P_3, impls)    Γ ⊢ ParseModalBody(P_3) ⇓ (P_4, states)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseItem(P) ⇓ (P_4, ⟨ModalDecl, vis, name, impls, states, SpanBetween(P, P_4), []⟩)

**(Parse-Class)**
Γ ⊢ ParseVis(P) ⇓ (P_1, vis)    IsKw(Tok(P_1), `class`)    Γ ⊢ ParseIdent(Advance(P_1)) ⇓ (P_2, name)    Γ ⊢ ParseSuperclassOpt(P_2) ⇓ (P_3, supers)    Γ ⊢ ParseClassBody(P_3) ⇓ (P_4, items)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseItem(P) ⇓ (P_4, ⟨ClassDecl, vis, name, supers, items, SpanBetween(P, P_4), []⟩)

**(Parse-ModalBody)**
IsPunc(Tok(P), "{")    Γ ⊢ ParseStateBlockList(Advance(P)) ⇓ (P_1, states)    IsPunc(Tok(P_1), "}")
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseModalBody(P) ⇓ (Advance(P_1), states)

**(Parse-StateBlock)**
IsOp(Tok(P), "@")    Γ ⊢ ParseIdent(Advance(P)) ⇓ (P_1, name)    IsPunc(Tok(P_1), "{")    Γ ⊢ ParseStateMemberList(Advance(P_1)) ⇓ (P_2, members)    IsPunc(Tok(P_2), "}")
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseStateBlock(P) ⇓ (Advance(P_2), ⟨name, members, SpanBetween(P, P_2), []⟩)

**(Parse-StateMember-Field)**
Γ ⊢ ParseVis(P) ⇓ (P_1, vis)    Γ ⊢ ParseIdent(P_1) ⇓ (P_2, name)    IsPunc(Tok(P_2), ":")    Γ ⊢ ParseType(Advance(P_2)) ⇓ (P_3, ty)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseStateMember(P) ⇓ (P_3, ⟨StateFieldDecl, vis, name, ty, SpanBetween(P, P_3), []⟩)

**(Parse-StateMember-Method)**
Γ ⊢ ParseVis(P) ⇓ (P_0, vis)    IsKw(Tok(P_0), `procedure`)    Γ ⊢ ParseIdent(Advance(P_0)) ⇓ (P_1, name)    Γ ⊢ ParseSignature(P_1) ⇓ (P_2, params, ret_opt)    Γ ⊢ ParseBlock(P_2) ⇓ (P_3, body)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseStateMember(P) ⇓ (P_3, ⟨StateMethodDecl, vis, name, params, ret_opt, body, SpanBetween(P, P_3), []⟩)

**(Parse-StateMember-Transition)**
Γ ⊢ ParseVis(P) ⇓ (P_0, vis)    IsKw(Tok(P_0), `transition`)    Γ ⊢ ParseIdent(Advance(P_0)) ⇓ (P_1, name)    IsPunc(Tok(P_1), "(")    Γ ⊢ ParseParamList(Advance(P_1)) ⇓ (P_2, params)    IsPunc(Tok(P_2), ")")    P_2' = Advance(P_2)    IsOp(Tok(P_2'), "->")    IsOp(Tok(Advance(P_2')), "@")    Γ ⊢ ParseIdent(Advance(Advance(P_2'))) ⇓ (P_3, target)    Γ ⊢ ParseBlock(P_3) ⇓ (P_4, body)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseStateMember(P) ⇓ (P_4, ⟨TransitionDecl, vis, name, params, target, body, SpanBetween(P, P_4), []⟩)

##### 3.3.6.9. Implements Clauses

**(Parse-Implements-None)**
¬ IsOp(Tok(P), "<:")
──────────────────────────────────────────────
Γ ⊢ ParseImplementsOpt(P) ⇓ (P, [])

**(Parse-Implements-Yes)**
IsOp(Tok(P), "<:")    Γ ⊢ ParseClassList(Advance(P)) ⇓ (P_1, cls)
───────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseImplementsOpt(P) ⇓ (P_1, cls)

**(Parse-ClassList-Cons)**
Γ ⊢ ParseClassPath(P) ⇓ (P_1, c_0)    Γ ⊢ ParseClassListTail(P_1, [c_0]) ⇓ (P_2, cs)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseClassList(P) ⇓ (P_2, cs)

**(Parse-ClassListTail-End)**
¬ IsPunc(Tok(P), ",")
──────────────────────────────────────────────
Γ ⊢ ParseClassListTail(P, cs) ⇓ (P, cs)

**(Parse-ClassListTail-Comma)**
IsPunc(Tok(P), ",")    Γ ⊢ ParseClassPath(Advance(P)) ⇓ (P_1, c)    Γ ⊢ ParseClassListTail(P_1, cs ++ [c]) ⇓ (P_2, cs')
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseClassListTail(P, cs) ⇓ (P_2, cs')


##### 3.3.6.10. Class Declarations

**(Parse-Superclass-None)**
¬ IsOp(Tok(P), "<:")
──────────────────────────────────────────────
Γ ⊢ ParseSuperclassOpt(P) ⇓ (P, [])

**(Parse-Superclass-Yes)**
IsOp(Tok(P), "<:")    Γ ⊢ ParseSuperclassBounds(Advance(P)) ⇓ (P_1, supers)
────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseSuperclassOpt(P) ⇓ (P_1, supers)

**(Parse-SuperclassBounds-Cons)**
Γ ⊢ ParseClassPath(P) ⇓ (P_1, c_0)    Γ ⊢ ParseSuperclassBoundsTail(P_1, [c_0]) ⇓ (P_2, cs)
──────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseSuperclassBounds(P) ⇓ (P_2, cs)

**(Parse-SuperclassBoundsTail-End)**
¬ IsOp(Tok(P), "+")
──────────────────────────────────────────────────────────────
Γ ⊢ ParseSuperclassBoundsTail(P, cs) ⇓ (P, cs)

**(Parse-SuperclassBoundsTail-Plus)**
IsOp(Tok(P), "+")    Γ ⊢ ParseClassPath(Advance(P)) ⇓ (P_1, c)    Γ ⊢ ParseSuperclassBoundsTail(P_1, cs ++ [c]) ⇓ (P_2, cs')
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseSuperclassBoundsTail(P, cs) ⇓ (P_2, cs')

**(Parse-ClassBody)**
IsPunc(Tok(P), "{")    Γ ⊢ ParseClassItemList(Advance(P)) ⇓ (P_1, items)    IsPunc(Tok(P_1), "}")
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseClassBody(P) ⇓ (Advance(P_1), items)

**(Parse-ClassItemList-End)**
IsPunc(Tok(P), "}")
────────────────────────────────────────────
Γ ⊢ ParseClassItemList(P) ⇓ (P, [])

**(Parse-ClassItemList-Cons)**
¬ IsPunc(Tok(P), "}")    Γ ⊢ ParseClassItem(P) ⇓ (P_1, it)    Γ ⊢ ParseClassItemList(P_1) ⇓ (P_2, rest)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseClassItemList(P) ⇓ (P_2, [it] ++ rest)

**(Parse-ClassItem-Method)**
Γ ⊢ ParseVis(P) ⇓ (P_0, vis)    IsKw(Tok(P_0), `procedure`)    Γ ⊢ ParseIdent(Advance(P_0)) ⇓ (P_1, name)    Γ ⊢ ParseMethodSignature(P_1) ⇓ (P_2, receiver, params, ret_opt)    Γ ⊢ ParseClassMethodBody(P_2) ⇓ (P_3, body_opt)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseClassItem(P) ⇓ (P_3, ⟨ClassMethodDecl, vis, name, receiver, params, ret_opt, body_opt, SpanBetween(P, P_3), []⟩)

**(Parse-ClassItem-Field)**
Γ ⊢ ParseVis(P) ⇓ (P_0, vis)    Γ ⊢ ParseIdent(P_0) ⇓ (P_1, name)    IsPunc(Tok(P_1), ":")    Γ ⊢ ParseType(Advance(P_1)) ⇓ (P_2, ty)    Γ ⊢ ConsumeTerminatorReq(P_2) ⇓ P_3
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseClassItem(P) ⇓ (P_3, ⟨ClassFieldDecl, vis, name, ty, SpanBetween(P, P_3), []⟩)

**(Parse-ClassMethodBody-Concrete)**
IsPunc(Tok(P), "{")    Γ ⊢ ParseBlock(P) ⇓ (P_1, body)
──────────────────────────────────────────────────────────
Γ ⊢ ParseClassMethodBody(P) ⇓ (P_1, body)

**(Parse-ClassMethodBody-Abstract)**
¬ IsPunc(Tok(P), "{")    Γ ⊢ ConsumeTerminatorReq(P) ⇓ P_1
──────────────────────────────────────────────────────────
Γ ⊢ ParseClassMethodBody(P) ⇓ (P_1, ⊥)

UnsupportedClassItem = {`modal_class`, `type_item`, `abstract_state`, `override_in_class`}
UnsupportedClassItem ⊆ UnsupportedForm

##### 3.3.6.11. Type Alias Declarations

**(Parse-Type-Alias)**
Γ ⊢ ParseVis(P) ⇓ (P_1, vis)    IsKw(Tok(P_1), `type`)    Γ ⊢ ParseIdent(Advance(P_1)) ⇓ (P_2, name)    IsOp(Tok(P_2), "=")    Γ ⊢ ParseType(Advance(P_2)) ⇓ (P_3, ty)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseItem(P) ⇓ (P_3, ⟨TypeAliasDecl, vis, name, ty, SpanBetween(P, P_3), []⟩)

UnsupportedWhereClause = {`where_clause`}
UnsupportedWhereClause ⊆ UnsupportedForm

##### 3.3.6.13. Item Helper Parsing Rules

**Type Paths.**

**(Parse-TypePath)**
Γ ⊢ ParseIdent(P) ⇓ (P_1, id)    Γ ⊢ ParseTypePathTail(P_1, [id]) ⇓ (P_2, path)
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseTypePath(P) ⇓ (P_2, path)


**(Parse-ClassPath)**
Γ ⊢ ParseTypePath(P) ⇓ (P_1, path)
────────────────────────────────────────────
Γ ⊢ ParseClassPath(P) ⇓ (P_1, path)

**(Parse-TypePathTail-End)**
¬ IsOp(Tok(P), "::")
────────────────────────────────────────────
Γ ⊢ ParseTypePathTail(P, xs) ⇓ (P, xs)

**(Parse-TypePathTail-Cons)**
IsOp(Tok(P), "::")    Γ ⊢ ParseIdent(Advance(P)) ⇓ (P_1, id)    Γ ⊢ ParseTypePathTail(P_1, xs ++ [id]) ⇓ (P_2, ys)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseTypePathTail(P, xs) ⇓ (P_2, ys)

**Qualified Head.**


**(Parse-QualifiedHead)**
Γ ⊢ ParseIdent(P) ⇓ (P_1, id_0)    IsOp(Tok(P_1), "::")    Γ ⊢ ParseModulePathTail(P_1, [id_0]) ⇓ (P_2, xs)    xs = ys ++ [name]    |xs| ≥ 2
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseQualifiedHead(P) ⇓ (P_2, ys, name)

**Using Lists.**


**(Parse-UsingSpec)**
Γ ⊢ ParseIdent(P) ⇓ (P_1, name)    Γ ⊢ ParseAliasOpt(P_1) ⇓ (P_2, alias_opt)
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseUsingSpec(P) ⇓ (P_2, ⟨name, alias_opt⟩)

**Bindings.**

**(Parse-BindingAfterLetVar)**
Tok(P) = kw ∈ {Keyword(`let`), Keyword(`var`)}    Γ ⊢ ParsePattern(Advance(P)) ⇓ (P_1, pat)    Γ ⊢ ParseTypeAnnotOpt(P_1) ⇓ (P_2, ty_opt)    Tok(P_2) ∈ {Operator("="), Operator(":=")}    op = Tok(P_2)    Γ ⊢ ParseExpr(Advance(P_2)) ⇓ (P_3, init)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseBindingAfterLetVar(P) ⇓ (P_3, ⟨pat, ty_opt, op, init, SpanBetween(P, P_3)⟩)

**(Parse-ShadowBinding)**
Tok(P) = kw ∈ {Keyword(`let`), Keyword(`var`)}    Γ ⊢ ParseIdent(Advance(P)) ⇓ (P_1, name)    Γ ⊢ ParseTypeAnnotOpt(P_1) ⇓ (P_2, ty_opt)    IsOp(Tok(P_2), "=")    Γ ⊢ ParseExpr(Advance(P_2)) ⇓ (P_3, init)    s = (ShadowLetStmt(name, ty_opt, init) if kw = Keyword(`let`) else ShadowVarStmt(name, ty_opt, init))
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseShadowBinding(P) ⇓ (P_3, s)

**Record Bodies.**

**(Parse-RecordBody)**
IsPunc(Tok(P), "{")    Γ ⊢ ParseRecordMemberList(Advance(P)) ⇓ (P_1, members)    IsPunc(Tok(P_1), "}")
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseRecordBody(P) ⇓ (Advance(P_1), members)

**Record Member Lists.**

**(Parse-RecordMemberList-End)**
IsPunc(Tok(P), "}")
────────────────────────────────────────────
Γ ⊢ ParseRecordMemberList(P) ⇓ (P, [])

**(Parse-RecordMemberList-Cons)**
Γ ⊢ ParseRecordMember(P) ⇓ (P_1, m)    Γ ⊢ ParseRecordMemberTail(P_1, [m]) ⇓ (P_2, ms)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseRecordMemberList(P) ⇓ (P_2, ms)

**(Parse-RecordMemberTail-End)**
IsPunc(Tok(P), "}")
──────────────────────────────────────────────
Γ ⊢ ParseRecordMemberTail(P, xs) ⇓ (P, xs)

**(Parse-RecordMemberTail-TrailingComma)**
IsPunc(Tok(P), ",")    IsPunc(Tok(Advance(P)), "}")    Γ ⊢ Emit(Code(Unsupported-Construct))
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseRecordMemberTail(P, xs) ⇓ (Advance(P), xs)

**(Parse-RecordMemberTail-Comma)**
IsPunc(Tok(P), ",")    Γ ⊢ ParseRecordMember(Advance(P)) ⇓ (P_1, m)    Γ ⊢ ParseRecordMemberTail(P_1, xs ++ [m]) ⇓ (P_2, ys)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseRecordMemberTail(P, xs) ⇓ (P_2, ys)

**Record Members.**

**(Parse-RecordMember-Method)**
Γ ⊢ ParseVis(P) ⇓ (P_1, vis)    IsKw(Tok(P_1), `procedure`)    Γ ⊢ ParseMethodDefAfterVis(P_1, vis) ⇓ (P_2, m)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseRecordMember(P) ⇓ (P_2, m)

**(Parse-RecordMember-Field)**
Γ ⊢ ParseVis(P) ⇓ (P_1, vis)    ¬ IsKw(Tok(P_1), `procedure`)    Γ ⊢ ParseRecordFieldDeclAfterVis(P_1, vis) ⇓ (P_2, f)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseRecordMember(P) ⇓ (P_2, f)

**Record Method Definitions.**

**(Parse-MethodDefAfterVis)**
Γ ⊢ ParseOverrideOpt(P) ⇓ (P_0, ov)    IsKw(Tok(P_0), `procedure`)    Γ ⊢ ParseIdent(Advance(P_0)) ⇓ (P_1, name)    Γ ⊢ ParseMethodSignature(P_1) ⇓ (P_2, receiver, params, ret_opt)    Γ ⊢ ParseBlock(P_2) ⇓ (P_3, body)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseMethodDefAfterVis(P, vis) ⇓ (P_3, ⟨MethodDecl, vis, ov, name, receiver, params, ret_opt, body, SpanBetween(P, P_3), []⟩)

**(Parse-Override-Yes)**
IsKw(Tok(P), `override`)
────────────────────────────────────────────
Γ ⊢ ParseOverrideOpt(P) ⇓ (Advance(P), true)

**(Parse-Override-No)**
¬ IsKw(Tok(P), `override`)
──────────────────────────────────────────
Γ ⊢ ParseOverrideOpt(P) ⇓ (P, false)


**(Parse-MethodSignature)**
IsPunc(Tok(P), "(")    Γ ⊢ ParseReceiver(Advance(P)) ⇓ (P_1, r)    Γ ⊢ ParseMethodParams(P_1) ⇓ (P_2, params)    IsPunc(Tok(P_2), ")")    Γ ⊢ ParseReturnOpt(Advance(P_2)) ⇓ (P_3, ret_opt)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseMethodSignature(P) ⇓ (P_3, r, params, ret_opt)

**(Parse-MethodParams-None)**
IsPunc(Tok(P), ")")
────────────────────────────────────────────
Γ ⊢ ParseMethodParams(P) ⇓ (P, [])

**(Parse-MethodParams-Comma)**
IsPunc(Tok(P), ",")    Γ ⊢ ParseParamList(Advance(P)) ⇓ (P_1, params)
────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseMethodParams(P) ⇓ (P_1, params)

**Receiver Parsing.**

**(Parse-Receiver-Short-Const)**
IsOp(Tok(P), "~")
──────────────────────────────────────────────────────────────
Γ ⊢ ParseReceiver(P) ⇓ (Advance(P), ReceiverShorthand(`const`))

**(Parse-Receiver-Short-Unique)**
IsOp(Tok(P), "~!")
──────────────────────────────────────────────────────────────
Γ ⊢ ParseReceiver(P) ⇓ (Advance(P), ReceiverShorthand(`unique`))

**(Parse-Receiver-Explicit)**
Γ ⊢ ParseParamModeOpt(P) ⇓ (P_1, mode)    IsIdent(Tok(P_1))    Lexeme(Tok(P_1)) = `self`    IsPunc(Tok(Advance(P_1)), ":")    Γ ⊢ ParseType(Advance(Advance(P_1))) ⇓ (P_2, ty)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseReceiver(P) ⇓ (P_2, ReceiverExplicit(mode, ty))

**State Block Lists.**

**(Parse-StateBlockList-Empty)**
IsPunc(Tok(P), "}")
───────────────────────────────────────────────
Γ ⊢ ParseStateBlockList(P) ⇓ (P, [])

**(Parse-StateBlockList-Cons)**
Γ ⊢ ParseStateBlock(P) ⇓ (P_1, st)    Γ ⊢ ParseStateBlockList(P_1) ⇓ (P_2, sts)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseStateBlockList(P) ⇓ (P_2, [st] ++ sts)

**State Member Lists.**

**(Parse-StateMemberList-End)**
IsPunc(Tok(P), "}")
───────────────────────────────────────────────
Γ ⊢ ParseStateMemberList(P) ⇓ (P, [])

**(Parse-StateMemberList-Cons)**
Γ ⊢ ParseStateMember(P) ⇓ (P_1, m)    Γ ⊢ ParseStateMemberList(P_1) ⇓ (P_2, ms)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseStateMemberList(P) ⇓ (P_2, [m] ++ ms)


**Enum Bodies.**

**(Parse-EnumBody)**
IsPunc(Tok(P), "{")    Γ ⊢ ParseVariantList(Advance(P)) ⇓ (P_1, vars)    IsPunc(Tok(P_1), "}")
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseEnumBody(P) ⇓ (Advance(P_1), vars)

**Parameters and Returns.**

**(Parse-ParamList-Empty)**
IsPunc(Tok(P), ")")
────────────────────────────────────────────
Γ ⊢ ParseParamList(P) ⇓ (P, [])

**(Parse-ParamList-Cons)**
Γ ⊢ ParseParam(P) ⇓ (P_1, param)    Γ ⊢ ParseParamTail(P_1, [param]) ⇓ (P_2, params)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseParamList(P) ⇓ (P_2, params)

**(Parse-ReturnOpt-None)**
¬ IsOp(Tok(P), "->")
─────────────────────────────────────────────
Γ ⊢ ParseReturnOpt(P) ⇓ (P, ⊥)

**(Parse-ReturnOpt-Arrow)**
IsOp(Tok(P), "->")    Γ ⊢ ParseType(Advance(P)) ⇓ (P_1, ty)
────────────────────────────────────────────────────────────────
Γ ⊢ ParseReturnOpt(P) ⇓ (P_1, ty)

**(Parse-AliasOpt-None)**
¬ IsKw(Tok(P), `as`)
───────────────────────────────────────────
Γ ⊢ ParseAliasOpt(P) ⇓ (P, ⊥)

**(Parse-AliasOpt-Yes)**
IsKw(Tok(P), `as`)    Γ ⊢ ParseIdent(Advance(P)) ⇓ (P_1, id)
────────────────────────────────────────────────────────────────
Γ ⊢ ParseAliasOpt(P) ⇓ (P_1, id)

**(Parse-TypeAnnotOpt-None)**
¬ IsPunc(Tok(P), ":")
─────────────────────────────────────────────
Γ ⊢ ParseTypeAnnotOpt(P) ⇓ (P, ⊥)

**(Parse-TypeAnnotOpt-Yes)**
IsPunc(Tok(P), ":")    Γ ⊢ ParseType(Advance(P)) ⇓ (P_1, ty)
──────────────────────────────────────────────────────────────
Γ ⊢ ParseTypeAnnotOpt(P) ⇓ (P_1, ty)

**(Parse-UsingList-Empty)**
IsPunc(Tok(P), "}")    Γ ⊢ Emit(Code(Unsupported-Construct))
──────────────────────────────────────────────────────────────
Γ ⊢ ParseUsingList(P) ⇓ (Advance(P), [])

**(Parse-UsingList-Cons)**
Γ ⊢ ParseUsingSpec(P) ⇓ (P_1, s)    Γ ⊢ ParseUsingListTail(P_1, [s]) ⇓ (P_2, specs)    IsPunc(Tok(P_2), "}")
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseUsingList(P) ⇓ (Advance(P_2), specs)

**(Parse-UsingListTail-End)**
IsPunc(Tok(P), "}")
──────────────────────────────────────────────
Γ ⊢ ParseUsingListTail(P, xs) ⇓ (P, xs)

**(Parse-UsingListTail-TrailingComma)**
IsPunc(Tok(P), ",")    IsPunc(Tok(Advance(P)), "}")    Γ ⊢ Emit(Code(Unsupported-Construct))
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseUsingListTail(P, xs) ⇓ (Advance(P), xs)

**(Parse-UsingListTail-Comma)**
IsPunc(Tok(P), ",")    Γ ⊢ ParseUsingSpec(Advance(P)) ⇓ (P_1, s)    Γ ⊢ ParseUsingListTail(P_1, xs ++ [s]) ⇓ (P_2, ys)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseUsingListTail(P, xs) ⇓ (P_2, ys)

**(Parse-RecordFieldDeclList-Empty)**
IsPunc(Tok(P), "}")
──────────────────────────────────────────────────
Γ ⊢ ParseRecordFieldDeclList(P) ⇓ (P, [])

**(Parse-RecordFieldDeclList-Cons)**
Γ ⊢ ParseRecordFieldDecl(P) ⇓ (P_1, f)    Γ ⊢ ParseRecordFieldDeclTail(P_1, [f]) ⇓ (P_2, fields)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseRecordFieldDeclList(P) ⇓ (P_2, fields)

**(Parse-RecordFieldDecl)**
Γ ⊢ ParseVis(P) ⇓ (P_1, vis)    Γ ⊢ ParseRecordFieldDeclAfterVis(P_1, vis) ⇓ (P_2, f)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseRecordFieldDecl(P) ⇓ (P_2, f)

**(Parse-RecordFieldDeclAfterVis)**
Γ ⊢ ParseIdent(P) ⇓ (P_1, name)    IsPunc(Tok(P_1), ":")    Γ ⊢ ParseType(Advance(P_1)) ⇓ (P_2, ty)    Γ ⊢ ParseRecordFieldInitOpt(P_2) ⇓ (P_3, init_opt)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseRecordFieldDeclAfterVis(P, vis) ⇓ (P_3, ⟨vis, name, ty, init_opt⟩)

**(Parse-RecordFieldInitOpt-None)**
¬ IsOp(Tok(P), "=")
────────────────────────────────────────────────
Γ ⊢ ParseRecordFieldInitOpt(P) ⇓ (P, ⊥)

**(Parse-RecordFieldInitOpt-Yes)**
IsOp(Tok(P), "=")    Γ ⊢ ParseExpr(Advance(P)) ⇓ (P_1, e)
────────────────────────────────────────────────────────────
Γ ⊢ ParseRecordFieldInitOpt(P) ⇓ (P_1, e)

**(Parse-RecordFieldDeclTail-End)**
IsPunc(Tok(P), "}")
────────────────────────────────────────────────────
Γ ⊢ ParseRecordFieldDeclTail(P, xs) ⇓ (P, xs)

**(Parse-RecordFieldDeclTail-TrailingComma)**
IsPunc(Tok(P), ",")    IsPunc(Tok(Advance(P)), "}")    Γ ⊢ Emit(Code(Unsupported-Construct))
──────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseRecordFieldDeclTail(P, xs) ⇓ (Advance(P), xs)

**(Parse-RecordFieldDeclTail-Comma)**
IsPunc(Tok(P), ",")    Γ ⊢ ParseRecordFieldDecl(Advance(P)) ⇓ (P_1, f)    Γ ⊢ ParseRecordFieldDeclTail(P_1, xs ++ [f]) ⇓ (P_2, ys)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseRecordFieldDeclTail(P, xs) ⇓ (P_2, ys)

**(Parse-FieldDeclList-Empty)**
IsPunc(Tok(P), "}")
──────────────────────────────────────────────
Γ ⊢ ParseFieldDeclList(P) ⇓ (P, [])

**(Parse-FieldDeclList-Cons)**
Γ ⊢ ParseFieldDecl(P) ⇓ (P_1, f)    Γ ⊢ ParseFieldDeclTail(P_1, [f]) ⇓ (P_2, fields)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseFieldDeclList(P) ⇓ (P_2, fields)

**(Parse-FieldDecl)**
Γ ⊢ ParseVis(P) ⇓ (P_1, vis)    Γ ⊢ ParseIdent(P_1) ⇓ (P_2, name)    IsPunc(Tok(P_2), ":")    Γ ⊢ ParseType(Advance(P_2)) ⇓ (P_3, ty)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseFieldDecl(P) ⇓ (P_3, ⟨vis, name, ty, ⊥⟩)

**(Parse-FieldDeclTail-End)**
IsPunc(Tok(P), "}")
──────────────────────────────────────────────────
Γ ⊢ ParseFieldDeclTail(P, xs) ⇓ (P, xs)

**(Parse-FieldDeclTail-TrailingComma)**
IsPunc(Tok(P), ",")    IsPunc(Tok(Advance(P)), "}")    Γ ⊢ Emit(Code(Unsupported-Construct))
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseFieldDeclTail(P, xs) ⇓ (Advance(P), xs)

**(Parse-FieldDeclTail-Comma)**
IsPunc(Tok(P), ",")    Γ ⊢ ParseFieldDecl(Advance(P)) ⇓ (P_1, f)    Γ ⊢ ParseFieldDeclTail(P_1, xs ++ [f]) ⇓ (P_2, ys)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseFieldDeclTail(P, xs) ⇓ (P_2, ys)

**(Parse-VariantList-Empty)**
IsPunc(Tok(P), "}")
────────────────────────────────────────────
Γ ⊢ ParseVariantList(P) ⇓ (P, [])

**(Parse-VariantList-Cons)**
Γ ⊢ ParseVariant(P) ⇓ (P_1, v)    Γ ⊢ ParseVariantTail(P_1, [v]) ⇓ (P_2, vs)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseVariantList(P) ⇓ (P_2, vs)

**(Parse-Variant)**
Γ ⊢ ParseIdent(P) ⇓ (P_1, name)    Γ ⊢ ParseVariantPayloadOpt(P_1) ⇓ (P_2, payload_opt)    Γ ⊢ ParseVariantDiscriminantOpt(P_2) ⇓ (P_3, disc_opt)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseVariant(P) ⇓ (P_3, ⟨name, payload_opt, disc_opt⟩)

**(Parse-VariantPayloadOpt-None)**
Tok(P) ∉ {Punctuator("("), Punctuator("{")}
────────────────────────────────────────────────────────────
Γ ⊢ ParseVariantPayloadOpt(P) ⇓ (P, ⊥)

**(Parse-VariantPayloadOpt-Tuple)**
IsPunc(Tok(P), "(")    Γ ⊢ ParseTypeList(Advance(P)) ⇓ (P_1, ts)    IsPunc(Tok(P_1), ")")
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseVariantPayloadOpt(P) ⇓ (Advance(P_1), TuplePayload(ts))

**(Parse-VariantPayloadOpt-Record)**
IsPunc(Tok(P), "{")    Γ ⊢ ParseFieldDeclList(Advance(P)) ⇓ (P_1, fs)    IsPunc(Tok(P_1), "}")
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseVariantPayloadOpt(P) ⇓ (Advance(P_1), RecordPayload(fs))

**(Parse-VariantDiscriminantOpt-None)**
¬ IsOp(Tok(P), "=")
─────────────────────────────────────────────────────────────
Γ ⊢ ParseVariantDiscriminantOpt(P) ⇓ (P, ⊥)

**(Parse-VariantDiscriminantOpt-Yes)**
IsOp(Tok(P), "=")    t = Tok(Advance(P))    t.kind = IntLiteral
──────────────────────────────────────────────────────────────
Γ ⊢ ParseVariantDiscriminantOpt(P) ⇓ (Advance(Advance(P)), t)
**(Parse-VariantTail-End)**
IsPunc(Tok(P), "}")
────────────────────────────────────────────
Γ ⊢ ParseVariantTail(P, xs) ⇓ (P, xs)

**(Parse-VariantTail-TrailingComma)**
IsPunc(Tok(P), ",")    IsPunc(Tok(Advance(P)), "}")    Γ ⊢ Emit(Code(Unsupported-Construct))
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseVariantTail(P, xs) ⇓ (Advance(P), xs)

**(Parse-VariantTail-Comma)**
IsPunc(Tok(P), ",")    Γ ⊢ ParseVariant(Advance(P)) ⇓ (P_1, v)    Γ ⊢ ParseVariantTail(P_1, xs ++ [v]) ⇓ (P_2, ys)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseVariantTail(P, xs) ⇓ (P_2, ys)

**(Parse-Param)**
Γ ⊢ ParseParamModeOpt(P) ⇓ (P_1, mode)    Γ ⊢ ParseIdent(P_1) ⇓ (P_2, name)    IsPunc(Tok(P_2), ":")    Γ ⊢ ParseType(Advance(P_2)) ⇓ (P_3, ty)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseParam(P) ⇓ (P_3, ⟨mode, name, ty⟩)

**(Parse-ParamMode-None)**
¬ IsKw(Tok(P), `move`)
──────────────────────────────────────────────
Γ ⊢ ParseParamModeOpt(P) ⇓ (P, ⊥)

**(Parse-ParamMode-Move)**
IsKw(Tok(P), `move`)
─────────────────────────────────────────────────
Γ ⊢ ParseParamModeOpt(P) ⇓ (Advance(P), `move`)

**(Parse-ParamTail-End)**
IsPunc(Tok(P), ")")
────────────────────────────────────────────
Γ ⊢ ParseParamTail(P, xs) ⇓ (P, xs)

**(Parse-ParamTail-TrailingComma)**
IsPunc(Tok(P), ",")    IsPunc(Tok(Advance(P)), ")")    Γ ⊢ Emit(Code(Unsupported-Construct))
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseParamTail(P, xs) ⇓ (Advance(P), xs)

**(Parse-ParamTail-Comma)**
IsPunc(Tok(P), ",")    Γ ⊢ ParseParam(Advance(P)) ⇓ (P_1, p)    Γ ⊢ ParseParamTail(P_1, xs ++ [p]) ⇓ (P_2, ys)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseParamTail(P, xs) ⇓ (P_2, ys)
**(Parse-Signature)**
IsPunc(Tok(P), "(")    Γ ⊢ ParseParamList(Advance(P)) ⇓ (P_1, params)    IsPunc(Tok(P_1), ")")    Γ ⊢ ParseReturnOpt(Advance(P_1)) ⇓ (P_2, ret_opt)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseSignature(P) ⇓ (P_2, params, ret_opt)



##### 3.3.6.14. Unsupported Constructs (Parsing)

**(Parse-Import-Unsupported)**
IsKw(Tok(P), `import`)    Γ ⊢ Emit(Code(WF-Import-Unsupported))
────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseItem(P) ⇓ (SyncItem(Advance(P)), ErrorItem(SpanBetween(P, Advance(P))))

**(Parse-Attribute-Unsupported)**
IsPunc(Tok(P), "[")    IsPunc(Tok(Advance(P)), "[")    Γ ⊢ Emit(Code(WF-Attr-Unsupported))    Γ ⊢ SyncItem(P) ⇓ P_1
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseItem(P) ⇓ (P_1, ErrorItem(SpanBetween(P, P_1)))

**(Parse-Modal-Class-Unsupported)**
IsKw(Tok(P), `modal`)    IsKw(Tok(Advance(P)), `class`)    Γ ⊢ Emit(Code(Unsupported-Construct))
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseItem(P) ⇓ (SyncItem(Advance(Advance(P))), ErrorItem(SpanBetween(P, Advance(Advance(P)))))

**(Parse-Extern-Unsupported)**
IsIdent(Tok(P))    Lexeme(Tok(P)) = `extern`    Γ ⊢ Emit(Code(Parse-Extern-Unsupported))
────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseItem(P) ⇓ (SyncItem(Advance(P)), ErrorItem(SpanBetween(P, Advance(P))))

**(Parse-Use-Unsupported)**
IsIdent(Tok(P))    Lexeme(Tok(P)) = `use`    Γ ⊢ Emit(Code(Unsupported-Construct))
────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseItem(P) ⇓ (SyncItem(Advance(P)), ErrorItem(SpanBetween(P, Advance(P))))

**(Return-At-Module-Err)**
IsKw(Tok(P), `return`)    Γ ⊢ Emit(Code(Return-At-Module-Err))
────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseItem(P) ⇓ (SyncItem(Advance(P)), ErrorItem(SpanBetween(P, Advance(P))))

**Generic Syntax Rejection.**
AngleDelta(Operator("<")) = 1
AngleDelta(Operator(">")) = -1
AngleDelta(Operator(">>")) = -2
AngleDelta(t) = 0 if t.kind ∉ {Operator("<"), Operator(">"), Operator(">>")}

AngleStep(P, d) = ⟨Advance(P), d + AngleDelta(Tok(P))⟩
AngleScan(P_0, P, d) ⇓ P'
Tok(P) = EOF
────────────────────────────────────────
AngleScan(P_0, P, d) ⇓ P_0
Tok(P) ≠ EOF    AngleStep(P, d) = ⟨P_1, d_1⟩    d_1 ≠ 0    AngleScan(P_0, P_1, d_1) ⇓ P'
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────
AngleScan(P_0, P, d) ⇓ P'
Tok(P) ≠ EOF    AngleStep(P, d) = ⟨P_1, d_1⟩    d_1 = 0
────────────────────────────────────────────────────────────────────────
AngleScan(P_0, P, d) ⇓ P_1

SkipAngles(P_0) ⇓ P' ⇔ AngleScan(P_0, P_0, 0) ⇓ P'

**(Parse-Generic-Header-Unsupported)**
Γ ⊢ ParseVis(P) ⇓ (P_1, vis)    Tok(P_1) = Keyword(kw)    kw ∈ {`procedure`, `record`, `enum`, `class`, `modal`, `type`}    Γ ⊢ ParseIdent(Advance(P_1)) ⇓ (P_2, name)    IsOp(Tok(P_2), "<")    P_2' = SkipAngles(P_2)    Γ ⊢ Emit(Code(Unsupported-Construct))    Γ ⊢ SyncItem(P_2') ⇓ P_3
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseItem(P) ⇓ (P_3, ErrorItem(SpanBetween(P, P_3)))


**(Parse-Item-Err)**
c = Code(Parse-Syntax-Err)    Γ ⊢ Emit(c, Tok(P).span)    P_1 = AdvanceOrEOF(P)    Γ ⊢ SyncItem(P_1) ⇓ P_2
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseItem(P) ⇓ (P_2, ErrorItem(SpanBetween(P, P_2)))

#### 3.3.7. Type Parsing (C0 Subset)

**Type Start Predicate.**
TypeStart(t) ⇔ IsKw(t, `const`) ∨ IsKw(t, `unique`) ∨ IsKw(t, `shared`) ∨ Lexeme(t) ∈ PrimTypes_C0 ∨ IsPunc(t, "(") ∨ IsPunc(t, "[") ∨ IsOp(t, "*") ∨ IsOp(t, "$") ∨ Lexeme(t) ∈ {`string`, `Ptr`} ∨ IsIdent(t)

**(Parse-Type)**
Γ ⊢ ParsePermOpt(P) ⇓ (P_1, perm_opt)    Γ ⊢ ParseNonPermType(P_1) ⇓ (P_2, base)    Γ ⊢ ParseUnionTail(P_2) ⇓ (P_3, ts)    base' = (base if ts = [] else TypeUnion([base] ++ ts))    ty = (base' if perm_opt = ⊥ else TypePerm(perm_opt, base'))
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseType(P) ⇓ (P_3, ty)

**(Parse-Type-Err)**
Γ ⊢ ParsePermOpt(P) ⇓ (P_1, perm_opt)    c = Code(Parse-Syntax-Err)    Γ ⊢ Emit(c, Tok(P_1).span)    base = TypePrim("!")    ty = (base if perm_opt = ⊥ else TypePerm(perm_opt, base))
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseType(P) ⇓ (P_1, ty)


**(Parse-Perm-Const)**
IsKw(Tok(P), `const`)
──────────────────────────────────────────────
Γ ⊢ ParsePermOpt(P) ⇓ (Advance(P), `const`)

**(Parse-Perm-Unique)**
IsKw(Tok(P), `unique`)
──────────────────────────────────────────────
Γ ⊢ ParsePermOpt(P) ⇓ (Advance(P), `unique`)

**(Parse-Perm-Shared)**
IsKw(Tok(P), `shared`)
──────────────────────────────────────────────
Γ ⊢ ParsePermOpt(P) ⇓ (Advance(P), `shared`)

**(Parse-Perm-None)**
¬ (IsKw(Tok(P), `const`) ∨ IsKw(Tok(P), `unique`) ∨ IsKw(Tok(P), `shared`))
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePermOpt(P) ⇓ (P, ⊥)

**(Parse-UnionTail-None)**
¬ IsOp(Tok(P), "|")
──────────────────────────────────────────────
Γ ⊢ ParseUnionTail(P) ⇓ (P, [])

**(Parse-UnionTail-Cons)**
IsOp(Tok(P), "|")    Γ ⊢ ParseNonPermType(Advance(P)) ⇓ (P_1, t_1)    Γ ⊢ ParseUnionTail(P_1) ⇓ (P_2, ts)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseUnionTail(P) ⇓ (P_2, [t_1] ++ ts)


**Non-Permission Types.**

PrimLexemeSet = PrimTypes_C0 \ {"()", "!"}
BuiltinTypePath(path) ⇔ (|path| = 1 ∧ path[0] ∈ PrimLexemeSet) ∨ path = ["string"] ∨ path = ["bytes"]

**(Parse-Prim-Type)**
IsIdent(Tok(P))    Lexeme(Tok(P)) ∈ PrimLexemeSet
────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseNonPermType(P) ⇓ (Advance(P), TypePrim(Lexeme(Tok(P))))

**(Parse-Unit-Type)**
IsPunc(Tok(P), "(")    IsPunc(Tok(Advance(P)), ")")
──────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseNonPermType(P) ⇓ (Advance(Advance(P)), TypePrim("()"))

**(Parse-Never-Type)**
IsOp(Tok(P), "!")
────────────────────────────────────────────────────────────
Γ ⊢ ParseNonPermType(P) ⇓ (Advance(P), TypePrim("!"))

**(Parse-Func-Type)**
IsPunc(Tok(P), "(")    Γ ⊢ ParseParamTypeList(Advance(P)) ⇓ (P_1, params)    IsPunc(Tok(P_1), ")")    IsOp(Tok(Advance(P_1)), "->")    Γ ⊢ ParseType(Advance(Advance(P_1))) ⇓ (P_2, ret)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseNonPermType(P) ⇓ (P_2, TypeFunc(params, ret))

**(Parse-Tuple-Type)**
IsPunc(Tok(P), "(")    Γ ⊢ ParseTupleTypeElems(Advance(P)) ⇓ (P_1, elems)    elems ≠ []    IsPunc(Tok(P_1), ")")
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseNonPermType(P) ⇓ (Advance(P_1), TypeTuple(elems))

**(Parse-Array-Type)**
IsPunc(Tok(P), "[")    Γ ⊢ ParseType(Advance(P)) ⇓ (P_1, t)    IsPunc(Tok(P_1), ";")    Γ ⊢ ParseExpr(Advance(P_1)) ⇓ (P_2, e)    IsPunc(Tok(P_2), "]")
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseNonPermType(P) ⇓ (Advance(P_2), TypeArray(t, e))

**(Parse-Slice-Type)**
IsPunc(Tok(P), "[")    Γ ⊢ ParseType(Advance(P)) ⇓ (P_1, t)    IsPunc(Tok(P_1), "]")
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseNonPermType(P) ⇓ (Advance(P_1), TypeSlice(t))

**(Parse-Safe-Pointer-Type-ShiftSplit)**
IsIdent(Tok(P))    Lexeme(Tok(P)) = `Ptr`    IsOp(Tok(Advance(P)), "<")    Γ ⊢ ParseType(Advance(Advance(P))) ⇓ (P_1, t)    IsOp(Tok(P_1), ">>")    P_1' = SplitShiftR(P_1)    Γ ⊢ ParsePtrState(Advance(P_1')) ⇓ (P_2, st_opt)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseNonPermType(P) ⇓ (P_2, TypePtr(t, st_opt))

**(Parse-Safe-Pointer-Type)**
IsIdent(Tok(P))    Lexeme(Tok(P)) = `Ptr`    IsOp(Tok(Advance(P)), "<")    Γ ⊢ ParseType(Advance(Advance(P))) ⇓ (P_1, t)    IsOp(Tok(P_1), ">")    Γ ⊢ ParsePtrState(Advance(P_1)) ⇓ (P_2, st_opt)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseNonPermType(P) ⇓ (P_2, TypePtr(t, st_opt))

**(Parse-Raw-Pointer-Type)**
IsOp(Tok(P), "*")    IsKw(Tok(Advance(P)), q)    q ∈ {`imm`, `mut`}    Γ ⊢ ParseType(Advance(Advance(P))) ⇓ (P_1, t)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseNonPermType(P) ⇓ (P_1, TypeRawPtr(q, t))

**(Parse-String-Type)**
IsIdent(Tok(P))    Lexeme(Tok(P)) = `string`    Γ ⊢ ParseStringState(Advance(P)) ⇓ (P_1, st_opt)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseNonPermType(P) ⇓ (P_1, TypeString(st_opt))

**(Parse-Bytes-Type)**
IsIdent(Tok(P))    Lexeme(Tok(P)) = `bytes`    Γ ⊢ ParseBytesState(Advance(P)) ⇓ (P_1, st_opt)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseNonPermType(P) ⇓ (P_1, TypeBytes(st_opt))

**(Parse-Dynamic-Type)**
IsOp(Tok(P), "$")    Γ ⊢ ParseTypePath(Advance(P)) ⇓ (P_1, path)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseNonPermType(P) ⇓ (P_1, TypeDynamic(path))

**(Parse-Modal-State-Type)**
Γ ⊢ ParseTypePath(P) ⇓ (P_1, path)    IsOp(Tok(P_1), "@")    Γ ⊢ ParseIdent(Advance(P_1)) ⇓ (P_2, state)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseNonPermType(P) ⇓ (P_2, TypeModalState(path, state))

**(Parse-Type-Generic-Unsupported)**
Γ ⊢ ParseTypePath(P) ⇓ (P_1, path)    IsOp(Tok(P_1), "<")    path ≠ [`Ptr`]    P_1' = SkipAngles(P_1)    Γ ⊢ Emit(Code(Unsupported-Construct))    Γ ⊢ SyncType(P_1') ⇓ P_2
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseNonPermType(P) ⇓ (P_2, TypePath(path))

**(Parse-Type-Path)**
Γ ⊢ ParseTypePath(P) ⇓ (P_1, path)    ¬ IsOp(Tok(P_1), "@")    ¬ IsOp(Tok(P_1), "<")    ¬ BuiltinTypePath(path)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseNonPermType(P) ⇓ (P_1, TypePath(path))

##### 3.3.7.1. Type Helper Parsing Rules

**Tuple Type Elements.**

**(Parse-TupleTypeElems-Empty)**
IsPunc(Tok(P), ")")
──────────────────────────────────────────────
Γ ⊢ ParseTupleTypeElems(P) ⇓ (P, [])

**(Parse-TupleTypeElems-One)**
Γ ⊢ ParseType(P) ⇓ (P_1, t)    IsPunc(Tok(P_1), ";")
────────────────────────────────────────────────────────────────
Γ ⊢ ParseTupleTypeElems(P) ⇓ (Advance(P_1), [t])

**(Parse-TupleTypeElems-TrailingComma)**
Γ ⊢ ParseType(P) ⇓ (P_1, t)    IsPunc(Tok(P_1), ",")    IsPunc(Tok(Advance(P_1)), ")")    Γ ⊢ Emit(Code(Unsupported-Construct))
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseTupleTypeElems(P) ⇓ (Advance(P_1), [t])

**(Parse-TupleTypeElems-Many)**
Γ ⊢ ParseType(P) ⇓ (P_1, t_1)    IsPunc(Tok(P_1), ",")    Γ ⊢ ParseType(Advance(P_1)) ⇓ (P_2, t_2)    Γ ⊢ ParseTypeListTail(P_2, [t_2]) ⇓ (P_3, ts)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseTupleTypeElems(P) ⇓ (P_3, [t_1] ++ ts)

**Param Type Lists.**

**(Parse-ParamTypeList-Empty)**
IsPunc(Tok(P), ")")
──────────────────────────────────────────────
Γ ⊢ ParseParamTypeList(P) ⇓ (P, [])

**(Parse-ParamTypeList-Cons)**
Γ ⊢ ParseParamType(P) ⇓ (P_1, pt)    Γ ⊢ ParseParamTypeListTail(P_1, [pt]) ⇓ (P_2, pts)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseParamTypeList(P) ⇓ (P_2, pts)

**(Parse-ParamTypeListTail-End)**
¬ IsPunc(Tok(P), ",")
────────────────────────────────────────────────────
Γ ⊢ ParseParamTypeListTail(P, ps) ⇓ (P, ps)

**(Parse-ParamTypeListTail-Comma)**
IsPunc(Tok(P), ",")    Γ ⊢ ParseParamType(Advance(P)) ⇓ (P_1, pt)    Γ ⊢ ParseParamTypeListTail(P_1, ps ++ [pt]) ⇓ (P_2, ps')
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseParamTypeListTail(P, ps) ⇓ (P_2, ps')

**Param Types.**

**(Parse-ParamType-Move)**
IsKw(Tok(P), `move`)    Γ ⊢ ParseType(Advance(P)) ⇓ (P_1, ty)
────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseParamType(P) ⇓ (P_1, ⟨`move`, ty⟩)

**(Parse-ParamType-Plain)**
¬ IsKw(Tok(P), `move`)    Γ ⊢ ParseType(P) ⇓ (P_1, ty)
────────────────────────────────────────────────────────────────
Γ ⊢ ParseParamType(P) ⇓ (P_1, ⟨⊥, ty⟩)

**String State.**

**(Parse-StringState-None)**
¬ IsOp(Tok(P), "@")
───────────────────────────────────────────────
Γ ⊢ ParseStringState(P) ⇓ (P, ⊥)

**(Parse-StringState-Managed)**
IsOp(Tok(P), "@")    IsIdent(Tok(Advance(P)))    Lexeme(Tok(Advance(P))) = `Managed`
────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseStringState(P) ⇓ (Advance(Advance(P)), `Managed`)

**(Parse-StringState-View)**
IsOp(Tok(P), "@")    IsIdent(Tok(Advance(P)))    Lexeme(Tok(Advance(P))) = `View`
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseStringState(P) ⇓ (Advance(Advance(P)), `View`)

**Bytes State.**

**(Parse-BytesState-None)**
¬ IsOp(Tok(P), "@")
───────────────────────────────────────────────
Γ ⊢ ParseBytesState(P) ⇓ (P, ⊥)

**(Parse-BytesState-Managed)**
IsOp(Tok(P), "@")    IsIdent(Tok(Advance(P)))    Lexeme(Tok(Advance(P))) = `Managed`
────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseBytesState(P) ⇓ (Advance(Advance(P)), `Managed`)

**(Parse-BytesState-View)**
IsOp(Tok(P), "@")    IsIdent(Tok(Advance(P)))    Lexeme(Tok(Advance(P))) = `View`
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseBytesState(P) ⇓ (Advance(Advance(P)), `View`)

**Pointer State.**

**(Parse-PtrState-None)**
¬ IsOp(Tok(P), "@")
──────────────────────────────────────────────
Γ ⊢ ParsePtrState(P) ⇓ (P, ⊥)

**(Parse-PtrState-Valid)**
IsOp(Tok(P), "@")    IsIdent(Tok(Advance(P)))    Lexeme(Tok(Advance(P))) = `Valid`
────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePtrState(P) ⇓ (Advance(Advance(P)), `Valid`)

**(Parse-PtrState-Null)**
IsOp(Tok(P), "@")    IsIdent(Tok(Advance(P)))    Lexeme(Tok(Advance(P))) = `Null`
──────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePtrState(P) ⇓ (Advance(Advance(P)), `Null`)

**(Parse-PtrState-Expired)**
IsOp(Tok(P), "@")    IsIdent(Tok(Advance(P)))    Lexeme(Tok(Advance(P))) = `Expired`
──────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePtrState(P) ⇓ (Advance(Advance(P)), `Expired`)

**Type Lists.**

**(Parse-TypeList-Empty)**
IsPunc(Tok(P), ")")
──────────────────────────────────────────────
Γ ⊢ ParseTypeList(P) ⇓ (P, [])

**(Parse-TypeList-Cons)**
Γ ⊢ ParseType(P) ⇓ (P_1, t)    Γ ⊢ ParseTypeListTail(P_1, [t]) ⇓ (P_2, ts)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseTypeList(P) ⇓ (P_2, ts)

**(Parse-TypeListTail-End)**
Tok(P) ∈ {Punctuator(")"), Punctuator("}")}
──────────────────────────────────────────────────────────────
Γ ⊢ ParseTypeListTail(P, xs) ⇓ (P, xs)

**(Parse-TypeListTail-TrailingComma)**
IsPunc(Tok(P), ",")    Tok(Advance(P)) ∈ {Punctuator(")"), Punctuator("}")}    Γ ⊢ Emit(Code(Unsupported-Construct))
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseTypeListTail(P, xs) ⇓ (Advance(P), xs)

**(Parse-TypeListTail-Comma)**
IsPunc(Tok(P), ",")    Γ ⊢ ParseType(Advance(P)) ⇓ (P_1, t)    Γ ⊢ ParseTypeListTail(P_1, xs ++ [t]) ⇓ (P_2, ys)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseTypeListTail(P, xs) ⇓ (P_2, ys)

#### 3.3.8. Expression Parsing and Precedence

**Operator Sets.**
LogicalOrOps = {"||"}
LogicalAndOps = {"&&"}
ComparisonOps = {"==", "!=", "<", "<=", ">", ">="}
BitOrOps = {"|"}
BitXorOps = {"^"}
BitAndOps = {"&"}
AddOps = {"+", "-"}
MulOps = {"*", "/", "%"}

**(Parse-Expr)**
Γ ⊢ ParseRange(P) ⇓ (P_1, e)
──────────────────────────────────────────────
Γ ⊢ ParseExpr(P) ⇓ (P_1, e)

##### 3.3.8.1. Range Expressions

ExprStart(t) ⇔ IsIdent(t) ∨ (t ∈ LiteralToken) ∨ IsPunc(t, "(") ∨ IsPunc(t, "[") ∨ IsPunc(t, "{")
              ∨ IsOp(t, "!") ∨ IsOp(t, "-") ∨ IsOp(t, "&") ∨ IsOp(t, "*") ∨ IsOp(t, "^")
              ∨ IsKw(t, `if`) ∨ IsKw(t, `match`) ∨ IsKw(t, `loop`) ∨ IsKw(t, `unsafe`) ∨ IsKw(t, `move`) ∨ IsKw(t, `transmute`) ∨ IsKw(t, `widen`)

**(Parse-Range-To)**
IsOp(Tok(P), "..")    Γ ⊢ ParseLogicalOr(Advance(P)) ⇓ (P_1, e)
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseRange(P) ⇓ (P_1, Range(To, ⊥, e))

**(Parse-Range-ToInc)**
IsOp(Tok(P), "..=")    Γ ⊢ ParseLogicalOr(Advance(P)) ⇓ (P_1, e)
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseRange(P) ⇓ (P_1, Range(ToInclusive, ⊥, e))

**(Parse-Range-Full)**
IsOp(Tok(P), "..")    Tok(Advance(P)) ∉ ExprStart
────────────────────────────────────────────────────────────────
Γ ⊢ ParseRange(P) ⇓ (Advance(P), Range(Full, ⊥, ⊥))

**(Parse-Range-Lhs)**
Γ ⊢ ParseLogicalOr(P) ⇓ (P_1, e_0)    Γ ⊢ ParseRangeTail(P_1, e_0) ⇓ (P_2, e)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseRange(P) ⇓ (P_2, e)

**(Parse-RangeTail-None)**
¬ (IsOp(Tok(P), "..") ∨ IsOp(Tok(P), "..="))
───────────────────────────────────────────────────────────────
Γ ⊢ ParseRangeTail(P, e) ⇓ (P, e)

**(Parse-RangeTail-From)**
IsOp(Tok(P), "..")    Tok(Advance(P)) ∉ ExprStart
────────────────────────────────────────────────────────────────
Γ ⊢ ParseRangeTail(P, e_0) ⇓ (Advance(P), Range(From, e_0, ⊥))

**(Parse-RangeTail-Excl)**
IsOp(Tok(P), "..")    Tok(Advance(P)) ∈ ExprStart    Γ ⊢ ParseLogicalOr(Advance(P)) ⇓ (P_1, e_1)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseRangeTail(P, e_0) ⇓ (P_1, Range(Exclusive, e_0, e_1))

**(Parse-RangeTail-Incl)**
IsOp(Tok(P), "..=")    Γ ⊢ ParseLogicalOr(Advance(P)) ⇓ (P_1, e_1)
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseRangeTail(P, e_0) ⇓ (P_1, Range(Inclusive, e_0, e_1))

##### 3.3.8.2. Binary Operator Chains

Γ ⊢ ParseLogicalOr(P) ⇓ (P', e) ⇔ Γ ⊢ ParseLeftChain(P, LogicalOrOps, ParseLogicalAnd) ⇓ (P', e)
Γ ⊢ ParseLogicalAnd(P) ⇓ (P', e) ⇔ Γ ⊢ ParseLeftChain(P, LogicalAndOps, ParseComparison) ⇓ (P', e)
Γ ⊢ ParseComparison(P) ⇓ (P', e) ⇔ Γ ⊢ ParseLeftChain(P, ComparisonOps, ParseBitOr) ⇓ (P', e)
Γ ⊢ ParseBitOr(P) ⇓ (P', e) ⇔ Γ ⊢ ParseLeftChain(P, BitOrOps, ParseBitXor) ⇓ (P', e)
Γ ⊢ ParseBitXor(P) ⇓ (P', e) ⇔ Γ ⊢ ParseLeftChain(P, BitXorOps, ParseBitAnd) ⇓ (P', e)
Γ ⊢ ParseBitAnd(P) ⇓ (P', e) ⇔ Γ ⊢ ParseLeftChain(P, BitAndOps, ParseShift) ⇓ (P', e)
Γ ⊢ ParseShift(P) ⇓ (P', e) ⇔ Γ ⊢ ParseLeftChain(P, ShiftOps, ParseAdd) ⇓ (P', e)
Γ ⊢ ParseAdd(P) ⇓ (P', e) ⇔ Γ ⊢ ParseLeftChain(P, AddOps, ParseMul) ⇓ (P', e)
Γ ⊢ ParseMul(P) ⇓ (P', e) ⇔ Γ ⊢ ParseLeftChain(P, MulOps, ParsePower) ⇓ (P', e)

**(Parse-LeftChain)**
Γ ⊢ ParseHigher(P) ⇓ (P_1, e_0)    Γ ⊢ ParseLeftChainTail(P_1, e_0, OpSet, ParseHigher) ⇓ (P_2, e)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseLeftChain(P, OpSet, ParseHigher) ⇓ (P_2, e)

**(Parse-LeftChain-Stop)**
Tok(P) ∉ OpSet
──────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseLeftChainTail(P, e, OpSet, ParseHigher) ⇓ (P, e)

**(Parse-LeftChain-Cons)**
Tok(P) = op ∈ OpSet    Γ ⊢ ParseHigher(Advance(P)) ⇓ (P_1, e_1)    e' = Binary(op, e, e_1)    Γ ⊢ ParseLeftChainTail(P_1, e', OpSet, ParseHigher) ⇓ (P_2, e'')
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseLeftChainTail(P, e, OpSet, ParseHigher) ⇓ (P_2, e'')


##### 3.3.8.3. Power (Right-Associative)

**(Parse-Power)**
Γ ⊢ ParseCast(P) ⇓ (P_1, e_0)    Γ ⊢ ParsePowerTail(P_1, e_0) ⇓ (P_2, e)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePower(P) ⇓ (P_2, e)

**(Parse-PowerTail-None)**
¬ IsOp(Tok(P), "**")
──────────────────────────────────────────────
Γ ⊢ ParsePowerTail(P, e) ⇓ (P, e)

**(Parse-PowerTail-Cons)**
IsOp(Tok(P), "**")    Γ ⊢ ParsePower(Advance(P)) ⇓ (P_1, e_1)
────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePowerTail(P, e_0) ⇓ (P_1, Binary("**", e_0, e_1))

##### 3.3.8.4. Cast Expressions

**(Parse-Cast)**
Γ ⊢ ParseUnary(P) ⇓ (P_1, e)    Γ ⊢ ParseCastTail(P_1, e) ⇓ (P_2, e')
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseCast(P) ⇓ (P_2, e')

**(Parse-CastTail-None)**
¬ IsKw(Tok(P), `as`)
──────────────────────────────────────────────
Γ ⊢ ParseCastTail(P, e) ⇓ (P, e)

**(Parse-CastTail-As)**
IsKw(Tok(P), `as`)    Γ ⊢ ParseType(Advance(P)) ⇓ (P_1, t)
────────────────────────────────────────────────────────────────
Γ ⊢ ParseCastTail(P, e) ⇓ (P_1, Cast(e, t))

##### 3.3.8.5. Unary and Postfix

**(Parse-Unary-Prefix)**
Tok(P) = op ∈ {"!", "-"}    Γ ⊢ ParseUnary(Advance(P)) ⇓ (P_1, e)
────────────────────────────────────────────────────────────────
Γ ⊢ ParseUnary(P) ⇓ (P_1, Unary(op, e))

**(Parse-Unary-Deref)**
IsOp(Tok(P), "*")    Γ ⊢ ParseUnary(Advance(P)) ⇓ (P_1, e)
────────────────────────────────────────────────────────────────
Γ ⊢ ParseUnary(P) ⇓ (P_1, Deref(e))

**(Parse-Unary-AddressOf)**
IsOp(Tok(P), "&")    Γ ⊢ ParsePlace(Advance(P)) ⇓ (P_1, p)
────────────────────────────────────────────────────────────────
Γ ⊢ ParseUnary(P) ⇓ (P_1, AddressOf(p))

**(Parse-Unary-Move)**
IsKw(Tok(P), `move`)    Γ ⊢ ParsePlace(Advance(P)) ⇓ (P_1, p)
────────────────────────────────────────────────────────────────
Γ ⊢ ParseUnary(P) ⇓ (P_1, MoveExpr(p))

**(Parse-Unary-Widen)**
IsKw(Tok(P), `widen`)    Γ ⊢ ParseUnary(Advance(P)) ⇓ (P_1, e)
────────────────────────────────────────────────────────────────
Γ ⊢ ParseUnary(P) ⇓ (P_1, Unary(`widen`, e))

**(Parse-Unary-Postfix)**
Γ ⊢ ParsePostfix(P) ⇓ (P_1, e)
──────────────────────────────────────────────
Γ ⊢ ParseUnary(P) ⇓ (P_1, e)

**(Parse-Postfix)**
Γ ⊢ ParsePrimary(P) ⇓ (P_1, e_0)    Γ ⊢ ParsePostfixTail(P_1, e_0) ⇓ (P_2, e)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePostfix(P) ⇓ (P_2, e)

##### 3.3.8.6. Primary Expressions

**(Parse-Comptime-Unsupported)**
IsIdent(Tok(P))    Lexeme(Tok(P)) = `comptime`    IsPunc(Tok(Advance(P)), "{")    Γ ⊢ Emit(Code(Unsupported-Construct))    Γ ⊢ SyncStmt(P) ⇓ P_1
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePrimary(P) ⇓ (P_1, ErrorExpr(SpanBetween(P, P_1)))

**(Parse-Literal-Expr)**
Tok(P).kind ∈ {IntLiteral, FloatLiteral, StringLiteral, CharLiteral, BoolLiteral, NullLiteral}
───────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePrimary(P) ⇓ (Advance(P), Literal(Tok(P)))

**(Parse-Null-Ptr)**
IsIdent(Tok(P))    Lexeme(Tok(P)) = `Ptr`    IsOp(Tok(Advance(P)), "::")    Tok(Advance(Advance(P))).kind = NullLiteral    IsPunc(Tok(Advance(Advance(Advance(P)))), "(")    IsPunc(Tok(Advance(Advance(Advance(Advance(P))))), ")")
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePrimary(P) ⇓ (Advance(Advance(Advance(Advance(Advance(P))))), PtrNullExpr)

**(Parse-Alloc-Implicit)**
IsOp(Tok(P), "^")    Γ ⊢ ParseExpr(Advance(P)) ⇓ (P_1, e)
────────────────────────────────────────────────────────────────
Γ ⊢ ParsePrimary(P) ⇓ (P_1, AllocExpr(⊥, e))


**(Parse-Identifier-Expr)**
IsIdent(Tok(P))    ¬ IsOp(Tok(Advance(P)), "::")    ¬ IsOp(Tok(Advance(P)), "@")    ¬ IsPunc(Tok(Advance(P)), "{")
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePrimary(P) ⇓ (Advance(P), Identifier(Lexeme(Tok(P))))

**(Parse-Qualified-Generic-Unsupported)**
Γ ⊢ ParseQualifiedHead(P) ⇓ (P_1, path, name)    IsOp(Tok(P_1), "<")    P_1' = SkipAngles(P_1)    Γ ⊢ Emit(Code(Unsupported-Construct))    Γ ⊢ SyncStmt(P_1') ⇓ P_2
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePrimary(P) ⇓ (P_2, ErrorExpr(SpanBetween(P, P_2)))

**(Parse-Qualified-Name)**
Γ ⊢ ParseQualifiedHead(P) ⇓ (P_1, path, name)    Tok(P_1) ∉ {Punctuator("("), Punctuator("{")}    ¬ IsOp(Tok(P_1), "@")
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePrimary(P) ⇓ (P_1, QualifiedName(path, name))

**(Parse-Qualified-Apply-Paren)**
Γ ⊢ ParseQualifiedHead(P) ⇓ (P_1, path, name)    IsPunc(Tok(P_1), "(")    Γ ⊢ ParseArgList(Advance(P_1)) ⇓ (P_2, args)    IsPunc(Tok(P_2), ")")
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePrimary(P) ⇓ (Advance(P_2), QualifiedApply(path, name, Paren(args)))

**(Parse-Qualified-Apply-Brace)**
Γ ⊢ ParseQualifiedHead(P) ⇓ (P_1, path, name)    IsPunc(Tok(P_1), "{")    Γ ⊢ ParseFieldInitList(Advance(P_1)) ⇓ (P_2, fields)    IsPunc(Tok(P_2), "}")
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePrimary(P) ⇓ (Advance(P_2), QualifiedApply(path, name, Brace(fields)))

**(Parse-Parenthesized-Expr)**
IsPunc(Tok(P), "(")    ¬ TupleParen(P)    Γ ⊢ ParseExpr(Advance(P)) ⇓ (P_1, e)    IsPunc(Tok(P_1), ")")
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePrimary(P) ⇓ (Advance(P_1), e)

**(Parse-Tuple-Literal)**
IsPunc(Tok(P), "(")    TupleParen(P)    Γ ⊢ ParseTupleExprElems(Advance(P)) ⇓ (P_1, elems)    IsPunc(Tok(P_1), ")")
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePrimary(P) ⇓ (Advance(P_1), TupleExpr(elems))

**(Parse-Array-Literal)**
IsPunc(Tok(P), "[")    Γ ⊢ ParseExprList(Advance(P)) ⇓ (P_1, elems)    IsPunc(Tok(P_1), "]")
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePrimary(P) ⇓ (Advance(P_1), ArrayExpr(elems))

**(Parse-Record-Literal-ModalState)**
Γ ⊢ ParseTypePath(P) ⇓ (P_1, path)    IsOp(Tok(P_1), "@")    Γ ⊢ ParseIdent(Advance(P_1)) ⇓ (P_2, state)    IsPunc(Tok(P_2), "{")    Γ ⊢ ParseFieldInitList(Advance(P_2)) ⇓ (P_3, fields)    IsPunc(Tok(P_3), "}")
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePrimary(P) ⇓ (Advance(P_3), RecordExpr(ModalStateRef(path, state), fields))

**(Parse-Record-Literal)**
Γ ⊢ ParseTypePath(P) ⇓ (P_1, path)    |path| = 1    IsPunc(Tok(P_1), "{")    Γ ⊢ ParseFieldInitList(Advance(P_1)) ⇓ (P_2, fields)    IsPunc(Tok(P_2), "}")
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePrimary(P) ⇓ (Advance(P_2), RecordExpr(TypePath(path), fields))


RuleSet(ParsePrimary_NoBrace) = RuleSet(ParsePrimary) \ {Parse-Record-Literal, Parse-Record-Literal-ModalState, Parse-Qualified-Apply-Brace}
RuleSet(ParsePostfix_NoBrace) = RuleSet(ParsePostfix) with ParsePrimary replaced by ParsePrimary_NoBrace
RuleSet(ParseUnary_NoBrace) = RuleSet(ParseUnary) with ParsePostfix replaced by ParsePostfix_NoBrace
RuleSet(ParseLogicalOr_NoBrace) = RuleSet(ParseLogicalOr) with ParseLogicalAnd replaced by ParseLogicalAnd_NoBrace
RuleSet(ParseLogicalAnd_NoBrace) = RuleSet(ParseLogicalAnd) with ParseComparison replaced by ParseComparison_NoBrace
RuleSet(ParseComparison_NoBrace) = RuleSet(ParseComparison) with ParseBitOr replaced by ParseBitOr_NoBrace
RuleSet(ParseBitOr_NoBrace) = RuleSet(ParseBitOr) with ParseBitXor replaced by ParseBitXor_NoBrace
RuleSet(ParseBitXor_NoBrace) = RuleSet(ParseBitXor) with ParseBitAnd replaced by ParseBitAnd_NoBrace
RuleSet(ParseBitAnd_NoBrace) = RuleSet(ParseBitAnd) with ParseShift replaced by ParseShift_NoBrace
RuleSet(ParseShift_NoBrace) = RuleSet(ParseShift) with ParseAdd replaced by ParseAdd_NoBrace
RuleSet(ParseAdd_NoBrace) = RuleSet(ParseAdd) with ParseMul replaced by ParseMul_NoBrace
RuleSet(ParseMul_NoBrace) = RuleSet(ParseMul) with ParsePower replaced by ParsePower_NoBrace
RuleSet(ParseRange_NoBrace) = RuleSet(ParseRange) with ParseLogicalOr replaced by ParseLogicalOr_NoBrace
RuleSet(ParseExpr_NoBrace) = RuleSet(ParseExpr) with ParseRange replaced by ParseRange_NoBrace

Γ ⊢ ParseRange_NoBrace(P) ⇓ (P_1, e)
───────────────────────────────────────────────
Γ ⊢ ParseExpr_NoBrace(P) ⇓ (P_1, e)

**(Parse-If-Expr)**
IsKw(Tok(P), `if`)    Γ ⊢ ParseExpr_NoBrace(Advance(P)) ⇓ (P_1, c)    Γ ⊢ ParseBlock(P_1) ⇓ (P_2, b1)    Γ ⊢ ParseElseOpt(P_2) ⇓ (P_3, b2)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePrimary(P) ⇓ (P_3, IfExpr(c, b1, b2))

**(Parse-Match-Expr)**
IsKw(Tok(P), `match`)    Γ ⊢ ParseExpr_NoBrace(Advance(P)) ⇓ (P_1, e)    IsPunc(Tok(P_1), "{")    Γ ⊢ ParseMatchArms(Advance(P_1)) ⇓ (P_2, arms)    IsPunc(Tok(P_2), "}")
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePrimary(P) ⇓ (Advance(P_2), MatchExpr(e, arms))

**(Parse-Loop-Expr)**
IsKw(Tok(P), `loop`)    Γ ⊢ ParseLoopTail(Advance(P)) ⇓ (P_1, loop)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePrimary(P) ⇓ (P_1, loop)

**(Parse-Block-Expr)**
IsPunc(Tok(P), "{")    Γ ⊢ ParseBlock(P) ⇓ (P_1, b)
──────────────────────────────────────────────────────────────
Γ ⊢ ParsePrimary(P) ⇓ (P_1, b)

**(Parse-Unsafe-Expr)**
IsKw(Tok(P), `unsafe`)    Γ ⊢ ParseBlock(Advance(P)) ⇓ (P_1, b)
──────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePrimary(P) ⇓ (P_1, UnsafeBlockExpr(b))

**(Parse-Transmute-Expr-ShiftSplit)**
IsKw(Tok(P), `transmute`)    IsOp(Tok(Advance(P)), "::")    IsOp(Tok(Advance(Advance(P))), "<")    Γ ⊢ ParseType(Advance(Advance(Advance(P)))) ⇓ (P_1, t1)    IsPunc(Tok(P_1), ",")    Γ ⊢ ParseType(Advance(P_1)) ⇓ (P_2, t2)    IsOp(Tok(P_2), ">>")    P_2' = SplitShiftR(P_2)    IsOp(Tok(P_2'), ">")    IsPunc(Tok(Advance(P_2')), "(")    Γ ⊢ ParseExpr(Advance(Advance(P_2'))) ⇓ (P_3, e)    IsPunc(Tok(P_3), ")")
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePrimary(P) ⇓ (Advance(P_3), TransmuteExpr(t1, t2, e))

**(Parse-Transmute-Expr)**
IsKw(Tok(P), `transmute`)    IsOp(Tok(Advance(P)), "::")    IsOp(Tok(Advance(Advance(P))), "<")    Γ ⊢ ParseType(Advance(Advance(Advance(P)))) ⇓ (P_1, t1)    IsPunc(Tok(P_1), ",")    Γ ⊢ ParseType(Advance(P_1)) ⇓ (P_2, t2)    IsOp(Tok(P_2), ">")    IsPunc(Tok(Advance(P_2)), "(")    Γ ⊢ ParseExpr(Advance(Advance(P_2))) ⇓ (P_3, e)    IsPunc(Tok(P_3), ")")
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePrimary(P) ⇓ (Advance(P_3), TransmuteExpr(t1, t2, e))


**(Parse-Primary-Err)**
c = Code(Parse-Syntax-Err)    Γ ⊢ Emit(c, Tok(P).span)    P_1 = AdvanceOrEOF(P)    Γ ⊢ SyncStmt(P_1) ⇓ P_2
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePrimary(P) ⇓ (P_2, ErrorExpr(SpanBetween(P, P_2)))

##### 3.3.8.7. Expression Helper Parsing Rules

**Postfix Tail.**

**(Parse-PostfixTail-Stop)**
Tok(P) ∉ PostfixStart
──────────────────────────────────────────────
Γ ⊢ ParsePostfixTail(P, e) ⇓ (P, e)

**(Parse-PostfixTail-Cons)**
Tok(P) ∈ PostfixStart    Γ ⊢ PostfixStep(P, e) ⇓ (P_1, e_1)    Γ ⊢ ParsePostfixTail(P_1, e_1) ⇓ (P_2, e_2)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePostfixTail(P, e) ⇓ (P_2, e_2)

PostfixStart = {Punctuator("."), Punctuator("["), Punctuator("("), Operator("~>"), Operator("?")}

**(Postfix-Field)**
IsPunc(Tok(P), ".")    IsIdent(Tok(Advance(P)))    name = Lexeme(Tok(Advance(P)))
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ PostfixStep(P, e) ⇓ (Advance(Advance(P)), FieldAccess(e, name))

**(Postfix-TupleIndex)**
IsPunc(Tok(P), ".")    t = Tok(Advance(P))    t.kind = IntLiteral    idx = IntValue(t)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ PostfixStep(P, e) ⇓ (Advance(Advance(P)), TupleAccess(e, idx))

**(Postfix-Index)**
IsPunc(Tok(P), "[")    Γ ⊢ ParseExpr(Advance(P)) ⇓ (P_1, idx)    IsPunc(Tok(P_1), "]")
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ PostfixStep(P, e) ⇓ (Advance(P_1), IndexAccess(e, idx))

**(Postfix-Call)**
IsPunc(Tok(P), "(")    Γ ⊢ ParseArgList(Advance(P)) ⇓ (P_1, args)    IsPunc(Tok(P_1), ")")
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ PostfixStep(P, e) ⇓ (Advance(P_1), Call(e, args))

**(Postfix-MethodCall)**
IsOp(Tok(P), "~>")    Γ ⊢ ParseIdent(Advance(P)) ⇓ (P_1, name)    IsPunc(Tok(P_1), "(")    Γ ⊢ ParseArgList(Advance(P_1)) ⇓ (P_2, args)    IsPunc(Tok(P_2), ")")
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ PostfixStep(P, e) ⇓ (Advance(P_2), MethodCall(e, name, args))

**(Postfix-Propagate)**
IsOp(Tok(P), "?")
──────────────────────────────────────────────────────────────
Γ ⊢ PostfixStep(P, e) ⇓ (Advance(P), Propagate(e))

**Argument Lists.**

**(Parse-ArgList-Empty)**
IsPunc(Tok(P), ")")
──────────────────────────────────────────────
Γ ⊢ ParseArgList(P) ⇓ (P, [])

**(Parse-ArgList-Cons)**
Γ ⊢ ParseArg(P) ⇓ (P_1, a)    Γ ⊢ ParseArgTail(P_1, [a]) ⇓ (P_2, args)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseArgList(P) ⇓ (P_2, args)

**(Parse-Arg)**
Γ ⊢ ParseArgMoveOpt(P) ⇓ (P_1, moved)    Γ ⊢ ParseExpr(P_1) ⇓ (P_2, e)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseArg(P) ⇓ (P_2, ⟨moved, e, SpanBetween(P, P_2)⟩)

**(Parse-ArgMoveOpt-None)**
¬ IsKw(Tok(P), `move`)
──────────────────────────────────────────────────────────────
Γ ⊢ ParseArgMoveOpt(P) ⇓ (P, false)

**(Parse-ArgMoveOpt-Yes)**
IsKw(Tok(P), `move`)
──────────────────────────────────────────────────────
Γ ⊢ ParseArgMoveOpt(P) ⇓ (Advance(P), true)

**Expression Lists.**

**(Parse-ExprList-Cons)**
Γ ⊢ ParseExpr(P) ⇓ (P_1, e)    Γ ⊢ ParseExprListTail(P_1, [e]) ⇓ (P_2, es)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseExprList(P) ⇓ (P_2, es)

**(Parse-ExprList-Empty)**
Tok(P) ∈ {Punctuator(")"), Punctuator("]")}    Γ ⊢ Emit(Code(Unsupported-Construct))
──────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseExprList(P) ⇓ (P, [])

**Tuple Expression Elements.**

**(Parse-TupleExprElems-Empty)**
IsPunc(Tok(P), ")")
──────────────────────────────────────────────
Γ ⊢ ParseTupleExprElems(P) ⇓ (P, [])

**(Parse-TupleExprElems-Single)**
Γ ⊢ ParseExpr(P) ⇓ (P_1, e)    IsPunc(Tok(P_1), ";")
────────────────────────────────────────────────────────────────
Γ ⊢ ParseTupleExprElems(P) ⇓ (Advance(P_1), [e])

**(Parse-TupleExprElems-TrailingComma)**
Γ ⊢ ParseExpr(P) ⇓ (P_1, e)    IsPunc(Tok(P_1), ",")    IsPunc(Tok(Advance(P_1)), ")")    Γ ⊢ Emit(Code(Unsupported-Construct))
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseTupleExprElems(P) ⇓ (Advance(P_1), [e])

**(Parse-TupleExprElems-Many)**
Γ ⊢ ParseExpr(P) ⇓ (P_1, e_1)    IsPunc(Tok(P_1), ",")    Γ ⊢ ParseExpr(Advance(P_1)) ⇓ (P_2, e_2)    Γ ⊢ ParseExprListTail(P_2, [e_2]) ⇓ (P_3, es)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseTupleExprElems(P) ⇓ (P_3, [e_1] ++ es)

**Tuple vs Parenthesized Disambiguation.**
ParenDelta(Punctuator("(")) = 1
ParenDelta(Punctuator(")")) = -1
ParenDelta(t) = 0 if t.kind ∉ {Punctuator("("), Punctuator(")")}

TupleScan(P, d) ⇓ b
Tok(P) = EOF
────────────────────────────────────────
TupleScan(P, d) ⇓ false
Tok(P) = Punctuator(")") ∧ d = 1
────────────────────────────────────────────
TupleScan(P, d) ⇓ false
Tok(P) ∈ {Punctuator(","), Punctuator(";")} ∧ d = 1
──────────────────────────────────────────────
TupleScan(P, d) ⇓ true
Tok(P) ∉ {Punctuator(")"), Punctuator(","), Punctuator(";")}    TupleScan(Advance(P), d + ParenDelta(Tok(P))) ⇓ b
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
TupleScan(P, d) ⇓ b

TupleParen(P) ⇔ IsPunc(Tok(P), "(") ∧ (IsPunc(Tok(Advance(P)), ")") ∨ TupleScan(Advance(P), 1) ⇓ true)
**Field Initializers.**

**(Parse-FieldInitList-Empty)**
IsPunc(Tok(P), "}")    Γ ⊢ Emit(Code(Unsupported-Construct))
──────────────────────────────────────────────────────────────
Γ ⊢ ParseFieldInitList(P) ⇓ (P, [])

**(Parse-FieldInitList-Cons)**
Γ ⊢ ParseFieldInit(P) ⇓ (P_1, f)    Γ ⊢ ParseFieldInitTail(P_1, [f]) ⇓ (P_2, fs)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseFieldInitList(P) ⇓ (P_2, fs)


**(Parse-FieldInit-Explicit)**
Γ ⊢ ParseIdent(P) ⇓ (P_1, name)    IsPunc(Tok(P_1), ":")    Γ ⊢ ParseExpr(Advance(P_1)) ⇓ (P_2, e)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseFieldInit(P) ⇓ (P_2, ⟨name, e⟩)

**(Parse-FieldInit-Shorthand)**
Γ ⊢ ParseIdent(P) ⇓ (P_1, name)    ¬ IsPunc(Tok(P_1), ":")
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseFieldInit(P) ⇓ (P_1, ⟨name, Identifier(name)⟩)

**Match Arms.**

**(Parse-MatchArms-Cons)**
Γ ⊢ ParseMatchArm(P) ⇓ (P_1, a)    Γ ⊢ ParseMatchArmsTail(P_1, [a]) ⇓ (P_2, arms)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseMatchArms(P) ⇓ (P_2, arms)

**(Parse-MatchArm)**
Γ ⊢ ParsePattern(P) ⇓ (P_1, pat)    Γ ⊢ ParseGuardOpt(P_1) ⇓ (P_2, guard_opt)    IsOp(Tok(P_2), "=>")    Γ ⊢ ParseArmBody(Advance(P_2)) ⇓ (P_3, body)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseMatchArm(P) ⇓ (P_3, ⟨pat, guard_opt, body⟩)

**(Parse-MatchArmsTail-End)**
Tok(P) = Punctuator("}")
──────────────────────────────────────────────────────────────
Γ ⊢ ParseMatchArmsTail(P, xs) ⇓ (P, xs)

**(Parse-MatchArmsTail-Comma)**
IsPunc(Tok(P), ",")    Γ ⊢ ParseMatchArm(Advance(P)) ⇓ (P_1, a)    Γ ⊢ ParseMatchArmsTail(P_1, xs ++ [a]) ⇓ (P_2, ys)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseMatchArmsTail(P, xs) ⇓ (P_2, ys)

**(Parse-GuardOpt-None)**
¬ IsKw(Tok(P), `if`)
──────────────────────────────────────────────
Γ ⊢ ParseGuardOpt(P) ⇓ (P, ⊥)

**(Parse-GuardOpt-Yes)**
IsKw(Tok(P), `if`)    Γ ⊢ ParseExpr(Advance(P)) ⇓ (P_1, e)
────────────────────────────────────────────────────────────────
Γ ⊢ ParseGuardOpt(P) ⇓ (P_1, e)

**(Parse-ArmBody-Block)**
IsPunc(Tok(P), "{")    Γ ⊢ ParseBlock(P) ⇓ (P_1, b)
──────────────────────────────────────────────────────────────
Γ ⊢ ParseArmBody(P) ⇓ (P_1, b)

**(Parse-ArmBody-Expr)**
Γ ⊢ ParseExpr(P) ⇓ (P_1, e)
──────────────────────────────────────────────
Γ ⊢ ParseArmBody(P) ⇓ (P_1, e)

**Argument and Expression Tail Lists.**

**(Parse-ArgTail-End)**
IsPunc(Tok(P), ")")
───────────────────────────────────────────
Γ ⊢ ParseArgTail(P, xs) ⇓ (P, xs)

**(Parse-ArgTail-TrailingComma)**
IsPunc(Tok(P), ",")    IsPunc(Tok(Advance(P)), ")")    Γ ⊢ Emit(Code(Unsupported-Construct))
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseArgTail(P, xs) ⇓ (Advance(P), xs)

**(Parse-ArgTail-Comma)**
IsPunc(Tok(P), ",")    Γ ⊢ ParseArg(Advance(P)) ⇓ (P_1, a)    Γ ⊢ ParseArgTail(P_1, xs ++ [a]) ⇓ (P_2, ys)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseArgTail(P, xs) ⇓ (P_2, ys)

**(Parse-ExprListTail-End)**
Tok(P) ∈ {Punctuator(")"), Punctuator("]"), Punctuator("}")}
──────────────────────────────────────────────────────────────
Γ ⊢ ParseExprListTail(P, xs) ⇓ (P, xs)

**(Parse-ExprListTail-TrailingComma)**
IsPunc(Tok(P), ",")    Tok(Advance(P)) ∈ {Punctuator(")"), Punctuator("]"), Punctuator("}")}    Γ ⊢ Emit(Code(Unsupported-Construct))
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseExprListTail(P, xs) ⇓ (Advance(P), xs)

**(Parse-ExprListTail-Comma)**
IsPunc(Tok(P), ",")    Γ ⊢ ParseExpr(Advance(P)) ⇓ (P_1, e)    Γ ⊢ ParseExprListTail(P_1, xs ++ [e]) ⇓ (P_2, ys)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseExprListTail(P, xs) ⇓ (P_2, ys)

**(Parse-FieldInitTail-End)**
IsPunc(Tok(P), "}")
──────────────────────────────────────────────
Γ ⊢ ParseFieldInitTail(P, xs) ⇓ (P, xs)

**(Parse-FieldInitTail-TrailingComma)**
IsPunc(Tok(P), ",")    IsPunc(Tok(Advance(P)), "}")    Γ ⊢ Emit(Code(Unsupported-Construct))
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseFieldInitTail(P, xs) ⇓ (Advance(P), xs)

**(Parse-FieldInitTail-Comma)**
IsPunc(Tok(P), ",")    Γ ⊢ ParseFieldInit(Advance(P)) ⇓ (P_1, f)    Γ ⊢ ParseFieldInitTail(P_1, xs ++ [f]) ⇓ (P_2, ys)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseFieldInitTail(P, xs) ⇓ (P_2, ys)
**Loop Tail.**

**(Parse-LoopTail-Infinite)**
IsPunc(Tok(P), "{")    Γ ⊢ ParseBlock(P) ⇓ (P_1, b)
────────────────────────────────────────────────────────────────
Γ ⊢ ParseLoopTail(P) ⇓ (P_1, LoopInfinite(b))

**(Parse-LoopTail-Iter)**
Γ ⊢ TryParsePatternIn(P) ⇓ (P_1, pat)    Γ ⊢ ParseTypeAnnotOpt(P_1) ⇓ (P_2, ty_opt)    IsIdent(Tok(P_2))    Lexeme(Tok(P_2)) = `in`    Γ ⊢ ParseExpr_NoBrace(Advance(P_2)) ⇓ (P_3, iter)    Γ ⊢ ParseBlock(P_3) ⇓ (P_4, body)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseLoopTail(P) ⇓ (P_4, LoopIter(pat, ty_opt, iter, body))

**(Parse-LoopTail-Cond)**
Γ ⊢ ParseExpr_NoBrace(P) ⇓ (P_1, cond)    Γ ⊢ ParseBlock(P_1) ⇓ (P_2, body)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseLoopTail(P) ⇓ (P_2, LoopConditional(cond, body))

**(TryParsePatternIn-Ok)**
P_c = Clone(P)    Γ ⊢ ParsePattern(P_c) ⇓ (P_1, pat)    Γ ⊢ ParseTypeAnnotOpt(P_1) ⇓ (P_2, ty_opt)    IsIdent(Tok(P_2))    Lexeme(Tok(P_2)) = `in`    P' = MergeDiag(P, P_2, P_1)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TryParsePatternIn(P) ⇓ (P', pat)

**(TryParsePatternIn-Fail)**
P_c = Clone(P)    ¬ (Γ ⊢ ParsePattern(P_c) ⇓ (P_1, pat) ∧ Γ ⊢ ParseTypeAnnotOpt(P_1) ⇓ (P_2, ty_opt) ∧ IsIdent(Tok(P_2)) ∧ Lexeme(Tok(P_2)) = `in`)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TryParsePatternIn(P) ↑

**Else Clause.**

**(Parse-ElseOpt-None)**
¬ IsKw(Tok(P), `else`)
──────────────────────────────────────────────
Γ ⊢ ParseElseOpt(P) ⇓ (P, ⊥)

**(Parse-ElseOpt-If)**
IsKw(Tok(P), `else`)    IsKw(Tok(Advance(P)), `if`)    Γ ⊢ ParsePrimary(Advance(P)) ⇓ (P_1, e)
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseElseOpt(P) ⇓ (P_1, e)

**(Parse-ElseOpt-Block)**
IsKw(Tok(P), `else`)    Γ ⊢ ParseBlock(Advance(P)) ⇓ (P_1, b)
────────────────────────────────────────────────────────────────
Γ ⊢ ParseElseOpt(P) ⇓ (P_1, b)

**Optional Expression.**

**(Parse-ExprOpt-None)**
Tok(P) ∈ {Punctuator(";"), Punctuator("}"), Newline, EOF}
──────────────────────────────────────────────────────────────
Γ ⊢ ParseExprOpt(P) ⇓ (P, ⊥)

**(Parse-ExprOpt-Yes)**
Tok(P) ∉ {Punctuator(";"), Punctuator("}"), Newline, EOF}    Γ ⊢ ParseExpr(P) ⇓ (P_1, e)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseExprOpt(P) ⇓ (P_1, e)

**Place Expressions.**

IsPlace(e) ⇔ e ∈ {Identifier(_), FieldAccess(_, _), TupleAccess(_, _), IndexAccess(_, _)} ∨ (∃ p. e = Deref(p) ∧ IsPlace(p))
PlaceExprParseErr = Parse-Syntax-Err

**(Parse-Place-Deref)**
IsOp(Tok(P), "*")    Γ ⊢ ParsePlace(Advance(P)) ⇓ (P_1, p)
────────────────────────────────────────────────────────────────
Γ ⊢ ParsePlace(P) ⇓ (P_1, Deref(p))

**(Parse-Place-Postfix)**
Γ ⊢ ParsePostfix(P) ⇓ (P_1, e)    IsPlace(e)
──────────────────────────────────────────────────────────────
Γ ⊢ ParsePlace(P) ⇓ (P_1, e)

**(Parse-Place-Err)**
Γ ⊢ ParsePostfix(P) ⇓ (P_1, e)    ¬ IsPlace(e)    c = Code(PlaceExprParseErr)    Γ ⊢ Emit(c, Tok(P).span)    Γ ⊢ SyncStmt(P_1) ⇓ P_2
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePlace(P) ⇓ (P_2, ErrorExpr(SpanBetween(P, P_2)))

#### 3.3.9. Pattern Parsing

**(Parse-Pattern)**
Γ ⊢ ParsePatternRange(P) ⇓ (P_1, pat)
───────────────────────────────────────────────
Γ ⊢ ParsePattern(P) ⇓ (P_1, pat)

**(Parse-Pattern-Err)**
c = Code(Parse-Syntax-Err)    Γ ⊢ Emit(c, Tok(P).span)
────────────────────────────────────────────────────────────────
Γ ⊢ ParsePattern(P) ⇓ (P, WildcardPattern)

**(Parse-Pattern-Range-None)**
Γ ⊢ ParsePatternAtom(P) ⇓ (P_1, p)    ¬ (IsOp(Tok(P_1), "..") ∨ IsOp(Tok(P_1), "..="))
──────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePatternRange(P) ⇓ (P_1, p)

**(Parse-Pattern-Range)**
Γ ⊢ ParsePatternAtom(P) ⇓ (P_1, p_0)    Tok(P_1) = op ∈ {"..", "..="}    Γ ⊢ ParsePatternAtom(Advance(P_1)) ⇓ (P_2, p_1)    kind = (`Exclusive` if op = ".." else `Inclusive`)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePatternRange(P) ⇓ (P_2, RangePattern(kind, p_0, p_1))

**(Parse-Pattern-Literal)**
Tok(P).kind ∈ {IntLiteral, FloatLiteral, StringLiteral, CharLiteral, BoolLiteral, NullLiteral}
───────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePatternAtom(P) ⇓ (Advance(P), LiteralPattern(Tok(P)))

**(Parse-Pattern-Wildcard)**
IsIdent(Tok(P))    Lexeme(Tok(P)) = "_"
──────────────────────────────────────────────────────────────
Γ ⊢ ParsePatternAtom(P) ⇓ (Advance(P), WildcardPattern)

**(Parse-Pattern-Typed)**
IsIdent(Tok(P))    IsPunc(Tok(Advance(P)), ":")    Γ ⊢ ParseType(Advance(Advance(P))) ⇓ (P_1, ty)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePatternAtom(P) ⇓ (P_1, TypedPattern(Lexeme(Tok(P)), ty))

**(Parse-Pattern-Identifier)**
IsIdent(Tok(P))
────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePatternAtom(P) ⇓ (Advance(P), IdentifierPattern(Lexeme(Tok(P))))

**(Parse-Pattern-Tuple)**
IsPunc(Tok(P), "(")    Γ ⊢ ParseTuplePatternElems(Advance(P)) ⇓ (P_1, elems)    IsPunc(Tok(P_1), ")")
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePatternAtom(P) ⇓ (Advance(P_1), TuplePattern(elems))

**(Parse-Pattern-Record)**
Γ ⊢ ParseTypePath(P) ⇓ (P_1, path)    IsPunc(Tok(P_1), "{")    Γ ⊢ ParseFieldPatternList(Advance(P_1)) ⇓ (P_2, fields)    IsPunc(Tok(P_2), "}")
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePatternAtom(P) ⇓ (Advance(P_2), RecordPattern(path, fields))

**(Parse-Pattern-Enum)**
Γ ⊢ ParseTypePath(P) ⇓ (P_1, path)    IsOp(Tok(P_1), "::")    Γ ⊢ ParseIdent(Advance(P_1)) ⇓ (P_2, name)    Γ ⊢ ParseEnumPatternPayloadOpt(P_2) ⇓ (P_3, payload_opt)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePatternAtom(P) ⇓ (P_3, EnumPattern(path, name, payload_opt))

**(Parse-Pattern-Modal)**
IsOp(Tok(P), "@")    Γ ⊢ ParseIdent(Advance(P)) ⇓ (P_1, name)    Γ ⊢ ParseModalPatternPayloadOpt(P_1) ⇓ (P_2, fields_opt)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePatternAtom(P) ⇓ (P_2, ModalPattern(name, fields_opt))

##### 3.3.9.1. Pattern Helper Parsing Rules

**Pattern Lists.**

**(Parse-PatternList-Empty)**
IsPunc(Tok(P), ")")
──────────────────────────────────────────────
Γ ⊢ ParsePatternList(P) ⇓ (P, [])

**(Parse-PatternList-Cons)**
Γ ⊢ ParsePattern(P) ⇓ (P_1, p)    Γ ⊢ ParsePatternListTail(P_1, [p]) ⇓ (P_2, ps)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePatternList(P) ⇓ (P_2, ps)

**(Parse-PatternListTail-End)**
IsPunc(Tok(P), ")")
──────────────────────────────────────────────
Γ ⊢ ParsePatternListTail(P, xs) ⇓ (P, xs)

**(Parse-PatternListTail-TrailingComma)**
IsPunc(Tok(P), ",")    IsPunc(Tok(Advance(P)), ")")    Γ ⊢ Emit(Code(Unsupported-Construct))
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePatternListTail(P, xs) ⇓ (Advance(P), xs)

**(Parse-PatternListTail-Comma)**
IsPunc(Tok(P), ",")    Γ ⊢ ParsePattern(Advance(P)) ⇓ (P_1, p)    Γ ⊢ ParsePatternListTail(P_1, xs ++ [p]) ⇓ (P_2, ys)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParsePatternListTail(P, xs) ⇓ (P_2, ys)

**Tuple Pattern Elements.**

**(Parse-TuplePatternElems-Empty)**
IsPunc(Tok(P), ")")
──────────────────────────────────────────────
Γ ⊢ ParseTuplePatternElems(P) ⇓ (P, [])

**(Parse-TuplePatternElems-Single)**
Γ ⊢ ParsePattern(P) ⇓ (P_1, p)    IsPunc(Tok(P_1), ";")
────────────────────────────────────────────────────────────────
Γ ⊢ ParseTuplePatternElems(P) ⇓ (Advance(P_1), [p])

**(Parse-TuplePatternElems-TrailingComma)**
Γ ⊢ ParsePattern(P) ⇓ (P_1, p)    IsPunc(Tok(P_1), ",")    IsPunc(Tok(Advance(P_1)), ")")    Γ ⊢ Emit(Code(Unsupported-Construct))
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseTuplePatternElems(P) ⇓ (Advance(P_1), [p])

**(Parse-TuplePatternElems-Many)**
Γ ⊢ ParsePattern(P) ⇓ (P_1, p_1)    IsPunc(Tok(P_1), ",")    Γ ⊢ ParsePattern(Advance(P_1)) ⇓ (P_2, p_2)    Γ ⊢ ParsePatternListTail(P_2, [p_2]) ⇓ (P_3, ps)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseTuplePatternElems(P) ⇓ (P_3, [p_1] ++ ps)

**Field Pattern Lists.**

**(Parse-FieldPatternList-Empty)**
IsPunc(Tok(P), "}")
────────────────────────────────────────────────
Γ ⊢ ParseFieldPatternList(P) ⇓ (P, [])

**(Parse-FieldPatternList-Cons)**
Γ ⊢ ParseFieldPattern(P) ⇓ (P_1, f)    Γ ⊢ ParseFieldPatternTail(P_1, [f]) ⇓ (P_2, fs)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseFieldPatternList(P) ⇓ (P_2, fs)

**(Parse-FieldPattern)**
Γ ⊢ ParseIdent(P) ⇓ (P_1, name)    Γ ⊢ ParseFieldPatternTailOpt(P_1, name) ⇓ (P_2, pat_opt)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseFieldPattern(P) ⇓ (P_2, ⟨name, pat_opt, SpanBetween(P, P_2)⟩)

**(Parse-FieldPatternTailOpt-None)**
¬ IsPunc(Tok(P), ":")
──────────────────────────────────────────────────────────────
Γ ⊢ ParseFieldPatternTailOpt(P, name) ⇓ (P, ⊥)

**(Parse-FieldPatternTailOpt-Yes)**
IsPunc(Tok(P), ":")    Γ ⊢ ParsePattern(Advance(P)) ⇓ (P_1, pat)
────────────────────────────────────────────────────────────────
Γ ⊢ ParseFieldPatternTailOpt(P, name) ⇓ (P_1, pat)

**(Parse-FieldPatternTail-End)**
IsPunc(Tok(P), "}")
────────────────────────────────────────────────
Γ ⊢ ParseFieldPatternTail(P, xs) ⇓ (P, xs)

**(Parse-FieldPatternTail-TrailingComma)**
IsPunc(Tok(P), ",")    IsPunc(Tok(Advance(P)), "}")    Γ ⊢ Emit(Code(Unsupported-Construct))
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseFieldPatternTail(P, xs) ⇓ (Advance(P), xs)

**(Parse-FieldPatternTail-Comma)**
IsPunc(Tok(P), ",")    Γ ⊢ ParseFieldPattern(Advance(P)) ⇓ (P_1, f)    Γ ⊢ ParseFieldPatternTail(P_1, xs ++ [f]) ⇓ (P_2, ys)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseFieldPatternTail(P, xs) ⇓ (P_2, ys)
**Enum Pattern Payload.**

**(Parse-EnumPatternPayloadOpt-None)**
Tok(P) ∉ {Punctuator("("), Punctuator("{")}
──────────────────────────────────────────────────────────────
Γ ⊢ ParseEnumPatternPayloadOpt(P) ⇓ (P, ⊥)

**(Parse-EnumPatternPayloadOpt-Tuple)**
IsPunc(Tok(P), "(")    Γ ⊢ ParseTuplePatternElems(Advance(P)) ⇓ (P_1, ps)    IsPunc(Tok(P_1), ")")
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseEnumPatternPayloadOpt(P) ⇓ (Advance(P_1), TuplePayloadPattern(ps))

**(Parse-EnumPatternPayloadOpt-Record)**
IsPunc(Tok(P), "{")    Γ ⊢ ParseFieldPatternList(Advance(P)) ⇓ (P_1, fs)    IsPunc(Tok(P_1), "}")
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseEnumPatternPayloadOpt(P) ⇓ (Advance(P_1), RecordPayloadPattern(fs))

**Modal Pattern Payload.**

**(Parse-ModalPatternPayloadOpt-None)**
¬ IsPunc(Tok(P), "{")
──────────────────────────────────────────────────────────────
Γ ⊢ ParseModalPatternPayloadOpt(P) ⇓ (P, ⊥)

**(Parse-ModalPatternPayloadOpt-Record)**
IsPunc(Tok(P), "{")    Γ ⊢ ParseFieldPatternList(Advance(P)) ⇓ (P_1, fs)    IsPunc(Tok(P_1), "}")
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseModalPatternPayloadOpt(P) ⇓ (Advance(P_1), ModalRecordPayload(fs))

#### 3.3.10. Statement and Block Parsing

**Statement Terminators.**
StmtTerm = {Punctuator(";"), Newline}
Terminates(t) ⇔ t ∈ StmtTerm

**(Parse-Statement)**
Γ ⊢ ParseStmtCore(P) ⇓ (P_1, s)    Γ ⊢ ConsumeTerminatorOpt(P_1, s) ⇓ P_2
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseStmt(P) ⇓ (P_2, s)

**(Parse-Statement-Err)**
c = Code(Parse-Syntax-Err)    Γ ⊢ Emit(c, Tok(P).span)    P_1 = AdvanceOrEOF(P)    Γ ⊢ SyncStmt(P_1) ⇓ P_2
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseStmt(P) ⇓ (P_2, ErrorStmt(SpanBetween(P, P_2)))


**(Parse-Binding-Stmt)**
Tok(P) ∈ {Keyword(`let`), Keyword(`var`)}    Γ ⊢ ParseBindingAfterLetVar(P) ⇓ (P_1, bind)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseStmtCore(P) ⇓ (P_1, LetOrVarStmt(P, bind))

**(Parse-Shadow-Stmt)**
IsKw(Tok(P), `shadow`)    Tok(Advance(P)) ∈ {Keyword(`let`), Keyword(`var`)}    Γ ⊢ ParseShadowBinding(Advance(P)) ⇓ (P_1, s)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseStmtCore(P) ⇓ (P_1, s)

**(Parse-Assign-Stmt)**
Γ ⊢ ParsePlace(P) ⇓ (P_1, p)    Tok(P_1) ∈ {Operator("="), Operator("+="), Operator("-="), Operator("*="), Operator("/="), Operator("%=")}    Γ ⊢ ParseExpr(Advance(P_1)) ⇓ (P_2, e)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseStmtCore(P) ⇓ (P_2, AssignOrCompound(P_1, p, e))

**(Parse-Return-Stmt)**
IsKw(Tok(P), `return`)    Γ ⊢ ParseExprOpt(Advance(P)) ⇓ (P_1, e_opt)
──────────────────────────────────────────────────────────────
Γ ⊢ ParseStmtCore(P) ⇓ (P_1, ReturnStmt(e_opt))

**(Parse-Result-Stmt)**
IsKw(Tok(P), `result`)    Γ ⊢ ParseExpr(Advance(P)) ⇓ (P_1, e)
──────────────────────────────────────────────────────────────
Γ ⊢ ParseStmtCore(P) ⇓ (P_1, ResultStmt(e))

**(Parse-Break-Stmt)**
IsKw(Tok(P), `break`)    Γ ⊢ ParseExprOpt(Advance(P)) ⇓ (P_1, e_opt)
──────────────────────────────────────────────────────────────
Γ ⊢ ParseStmtCore(P) ⇓ (P_1, BreakStmt(e_opt))

**(Parse-Continue-Stmt)**
IsKw(Tok(P), `continue`)
──────────────────────────────────────────────
Γ ⊢ ParseStmtCore(P) ⇓ (Advance(P), ContinueStmt)

**(Parse-Unsafe-Block)**
IsKw(Tok(P), `unsafe`)    Γ ⊢ ParseBlock(Advance(P)) ⇓ (P_1, b)
────────────────────────────────────────────────────────────────
Γ ⊢ ParseStmtCore(P) ⇓ (P_1, UnsafeBlockStmt(b))

**(Parse-Defer-Stmt)**
IsKw(Tok(P), `defer`)    Γ ⊢ ParseBlock(Advance(P)) ⇓ (P_1, b)
──────────────────────────────────────────────────────────────
Γ ⊢ ParseStmtCore(P) ⇓ (P_1, DeferStmt(b))

**(Parse-Region-Opts-None)**
¬ IsPunc(Tok(P), "(")
────────────────────────────────────────────────
Γ ⊢ ParseRegionOptsOpt(P) ⇓ (P, ⊥)

**(Parse-Region-Opts-Some)**
IsPunc(Tok(P), "(")    Γ ⊢ ParseExpr(Advance(P)) ⇓ (P_1, e)    IsPunc(Tok(P_1), ")")
────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseRegionOptsOpt(P) ⇓ (Advance(P_1), e)

**(Parse-Region-Alias-None)**
¬ IsKw(Tok(P), `as`)
──────────────────────────────────────────────
Γ ⊢ ParseRegionAliasOpt(P) ⇓ (P, ⊥)

**(Parse-Region-Alias-Some)**
IsKw(Tok(P), `as`)    Γ ⊢ ParseIdent(Advance(P)) ⇓ (P_1, name)
────────────────────────────────────────────────────────────────
Γ ⊢ ParseRegionAliasOpt(P) ⇓ (P_1, name)

**(Parse-Region-Stmt)**
IsKw(Tok(P), `region`)    Γ ⊢ ParseRegionOptsOpt(Advance(P)) ⇓ (P_1, opts_opt)    Γ ⊢ ParseRegionAliasOpt(P_1) ⇓ (P_2, alias_opt)    Γ ⊢ ParseBlock(P_2) ⇓ (P_3, b)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseStmtCore(P) ⇓ (P_3, RegionStmt(opts_opt, alias_opt, b))

**(Parse-Frame-Stmt)**
IsKw(Tok(P), `frame`)    Γ ⊢ ParseBlock(Advance(P)) ⇓ (P_1, b)
────────────────────────────────────────────────────────────────
Γ ⊢ ParseStmtCore(P) ⇓ (P_1, FrameStmt(⊥, b))

**(Parse-Frame-Explicit)**
IsIdent(Tok(P))    IsPunc(Tok(Advance(P)), ".")    IsKw(Tok(Advance(Advance(P))), `frame`)    name = Lexeme(Tok(P))    Γ ⊢ ParseBlock(Advance(Advance(Advance(P)))) ⇓ (P_1, b)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseStmtCore(P) ⇓ (P_1, FrameStmt(name, b))

**(Parse-Expr-Stmt)**
Γ ⊢ ParseExpr(P) ⇓ (P_1, e)
──────────────────────────────────────────────
Γ ⊢ ParseStmtCore(P) ⇓ (P_1, ExprStmt(e))

**Block Parsing.**

**(Parse-Block)**
IsPunc(Tok(P), "{")    Γ ⊢ ParseStmtSeq(Advance(P)) ⇓ (P_1, stmts, tail)    IsPunc(Tok(P_1), "}")
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseBlock(P) ⇓ (Advance(P_1), BlockExpr(stmts, tail))

##### 3.3.10.1. Statement Helper Parsing Rules
**Terminator Consumption.**

ReqTerm(s) ⇔ s ∈ {LetStmt(_), VarStmt(_), ShadowLetStmt(_), ShadowVarStmt(_), AssignStmt(_, _), CompoundAssignStmt(_, _, _), ExprStmt(_)}

**(ConsumeTerminatorOpt-Req-Yes)**
ReqTerm(s)    IsTerm(Tok(P))
──────────────────────────────────────────────
Γ ⊢ ConsumeTerminatorOpt(P, s) ⇓ Advance(P)

**(ConsumeTerminatorOpt-Req-No)**
ReqTerm(s)    ¬ IsTerm(Tok(P))    Emit(Code(Missing-Terminator-Err), Span = Tok(P).span)    Γ ⊢ SyncStmt(P) ⇓ P_1
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ConsumeTerminatorOpt(P, s) ⇓ P_1

**(ConsumeTerminatorOpt-Opt-Yes)**
¬ ReqTerm(s)    IsTerm(Tok(P))
──────────────────────────────────────────────
Γ ⊢ ConsumeTerminatorOpt(P, s) ⇓ Advance(P)

**(ConsumeTerminatorOpt-Opt-No)**
¬ ReqTerm(s)    ¬ IsTerm(Tok(P))
──────────────────────────────────────────────
Γ ⊢ ConsumeTerminatorOpt(P, s) ⇓ P

**(ConsumeTerminatorReq-Yes)**
Tok(P) ∈ {Punctuator(";"), Newline}
──────────────────────────────────────────────
Γ ⊢ ConsumeTerminatorReq(P) ⇓ Advance(P)

**(ConsumeTerminatorReq-No)**
Tok(P) ∉ {Punctuator(";"), Newline}    c = Code(Missing-Terminator-Err)    Γ ⊢ Emit(c, Tok(P).span)    Γ ⊢ SyncStmt(P) ⇓ P_1
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ConsumeTerminatorReq(P) ⇓ P_1

**Let/Var Statements.**

**(LetOrVarStmt-Let)**
Tok(P) = Keyword(`let`)
──────────────────────────────────────────────
Γ ⊢ LetOrVarStmt(P, bind) ⇓ LetStmt(bind)

**(LetOrVarStmt-Var)**
Tok(P) = Keyword(`var`)
──────────────────────────────────────────────
Γ ⊢ LetOrVarStmt(P, bind) ⇓ VarStmt(bind)

**Assignments.**

**(AssignOrCompound-Assign)**
Tok(P_1) = Operator("=")
──────────────────────────────────────────────
Γ ⊢ AssignOrCompound(P_1, p, e) ⇓ AssignStmt(p, e)

**(AssignOrCompound-Compound)**
Tok(P_1) = Operator(op)    op ∈ {"+=", "-=", "*=", "/=", "%="}
────────────────────────────────────────────────────────────────────────────
Γ ⊢ AssignOrCompound(P_1, p, e) ⇓ CompoundAssignStmt(p, op, e)

**Statement Sequences.**

**(ParseStmtSeq-End)**
Tok(P) = Punctuator("}")
──────────────────────────────────────────────
Γ ⊢ ParseStmtSeq(P) ⇓ (P, [], ⊥)

**(ParseStmtSeq-TailExpr)**
Tok(P) ∉ {Punctuator("}")}    Γ ⊢ ParseExpr(P) ⇓ (P_1, e)    Tok(P_1) = Punctuator("}")
──────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseStmtSeq(P) ⇓ (P_1, [], e)

**(ParseStmtSeq-Cons)**
Γ ⊢ ParseStmt(P) ⇓ (P_1, s)    Γ ⊢ ParseStmtSeq(P_1) ⇓ (P_2, ss, tail)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseStmtSeq(P) ⇓ (P_2, [s] ++ ss, tail)

#### 3.3.11. Doc Comment Association (Phase 1)

DocSeq(D) = D
ItemSeq(Items) = Items

**(Attach-Doc-Line)**
d.kind = LineDoc    Items = [i_1, …, i_k]    j = min{ t | d.span.end_offset ≤ i_t.span.start_offset }
──────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ AttachDoc(d, i_j)
LineDocTarget(d, Items) = i_j ⇔ Γ ⊢ AttachDoc(d, i_j)
LineDocsFor(i, D, Items) = [d ∈ D | d.kind = LineDoc ∧ LineDocTarget(d, Items) = i]

**(Attach-Doc-Module)**
d.kind = ModuleDoc
──────────────────────────────────────────────
Γ ⊢ AttachModuleDoc(d)

ModuleDocs(D) = [d ∈ D | d.kind = ModuleDoc]

#### 3.3.12. Error Recovery and Synchronization

**Statement Synchronization Set.**

SyncStmt = {Punctuator(";"), Newline, Punctuator("}"), EOF}

**Item Synchronization Set.**

SyncItem = {Keyword(`procedure`), Keyword(`record`), Keyword(`enum`), Keyword(`modal`), Keyword(`class`), Keyword(`type`), Keyword(`using`), Keyword(`let`), Keyword(`var`), Punctuator("}"), EOF}

**Type Synchronization Set.**

SyncType = {Punctuator(","), Punctuator(";"), Newline, Punctuator(")"), Punctuator("]"), Punctuator("}"), EOF}

**(Sync-Stmt-Stop)**
Tok(P) ∈ {Punctuator("}"), EOF}
──────────────────────────────────────────────
Γ ⊢ SyncStmt(P) ⇓ P

**(Sync-Stmt-Consume)**
Tok(P) ∈ {Punctuator(";"), Newline}
──────────────────────────────────────────────
Γ ⊢ SyncStmt(P) ⇓ Advance(P)

**(Sync-Stmt-Advance)**
Tok(P) ∉ SyncStmt
──────────────────────────────────────────────
Γ ⊢ SyncStmt(P) ⇓ SyncStmt(Advance(P))

**(Sync-Item-Stop)**
Tok(P) ∈ SyncItem
──────────────────────────────────────────────
Γ ⊢ SyncItem(P) ⇓ P

**(Sync-Item-Advance)**
Tok(P) ∉ SyncItem
──────────────────────────────────────────────
Γ ⊢ SyncItem(P) ⇓ SyncItem(Advance(P))

**(Sync-Type-Stop)**
Tok(P) ∈ {Punctuator(")"), Punctuator("]"), Punctuator("}"), EOF}
──────────────────────────────────────────────
Γ ⊢ SyncType(P) ⇓ P

**(Sync-Type-Consume)**
Tok(P) ∈ {Punctuator(","), Punctuator(";"), Newline}
──────────────────────────────────────────────
Γ ⊢ SyncType(P) ⇓ Advance(P)

**(Sync-Type-Advance)**
Tok(P) ∉ SyncType
──────────────────────────────────────────────
Γ ⊢ SyncType(P) ⇓ SyncType(Advance(P))

StmtParseErrRule = Parse-Statement-Err
ItemParseErrRule = Parse-Item-Err

#### 3.3.13. Diagnostics (Phase 1)

Phase1DiagRules = {Unsupported-Construct, Missing-Terminator-Err, Parse-Syntax-Err}

**(Parse-Syntax-Err)**
GenericParseRules = {Parse-Ident-Err, Parse-Type-Err, Parse-Pattern-Err, Parse-Primary-Err, Parse-Statement-Err, Parse-Item-Err}

r ∈ GenericParseRules    PremisesHold(r, P)
────────────────────────────────────────────────
Γ ⊢ Emit(Code(Parse-Syntax-Err))

### 3.4. Module Aggregation

#### 3.4.1. Inputs, Outputs, and Invariants

ModuleAggInputs(P) = ⟨P.modules, P.source_root, { CompilationUnit(DirOf(p, P.source_root)) | p ∈ P.modules }⟩
ModuleAggOutputs(P) = ⟨{ ParseModule(p, P.source_root) | p ∈ P.modules }, { NameMap(P, p) | p ∈ P.modules }, G, InitOrder, InitPlan⟩
ModuleMap(P, p) = M ⇔ Γ ⊢ ParseModules(P) ⇓ Ms ∧ M ∈ Ms ∧ M.path = p
PathOfModule(p) = [c_1, …, c_n] ⇔ p = c_1 :: ··· :: c_n
StringOfPath([c_1, …, c_n]) = Join("::", [c_1, …, c_n])
NameCollectAfterParse(P) ⇔ Γ ⊢ ParseModules(P) ⇓ Ms ∧ ∀ M ∈ Ms. ∃ N. Γ ⊢ CollectNames(M) ⇓ N
NameCollectOrderIndepRef = {"5.1.5"}
ForwardRefOrderRef = {"5.12"}

#### 3.4.2. Module Aggregation (`ParseModule`)

**Module Directory of a Module Path.**

**(DirOf-Root)**
p = A
──────────────────────────────────────────────
Γ ⊢ DirOf(p, S) = S

**(DirOf-Rel)**
p = c_1 :: ··· :: c_n    n ≥ 1
──────────────────────────────────────────────────────────────
Γ ⊢ DirOf(p, S) = S / c_1 / ··· / c_n

A = P.assembly.name

**ParseModule (Big-Step).**
U = CompilationUnit(DirOf(p, S))
U = [f_1, …, f_n]
ReadBytes : Path ⇀ Bytes

**(ReadBytes-Ok)**
read_ok(f) = B
──────────────────────────────────────────────
Γ ⊢ ReadBytes(f) ⇓ B

**(ReadBytes-Err)**
read_ok(f) ⇑    c = Code(ReadBytes-Err)
──────────────────────────────────────────────
Γ ⊢ ReadBytes(f) ⇑ c

Bytes(f) = B ⇔ Γ ⊢ ReadBytes(f) ⇓ B

**(ParseModule-Ok)**
∀ i, Γ ⊢ ReadBytes(f_i) ⇓ B_i    Γ ⊢ LoadSource(f_i, B_i) ⇓ S_i    Γ ⊢ ParseFile(S_i) ⇓ F_i
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseModule(p, S) ⇓ ⟨p, F_1.items ++ ··· ++ F_n.items, F_1.module_doc ++ ··· ++ F_n.module_doc⟩

**(ParseModule-Err-Read)**
∃ i, Γ ⊢ ReadBytes(f_i) ⇑ c
──────────────────────────────────────────────
Γ ⊢ ParseModule(p, S) ⇑ c

**(ParseModule-Err-Load)**
∃ i, Γ ⊢ ReadBytes(f_i) ⇓ B_i    Γ ⊢ LoadSource(f_i, B_i) ⇑ c
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseModule(p, S) ⇑ c

**LoadSource Short-Circuit.**
If Γ ⊢ LoadSource(f, B) ⇑ c for any file in a compilation unit, ParseModule MUST NOT invoke Tokenize, ParseFile, or subset checks for that file.

**(ParseModule-Err-Unit)**
Γ ⊢ CompilationUnit(DirOf(p, S)) ⇑ c
──────────────────────────────────────────────
Γ ⊢ ParseModule(p, S) ⇑ c

**(ParseModule-Err-Parse)**
∃ i, Γ ⊢ ReadBytes(f_i) ⇓ B_i    Γ ⊢ LoadSource(f_i, B_i) ⇓ S_i    Γ ⊢ ParseFile(S_i) ⇑ c
──────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseModule(p, S) ⇑ c

ParseFileBestEffort(S) ⇔ ∃ F. Γ ⊢ ParseFile(S) ⇓ F
ParseFileOk(S) ⇔ ParseFileBestEffort(S) ∧ ¬ HasError(ParseFileDiag(S))
ParseFileDiag(S) = Δ ⇔ Γ ⊢ Tokenize(S) ⇓ (K_raw, D) ∧ K = Filter(K_raw) ∧ P_0 = ⟨K, 0, D, 0, 0, []⟩ ∧ Γ ⊢ ParseItems(P_0) ⇓ (P_1, I, MDoc) ∧ DiagStream(P_1) = Δ
HasError(Δ) ⇔ ∃ d ∈ Δ. d.severity = Error

**ParseModule (Small-Step).**
ModState = {ModStart(p, S), ModScan(p, S, U, items, docs), ModDone(M), Error(code)}

**(Mod-Start)**
U = CompilationUnit(DirOf(p, S))
──────────────────────────────────────────────
⟨ModStart(p, S)⟩ → ⟨ModScan(p, S, U, [], [])⟩

**(Mod-Start-Err-Unit)**
Γ ⊢ CompilationUnit(DirOf(p, S)) ⇑ c
──────────────────────────────────────────────
⟨ModStart(p, S)⟩ → ⟨Error(c)⟩

**(Mod-Scan)**
U = f :: fs    Γ ⊢ ReadBytes(f) ⇓ B    Γ ⊢ LoadSource(f, B) ⇓ S_f    Γ ⊢ ParseFile(S_f) ⇓ F
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨ModScan(p, S, f :: fs, items, docs)⟩ → ⟨ModScan(p, S, fs, items ++ F.items, docs ++ F.module_doc)⟩

**(Mod-Scan-Err-Read)**
U = f :: fs    Γ ⊢ ReadBytes(f) ⇑ c
──────────────────────────────────────────────────────────────
⟨ModScan(p, S, f :: fs, items, docs)⟩ → ⟨Error(c)⟩

**(Mod-Scan-Err-Load)**
U = f :: fs    Γ ⊢ ReadBytes(f) ⇓ B    Γ ⊢ LoadSource(f, B) ⇑ c
──────────────────────────────────────────────────────────────────────────────
⟨ModScan(p, S, f :: fs, items, docs)⟩ → ⟨Error(c)⟩

**(Mod-Scan-Err-Parse)**
U = f :: fs    Γ ⊢ ReadBytes(f) ⇓ B    Γ ⊢ LoadSource(f, B) ⇓ S_f    Γ ⊢ ParseFile(S_f) ⇑ c
──────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨ModScan(p, S, f :: fs, items, docs)⟩ → ⟨Error(c)⟩

**(Mod-Done)**
──────────────────────────────────────────────────────────────────────────────
⟨ModScan(p, S, [], items, docs)⟩ → ⟨ModDone(⟨p, items, docs⟩)⟩

**ParseModules (Big-Step).**

P.modules = [p_1, …, p_k]

**(ParseModules-Ok)**
∀ i, Γ ⊢ ParseModule(p_i, P.source_root) ⇓ M_i
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ ParseModules(P) ⇓ [M_1, …, M_k]

**(ParseModules-Err)**
∃ i, Γ ⊢ ParseModule(p_i, P.source_root) ⇑ c
──────────────────────────────────────────────
Γ ⊢ ParseModules(P) ⇑ c

## 4. Phase 2: Compile-Time Execution (Deferred in Cursive0)

ComptimeForm = {`comptime`}
ComptimeForm ⊆ UnsupportedForm

## 5. Phase 3: Name Resolution + Type Checking

### 5.1. Name Resolution and Scopes (Cursive0)

#### 5.1.1. Scope Context and Identifiers

IdKeyRef = {"3.1.6"}
ScopeKey(S) ⇔ dom(S) ⊆ {IdKey(x) | x ∈ Identifier}

Σ = ⟨Σ.Mods, Σ.Types, Σ.Classes⟩
Σ.Mods ∈ [ASTModule]
Σ.Types : Path ⇀ TypeDecl
Σ.Classes : Path ⇀ ClassDecl

Γ = ⟨P, Σ, m, S⟩
Project(Γ) = P
ResCtx(Γ) = ⟨Σ, m⟩
CurrentModule(Γ) = m
Scopes(Γ) = S

EntityKind = {Value, Type, Class, ModuleAlias}
EntitySource = {Decl, Using, RegionAlias}
Entity = ⟨kind, origin_opt, target_opt, source⟩
origin_opt ∈ ModulePath ∪ {⊥}    target_opt ∈ Identifier ∪ {⊥}

**Unified Namespace.**

S : IdKey ⇀ Entity

Scopes(Γ) = [S_1, …, S_k, S_proc, S_module, S_universe]    (k ≥ 0)

LocalScopes(Γ) = [S_1, …, S_k]
ProcScope(Γ) = S_proc
ModuleScope(Γ) = S_module
UniverseScope(Γ) = S_universe

UniverseProtectedRef = {"3.2.3"}

UniverseBindings = { IdKey(x) ↦ ⟨Type, ⊥, ⊥, Decl⟩ | x ∈ UniverseProtected } ∪ { IdKey(`cursive`) ↦ ⟨ModuleAlias, `cursive`, ⊥, Decl⟩ }
S_universe = UniverseBindings

BytePrefix(p, s) ⇔ ∃ r. s = p ++ r
Prefix(s, p) ⇔ BytePrefix(p, s)

ReservedGen(x) ⇔ Prefix(IdKey(x), IdKey(`gen_`))
ReservedCursive(x) ⇔ IdEq(x, `cursive`)
ReservedId(x) ⇔ ReservedGen(x) ∨ ReservedCursive(x)
ReservedModulePath(path) ⇔ (|path| ≥ 1 ∧ IdEq(path[0], `cursive`)) ∨ (∃ i. ReservedGen(path[i]))

<!-- Source: "The `cursive.*` namespace prefix is reserved for specification-defined features. User programs and vendor extensions MUST NOT use this namespace." -->

PrimTypeNames = {`i8`, `i16`, `i32`, `i64`, `i128`, `u8`, `u16`, `u32`, `u64`, `u128`, `f16`, `f32`, `f64`, `bool`, `char`, `usize`, `isize`}
SpecialTypeNames = {`Self`, `Drop`, `Bitcopy`, `Clone`, `string`, `bytes`, `Modal`, `Region`, `RegionOptions`, `Context`, `System`}
AsyncTypeNames = {`Async`, `Future`, `Sequence`, `Stream`, `Pipe`, `Exchange`}

PrimTypeKeys = {IdKey(x) | x ∈ PrimTypeNames}
SpecialTypeKeys = {IdKey(x) | x ∈ SpecialTypeNames}
AsyncTypeKeys = {IdKey(x) | x ∈ AsyncTypeNames}

KeywordKey(n) ⇔ ∃ s. n = IdKey(s) ∧ Keyword(s)

#### 5.1.2. Name Introduction and Shadowing

dom(S) = keys(S)
Scopes(Γ) = [S_cur] ++ Γ_out
InScope(S, x) ⇔ IdKey(x) ∈ dom(S)
InOuter(Γ, x) ⇔ ∃ S ∈ Γ_out. InScope(S, x)

**(Intro-Ok)**
¬ InScope(S_cur, x)    ¬ InOuter(Γ, x)    ¬ ReservedId(x)    (S_cur ≠ S_module ∨ x ∉ UniverseProtected)    Scopes(Γ') = [S_cur[IdKey(x) ↦ ent]] ++ Γ_out    Project(Γ') = Project(Γ)    ResCtx(Γ') = ResCtx(Γ)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Intro(x, ent) ⇓ Γ'

**(Intro-Dup)**
InScope(S_cur, x)
──────────────────────────────────────────────
Γ ⊢ Intro(x, ent) ⇑

**(Intro-Shadow-Required)**
¬ InScope(S_cur, x)    InOuter(Γ, x)    c = Code(Intro-Shadow-Required)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Intro(x, ent) ⇑ c

**(Shadow-Ok)**
¬ InScope(S_cur, x)    InOuter(Γ, x)    ¬ ReservedId(x)    (S_cur ≠ S_module ∨ x ∉ UniverseProtected)    Scopes(Γ') = [S_cur[IdKey(x) ↦ ent]] ++ Γ_out    Project(Γ') = Project(Γ)    ResCtx(Γ') = ResCtx(Γ)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ShadowIntro(x, ent) ⇓ Γ'

**(Shadow-Unnecessary)**
¬ InScope(S_cur, x)    ¬ InOuter(Γ, x)    c = Code(Shadow-Unnecessary)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ShadowIntro(x, ent) ⇑ c

**(Intro-Reserved-Gen-Err)**
ReservedGen(x)    c = Code(Intro-Reserved-Gen-Err)
──────────────────────────────────────────────────────
Γ ⊢ Intro(x, ent) ⇑ c

**(Intro-Reserved-Cursive-Err)**
ReservedCursive(x)    c = Code(Intro-Reserved-Cursive-Err)
──────────────────────────────────────────────────────────
Γ ⊢ Intro(x, ent) ⇑ c

**(Shadow-Reserved-Gen-Err)**
ReservedGen(x)    c = Code(Shadow-Reserved-Gen-Err)
────────────────────────────────────────────────────────
Γ ⊢ ShadowIntro(x, ent) ⇑ c

**(Shadow-Reserved-Cursive-Err)**
ReservedCursive(x)    c = Code(Shadow-Reserved-Cursive-Err)
────────────────────────────────────────────────────────────
Γ ⊢ ShadowIntro(x, ent) ⇑ c

**Deterministic Rule Priority (Intro / ShadowIntro).**

When multiple Intro/ShadowIntro rules are simultaneously applicable, an implementation MUST apply the first matching clause in the following ordered checks.

**Intro Priority.**
1. If ReservedGen(x) then apply **(Intro-Reserved-Gen-Err)**.
2. Else if ReservedCursive(x) then apply **(Intro-Reserved-Cursive-Err)**.
3. Else if InScope(S_cur, x) then apply **(Intro-Dup)**.
4. Else if InOuter(Γ, x) then apply **(Intro-Shadow-Required)**.
5. Else if the premises of **(Intro-Ok)** hold then apply **(Intro-Ok)**.
6. Otherwise, Γ ⊢ Intro(x, ent) ⇑ with no diagnostic code.

**ShadowIntro Priority.**
1. If ReservedGen(x) then apply **(Shadow-Reserved-Gen-Err)**.
2. Else if ReservedCursive(x) then apply **(Shadow-Reserved-Cursive-Err)**.
3. Else if InScope(S_cur, x) then Γ ⊢ ShadowIntro(x, ent) ⇑ with no diagnostic code.
4. Else if ¬ InOuter(Γ, x) then apply **(Shadow-Unnecessary)**.
5. Else if the premises of **(Shadow-Ok)** hold then apply **(Shadow-Ok)**.
6. Otherwise, Γ ⊢ ShadowIntro(x, ent) ⇑ with no diagnostic code.

**Binding Introduction Selection.**
If a binding is introduced by a syntactic form without the `shadow` keyword, the implementation MUST invoke Intro for that binding. If a binding is introduced by a `shadow` form (e.g., `ShadowLetStmt`/`ShadowVarStmt`), the implementation MUST invoke ShadowIntro for that binding. The implementation MUST NOT substitute the other judgment.

**Module-Scope Name Validation.**

Names(N) = dom(N)

**(Validate-Module-Ok)**
∀ n ∈ Names(N). ¬ KeywordKey(n) ∧ n ∉ PrimTypeKeys ∧ n ∉ SpecialTypeKeys ∧ n ∉ AsyncTypeKeys
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ValidateModuleNames(N) ⇓ ok

**(Validate-Module-Keyword-Err)**
∃ n ∈ Names(N). KeywordKey(n)    c = Code(Validate-Module-Keyword-Err)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ValidateModuleNames(N) ⇑ c

**(Validate-Module-Prim-Shadow-Err)**
∃ n ∈ Names(N). n ∈ PrimTypeKeys    c = Code(Validate-Module-Prim-Shadow-Err)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ValidateModuleNames(N) ⇑ c

**(Validate-Module-Special-Shadow-Err)**
∃ n ∈ Names(N). n ∈ SpecialTypeKeys    c = Code(Validate-Module-Special-Shadow-Err)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ValidateModuleNames(N) ⇑ c

**(Validate-Module-Async-Shadow-Err)**
∃ n ∈ Names(N). n ∈ AsyncTypeKeys    c = Code(Validate-Module-Async-Shadow-Err)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ValidateModuleNames(N) ⇑ c

#### 5.1.3. Lookup

**Unqualified Lookup.**

Scopes(Γ) = [S_1, …, S_n]
i = min{j | IdKey(x) ∈ dom(S_j)}

**(Lookup-Unqualified)**
i defined
──────────────────────────────────────────────
Γ ⊢ Lookup(x) ⇓ S_i[IdKey(x)]

**(Lookup-Unqualified-None)**
i undefined
──────────────────────────────────────────────
Γ ⊢ Lookup(x) ↑

**Kind Filtering.**
ValueKind(ent) ⇔ ent.kind = Value
TypeKind(ent) ⇔ ent.kind = Type
ClassKind(ent) ⇔ ent.kind = Class
ModuleKind(ent) ⇔ ent.kind = ModuleAlias
RegionAlias(ent) ⇔ ent.source = RegionAlias

RegionAliasName(Γ, x) ⇔ Γ ⊢ ResolveValueName(x) ⇓ ent ∧ RegionAlias(ent)

**(Resolve-Value-Name)**
Γ ⊢ Lookup(x) ⇓ ent    ValueKind(ent)
──────────────────────────────────────────────
Γ ⊢ ResolveValueName(x) ⇓ ent

**(Resolve-Type-Name)**
Γ ⊢ Lookup(x) ⇓ ent    TypeKind(ent)
──────────────────────────────────────────────
Γ ⊢ ResolveTypeName(x) ⇓ ent

**(Resolve-Class-Name)**
Γ ⊢ Lookup(x) ⇓ ent    ClassKind(ent)
──────────────────────────────────────────────
Γ ⊢ ResolveClassName(x) ⇓ ent

**(Resolve-Module-Name)**
Γ ⊢ Lookup(x) ⇓ ent    ModuleKind(ent)
──────────────────────────────────────────────
Γ ⊢ ResolveModuleName(x) ⇓ ent

**Qualified Lookup.**
P = Project(Γ)
m = CurrentModule(Γ)
ModulePaths = { p | p ∈ P.modules }
Alias = AliasMap(m)

**(Resolve-ModulePath)**
Γ ⊢ AliasExpand(path, Alias) ⇓ path'    StringOfPath(path') ∈ ModuleNames
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveModulePath(path, Alias, ModuleNames) ⇓ path'

**(ResolveModulePath-Err)**
Γ ⊢ AliasExpand(path, Alias) ⇓ path'    StringOfPath(path') ∉ ModuleNames    c = Code(ResolveModulePath-Err)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveModulePath(path, Alias, ModuleNames) ⇑ c

**(Resolve-Qualified)**
Γ ⊢ ResolveModulePath(path, Alias, ModuleNames) ⇓ mp    NameMap(P, mp)[IdKey(name)] = ent    Γ ⊢ CanAccess(m, DeclOf(mp, name)) ⇓ ok    K(ent)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveQualified(path, name, K) ⇓ ent

K ∈ {ValueKind, TypeKind, ClassKind, ModuleKind}

#### 5.1.4. Visibility and Accessibility

DeclOf(mp, name) = it ⇔ ModuleOf(it) = mp ∧ IdKey(name) ∈ dom(ItemBindings(it, mp))
ModuleOf(it) = p ⇔ it ∈ ASTModule(P, p).items
ModuleOfPath(path) = mp ⇔ SplitLast(path) = (mp, name)
Vis(it) = it.vis

**(Access-Public)**
Vis(it) = `public`
──────────────────────────────────────────────
Γ ⊢ CanAccess(m, it) ⇓ ok

**(Access-Internal)**
Vis(it) = `internal`
──────────────────────────────────────────────
Γ ⊢ CanAccess(m, it) ⇓ ok

**(Access-Private)**
Vis(it) = `private`    ModuleOf(it) = m
──────────────────────────────────────────────────────────────
Γ ⊢ CanAccess(m, it) ⇓ ok

**(Access-Protected)**
Vis(it) = `protected`    ModuleOf(it) = m
──────────────────────────────────────────────────────────────
Γ ⊢ CanAccess(m, it) ⇓ ok

**(Access-Err)**
Vis(it) ∈ {`private`, `protected`}    ModuleOf(it) ≠ m    c = Code(Access-Err)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ CanAccess(m, it) ⇑ c

**Top-Level `protected`.**

TopLevelDecl(it) ⇔ it ∈ ASTModule(P, ModuleOf(it)).items

**(Protected-TopLevel-Ok)**
Vis(it) ≠ `protected`
──────────────────────────────────────────────
Γ ⊢ TopLevelVis(it) ⇓ ok

**(Protected-TopLevel-Err)**
Vis(it) = `protected`    TopLevelDecl(it)    c = Code(Protected-TopLevel-Err)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TopLevelVis(it) ⇑ c

#### 5.1.5. Top-Level Name Collection (Order Independence)

**Order Independence.**
∀ items'. Permutation(items', items) ∧ Γ ⊢ CollectNames(items, p, ∅) ⇓ N ⇒ Γ ⊢ CollectNames(items', p, ∅) ⇓ N

**Binding Kinds.**

BindKind = {Value, Type, Class, ModuleAlias}
BindSource = {Decl, Using}
NameInfo = ⟨kind, origin, target_opt, source⟩
P = Project(Γ)
NameMap(P, mp) = N ⇔ ModuleMap(P, mp) = M ∧ Γ ⊢ CollectNames(M) ⇓ N
AliasMap(m) = { n ↦ origin | NameMap(P, m)[n].kind = ModuleAlias }
UsingMap(m) = { n ↦ ⟨k, origin, target_opt⟩ | NameMap(P, m)[n].source = Using ∧ NameMap(P, m)[n].kind = k ∧ k ∈ {Value, Type, Class} }
UsingValueMap(m) = { n ↦ origin | NameMap(P, m)[n].source = Using ∧ NameMap(P, m)[n].kind = Value }
UsingTypeMap(m) = { n ↦ origin | NameMap(P, m)[n].source = Using ∧ NameMap(P, m)[n].kind ∈ {Type, Class} }
TypeMap(m) = { n ↦ origin | NameMap(P, m)[n].kind = Type }
ClassMap(m) = { n ↦ origin | NameMap(P, m)[n].kind = Class }

**Pattern Name Extraction.**

──────────────────────────────────────────────
Γ ⊢ PatNames(IdentifierPattern(x)) ⇓ [x]

**(Pat-Typed)**
──────────────────────────────────────────────
Γ ⊢ PatNames(TypedPattern(x, _)) ⇓ [x]

**(Pat-Wild)**
──────────────────────────────────────────────
Γ ⊢ PatNames(WildcardPattern) ⇓ []

**(Pat-Lit)**
──────────────────────────────────────────────
Γ ⊢ PatNames(LiteralPattern(lit)) ⇓ []

∀ i, Γ ⊢ PatNames(p_i) ⇓ N_i
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ PatNames(TuplePattern([p_1, …, p_n])) ⇓ N_1 ++ ··· ++ N_n

**(Pat-Record-Field-Explicit)**
Γ ⊢ PatNames(p) ⇓ N
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ PatNames(⟨name, pattern_opt = p, span⟩) ⇓ N

**(Pat-Record-Field-Implicit)**
──────────────────────────────────────────────
Γ ⊢ PatNames(⟨name, pattern_opt = ⊥, span⟩) ⇓ [name]

∀ i, Γ ⊢ PatNames(f_i) ⇓ N_i
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ PatNames(RecordPattern(_, [f_1, …, f_n])) ⇓ N_1 ++ ··· ++ N_n

**(Pat-Enum-None)**
──────────────────────────────────────────────
Γ ⊢ PatNames(EnumPattern(_, _, ⊥)) ⇓ []

**(Pat-Enum-Tuple)**
∀ i, Γ ⊢ PatNames(p_i) ⇓ N_i
──────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ PatNames(EnumPattern(_, _, TuplePayloadPattern([p_1, …, p_n]))) ⇓ N_1 ++ ··· ++ N_n

**(Pat-Enum-Record)**
∀ i, Γ ⊢ PatNames(f_i) ⇓ N_i
──────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ PatNames(EnumPattern(_, _, RecordPayloadPattern([f_1, …, f_n]))) ⇓ N_1 ++ ··· ++ N_n

**(Pat-Range)**
Γ ⊢ PatNames(p_l) ⇓ N_l    Γ ⊢ PatNames(p_h) ⇓ N_h
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ PatNames(RangePattern(_, p_l, p_h)) ⇓ N_l ++ N_h

**Using Bindings.**
ModuleNames = { StringOfPath(p) | p ∈ ModulePaths }
Last([c_1, …, c_n]) = c_n if n ≥ 1
IsModulePath(path) ⇔ StringOfPath(path) ∈ ModuleNames
SplitLast(path) = (mp, name) ⇔ path = mp ++ [name] ∧ |path| ≥ 2
ModuleByPath(P, p) = m ⇔ ASTModule(P, p) = m

**Item Names.**
ItemNames(mp) = { n | NameMap(P, mp)[n].kind ∈ {Value, Type, Class} }

**Using Spec Names.**
UsingSpecName(⟨name, alias_opt⟩) = name
UsingSpecNames([s_1, …, s_n]) = [UsingSpecName(s_1), …, UsingSpecName(s_n)]

**Declared Names (Non-Using).**

**(DeclNames-Empty)**
──────────────────────────────────────────────
Γ ⊢ DeclNames([], p) ⇓ ∅

**(DeclNames-Using)**
Γ ⊢ DeclNames(rest, p) ⇓ D
────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ DeclNames(⟨UsingDecl, _, _, _, _⟩ :: rest, p) ⇓ D

**(DeclNames-Item)**
it ≠ ⟨UsingDecl, _, _, _, _⟩    Γ ⊢ ItemBindings(it, p) ⇓ B    Γ ⊢ DeclNames(rest, p) ⇓ D
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ DeclNames(it :: rest, p) ⇓ Names(B) ∪ D

DeclNames(m) = DeclNames(m.items, m.path)

**Item-Path Resolution.**

**(ItemOfPath)**
|path| ≥ 2    SplitLast(path) = (mp, name)    IsModulePath(mp)    m = ModuleByPath(P, mp)    IdKey(name) ∈ ItemNames(mp)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ItemOfPath(path) ⇓ (mp, name)

**(ItemOfPath-None)**
¬ (|path| ≥ 2 ∧ SplitLast(path) = (mp, name) ∧ IsModulePath(mp) ∧ m = ModuleByPath(P, mp) ∧ IdKey(name) ∈ ItemNames(mp))
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ItemOfPath(path) ↑

**Using Path Resolution.**

**(Resolve-Using-Item)**
Γ ⊢ ItemOfPath(path) ⇓ (mp, name)    ¬ IsModulePath(path)
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveUsingPath(path) ⇓ ⟨Item, mp, name⟩

**(Resolve-Using-Module)**
IsModulePath(path)    Γ ⊢ ItemOfPath(path) ↑
────────────────────────────────────────────────────────────────
Γ ⊢ ResolveUsingPath(path) ⇓ ⟨Module, path⟩

**(Resolve-Using-Ambig)**
IsModulePath(path)    Γ ⊢ ItemOfPath(path) ⇓ (mp, name)    c = Code(Resolve-Using-Ambig)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveUsingPath(path) ⇑ c

**(Resolve-Using-None)**
¬ IsModulePath(path)    Γ ⊢ ItemOfPath(path) ↑    c = Code(Resolve-Using-None)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveUsingPath(path) ⇑ c

**(Using-Path-Item)**
u = ⟨UsingDecl, vis, ⟨UsingPath, path, alias_opt⟩, _, _⟩    Γ ⊢ ResolveUsingPath(path) ⇓ ⟨Item, mp, item⟩    Γ ⊢ CanAccess(m, DeclOf(mp, item)) ⇓ ok    (vis = `public` ⇒ Vis(DeclOf(mp, item)) = `public`)    NameMap(P, mp)[IdKey(item)] = ⟨k, _, _, _⟩    k ∈ {Value, Type, Class}    name = alias_opt if present, else item
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ UsingNames(u) ⇓ [(name, ⟨k, mp, item, Using⟩)]

**(Using-Path-Item-Public-Err)**
u = ⟨UsingDecl, `public`, ⟨UsingPath, path, _⟩, _, _⟩    Γ ⊢ ResolveUsingPath(path) ⇓ ⟨Item, mp, item⟩    Vis(DeclOf(mp, item)) ≠ `public`    c = Code(Using-Path-Item-Public-Err)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ UsingNames(u) ⇑ c

**(Using-Path-Module)**
u = ⟨UsingDecl, _, ⟨UsingPath, path, alias_opt⟩, _, _⟩    Γ ⊢ ResolveUsingPath(path) ⇓ ⟨Module, path⟩    name = alias_opt if present, else Last(path)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ UsingNames(u) ⇓ [(name, ⟨ModuleAlias, path, ⊥, Using⟩)]

**(Using-List)**
u = ⟨UsingDecl, vis, ⟨UsingList, mp, [s_1, …, s_n]⟩, _, _⟩    Distinct(UsingSpecNames([s_1, …, s_n]))    ∀ i, s_i = ⟨name_i, alias_i⟩    NameMap(P, mp)[IdKey(name_i)] = ⟨k_i, _, _, _⟩    k_i ∈ {Value, Type, Class}    Γ ⊢ CanAccess(m, DeclOf(mp, name_i)) ⇓ ok    (vis = `public` ⇒ Vis(DeclOf(mp, name_i)) = `public`)    bind_i = ⟨(alias_i if present else name_i), ⟨k_i, mp, name_i, Using⟩⟩
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ UsingNames(u) ⇓ [bind_1, …, bind_n]

**(Using-List-Dup)**
u = ⟨UsingDecl, _, ⟨UsingList, _, specs⟩, _, _⟩    ¬ Distinct(UsingSpecNames(specs))    c = Code(Using-List-Dup)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ UsingNames(u) ⇑ c

**(Using-List-Public-Err)**
u = ⟨UsingDecl, `public`, ⟨UsingList, mp, [s_1, …, s_n]⟩, _, _⟩    ∃ i. s_i = ⟨name_i, _⟩ ∧ Vis(DeclOf(mp, name_i)) ≠ `public`    c = Code(Using-List-Public-Err)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ UsingNames(u) ⇑ c

**Item Bindings.**

**(Bind-Procedure)**
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ItemBindings(⟨ProcedureDecl, _, name, _, _, _, _, _⟩, p) ⇓ [(name, ⟨Value, p, ⊥, Decl⟩)]

**(Bind-Record)**
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ItemBindings(⟨RecordDecl, _, name, _, _, _, _⟩, p) ⇓ [(name, ⟨Type, p, ⊥, Decl⟩)]

**(Bind-Enum)**
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ItemBindings(⟨EnumDecl, _, name, _, _, _, _⟩, p) ⇓ [(name, ⟨Type, p, ⊥, Decl⟩)]

**(Bind-Class)**
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ItemBindings(⟨ClassDecl, _, name, _, _, _, _⟩, p) ⇓ [(name, ⟨Class, p, ⊥, Decl⟩)]

**(Bind-TypeAlias)**
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ItemBindings(⟨TypeAliasDecl, _, name, _, _, _⟩, p) ⇓ [(name, ⟨Type, p, ⊥, Decl⟩)]

**(Bind-Static)**
Γ ⊢ PatNames(pat) ⇓ N
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ItemBindings(⟨StaticDecl, _, _, ⟨pat, _, _, _, _⟩, _, _⟩, p) ⇓ [(n, ⟨Value, p, ⊥, Decl⟩) | n ∈ N]

**(Bind-Using)**
Γ ⊢ UsingNames(u) ⇓ B
──────────────────────────────────────────────
Γ ⊢ ItemBindings(u, p) ⇓ B

**(Bind-Using-Err)**
Γ ⊢ UsingNames(u) ⇑ c
──────────────────────────────────────────────
Γ ⊢ ItemBindings(u, p) ⇑ c

**(Bind-ErrorItem)**
──────────────────────────────────────────────────────────────
Γ ⊢ ItemBindings(ErrorItem(_), p) ⇓ []

**CollectNames (Big-Step).**

**(Collect-Ok)**
Γ ⊢ CollectNames(items, p, ∅) ⇓ N
──────────────────────────────────────────────
Γ ⊢ CollectNames(M) ⇓ N

**(Collect-Scan)**
Γ ⊢ ItemBindings(it, p) ⇓ B    DisjointNames(B, N)    NoDup(B)    Γ ⊢ CollectNames(rest, p, N ∪ B) ⇓ N'
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ CollectNames(it :: rest, p, N) ⇓ N'

**(Collect-Dup)**
Γ ⊢ ItemBindings(it, p) ⇓ B    (¬ DisjointNames(B, N) ∨ ¬ NoDup(B))    c = Code(Collect-Dup)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ CollectNames(it :: rest, p, N) ⇑ c

**(Collect-Err)**
Γ ⊢ ItemBindings(it, p) ⇑ c
────────────────────────────────────────────────────────────────
Γ ⊢ CollectNames(it :: rest, p, N) ⇑ c

Names(B) = { n | (n, _) ∈ B }
NoDup(B) ⇔ Distinct(Names(B))
DisjointNames(B, N) ⇔ Names(B) ∩ dom(N) = ∅
N ∪ B = { (n, v) | (n, v) ∈ N ∨ (n, v) ∈ B }

**CollectNames (Small-Step).**
NamesState = {NamesStart(M), NamesScan(items, p, N), NamesDone(N), Error(code)}

**(Names-Start)**
────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨NamesStart(M)⟩ → ⟨NamesScan(M.items, M.path, ∅)⟩

**(Names-Step)**
Γ ⊢ ItemBindings(it, p) ⇓ B    DisjointNames(B, N)    NoDup(B)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨NamesScan(it :: rest, p, N)⟩ → ⟨NamesScan(rest, p, N ∪ B)⟩

**(Names-Step-Dup)**
Γ ⊢ ItemBindings(it, p) ⇓ B    (¬ DisjointNames(B, N) ∨ ¬ NoDup(B))
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨NamesScan(it :: rest, p, N)⟩ → ⟨Error(Code(Names-Step-Dup))⟩

**(Names-Step-Err)**
Γ ⊢ ItemBindings(it, p) ⇑ c
────────────────────────────────────────────────────────────────
⟨NamesScan(it :: rest, p, N)⟩ → ⟨Error(c)⟩

**(Names-Done)**
──────────────────────────────────────────────────────────────
⟨NamesScan([], p, N)⟩ → ⟨NamesDone(N)⟩


#### 5.1.6. Qualified Disambiguation (Resolution-Time)

ResolveQualifiedForm : Expr ⇀ Expr

ResolveArgs : [Arg] ⇀ [Arg]
ResolveFieldInits : [FieldInit] ⇀ [FieldInit]
ResolveRecordPath : Path × Identifier ⇀ Path
ResolveEnumUnit : Path × Identifier ⇀ Path
ResolveEnumTuple : Path × Identifier ⇀ Path
ResolveEnumRecord : Path × Identifier ⇀ Path

**(ResolveArgs-Empty)**
──────────────────────────────────────────────
Γ ⊢ ResolveArgs([]) ⇓ []

**(ResolveArgs-Cons)**
Γ ⊢ ResolveExpr(e) ⇓ e'    Γ ⊢ ResolveArgs(rest) ⇓ rest'
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveArgs([⟨moved, e, span⟩] ++ rest) ⇓ [⟨moved, e', span⟩] ++ rest'

**(ResolveFieldInits-Empty)**
──────────────────────────────────────────────
Γ ⊢ ResolveFieldInits([]) ⇓ []

**(ResolveFieldInits-Cons)**
Γ ⊢ ResolveExpr(e) ⇓ e'    Γ ⊢ ResolveFieldInits(rest) ⇓ rest'
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveFieldInits([⟨f, e⟩] ++ rest) ⇓ [⟨f, e'⟩] ++ rest'

ResolvePathJudg = {ResolveRecordPath, ResolveEnumUnit, ResolveEnumTuple, ResolveEnumRecord}

**(Resolve-RecordPath)**
Γ ⊢ ResolveTypePath(path ++ [name]) ⇓ p    RecordDecl(p) = R
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveRecordPath(path, name) ⇓ p

**(Resolve-EnumUnit)**
Γ ⊢ ResolveTypePath(path) ⇓ p    EnumDecl(p) = E    VariantPayload(E, name) = ⊥
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveEnumUnit(path, name) ⇓ p

**(Resolve-EnumTuple)**
Γ ⊢ ResolveTypePath(path) ⇓ p    EnumDecl(p) = E    VariantPayload(E, name) = TuplePayload(_)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveEnumTuple(path, name) ⇓ p

**(Resolve-EnumRecord)**
Γ ⊢ ResolveTypePath(path) ⇓ p    EnumDecl(p) = E    VariantPayload(E, name) = RecordPayload(_)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveEnumRecord(path, name) ⇓ p

**(ResolveQual-Name-Value)**
Γ ⊢ ResolveQualified(path, name, ValueKind) ⇓ ent    ent.origin_opt = mp    name' = (ent.target_opt if present, else name)    PathOfModule(mp) = path'
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveQualifiedForm(QualifiedName(path, name)) ⇓ Path(path', name')

**(ResolveQual-Name-Record)**
Γ ⊢ ResolveRecordPath(path, name) ⇓ p    SplitLast(p) = (mp, name')
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveQualifiedForm(QualifiedName(path, name)) ⇓ Path(mp, name')

**(ResolveQual-Name-Enum)**
Γ ⊢ ResolveQualified(path, name, ValueKind) ↑    Γ ⊢ ResolveRecordPath(path, name) ↑    Γ ⊢ ResolveEnumUnit(path, name) ⇓ p
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveQualifiedForm(QualifiedName(path, name)) ⇓ EnumLiteral(FullPath(p, name), ⊥)

**(ResolveQual-Name-Err)**
Γ ⊢ ResolveQualified(path, name, ValueKind) ↑    Γ ⊢ ResolveRecordPath(path, name) ↑    Γ ⊢ ResolveEnumUnit(path, name) ↑    c = Code(ResolveExpr-Ident-Err)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveQualifiedForm(QualifiedName(path, name)) ⇑ c

**(ResolveQual-Apply-Value)**
Γ ⊢ ResolveArgs(args) ⇓ args'    Γ ⊢ ResolveQualified(path, name, ValueKind) ⇓ ent    ent.origin_opt = mp    name' = (ent.target_opt if present, else name)    PathOfModule(mp) = path'
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveQualifiedForm(QualifiedApply(path, name, Paren(args))) ⇓ Call(Path(path', name'), args')

**(ResolveQual-Apply-Record)**
Γ ⊢ ResolveArgs(args) ⇓ args'    Γ ⊢ ResolveRecordPath(path, name) ⇓ p    SplitLast(p) = (mp, name')
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveQualifiedForm(QualifiedApply(path, name, Paren(args))) ⇓ Call(Path(mp, name'), args')

**(ResolveQual-Apply-Enum-Tuple)**
Γ ⊢ ResolveArgs(args) ⇓ args'    Γ ⊢ ResolveQualified(path, name, ValueKind) ↑    Γ ⊢ ResolveRecordPath(path, name) ↑    Γ ⊢ ResolveEnumTuple(path, name) ⇓ p
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveQualifiedForm(QualifiedApply(path, name, Paren(args))) ⇓ EnumLiteral(FullPath(p, name), Paren(ArgsExprs(args')))

**(ResolveQual-Apply-Err)**
Γ ⊢ ResolveArgs(args) ⇓ args'    Γ ⊢ ResolveQualified(path, name, ValueKind) ↑    Γ ⊢ ResolveRecordPath(path, name) ↑    Γ ⊢ ResolveEnumTuple(path, name) ↑    c = Code(ResolveExpr-Ident-Err)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveQualifiedForm(QualifiedApply(path, name, Paren(args))) ⇑ c

**Qualified Apply (Brace).**

**(ResolveQual-Apply-RecordLit)**
Γ ⊢ ResolveFieldInits(fields) ⇓ fields'    Γ ⊢ ResolveRecordPath(path, name) ⇓ p
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveQualifiedForm(QualifiedApply(path, name, Brace(fields))) ⇓ RecordExpr(TypePath(p), fields')

**(ResolveQual-Apply-Enum-Record)**
Γ ⊢ ResolveFieldInits(fields) ⇓ fields'    Γ ⊢ ResolveRecordPath(path, name) ↑    Γ ⊢ ResolveEnumRecord(path, name) ⇓ p
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveQualifiedForm(QualifiedApply(path, name, Brace(fields))) ⇓ EnumLiteral(FullPath(p, name), Brace(fields'))

**(ResolveQual-Apply-Brace-Err)**
Γ ⊢ ResolveFieldInits(fields) ⇓ fields'    Γ ⊢ ResolveRecordPath(path, name) ↑    Γ ⊢ ResolveEnumRecord(path, name) ↑    c = Code(ResolveExpr-Ident-Err)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveQualifiedForm(QualifiedApply(path, name, Brace(fields))) ⇑ c

#### 5.1.7. Resolution Pass (Big-Step and Small-Step)

P = Project(Γ)
m = CurrentModule(Γ)
M = ASTModule(P, m)
ResolveInputs = ⟨M, ModulePaths, { NameMap(P, p) | p ∈ ModulePaths }⟩
ResolveOutputs = ⟨M'⟩
PathOfModuleRef = {"3.4.1"}

**(Validate-ModulePath-Ok)**
¬ ReservedModulePath(PathOfModule(p))
──────────────────────────────────────────────
Γ ⊢ ValidateModulePath(p) ⇓ ok

**(Validate-ModulePath-Reserved-Err)**
ReservedModulePath(PathOfModule(p))    c = Code(Validate-ModulePath-Reserved-Err)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ValidateModulePath(p) ⇑ c

**(ResolveModule-Ok)**
Γ ⊢ CollectNames(M) ⇓ N    Γ ⊢ ValidateModulePath(M.path) ⇓ ok    Γ ⊢ ValidateModuleNames(N) ⇓ ok    S_module = N    Γ_N = [S_module, S_universe]    Γ_N ⊢ ResolveItems(M.items) ⇓ items'
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveModule(M) ⇓ ⟨M.path, items', M.module_doc⟩

**(ResolveItems-Empty)**
──────────────────────────────────────────────
Γ ⊢ ResolveItems([]) ⇓ []

**(ResolveItems-Cons)**
Γ ⊢ TopLevelVis(it) ⇓ ok    Γ ⊢ ResolveItem(it) ⇓ it'    Γ ⊢ ResolveItems(rest) ⇓ rest'
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveItems(it :: rest) ⇓ it' :: rest'

**(ResolveItem-Static)**
Γ ⊢ ResolvePattern(pat) ⇓ pat'    Γ ⊢ ResolveExpr(init) ⇓ init'    Γ ⊢ ResolveTypeOpt(ty_opt) ⇓ ty_opt'
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveItem(StaticDecl(vis, mut, ⟨pat, ty_opt, op, init, span⟩, span', doc)) ⇓ StaticDecl(vis, mut, ⟨pat', ty_opt', op, init', span⟩, span', doc)

**(ResolveItem-Procedure)**
S_proc = { IdKey(p.name) ↦ ⟨Value, ⊥, ⊥, Decl⟩ | p ∈ params }    Γ_p = [S_proc, S_module, S_universe]    Γ_p ⊢ ResolveParams(params) ⇓ params'    Γ_p ⊢ ResolveTypeOpt(ret_opt) ⇓ ret_opt'    Γ_p ⊢ ResolveExpr(body) ⇓ body'
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveItem(ProcedureDecl(vis, name, params, ret_opt, body, span, doc)) ⇓ ProcedureDecl(vis, name, params', ret_opt', body', span, doc)

**(ResolveItem-Using)**
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveItem(UsingDecl(vis, clause, span, doc)) ⇓ UsingDecl(vis, clause, span, doc)

**(ResolveItem-TypeAlias)**
Γ ⊢ ResolveType(ty) ⇓ ty'
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveItem(TypeAliasDecl(vis, name, ty, span, doc)) ⇓ TypeAliasDecl(vis, name, ty', span, doc)

**(ResolveItem-Record)**
R = RecordDecl(vis, name, impls, members, span, doc)    Γ ⊢ ResolveClassPathList(impls) ⇓ impls'    Γ ⊢ ResolveRecordMemberList(R, members) ⇓ members'
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveItem(R) ⇓ RecordDecl(vis, name, impls', members', span, doc)

**(ResolveItem-Enum)**
Γ ⊢ ResolveClassPathList(impls) ⇓ impls'    Γ ⊢ ResolveVariantList(vars) ⇓ vars'
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveItem(EnumDecl(vis, name, impls, vars, span, doc)) ⇓ EnumDecl(vis, name, impls', vars', span, doc)

**(ResolveItem-Modal)**
Γ ⊢ ResolveClassPathList(impls) ⇓ impls'    Γ ⊢ ResolveStateBlockList(states) ⇓ states'
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveItem(ModalDecl(vis, name, impls, states, span, doc)) ⇓ ModalDecl(vis, name, impls', states', span, doc)

**(ResolveItem-Class)**
Γ ⊢ ResolveClassPathList(supers) ⇓ supers'    Γ ⊢ ResolveClassItemList(items) ⇓ items'
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveItem(ClassDecl(vis, name, supers, items, span, doc)) ⇓ ClassDecl(vis, name, supers', items', span, doc)

**Self Binding for Methods.**

**(BindSelf-Record)**
RecordPath(R) = p    SplitLast(p) = (mp, name)    S_proc' = S_proc[IdKey(`Self`) ↦ ⟨Type, mp, name, Decl⟩]
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BindSelfRecord(R, S_proc) ⇓ S_proc'

**(BindSelf-Class)**
S_proc' = S_proc[IdKey(`Self`) ↦ ⟨Type, ⊥, ⊥, Decl⟩]
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BindSelfClass(S_proc) ⇓ S_proc'

**(ResolveReceiver-Shorthand)**
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveReceiver(ReceiverShorthand(perm)) ⇓ ReceiverShorthand(perm)

**(ResolveReceiver-Explicit)**
Γ ⊢ ResolveType(ty) ⇓ ty'
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveReceiver(ReceiverExplicit(mode_opt, ty)) ⇓ ReceiverExplicit(mode_opt, ty')

**(ResolveClassPathList-Empty)**
──────────────────────────────────────────────
Γ ⊢ ResolveClassPathList([]) ⇓ []

**(ResolveClassPathList-Cons)**
Γ ⊢ ResolveClassPath(p) ⇓ p'    Γ ⊢ ResolveClassPathList(ps) ⇓ ps'
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveClassPathList(p :: ps) ⇓ p' :: ps'

**(ResolveTypeList-Empty)**
──────────────────────────────────────────────
Γ ⊢ ResolveTypeList([]) ⇓ []

**(ResolveTypeList-Cons)**
Γ ⊢ ResolveType(t) ⇓ t'    Γ ⊢ ResolveTypeList(ts) ⇓ ts'
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveTypeList(t :: ts) ⇓ t' :: ts'

**(ResolveFieldDecl)**
Γ ⊢ ResolveType(ty) ⇓ ty'    Γ ⊢ ResolveExprOpt(init_opt) ⇓ init_opt'
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveFieldDecl(FieldDecl(vis, name, ty, init_opt, span, doc_opt)) ⇓ FieldDecl(vis, name, ty', init_opt', span, doc_opt)

**(ResolveFieldDeclList-Empty)**
──────────────────────────────────────────────
Γ ⊢ ResolveFieldDeclList([]) ⇓ []

**(ResolveFieldDeclList-Cons)**
Γ ⊢ ResolveFieldDecl(f) ⇓ f'    Γ ⊢ ResolveFieldDeclList(fs) ⇓ fs'
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveFieldDeclList(f :: fs) ⇓ f' :: fs'

**(ResolveRecordMember-Field)**
Γ ⊢ ResolveFieldDecl(f) ⇓ f'
──────────────────────────────────────────────
Γ ⊢ ResolveRecordMember(R, f) ⇓ f'

**(ResolveRecordMember-Method)**
S_proc = { IdKey(p.name) ↦ ⟨Value, ⊥, ⊥, Decl⟩ | p ∈ params }    Γ ⊢ BindSelfRecord(R, S_proc) ⇓ S_proc'    Γ_m = [S_proc', S_module, S_universe]    Γ_m ⊢ ResolveReceiver(recv) ⇓ recv'    Γ_m ⊢ ResolveParams(params) ⇓ params'    Γ_m ⊢ ResolveTypeOpt(ret_opt) ⇓ ret_opt'    Γ_m ⊢ ResolveExpr(body) ⇓ body'
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveRecordMember(R, MethodDecl(vis, override, name, recv, params, ret_opt, body, span, doc_opt)) ⇓ MethodDecl(vis, override, name, recv', params', ret_opt', body', span, doc_opt)

**(ResolveRecordMemberList-Empty)**
──────────────────────────────────────────────
Γ ⊢ ResolveRecordMemberList(R, []) ⇓ []

**(ResolveRecordMemberList-Cons)**
Γ ⊢ ResolveRecordMember(R, m) ⇓ m'    Γ ⊢ ResolveRecordMemberList(R, ms) ⇓ ms'
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveRecordMemberList(R, m :: ms) ⇓ m' :: ms'

**(ResolveClassItem-Field)**
Γ ⊢ ResolveType(ty) ⇓ ty'
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveClassItem(ClassFieldDecl(vis, name, ty, span, doc_opt)) ⇓ ClassFieldDecl(vis, name, ty', span, doc_opt)

**(ResolveClassItem-Method-Abstract)**
S_proc = { IdKey(p.name) ↦ ⟨Value, ⊥, ⊥, Decl⟩ | p ∈ params }    Γ ⊢ BindSelfClass(S_proc) ⇓ S_proc'    Γ_m = [S_proc', S_module, S_universe]    Γ_m ⊢ ResolveReceiver(recv) ⇓ recv'    Γ_m ⊢ ResolveParams(params) ⇓ params'    Γ_m ⊢ ResolveTypeOpt(ret_opt) ⇓ ret_opt'
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveClassItem(ClassMethodDecl(vis, name, recv, params, ret_opt, ⊥, span, doc_opt)) ⇓ ClassMethodDecl(vis, name, recv', params', ret_opt', ⊥, span, doc_opt)

**(ResolveClassItem-Method-Concrete)**
S_proc = { IdKey(p.name) ↦ ⟨Value, ⊥, ⊥, Decl⟩ | p ∈ params }    Γ ⊢ BindSelfClass(S_proc) ⇓ S_proc'    Γ_m = [S_proc', S_module, S_universe]    Γ_m ⊢ ResolveReceiver(recv) ⇓ recv'    Γ_m ⊢ ResolveParams(params) ⇓ params'    Γ_m ⊢ ResolveTypeOpt(ret_opt) ⇓ ret_opt'    Γ_m ⊢ ResolveExpr(body) ⇓ body'
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveClassItem(ClassMethodDecl(vis, name, recv, params, ret_opt, body, span, doc_opt)) ⇓ ClassMethodDecl(vis, name, recv', params', ret_opt', body', span, doc_opt)

**(ResolveClassItemList-Empty)**
──────────────────────────────────────────────
Γ ⊢ ResolveClassItemList([]) ⇓ []

**(ResolveClassItemList-Cons)**
Γ ⊢ ResolveClassItem(it) ⇓ it'    Γ ⊢ ResolveClassItemList(its) ⇓ its'
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveClassItemList(it :: its) ⇓ it' :: its'

**(ResolveVariantPayloadOpt-None)**
──────────────────────────────────────────────
Γ ⊢ ResolveVariantPayloadOpt(⊥) ⇓ ⊥

**(ResolveVariantPayloadOpt-Tuple)**
Γ ⊢ ResolveTypeList(ts) ⇓ ts'
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveVariantPayloadOpt(TuplePayload(ts)) ⇓ TuplePayload(ts')

**(ResolveVariantPayloadOpt-Record)**
Γ ⊢ ResolveFieldDeclList(fs) ⇓ fs'
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveVariantPayloadOpt(RecordPayload(fs)) ⇓ RecordPayload(fs')

**(ResolveVariant)**
Γ ⊢ ResolveVariantPayloadOpt(payload_opt) ⇓ payload_opt'    Γ ⊢ ResolveExprOpt(disc_opt) ⇓ disc_opt'
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveVariant(VariantDecl(name, payload_opt, disc_opt, span, doc_opt)) ⇓ VariantDecl(name, payload_opt', disc_opt', span, doc_opt)

**(ResolveVariantList-Empty)**
──────────────────────────────────────────────
Γ ⊢ ResolveVariantList([]) ⇓ []

**(ResolveVariantList-Cons)**
Γ ⊢ ResolveVariant(v) ⇓ v'    Γ ⊢ ResolveVariantList(vs) ⇓ vs'
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveVariantList(v :: vs) ⇓ v' :: vs'

**(ResolveStateMember-Field)**
Γ ⊢ ResolveType(ty) ⇓ ty'
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveStateMember(StateFieldDecl(vis, name, ty, span, doc_opt)) ⇓ StateFieldDecl(vis, name, ty', span, doc_opt)

**(ResolveStateMember-Method)**
S_proc = { IdKey(p.name) ↦ ⟨Value, ⊥, ⊥, Decl⟩ | p ∈ params }    Γ_m = [S_proc, S_module, S_universe]    Γ_m ⊢ ResolveParams(params) ⇓ params'    Γ_m ⊢ ResolveTypeOpt(ret_opt) ⇓ ret_opt'    Γ_m ⊢ ResolveExpr(body) ⇓ body'
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveStateMember(StateMethodDecl(vis, name, params, ret_opt, body, span, doc_opt)) ⇓ StateMethodDecl(vis, name, params', ret_opt', body', span, doc_opt)

**(ResolveStateMember-Transition)**
S_proc = { IdKey(p.name) ↦ ⟨Value, ⊥, ⊥, Decl⟩ | p ∈ params }    Γ_m = [S_proc, S_module, S_universe]    Γ_m ⊢ ResolveParams(params) ⇓ params'    Γ_m ⊢ ResolveExpr(body) ⇓ body'
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveStateMember(TransitionDecl(vis, name, params, target_state, body, span, doc_opt)) ⇓ TransitionDecl(vis, name, params', target_state, body', span, doc_opt)

**(ResolveStateMemberList-Empty)**
──────────────────────────────────────────────
Γ ⊢ ResolveStateMemberList([]) ⇓ []

**(ResolveStateMemberList-Cons)**
Γ ⊢ ResolveStateMember(m) ⇓ m'    Γ ⊢ ResolveStateMemberList(ms) ⇓ ms'
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveStateMemberList(m :: ms) ⇓ m' :: ms'

**(ResolveStateBlock)**
Γ ⊢ ResolveStateMemberList(members) ⇓ members'
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveStateBlock(StateBlock(name, members, span, doc_opt)) ⇓ StateBlock(name, members', span, doc_opt)

**(ResolveStateBlockList-Empty)**
──────────────────────────────────────────────
Γ ⊢ ResolveStateBlockList([]) ⇓ []

**(ResolveStateBlockList-Cons)**
Γ ⊢ ResolveStateBlock(b) ⇓ b'    Γ ⊢ ResolveStateBlockList(bs) ⇓ bs'
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveStateBlockList(b :: bs) ⇓ b' :: bs'

**(ResolveTypeOpt-None)**
──────────────────────────────────────────────
Γ ⊢ ResolveTypeOpt(⊥) ⇓ ⊥

**(ResolveTypeOpt-Some)**
Γ ⊢ ResolveType(ty) ⇓ ty'
──────────────────────────────────────────────
Γ ⊢ ResolveTypeOpt(ty) ⇓ ty'

**(ResolveExprOpt-None)**
──────────────────────────────────────────────
Γ ⊢ ResolveExprOpt(⊥) ⇓ ⊥

**(ResolveExprOpt-Some)**
Γ ⊢ ResolveExpr(e) ⇓ e'
──────────────────────────────────────────────
Γ ⊢ ResolveExprOpt(e) ⇓ e'

**(ResolveTypePath-Ident)**
|path| = 1    Γ ⊢ ResolveTypeName(path[0]) ⇓ ent    ent.origin_opt = p    name = (ent.target_opt if present, else path[0])
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveTypePath(path) ⇓ FullPath(PathOfModule(p), name)

**(ResolveTypePath-Ident-Local)**
|path| = 1    Γ ⊢ ResolveTypeName(path[0]) ⇓ ent    ent.origin_opt = ⊥    name = (ent.target_opt if present, else path[0])
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveTypePath(path) ⇓ [name]

**(ResolveTypePath-Qual)**
|path| ≥ 2    path = p ++ [name]    Γ ⊢ ResolveQualified(p, name, TypeKind) ⇓ ent    ent.origin_opt = mp    name' = (ent.target_opt if present, else name)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveTypePath(path) ⇓ FullPath(PathOfModule(mp), name')

LocalTypePath(path) ⇔ |path| = 1 ∧ Γ ⊢ ResolveTypeName(path[0]) ⇓ ent ∧ ent.origin_opt = ⊥

**(ResolveClassPath-Ident)**
|path| = 1    Γ ⊢ ResolveClassName(path[0]) ⇓ ent    ent.origin_opt = p    name = (ent.target_opt if present, else path[0])
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveClassPath(path) ⇓ FullPath(PathOfModule(p), name)

**(ResolveClassPath-Qual)**
|path| ≥ 2    path = p ++ [name]    Γ ⊢ ResolveQualified(p, name, ClassKind) ⇓ ent    ent.origin_opt = mp    name' = (ent.target_opt if present, else name)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveClassPath(path) ⇓ FullPath(PathOfModule(mp), name')

**(ResolveType-Path)**
Γ ⊢ ResolveTypePath(path) ⇓ path'
──────────────────────────────────────────────
Γ ⊢ ResolveType(TypePath(path)) ⇓ TypePath(path')

**(ResolveType-Dynamic)**
Γ ⊢ ResolveClassPath(path) ⇓ path'
──────────────────────────────────────────────
Γ ⊢ ResolveType(TypeDynamic(path)) ⇓ TypeDynamic(path')

**(ResolveType-ModalState)**
Γ ⊢ ResolveTypePath(path) ⇓ path'
──────────────────────────────────────────────────────────────
Γ ⊢ ResolveType(TypeModalState(path, state)) ⇓ TypeModalState(path', state)

**(ResolveTypeRef-Path)**
Γ ⊢ ResolveTypePath(path) ⇓ path'
──────────────────────────────────────────────
Γ ⊢ ResolveTypeRef(TypePath(path)) ⇓ TypePath(path')

**(ResolveTypeRef-ModalState)**
Γ ⊢ ResolveTypePath(path) ⇓ path'
──────────────────────────────────────────────────────────────
Γ ⊢ ResolveTypeRef(ModalStateRef(path, state)) ⇓ ModalStateRef(path', state)

**(ResolveType-Hom)**
∀ i, Γ ⊢ ResolveType(t_i) ⇓ t_i'
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveType(C(t_1, …, t_n)) ⇓ C(t_1', …, t_n')

**(ResolveParam)**
Γ ⊢ ResolveType(p.type) ⇓ ty'
──────────────────────────────────────────────
Γ ⊢ ResolveParam(p) ⇓ p[type = ty']

**(ResolveParams-Empty)**
──────────────────────────────────────────────
Γ ⊢ ResolveParams([]) ⇓ []

**(ResolveParams-Cons)**
Γ ⊢ ResolveParam(p) ⇓ p'    Γ ⊢ ResolveParams(ps) ⇓ ps'
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveParams(p :: ps) ⇓ p' :: ps'

ResolvePattern : Pattern ⇀ Pattern

**(ResolvePat-Wild)**
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolvePattern(WildcardPattern) ⇓ WildcardPattern

**(ResolvePat-Ident)**
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolvePattern(IdentifierPattern(x)) ⇓ IdentifierPattern(x)

**(ResolvePat-Literal)**
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolvePattern(LiteralPattern(lit)) ⇓ LiteralPattern(lit)

**(ResolvePat-Typed)**
Γ ⊢ ResolveType(ty) ⇓ ty'
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolvePattern(TypedPattern(x, ty)) ⇓ TypedPattern(x, ty')

**(ResolvePat-Tuple)**
Γ ⊢ ResolvePatternList(ps) ⇓ ps'
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolvePattern(TuplePattern(ps)) ⇓ TuplePattern(ps')

**(ResolvePat-Record)**
Γ ⊢ ResolveTypePath(tp) ⇓ tp'    Γ ⊢ ResolveFieldPatternList(fs) ⇓ fs'
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolvePattern(RecordPattern(tp, fs)) ⇓ RecordPattern(tp', fs')

**(ResolvePat-Enum-Record-Fallback)**
Γ ⊢ ResolveTypePath(tp) ⇓ tp_e    EnumDecl(tp_e) ↑    Γ ⊢ ResolveTypePath(tp ++ [name]) ⇓ tp_r    Γ ⊢ ResolveFieldPatternList(fs) ⇓ fs'
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolvePattern(EnumPattern(tp, name, RecordPayloadPattern(fs))) ⇓ RecordPattern(tp_r, fs')

**(ResolvePat-Enum)**
Γ ⊢ ResolveTypePath(tp) ⇓ tp'    Γ ⊢ ResolveEnumPayloadPattern(payload_opt) ⇓ payload_opt'
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolvePattern(EnumPattern(tp, name, payload_opt)) ⇓ EnumPattern(tp', name, payload_opt')

**(ResolvePat-Modal)**
Γ ⊢ ResolveFieldPatternListOpt(fields_opt) ⇓ fields_opt'
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolvePattern(ModalPattern(state, fields_opt)) ⇓ ModalPattern(state, fields_opt')

**(ResolvePat-Range)**
Γ ⊢ ResolvePattern(p_l) ⇓ p_l'    Γ ⊢ ResolvePattern(p_h) ⇓ p_h'
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolvePattern(RangePattern(kind, p_l, p_h)) ⇓ RangePattern(kind, p_l', p_h')

**(ResolvePatternList-Empty)**
──────────────────────────────────────────────
Γ ⊢ ResolvePatternList([]) ⇓ []

**(ResolvePatternList-Cons)**
Γ ⊢ ResolvePattern(p) ⇓ p'    Γ ⊢ ResolvePatternList(ps) ⇓ ps'
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolvePatternList(p :: ps) ⇓ p' :: ps'

**(ResolveFieldPattern-Implicit)**
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveFieldPattern(⟨name, ⊥, span⟩) ⇓ ⟨name, ⊥, span⟩

**(ResolveFieldPattern-Explicit)**
Γ ⊢ ResolvePattern(p) ⇓ p'
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveFieldPattern(⟨name, p, span⟩) ⇓ ⟨name, p', span⟩

**(ResolveFieldPatternList-Empty)**
──────────────────────────────────────────────
Γ ⊢ ResolveFieldPatternList([]) ⇓ []

**(ResolveFieldPatternList-Cons)**
Γ ⊢ ResolveFieldPattern(f) ⇓ f'    Γ ⊢ ResolveFieldPatternList(fs) ⇓ fs'
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveFieldPatternList(f :: fs) ⇓ f' :: fs'

**(ResolveEnumPayloadPattern-None)**
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveEnumPayloadPattern(⊥) ⇓ ⊥

**(ResolveEnumPayloadPattern-Tuple)**
Γ ⊢ ResolvePatternList(ps) ⇓ ps'
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveEnumPayloadPattern(TuplePayloadPattern(ps)) ⇓ TuplePayloadPattern(ps')

**(ResolveEnumPayloadPattern-Record)**
Γ ⊢ ResolveFieldPatternList(fs) ⇓ fs'
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveEnumPayloadPattern(RecordPayloadPattern(fs)) ⇓ RecordPayloadPattern(fs')

**(ResolveFieldPatternListOpt-None)**
────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveFieldPatternListOpt(⊥) ⇓ ⊥

**(ResolveFieldPatternListOpt-Some)**
Γ ⊢ ResolveFieldPatternList(fs) ⇓ fs'
────────────────────────────────────────────────────────────────
Γ ⊢ ResolveFieldPatternListOpt(fs) ⇓ fs'

**(ResolveExpr-Ident)**
Γ ⊢ ResolveValueName(x) ⇓ ent
────────────────────────────────────────────────────────────────
Γ ⊢ ResolveExpr(Identifier(x)) ⇓ Identifier(x)

**(ResolveExpr-Ident-Err)**
Γ ⊢ ResolveValueName(x) ⇑    c = Code(ResolveExpr-Ident-Err)
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveExpr(Identifier(x)) ⇑ c

**(ResolveExpr-Qualified)**
Γ ⊢ ResolveQualifiedForm(e) ⇓ e'
──────────────────────────────────────────────
Γ ⊢ ResolveExpr(e) ⇓ e'

ResolveArgsRef = {"5.1.6"}
ResolveFieldInitsRef = {"5.1.6"}

ResolveExprListJudg = {ResolveExprList}

**(ResolveExprList-Empty)**
──────────────────────────────────────────────
Γ ⊢ ResolveExprList([]) ⇓ []

**(ResolveExprList-Cons)**
Γ ⊢ ResolveExpr(e) ⇓ e'    Γ ⊢ ResolveExprList(es) ⇓ es'
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveExprList(e :: es) ⇓ e' :: es'

ResolveEnumPayloadJudg = {ResolveEnumPayload}

**(ResolveEnumPayload-None)**
────────────────────────────────────────────────────────────────
Γ ⊢ ResolveEnumPayload(⊥) ⇓ ⊥

**(ResolveEnumPayload-Tuple)**
Γ ⊢ ResolveExprList(es) ⇓ es'
────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveEnumPayload(Paren(es)) ⇓ Paren(es')

**(ResolveEnumPayload-Record)**
Γ ⊢ ResolveFieldInits(fields) ⇓ fields'
────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveEnumPayload(Brace(fields)) ⇓ Brace(fields')

ResolveCalleeJudg = {ResolveCallee}

**(ResolveCallee-Ident-Value)**
Γ ⊢ ResolveValueName(x) ⇓ ent
────────────────────────────────────────────────────────────────
Γ ⊢ ResolveCallee(Identifier(x), args) ⇓ Identifier(x)

**(ResolveCallee-Ident-Record)**
Γ ⊢ ResolveValueName(x) ⇑    args = []    Γ ⊢ ResolveTypeName(x) ⇓ ent    ent.origin_opt = p    name = (ent.target_opt if present, else x)    RecordDecl(FullPath(PathOfModule(p), name)) = R
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveCallee(Identifier(x), args) ⇓ Identifier(x)

**(ResolveCallee-Path-Value)**
Γ ⊢ ResolveQualified(path, name, ValueKind) ⇓ ent
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveCallee(Path(path, name), args) ⇓ Path(path, name)

**(ResolveCallee-Path-Record)**
Γ ⊢ ResolveRecordPath(path, name) ⇓ p    args = []
────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveCallee(Path(path, name), args) ⇓ Path(path, name)

**(ResolveCallee-Other)**
Γ ⊢ ResolveExpr(callee) ⇓ callee'
──────────────────────────────────────────────────────────────
Γ ⊢ ResolveCallee(callee, args) ⇓ callee'

**(ResolveExpr-Call)**
Γ ⊢ ResolveArgs(args) ⇓ args'    Γ ⊢ ResolveCallee(callee, args') ⇓ callee'
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveExpr(Call(callee, args)) ⇓ Call(callee', args')

**(ResolveExpr-RecordExpr)**
Γ ⊢ ResolveTypeRef(tr) ⇓ tr'    Γ ⊢ ResolveFieldInits(fields) ⇓ fields'
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveExpr(RecordExpr(tr, fields)) ⇓ RecordExpr(tr', fields')

**(ResolveExpr-EnumLiteral)**
Γ ⊢ ResolveEnumPayload(payload_opt) ⇓ payload_opt'
────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveExpr(EnumLiteral(path, payload_opt)) ⇓ EnumLiteral(path, payload_opt')

ResolveArmJudg = {ResolveArm, ResolveArms}

**(ResolveArm)**
Γ_0 = PushScope(Γ)    Γ_0 ⊢ ResolvePattern(p) ⇓ p'    Γ_0 ⊢ BindPattern(p') ⇓ Γ_1    Γ_1 ⊢ ResolveExprOpt(g) ⇓ g'    Γ_1 ⊢ ResolveExpr(b) ⇓ b'
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveArm(⟨p, g, b⟩) ⇓ ⟨p', g', b'⟩

**(ResolveArms-Empty)**
──────────────────────────────────────────────
Γ ⊢ ResolveArms([]) ⇓ []

**(ResolveArms-Cons)**
Γ ⊢ ResolveArm(a) ⇓ a'    Γ ⊢ ResolveArms(as) ⇓ as'
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveArms(a :: as) ⇓ a' :: as'

**(ResolveExpr-Match)**
Γ ⊢ ResolveExpr(scrutinee) ⇓ scrutinee'    Γ ⊢ ResolveArms(arms) ⇓ arms'
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveExpr(MatchExpr(scrutinee, arms)) ⇓ MatchExpr(scrutinee', arms')

**(ResolveExpr-LoopIter)**
Γ ⊢ ResolvePattern(pat) ⇓ pat'    Γ ⊢ ResolveTypeOpt(ty_opt) ⇓ ty_opt'    Γ ⊢ ResolveExpr(iter) ⇓ iter'    Γ_0 = PushScope(Γ)    Γ_0 ⊢ BindPattern(pat') ⇓ Γ_1    Γ_1 ⊢ ResolveExpr(body) ⇓ body'
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveExpr(LoopIter(pat, ty_opt, iter, body)) ⇓ LoopIter(pat', ty_opt', iter', body')

**(ResolveExpr-Alloc-Explicit-ByAlias)**
Γ ⊢ ResolveValueName(r) ⇓ ent    RegionAlias(ent)    Γ ⊢ ResolveExpr(e) ⇓ e'
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveExpr(Binary("^", Identifier(r), e)) ⇓ AllocExpr(r, e')

ResolveExprRules = {ResolveExpr-Ident, ResolveExpr-Qualified, ResolveExpr-Call, ResolveExpr-RecordExpr, ResolveExpr-EnumLiteral, ResolveExpr-Match, ResolveExpr-LoopIter, ResolveExpr-Alloc-Explicit-ByAlias, ResolveExpr-Hom, ResolveExpr-Alloc-Implicit, ResolveExpr-Alloc-Explicit, ResolveExpr-Block}

NoSpecificResolveExpr(e) ⇔ ¬ ∃ r ∈ ResolveExprRules \ {ResolveExpr-Hom}. PremisesHold(r, e)

**(ResolveExpr-Hom)**
NoSpecificResolveExpr(C(e_1, …, e_n))    ∀ i, Γ ⊢ ResolveExpr(e_i) ⇓ e_i'
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveExpr(C(e_1, …, e_n)) ⇓ C(e_1', …, e_n')

**(ResolveExpr-Alloc-Implicit)**
Γ ⊢ ResolveExpr(e) ⇓ e'
────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveExpr(AllocExpr(⊥, e)) ⇓ AllocExpr(⊥, e')

**(ResolveExpr-Alloc-Explicit)**
Γ ⊢ ResolveValueName(r) ⇓ ent    Γ ⊢ ResolveExpr(e) ⇓ e'
────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveExpr(AllocExpr(r, e)) ⇓ AllocExpr(r, e')

ResolveStmtSeqJudg = {ResolveStmtSeq}

**(ResolveStmtSeq-Empty)**
────────────────────────────────────────────────────────────────
Γ ⊢ ResolveStmtSeq([]) ⇓ (Γ, [])

**(ResolveStmtSeq-Cons)**
Γ ⊢ ResolveStmt(s) ⇓ (Γ_1, s')    Γ_1 ⊢ ResolveStmtSeq(ss) ⇓ (Γ_2, ss')
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveStmtSeq(s :: ss) ⇓ (Γ_2, s' :: ss')

**(ResolveExpr-Block)**
Γ_0 = PushScope(Γ)    Γ_0 ⊢ ResolveStmtSeq(stmts) ⇓ (Γ_1, stmts')    (tail_opt = ⊥ ⇒ tail_opt' = ⊥)    (tail_opt = e ⇒ Γ_1 ⊢ ResolveExpr(e) ⇓ e' ∧ tail_opt' = e')
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveExpr(BlockExpr(stmts, tail_opt)) ⇓ BlockExpr(stmts', tail_opt')

**(BindNames-Empty)**
──────────────────────────────────────────────
Γ ⊢ BindNames([]) ⇓ Γ

**(BindNames-Cons)**
Γ ⊢ Intro(x, ⟨Value, ⊥, ⊥, Decl⟩) ⇓ Γ_1    Γ_1 ⊢ BindNames(xs) ⇓ Γ_2
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BindNames(x :: xs) ⇓ Γ_2

**(BindPattern)**
Γ ⊢ PatNames(p) ⇓ ns    Γ ⊢ BindNames(ns) ⇓ Γ'
────────────────────────────────────────────────────────────────
Γ ⊢ BindPattern(p) ⇓ Γ'

**(ResolveStmt-Let)**
Γ ⊢ ResolveExpr(init) ⇓ init'    Γ ⊢ ResolveTypeOpt(ty_opt) ⇓ ty_opt'    Γ ⊢ ResolvePattern(pat) ⇓ pat'    Γ ⊢ BindPattern(pat') ⇓ Γ'
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveStmt(LetStmt(⟨pat, ty_opt, op, init, span⟩)) ⇓ (Γ', LetStmt(⟨pat', ty_opt', op, init', span⟩))

**(ResolveStmt-Var)**
Γ ⊢ ResolveExpr(init) ⇓ init'    Γ ⊢ ResolveTypeOpt(ty_opt) ⇓ ty_opt'    Γ ⊢ ResolvePattern(pat) ⇓ pat'    Γ ⊢ BindPattern(pat') ⇓ Γ'
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveStmt(VarStmt(⟨pat, ty_opt, op, init, span⟩)) ⇓ (Γ', VarStmt(⟨pat', ty_opt', op, init', span⟩))

**(ResolveStmt-ShadowLet)**
Γ ⊢ ResolveExpr(init) ⇓ init'    Γ ⊢ ResolveTypeOpt(ty_opt) ⇓ ty_opt'    Γ ⊢ ShadowIntro(x, ⟨Value, ⊥, ⊥, Decl⟩) ⇓ Γ'
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveStmt(ShadowLetStmt(x, ty_opt, init)) ⇓ (Γ', ShadowLetStmt(x, ty_opt', init'))

**(ResolveStmt-ShadowVar)**
Γ ⊢ ResolveExpr(init) ⇓ init'    Γ ⊢ ResolveTypeOpt(ty_opt) ⇓ ty_opt'    Γ ⊢ ShadowIntro(x, ⟨Value, ⊥, ⊥, Decl⟩) ⇓ Γ'
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveStmt(ShadowVarStmt(x, ty_opt, init)) ⇓ (Γ', ShadowVarStmt(x, ty_opt', init'))

**(ResolveStmt-Defer)**
Γ ⊢ ResolveExpr(b) ⇓ b'
────────────────────────────────────────────────────────────────
Γ ⊢ ResolveStmt(DeferStmt(b)) ⇓ (Γ, DeferStmt(b'))

**(ResolveStmt-Frame-Implicit)**
Γ ⊢ ResolveExpr(b) ⇓ b'
────────────────────────────────────────────────────────────────
Γ ⊢ ResolveStmt(FrameStmt(⊥, b)) ⇓ (Γ, FrameStmt(⊥, b'))

**(ResolveStmt-Frame-Explicit)**
Γ ⊢ ResolveValueName(r) ⇓ ent    Γ ⊢ ResolveExpr(b) ⇓ b'
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveStmt(FrameStmt(r, b)) ⇓ (Γ, FrameStmt(r, b'))

**(ResolveStmt-Region)**
Γ ⊢ ResolveExprOpt(opts_opt) ⇓ opts_opt'    (alias_opt = ⊥ ⇒ Γ_r = Γ)    (alias_opt = r ⇒ Γ ⊢ Intro(r, ⟨Value, ⊥, ⊥, RegionAlias⟩) ⇓ Γ_r)    Γ_r ⊢ ResolveExpr(b) ⇓ b'
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveStmt(RegionStmt(opts_opt, alias_opt, b)) ⇓ (Γ, RegionStmt(opts_opt', alias_opt, b'))

ResolveExprOpt(⊥) = ⊥
ResolveExprOpt(e) = e' ⇔ Γ ⊢ ResolveExpr(e) ⇓ e'

ResState = {ResStart(M), ResNames(M, N), ResItems(M, N), ResDone(M'), Error(code)}

**(Res-Start)**
──────────────────────────────────────────────
⟨ResStart(M)⟩ → ⟨ResNames(M, _)⟩

**(Res-Names)**
Γ ⊢ CollectNames(M) ⇓ N
──────────────────────────────────────────────────────────────
⟨ResNames(M, _)⟩ → ⟨ResItems(M, N)⟩

**(Res-Items)**
Γ ⊢ ResolveModule(M) ⇓ M'
──────────────────────────────────────────────────────────────
⟨ResItems(M, N)⟩ → ⟨ResDone(M')⟩

**ResolveModules (Big-Step).**

**(ResolveModules-Ok)**
Γ ⊢ ParseModules(P) ⇓ [M_1, …, M_k]    ∀ i, Γ ⊢ ResolveModule(M_i) ⇓ M_i'
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveModules(P) ⇓ [M_1', …, M_k']

**(ResolveModules-Err-Parse)**
Γ ⊢ ParseModules(P) ⇑ c
──────────────────────────────────────────────────────────────
Γ ⊢ ResolveModules(P) ⇑ c

**(ResolveModules-Err-Resolve)**
Γ ⊢ ParseModules(P) ⇓ [M_1, …, M_k]    ∃ i. Γ ⊢ ResolveModule(M_i) ⇑ c
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ResolveModules(P) ⇑ c

### 5.2. Type System Core (Cursive0)

#### 5.2.1. Type Equivalence

TypeEqJudg = {≡}
ConstLenJudg = {ConstLen}

**(ConstLen-Lit)**
e = Literal(lit)    lit.kind = IntLiteral    InRange(IntValue(lit), "usize")    n = IntValue(lit)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ConstLen(e) ⇓ n

**(ConstLen-Path)**
e = Path(path, name)    ValuePathType(path, name) = T    StaticDecl(_, _, ⟨IdentPattern(name), _, "=", init, _⟩, _, _) ∈ Γ    Γ ⊢ ConstLen(init) ⇓ n
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ConstLen(e) ⇓ n

**(ConstLen-Err)**
¬ ∃ n. Γ ⊢ ConstLen(e) ⇓ n    c = Code(ConstLen-Err)
────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ConstLen(e) ⇑ c

MembersEq([T_1, …, T_n], [U_1, …, U_n]) ⇔ ∃ U'. Permutation(U', [U_1, …, U_n]) ∧ ∀ i. 0 ≤ i < n ⇒ Γ ⊢ T_i ≡ U'[i]

**(T-Equiv-Prim)**
T = TypePrim(n)    U = TypePrim(n)
──────────────────────────────────────────────
Γ ⊢ T ≡ U

**(T-Equiv-Perm)**
T = TypePerm(p, T_0)    U = TypePerm(p, U_0)    Γ ⊢ T_0 ≡ U_0
────────────────────────────────────────────────────────────────
Γ ⊢ T ≡ U

**(T-Equiv-Tuple)**
T = TypeTuple([T_1, …, T_n])    U = TypeTuple([U_1, …, U_n])    ∀ i, Γ ⊢ T_i ≡ U_i
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ T ≡ U

**(T-Equiv-Array)**
T = TypeArray(T_0, e_0)    U = TypeArray(U_0, e_1)    Γ ⊢ ConstLen(e_0) ⇓ n    Γ ⊢ ConstLen(e_1) ⇓ n    Γ ⊢ T_0 ≡ U_0
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ T ≡ U

**(T-Equiv-Slice)**
T = TypeSlice(T_0)    U = TypeSlice(U_0)    Γ ⊢ T_0 ≡ U_0
────────────────────────────────────────────────────────────────
Γ ⊢ T ≡ U

**(T-Equiv-Func)**
T = TypeFunc([⟨m_1, T_1⟩, …, ⟨m_n, T_n⟩], R)    U = TypeFunc([⟨m_1, U_1⟩, …, ⟨m_n, U_n⟩], S)    ∀ i, Γ ⊢ T_i ≡ U_i    Γ ⊢ R ≡ S
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ T ≡ U

**(T-Equiv-Union)**
T = TypeUnion([T_1, …, T_n])    U = TypeUnion([U_1, …, U_n])    MembersEq([T_1, …, T_n], [U_1, …, U_n])
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ T ≡ U

**(T-Equiv-Path)**
T = TypePath(p)    U = TypePath(p)
──────────────────────────────────────────────
Γ ⊢ T ≡ U

**(T-Equiv-ModalState)**
T = TypeModalState(p, S)    U = TypeModalState(p, S)
──────────────────────────────────────────────────────────────
Γ ⊢ T ≡ U

**(T-Equiv-String)**
T = TypeString(st)    U = TypeString(st)
──────────────────────────────────────────────
Γ ⊢ T ≡ U

**(T-Equiv-Bytes)**
T = TypeBytes(st)    U = TypeBytes(st)
──────────────────────────────────────────────
Γ ⊢ T ≡ U

**(T-Equiv-Range)**
T = TypeRange    U = TypeRange
──────────────────────────────────────────────
Γ ⊢ T ≡ U

**(T-Equiv-Ptr)**
T = TypePtr(T_0, s)    U = TypePtr(U_0, s)    Γ ⊢ T_0 ≡ U_0
────────────────────────────────────────────────────────────────
Γ ⊢ T ≡ U

**(T-Equiv-RawPtr)**
T = TypeRawPtr(q, T_0)    U = TypeRawPtr(q, U_0)    Γ ⊢ T_0 ≡ U_0
────────────────────────────────────────────────────────────────
Γ ⊢ T ≡ U

**(T-Equiv-Dynamic)**
T = TypeDynamic(p)    U = TypeDynamic(p)
──────────────────────────────────────────────
Γ ⊢ T ≡ U

#### 5.2.2. Subtyping

SubtypingJudg = {<:}

∀ T, U ∈ IntTypes. T ≠ U ⇒ ¬(Γ ⊢ T <: U)

∀ T, U ∈ FloatTypes. T ≠ U ⇒ ¬(Γ ⊢ T <: U)

PermSubJudg = {PermSub}

**(Perm-Const)**
──────────────────────────────────────────────
`const` PermSub `const`

**(Perm-Unique)**
──────────────────────────────────────────────
`unique` PermSub `unique`

**(Perm-Unique-Const)**
──────────────────────────────────────────────
`unique` PermSub `const`

**(Sub-Perm)**
T = TypePerm(p, T_0)    U = TypePerm(q, U_0)    PermSub(p, q)    Γ ⊢ T_0 <: U_0
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ T <: U

**(Sub-Never)**
T ∈ 𝒯
──────────────────────────────────────────────────────────────
Γ ⊢ TypePrim("!") <: T

**(Sub-Tuple)**
T = TypeTuple([T_1, …, T_n])    U = TypeTuple([U_1, …, U_n])    ∀ i, Γ ⊢ T_i <: U_i
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ T <: U

**(Sub-Array)**
T = TypeArray(T_0, e_0)    U = TypeArray(U_0, e_1)    Γ ⊢ ConstLen(e_0) ⇓ n    Γ ⊢ ConstLen(e_1) ⇓ n    Γ ⊢ T_0 <: U_0
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ T <: U

**(Sub-Slice)**
T = TypeSlice(T_0)    U = TypeSlice(U_0)    Γ ⊢ T_0 <: U_0
────────────────────────────────────────────────────────────────
Γ ⊢ T <: U

**(Sub-Range)**
T = TypeRange    U = TypeRange
──────────────────────────────────────────────
Γ ⊢ T <: U

**(Sub-Ptr-State)**
s ∈ {`Valid`, `Null`}
──────────────────────────────────────────────────────────────
Γ ⊢ TypePtr(T, s) <: TypePtr(T, ⊥)

**(Sub-Modal-Niche)**
Σ.Types[p] = `modal` M    S ∈ States(M)    NicheCompatible(M, S)
────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TypeModalState(p, S) <: TypePath(p)

**(Sub-Func)**
T = TypeFunc([⟨m_1, T_1⟩, …, ⟨m_n, T_n⟩], R)    U = TypeFunc([⟨m_1, U_1⟩, …, ⟨m_n, U_n⟩], S)    ∀ i, Γ ⊢ U_i <: T_i    Γ ⊢ R <: S
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ T <: U

Member(T, U) ⇔ U = TypeUnion([U_1, …, U_n]) ∧ ∃ i. Γ ⊢ T ≡ U_i

**(Sub-Member-Union)**
Member(T, U)
──────────────────────────────────────────────
Γ ⊢ T <: U

**(Sub-Union-Width)**
U_1 = TypeUnion([T_1, …, T_n])    U_2 = TypeUnion([U_1', …, U_m'])    ∀ i, Member(T_i, U_2)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ U_1 <: U_2

SubtypingRules = RulesIn({"5.2.2", "5.3.1", "5.7", "5.8"})

#### 5.2.3. Function Types

**(T-Proc-As-Value)**
procedure f(m_1 x_1 : T_1, …, m_n x_n : T_n) -> R declared
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ f : TypeFunc([⟨m_1, T_1⟩, …, ⟨m_n, T_n⟩], R)

FuncTypeRules = RulesIn({"5.2.1", "5.2.2"})

#### 5.2.4. Procedure Calls

ArgsOkTJudg = {ArgsOk_T}
ParamMode(⟨mode, T⟩) = mode
ParamType(⟨mode, T⟩) = T
ArgMoved(⟨moved, e, span⟩) = moved
ArgExpr(⟨moved, e, span⟩) = e
PlaceType(p) = T ⇔ Γ; R; L ⊢ p :place T
ArgType(a) =
  { ExprType(MovedArg(ArgMoved(a), ArgExpr(a)))  if ArgMoved(a) = true
    PlaceType(ArgExpr(a))                        if ArgMoved(a) = false }

**(ArgsT-Empty)**
──────────────────────────────────────────────
Γ; R; L ⊢ ArgsOk_T([], [])

**(ArgsT-Cons)**
Γ; R; L ⊢ MovedArg(moved, e) ⇐ T_p ⊣ ∅    moved = true    Γ; R; L ⊢ ArgsOk_T(ps, as)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ ArgsOk_T([⟨`move`, T_p⟩] ++ ps, [⟨moved, e, _⟩] ++ as)

**(ArgsT-Cons-Ref)**
Γ; R; L ⊢ e ⇐_place T_p    AddrOfOk(e)    moved = false    Γ; R; L ⊢ ArgsOk_T(ps, as)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ ArgsOk_T([⟨⊥, T_p⟩] ++ ps, [⟨moved, e, _⟩] ++ as)

**(T-Call)**
Γ; R; L ⊢ callee : TypeFunc(params, R_c)    Γ; R; L ⊢ ArgsOk_T(params, args)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Call(callee, args) : R_c

**(Call-Callee-NotFunc)**
Γ; R; L ⊢ callee : T    T ≠ TypeFunc(_, _)    ¬(RecordCallee(callee) ∧ args = [])    c = Code(Call-Callee-NotFunc)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Call(callee, args) ⇑ c

**(Call-ArgCount-Err)**
Γ; R; L ⊢ callee : TypeFunc(params, _)    |params| ≠ |args|    c = Code(Call-ArgCount-Err)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Call(callee, args) ⇑ c

**(Call-ArgType-Err)**
Γ; R; L ⊢ callee : TypeFunc(params, _)    ∃ i. ¬(Γ; R; L ⊢ ArgType(args[i]) <: ParamType(params[i]))    c = Code(Call-ArgType-Err)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Call(callee, args) ⇑ c

**(Call-Move-Missing)**
Γ; R; L ⊢ callee : TypeFunc(params, _)    ∃ i. ParamMode(params[i]) = `move` ∧ ArgMoved(args[i]) = false    c = Code(Call-Move-Missing)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Call(callee, args) ⇑ c

**(Call-Move-Unexpected)**
Γ; R; L ⊢ callee : TypeFunc(params, _)    ∃ i. ParamMode(params[i]) = ⊥ ∧ ArgMoved(args[i]) = true    c = Code(Call-Move-Unexpected)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Call(callee, args) ⇑ c

**(Call-Arg-NotPlace)**
Γ; R; L ⊢ callee : TypeFunc(params, _)    ∃ i. ParamMode(params[i]) = ⊥ ∧ ¬ IsPlace(ArgExpr(args[i]))    c = Code(Call-Arg-NotPlace)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Call(callee, args) ⇑ c

#### 5.2.5. Tuples

**(T-Unit-Literal)**
────────────────────────────────────────────────────────────────
Γ ⊢ TupleExpr([]) : TypePrim("()")

**(T-Tuple-Literal)**
n ≥ 1    ∀ i, Γ ⊢ e_i : T_i
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TupleExpr([e_1, …, e_n]) : TypeTuple([T_1, …, T_n])

**(T-Tuple-Index)**
Γ ⊢ e : TypeTuple([T_0, …, T_{n-1}])    0 ≤ i < n    BitcopyType(T_i)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TupleAccess(e, i) : T_i

**(T-Tuple-Index-Perm)**
Γ ⊢ e : TypePerm(p, TypeTuple([T_0, …, T_{n-1}]))    0 ≤ i < n    BitcopyType(TypePerm(p, T_i))
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TupleAccess(e, i) : TypePerm(p, T_i)

**(P-Tuple-Index)**
Γ ⊢ e :place TypeTuple([T_0, …, T_{n-1}])    0 ≤ i < n
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TupleAccess(e, i) :place T_i

**(P-Tuple-Index-Perm)**
Γ ⊢ e :place TypePerm(p, TypeTuple([T_0, …, T_{n-1}]))    0 ≤ i < n
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TupleAccess(e, i) :place TypePerm(p, T_i)

ConstTupleIndex(i) ⇔ ∃ n ∈ ℤ. i = n

**(TupleIndex-NonConst)**
¬ ConstTupleIndex(i)    c = Code(TupleIndex-NonConst)
────────────────────────────────────────────────────────────────
Γ ⊢ TupleAccess(e, i) ⇑ c

**(TupleIndex-OOB)**
Γ ⊢ e : TypeTuple([T_0, …, T_{n-1}])    (i < 0 ∨ i ≥ n)    c = Code(TupleIndex-OOB)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TupleAccess(e, i) ⇑ c

**(TupleAccess-NotTuple)**
Γ ⊢ e : T    StripPerm(T) ≠ TypeTuple(_)    c = Code(TupleAccess-NotTuple)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TupleAccess(e, i) ⇑ c

#### 5.2.6. Arrays and Slices

ConstIndex(e) ⇔ ∃ n. Γ ⊢ ConstLen(e) ⇓ n

**(T-Array-Literal-List)**
∀ i, Γ ⊢ e_i : T
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ArrayExpr([e_1, …, e_n]) : TypeArray(T, Literal(IntLiteral(n)))

**(T-Index-Array)**
Γ ⊢ e_1 : TypeArray(T, len)    Γ ⊢ e_2 : TypePrim("usize")    ConstIndex(e_2)    Γ ⊢ ConstLen(e_2) ⇓ i    Γ ⊢ ConstLen(len) ⇓ n    i < n    BitcopyType(T)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ IndexAccess(e_1, e_2) : T

**(T-Index-Array-Perm)**
Γ ⊢ e_1 : TypePerm(p, TypeArray(T, len))    Γ ⊢ e_2 : TypePrim("usize")    ConstIndex(e_2)    Γ ⊢ ConstLen(e_2) ⇓ i    Γ ⊢ ConstLen(len) ⇓ n    i < n    BitcopyType(TypePerm(p, T))
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ IndexAccess(e_1, e_2) : TypePerm(p, T)

**(Index-Array-NonConst-Err)**
Γ ⊢ e_1 : TypeArray(T, _)    Γ ⊢ e_2 : TypePrim("usize")    ¬ ConstIndex(e_2)    c = Code(Index-Array-NonConst-Err)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ IndexAccess(e_1, e_2) ⇑ c

**(Index-Array-OOB-Err)**
Γ ⊢ e_1 : TypeArray(T, len)    ConstIndex(e_2)    Γ ⊢ ConstLen(e_2) ⇓ i    Γ ⊢ ConstLen(len) ⇓ n    i ≥ n    c = Code(Index-Array-OOB-Err)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ IndexAccess(e_1, e_2) ⇑ c

**(Index-Slice-Direct-Err)**
Γ ⊢ e_1 : TypeSlice(T)    Γ ⊢ e_2 : TypePrim("usize")    c = Code(Index-Slice-Direct-Err)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ IndexAccess(e_1, e_2) ⇑ c

**(Index-Slice-Perm-Direct-Err)**
Γ ⊢ e_1 : TypePerm(p, TypeSlice(T))    Γ ⊢ e_2 : TypePrim("usize")    c = Code(Index-Slice-Direct-Err)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ IndexAccess(e_1, e_2) ⇑ c

**(T-Slice-From-Array)**
Γ ⊢ e_1 : TypeArray(T, n)    Γ; R; L ⊢ e_2 : Range    BitcopyType(TypeSlice(T))
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ IndexAccess(e_1, e_2) : TypeSlice(T)

**(T-Slice-From-Array-Perm)**
Γ ⊢ e_1 : TypePerm(p, TypeArray(T, n))    Γ; R; L ⊢ e_2 : Range    BitcopyType(TypePerm(p, TypeSlice(T)))
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ IndexAccess(e_1, e_2) : TypePerm(p, TypeSlice(T))

**(T-Slice-From-Slice)**
Γ ⊢ e_1 : TypeSlice(T)    Γ; R; L ⊢ e_2 : Range    BitcopyType(TypeSlice(T))
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ IndexAccess(e_1, e_2) : TypeSlice(T)

**(T-Slice-From-Slice-Perm)**
Γ ⊢ e_1 : TypePerm(p, TypeSlice(T))    Γ; R; L ⊢ e_2 : Range    BitcopyType(TypePerm(p, TypeSlice(T)))
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ IndexAccess(e_1, e_2) : TypePerm(p, TypeSlice(T))

**(P-Index-Array)**
Γ ⊢ e_1 :place TypeArray(T, len)    Γ ⊢ e_2 : TypePrim("usize")    ConstIndex(e_2)    Γ ⊢ ConstLen(e_2) ⇓ i    Γ ⊢ ConstLen(len) ⇓ n    i < n
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ IndexAccess(e_1, e_2) :place T

**(P-Index-Array-Perm)**
Γ ⊢ e_1 :place TypePerm(p, TypeArray(T, len))    Γ ⊢ e_2 : TypePrim("usize")    ConstIndex(e_2)    Γ ⊢ ConstLen(e_2) ⇓ i    Γ ⊢ ConstLen(len) ⇓ n    i < n
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ IndexAccess(e_1, e_2) :place TypePerm(p, T)

**(P-Slice-From-Array)**
Γ ⊢ e_1 :place TypeArray(T, n)    Γ; R; L ⊢ e_2 : Range
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ IndexAccess(e_1, e_2) :place TypeSlice(T)

**(P-Slice-From-Array-Perm)**
Γ ⊢ e_1 :place TypePerm(p, TypeArray(T, n))    Γ; R; L ⊢ e_2 : Range
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ IndexAccess(e_1, e_2) :place TypePerm(p, TypeSlice(T))

**(P-Slice-From-Slice)**
Γ ⊢ e_1 :place TypeSlice(T)    Γ; R; L ⊢ e_2 : Range
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ IndexAccess(e_1, e_2) :place TypeSlice(T)

**(P-Slice-From-Slice-Perm)**
Γ ⊢ e_1 :place TypePerm(p, TypeSlice(T))    Γ; R; L ⊢ e_2 : Range
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ IndexAccess(e_1, e_2) :place TypePerm(p, TypeSlice(T))

**(Coerce-Array-Slice)**
Γ ⊢ e : TypePerm(p, TypeArray(T, n))
──────────────────────────────────────────────────────────────
Γ ⊢ e : TypePerm(p, TypeSlice(T))

**(Index-Array-NonUsize)**
Γ ⊢ e_1 : TypeArray(T, _)    Γ ⊢ e_2 : T_i    T_i ≠ TypePrim("usize")    c = Code(Index-Array-NonUsize)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ IndexAccess(e_1, e_2) ⇑ c

**(Index-NonIndexable)**
Γ ⊢ e_1 : T    StripPerm(T) ∉ {TypeArray(_, _), TypeSlice(_)}    c = Code(Index-NonIndexable)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ IndexAccess(e_1, e_2) ⇑ c

#### 5.2.7. Union Types

Members(TypeUnion([T_1, …, T_n])) = [T_1, …, T_n]
DistinctMembers(U) = [T_i ∈ Members(U) | ∀ j < i. ¬(Γ ⊢ T_i ≡ T_j)]
SetMembers(U) = { T | T ∈ DistinctMembers(U) }

**(T-Union-Intro)**
Γ ⊢ e : T    Member(T, U)
──────────────────────────────────────────────
Γ ⊢ e : U

**(Union-DirectAccess-Err)**
Γ; R; L ⊢ e : U    StripPerm(U) = TypeUnion(_)    c = Code(Union-DirectAccess-Err)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ FieldAccess(e, f) ⇑ c

#### 5.2.8. Default Record Construction

Fields(R) = Fields_{§ 5.3.2}(R)

InitOk(f) ⇔ f = FieldDecl(vis, name, T_f, init_opt, span, doc) ∧ (init_opt = ⊥) ∨ (init_opt = e ∧ Γ; ⊥; ⊥ ⊢ e : T ∧ Γ ⊢ T <: T_f)

**(WF-Record)**
∀ f ∈ Fields(R), InitOk(f)    ∀ f_i ≠ f_j, f_i.name ≠ f_j.name
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ R record wf

**(WF-Record-DupField)**
∃ f_i ≠ f_j. f_i.name = f_j.name    c = Code(WF-Record-DupField)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ R record wf ⇑ c

DefaultConstructible(R) ⇔ ∀ f ∈ Fields(R). f.init_opt ≠ ⊥
RecordPath(R) = FullPath(ModuleOf(R), R.name)
RecordCallee(callee) = R ⇔ (callee = Identifier(name) ∨ callee = Path(path, name)) ∧ Γ ⊢ ResolveTypeName(name) ⇓ ent ∧ ent.origin_opt = mp ∧ name' = (ent.target_opt if present, else name) ∧ RecordDecl(FullPath(PathOfModule(mp), name')) = R

**(T-Record-Default)**
RecordCallee(callee) = R    Γ ⊢ R record wf    DefaultConstructible(R)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Call(callee, []) : TypePath(RecordPath(R))

**(Record-Default-Init-Err)**
RecordCallee(callee) = R    ¬ DefaultConstructible(R)    c = Code(Record-Default-Init-Err)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Call(callee, []) ⇑ c

#### 5.2.9. Type Inference (Cursive0)

TypeInfJudg = {⇒, ⇐, Solve}

Constraint = Type × Type
ConstraintSet = ℘(Constraint)
Solve(C) ⇓ id ⇔ C = ∅
Solve(C) ⇑ ⇔ C ≠ ∅
∀ Γ, R, L, e, T, C. Γ; R; L ⊢ e ⇒ T ⊣ C ⇒ C = ∅
∀ Γ, R, L, e, T, C. Γ; R; L ⊢ e ⇐ T ⊣ C ⇒ C = ∅

**(Syn-Expr)**
Γ; R; L ⊢ e : T
──────────────────────────────────────────────
Γ; R; L ⊢ e ⇒ T ⊣ ∅

**(Syn-Ident)**
(x : T) ∈ Γ
──────────────────────────────────────────────
Γ; R; L ⊢ Identifier(x) ⇒ T ⊣ ∅

**(Syn-Unit)**
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ TupleExpr([]) ⇒ TypePrim("()") ⊣ ∅

**(Syn-Tuple)**
n ≥ 1    ∀ i, Γ; R; L ⊢ e_i ⇒ T_i ⊣ C_i
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ TupleExpr([e_1, …, e_n]) ⇒ TypeTuple([T_1, …, T_n]) ⊣ ⋃_i C_i

**(Syn-Call)**
Γ; R; L ⊢ callee ⇒ TypeFunc(params, R_c) ⊣ C_0    Γ; R; L ⊢ ArgsOk_T(params, args)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Call(callee, args) ⇒ R_c ⊣ C_0

**(Syn-Call-Err)**
Γ; R; L ⊢ Call(callee, args) ⇑ c
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Call(callee, args) ⇒ T ⊣ C ⇑ c

**(Chk-Subsumption-Modal-NonNiche)**
Γ; R; L ⊢ e ⇒ S ⊣ C    StripPerm(S) = TypeModalState(p, S_s)    StripPerm(T) = TypePath(p)    Σ.Types[p] = `modal` M    ¬ NicheCompatible(M, S_s)    c = Code(Chk-Subsumption-Modal-NonNiche)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ e ⇐ T ⇑ c

**(Chk-Subsumption)**
Γ; R; L ⊢ e ⇒ S ⊣ C    Γ ⊢ S <: T
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ e ⇐ T ⊣ C

**(Chk-Null-Ptr)**
T = TypePtr(U, s)    s ∈ {`Null`, ⊥}
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ PtrNullExpr ⇐ T ⊣ ∅

PtrNullExpected(T) ⇔ T = TypePtr(U, s) ∧ s ∈ {`Null`, ⊥}

**(Syn-PtrNull-Err)**
c = Code(PtrNull-Infer-Err)
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ PtrNullExpr ⇒ T ⊣ C ⇑ c

**(Chk-PtrNull-Err)**
¬ PtrNullExpected(T)    c = Code(PtrNull-Infer-Err)
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ PtrNullExpr ⇐ T ⊣ C ⇑ c

#### 5.2.10. Literal Expressions (Cursive0)

IntTypes = {`i8`, `i16`, `i32`, `i64`, `i128`, `u8`, `u16`, `u32`, `u64`, `u128`, `isize`, `usize`}
FloatTypes = {`f16`, `f32`, `f64`}
FloatFormat("f16") = `binary16`    FloatFormat("f32") = `binary32`    FloatFormat("f64") = `binary64`
FloatBitWidth("f16") = 16    FloatBitWidth("f32") = 32    FloatBitWidth("f64") = 64
FloatValueSet(t) = { v | v is a value representable by IEEE 754-2019 format FloatFormat(t) }
IEEE754Encode(t, v) = bits ⇔ v ∈ FloatValueSet(t) ∧ bits ∈ [0, 2^{FloatBitWidth(t)} - 1] ∧ ((v is NaN in IEEE 754-2019 format FloatFormat(t) ∧ bits = CanonicalNaNBits(t)) ∨ (v is not NaN in IEEE 754-2019 format FloatFormat(t) ∧ bits is the IEEE 754-2019 encoding of v in format FloatFormat(t)))
CanonicalNaNBits("f16") = 0x7E00    CanonicalNaNBits("f32") = 0x7FC00000    CanonicalNaNBits("f64") = 0x7FF8000000000000
CanonicalNaN(t) = v ⇔ IEEE754Encode(t, v) = CanonicalNaNBits(t)
NonNaNValueSet(t) = { v ∈ FloatValueSet(t) | IEEE754Encode(t, v) ≠ CanonicalNaNBits(t) }
LSB(n) = n mod 2
EvenSignificandLSB(t, v) ⇔ LSB(IEEE754Encode(t, v)) = 0
DefaultInt = `i32`
DefaultFloat = `f64`
IntValue : Token ⇀ ℤ    FloatValue : Token ⇀ ℝ
IntSuffix : Token ⇀ IntTypes    FloatSuffix : Token ⇀ FloatTypes
IntCore(s) ⇔ s matches (`decimal_integer` | `hex_integer` | `octal_integer` | `binary_integer`)
FloatCore(s) ⇔ s matches `float_literal`
StripIntSuffix(s) = ⟨core, t⟩ ⇔ s = core ++ t ∧ t ∈ IntSuffixSet ∧ IntCore(core)
StripIntSuffix(s) = ⟨s, ⊥⟩ ⇔ ¬ ∃ t. s = core ++ t ∧ t ∈ IntSuffixSet ∧ IntCore(core)
StripFloatSuffix(s) = ⟨core, t⟩ ⇔ s = core ++ t ∧ t ∈ FloatSuffixSet ∧ FloatCore(core)
StripFloatSuffix(s) = ⟨s, ⊥⟩ ⇔ ¬ ∃ t. s = core ++ t ∧ t ∈ FloatSuffixSet ∧ FloatCore(core)
IntSuffix(lit) = t ⇔ lit.kind = IntLiteral ∧ StripIntSuffix(Lexeme(lit)) = ⟨_, t⟩
FloatSuffix(lit) = t ⇔ lit.kind = FloatLiteral ∧ StripFloatSuffix(Lexeme(lit)) = ⟨_, t⟩
IntValueCore(s) = v ⇔ (s = "0x" ++ h ∧ v = HexValue(Digits(h))) ∨ (s = "0o" ++ o ∧ v = OctValue(Digits(o))) ∨ (s = "0b" ++ b ∧ v = BinValue(Digits(b))) ∨ (s matches `decimal_integer` ∧ v = DecValue(Digits(s)))
IntValue(lit) = v ⇔ lit.kind = IntLiteral ∧ StripIntSuffix(Lexeme(lit)) = ⟨core, _⟩ ∧ IntValueCore(core) = v
FloatParts(s) = ⟨a, b, e⟩ ⇔ s = a ++ "." ++ b ++ exp ∧ (exp = "" ⇒ e = 0) ∧ (exp ≠ "" ⇒ exp = "e" ++ sign ++ d ∨ exp = "E" ++ sign ++ d) ∧ (sign = "" ⇒ e = DecValue(Digits(d))) ∧ (sign = "+" ⇒ e = DecValue(Digits(d))) ∧ (sign = "-" ⇒ e = -DecValue(Digits(d)))
FloatValueCore(s) = v ⇔ FloatParts(s) = ⟨a, b, e⟩ ∧ v = (DecValue(Digits(a)) · 10^{|Digits(b)|} + DecValue(Digits(b))) · 10^{e - |Digits(b)|}
FloatValue(lit) = v ⇔ lit.kind = FloatLiteral ∧ StripFloatSuffix(Lexeme(lit)) = ⟨core, _⟩ ∧ FloatValueCore(core) = v
InRange(v, T) ⇔ v ∈ RangeOf(T)
RangeOf : TypePrimName → ℘(ℝ)
RangeOf(t) = [-2^{w-1}, 2^{w-1} - 1] if t ∈ SignedIntTypes ∧ w = IntWidth(t)
RangeOf(t) = [0, 2^{w} - 1] if t ∈ UnsignedIntTypes ∧ w = IntWidth(t)
RangeOf(t) undefined otherwise

**(T-Int-Literal-Suffix)**
lit.kind = IntLiteral    IntSuffix(lit) = t    InRange(IntValue(lit), t)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Literal(lit) : TypePrim(t)

**(T-Int-Literal-Default)**
lit.kind = IntLiteral    IntSuffix(lit) = ⊥    InRange(IntValue(lit), `i32`)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Literal(lit) : TypePrim("i32")

**(T-Float-Literal-Suffix)**
lit.kind = FloatLiteral    FloatSuffix(lit) = t
────────────────────────────────────────────────────────────────
Γ ⊢ Literal(lit) : TypePrim(t)

**(T-Float-Literal-Default)**
lit.kind = FloatLiteral    FloatSuffix(lit) = ⊥
────────────────────────────────────────────────────────────────
Γ ⊢ Literal(lit) : TypePrim("f64")

**(T-Bool-Literal)**
lit.kind = BoolLiteral
──────────────────────────────────────────────
Γ ⊢ Literal(lit) : TypePrim("bool")

**(T-Char-Literal)**
lit.kind = CharLiteral
──────────────────────────────────────────────
Γ ⊢ Literal(lit) : TypePrim("char")

**(T-String-Literal)**
lit.kind = StringLiteral
──────────────────────────────────────────────────────────────
Γ ⊢ Literal(lit) : TypeString(`@View`)

**(Syn-Literal)**
Γ ⊢ Literal(lit) : T
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Literal(lit) ⇒ T ⊣ ∅

NullLiteralExpected(T) ⇔ T = TypeRawPtr(q, U)

**(Chk-Int-Literal)**
lit.kind = IntLiteral    T = TypePrim(t)    t ∈ IntTypes    InRange(IntValue(lit), t)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Literal(lit) ⇐ T ⊣ ∅

**(Chk-Float-Literal)**
lit.kind = FloatLiteral    T = TypePrim(t)    t ∈ FloatTypes
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Literal(lit) ⇐ T ⊣ ∅

**(Chk-Null-Literal)**
lit.kind = NullLiteral    T = TypeRawPtr(q, U)
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Literal(lit) ⇐ T ⊣ ∅

**(Syn-Null-Literal-Err)**
lit.kind = NullLiteral    c = Code(NullLiteral-Infer-Err)
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Literal(lit) ⇒ T ⊣ C ⇑ c

**(Chk-Null-Literal-Err)**
lit.kind = NullLiteral    ¬ NullLiteralExpected(T)    c = Code(NullLiteral-Infer-Err)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Literal(lit) ⇐ T ⊣ C ⇑ c

#### 5.2.11. Statement Typing (Cursive0)

MutKind = {`let`, `var`}
Bind = {⟨mut, T⟩ | mut ∈ MutKind ∧ T ∈ Type}
BindOf(Γ, x) = ⟨mut, T⟩ ⇔ ⟨mut, T⟩ is the binding for x in the nearest scope of Scopes(Γ)
(x : T) ∈ Γ ⇔ ∃ mut. BindOf(Γ, x) = ⟨mut, T⟩
MutOf(Γ, x) = mut ⇔ BindOf(Γ, x) = ⟨mut, T⟩

StmtJudg = {Γ; R; L ⊢ s ⇒ Γ' ▷ ⟨Res, Brk, BrkVoid⟩, Γ; R; L ⊢ ss ⇒ Γ' ▷ ⟨Res, Brk, BrkVoid⟩, Γ; R; L ⊢ e : T, Γ; R; L ⊢ b : T, Γ; R; L ⊢ BlockInfo(b) ⇓ ⟨T, Brk, BrkVoid⟩, Γ ⊢ pat ⇐ T ⊣ B}

LoopFlag = {⊥, `loop`}

PushScope(Γ) = Γ' ⇔ Scopes(Γ') = [∅] ++ Scopes(Γ) ∧ Project(Γ') = Project(Γ) ∧ ResCtx(Γ') = ResCtx(Γ)
PopScope(Γ') = Γ ⇔ Scopes(Γ') = [_] ++ Scopes(Γ) ∧ Project(Γ') = Project(Γ) ∧ ResCtx(Γ') = ResCtx(Γ)

IntroEnt = ⟨Value, ⊥, ⊥, Decl⟩

**(IntroAll-Empty)**
────────────────────────────────────────────────────────────────
IntroAll(Γ, []) ⇓ Γ

**(IntroAll-Cons)**
Γ ⊢ Intro(x, IntroEnt) ⇓ Γ_1    IntroAll(Γ_1 ∪ {x ↦ ⟨`let`, T⟩}, rest) ⇓ Γ_2
────────────────────────────────────────────────────────────────────────────────────────────────
IntroAll(Γ, [(x, T)] ++ rest) ⇓ Γ_2

**(IntroAllVar-Empty)**
────────────────────────────────────────────────────────────────
IntroAllVar(Γ, []) ⇓ Γ

**(IntroAllVar-Cons)**
Γ ⊢ Intro(x, IntroEnt) ⇓ Γ_1    IntroAllVar(Γ_1 ∪ {x ↦ ⟨`var`, T⟩}, rest) ⇓ Γ_2
────────────────────────────────────────────────────────────────────────────────────────────────
IntroAllVar(Γ, [(x, T)] ++ rest) ⇓ Γ_2

**(ShadowAll-Empty)**
────────────────────────────────────────────────────────────────
ShadowAll(Γ, []) ⇓ Γ

**(ShadowAll-Cons)**
Γ ⊢ ShadowIntro(x, IntroEnt) ⇓ Γ_1    ShadowAll(Γ_1 ∪ {x ↦ ⟨`let`, T⟩}, rest) ⇓ Γ_2
────────────────────────────────────────────────────────────────────────────────────────────────
ShadowAll(Γ, [(x, T)] ++ rest) ⇓ Γ_2

**(ShadowAllVar-Empty)**
────────────────────────────────────────────────────────────────
ShadowAllVar(Γ, []) ⇓ Γ

**(ShadowAllVar-Cons)**
Γ ⊢ ShadowIntro(x, IntroEnt) ⇓ Γ_1    ShadowAllVar(Γ_1 ∪ {x ↦ ⟨`var`, T⟩}, rest) ⇓ Γ_2
────────────────────────────────────────────────────────────────────────────────────────────────
ShadowAllVar(Γ, [(x, T)] ++ rest) ⇓ Γ_2

ResType([T_1, …, T_n]) = T ⇔ n ≥ 1 ∧ ∀ i. Γ ⊢ T_i ≡ T
ResType([]) = ⊥

LoopTypeInf(Brk, BrkVoid) =
  { TypePrim("!")    if Brk = [] ∧ BrkVoid = false
    TypePrim("()")   if Brk = [] ∧ BrkVoid = true
    T                if BrkVoid = false ∧ ResType(Brk) = T
    ⊥                otherwise }

LoopTypeFin(Brk, BrkVoid) =
  { TypePrim("()")   if Brk = []
    T                if BrkVoid = false ∧ ResType(Brk) = T
    ⊥                otherwise }

**Statement Sequences**

**(StmtSeq-Empty)**
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ [] ⇒ Γ ▷ ⟨[], [], false⟩

**(StmtSeq-Cons)**
Γ; R; L ⊢ s ⇒ Γ_1 ▷ ⟨Res_1, Brk_1, B_1⟩    Γ_1; R; L ⊢ ss ⇒ Γ_2 ▷ ⟨Res_2, Brk_2, B_2⟩
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ s :: ss ⇒ Γ_2 ▷ ⟨Res_1 ++ Res_2, Brk_1 ++ Brk_2, B_1 ∨ B_2⟩

**Bindings**

binding = ⟨pat, ty_opt, op, init, _⟩

**(T-LetStmt-Ann)**
ty_opt = T_a    Γ; R; L ⊢ init ⇐ T_a ⊣ ∅    Γ ⊢ pat ⇐ T_a ⊣ B    Distinct(PatNames(pat))    IntroAll(Γ, B) ⇓ Γ'
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ LetStmt(binding) ⇒ Γ' ▷ ⟨[], [], false⟩

**(T-LetStmt-Ann-Mismatch)**
ty_opt = T_a    Γ; R; L ⊢ init ⇒ T_i ⊣ C    ¬(Γ ⊢ T_i <: T_a)    c = Code(T-LetStmt-Ann-Mismatch)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ LetStmt(binding) ⇑ c

**(T-LetStmt-Infer)**
ty_opt = ⊥    Γ; R; L ⊢ init ⇒ T_i ⊣ C    Solve(C) ⇓ θ    T_b = θ(T_i)    Γ ⊢ pat ⇐ T_b ⊣ B    Distinct(PatNames(pat))    IntroAll(Γ, B) ⇓ Γ'
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ LetStmt(binding) ⇒ Γ' ▷ ⟨[], [], false⟩

**(T-LetStmt-Infer-Err)**
ty_opt = ⊥    Γ; R; L ⊢ init ⇒ T_i ⊣ C    Solve(C) ⇑    c = Code(T-LetStmt-Infer-Err)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ LetStmt(binding) ⇑ c

**(T-VarStmt-Ann)**
ty_opt = T_a    Γ; R; L ⊢ init ⇐ T_a ⊣ ∅    Γ ⊢ pat ⇐ T_a ⊣ B    Distinct(PatNames(pat))    IntroAllVar(Γ, B) ⇓ Γ'
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ VarStmt(binding) ⇒ Γ' ▷ ⟨[], [], false⟩

**(T-VarStmt-Ann-Mismatch)**
ty_opt = T_a    Γ; R; L ⊢ init ⇒ T_i ⊣ C    ¬(Γ ⊢ T_i <: T_a)    c = Code(T-VarStmt-Ann-Mismatch)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ VarStmt(binding) ⇑ c

**(T-VarStmt-Infer)**
ty_opt = ⊥    Γ; R; L ⊢ init ⇒ T_i ⊣ C    Solve(C) ⇓ θ    T_b = θ(T_i)    Γ ⊢ pat ⇐ T_b ⊣ B    Distinct(PatNames(pat))    IntroAllVar(Γ, B) ⇓ Γ'
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ VarStmt(binding) ⇒ Γ' ▷ ⟨[], [], false⟩

**(T-VarStmt-Infer-Err)**
ty_opt = ⊥    Γ; R; L ⊢ init ⇒ T_i ⊣ C    Solve(C) ⇑    c = Code(T-VarStmt-Infer-Err)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ VarStmt(binding) ⇑ c

**(T-ShadowLetStmt-Ann)**
ty_opt = T_a    Γ; R; L ⊢ init ⇐ T_a ⊣ ∅    ShadowAll(Γ, [⟨name, T_a⟩]) ⇓ Γ'
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ ShadowLetStmt(name, ty_opt, init) ⇒ Γ' ▷ ⟨[], [], false⟩

**(T-ShadowLetStmt-Ann-Mismatch)**
ty_opt = T_a    Γ; R; L ⊢ init ⇒ T_i ⊣ C    ¬(Γ ⊢ T_i <: T_a)    c = Code(T-ShadowLetStmt-Ann-Mismatch)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ ShadowLetStmt(name, ty_opt, init) ⇑ c

**(T-ShadowLetStmt-Infer)**
ty_opt = ⊥    Γ; R; L ⊢ init ⇒ T_i ⊣ C    Solve(C) ⇓ θ    T_b = θ(T_i)    ShadowAll(Γ, [⟨name, T_b⟩]) ⇓ Γ'
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ ShadowLetStmt(name, ty_opt, init) ⇒ Γ' ▷ ⟨[], [], false⟩

**(T-ShadowLetStmt-Infer-Err)**
ty_opt = ⊥    Γ; R; L ⊢ init ⇒ T_i ⊣ C    Solve(C) ⇑    c = Code(T-ShadowLetStmt-Infer-Err)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ ShadowLetStmt(name, ty_opt, init) ⇑ c

**(T-ShadowVarStmt-Ann)**
ty_opt = T_a    Γ; R; L ⊢ init ⇐ T_a ⊣ ∅    ShadowAllVar(Γ, [⟨name, T_a⟩]) ⇓ Γ'
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ ShadowVarStmt(name, ty_opt, init) ⇒ Γ' ▷ ⟨[], [], false⟩

**(T-ShadowVarStmt-Ann-Mismatch)**
ty_opt = T_a    Γ; R; L ⊢ init ⇒ T_i ⊣ C    ¬(Γ ⊢ T_i <: T_a)    c = Code(T-ShadowVarStmt-Ann-Mismatch)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ ShadowVarStmt(name, ty_opt, init) ⇑ c

**(T-ShadowVarStmt-Infer)**
ty_opt = ⊥    Γ; R; L ⊢ init ⇒ T_i ⊣ C    Solve(C) ⇓ θ    T_b = θ(T_i)    ShadowAllVar(Γ, [⟨name, T_b⟩]) ⇓ Γ'
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ ShadowVarStmt(name, ty_opt, init) ⇒ Γ' ▷ ⟨[], [], false⟩

**(T-ShadowVarStmt-Infer-Err)**
ty_opt = ⊥    Γ; R; L ⊢ init ⇒ T_i ⊣ C    Solve(C) ⇑    c = Code(T-ShadowVarStmt-Infer-Err)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ ShadowVarStmt(name, ty_opt, init) ⇑ c

**Assignments**

PlaceRoot(Identifier(x)) = x
PlaceRoot(FieldAccess(p, _)) = PlaceRoot(p)
PlaceRoot(TupleAccess(p, _)) = PlaceRoot(p)
PlaceRoot(IndexAccess(p, _)) = PlaceRoot(p)
PlaceRoot(Deref(p)) = PlaceRoot(p)

**(T-Assign)**
IsPlace(p)    PlaceRoot(p) = x    MutOf(Γ, x) = `var`    Γ; R; L ⊢ p :place T_p    Γ; R; L ⊢ e ⇐ T_p ⊣ ∅
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ AssignStmt(p, e) ⇒ Γ ▷ ⟨[], [], false⟩

**(T-CompoundAssign)**
IsPlace(p)    PlaceRoot(p) = x    MutOf(Γ, x) = `var`    Γ; R; L ⊢ p :place T_p    StripPerm(T_p) = TypePrim(t)    t ∈ NumericTypes    Γ; R; L ⊢ e : T_e    Γ ⊢ T_e <: TypePrim(t)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ CompoundAssignStmt(p, op, e) ⇒ Γ ▷ ⟨[], [], false⟩

**(Assign-NotPlace)**
stmt ∈ {AssignStmt(p, e), CompoundAssignStmt(p, op, e)}    ¬ IsPlace(p)    c = Code(Assign-NotPlace)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ stmt ⇑ c

**(Assign-Immutable-Err)**
stmt ∈ {AssignStmt(p, e), CompoundAssignStmt(p, op, e)}    IsPlace(p)    PlaceRoot(p) = x    MutOf(Γ, x) = `let`    c = Code(Assign-Immutable-Err)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ stmt ⇑ c

**(Assign-Type-Err)**
stmt ∈ {AssignStmt(p, e), CompoundAssignStmt(p, op, e)}    IsPlace(p)    Γ; R; L ⊢ p :place T_p    Γ; R; L ⊢ e ⇒ T_e ⊣ C    ((stmt = AssignStmt(p, e) ∧ ¬(Γ ⊢ T_e <: T_p)) ∨ (stmt = CompoundAssignStmt(p, op, e) ∧ (¬(Γ ⊢ T_e <: StripPerm(T_p)) ∨ ¬ ∃ t. StripPerm(T_p) = TypePrim(t) ∧ t ∈ NumericTypes)))    c = Code(Assign-Type-Err)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ stmt ⇑ c

**Const-Qualified Places.**

**(Assign-Const-Err)**
stmt ∈ {AssignStmt(p, e), CompoundAssignStmt(p, op, e)}    Γ; R; L ⊢ p : TypePerm(`const`, T)    c = Code(Assign-Const-Err)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ stmt ⇑ c

**Expression and Unsafe Statements**

**(T-ExprStmt)**
Γ; R; L ⊢ e : T
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ ExprStmt(e) ⇒ Γ ▷ ⟨[], [], false⟩

**(T-UnsafeStmt)**
Γ; R; L ⊢ b : T
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ UnsafeBlockStmt(b) ⇒ Γ ▷ ⟨[], [], false⟩

**(T-DeferStmt)**
Γ; R; L ⊢ b ⇐ TypePrim("()") ⊣ ∅    DeferSafe(b)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ DeferStmt(b) ⇒ Γ ▷ ⟨[], [], false⟩

**(Defer-NonUnit-Err)**
Γ; R; L ⊢ b : T_b    T_b ≠ TypePrim("()")    c = Code(Defer-NonUnit-Err)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ DeferStmt(b) ⇑ c

**(Defer-NonLocal-Err)**
¬ DeferSafe(b)    c = Code(Defer-NonLocal-Err)
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ DeferStmt(b) ⇑ c

**(HasNonLocalCtrl-Return)**
────────────────────────────────────────────────────────────────
HasNonLocalCtrl(ReturnStmt(_), in_loop)

**(HasNonLocalCtrl-Result)**
────────────────────────────────────────────────────────────────
HasNonLocalCtrl(ResultStmt(_), in_loop)

**(HasNonLocalCtrl-Break)**
in_loop = false
────────────────────────────────────────────────────────────────
HasNonLocalCtrl(BreakStmt(_), in_loop)

**(HasNonLocalCtrl-Continue)**
in_loop = false
────────────────────────────────────────────────────────────────
HasNonLocalCtrl(ContinueStmt, in_loop)

**(HasNonLocalCtrl-LoopInfinite)**
HasNonLocalCtrl(body, true)
────────────────────────────────────────────────────────────────
HasNonLocalCtrl(LoopInfinite(body), in_loop)

**(HasNonLocalCtrl-LoopConditional)**
HasNonLocalCtrl(body, true)
────────────────────────────────────────────────────────────────
HasNonLocalCtrl(LoopConditional(_, body), in_loop)

**(HasNonLocalCtrl-LoopIter)**
HasNonLocalCtrl(body, true)
────────────────────────────────────────────────────────────────
HasNonLocalCtrl(LoopIter(_, _, _, body), in_loop)

**(HasNonLocalCtrl-Child)**
C ∉ {LoopInfinite, LoopConditional, LoopIter}    ∃ i. HasNonLocalCtrl(x_i, in_loop)
────────────────────────────────────────────────────────────────────────────────────────────────
HasNonLocalCtrl(C(x_1, …, x_n), in_loop)

DeferSafe(b) ⇔ ¬ HasNonLocalCtrl(b, false)

**(T-RegionStmt)**
RegionOptsExpr(opts_opt) = opts    Γ; R; L ⊢ opts ⇐ TypePath([`RegionOptions`]) ⊣ ∅    RegionBind(Γ, alias_opt) = Γ_r    Γ_r; R; L ⊢ b : T_b
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ RegionStmt(opts_opt, alias_opt, b) ⇒ Γ ▷ ⟨[], [], false⟩

**(T-FrameStmt-Implicit)**
FrameBind(Γ, ⊥) = Γ_f    Γ_f; R; L ⊢ b : T_b
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ FrameStmt(⊥, b) ⇒ Γ ▷ ⟨[], [], false⟩

**(T-FrameStmt-Explicit)**
FrameBind(Γ, r) = Γ_f    Γ_f; R; L ⊢ b : T_b
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ FrameStmt(r, b) ⇒ Γ ▷ ⟨[], [], false⟩

**(Frame-NoActiveRegion-Err)**
InnermostActiveRegion(Γ) undefined    c = Code(Frame-NoActiveRegion-Err)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ FrameStmt(⊥, b) ⇑ c

**(Frame-Target-NotActive-Err)**
Γ; R; L ⊢ Identifier(r) : T_r    ¬ RegionActiveType(T_r)    c = Code(Frame-Target-NotActive-Err)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ FrameStmt(r, b) ⇑ c

**Control Flow Statements**

**(T-Return-Value)**
Γ; R; L ⊢ e ⇐ R ⊣ ∅
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ ReturnStmt(e) ⇒ Γ ▷ ⟨[], [], false⟩

**(T-Return-Unit)**
R = TypePrim("()")
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ ReturnStmt(⊥) ⇒ Γ ▷ ⟨[], [], false⟩

**(Return-Type-Err)**
Γ; R; L ⊢ e : T    ¬(Γ ⊢ T <: R)    c = Code(Return-Type-Err)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ ReturnStmt(e) ⇑ c

**(Return-Unit-Err)**
R ≠ TypePrim("()")    c = Code(Return-Type-Err)
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ ReturnStmt(⊥) ⇑ c

**(T-ResultStmt)**
Γ; R; L ⊢ e : T
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ ResultStmt(e) ⇒ Γ ▷ ⟨[T], [], false⟩

ResultNotLast(stmts) ⇔ ∃ pre, rest, e. stmts = pre ++ [ResultStmt(e)] ++ rest ∧ rest ≠ []

FirstResultSpan([]) = ⊥
FirstResultSpan(ResultStmt(e) :: rest) = span(ResultStmt(e))
FirstResultSpan(s :: rest) = FirstResultSpan(rest) if s ≠ ResultStmt(_)

**(Warn-Result-Unreachable)**
ResultNotLast(stmts)    FirstResultSpan(stmts) = sp    Γ ⊢ Emit(W-SEM-1001, sp)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ WarnResultUnreachable(stmts) ⇓ ok

**(Warn-Result-Ok)**
¬ ResultNotLast(stmts)
──────────────────────────────────────────────
Γ ⊢ WarnResultUnreachable(stmts) ⇓ ok

**(T-Break-Value)**
L = `loop`    Γ; R; L ⊢ e : T
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ BreakStmt(e) ⇒ Γ ▷ ⟨[], [T], false⟩

**(T-Break-Unit)**
L = `loop`
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ BreakStmt(⊥) ⇒ Γ ▷ ⟨[], [], true⟩

**(Break-Outside-Loop)**
L ≠ `loop`    c = Code(Break-Outside-Loop)
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ BreakStmt(_) ⇑ c

**(T-Continue)**
L = `loop`
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ ContinueStmt ⇒ Γ ▷ ⟨[], [], false⟩

**(Continue-Outside-Loop)**
L ≠ `loop`    c = Code(Continue-Outside-Loop)
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ ContinueStmt ⇑ c

**(T-ErrorStmt)**
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ ErrorStmt(_) ⇒ Γ ▷ ⟨[], [], false⟩

**Block Expressions**

LastStmt([]) = ⊥
LastStmt([s_1, …, s_n]) = s_n    (n ≥ 1)

**(BlockInfo-Res)**
Γ_0 = PushScope(Γ)    Γ_0; R; L ⊢ stmts ⇒ Γ_1 ▷ ⟨Res, Brk, BrkVoid⟩    Γ ⊢ WarnResultUnreachable(stmts) ⇓ ok    ResType(Res) = T    (tail_opt = e ⇒ Γ_1; R; L ⊢ e : T_e)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ BlockInfo(BlockExpr(stmts, tail_opt)) ⇓ ⟨T, Brk, BrkVoid⟩

**(BlockInfo-Res-Err)**
Γ_0 = PushScope(Γ)    Γ_0; R; L ⊢ stmts ⇒ Γ_1 ▷ ⟨Res, Brk, BrkVoid⟩    Res ≠ []    ¬ ∃ T. ResType(Res) = T    c = Code(BlockInfo-Res-Err)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ BlockInfo(BlockExpr(stmts, tail_opt)) ⇑ c

**(BlockInfo-Tail)**
Γ_0 = PushScope(Γ)    Γ_0; R; L ⊢ stmts ⇒ Γ_1 ▷ ⟨Res, Brk, BrkVoid⟩    Γ ⊢ WarnResultUnreachable(stmts) ⇓ ok    ResType(Res) = ⊥    tail_opt = e    Γ_1; R; L ⊢ e : T
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ BlockInfo(BlockExpr(stmts, tail_opt)) ⇓ ⟨T, Brk, BrkVoid⟩

**(BlockInfo-ReturnTail)**
Γ_0 = PushScope(Γ)    Γ_0; R; L ⊢ stmts ⇒ Γ_1 ▷ ⟨Res, Brk, BrkVoid⟩    Γ ⊢ WarnResultUnreachable(stmts) ⇓ ok    ResType(Res) = ⊥    tail_opt = ⊥    LastStmt(stmts) = ReturnStmt(_)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ BlockInfo(BlockExpr(stmts, ⊥)) ⇓ ⟨TypePrim("!"), Brk, BrkVoid⟩

**(BlockInfo-Unit)**
Γ_0 = PushScope(Γ)    Γ_0; R; L ⊢ stmts ⇒ Γ_1 ▷ ⟨Res, Brk, BrkVoid⟩    Γ ⊢ WarnResultUnreachable(stmts) ⇓ ok    ResType(Res) = ⊥    tail_opt = ⊥    LastStmt(stmts) ≠ ReturnStmt(_)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ BlockInfo(BlockExpr(stmts, ⊥)) ⇓ ⟨TypePrim("()"), Brk, BrkVoid⟩

**(T-Block)**
Γ; R; L ⊢ BlockInfo(b) ⇓ ⟨T, _, _⟩
──────────────────────────────────────────────
Γ; R; L ⊢ b : T

**(Chk-Block-Tail)**
Γ_0 = PushScope(Γ)    Γ_0; R; L ⊢ stmts ⇒ Γ_1 ▷ ⟨Res, Brk, BrkVoid⟩    Γ ⊢ WarnResultUnreachable(stmts) ⇓ ok    ResType(Res) = ⊥    tail_opt = e    Γ_1; R; L ⊢ e ⇐ T ⊣ ∅
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ BlockExpr(stmts, tail_opt) ⇐ T ⊣ ∅

**(Chk-Block-Return)**
Γ_0 = PushScope(Γ)    Γ_0; R; L ⊢ stmts ⇒ Γ_1 ▷ ⟨Res, Brk, BrkVoid⟩    Γ ⊢ WarnResultUnreachable(stmts) ⇓ ok    ResType(Res) = ⊥    tail_opt = ⊥    LastStmt(stmts) = ReturnStmt(_)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ BlockExpr(stmts, ⊥) ⇐ T ⊣ ∅

**(Chk-Block-Unit)**
Γ_0 = PushScope(Γ)    Γ_0; R; L ⊢ stmts ⇒ Γ_1 ▷ ⟨Res, Brk, BrkVoid⟩    Γ ⊢ WarnResultUnreachable(stmts) ⇓ ok    ResType(Res) = ⊥    tail_opt = ⊥    LastStmt(stmts) ≠ ReturnStmt(_)    T = TypePrim("()")
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ BlockExpr(stmts, ⊥) ⇐ T ⊣ ∅

**(T-Unsafe-Expr)**
Γ; R; L ⊢ b : T
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ UnsafeBlockExpr(b) : T

**(Chk-Unsafe-Expr)**
Γ; R; L ⊢ b ⇐ T ⊣ ∅
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ UnsafeBlockExpr(b) ⇐ T ⊣ ∅

**Loop Expressions**

**(T-Loop-Infinite)**
Γ; R; `loop` ⊢ BlockInfo(body) ⇓ ⟨T_b, Brk, BrkVoid⟩    LoopTypeInf(Brk, BrkVoid) = T
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LoopInfinite(body) : T

**(T-Loop-Conditional)**
Γ; R; L ⊢ cond : TypePrim("bool")    Γ; R; `loop` ⊢ BlockInfo(body) ⇓ ⟨T_b, Brk, BrkVoid⟩    LoopTypeFin(Brk, BrkVoid) = T
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LoopConditional(cond, body) : T

**(T-Loop-Iter)**
(Γ; R; L ⊢ iter : TypePerm(p, TypeSlice(T)) ∨ Γ; R; L ⊢ iter : TypeSlice(T) ∨ Γ; R; L ⊢ iter : TypePerm(p, TypeArray(T, n)) ∨ Γ; R; L ⊢ iter : TypeArray(T, n))    (ty_opt = ⊥ ⇒ T_p = T)    (ty_opt = T_a ⇒ Γ ⊢ T <: T_a ∧ T_p = T_a)    Γ ⊢ pat ⇐ T_p ⊣ B    Distinct(PatNames(pat))    Γ_0 = PushScope(Γ)    IntroAll(Γ_0, B) ⇓ Γ_1    Γ_1; R; `loop` ⊢ BlockInfo(body) ⇓ ⟨T_b, Brk, BrkVoid⟩    LoopTypeFin(Brk, BrkVoid) = T_r
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LoopIter(pat, ty_opt, iter, body) : T_r

**Irrefutable Binding Patterns**

PatJudg = {Γ ⊢ pat ⇐ T ⊣ B}

**(Pat-Dup-Err)**
¬ Distinct(PatNames(pat))    c = Code(Pat-Dup-Err)
────────────────────────────────────────────────────────────────
Γ ⊢ pat ⇐ T ⇑ c

**(Pat-Wildcard)**
──────────────────────────────────────────────
Γ ⊢ _ ⇐ T ⊣ ∅

**(Pat-Ident)**
──────────────────────────────────────────────
Γ ⊢ x ⇐ T ⊣ {x ↦ T}

**(Pat-Unit)**
T = TypePrim("()")
──────────────────────────────────────────────
Γ ⊢ () ⇐ T ⊣ ∅

**(Pat-Tuple-Arity-Err)**
T = TypeTuple([T_1, …, T_n])    ps = [p_1, …, p_m]    m ≠ n    c = Code(Pat-Tuple-Arity-Err)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ (p_1, …, p_m) ⇐ T ⇑ c

**(Pat-Tuple)**
T = TypeTuple([T_1, …, T_n])    ∀ i, Γ ⊢ p_i ⇐ T_i ⊣ B_i    B = ⊎_i B_i
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ (p_1, …, p_n) ⇐ T ⊣ B

**(Pat-Record)**
RecordDecl(p) = R ⇔ RecordPath(R) = p
FieldType(R, f) = T_f ⇔ ⟨_, f, T_f, _, _, _⟩ ∈ Fields(R)
FieldName(⟨f, _, _⟩) = f
PatOf(⟨f, ⊥, _⟩) = IdentifierPattern(f)
PatOf(⟨f, p, _⟩) = p    if p ≠ ⊥

T = TypePath(p)    RecordDecl(p) = R    ∀ fp ∈ fs, FieldType(R, FieldName(fp)) = T_f ∧ FieldVisible(m, R, FieldName(fp)) ∧ Γ ⊢ PatOf(fp) ⇐ T_f ⊣ B_f    B = ⊎_{fp ∈ fs} B_f
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ p{fs} ⇐ T ⊣ B

**(RecordPattern-UnknownField-Bind)**
T = TypePath(p)    RecordDecl(p) = R    ∃ fp ∈ fs. FieldName(fp) ∉ FieldNameSet(R)    c = Code(RecordPattern-UnknownField)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ p{fs} ⇐ T ⇑ c

**(Let-Refutable-Pattern-Err)**
pat ∈ {TypedPattern(_, _), LiteralPattern(_), EnumPattern(_, _, _), ModalPattern(_, _), RangePattern(_, _, _)}    c = Code(Let-Refutable-Pattern-Err)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ pat ⇐ T ⇑ c

#### 5.2.12. Expression Typing (Cursive0)

UnresolvedExpr = {QualifiedName(_, _), QualifiedApply(_, _, _)}

**(Expr-Unresolved-Err)**
e ∈ UnresolvedExpr    c = Code(ResolveExpr-Ident-Err)
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ e ⇑ c

ExprJudg = {Γ; R; L ⊢ e : T, Γ; R; L ⊢ e ⇐ T ⊣ C, Γ; R; L ⊢ p :place T, Γ; R; L ⊢ p ⇐_place T, Γ; R; L ⊢ r : Range}

**(Lift-Expr)**
Γ ⊢ e : T
──────────────────────────────────────────────
Γ; R; L ⊢ e : T

**(Place-Check)**
Γ; R; L ⊢ p :place S    Γ ⊢ S <: T
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ p ⇐_place T

StripPerm(T) =
  { T_0   if T = TypePerm(p, T_0)
    T     otherwise }
BitcopyType(T) ⇔ BitcopyTypeCore(T)
SignedIntTypes = {i8, i16, i32, i64, i128, isize}
NumericTypes = IntTypes ∪ FloatTypes
EqType(T) ⇔ (StripPerm(T) = TypePrim(t) ∧ t ∈ NumericTypes ∪ {`bool`, `char`}) ∨ StripPerm(T) = TypePtr(U, s) ∨ StripPerm(T) = TypeRawPtr(q, U) ∨ StripPerm(T) = TypeString(st) ∨ StripPerm(T) = TypeBytes(st)
OrdType(T) ⇔ StripPerm(T) = TypePrim(t) ∧ t ∈ IntTypes ∪ FloatTypes ∪ {`char`}
ValuePathType : Path × Ident ⇀ Type
ValuePathType(path, name) = StaticType(path, name) if StaticType(path, name) defined
ValuePathType(path, name) = TypeFunc([⟨mode, T⟩ | ⟨mode, x, T⟩ ∈ params], ProcReturn(ret_opt)) if DeclOf(path, name) = ProcedureDecl(_, name, params, ret_opt, _, _, _)
ValuePathType(path, name) undefined otherwise
UnsafeSpan(span) ⇔ ∃ U, K, D, K', i. ParseInputs(U, K, D, K') ∧ SourceOf(K'_i).path = span.file ∧ ∃ sp ∈ UnsafeSpans(K'_i). span ⊆ sp


**Identifiers and Paths**

**(T-Ident)**
(x : T) ∈ Γ    BitcopyType(T)
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Identifier(x) : T

**(T-Path-Value)**
ValuePathType(path, name) = T
──────────────────────────────────────────────
Γ; R; L ⊢ Path(path, name) : T

**Place Typing**

**(P-Ident)**
(x : T) ∈ Γ
──────────────────────────────────────────────
Γ; R; L ⊢ Identifier(x) :place T

**Record Field Access**

FieldVis(R, f) = vis ⇔ ⟨vis, f, T_f, _⟩ ∈ Fields(R)
FieldVisible(m, R, f) ⇔ FieldVis(R, f) ∈ {`public`, `internal`} ∨ (FieldVis(R, f) ∈ {`private`, `protected`} ∧ ModuleOfPath(RecordPath(R)) = m)

**(T-Field-Record)**
Γ; R; L ⊢ e : TypePath(p)    RecordDecl(p) = R    FieldType(R, f) = T_f    FieldVisible(m, R, f)    BitcopyType(T_f)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ FieldAccess(e, f) : T_f

**(T-Field-Record-Perm)**
Γ; R; L ⊢ e : TypePerm(p, TypePath(q))    RecordDecl(q) = R    FieldType(R, f) = T_f    FieldVisible(m, R, f)    BitcopyType(TypePerm(p, T_f))
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ FieldAccess(e, f) : TypePerm(p, T_f)

**(P-Field-Record)**
Γ; R; L ⊢ e :place TypePath(p)    RecordDecl(p) = R    FieldType(R, f) = T_f    FieldVisible(m, R, f)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ FieldAccess(e, f) :place T_f

**(P-Field-Record-Perm)**
Γ; R; L ⊢ e :place TypePerm(p, TypePath(q))    RecordDecl(q) = R    FieldType(R, f) = T_f    FieldVisible(m, R, f)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ FieldAccess(e, f) :place TypePerm(p, T_f)

**(FieldAccess-Unknown)**
Γ; R; L ⊢ e : TypePath(p)    RecordDecl(p) = R    f ∉ FieldNameSet(R)    c = Code(FieldAccess-Unknown)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ FieldAccess(e, f) ⇑ c

**(FieldAccess-NotVisible)**
Γ; R; L ⊢ e : TypePath(p)    RecordDecl(p) = R    f ∈ FieldNameSet(R)    ¬ FieldVisible(m, R, f)    c = Code(FieldAccess-NotVisible)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ FieldAccess(e, f) ⇑ c

**(FieldAccess-Enum)**
Γ; R; L ⊢ e : TypePath(p)    EnumDecl(p) = E    c = Code(FieldAccess-Unknown)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ FieldAccess(e, f) ⇑ c

**(ValueUse-NonBitcopyPlace)**
IsPlace(e)    ¬ BitcopyType(ExprType(e))    c = Code(ValueUse-NonBitcopyPlace)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ e ⇑ c

**Error Expression**

**(T-ErrorExpr)**
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ ErrorExpr(_) : TypePrim("!")

**Range Expressions**

Γ; R; L ⊢ r : Range ⇒ ExprType(r) = TypeRange

**(Range-Full)**
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Range(`Full`, ⊥, ⊥) : Range

**(Range-To)**
Γ; R; L ⊢ e : TypePrim("usize")
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Range(`To`, ⊥, e) : Range

**(Range-ToInclusive)**
Γ; R; L ⊢ e : TypePrim("usize")
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Range(`ToInclusive`, ⊥, e) : Range

**(Range-From)**
Γ; R; L ⊢ e : TypePrim("usize")
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Range(`From`, e, ⊥) : Range

**(Range-Exclusive)**
Γ; R; L ⊢ e_1 : TypePrim("usize")    Γ; R; L ⊢ e_2 : TypePrim("usize")
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Range(`Exclusive`, e_1, e_2) : Range

**(Range-Inclusive)**
Γ; R; L ⊢ e_1 : TypePrim("usize")    Γ; R; L ⊢ e_2 : TypePrim("usize")
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Range(`Inclusive`, e_1, e_2) : Range

**(Range-NonIndex-Err)**
e = Range(kind, lo_opt, hi_opt)    c = Code(Range-NonIndex-Err)
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ e ⇑ c

**If Expressions**

**(T-If)**
Γ; R; L ⊢ c : TypePrim("bool")    Γ; R; L ⊢ b_t : T    Γ; R; L ⊢ b_f : T
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ IfExpr(c, b_t, b_f) : T

**(T-If-No-Else)**
Γ; R; L ⊢ c : TypePrim("bool")    Γ; R; L ⊢ b_t : TypePrim("()")
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ IfExpr(c, b_t, ⊥) : TypePrim("()")

**(Chk-If)**
Γ; R; L ⊢ c : TypePrim("bool")    Γ; R; L ⊢ b_t ⇐ T ⊣ ∅    Γ; R; L ⊢ b_f ⇐ T ⊣ ∅
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ IfExpr(c, b_t, b_f) ⇐ T ⊣ ∅

**(Chk-If-No-Else)**
Γ; R; L ⊢ c : TypePrim("bool")    T = TypePrim("()")    Γ; R; L ⊢ b_t ⇐ T ⊣ ∅
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ IfExpr(c, b_t, ⊥) ⇐ T ⊣ ∅

**Unary Operators**

**(T-Not-Bool)**
Γ; R; L ⊢ e : TypePrim("bool")
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Unary("!", e) : TypePrim("bool")

**(T-Not-Int)**
Γ; R; L ⊢ e : TypePrim(t)    t ∈ IntTypes
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Unary("!", e) : TypePrim(t)

**(T-Neg)**
Γ; R; L ⊢ e : TypePrim(t)    t ∈ SignedIntTypes ∪ FloatTypes
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Unary("-", e) : TypePrim(t)

**Binary Operators**

ArithOps = {"+", "-", "*", "/", "%", "**"}
BitOps = {"&", "|", "^"}
ShiftOps = {"<<", ">>"}
CmpOps = {"==", "!=", "<", "<=", ">", ">="}
LogicOps = {"&&", "||"}

**(T-Arith)**
Γ; R; L ⊢ e_1 : T    Γ; R; L ⊢ e_2 : T    StripPerm(T) = TypePrim(t)    t ∈ NumericTypes    op ∈ ArithOps
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Binary(op, e_1, e_2) : TypePrim(t)

**(T-Bitwise)**
Γ; R; L ⊢ e_1 : T    Γ; R; L ⊢ e_2 : T    StripPerm(T) = TypePrim(t)    t ∈ IntTypes    op ∈ BitOps
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Binary(op, e_1, e_2) : TypePrim(t)

**(T-Shift)**
Γ; R; L ⊢ e_1 : T_1    Γ; R; L ⊢ e_2 : TypePrim("u32")    StripPerm(T_1) = TypePrim(t)    t ∈ IntTypes    op ∈ ShiftOps
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Binary(op, e_1, e_2) : TypePrim(t)

**(T-Compare-Eq)**
Γ; R; L ⊢ e_1 : T    Γ; R; L ⊢ e_2 : T    EqType(T)    op ∈ {"==", "!="}
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Binary(op, e_1, e_2) : TypePrim("bool")

**(T-Compare-Ord)**
Γ; R; L ⊢ e_1 : T    Γ; R; L ⊢ e_2 : T    OrdType(T)    op ∈ {"<", "<=", ">", ">="}
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Binary(op, e_1, e_2) : TypePrim("bool")

**(T-Logical)**
Γ; R; L ⊢ e_1 : TypePrim("bool")    Γ; R; L ⊢ e_2 : TypePrim("bool")    op ∈ LogicOps
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Binary(op, e_1, e_2) : TypePrim("bool")

**Casts**

S' = StripPerm(S)
T' = StripPerm(T)
CastValid(S, T) ⇔ (S' = TypePrim(s) ∧ T' = TypePrim(t) ∧ s, t ∈ NumericTypes) ∨ (S' = TypePrim("bool") ∧ T' = TypePrim(t) ∧ t ∈ IntTypes) ∨ (S' = TypePrim(t) ∧ t ∈ IntTypes ∧ T' = TypePrim("bool")) ∨ (S' = TypePrim("char") ∧ T' = TypePrim("u32")) ∨ (S' = TypePrim("u32") ∧ T' = TypePrim("char"))

**(T-Cast)**
Γ; R; L ⊢ e : S    CastValid(S, T)
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Cast(e, T) : T

**Address-Of, Dereference, Move**

AddrOfOk(p) ⇔ IsPlace(p) ∧ (p = IndexAccess(_, idx) ⇒ Γ; R; L ⊢ idx : T_i ∧ StripPerm(T_i) = TypePrim("usize"))

**(T-AddrOf)**
Γ; R; L ⊢ p :place T    AddrOfOk(p)
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ AddressOf(p) : TypePtr(T, `Valid`)

**(T-Alloc-Explicit)**
Γ; R; L ⊢ e : T    Γ; R; L ⊢ Identifier(r) : T_r    RegionActiveType(T_r)
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ AllocExpr(r, e) : T

**(T-Alloc-Implicit)**
InnermostActiveRegion(Γ) = r    Γ; R; L ⊢ e : T
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ AllocExpr(⊥, e) : T

**(Alloc-Region-NotFound-Err)**
e = AllocExpr(r, _)    r ≠ ⊥    Γ; R; L ⊢ Identifier(r) : T_r    ¬ RegionActiveType(T_r)    c = Code(Alloc-Region-NotFound-Err)
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ e ⇑ c

**(Alloc-Implicit-NoRegion-Err)**
e = AllocExpr(⊥, _)    InnermostActiveRegion(Γ) undefined    c = Code(Alloc-Implicit-NoRegion-Err)
──────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ e ⇑ c

**(T-Deref-Ptr)**
Γ; R; L ⊢ e : TypePtr(T, `Valid`)    BitcopyType(T)
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Deref(e) : T

**(T-Deref-Raw)**
UnsafeSpan(span(Deref(e)))    Γ; R; L ⊢ e : TypeRawPtr(q, T)    BitcopyType(T)
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Deref(e) : T

**(P-Deref-Ptr)**
Γ; R; L ⊢ e : TypePtr(T, `Valid`)
──────────────────────────────────────────────
Γ; R; L ⊢ Deref(e) :place T

**(P-Deref-Raw-Imm)**
UnsafeSpan(span(Deref(e)))    Γ; R; L ⊢ e : TypeRawPtr(`imm`, T)
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Deref(e) :place TypePerm(`const`, T)

**(P-Deref-Raw-Mut)**
UnsafeSpan(span(Deref(e)))    Γ; R; L ⊢ e : TypeRawPtr(`mut`, T)
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Deref(e) :place TypePerm(`unique`, T)

**(T-Move)**
Γ; R; L ⊢ p :place T
──────────────────────────────────────────────
Γ; R; L ⊢ MoveExpr(p) : T

**(AddrOf-NonPlace)**
¬ IsPlace(e)    c = Code(AddrOf-NonPlace)
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ AddressOf(e) ⇑ c

**(AddrOf-Index-Array-NonUsize)**
p = IndexAccess(base, idx)    Γ; R; L ⊢ base : TypeArray(T, n)    Γ; R; L ⊢ idx : T_i    StripPerm(T_i) ≠ TypePrim("usize")    c = Code(Index-Array-NonUsize)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ AddressOf(p) ⇑ c

**(AddrOf-Index-Slice-NonUsize)**
p = IndexAccess(base, idx)    Γ; R; L ⊢ base : TypeSlice(T, n)    Γ; R; L ⊢ idx : T_i    StripPerm(T_i) ≠ TypePrim("usize")    c = Code(Index-Slice-NonUsize)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ AddressOf(p) ⇑ c

**(Deref-Null)**
Γ; R; L ⊢ e : TypePtr(T, `Null`)    c = Code(Deref-Null)
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Deref(e) ⇑ c

**(Deref-Expired)**
Γ; R; L ⊢ e : TypePtr(T, `Expired`)    c = Code(Deref-Expired)
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Deref(e) ⇑ c

**(Deref-Raw-Unsafe)**
Γ; R; L ⊢ e : TypeRawPtr(q, T)    ¬ UnsafeSpan(span(Deref(e)))    c = Code(Deref-Raw-Unsafe)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Deref(e) ⇑ c

**Transmute**

**(T-Transmute-SizeEq)**
Γ ⊢ sizeof(t_1) = sizeof(t_2)
────────────────────────────────────────────────────────────────
Γ ⊢ TransmuteSizeOk(t_1, t_2)

**(T-Transmute-AlignEq)**
Γ ⊢ alignof(t_1) = alignof(t_2)
────────────────────────────────────────────────────────────────
Γ ⊢ TransmuteAlignOk(t_1, t_2)

**(T-Transmute)**
UnsafeSpan(span(TransmuteExpr(t_1, t_2, e)))    Γ ⊢ t_1 wf    Γ ⊢ t_2 wf    Γ ⊢ TransmuteSizeOk(t_1, t_2)    Γ ⊢ TransmuteAlignOk(t_1, t_2)    Γ; R; L ⊢ e : t_1
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ TransmuteExpr(t_1, t_2, e) : t_2

**(Transmute-Unsafe-Err)**
¬ UnsafeSpan(span(TransmuteExpr(t_1, t_2, e)))    c = Code(Transmute-Unsafe-Err)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ TransmuteExpr(t_1, t_2, e) ⇑ c

**Propagation (`?`)**

SuccessMember(R, U) = T_s ⇔ U = TypeUnion([T_1, …, T_n]) ∧ ¬(Γ ⊢ T_s <: R) ∧ ∀ i ≠ s. Γ ⊢ T_i <: R

**(T-Propagate)**
Γ; R; L ⊢ e : U    SuccessMember(R, U) = T_s
────────────────────────────────────────────────────────────────
Γ; R; L ⊢ Propagate(e) : T_s

**Record Literals**

FieldNames(R) = [ f.name | f ∈ Fields(R) ]
FieldInitNames(fields) = [ f | ⟨f, _⟩ ∈ fields ]
Set(xs) = { x | x ∈ xs }
FieldNameSet(R) = Set(FieldNames(R))
FieldInitSet(fields) = Set(FieldInitNames(fields))

**(T-Record-Literal)**
RecordDecl(p) = R    Distinct(FieldInitNames(fields))    FieldInitSet(fields) = FieldNameSet(R)    ∀ ⟨f, e⟩ ∈ fields, FieldType(R, f) = T_f ∧ FieldVisible(m, R, f) ∧ Γ; R; L ⊢ e ⇐ T_f ⊣ ∅
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ RecordExpr(TypePath(p), fields) : TypePath(p)

**(Record-FieldInit-Dup)**
RecordDecl(p) = R    ¬ Distinct(FieldInitNames(fields))    c = Code(Record-FieldInit-Dup)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ RecordExpr(TypePath(p), fields) ⇑ c

**(Record-FieldInit-Missing)**
RecordDecl(p) = R    FieldInitSet(fields) ≠ FieldNameSet(R)    c = Code(Record-FieldInit-Missing)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ RecordExpr(TypePath(p), fields) ⇑ c

**(Record-Field-Unknown)**
RecordDecl(p) = R    ∃ ⟨f, e⟩ ∈ fields. f ∉ FieldNameSet(R)    c = Code(Record-Field-Unknown)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ RecordExpr(TypePath(p), fields) ⇑ c

**(Record-Field-NotVisible)**
RecordDecl(p) = R    ∃ ⟨f, e⟩ ∈ fields. ¬ FieldVisible(m, R, f)    c = Code(Record-Field-NotVisible)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ RecordExpr(TypePath(p), fields) ⇑ c

**(Record-Field-NonBitcopy-Move)**
RecordDecl(p) = R    ∃ ⟨f, e⟩ ∈ fields. FieldType(R, f) = T_f ∧ ¬ BitcopyType(T_f) ∧ IsPlace(e) ∧ e ≠ MoveExpr(_)    c = Code(Record-Field-NonBitcopy-Move)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ RecordExpr(TypePath(p), fields) ⇑ c

**Enum Literals**

EnumDecl(p) = E ⇔ EnumPathOf(E) = p
VariantPayload(E, v) = payload_opt ⇔ ∃ disc, span, doc. VariantDecl(v, payload_opt, disc, span, doc) ∈ E.variants
VariantFieldNames(fs) = [ f | FieldDecl(_, f, _, _, _, _) ∈ fs ]
VariantFieldNameSet(fs) = Set(VariantFieldNames(fs))

EnumFieldType(E, v, f) = T_f ⇔ VariantPayload(E, v) = RecordPayload(fs) ∧ ⟨_, f, T_f, _, _, _⟩ ∈ fs

**(T-Enum-Lit-Unit)**
EnumDecl(EnumPath(path)) = E    v = VariantName(path)    VariantPayload(E, v) = ⊥
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ EnumLiteral(path, ⊥) : TypePath(EnumPath(path))

**(T-Enum-Lit-Tuple)**
EnumDecl(EnumPath(path)) = E    v = VariantName(path)    VariantPayload(E, v) = TuplePayload([T_1, …, T_n])    ∀ i, Γ; R; L ⊢ e_i ⇐ T_i ⊣ ∅
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ EnumLiteral(path, Paren([e_1, …, e_n])) : TypePath(EnumPath(path))

**(T-Enum-Lit-Record)**
EnumDecl(EnumPath(path)) = E    v = VariantName(path)    VariantPayload(E, v) = RecordPayload(fs)    Distinct(FieldInitNames(fields))    FieldInitSet(fields) = VariantFieldNameSet(fs)    ∀ ⟨f, e⟩ ∈ fields, Γ; R; L ⊢ e ⇐ EnumFieldType(E, v, f) ⊣ ∅
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ EnumLiteral(path, Brace(fields)) : TypePath(EnumPath(path))

#### 5.2.13. Pattern Matching (Cursive0)

MatchJudg = {Γ ⊢ pat ◁ T ⊣ B, Γ; R; L ⊢ ArmBody(body) : T, Γ; R; L ⊢ ArmBody(body) ⇐ T}

PermWrap(T, B) =
  { { x ↦ TypePerm(p, T_x) | x ↦ T_x ∈ B }    if T = TypePerm(p, U)
    B                                      otherwise }

**(Pat-StripPerm)**
Γ ⊢ pat ◁ StripPerm(T) ⊣ B
──────────────────────────────────────────────
Γ ⊢ pat ◁ T ⊣ PermWrap(T, B)

PatternEffectRules = RulesIn({"5.2.15"})

ConstPatInt(p) = n ⇔ p = LiteralPattern(IntLiteral(n))

**(ArmBody-Expr)**
Γ; R; L ⊢ e : T
──────────────────────────────────────────────
Γ; R; L ⊢ ArmBody(e) : T

**(ArmBody-Block)**
Γ; R; L ⊢ b : T
──────────────────────────────────────────────
Γ; R; L ⊢ ArmBody(b) : T

**(ArmBody-Expr-Chk)**
Γ; R; L ⊢ e ⇐ T ⊣ ∅
──────────────────────────────────────────────
Γ; R; L ⊢ ArmBody(e) ⇐ T

**(ArmBody-Block-Chk)**
Γ; R; L ⊢ b ⇐ T ⊣ ∅
──────────────────────────────────────────────
Γ; R; L ⊢ ArmBody(b) ⇐ T

**(Pat-Dup-R-Err)**
¬ Distinct(PatNames(pat))    c = Code(Pat-Dup-Err)
────────────────────────────────────────────────────────────────
Γ ⊢ pat ◁ T ⇑ c

**(Pat-Wildcard-R)**
──────────────────────────────────────────────
Γ ⊢ _ ◁ T ⊣ ∅

**(Pat-Ident-R)**
──────────────────────────────────────────────
Γ ⊢ x ◁ T ⊣ {x ↦ T}

**(Pat-Literal-R)**
Γ ⊢ Literal(lit) : T_l    Γ ⊢ T_l <: T
────────────────────────────────────────────────────────────────
Γ ⊢ LiteralPattern(lit) ◁ T ⊣ ∅

**(Pat-Tuple-R-Arity-Err)**
StripPerm(T) = TypeTuple([T_1, …, T_n])    ps = [p_1, …, p_m]    m ≠ n    c = Code(Pat-Tuple-Arity-Err)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TuplePattern(ps) ◁ T ⇑ c

**(Pat-Tuple-R)**
StripPerm(T) = TypeTuple([T_1, …, T_n])    ∀ i, Γ ⊢ p_i ◁ T_i ⊣ B_i    B = ⊎_i B_i
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TuplePattern([p_1, …, p_n]) ◁ T ⊣ B

**(Pat-Record-R)**
StripPerm(T) = TypePath(p)    RecordDecl(p) = R    ∀ fp ∈ fs, FieldType(R, FieldName(fp)) = T_f ∧ FieldVisible(m, R, FieldName(fp)) ∧ Γ ⊢ PatOf(fp) ◁ T_f ⊣ B_f    B = ⊎_{fp ∈ fs} B_f
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ RecordPattern(p, fs) ◁ T ⊣ B

**(RecordPattern-UnknownField)**
StripPerm(T) = TypePath(p)    RecordDecl(p) = R    ∃ fp ∈ fs. FieldName(fp) ∉ FieldNameSet(R)    c = Code(RecordPattern-UnknownField)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ RecordPattern(p, fs) ◁ T ⇑ c

**(Pat-Enum-Unit-R)**
StripPerm(T) = TypePath(p)    EnumDecl(p) = E    VariantPayload(E, v) = ⊥
────────────────────────────────────────────────────────────────
Γ ⊢ EnumPattern(p, v, ⊥) ◁ T ⊣ ∅

**(Pat-Enum-Tuple-R)**
StripPerm(T) = TypePath(p)    EnumDecl(p) = E    VariantPayload(E, v) = TuplePayload([T_1, …, T_n])    ∀ i, Γ ⊢ p_i ◁ T_i ⊣ B_i    B = ⊎_i B_i
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EnumPattern(p, v, TuplePayloadPattern([p_1, …, p_n])) ◁ T ⊣ B

**(Pat-Enum-Record-R)**
StripPerm(T) = TypePath(p)    EnumDecl(p) = E    VariantPayload(E, v) = RecordPayload(fs')    ∀ fp ∈ fs, EnumFieldType(E, v, FieldName(fp)) = T_f ∧ Γ ⊢ PatOf(fp) ◁ T_f ⊣ B_f    B = ⊎_{fp ∈ fs} B_f
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EnumPattern(p, v, RecordPayloadPattern(fs)) ◁ T ⊣ B

**(Pat-Modal-R)**
StripPerm(T) = TypePath(p)    Σ.Types[p] = `modal` M    S ∈ States(M)    ∀ fp ∈ fs, PayloadMap(M, S)(FieldName(fp)) = T_f ∧ Γ ⊢ PatOf(fp) ◁ T_f ⊣ B_f    B = ⊎_{fp ∈ fs} B_f
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ModalPattern(S, fs) ◁ T ⊣ B

**(Pat-Modal-State-R)**
StripPerm(T) = TypeModalState(p, S)    Σ.Types[p] = `modal` M    ∀ fp ∈ fs, PayloadMap(M, S)(FieldName(fp)) = T_f ∧ Γ ⊢ PatOf(fp) ◁ T_f ⊣ B_f    B = ⊎_{fp ∈ fs} B_f
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ModalPattern(S, fs) ◁ T ⊣ B

**(Pat-Range-R)**
StripPerm(T) = TypePrim(t)    t ∈ IntTypes    ConstPatInt(p_l) = n_l    ConstPatInt(p_h) = n_h    (kind = ".." ⇒ n_l < n_h)    (kind = "..=" ⇒ n_l ≤ n_h)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ RangePattern(kind, p_l, p_h) ◁ T ⊣ ∅

**(RangePattern-NonConst)**
(ConstPatInt(p_l) undefined ∨ ConstPatInt(p_h) undefined)    c = Code(RangePattern-NonConst)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ RangePattern(kind, p_l, p_h) ◁ T ⇑ c

**(RangePattern-Empty)**
ConstPatInt(p_l) = n_l    ConstPatInt(p_h) = n_h    ((kind = "..") ⇒ n_l ≥ n_h)    ((kind = "..=") ⇒ n_l > n_h)    c = Code(RangePattern-Empty)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ RangePattern(kind, p_l, p_h) ◁ T ⇑ c

AllEq_Γ([T_1, …, T_n]) ⇔ ∀ i. Γ ⊢ T_i ≡ T_1
Irrefutable(pat, T) ⇔ pat = WildcardPattern ∨ pat = IdentifierPattern(_) ∨ (pat = TuplePattern([p_1, …, p_n]) ∧ StripPerm(T) = TypeTuple([T_1, …, T_n]) ∧ ∀ i. Irrefutable(p_i, T_i)) ∨ (pat = RecordPattern(p, fs) ∧ StripPerm(T) = TypePath(p) ∧ RecordDecl(p) = R ∧ ∀ fp ∈ fs. Irrefutable(PatOf(fp), FieldType(R, FieldName(fp))))
HasIrrefutableArm(arms, T) ⇔ ∃ arm ∈ arms. ∃ p, g, b. arm = ⟨p, g, b⟩ ∧ Irrefutable(p, T)

**Enum Match**

VariantNames(E) = [ v.name | v ∈ E.variants ]
ArmVariants(arms) = { v | ∃ p, g, b. ⟨p, g, b⟩ ∈ arms ∧ p = EnumPattern(_, v, _) }

**(T-Match-Enum)**
Γ; R; L ⊢ e : TypePath(p)    EnumDecl(p) = E    ∀ i, arm_i = ⟨p_i, g_i, b_i⟩    Γ ⊢ p_i ◁ TypePath(p) ⊣ B_i    Distinct(PatNames(p_i))    Γ_i = IntroAll(Γ, B_i)    (g_i ≠ ⊥ ⇒ Γ_i; R; L ⊢ g_i : TypePrim("bool"))    Γ_i; R; L ⊢ ArmBody(b_i) : T_i    AllEq_Γ([T_1, …, T_k])    (HasIrrefutableArm(arms, TypePath(p)) ∨ ArmVariants(arms) = VariantNames(E))
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ MatchExpr(e, arms) : T_1

**Modal Match**

StateNames(M) = States(M)
ArmStates(arms) = { s | ∃ p, g, b. ⟨p, g, b⟩ ∈ arms ∧ p = ModalPattern(_, s) }

**(T-Match-Modal)**
Γ; R; L ⊢ e : TypePath(p)    Σ.Types[p] = `modal` M    ∀ i, arm_i = ⟨p_i, g_i, b_i⟩    Γ ⊢ p_i ◁ TypePath(p) ⊣ B_i    Distinct(PatNames(p_i))    Γ_i = IntroAll(Γ, B_i)    (g_i ≠ ⊥ ⇒ Γ_i; R; L ⊢ g_i : TypePrim("bool"))    Γ_i; R; L ⊢ ArmBody(b_i) : T_i    AllEq_Γ([T_1, …, T_k])    (HasIrrefutableArm(arms, TypePath(p)) ∨ ArmStates(arms) = StateNames(M))
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ MatchExpr(e, arms) : T_1

**(Match-Modal-NonExhaustive)**
Γ; R; L ⊢ e : TypePath(p)    Σ.Types[p] = `modal` M    ¬(HasIrrefutableArm(arms, TypePath(p)) ∨ ArmStates(arms) = StateNames(M))    c = Code(Match-Modal-NonExhaustive)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ MatchExpr(e, arms) ⇑ c

**Union Match**

UnionTypes(U) = [T_1, …, T_n] ⇔ U = TypeUnion([T_1, …, T_n])
ArmUnionTypes(arms) = { T | ∃ p, g, b. ⟨p, g, b⟩ ∈ arms ∧ p = Pat-Union(T, _) }
UnionTypesExhaustive(arms, types) ⇔ ∀ T ∈ types. ∃ arm ∈ arms. ∃ p, g, b. arm = ⟨p, g, b⟩ ∧ p = Pat-Union(T, _)

**(T-Match-Union)**
Γ; R; L ⊢ e : TypeUnion([T_1, …, T_n])    ∀ i, arm_i = ⟨p_i, g_i, b_i⟩    Γ ⊢ p_i ◁ TypeUnion([T_1, …, T_n]) ⊣ B_i    Distinct(PatNames(p_i))    Γ_i = IntroAll(Γ, B_i)    (g_i ≠ ⊥ ⇒ Γ_i; R; L ⊢ g_i : TypePrim("bool"))    Γ_i; R; L ⊢ ArmBody(b_i) : T_i'    AllEq_Γ([T_1', …, T_k'])    (HasIrrefutableArm(arms, TypeUnion([T_1, …, T_n])) ∨ UnionTypesExhaustive(arms, [T_1, …, T_n]))
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ MatchExpr(e, arms) : T_1'

**(Match-Union-NonExhaustive)**
Γ; R; L ⊢ e : TypeUnion([T_1, …, T_n])    ¬(HasIrrefutableArm(arms, TypeUnion([T_1, …, T_n])) ∨ UnionTypesExhaustive(arms, [T_1, …, T_n]))    c = Code(Match-Union-NonExhaustive)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ MatchExpr(e, arms) ⇑ c

**(Chk-Match-Union)**
Γ; R; L ⊢ e : TypeUnion([T_1, …, T_n])    ∀ i, arm_i = ⟨p_i, g_i, b_i⟩    Γ ⊢ p_i ◁ TypeUnion([T_1, …, T_n]) ⊣ B_i    Distinct(PatNames(p_i))    Γ_i = IntroAll(Γ, B_i)    (g_i ≠ ⊥ ⇒ Γ_i; R; L ⊢ g_i : TypePrim("bool"))    Γ_i; R; L ⊢ ArmBody(b_i) ⇐ T    (HasIrrefutableArm(arms, TypeUnion([T_1, …, T_n])) ∨ UnionTypesExhaustive(arms, [T_1, …, T_n]))
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ MatchExpr(e, arms) ⇐ T ⊣ ∅

**Other Matches**

**(T-Match-Other)**
Γ; R; L ⊢ e : T    ∀ i, arm_i = ⟨p_i, g_i, b_i⟩    Γ ⊢ p_i ◁ T ⊣ B_i    Distinct(PatNames(p_i))    Γ_i = IntroAll(Γ, B_i)    (g_i ≠ ⊥ ⇒ Γ_i; R; L ⊢ g_i : TypePrim("bool"))    Γ_i; R; L ⊢ ArmBody(b_i) : T_i    AllEq_Γ([T_1, …, T_k])    HasIrrefutableArm(arms, T)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ MatchExpr(e, arms) : T_1

**(Chk-Match-Enum)**
Γ; R; L ⊢ e : TypePath(p)    EnumDecl(p) = E    ∀ i, arm_i = ⟨p_i, g_i, b_i⟩    Γ ⊢ p_i ◁ TypePath(p) ⊣ B_i    Distinct(PatNames(p_i))    Γ_i = IntroAll(Γ, B_i)    (g_i ≠ ⊥ ⇒ Γ_i; R; L ⊢ g_i : TypePrim("bool"))    Γ_i; R; L ⊢ ArmBody(b_i) ⇐ T    (HasIrrefutableArm(arms, TypePath(p)) ∨ ArmVariants(arms) = VariantNames(E))
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ MatchExpr(e, arms) ⇐ T ⊣ ∅

**(Chk-Match-Modal)**
Γ; R; L ⊢ e : TypePath(p)    Σ.Types[p] = `modal` M    ∀ i, arm_i = ⟨p_i, g_i, b_i⟩    Γ ⊢ p_i ◁ TypePath(p) ⊣ B_i    Distinct(PatNames(p_i))    Γ_i = IntroAll(Γ, B_i)    (g_i ≠ ⊥ ⇒ Γ_i; R; L ⊢ g_i : TypePrim("bool"))    Γ_i; R; L ⊢ ArmBody(b_i) ⇐ T    (HasIrrefutableArm(arms, TypePath(p)) ∨ ArmStates(arms) = StateNames(M))
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ MatchExpr(e, arms) ⇐ T ⊣ ∅

**(Chk-Match-Other)**
Γ; R; L ⊢ e : T_s    ∀ i, arm_i = ⟨p_i, g_i, b_i⟩    Γ ⊢ p_i ◁ T_s ⊣ B_i    Distinct(PatNames(p_i))    Γ_i = IntroAll(Γ, B_i)    (g_i ≠ ⊥ ⇒ Γ_i; R; L ⊢ g_i : TypePrim("bool"))    Γ_i; R; L ⊢ ArmBody(b_i) ⇐ T    HasIrrefutableArm(arms, T_s)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ MatchExpr(e, arms) ⇐ T ⊣ ∅

#### 5.2.14. Declaration Typing (Cursive0)

DeclJudg = {Γ ⊢ ProcedureDecl : ok, Γ ⊢ StaticDecl : ok, Γ ⊢ RecordDecl : ok, Γ ⊢ EnumDecl : ok, Γ ⊢ ModalDecl : ok, Γ ⊢ ClassDecl : ok}

**DeclTyping.**
DeclTyping(Ms) ⇓ ok ⇔ ∀ M ∈ Ms. Γ ⊢ DeclTypingMod(M) ⇓ ok
DeclTypingMod(M) ⇓ ok ⇔ ∀ it ∈ M.items. Γ ⊢ DeclTypingItem(M.path, it) ⇓ ok

ProvBindCheck(params, body) ⇓ ok ⇔ body = BlockExpr(stmts, tail_opt) ∧ ∃ vec{π}. |vec{π}| = |params| ∧ Γ; InitProvEnv(params, vec{π}, []) ⊢ BlockProv(stmts, tail_opt) ⇓ π

DeclTypingItem(m, UsingDecl(_)) ⇓ ok
DeclTypingItem(m, StaticDecl(_, _, _, _, _)) ⇓ ok ⇔ Γ ⊢ StaticDecl : ok
DeclTypingItem(m, TypeAliasDecl(_, name, _, _, _)) ⇓ ok ⇔ Γ ⊢ FullPath(m, name) : TypeAliasOk
DeclTypingItem(m, ProcedureDecl(_, _, params, _, body, _, _) = item) ⇓ ok ⇔ Γ ⊢ ProcedureDecl : ok ∧ ProcBindCheck(m, item) ⇓ ok ∧ ProvBindCheck(params, body) ⇓ ok
DeclTypingItem(m, R) ⇓ ok ⇔ R = RecordDecl(_, _, _, _, _, _) ∧ Γ ⊢ R record : ok ∧ ∀ md ∈ Methods(R). MethodBindCheck(m, TypePath(RecordPath(R)), md) ⇓ ok ∧ ProvBindCheck(MethodParamsDecl(TypePath(RecordPath(R)), md), md.body) ⇓ ok
DeclTypingItem(m, E) ⇓ ok ⇔ E = EnumDecl(_, _, _, _, _, _) ∧ Γ ⊢ E enum : ok
DeclTypingItem(m, M) ⇓ ok ⇔ M = ModalDecl(_, _, _, _, _, _) ∧ Γ ⊢ M modal : ok ∧ ∀ S ∈ States(M), ∀ md ∈ Methods(S). StateMethodBindCheck(m, M, S, md) ⇓ ok ∧ ProvBindCheck(StateMethodParams(M, S, md), md.body) ⇓ ok ∧ ∀ tr ∈ Transitions(S). TransitionBindCheck(m, M, S, tr) ⇓ ok ∧ ProvBindCheck(TransitionParams(M, S, tr), tr.body) ⇓ ok
DeclTypingItem(m, Cl) ⇓ ok ⇔ Cl = ClassDecl(_, _, _, _, _) ∧ Γ ⊢ Cl : ok ∧ ∀ md ∈ ClassMethods(Cl). (md.body_opt = ⊥ ∨ (ClassMethodBindCheck(m, Cl, md) ⇓ ok ∧ ProvBindCheck(ClassMethodParams(Cl, md), md.body_opt) ⇓ ok))

ParamBinds(params) = [⟨x, T⟩ | ⟨_, x, T⟩ ∈ params]
ProcReturn(ret_opt) =
  { TypePrim("()")   if ret_opt = ⊥
    ret_opt          otherwise }
ReturnAnnOk(ret_opt) ⇔ ret_opt ≠ ⊥

ExplicitReturn(BlockExpr(stmts, tail_opt)) ⇔ tail_opt = ⊥ ∧ stmts ≠ [] ∧ LastStmt(stmts) = ReturnStmt(_)

VisRank(`public`) = 3    VisRank(`internal`) = 2    VisRank(`private`) = 1    VisRank(`protected`) = 1
FieldVisOk(R) ⇔ ∀ f ∈ Fields(R). VisRank(f.vis) ≤ VisRank(R.vis)
StateMemberVisOk(M) ⇔ ∀ S ∈ States(M), ∀ m ∈ Payload(M, S) ∪ Methods(S) ∪ Transitions(S). VisRank(m.vis) ≤ VisRank(M.vis)

**(WF-ProcedureDecl)**
Distinct(ParamNames(params))    ReturnAnnOk(ret_opt)    R = ProcReturn(ret_opt)    ∀ ⟨_, x_i, T_i⟩ ∈ params, Γ ⊢ T_i wf    Γ_0 = PushScope(Γ)    IntroAll(Γ_0, ParamBinds(params)) ⇓ Γ_1    Γ_1; R; ⊥ ⊢ body : T_b    Γ ⊢ T_b <: R    (R ≠ TypePrim("()") ⇒ ExplicitReturn(body))
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ProcedureDecl : ok

**(WF-ProcedureDecl-MissingReturnType)**
item = ProcedureDecl(_, _, _, ret_opt, _, _, _)    ¬ ReturnAnnOk(ret_opt)    c = Code(WF-ProcedureDecl-MissingReturnType)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ item ⇑ c

**(WF-ProcBody-ExplicitReturn-Err)**
item = ProcedureDecl(_, _, _, ret_opt, body, _, _)    R = ProcReturn(ret_opt)    R ≠ TypePrim("()")    ¬ ExplicitReturn(body)    c = Code(WF-ProcBody-ExplicitReturn-Err)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ item ⇑ c

StaticVisOk(vis, mut) ⇔ ¬ (vis = `public` ∧ mut = `var`)

**(WF-StaticDecl)**
binding = ⟨pat, ty_opt, op, init, _⟩    StaticVisOk(vis, mut)    ty_opt = T_a    Γ; ⊥; ⊥ ⊢ init ⇐ T_a ⊣ ∅    Γ ⊢ pat ⇐ T_a ⊣ B    Distinct(PatNames(pat))
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ StaticDecl : ok

**(WF-StaticDecl-Ann-Mismatch)**
item = StaticDecl(vis, mut, ⟨pat, ty_opt, op, init, _⟩, _, _)    ty_opt = T_a    Γ; ⊥; ⊥ ⊢ init ⇒ T_i ⊣ C    Solve(C) ⇓ θ    ¬(Γ ⊢ θ(T_i) <: T_a)    c = Code(WF-StaticDecl-Ann-Mismatch)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ item ⇑ c

**(WF-StaticDecl-MissingType)**
item = StaticDecl(_, _, ⟨pat, ty_opt, op, init, _⟩, _, _)    ty_opt = ⊥    c = Code(WF-StaticDecl-MissingType)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ item ⇑ c

**(StaticVisOk-Err)**
item = StaticDecl(vis, mut, _, _, _)    ¬ StaticVisOk(vis, mut)    c = Code(StaticVisOk-Err)
────────────────────────────────────────────────────────────────
Γ ⊢ item ⇑ c

**(WF-RecordDecl)**
∀ f ∈ Fields(R), Γ ⊢ f.type wf    FieldVisOk(R)    Γ ⊢ R record wf    Γ ⊢ Methods(R) : ok    Γ ⊢ TypePath(RecordPath(R)) : ImplementsOk
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ R record : ok

**(FieldVisOk-Err)**
R = RecordDecl(_, _, _, _, _, _)    ¬ FieldVisOk(R)    c = Code(FieldVisOk-Err)
────────────────────────────────────────────────────────────────
Γ ⊢ R ⇑ c

EnumPathOf(E) = FullPath(ModuleOf(E), E.name)

**Enum Variant Visibility.**

VariantVis(E, v) = Vis(E)
VariantVisible(m, E, v) ⇔ Γ ⊢ CanAccess(m, E) ⇓ ok

**(WF-EnumDecl)**
variants ≠ []    Distinct([v.name | v ∈ variants])    ∀ v ∈ variants, Γ ⊢ v.payload_opt wf    EnumDiscriminants(E) ⇓ _    Γ ⊢ TypePath(EnumPathOf(E)) : ImplementsOk
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ E enum : ok

PayloadOptWF(⊥)
PayloadOptWF(TuplePayload([T_1, …, T_n])) ⇔ ∀ i. Γ ⊢ T_i wf
PayloadOptWF(RecordPayload([⟨f_1, T_1⟩, …, ⟨f_k, T_k⟩])) ⇔ Distinct([f_1, …, f_k]) ∧ ∀ i. Γ ⊢ T_i wf

Γ ⊢ payload_opt wf ⇔ PayloadOptWF(payload_opt)

ModalPath(M) = FullPath(ModuleOf(M), M.name)

**(WF-ModalDecl)**
p = ModalPath(M)    Γ ⊢ `modal` M wf    StateMemberVisOk(M)    Γ ⊢ TypePath(p) : ImplementsOk    ∀ S ∈ States(M), ∀ md ∈ Methods(S), Γ ⊢ md : StateMethodOK(M, S)    Γ ⊢ md : StateMethodBodyOK(p, S)    ∀ tr ∈ Transitions(S), Γ ⊢ tr : TransitionOK(M, S)    Γ ⊢ tr : TransitionBodyOK(p, S)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ M modal : ok

**(StateMemberVisOk-Err)**
M = ModalDecl(_, _, _, _, _, _)    ¬ StateMemberVisOk(M)    c = Code(StateMemberVisOk-Err)
────────────────────────────────────────────────────────────────
Γ ⊢ M ⇑ c

**(WF-ClassDecl)**
Γ ⊢ Cl : ClassOk
──────────────────────────────────────────────
Γ ⊢ Cl : ok

**Program Entry Point (Cursive0).**

MainDecls(P) = [ d | m ∈ P.modules, d ∈ ASTModule(P, m).items, d = ProcedureDecl(vis, name, params, ret_opt, body, span, doc), name = `main` ]

TypeParams(ProcedureDecl(_)) = []
MainGeneric(d) ⇔ TypeParams(d) ≠ []

MainSigOk(d) ⇔ d = ProcedureDecl(vis, `main`, params, ret_opt, _, _, _) ∧ vis = `public` ∧ params = [⟨mode, name, ty⟩] ∧ mode ∈ {⊥, `move`} ∧ ty = TypePath([`Context`]) ∧ BuiltInContext(TypePath([`Context`])) ∧ ret_opt = TypePrim("i32")
MainConsumesContext(d) ⇔ MainSigOk(d) ∧ ∃ mode. d.params = [⟨mode, _, _⟩] ∧ mode = `move`
MainRetainsContext(d) ⇔ MainSigOk(d) ∧ ∃ mode. d.params = [⟨mode, _, _⟩] ∧ mode = ⊥

MainCheck : Project ⇀ ok

**(Main-Ok)**
Executable(P)    MainDecls(P) = [d]    ¬ MainGeneric(d)    MainSigOk(d)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ MainCheck(P) ⇓ ok

**(Main-Bypass-Lib)**
¬ Executable(P)
──────────────────────────────────────────────
Γ ⊢ MainCheck(P) ⇓ ok

**(Main-Multiple)**
Executable(P)    |MainDecls(P)| > 1    c = Code(Main-Multiple)
────────────────────────────────────────────────────────────────
Γ ⊢ MainCheck(P) ⇑ c

**(Main-Generic-Err)**
Executable(P)    MainDecls(P) = [d]    MainGeneric(d)    c = Code(Main-Generic-Err)
────────────────────────────────────────────────────────────────
Γ ⊢ MainCheck(P) ⇑ c

**(Main-Signature-Err)**
Executable(P)    MainDecls(P) = [d]    ¬ MainSigOk(d)    c = Code(Main-Signature-Err)
────────────────────────────────────────────────────────────────
Γ ⊢ MainCheck(P) ⇑ c

**(Main-Missing)**
Executable(P)    MainDecls(P) = []    c = Code(Main-Missing)
────────────────────────────────────────────────────────────────
Γ ⊢ MainCheck(P) ⇑ c

MainDiagRefs = {"8.2"}

Phase3Checks(P, Ms) = [Γ ⊢ ResolveModules(P) ⇓ Ms, Γ ⊢ DeclTyping(Ms) ⇓ ok, Γ ⊢ MainCheck(P) ⇓ ok]
Phase3Order(P) ⇔ ∃ Ms. FirstFail(Phase3Checks(P, Ms)) = ⊥

#### 5.2.15. Binding and Permission State (Cursive0)

**Binding State.**

BindingState ::= Valid | Moved | PartiallyMoved(F)    (F ⊆ Name)

**Binding Metadata.**

Movability ::= mov | immov
Responsibility ::= resp | alias
Mutability = {`let`, `var`}
BindInfo ::= ⟨state, mov, mut, resp⟩    (mut ∈ Mutability)

**Binding Environment.**

BindScope = Map(Identifier, BindInfo)
𝔅 = [BindScope]

PushScope_B(𝔅) = [∅] ++ 𝔅
PopScope_B([_] ++ 𝔅) = 𝔅

Lookup_B([σ] ++ 𝔅', x) =
  { σ[x]                if x ∈ dom(σ)
    Lookup_B(𝔅', x)     otherwise }
Lookup_B([], x) = ⊥

Update_B([σ] ++ 𝔅', x, info) =
  { [σ[x ↦ info]] ++ 𝔅'            if x ∈ dom(σ)
    [σ] ++ Update_B(𝔅', x, info)   otherwise }
Update_B([], x, info) = ⊥

Intro_B([σ] ++ 𝔅', x, info) = [σ[x ↦ info]] ++ 𝔅'
ShadowIntro_B(𝔅, x, info) = Update_B(𝔅, x, info)

**Permission of a Type.**

PermOf(TypePerm(p, T)) = p
PermOf(T) = `const`    if T ≠ TypePerm(_, _)

**Permission State.**

ActiveState ::= Active | Inactive

PermKey = Identifier × FieldPath
PermScope = Map(PermKey, ActiveState)
Π = [PermScope]

PushScope_Π(Π) = [∅] ++ Π
PopScope_Π([_] ++ Π) = Π

Lookup_Π([σ] ++ Π', k) =
  { Inactive             if k ∈ dom(σ) ∧ σ[k] = Inactive
    Lookup_Π(Π', k)      otherwise }
Lookup_Π([], k) = Active

Update_Π([σ] ++ Π', k, s) = [σ[k ↦ s]] ++ Π'

**Join at Control-Flow Merge.**

JoinState(Moved, s) = Moved
JoinState(s, Moved) = Moved
JoinState(PartiallyMoved(F_1), PartiallyMoved(F_2)) = PartiallyMoved(F_1 ∪ F_2)
JoinState(Valid, PartiallyMoved(F)) = PartiallyMoved(F)
JoinState(PartiallyMoved(F), Valid) = PartiallyMoved(F)
JoinState(Valid, Valid) = Valid

JoinBindInfo(⟨s_1, mv_1, mut_1, resp_1⟩, ⟨s_2, mv_2, mut_2, resp_2⟩) =
  { ⟨JoinState(s_1, s_2), mv_1, mut_1, resp_1⟩   if mv_1 = mv_2 ∧ mut_1 = mut_2 ∧ resp_1 = resp_2
    ⊥                                            otherwise }

JoinScope_B(B_1, B_2) =
  { { x ↦ JoinBindInfo(B_1[x], B_2[x]) | x ∈ dom(B_1) }    if dom(B_1) = dom(B_2) ∧ ∀ x ∈ dom(B_1). JoinBindInfo(B_1[x], B_2[x]) ≠ ⊥
    ⊥                                                      otherwise }

Join_B([], []) = []
Join_B(B_1 :: 𝔅_1, B_2 :: 𝔅_2) =
  { JoinScope_B(B_1, B_2) :: Join_B(𝔅_1, 𝔅_2)    if JoinScope_B(B_1, B_2) ≠ ⊥ ∧ Join_B(𝔅_1, 𝔅_2) ≠ ⊥
    ⊥                                           otherwise }
Join_B(𝔅_1, 𝔅_2) = ⊥    if |𝔅_1| ≠ |𝔅_2|

JoinPermState(Active, Active) = Active
JoinPermState(_, _) = Inactive    otherwise

PermAt(B, x) =
  { B[x]     if x ∈ dom(B)
    Active   otherwise }

JoinScope_Π(B_1, B_2) = { x ↦ JoinPermState(PermAt(B_1, x), PermAt(B_2, x)) | x ∈ dom(B_1) ∪ dom(B_2) }

JoinPerm([], []) = []
JoinPerm(B_1 :: Π_1, B_2 :: Π_2) =
  { JoinScope_Π(B_1, B_2) :: JoinPerm(Π_1, Π_2)    if JoinScope_Π(B_1, B_2) ≠ ⊥ ∧ JoinPerm(Π_1, Π_2) ≠ ⊥
    ⊥                                             otherwise }
JoinPerm(Π_1, Π_2) = ⊥    if |Π_1| ≠ |Π_2|

**Place Roots and Field Heads.**

FieldHead(Identifier(x)) = ⊥
FieldHead(FieldAccess(p, f)) =
  { f                if FieldHead(p) = ⊥
    FieldHead(p)     otherwise }
FieldHead(TupleAccess(p, _)) = FieldHead(p)
FieldHead(IndexAccess(p, _)) = FieldHead(p)
FieldHead(Deref(p)) = ⊥

**Access Legality.**

AccessStateOk(Valid, p) = true
AccessStateOk(PartiallyMoved(F), p) = (FieldHead(p) = f ∧ f ∉ F)
AccessStateOk(Moved, p) = false

AccessOk_B(𝔅, p) ⇔ x = PlaceRoot(p) ∧ Lookup_B(𝔅, x) = ⟨s, _, _, _⟩ ∧ AccessStateOk(s, p)

AccessOk_Π(Π, p) ⇔ (PermOf(ExprType(p)) ≠ `unique`) ∨ AccessPathOk(Π, p)

AccessOk(𝔅, Π, p) ⇔ AccessOk_B(𝔅, p) ∧ AccessOk_Π(Π, p)

**Binding Introduction.**

MovOf("=") = mov
MovOf(":=") = immov

IsMoveExpr(MoveExpr(_)) = true
IsMoveExpr(_) = false    otherwise

RespOfInit(init) =
  { resp    if ¬ IsPlace(init)
    resp    if IsMoveExpr(init)
    alias   otherwise }

**Temporary Lifetime.**

InitExpr(⟨_, _, _, init, _⟩) = init

BindInitScope(e) = BindScope(s) ⇔
  (s = LetStmt(binding) ∧ InitExpr(binding) = e) ∨
  (s = VarStmt(binding) ∧ InitExpr(binding) = e) ∨
  (s = ShadowLetStmt(_, _, e)) ∨
  (s = ShadowVarStmt(_, _, e))

TempScope(e) =
  { BindInitScope(e)                  if BindInitScope(e) ≠ ⊥
    StmtScope(EnclosingStmt(e))       otherwise }

TempValue(e) ⇔ ¬ IsPlace(e)

TempOrderList([]) = []
TempOrderList([e] ++ es) = TempOrder(e) ++ TempOrderList(es)

TempOrder(e) =
  { TempOrderList(Children_LTR(e)) ++ [e]    if TempValue(e)
    TempOrderList(Children_LTR(e))          otherwise }

TempOrderStmt(s) = TempOrderList(StmtExprs(s))

ControlExpr(ReturnStmt(e)) = e    ControlExpr(ResultStmt(e)) = e    ControlExpr(BreakStmt(e)) = e
ControlExpr(s) = ⊥    if s ∉ {ReturnStmt(_), ResultStmt(_), BreakStmt(_)}

TempStmtList(s) = [ e ∈ TempOrderStmt(s) | TempScope(e) = StmtScope(s) ∧ e ≠ ControlExpr(s) ]
TempDropOrder(s) = Rev(TempStmtList(s))

**Judgments.**

BJudgment = {Γ; 𝔅; Π ⊢ e ⇒ 𝔅' ▷ Π', Γ; 𝔅; Π ⊢ s ⇒ 𝔅' ▷ Π'}

ExprType(e) = T ⇔ Γ; R; L ⊢ e : T
ExprType(p) = T ⇔ IsPlace(p) ∧ Γ; R; L ⊢ p :place T

BindType(⟨pat, ty_opt, op, init, _⟩) = T ⇔ ty_opt = T
BindType(⟨pat, ⊥, op, init, _⟩) = θ(T_i) ⇔ Γ; R; L ⊢ init ⇒ T_i ⊣ C ∧ Solve(C) ⇓ θ
BindType(ShadowLetStmt(_, ty_opt, init)) = T ⇔ ty_opt = T
BindType(ShadowLetStmt(_, ⊥, init)) = θ(T_i) ⇔ Γ; R; L ⊢ init ⇒ T_i ⊣ C ∧ Solve(C) ⇓ θ
BindType(ShadowVarStmt(_, ty_opt, init)) = T ⇔ ty_opt = T
BindType(ShadowVarStmt(_, ⊥, init)) = θ(T_i) ⇔ Γ; R; L ⊢ init ⇒ T_i ⊣ C ∧ Solve(C) ⇓ θ

MapUnion(M_1, M_2) = { x ↦ (M_2[x] if x ∈ dom(M_2) else M_1[x]) | x ∈ dom(M_1) ∪ dom(M_2) }

IntroAll_B([σ] ++ 𝔅', B) = [MapUnion(σ, B)] ++ 𝔅'

ShadowAll_B(𝔅, B) = ShadowAll_B(𝔅, Entries(B))
ShadowAll_B(𝔅, []) = 𝔅
ShadowAll_B(𝔅, [⟨x, info⟩] ++ xs) = ShadowAll_B(ShadowIntro_B(𝔅, x, info), xs)

BindInfoMap(f, B, mv, mut) = { x ↦ ⟨Valid, MovEff(mv, f(B[x])), mut, f(B[x])⟩ | x ∈ dom(B) }

MovEff(mv, resp) = mv    MovEff(mv, alias) = immov

T_Region = TypeModalState([`Region`], `Active`)
RegionBindName(Γ, alias_opt) =
  { alias_opt         if alias_opt ≠ ⊥
    FreshRegion(Γ)    otherwise }
RegionBindMap(Γ, alias_opt) = { r ↦ T_Region | r = RegionBindName(Γ, alias_opt) }
RegionBindInfo(Γ, alias_opt) = BindInfoMap(λ U. resp, RegionBindMap(Γ, alias_opt), mov, `let`)
FrameBindInfo(Γ) = RegionBindInfo(Γ, ⊥)

Names(B) = dom(B)

JoinAll_B([]) = ⊥
JoinAll_B([𝔅]) = 𝔅
JoinAll_B(𝔅_1 :: 𝔅_2 :: rest) = JoinAll_B([Join_B(𝔅_1, 𝔅_2)] ++ rest)

JoinAllPerm([]) = ⊥
JoinAllPerm([Π]) = Π
JoinAllPerm(Π_1 :: Π_2 :: rest) = JoinAllPerm([JoinPerm(Π_1, Π_2)] ++ rest)

Top([σ] ++ Π') = σ

SetTop([σ] ++ Π', σ') = [σ'] ++ Π'

InactivateScope(σ, K) = { x ↦ (Inactive if x ∈ K else σ[x]) | x ∈ dom(σ) ∪ K }

Roots(Π_2, Π_1) = { k | Top(Π_2)[k] = Inactive ∧ Lookup_Π(Π_1, k) = Active }

ConsumeOnMove(𝔅, e) =
  { Update_B(𝔅, x, ⟨Moved, mv, mut, resp⟩)    if IsMoveExpr(e) ∧ x = PlaceRoot(MoveInner(e)) ∧ Lookup_B(𝔅, x) = ⟨s, mv, mut, resp⟩
    𝔅                                         otherwise }

MoveInner(MoveExpr(p)) = p

OptList(⊥) = []
OptList(e) = [e]    if e ≠ ⊥

StmtExprs(LetStmt(⟨_, _, _, init, _⟩)) = [init]
StmtExprs(VarStmt(⟨_, _, _, init, _⟩)) = [init]
StmtExprs(ShadowLetStmt(_, _, init)) = [init]
StmtExprs(ShadowVarStmt(_, _, init)) = [init]
StmtExprs(AssignStmt(p, e)) = [e, p]
StmtExprs(CompoundAssignStmt(p, _, e)) = [p, e]
StmtExprs(ExprStmt(e)) = [e]
StmtExprs(ReturnStmt(e_opt)) = OptList(e_opt)
StmtExprs(ResultStmt(e)) = [e]
StmtExprs(BreakStmt(e_opt)) = OptList(e_opt)
StmtExprs(ContinueStmt) = []
StmtExprs(DeferStmt(_)) = []
StmtExprs(UnsafeBlockStmt(b)) = [b]
StmtExprs(RegionStmt(opts_opt, _, b)) = [RegionOptsExpr(opts_opt), b]
StmtExprs(FrameStmt(_, b)) = [b]
StmtExprs(ErrorStmt(_)) = []

StmtScope(s) = s
BindScope(s) = BlockOfStmt(s)
EnclosingStmt(e) = s ⇔ e ∈ SubExprs(s) ∧ ∀ s' ∈ SubStmts(s). e ∉ SubExprs(s')
BlockOfStmt(s) = b ⇔ s ∈ BlockStmts(b) ∧ ∀ b' ∈ SubBlocks(b). s ∉ BlockStmts(b')

BlockStmts(BlockExpr(stmts, _)) = stmts

StmtBlocks(UnsafeBlockStmt(b)) = [b]
StmtBlocks(DeferStmt(b)) = [b]
StmtBlocks(RegionStmt(_, _, b)) = [b]
StmtBlocks(FrameStmt(_, b)) = [b]
StmtBlocks(s) = []    if s ∉ {UnsafeBlockStmt(_), DeferStmt(_), RegionStmt(_, _, _), FrameStmt(_, _)}

SubExprs(s) = SubExprsList(StmtExprs(s))
SubExprsList([]) = ∅
SubExprsList([e] ++ es) = {e} ∪ SubExprsList(Children_LTR(e)) ∪ SubExprsList(es)

SubStmts(s) = SubStmtsList(StmtBlocks(s))
SubStmtsList([]) = ∅
SubStmtsList([b] ++ bs) = BlockStmts(b) ∪ SubStmtsSeq(BlockStmts(b)) ∪ SubStmtsList(bs)
SubStmtsSeq([]) = ∅
SubStmtsSeq([s] ++ ss) = SubStmts(s) ∪ SubStmtsSeq(ss)

SubBlocks(b) = SubBlocksSeq(BlockStmts(b))
SubBlocksSeq([]) = ∅
SubBlocksSeq([s] ++ ss) = StmtBlocks(s) ∪ (⋃_{b' ∈ StmtBlocks(s)} SubBlocks(b')) ∪ SubBlocksSeq(ss)

Entries(B) = [⟨x_1, B[x_1]⟩, …, ⟨x_n, B[x_n]⟩] ⇔ [x_1, …, x_n] enumerates dom(B) without repetition

SynthParams([⟨m_1, T_1⟩, …, ⟨m_n, T_n⟩]) = [⟨m_1, ⊥, T_1⟩, …, ⟨m_n, ⊥, T_n⟩]

CalleeProc(Identifier(x)) = proc ⇔ Γ ⊢ ResolveValueName(x) ⇓ ent ∧ ent.origin_opt = mp ∧ name = (ent.target_opt if present, else x) ∧ DeclOf(mp, name) = proc ∧ proc = ProcedureDecl(_)
CalleeProc(Path(path, name)) = proc ⇔ Γ ⊢ ResolveQualified(path, name, ValueKind) ⇓ ent ∧ ent.origin_opt = mp ∧ name' = (ent.target_opt if present, else name) ∧ DeclOf(mp, name') = proc ∧ proc = ProcedureDecl(_)

Params(Call(callee, args)) =
  { proc.params            if CalleeProc(callee) = proc
    SynthParams(params)    if ExprType(callee) = TypeFunc(params, _)
    ⊥                      otherwise }

MethodOf(base, name) = m ⇔ LookupMethod(StripPerm(ExprType(base)), name) = m
RecvBase(base, name) = T ⇔ MethodOf(base, name) = m ∧ T = StripPerm(ExprType(base))

RecvParams(base, name) = [⟨RecvMode(m.receiver), `self`, RecvType(T, m.receiver)⟩] ++ m.params ⇔ MethodOf(base, name) = m ∧ RecvBase(base, name) = T

**Static Binding Maps (Module Scope).**

StaticBindTypesMod(P, m) = ++_{item ∈ ASTModule(P, m).items, item = StaticDecl(_, _, binding, _, _)} StaticBindTypes(binding)

StaticBindInfo(item) = BindInfoMap(λ U. RespOfInit(init), StaticBindTypes(binding), MovOf(op), mut) ⇔ item = StaticDecl(_, mut, binding, _, _) ∧ binding = ⟨_, _, op, init, _⟩

StaticBindMap(P, m) = ++_{item ∈ ASTModule(P, m).items, item = StaticDecl(_, _, _, _, _)} StaticBindInfo(item)

**Procedure Entry.**

𝔅_global = IntroAll_B(PushScope_B(𝔅), StaticBindMap(Project(Γ), m))
𝔅_proc = IntroAll_B(PushScope_B(𝔅_global), ParamBindMap(params))

ParamBindMap([]) = ∅
ParamBindMap([⟨mode, x, T⟩] ++ ps) = MapUnion(ParamBindMap(ps), { x ↦ ⟨Valid, ParamMov(mode), `let`, ParamResp(mode)⟩ })
MethodParamBindMap(base, name) = ParamBindMap(RecvParams(base, name))

ParamTypeMap([]) = ∅
ParamTypeMap([⟨mode, x, T⟩] ++ ps) = MapUnion(ParamTypeMap(ps), { x ↦ T })

ParamMov(`move`) = mov    ParamMov(⊥) = immov
ParamResp(`move`) = resp    ParamResp(⊥) = alias

B_params = ParamTypeMap(params)
B_static = StaticBindTypesMod(Project(Γ), m)
Π_global = [{ x ↦ Active | (x:T) ∈ B_static ∧ PermOf(T) = `unique` }]
Π_proc = [{ x ↦ Active | (x:T) ∈ B_params ∧ PermOf(T) = `unique` }] ++ Π_global

**Expression Rules (Selected).**

**(B-Place)**
IsPlace(p)    AccessOk(𝔅, Π, p)
──────────────────────────────────────────────
Γ; 𝔅; Π ⊢ p ⇒ 𝔅 ▷ Π

**(B-Place-Unique-Err)**
IsPlace(p)    Γ; R; L ⊢ p : T_p    PermOf(T_p) = `unique`    ¬ AccessPathOk(Π, p)    c = Code(B-Place-Unique-Err)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ p ⇑ c

**(B-Place-Moved-Err)**
IsPlace(p)    Γ; R; L ⊢ p : T_p    x = PlaceRoot(p)    Lookup_B(𝔅, x) = ⟨s, _, _, _⟩    (s = Moved ∨ (s = PartiallyMoved(F) ∧ (FieldHead(p) = ⊥ ∨ FieldHead(p) ∈ F)))    (PermOf(T_p) ≠ `unique` ∨ AccessPathOk(Π, p))    c = Code(B-Place-Moved-Err)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ p ⇑ c

**(B-Move-Whole)**
IsPlace(p)    x = PlaceRoot(p)    FieldHead(p) = ⊥    Lookup_B(𝔅, x) = ⟨Valid, mv, m, r⟩    mv = mov
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ MoveExpr(p) ⇒ Update_B(𝔅, x, ⟨Moved, mov, m, r⟩) ▷ Π

**(B-Move-Field)**
IsPlace(p)    x = PlaceRoot(p)    FieldHead(p) = f    Γ; R; L ⊢ p : T_p    PermOf(T_p) = `unique`    Lookup_B(𝔅, x) = ⟨s, mv, m, r⟩    mv = mov
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ MoveExpr(p) ⇒ Update_B(𝔅, x, ⟨PM(s, f), mov, m, r⟩) ▷ Π

PM(Valid, f) = PartiallyMoved({f})
PM(PartiallyMoved(F), f) = PartiallyMoved(F ∪ {f})
PM(Moved, f) = ⊥

**(B-Move-Whole-Moved-Err)**
IsPlace(p)    FieldHead(p) = ⊥    x = PlaceRoot(p)    Lookup_B(𝔅, x) = ⟨s, mv, _, _⟩    s ≠ Valid    mv = mov    c = Code(B-Move-Whole-Moved-Err)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ MoveExpr(p) ⇑ c

**(B-Move-Field-Moved-Err)**
IsPlace(p)    FieldHead(p) = f    x = PlaceRoot(p)    Γ; R; L ⊢ p : T_p    PermOf(T_p) = `unique`    Lookup_B(𝔅, x) = ⟨s, mv, _, _⟩    (s = Moved ∨ (s = PartiallyMoved(F) ∧ f ∈ F))    mv = mov    c = Code(B-Move-Field-Moved-Err)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ MoveExpr(p) ⇑ c

**(B-Move-Field-NonUnique-Err)**
IsPlace(p)    FieldHead(p) = f    x = PlaceRoot(p)    Γ; R; L ⊢ p : T_p    PermOf(T_p) ≠ `unique`    Lookup_B(𝔅, x) = ⟨_, mv, _, _⟩    mv = mov    c = Code(B-Move-Field-NonUnique-Err)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ MoveExpr(p) ⇑ c

**(B-Move-Whole-Immovable-Err)**
IsPlace(p)    FieldHead(p) = ⊥    x = PlaceRoot(p)    Lookup_B(𝔅, x) = ⟨_, mv, _, _⟩    mv = immov    c = Code(B-Move-Whole-Immovable-Err)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ MoveExpr(p) ⇑ c

**(B-Move-Field-Immovable-Err)**
IsPlace(p)    FieldHead(p) = f    x = PlaceRoot(p)    Lookup_B(𝔅, x) = ⟨_, mv, _, _⟩    mv = immov    c = Code(B-Move-Field-Immovable-Err)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ MoveExpr(p) ⇑ c

BExprRules = {B-Place, B-Move-Whole, B-Move-Field, B-Call, B-MethodCall, B-Transition, B-Expr-Sub}

NoSpecificBExpr(e) ⇔ ¬ ∃ r ∈ BExprRules \ {B-Expr-Sub}. PremisesHold(r, e)

**(B-Expr-Sub)**
NoSpecificBExpr(e)    Children_LTR(e) = [e_1, …, e_n]    Γ; 𝔅_0; Π_0 ⊢ e_1 ⇒ 𝔅_1 ▷ Π_1  …  Γ; 𝔅_{n-1}; Π_{n-1} ⊢ e_n ⇒ 𝔅_n ▷ Π_n
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; 𝔅_0; Π_0 ⊢ e ⇒ 𝔅_n ▷ Π_n

**Call and Method Argument Passing.**

**Field-Path Permission Tracking.**

FieldPath = [Name]

FieldPathOf(Identifier(x)) = []
FieldPathOf(FieldAccess(p, f)) = FieldPathOf(p) ++ [f]
FieldPathOf(TupleAccess(p, _)) = FieldPathOf(p)
FieldPathOf(IndexAccess(p, _)) = FieldPathOf(p)
FieldPathOf(Deref(p)) = []

PlacePath(p) =
  { (PlaceRoot(p), [])            if p = Identifier(x)
    (PlaceRoot(p), FieldPathOf(p))    otherwise }

Prefixes([]) = [[]]
Prefixes([f] ++ fs) = [[]] ∪ { [f] ++ p | p ∈ Prefixes(fs) }
AncPaths(p) = { (PlaceRoot(p), fp) | fp ∈ Prefixes(FieldPathOf(p)) }

AccessPathOk(Π, p) ⇔ ∀ k ∈ AncPaths(p). Lookup_Π(Π, k) = Active

DowngradeUniquePath(Π, mode, p) =
  { SetTop(Π, InactivateScope(Top(Π), AncPaths(p)))    if mode = ⊥ ∧ IsPlace(p) ∧ PermOf(ExprType(p)) = `unique`
    Π                                                 otherwise }

DowngradeUnique(Π, mode, e) =
  { DowngradeUniquePath(Π, mode, e)    if IsPlace(e)
    Π                                 otherwise }

DowngradeUniqueBind(Π, init, T_b) =
  { DowngradeUniquePath(Π, ⊥, init)    if IsPlace(init) ∧ PermOf(ExprType(init)) = `unique` ∧ PermOf(T_b) = `const`
    Π                                  otherwise }

RemoveKeys(σ, D) = { k ↦ σ[k] | k ∈ dom(σ) ∧ k ∉ D }
Reactivate([σ] ++ Π', D) = [RemoveKeys(σ, D)] ++ Π'

ArgPassExpr(mode, moved, e) =
  { MovedArg(moved, e)    if mode = `move` ∧ moved = true
    e                     otherwise }

ArgPassJudg = {Γ; 𝔅; Π ⊢ ArgPass(params, args) ⇒ 𝔅' ▷ Π', D}

**(B-ArgPass-Empty)**
────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ ArgPass([], []) ⇒ 𝔅 ▷ Π, ∅

**(B-ArgPass-Move-Missing)**
params = [⟨`move`, _, T_p⟩] ++ ps    args = [⟨moved, e, _⟩] ++ as    moved = false    c = Code(B-ArgPass-Move-Missing)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ ArgPass(params, args) ⇑ c

**(B-ArgPass-Cons)**
Γ; 𝔅; Π ⊢ ArgPassExpr(mode, moved, e) ⇒ 𝔅_1 ▷ Π_1    (mode = ⊥ ⇒ IsPlace(e))    Π_2 = DowngradeUnique(Π_1, mode, e)    Γ; 𝔅_1; Π_2 ⊢ ArgPass(ps, as) ⇒ 𝔅_2 ▷ Π_3, D
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ ArgPass([⟨mode, _, T_p⟩] ++ ps, [⟨moved, e, _⟩] ++ as) ⇒ 𝔅_2 ▷ Π_3, D ∪ Roots(Π_2, Π_1)

**(B-Call)**
Γ; 𝔅; Π ⊢ f ⇒ 𝔅_1 ▷ Π_1    Γ; 𝔅_1; Π_1 ⊢ ArgPass(Params(Call(f, args)), args) ⇒ 𝔅_2 ▷ Π_2, D    Π_3 = Reactivate(Π_2, D)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ Call(f, args) ⇒ 𝔅_2 ▷ Π_3

**(B-MethodCall)**
Γ; 𝔅; Π ⊢ base ⇒ 𝔅_1 ▷ Π_1    Γ; 𝔅_1; Π_1 ⊢ ArgPass(RecvParams(base, name), args) ⇒ 𝔅_2 ▷ Π_2, D    Π_3 = Reactivate(Π_2, D)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ MethodCall(base, name, args) ⇒ 𝔅_2 ▷ Π_3

**(B-Transition)**
Γ; 𝔅; Π ⊢ e_self ⇒ 𝔅_0 ▷ Π_0    x = PlaceRoot(e_self)    Lookup_B(𝔅_0, x) = ⟨Valid, mov, m, r⟩    mov = mov    𝔅_1 = Update_B(𝔅_0, x, ⟨Moved, mov, m, r⟩)    Γ; 𝔅_1; Π_0 ⊢ ArgPass(tr.params, args) ⇒ 𝔅_2 ▷ Π_1, D
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ e_self ↠ t(args) ⇒ 𝔅_2 ▷ Reactivate(Π_1, D)

**Statement Rules (Selected).**

**(B-Seq-Empty)**
──────────────────────────────────────────────
Γ; 𝔅; Π ⊢ [] ⇒ 𝔅 ▷ Π

**(B-Seq-Cons)**
Γ; 𝔅; Π ⊢ s ⇒ 𝔅_1 ▷ Π_1    Γ; 𝔅_1; Π_1 ⊢ ss ⇒ 𝔅_2 ▷ Π_2
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ s :: ss ⇒ 𝔅_2 ▷ Π_2

**(B-LetVar-UniqueNonMove-Err)**
T_b = BindType(⟨pat, ty_opt, op, init, _⟩)    PermOf(T_b) = `unique`    IsPlace(init)    ¬ IsMoveExpr(init)    c = Code(B-LetVar-UniqueNonMove-Err)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ LetOrVarStmt(⟨pat, ty_opt, op, init, _⟩) ⇑ c

**(B-LetVar)**
Γ; 𝔅; Π ⊢ init ⇒ 𝔅_1 ▷ Π_1    T_b = BindType(⟨pat, ty_opt, op, init, _⟩)    Π_2 = DowngradeUniqueBind(Π_1, init, T_b)    𝔅_2 = ConsumeOnMove(𝔅_1, init)    Γ ⊢ pat ⇐ T_b ⊣ B    𝔅_3 = IntroAll_B(𝔅_2, BindInfoMap(λ U. RespOfInit(init), B, MovOf(op), mut))
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ LetOrVarStmt(⟨pat, ty_opt, op, init, _⟩) ⇒ 𝔅_3 ▷ Π_2

**(B-ShadowLet-UniqueNonMove-Err)**
T_b = BindType(ShadowLetStmt(x, ty_opt, init))    PermOf(T_b) = `unique`    IsPlace(init)    ¬ IsMoveExpr(init)    c = Code(B-ShadowLet-UniqueNonMove-Err)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ ShadowLetStmt(x, ty_opt, init) ⇑ c

**(B-ShadowLet)**
Γ; 𝔅; Π ⊢ init ⇒ 𝔅_1 ▷ Π_1    T_b = BindType(ShadowLetStmt(x, ty_opt, init))    Π_2 = DowngradeUniqueBind(Π_1, init, T_b)    𝔅_2 = ConsumeOnMove(𝔅_1, init)    B = { x ↦ T_b }    𝔅_3 = ShadowAll_B(𝔅_2, BindInfoMap(λ U. RespOfInit(init), B, MovOf("="), `let`))
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ ShadowLetStmt(x, ty_opt, init) ⇒ 𝔅_3 ▷ Π_2

**(B-ShadowVar-UniqueNonMove-Err)**
T_b = BindType(ShadowVarStmt(x, ty_opt, init))    PermOf(T_b) = `unique`    IsPlace(init)    ¬ IsMoveExpr(init)    c = Code(B-ShadowVar-UniqueNonMove-Err)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ ShadowVarStmt(x, ty_opt, init) ⇑ c

**(B-ShadowVar)**
Γ; 𝔅; Π ⊢ init ⇒ 𝔅_1 ▷ Π_1    T_b = BindType(ShadowVarStmt(x, ty_opt, init))    Π_2 = DowngradeUniqueBind(Π_1, init, T_b)    𝔅_2 = ConsumeOnMove(𝔅_1, init)    B = { x ↦ T_b }    𝔅_3 = ShadowAll_B(𝔅_2, BindInfoMap(λ U. RespOfInit(init), B, MovOf("="), `var`))
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ ShadowVarStmt(x, ty_opt, init) ⇒ 𝔅_3 ▷ Π_2

**(B-ExprStmt)**
Γ; 𝔅; Π ⊢ e ⇒ 𝔅_1 ▷ Π_1
────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ ExprStmt(e) ⇒ 𝔅_1 ▷ Π_1

**(B-ResultStmt)**
Γ; 𝔅; Π ⊢ e ⇒ 𝔅_1 ▷ Π_1
────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ ResultStmt(e) ⇒ 𝔅_1 ▷ Π_1

**(B-UnsafeStmt)**
Γ; 𝔅; Π ⊢ b ⇒ 𝔅_1 ▷ Π_1
──────────────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ UnsafeBlockStmt(b) ⇒ 𝔅_1 ▷ Π_1

**(B-Defer)**
Γ; 𝔅; Π ⊢ b ⇒ 𝔅_1 ▷ Π_1    𝔅_1 = 𝔅    Π_1 = Π
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ DeferStmt(b) ⇒ 𝔅 ▷ Π

**(B-RegionStmt)**
(opts_opt = ⊥ ⇒ 𝔅_0 = 𝔅 ∧ Π_0 = Π) ∧ (opts_opt = e ⇒ Γ; 𝔅; Π ⊢ e ⇒ 𝔅_0 ▷ Π_0)    𝔅_1 = PushScope_B(𝔅_0)    Π_1 = PushScope_Π(Π_0)    𝔅_2 = IntroAll_B(𝔅_1, RegionBindInfo(Γ, alias_opt))    Γ; 𝔅_2; Π_1 ⊢ b ⇒ 𝔅_3 ▷ Π_2
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ RegionStmt(opts_opt, alias_opt, b) ⇒ PopScope_B(𝔅_3) ▷ PopScope_Π(Π_2)

**(B-FrameStmt)**
𝔅_1 = PushScope_B(𝔅)    Π_1 = PushScope_Π(Π)    𝔅_2 = IntroAll_B(𝔅_1, FrameBindInfo(Γ))    Γ; 𝔅_2; Π_1 ⊢ b ⇒ 𝔅_3 ▷ Π_2
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ FrameStmt(r_opt, b) ⇒ PopScope_B(𝔅_3) ▷ PopScope_Π(Π_2)

**(B-Assign-Immutable-Err)**
stmt ∈ {AssignStmt(p, e), CompoundAssignStmt(p, op, e)}    IsPlace(p)    PlaceRoot(p) = x    Lookup_B(𝔅, x) = ⟨_, _, `let`, _⟩    c = Code(B-Assign-Immutable-Err)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ stmt ⇑ c

**(B-Assign)**
IsPlace(p)    PlaceRoot(p) = x    Lookup_B(𝔅, x) = ⟨s, mov, `var`, r⟩    Γ; 𝔅; Π ⊢ e ⇒ 𝔅_1 ▷ Π_1
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ AssignStmt(p, e) ⇒ Update_B(𝔅_1, x, ⟨Valid, mov, `var`, r⟩) ▷ Π_1

**(B-Assign-Const-Err)**
stmt ∈ {AssignStmt(p, e), CompoundAssignStmt(p, op, e)}    Γ; R; L ⊢ p : TypePerm(`const`, T)    c = Code(B-Assign-Const-Err)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ stmt ⇑ c

**(B-If)**
Γ; 𝔅; Π ⊢ c ⇒ 𝔅_c ▷ Π_c    Γ; 𝔅_c; Π_c ⊢ b_t ⇒ 𝔅_t ▷ Π_t    Γ; 𝔅_c; Π_c ⊢ b_f ⇒ 𝔅_f ▷ Π_f
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ IfExpr(c, b_t, b_f) ⇒ Join_B(𝔅_t, 𝔅_f) ▷ JoinPerm(Π_t, Π_f)

**(B-Match)**
Γ; 𝔅; Π ⊢ e ⇒ 𝔅_0 ▷ Π_0    Γ; R; L ⊢ e : T    moved = IsMoveExpr(e)    𝔅_1 = ConsumeOnMove(𝔅_0, e)    ∀ i, Γ; PushScope_B(𝔅_1); PushScope_Π(Π_0) ⊢ Arm(moved, p_i, g_i, b_i) ⇒ 𝔅_i ▷ Π_i
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ MatchExpr(e, arms) ⇒ JoinAll_B([𝔅_i]) ▷ JoinAllPerm([Π_i])

RespOfScrutinee(true) = resp
RespOfScrutinee(false) = alias

**(B-Arm)**
Γ ⊢ p ⇑ T ⊣ B    𝔅_0 = IntroAll_B(𝔅, BindInfoMap(λ U. RespOfScrutinee(moved), B, mov, `let`))    (g ≠ ⊥ ⇒ Γ; 𝔅_0; Π ⊢ g ⇒ 𝔅_1 ▷ Π_1)    (g = ⊥ ⇒ 𝔅_1 = 𝔅_0 ∧ Π_1 = Π)    Γ; 𝔅_1; Π_1 ⊢ b ⇒ 𝔅_2 ▷ Π_2
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ Arm(moved, p, g, b) ⇒ 𝔅_2 ▷ Π_2


**(B-Block)**
Γ; PushScope_B(𝔅); PushScope_Π(Π) ⊢ stmts ⇒ 𝔅_1 ▷ Π_1
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ BlockExpr(stmts, tail_opt) ⇒ PopScope_B(𝔅_1) ▷ PopScope_Π(Π_1)

**Loop Fixpoint.**

Moved <_B PartiallyMoved(F) <_B Valid
PartiallyMoved(F_1) <_B PartiallyMoved(F_2) ⇔ F_1 ⊇ F_2
x ≤_B y ⇔ x <_B y ∨ x = y
Active <_Π Inactive
x ≤_Π y ⇔ x <_Π y ∨ x = y
𝔅_1 ≤ 𝔅_2 ⇔ ∀ i. 𝔅_1[i] ≤_B 𝔅_2[i]
Π_1 ≤_Π Π_2 ⇔ ∀ i. Π_1[i] ≤_Π Π_2[i]

LoopStep : (𝔅, Π) → (𝔅, Π)
F(𝔅, Π) = (Join_B(𝔅_0, 𝔅'), JoinPerm(Π_0, Π')) where (𝔅', Π') = LoopStep(𝔅, Π)
(𝔅_0, Π_0) = (𝔅_init, Π_init)
(𝔅_{k+1}, Π_{k+1}) = F(𝔅_k, Π_k)
n = min{ k | (𝔅_k, Π_k) = (𝔅_{k+1}, Π_{k+1}) }
LoopFix(𝔅_init, Π_init, LoopStep) = (𝔅_n, Π_n)

**(B-Loop-Infinite)**
LoopStep(𝔅, Π) = (𝔅_b, Π_b) where Γ; 𝔅; Π ⊢ body ⇒ 𝔅_b ▷ Π_b    (𝔅', Π') = LoopFix(𝔅, Π, LoopStep)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ LoopInfinite(body) ⇒ 𝔅' ▷ Π'

**(B-Loop-Conditional)**
LoopStep(𝔅, Π) = (𝔅_b, Π_b) where Γ; 𝔅; Π ⊢ cond ⇒ 𝔅_c ▷ Π_c ∧ Γ; 𝔅_c; Π_c ⊢ body ⇒ 𝔅_b ▷ Π_b    (𝔅', Π') = LoopFix(𝔅, Π, LoopStep)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ LoopConditional(cond, body) ⇒ 𝔅' ▷ Π'

**(B-Loop-Iter)**
Γ; 𝔅; Π ⊢ iter ⇒ 𝔅_0 ▷ Π_0    Γ ⊢ pat ⇐ T_p ⊣ B    LoopStep(𝔅, Π) = (𝔅_b, Π_b) where 𝔅_1 = PushScope_B(𝔅) ∧ 𝔅_2 = IntroAll_B(𝔅_1, BindInfoMap(λ U. resp, B, mov, `let`)) ∧ Π_1 = PushScope_Π(Π) ∧ Γ; 𝔅_2; Π_1 ⊢ body ⇒ 𝔅_3 ▷ Π_2 ∧ 𝔅_b = PopScope_B(𝔅_3) ∧ Π_b = PopScope_Π(Π_2)    (𝔅', Π') = LoopFix(𝔅_0, Π_0, LoopStep)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ LoopIter(pat, ty_opt, iter, body) ⇒ 𝔅' ▷ Π'

**(B-Return)**
Γ; 𝔅; Π ⊢ e ⇒ 𝔅_1 ▷ Π_1
────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ ReturnStmt(e) ⇒ 𝔅_1 ▷ Π_1

**(B-Return-Unit)**
──────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ ReturnStmt(⊥) ⇒ 𝔅 ▷ Π

**(B-Break)**
Γ; 𝔅; Π ⊢ e ⇒ 𝔅_1 ▷ Π_1
────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ BreakStmt(e) ⇒ 𝔅_1 ▷ Π_1

**(B-Break-Unit)**
──────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ BreakStmt(⊥) ⇒ 𝔅 ▷ Π

**(B-Continue)**
──────────────────────────────────────────────
Γ; 𝔅; Π ⊢ ContinueStmt ⇒ 𝔅 ▷ Π

**(B-Move-Unique-Err)**
IsPlace(p)    Γ; R; L ⊢ p : T_p    PermOf(T_p) = `unique`    ¬ AccessPathOk(Π, p)    c = Code(B-Place-Unique-Err)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; 𝔅; Π ⊢ MoveExpr(p) ⇑ c

**Procedure/Method Binding-Check.**

Init_B(m, params) = IntroAll_B(PushScope_B(IntroAll_B(PushScope_B([]), StaticBindMap(Project(Γ), m))), ParamBindMap(params))
Init_Π(m, params) = [{ x ↦ Active | (x:T) ∈ ParamTypeMap(params) ∧ PermOf(T) = `unique` }] ++ [{ x ↦ Active | (x:T) ∈ StaticBindTypesMod(Project(Γ), m) ∧ PermOf(T) = `unique` }]

BindCheck(m, params, body) ⇓ ok ⇔ Γ; Init_B(m, params); Init_Π(m, params) ⊢ body ⇒ 𝔅' ▷ Π'

ProcBindCheck(m, ProcedureDecl(_, _, params, _, body, _, _)) ⇓ ok ⇔ BindCheck(m, params, body) ⇓ ok

MethodParamsDecl(T, m) = [⟨RecvMode(m.receiver), `self`, RecvType(T, m.receiver)⟩] ++ m.params
MethodBindCheck(m, T, md) ⇓ ok ⇔ md.body = body ∧ BindCheck(m, MethodParamsDecl(T, md), body) ⇓ ok
ClassMethodBindCheck(m, Cl, md) ⇓ ok ⇔ md.body_opt = body ∧ BindCheck(m, ClassMethodParams(Cl, md), body) ⇓ ok
StateMethodBindCheck(m, M, S, md) ⇓ ok ⇔ md.body = body ∧ BindCheck(m, StateMethodParams(M, S, md), body) ⇓ ok
TransitionBindCheck(m, M, S, tr) ⇓ ok ⇔ tr.body = body ∧ BindCheck(m, TransitionParams(M, S, tr), body) ⇓ ok

BindDiagRefs = {"8.2", "8.7", "8.10"}

#### 5.2.16. Safe Pointer Types (Cursive0)

PtrState = {`Valid`, `Null`, `Expired`}

Ptr<T> = TypePtr(T, ⊥)
Ptr<T>@s = TypePtr(T, s)    (s ≠ ⊥)

**Static Semantics**

BitcopyType(TypePtr(T, s))
CloneType(TypePtr(T, s))
¬ DropType(TypePtr(T, s))

sizeof(`Ptr<T>`) = sizeof(`usize`)    alignof(`Ptr<T>`) = alignof(`usize`)
PtrDiagRefs = {"8.10"}

#### 5.2.17. Regions, Frames, and Provenance (Cursive0)

**Built-in Record Type `RegionOptions`.**

RegionOptionsFields = [
  ⟨`public`, `stack_size`, TypePrim("usize"), Literal(IntLiteral(0)), ⊥, ⊥⟩,
  ⟨`public`, `name`, TypeString(⊥), Literal(StringLiteral("\"")), ⊥, ⊥⟩
]

RegionOptionsDecl = RecordDecl(`public`, `RegionOptions`, [], RegionOptionsFields, ⊥, ⊥)

Σ.Types[`RegionOptions`] = RegionOptionsDecl

RegionPrealloc(opts) = opts.stack_size
NoPrealloc(opts) ⇔ RegionPrealloc(opts) = 0

**Region/Frame Helpers.**

RegionActiveType(T) ⇔ StripPerm(T) = TypeModalState([`Region`], `Active`)

FreshRegion(Γ) ∈ Name \ dom(Γ)

RegionOptsExpr(⊥) = Call(Identifier(`RegionOptions`), [])
RegionOptsExpr(e) = e    if e ≠ ⊥

RegionBind(Γ, alias_opt) = Γ_r ⇔ r =
  { alias_opt           if alias_opt ≠ ⊥
    FreshRegion(Γ)      otherwise } ∧ IntroAll(Γ, [⟨r, TypeModalState([`Region`], `Active`)⟩]) ⇓ Γ_r

InnermostActiveRegion([]) = ⊥
InnermostActiveRegion([σ] ++ Γ') =
  { r                         if ∃ r. r ∈ dom(σ) ∧ RegionActiveType(σ[r])
    InnermostActiveRegion(Γ')  otherwise }

FrameBind(Γ, target_opt) = Γ_f ⇔ r =
  { InnermostActiveRegion(Γ)    if target_opt = ⊥
    target_opt                  if target_opt ≠ ⊥ ∧ Γ; R; L ⊢ Identifier(target_opt) : T_r ∧ RegionActiveType(T_r) } ∧ F = FreshRegion(Γ) ∧ IntroAll(Γ, [⟨F, TypeModalState([`Region`], `Active`)⟩]) ⇓ Γ_f

**Provenance Tags.**

π ::= π_Global | π_Stack(S) | π_Heap | π_Region(r) | ⊥

**Lifetime Order.**

π_1 < π_2 ⇔ (π_1 = π_Region(r_inner) ∧ π_2 = π_Region(r_outer) ∧ RegionNesting(r_inner, r_outer)) ∨ (π_1 = π_Region(r) ∧ π_2 = π_Stack(S)) ∨ (π_1 = π_Stack(S) ∧ π_2 = π_Heap) ∨ (π_1 = π_Heap ∧ π_2 = π_Global) ∨ (π_1 = π_Global ∧ π_2 = ⊥)

π_1 ≤ π_2 ⇔ π_1 = π_2 ∨ (π_1 < π_2) ∨ ∃ π. (π_1 < π ∧ π ≤ π_2)

FrameTargetRel(F, r) ⇔ FrameTarget(Γ, F) = r
FrameTargetRel(F, r) ⇒ π_Region(F) < π_Region(r)

JoinProv(π_1, π_2) =
  { π_1    if π_1 ≤ π_2
    π_2    if π_2 ≤ π_1
    ⊥      otherwise }

JoinAllProv([]) = ⊥
JoinAllProv([π]) = π
JoinAllProv([π_1, π_2] ++ ps) = JoinAllProv([JoinProv(π_1, π_2)] ++ ps)

**Provenance Environment.**

Ω = ⟨Σ_π, RS⟩
Scope_π = ⟨S, M⟩ where M : Ident ⇀ π
Σ_π ∈ [Scope_π]
RegionEntry_π = ⟨tag, target⟩
RS ∈ [RegionEntry_π]

ScopeId(⟨S, M⟩) = S
ScopeMap(⟨S, M⟩) = M
TopScopeId([⟨S, M⟩] ++ Σ_π) = S
StackProv(Σ_π) = π_Stack(TopScopeId(Σ_π))

PushScope_π(Σ_π) = [⟨S, ∅⟩] ++ Σ_π    (S fresh)
PopScope_π([_] ++ Σ_π) = Σ_π

Lookup_π([⟨S, M⟩] ++ Σ_π, x) =
  { M[x]                if x ∈ dom(M)
    Lookup_π(Σ_π, x)     otherwise }

Intro_π([⟨S, M⟩] ++ Σ_π, x, π) = [⟨S, M[x ↦ π]⟩] ++ Σ_π

ShadowIntro_π(Σ_π, x, π) =
  { [⟨S, M[x ↦ π]⟩] ++ Σ_π'                    if Σ_π = [⟨S, M⟩] ++ Σ_π' ∧ x ∈ dom(M)
    [⟨S, M⟩] ++ ShadowIntro_π(Σ_π', x, π)     if Σ_π = [⟨S, M⟩] ++ Σ_π' ∧ x ∉ dom(M)
    ⊥                                         if Σ_π = [] }

IntroAll_π(Σ_π, [], π) = Σ_π
IntroAll_π(Σ_π, [x] ++ xs, π) = IntroAll_π(Intro_π(Σ_π, x, π), xs, π)

ShadowAll_π(Σ_π, [], π) = Σ_π
ShadowAll_π(Σ_π, [x] ++ xs, π) = ShadowAll_π(ShadowIntro_π(Σ_π, x, π), xs, π)

ParamProvMap(params, vecπ) = { x_i ↦ π_i | params = [⟨_, x_i, _⟩], vecπ = [π_i] }
InitProvEnv(params, vecπ, RS) = ⟨[⟨S, ParamProvMap(params, vecπ)⟩], RS⟩    (S fresh)

AllocTag([], r) = ⊥
AllocTag([⟨tag, target⟩] ++ RS, ⊥) = tag
AllocTag([⟨tag, target⟩] ++ RS, r) =
  { tag              if target = r
    AllocTag(RS, r)  otherwise }

**Provenance of Places.**

ProvPlaceJudg = {Γ; Ω ⊢ p ⇓ π}

Lookup_π(Σ_π, x) = π
────────────────────────────────────────────────────────────────
Γ; ⟨Σ_π, RS⟩ ⊢ Identifier(x) ⇓ π

**(P-Field)**
Γ; Ω ⊢ p ⇓ π
────────────────────────────────────────────────────────────────
Γ; Ω ⊢ FieldAccess(p, f) ⇓ π

**(P-Tuple)**
Γ; Ω ⊢ p ⇓ π
────────────────────────────────────────────────────────────────
Γ; Ω ⊢ TupleAccess(p, i) ⇓ π

**(P-Index)**
Γ; Ω ⊢ p ⇓ π
────────────────────────────────────────────────────────────────
Γ; Ω ⊢ IndexAccess(p, i) ⇓ π

**(P-Deref)**
Γ; Ω ⊢ e ⇓ π
────────────────────────────────────────────────────────────────
Γ; Ω ⊢ Deref(e) ⇓ π

**Provenance of Expressions.**

ProvExprJudg = {Γ; Ω ⊢ e ⇓ π}

**(P-Place-Expr)**
IsPlace(p)    Γ; Ω ⊢ p ⇓ π
────────────────────────────────────────────────────────────────
Γ; Ω ⊢ p ⇓ π

**(P-Literal)**
────────────────────────────────────────────────────────────────
Γ; Ω ⊢ Literal(_) ⇓ ⊥

**(P-Move)**
Γ; Ω ⊢ p ⇓ π
────────────────────────────────────────────────────────────────
Γ; Ω ⊢ MoveExpr(p) ⇓ π

**(P-AddrOf)**
Γ; Ω ⊢ p ⇓ π
────────────────────────────────────────────────────────────────
Γ; Ω ⊢ AddressOf(p) ⇓ π

**(P-Alloc)**
Γ; Ω ⊢ e ⇓ π_e    AllocTag(RS, r_opt) = tag
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; ⟨Σ_π, RS⟩ ⊢ AllocExpr(r_opt, e) ⇓ π_Region(tag)

**(P-Region-Alloc-Method)**
name = `alloc`    (r : T_r) ∈ Γ    RegionActiveType(T_r)    AllocTag(RS, r) = tag
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; ⟨Σ_π, RS⟩ ⊢ MethodCall(Identifier(r), name, args) ⇓ π_Region(tag)

**(P-If)**
Γ; Ω ⊢ b_t ⇓ π_t    Γ; Ω ⊢ b_f ⇓ π_f    JoinProv(π_t, π_f) = π
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; Ω ⊢ IfExpr(c, b_t, b_f) ⇓ π

**(P-If-No-Else)**
────────────────────────────────────────────────────────────────
Γ; Ω ⊢ IfExpr(_, b_t, ⊥) ⇓ ⊥

ArmBodyProv(e, Ω) = π ⇔ Γ; Ω ⊢ e ⇓ π
ArmBodyProv(b, Ω) = π ⇔ Γ; Ω ⊢ b ⇓ π

ArmEnv(⟨Σ_π, RS⟩, pat) = ⟨Σ_π', RS⟩ ⇔ Γ ⊢ PatNames(pat) ⇓ N ∧ π_b = BindProv(⟨Σ_π, RS⟩, ⊥) ∧ Σ_π' = IntroAll_π(Σ_π, N, π_b)

ArmProv(⟨pat, _, body⟩) = π ⇔ ArmEnv(Ω, pat) = Ω' ∧ ArmBodyProv(body, Ω') = π

**(P-Match)**
∀ i, ArmProv(arm_i.body) = π_i    JoinAllProv([π_1, …, π_n]) = π
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; Ω ⊢ MatchExpr(_, arms) ⇓ π

ProvExprRules = {P-Place-Expr, P-Literal, P-Move, P-AddrOf, P-Alloc, P-Region-Alloc-Method, P-If, P-If-No-Else, P-Match, P-Block, P-Loop-Infinite, P-Loop-Conditional, P-Loop-Iter, P-Expr-Sub}

NoSpecificProvExpr(e) ⇔ ¬ ∃ r ∈ ProvExprRules \ {P-Expr-Sub}. PremisesHold(r, e)

**(P-Expr-Sub)**
NoSpecificProvExpr(e)    Children_LTR(e) = [e_1, …, e_n]    ∀ i, Γ; Ω ⊢ e_i ⇓ π_i    JoinAllProv([π_1, …, π_n]) = π
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; Ω ⊢ e ⇓ π

**Provenance of Statements.**

ProvStmtJudg = {Γ; Ω ⊢ s ⇒ Ω' ▷ ⟨Res, Brk, BrkVoid⟩, Γ; Ω ⊢ ss ⇒ Ω' ▷ ⟨Res, Brk, BrkVoid⟩}

LetOrVarStmt(binding) ∈ {LetStmt(binding), VarStmt(binding)}

**(Prov-Seq-Empty)**
────────────────────────────────────────────────────────────────
Γ; Ω ⊢ [] ⇒ Ω ▷ ⟨[], [], false⟩

**(Prov-Seq-Cons)**
Γ; Ω ⊢ s ⇒ Ω_1 ▷ ⟨Res_1, Brk_1, B_1⟩    Γ; Ω_1 ⊢ ss ⇒ Ω_2 ▷ ⟨Res_2, Brk_2, B_2⟩
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; Ω ⊢ s :: ss ⇒ Ω_2 ▷ ⟨Res_1 ++ Res_2, Brk_1 ++ Brk_2, B_1 ∨ B_2⟩

**(Prov-LetVar)**
binding = ⟨pat, _, _, init, _⟩    Γ; Ω ⊢ init ⇓ π_init    Γ ⊢ PatNames(pat) ⇓ N    π_bind = BindProv(Ω, π_init)    Σ_π' = IntroAll_π(Σ_π, N, π_bind)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; ⟨Σ_π, RS⟩ ⊢ LetOrVarStmt(binding) ⇒ ⟨Σ_π', RS⟩ ▷ ⟨[], [], false⟩

**(Prov-ShadowLet)**
Γ; Ω ⊢ init ⇓ π_init    π_bind = BindProv(Ω, π_init)    Σ_π' = ShadowAll_π(Σ_π, [x], π_bind)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; ⟨Σ_π, RS⟩ ⊢ ShadowLetStmt(x, _, init) ⇒ ⟨Σ_π', RS⟩ ▷ ⟨[], [], false⟩

**(Prov-ShadowVar)**
Γ; Ω ⊢ init ⇓ π_init    π_bind = BindProv(Ω, π_init)    Σ_π' = ShadowAll_π(Σ_π, [x], π_bind)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; ⟨Σ_π, RS⟩ ⊢ ShadowVarStmt(x, _, init) ⇒ ⟨Σ_π', RS⟩ ▷ ⟨[], [], false⟩

**(Prov-Assign)**
Γ; Ω ⊢ p ⇓ π_x    Γ; Ω ⊢ e ⇓ π_e    ¬(π_e < π_x)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; Ω ⊢ AssignStmt(p, e) ⇒ Ω ▷ ⟨[], [], false⟩

**(Prov-CompoundAssign)**
Γ; Ω ⊢ p ⇓ π_x    Γ; Ω ⊢ e ⇓ π_e    ¬(π_e < π_x)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; Ω ⊢ CompoundAssignStmt(p, _, e) ⇒ Ω ▷ ⟨[], [], false⟩

**(Prov-Escape-Err)**
stmt ∈ {AssignStmt(p, e), CompoundAssignStmt(p, _, e)}    Γ; Ω ⊢ p ⇓ π_x    Γ; Ω ⊢ e ⇓ π_e    π_e < π_x    c = Code(Prov-Escape-Err)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; Ω ⊢ stmt ⇑ c

**(Prov-ExprStmt)**
Γ; Ω ⊢ e ⇓ π
────────────────────────────────────────────────────────────────
Γ; Ω ⊢ ExprStmt(e) ⇒ Ω ▷ ⟨[], [], false⟩

**(Prov-ResultStmt)**
Γ; Ω ⊢ e ⇓ π
────────────────────────────────────────────────────────────────
Γ; Ω ⊢ ResultStmt(e) ⇒ Ω ▷ ⟨[π], [], false⟩

**(Prov-Return)**
Γ; Ω ⊢ e ⇓ π
────────────────────────────────────────────────────────────────
Γ; Ω ⊢ ReturnStmt(e) ⇒ Ω ▷ ⟨[π], [], false⟩

**(Prov-Return-Unit)**
────────────────────────────────────────────────────────────────
Γ; Ω ⊢ ReturnStmt(⊥) ⇒ Ω ▷ ⟨[], [], false⟩

**(Prov-Break)**
Γ; Ω ⊢ e ⇓ π
────────────────────────────────────────────────────────────────
Γ; Ω ⊢ BreakStmt(e) ⇒ Ω ▷ ⟨[], [π], false⟩

**(Prov-Break-Unit)**
────────────────────────────────────────────────────────────────
Γ; Ω ⊢ BreakStmt(⊥) ⇒ Ω ▷ ⟨[], [], true⟩

**(Prov-Continue)**
────────────────────────────────────────────────────────────────
Γ; Ω ⊢ ContinueStmt ⇒ Ω ▷ ⟨[], [], false⟩

**(Prov-UnsafeStmt)**
Γ; Ω ⊢ b ⇓ π
────────────────────────────────────────────────────────────────
Γ; Ω ⊢ UnsafeBlockStmt(b) ⇒ Ω ▷ ⟨[], [], false⟩

**(Prov-DeferStmt)**
Γ; Ω ⊢ b ⇓ π
────────────────────────────────────────────────────────────────
Γ; Ω ⊢ DeferStmt(b) ⇒ Ω ▷ ⟨[], [], false⟩

FrameTarget(Γ, ⊥) = r ⇔ InnermostActiveRegion(Γ) = r
FrameTarget(Γ, r) = r ⇔ Γ; R; L ⊢ Identifier(r) : T_r ∧ RegionActiveType(T_r)

**(Prov-RegionStmt)**
RegionOptsExpr(opts_opt) = opts    Γ; Ω ⊢ opts ⇓ π_opts    r = RegionBindName(Γ, alias_opt)    Σ_π^0 = PushScope_π(Σ_π)    π_r = BindProv(⟨Σ_π^0, RS⟩, ⊥)    Σ_π' = Intro_π(Σ_π^0, r, π_r)    RS' = ⟨r, r⟩ :: RS    Γ; ⟨Σ_π', RS'⟩ ⊢ b ⇓ π_b
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; ⟨Σ_π, RS⟩ ⊢ RegionStmt(opts_opt, alias_opt, b) ⇒ ⟨Σ_π, RS⟩ ▷ ⟨[], [], false⟩

**(Prov-FrameStmt)**
FrameTarget(Γ, target_opt) = r    F = FreshRegion(Γ)    Σ_π^0 = PushScope_π(Σ_π)    π_F = BindProv(⟨Σ_π^0, RS⟩, ⊥)    Σ_π' = Intro_π(Σ_π^0, F, π_F)    RS' = ⟨F, r⟩ :: RS    Γ; ⟨Σ_π', RS'⟩ ⊢ b ⇓ π_b
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; ⟨Σ_π, RS⟩ ⊢ FrameStmt(target_opt, b) ⇒ ⟨Σ_π, RS⟩ ▷ ⟨[], [], false⟩

**(Prov-ErrorStmt)**
────────────────────────────────────────────────────────────────
Γ; Ω ⊢ ErrorStmt(_) ⇒ Ω ▷ ⟨[], [], false⟩

**Block Provenance.**

BlockProvJudg = {Γ; Ω ⊢ BlockProv(stmts, tail_opt) ⇓ π}

Ω_0 = ⟨PushScope_π(Σ_π), RS⟩

**(BlockProv-Res)**
Γ; Ω_0 ⊢ stmts ⇒ Ω_1 ▷ ⟨Res, Brk, BrkVoid⟩    Res ≠ []    JoinAllProv(Res) = π    (tail_opt = e ⇒ Γ; Ω_1 ⊢ e ⇓ π_t)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; ⟨Σ_π, RS⟩ ⊢ BlockProv(stmts, tail_opt) ⇓ π

**(BlockProv-Tail)**
Γ; Ω_0 ⊢ stmts ⇒ Ω_1 ▷ ⟨Res, Brk, BrkVoid⟩    Res = []    tail_opt = e    Γ; Ω_1 ⊢ e ⇓ π
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; ⟨Σ_π, RS⟩ ⊢ BlockProv(stmts, tail_opt) ⇓ π

**(BlockProv-Unit)**
Γ; Ω_0 ⊢ stmts ⇒ Ω_1 ▷ ⟨Res, Brk, BrkVoid⟩    Res = []    tail_opt = ⊥
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; ⟨Σ_π, RS⟩ ⊢ BlockProv(stmts, ⊥) ⇓ ⊥

**(P-Block)**
Γ; Ω ⊢ BlockProv(stmts, tail_opt) ⇓ π
────────────────────────────────────────────────────────────────
Γ; Ω ⊢ BlockExpr(stmts, tail_opt) ⇓ π

**Loop Provenance.**

BreakProv(body, Ω) = ⟨Brk, BrkVoid⟩ ⇔ body = BlockExpr(stmts, tail_opt) ∧ Ω_0 = ⟨PushScope_π(Σ_π), RS⟩ ∧ Γ; Ω_0 ⊢ stmts ⇒ Ω_1 ▷ ⟨Res, Brk, BrkVoid⟩ ∧ (tail_opt = e ⇒ Γ; Ω_1 ⊢ e ⇓ π_t)

IterElemProv(iter, Ω) = π ⇔ Γ; Ω ⊢ iter ⇓ π

LoopProvInf(Brk, BrkVoid) = ⊥ ⇔ Brk = []
LoopProvInf(Brk, BrkVoid) = π ⇔ Brk = [π_1, …, π_n] ∧ BrkVoid = false ∧ JoinAllProv([π_1, …, π_n]) = π

LoopProvFin(Brk, BrkVoid) = ⊥ ⇔ Brk = []
LoopProvFin(Brk, BrkVoid) = π ⇔ Brk = [π_1, …, π_n] ∧ BrkVoid = false ∧ JoinAllProv([π_1, …, π_n]) = π

ExtendProv(⟨Σ_π, RS⟩, pat, π) = ⟨Σ_π', RS⟩ ⇔ Γ ⊢ PatNames(pat) ⇓ N ∧ Σ_π' = IntroAll_π(Σ_π, N, π)

**(P-Loop-Infinite)**
BreakProv(body, Ω) = ⟨Brk, BrkVoid⟩    LoopProvInf(Brk, BrkVoid) = π
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; Ω ⊢ LoopInfinite(body) ⇓ π

**(P-Loop-Conditional)**
BreakProv(body, Ω) = ⟨Brk, BrkVoid⟩    LoopProvFin(Brk, BrkVoid) = π
────────────────────────────────────────────────────────────────────────────────────────────────
Γ; Ω ⊢ LoopConditional(cond, body) ⇓ π

**(P-Loop-Iter)**
IterElemProv(iter, Ω) = π_elem    ExtendProv(Ω, pat, π_elem) = Ω'    BreakProv(body, Ω') = ⟨Brk, BrkVoid⟩    LoopProvFin(Brk, BrkVoid) = π
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; Ω ⊢ LoopIter(pat, ty_opt, iter, body) ⇓ π


EscapeOk(π_e, π_x) ⇔ ¬(π_e < π_x)

BindProv(⟨Σ_π, RS⟩, π_init) =
  { StackProv(Σ_π)    if π_init = ⊥
    π_init            otherwise }

StaticBindProv = π_Global

AssignProvOk(Ω, p, e) ⇔ Γ; Ω ⊢ p ⇓ π_x ∧ Γ; Ω ⊢ e ⇓ π_e ∧ EscapeOk(π_e, π_x)

### 5.3. Classes and Record Methods (Cursive0)

#### 5.3.1. Classes (Cursive0)

**Common Method-Signature Definitions.**

Distinct(xs) ⇔ ∀ i ≠ j. xs[i] ≠ xs[j]
Disjoint(xs, ys) ⇔ ∀ x ∈ xs. x ∉ ys

ReturnType(m) =
  { m.return_type_opt        if m.return_type_opt ≠ ⊥
    TypePrim("()")           if m.return_type_opt = ⊥ }

SelfVar = TypePath([`Self`])

SubstSelf(T, TypePath([`Self`])) = T
SubstSelf(T, TypePerm(p, ty)) = TypePerm(p, SubstSelf(T, ty))
SubstSelf(T, TypeTuple([t_1, …, t_n])) = TypeTuple([SubstSelf(T, t_1), …, SubstSelf(T, t_n)])
SubstSelf(T, TypeArray(ty, e)) = TypeArray(SubstSelf(T, ty), e)
SubstSelf(T, TypeSlice(ty)) = TypeSlice(SubstSelf(T, ty))
SubstSelf(T, TypeUnion([t_1, …, t_n])) = TypeUnion([SubstSelf(T, t_1), …, SubstSelf(T, t_n)])
SubstSelf(T, TypeFunc([⟨m_1, t_1⟩, …, ⟨m_n, t_n⟩], r)) = TypeFunc([⟨m_1, SubstSelf(T, t_1)⟩, …, ⟨m_n, SubstSelf(T, t_n)⟩], SubstSelf(T, r))
SubstSelf(T, TypePtr(ty, s)) = TypePtr(SubstSelf(T, ty), s)
SubstSelf(T, TypeRawPtr(q, ty)) = TypeRawPtr(q, SubstSelf(T, ty))
SubstSelf(T, TypeString(state_opt)) = TypeString(state_opt)
SubstSelf(T, TypeBytes(state_opt)) = TypeBytes(state_opt)
SubstSelf(T, TypeModalState(p, S)) = TypeModalState(p, S)
SubstSelf(T, TypeDynamic(p)) = TypeDynamic(p)
SubstSelf(T, TypePrim(n)) = TypePrim(n)
SubstSelf(T, TypePath(p)) = TypePath(p)    if p ≠ [`Self`]

RecvType(T, ReceiverShorthand(`const`)) = TypePerm(`const`, T)
RecvType(T, ReceiverShorthand(`unique`)) = TypePerm(`unique`, T)
RecvType(T, ReceiverExplicit(mode_opt, ty)) = SubstSelf(T, ty)

RecvMode(ReceiverShorthand(_)) = ⊥
RecvMode(ReceiverExplicit(mode_opt, _)) = mode_opt

PermOf(TypePerm(p, _)) = p
PermOf(ty) = `const`    otherwise

RecvPerm(T, r) = PermOf(RecvType(T, r))

ParamSig_T(T, params) = [⟨mode, SubstSelf(T, ty)⟩ | ⟨mode, name, ty⟩ ∈ params]
ParamBinds_T(T, params) = [⟨x_1, SubstSelf(T, T_1)⟩, …, ⟨x_n, SubstSelf(T, T_n)⟩]
ReturnType_T(T, m) = SubstSelf(T, ReturnType(m))

Sig_T(T, m) = ⟨RecvType(T, m.receiver), ParamSig_T(T, m.params), SubstSelf(T, ReturnType(m))⟩

SigSelf(m) = Sig_T(SelfVar, m)

**Class Declarations.**

ClassItems(Cl) = Cl.items
ClassMethods(Cl) = [ m | m ∈ ClassItems(Cl) ∧ ∃ vis, name, recv, params, ret, body, span, doc. m = ClassMethodDecl(vis, name, recv, params, ret, body, span, doc) ]
ClassFields(Cl) = [ f | f ∈ ClassItems(Cl) ∧ ∃ vis, name, ty, span, doc. f = ClassFieldDecl(vis, name, ty, span, doc) ]
MethodNames(Cl) = [ m.name | m ∈ ClassMethods(Cl) ]
FieldNames(Cl) = [ f.name | f ∈ ClassFields(Cl) ]

**Class Path Well-Formedness.**

**(WF-ClassPath)**
p ∈ dom(Σ.Classes)
──────────────────────────────────────────────
Γ ⊢ p : ClassPath

**(WF-ClassPath-Err)**
p ∉ dom(Σ.Classes)    c = Code(Superclass-Undefined)
────────────────────────────────────────────────────────────────
Γ ⊢ p : ClassPath ⇑ c

**Superclass Linearization (C3).**

Supers(Cl) = [S_1, …, S_n]

**(Lin-Base)**
Supers(Cl) = []
──────────────────────────────────────────────
Γ ⊢ Linearize(Cl) ⇓ [Cl]

Head(h :: t) = h
Tail([]) = []
Tail(h :: t) = t
HeadOk(h, Ls) ⇔ ∃ L ∈ Ls. L = h :: t ∧ ∀ L' ∈ Ls. h ∉ Tail(L')
SelectHead(Ls) = h ⇔ Ls = [L_1, …, L_n] ∧ L_i = h :: t ∧ HeadOk(h, Ls) ∧ ∀ j < i. ¬ HeadOk(Head(L_j), Ls)
SelectHead(Ls) = ⊥ ⇔ ¬ ∃ h. HeadOk(h, Ls)
PopHead(h, L) = t ⇔ L = h :: t
PopHead(h, L) = L ⇔ ¬(L = h :: t)
PopAll(h, Ls) = [PopHead(h, L) | L ∈ Ls]

**(Merge-Empty)**
∀ L ∈ Ls, L = []
──────────────────────────────────────────────
Γ ⊢ Merge(Ls) ⇓ []

**(Merge-Step)**
SelectHead(Ls) = h    Γ ⊢ Merge(PopAll(h, Ls)) ⇓ L
────────────────────────────────────────────────────────────────
Γ ⊢ Merge(Ls) ⇓ [h] ++ L

**(Merge-Fail)**
¬ ∃ h. HeadOk(h, Ls)
──────────────────────────────────────────────
Γ ⊢ Merge(Ls) ⇑

**(Lin-Ok)**
Γ ⊢ Merge([Linearize(S_1), …, Linearize(S_n), [S_1, …, S_n]]) ⇓ L
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Linearize(Cl) ⇓ [Cl] ++ L

**(Lin-Fail)**
Γ ⊢ Merge(⋯) ⇑
──────────────────────────────────────────────
Γ ⊢ Linearize(Cl) ⇑

**(Superclass-Cycle)**
Γ ⊢ Linearize(Cl) ⇑    c = Code(Superclass-Cycle)
────────────────────────────────────────────────────────────────
Γ ⊢ Cl ⇑ c

**Effective Method Set.**

Linearize(Cl) = [C_0, C_1, …, C_k] ∧ C_0 = Cl

EffMethods(Cl) = FirstByName(++_{i=0..k} ClassMethods(C_i))

FirstByName(ms) = FirstByName(ms, ∅)

FirstByName([], Seen) = []
FirstByName(m :: ms, Seen) =
  { m :: FirstByName(ms, Seen ∪ { m.name ↦ SigSelf(m) })    if m.name ∉ dom(Seen)
    FirstByName(ms, Seen)                                  if Seen[m.name] = SigSelf(m)
    ⇑                                                      otherwise }

**(EffMethods-Conflict)**
FirstByName(ms) ⇑    c = Code(EffMethods-Conflict)
────────────────────────────────────────────────────────────────
Γ ⊢ Emit(c)

**Effective Field Set.**

FieldSig(f) = SubstSelf(SelfVar, f.type)

EffFields(Cl) = FirstFieldByName(++_{i=0..k} ClassFields(C_i))

FirstFieldByName(fs) = FirstFieldByName(fs, ∅)

FirstFieldByName([], Seen) = []
FirstFieldByName(f :: fs, Seen) =
  { f :: FirstFieldByName(fs, Seen ∪ { f.name ↦ FieldSig(f) })    if f.name ∉ dom(Seen)
    FirstFieldByName(fs, Seen)                                   if Seen[f.name] = FieldSig(f)
    ⇑                                                            otherwise }

**(EffFields-Conflict)**
FirstFieldByName(fs) ⇑    c = Code(EffFields-Conflict)
────────────────────────────────────────────────────────────────
Γ ⊢ Emit(c)

**Dispatchability.**

SelfOccurs(TypePath([`Self`])) = true
SelfOccurs(TypePerm(p, ty)) = SelfOccurs(ty)
SelfOccurs(TypeTuple([t_1, …, t_n])) = ∨_i SelfOccurs(t_i)
SelfOccurs(TypeArray(ty, e)) = SelfOccurs(ty)
SelfOccurs(TypeSlice(ty)) = SelfOccurs(ty)
SelfOccurs(TypeUnion([t_1, …, t_n])) = ∨_i SelfOccurs(t_i)
SelfOccurs(TypeFunc([⟨m_1, t_1⟩, …, ⟨m_n, t_n⟩], r)) = (∨_i SelfOccurs(t_i)) ∨ SelfOccurs(r)
SelfOccurs(TypePtr(ty, s)) = SelfOccurs(ty)
SelfOccurs(TypeRawPtr(q, ty)) = SelfOccurs(ty)
SelfOccurs(TypeString(state_opt)) = false
SelfOccurs(TypeBytes(state_opt)) = false
SelfOccurs(TypeModalState(p, S)) = false
SelfOccurs(TypeDynamic(p)) = false
SelfOccurs(TypePrim(n)) = false
SelfOccurs(TypePath(p)) = false    if p ≠ [`Self`]

SelfOccurs(m) ⇔ SelfOccurs(ReturnType(m)) ∨ ∃ ⟨_, _, ty⟩ ∈ m.params. SelfOccurs(ty)
HasReceiver(m) ⇔ m.receiver ≠ ⊥
vtable_eligible(m) ⇔ HasReceiver(m) ∧ ¬ SelfOccurs(m)

dispatchable(Cl) ⇔ ∀ m ∈ EffMethods(Cl). vtable_eligible(m)

**Class Method Well-Formedness.**

SelfTypeClass(ty) ⇔ ty = SelfVar ∨ ∃ p. ty = TypePerm(p, SelfVar)

**(WF-Class-Method)**
(r = ReceiverExplicit(mode_opt, ty) ⇒ SelfTypeClass(ty))    (r = ReceiverShorthand(_) ⇒ true)    Γ ⊢ RecvType(SelfVar, r) wf    self ∉ ParamNames(params)    Distinct(ParamNames(params))    ∀ ⟨_, _, T_i⟩ ∈ params, Γ ⊢ T_i wf    (return_type_opt = ⊥ ∨ Γ ⊢ return_type_opt wf)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ⟨ClassMethodDecl, _, name, r, params, return_type_opt, body_opt, _, _⟩ : ClassMethodOK(Cl)

**(T-Class-Method-Body-Abstract)**
m.body_opt = ⊥
──────────────────────────────────────────────
Γ ⊢ m : ClassMethodBodyOK

**(T-Class-Method-Body)**
m.body_opt = body    T_self = RecvType(SelfVar, m.receiver)    R_m = ReturnType_T(SelfVar, m)    Γ_0 = PushScope(Γ)    IntroAll(Γ_0, [⟨`self`, T_self⟩] ++ ParamBinds_T(SelfVar, m.params)) ⇓ Γ_1    Γ_1; R_m; ⊥ ⊢ body : T_b    Γ ⊢ T_b <: R_m    (R_m ≠ TypePrim("()") ⇒ ExplicitReturn(body))
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ m : ClassMethodBodyOK

**(WF-Class)**
Distinct(MethodNames(Cl))    Distinct(FieldNames(Cl))    Disjoint(MethodNames(Cl), FieldNames(Cl))    Distinct(Supers(Cl))    ∀ S ∈ Supers(Cl), Γ ⊢ S : ClassPath    ∀ m ∈ ClassMethods(Cl), Γ ⊢ m : ClassMethodOK(Cl)    Γ ⊢ m : ClassMethodBodyOK    Γ ⊢ Linearize(Cl) ⇓ L
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Cl : ClassOk

**(Class-Method-Dup)**
¬ Distinct(MethodNames(Cl))    c = Code(Class-Method-Dup)
────────────────────────────────────────────────────────────────
Γ ⊢ Cl ⇑ c

**(Class-AbstractField-Dup)**
¬ Distinct(FieldNames(Cl))    c = Code(Class-AbstractField-Dup)
────────────────────────────────────────────────────────────────
Γ ⊢ Cl ⇑ c

**(Class-Name-Conflict)**
¬ Disjoint(MethodNames(Cl), FieldNames(Cl))    c = Code(Class-Name-Conflict)
────────────────────────────────────────────────────────────────
Γ ⊢ Cl ⇑ c

**(Superclass-Undefined)**
S ∈ Supers(Cl)    ¬(Γ ⊢ S : ClassPath)    c = Code(Superclass-Undefined)
────────────────────────────────────────────────────────────────
Γ ⊢ Cl ⇑ c

**Class Implementation.**

Implements(T) = impls ⇔ T = RecordDecl(_, _, impls, _, _, _) ∨ T = EnumDecl(_, _, impls, _, _, _) ∨ T = ModalDecl(_, _, impls, _, _, _) ∨ T = ClassDecl(_, _, impls, _, _)

**Implements Clause Constraints.**
NoDefaultMethods(Cl) ⇔ ∀ m ∈ ClassMethods(Cl). m.body = ⊥
BitcopyImpliesClone(T) ⇔ (`Bitcopy` ∈ Implements(T)) ⇒ (Clone ∈ Implements(T))
AbstractsImplemented(T) ⇔ ∀ Cl ∈ Implements(T). ∀ m ∈ ClassMethodTable(Cl). (m.body = ⊥ ⇒ MethodByName(T, m.name) ≠ ⊥)

**(Impl-Duplicate-Class-Err)**
¬ Distinct(Implements(T))    c = Code(Impl-Duplicate-Class-Err)
────────────────────────────────────────────────────────────────
Γ ⊢ T : ImplementsOk ⇑ c

Fields(T) = Fields(R) ⇔ T = TypePath(p) ∧ RecordDecl(p) = R
Methods(T) = Methods(R) ⇔ T = TypePath(p) ∧ RecordDecl(p) = R
Fields(T) = [] ⇔ T = TypePath(p) ∧ (EnumDecl(p) = E ∨ Σ.Types[p] = `modal` M)
Methods(T) = [] ⇔ T = TypePath(p) ∧ (EnumDecl(p) = E ∨ Σ.Types[p] = `modal` M)
MethodByName(T, name) = m' ⇔ m' ∈ Methods(T) ∧ m'.name = name
MethodByName(T, name) = ⊥ ⇔ ¬ ∃ m' ∈ Methods(T). m'.name = name
ClassMethodTable(Cl) = EffMethods(Cl)
ClassFieldTable(Cl) = EffFields(Cl)

**(Impl-Abstract-Method)**
m ∈ ClassMethodTable(Cl)    m.body = ⊥    MethodByName(T, m.name) = m'    Sig_T(T, m') = Sig_T(T, m)    m'.override = false
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ T implements abstract m

**(Impl-Missing-Method)**
m ∈ ClassMethodTable(Cl)    m.body = ⊥    MethodByName(T, m.name) = ⊥    c = Code(Impl-Missing-Method)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ T : ImplementsOk ⇑ c

**(Impl-Sig-Err)**
m ∈ ClassMethodTable(Cl)    m.body = ⊥    MethodByName(T, m.name) = m'    Sig_T(T, m') ≠ Sig_T(T, m)    c = Code(Impl-Missing-Method)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ T : ImplementsOk ⇑ c

**(Override-Abstract-Err)**
m ∈ ClassMethodTable(Cl)    m.body = ⊥    MethodByName(T, m.name) = m'    Sig_T(T, m') = Sig_T(T, m)    m'.override = true    c = Code(Override-Abstract-Err)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ T : ImplementsOk ⇑ c

**(Impl-Concrete-Default)**
m ∈ ClassMethodTable(Cl)    m.body ≠ ⊥    MethodByName(T, m.name) = ⊥
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ T uses default m

**(Impl-Concrete-Override)**
m ∈ ClassMethodTable(Cl)    m.body ≠ ⊥    MethodByName(T, m.name) = m'    Sig_T(T, m') = Sig_T(T, m)    m'.override = true
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ T overrides m

**(Override-Missing-Err)**
m ∈ ClassMethodTable(Cl)    m.body ≠ ⊥    MethodByName(T, m.name) = m'    Sig_T(T, m') = Sig_T(T, m)    m'.override = false    c = Code(Override-Missing-Err)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ T : ImplementsOk ⇑ c

**(Impl-Sig-Err-Concrete)**
m ∈ ClassMethodTable(Cl)    m.body ≠ ⊥    MethodByName(T, m.name) = m'    Sig_T(T, m') ≠ Sig_T(T, m)    c = Code(Impl-Missing-Method)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ T : ImplementsOk ⇑ c

**(Override-NoConcrete)**
m' ∈ Methods(T)    m'.override = true    ¬ ∃ Cl ∈ Implements(T). ∃ m ∈ ClassMethodTable(Cl). m.name = m'.name ∧ m.body ≠ ⊥    c = Code(Override-NoConcrete)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ T : ImplementsOk ⇑ c

**(Impl-Field)**
f : T_c ∈ ClassFieldTable(Cl)    f : T_i ∈ Fields(T)    T_i <: T_c
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ T satisfies field f

**(Impl-Field-Missing)**
f : T_c ∈ ClassFieldTable(Cl)    ¬ ∃ T_i. f : T_i ∈ Fields(T)    c = Code(Impl-Field-Missing)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ T : ImplementsOk ⇑ c

**(Impl-Field-Type-Err)**
f : T_c ∈ ClassFieldTable(Cl)    f : T_i ∈ Fields(T)    ¬(Γ ⊢ T_i <: T_c)    c = Code(Impl-Field-Type-Err)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ T : ImplementsOk ⇑ c

**(WF-Impl)**
∀ Cl ∈ Implements(T), Γ ⊢ Cl : ClassOk    Distinct(Implements(T))    Γ ⊢ T : BitcopyDropOk    ∀ Cl ∈ Implements(T), ∀ m ∈ ClassMethodTable(Cl), (Γ ⊢ T implements abstract m ∨ Γ ⊢ T overrides m ∨ Γ ⊢ T uses default m)    ∀ Cl ∈ Implements(T), ∀ f ∈ ClassFieldTable(Cl), Γ ⊢ T satisfies field f
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ T : ImplementsOk

**(Impl-Dup)**
¬ Distinct(Implements(T))    c = Code(Impl-Dup)
────────────────────────────────────────────────────────────────
Γ ⊢ T : ImplementsOk ⇑ c

Γ ⊢ T <: Cl ⇔ Cl ∈ Implements(T) ∧ Γ ⊢ T : ImplementsOk

**Superclass Closure.**
S ∈ Supers(Cl) ∨ (S ∈ Linearize(Cl) ∧ S ≠ Cl)
──────────────────────────────────────────────
Γ ⊢ Cl <: S

Γ ⊢ T <: Cl    Γ ⊢ Cl <: S
──────────────────────────────────────────────
Γ ⊢ T <: S

**Dynamic Class Types.**

**(T-Dynamic-Form)**
Γ; R; L ⊢ e :place T    IsPlace(e)    AddrOfOk(e)    Γ ⊢ Cl : ClassPath    Γ ⊢ StripPerm(T) <: Cl    dispatchable(Cl)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ e `as` TypeDynamic(Cl) : TypeDynamic(Cl)

**(Dynamic-NonDispatchable)**
Γ; R; L ⊢ e :place T    IsPlace(e)    Γ ⊢ Cl : ClassPath    Γ ⊢ StripPerm(T) <: Cl    ¬ dispatchable(Cl)    c = Code(Dynamic-NonDispatchable)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ e `as` TypeDynamic(Cl) ⇑ c

**Method Lookup for Concrete Types.**

ClassDefaults(T, name) = { m | ∃ Cl ∈ Implements(T). m ∈ ClassMethodTable(Cl) ∧ m.name = name ∧ m.body ≠ ⊥ }
LookupMethod(T, name) = m ⇔ MethodByName(T, name) = m
LookupMethod(T, name) = m ⇔ MethodByName(T, name) = ⊥ ∧ |ClassDefaults(T, name)| = 1 ∧ m ∈ ClassDefaults(T, name)
LookupMethod(T, name) = ⊥ ⇔ MethodByName(T, name) = ⊥ ∧ (|ClassDefaults(T, name)| = 0 ∨ |ClassDefaults(T, name)| > 1)

**(LookupMethod-NotFound)**
Γ; R; L ⊢ base : T_b    MethodByName(StripPerm(T_b), name) = ⊥    ClassDefaults(StripPerm(T_b), name) = ∅    c = Code(LookupMethod-NotFound)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ MethodCall(base, name, args) ⇑ c

**(LookupMethod-Ambig)**
Γ; R; L ⊢ base : T_b    MethodByName(StripPerm(T_b), name) = ⊥    |ClassDefaults(StripPerm(T_b), name)| > 1    c = Code(LookupMethod-Ambig)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ MethodCall(base, name, args) ⇑ c

**(Drop-Call-Err)**
Γ; R; L ⊢ base : T_b    LookupMethod(StripPerm(T_b), name) = m    MethodOwner(m) = `Drop`    MethodName(m) = `drop`    c = Code(Drop-Call-Err)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ MethodCall(base, name, args) ⇑ c

**(Drop-Call-Err-Dyn)**
Γ; R; L ⊢ base : TypeDynamic(Cl)    LookupClassMethod(Cl, name) = m    MethodOwner(m) = `Drop`    MethodName(m) = `drop`    c = Code(Drop-Call-Err-Dyn)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ MethodCall(base, name, args) ⇑ c

**(T-MethodCall)**
RecvBaseType(base, RecvMode(m.receiver)) = P_caller T    LookupMethod(T, name) = m    RecvPerm(T, m.receiver) = P_method    PermSub(P_caller, P_method)    RecvArgOk(base, RecvMode(m.receiver))    Γ; R; L ⊢ ArgsOk(m.params, args)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ MethodCall(base, name, args) : ReturnType(m)

**(MethodCall-RecvPerm-Err)**
RecvBaseType(base, RecvMode(m.receiver)) = P_caller T    LookupMethod(T, name) = m    RecvPerm(T, m.receiver) = P_method    ¬ PermSub(P_caller, P_method)    c = Code(MethodCall-RecvPerm-Err)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ MethodCall(base, name, args) ⇑ c
**(T-Dynamic-MethodCall)**
RecvBaseType(base, RecvMode(m.receiver)) = P_caller TypeDynamic(Cl)    LookupClassMethod(Cl, name) = m    RecvPerm(SelfVar, m.receiver) = P_method    PermSub(P_caller, P_method)    RecvArgOk(base, RecvMode(m.receiver))    Γ; R; L ⊢ ArgsOk(m.params, args)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ MethodCall(base, name, args) : ReturnType(m)

**(LookupClassMethod-NotFound)**
Γ; R; L ⊢ base : TypeDynamic(Cl)    LookupClassMethod(Cl, name) undefined    c = Code(LookupMethod-NotFound)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ MethodCall(base, name, args) ⇑ c


#### 5.3.2. Record Methods (Cursive0)

**Definitions.**

Fields(R) = [ f | f ∈ R.members ∧ ∃ vis, name, ty, init, span, doc. f = FieldDecl(vis, name, ty, init, span, doc) ]
Methods(R) = [ m | m ∈ R.members ∧ ∃ vis, override, name, recv, params, ret, body, span, doc. m = MethodDecl(vis, override, name, recv, params, ret, body, span, doc) ]
Self_R = TypePath(RecordPath(R))
SelfType(R, ty) ⇔ ty = Self_R ∨ ∃ p. ty = TypePerm(p, Self_R)

**Static Semantics**

**(Recv-Explicit)**
SelfType(R, ty)
────────────────────────────────────────────────────────────────
Γ ⊢ ReceiverExplicit(mode_opt, ty) : Recv(R, PermOf(ty), mode_opt)

**(Record-Method-RecvSelf-Err)**
¬ SelfType(R, ty)    c = Code(Record-Method-RecvSelf-Err)
────────────────────────────────────────────────────────────────
Γ ⊢ ReceiverExplicit(mode_opt, ty) ⇑ c

**(Recv-Const)**
────────────────────────────────────────────────────────────────
Γ ⊢ ReceiverShorthand(`const`) : Recv(R, `const`, ⊥)

**(Recv-Unique)**
────────────────────────────────────────────────────────────────
Γ ⊢ ReceiverShorthand(`unique`) : Recv(R, `unique`, ⊥)

ParamNames(params) = [x | ⟨_, x, _⟩ ∈ params]

**(WF-Record-Method)**
Γ ⊢ r : Recv(R, P, mode)    self ∉ ParamNames(params)    Distinct(ParamNames(params))    ∀ ⟨_, _, T_i⟩ ∈ params, Γ ⊢ T_i wf    (return_type_opt = ⊥ ∨ Γ ⊢ return_type_opt wf)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ⟨MethodDecl, _, name, r, params, return_type_opt, body, _, _⟩ : MethodOK(R, P, mode)

**(T-Record-Method-Body)**
Γ ⊢ m : MethodOK(R, P, mode)    T_self = RecvType(Self_R, m.receiver)    R_m = ReturnType_T(Self_R, m)    Γ_0 = PushScope(Γ)    IntroAll(Γ_0, [⟨`self`, T_self⟩] ++ ParamBinds_T(Self_R, m.params)) ⇓ Γ_1    Γ_1; R_m; ⊥ ⊢ m.body : T_b    Γ ⊢ T_b <: R_m    (R_m ≠ TypePrim("()") ⇒ ExplicitReturn(m.body))
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ m : MethodBodyOK(R)

MethodNames(R) = [ m.name | m ∈ Methods(R) ]

**(WF-Record-Methods)**
Distinct(MethodNames(R))    ∀ m ∈ Methods(R), Γ ⊢ m : MethodOK(R, _, _)    Γ ⊢ m : MethodBodyOK(R)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Methods(R) : ok

**(Record-Method-Dup)**
¬ Distinct(MethodNames(R))    c = Code(Record-Method-Dup)
────────────────────────────────────────────────────────────────
Γ ⊢ Methods(R) ⇑ c

**Method Lookup.**

LookupMethodRules = RulesIn({"5.3.1"})

**Argument Compatibility.**

ArgsOkJudg = {Γ; R; L ⊢ ArgsOk(params, args)}

RecvBaseType(base, mode) = P T ⇔ (mode = ⊥ ∧ Γ; R; L ⊢ base :place P T) ∨ (mode = `move` ∧ Γ; R; L ⊢ base : P T)

**(Args-Empty)**
──────────────────────────────────────────────
Γ; R; L ⊢ ArgsOk([], [])

**(Args-Cons)**
Γ; R; L ⊢ MovedArg(moved, e) ⇐ T_p ⊣ ∅    moved = true    Γ; R; L ⊢ ArgsOk(ps, as)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ ArgsOk([⟨`move`, x, T_p⟩] ++ ps, [⟨moved, e, _⟩] ++ as)

**(Args-Cons-Ref)**
Γ; R; L ⊢ e ⇐_place T_p    AddrOfOk(e)    moved = false    Γ; R; L ⊢ ArgsOk(ps, as)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ ArgsOk([⟨⊥, x, T_p⟩] ++ ps, [⟨moved, e, _⟩] ++ as)

RecvArgOk(base, mode) ⇔ (mode = ⊥ ∧ AddrOfOk(base)) ∨ (mode = `move` ∧ ∃ p. base = MoveExpr(p))
ArgsOkDiagRules = RulesIn({"5.2.4"})

**(T-Record-MethodCall)**
RecvBaseType(base, RecvMode(m.receiver)) = P_caller R_rec    LookupMethod(R_rec, name) = m    RecvPerm(R_rec, m.receiver) = P_method    PermSub(P_caller, P_method)    RecvArgOk(base, RecvMode(m.receiver))    Γ; R; L ⊢ ArgsOk(m.params, args)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ MethodCall(base, name, args) : ReturnType(m)

BindParams(m, v_self, vecv) = {`self` ↦ v_self} ∪ { x_i ↦ v_i | m.params = [⟨_, x_i, _⟩], vecv = [v_i] }
BindStmt(x, e) = LetStmt(⟨IdentifierPattern(x), ⊥, Operator("="), e, ⊥⟩)
BindStmts(m, v_self, vecv) = [BindStmt(`self`, v_self)] ++ [BindStmt(x_i, v_i) | m.params = [⟨_, x_i, _⟩], vecv = [v_i]]
ApplyMethod(m, v_self, vecv) = BlockExpr(bs ++ ss, t) ⇔ BindStmts(m, v_self, vecv) = bs ∧ m.body = BlockExpr(ss, t)

**(Step-MethodCall)**
Γ ⊢ v_self : P_caller R    LookupMethod(R, name) = m
────────────────────────────────────────────────────────────────
⟨MethodCall(v_self, name, vecv)⟩ → ⟨ApplyMethod(m, v_self, vecv)⟩


### 5.4. Modal Types (Definitions)

States(M) ≠ ∅
States(M) = { S | S ∈ StateNames(M) }
Payload(M, S) = [⟨f_1, T_1⟩, …, ⟨f_k, T_k⟩]
𝒯_M = {M} ∪ { M@S | S ∈ States(M) }

**Payload Map.**

PayloadMap(M, S) =
 { f_i ↦ T_i | ⟨f_i, T_i⟩ ∈ Payload(M, S) }    if Distinct([f_i | ⟨f_i, T_i⟩ ∈ Payload(M, S)])
 ⊥    otherwise

**(WF-Modal-Payload)**
∀ i, Γ ⊢ T_i wf    ∀ i ≠ j, f_i ≠ f_j
────────────────────────────────────────
Γ ⊢ Payload(M, S) wf

**(Modal-Payload-DupField)**
∃ i ≠ j. f_i = f_j    c = Code(Modal-Payload-DupField)
────────────────────────────────────────────────────────
Γ ⊢ Payload(M, S) wf ⇑ c

**Type Well-Formedness (Cursive0).**

TypesMap = Σ.Types
ClassesMap = Σ.Classes

TypeWFJudg = {Γ ⊢ T wf}

**(WF-Prim)**
T = TypePrim(name)    name ∈ PrimTypes_C0
────────────────────────────────────────
Γ ⊢ T wf

**(WF-Perm)**
T = TypePerm(p, T_0)    Γ ⊢ T_0 wf
────────────────────────────────────────
Γ ⊢ T wf

**(WF-Tuple)**
T = TypeTuple([T_1, …, T_n])    ∀ i, Γ ⊢ T_i wf
────────────────────────────────────────
Γ ⊢ T wf

**(WF-Array)**
T = TypeArray(T_0, e)    Γ ⊢ ConstLen(e) ⇓ n    Γ ⊢ T_0 wf
────────────────────────────────────────────────────────
Γ ⊢ T wf

**(WF-Slice)**
T = TypeSlice(T_0)    Γ ⊢ T_0 wf
────────────────────────────────────────
Γ ⊢ T wf

**(WF-Union)**
T = TypeUnion([T_1, …, T_n])    n ≥ 2    ∀ i, Γ ⊢ T_i wf
────────────────────────────────────────────────────────
Γ ⊢ T wf

**(WF-Union-TooFew)**
T = TypeUnion([T_1, …, T_n])    n < 2    c = Code(WF-Union-TooFew)
──────────────────────────────────────────────────────────────
Γ ⊢ T wf ⇑ c

**(WF-Func)**
T = TypeFunc([⟨m_1, T_1⟩, …, ⟨m_n, T_n⟩], R)    Γ ⊢ R wf    ∀ i, Γ ⊢ T_i wf
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ T wf

**(WF-Path)**
T = TypePath(p)    p ∈ dom(Σ.Types)
────────────────────────────────────────
Γ ⊢ T wf

**(WF-Dynamic)**
T = TypeDynamic(p)    p ∈ dom(Σ.Classes)
────────────────────────────────────────
Γ ⊢ T wf

**(WF-Dynamic-Err)**
T = TypeDynamic(p)    p ∉ dom(Σ.Classes)    c = Code(Superclass-Undefined)
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ T wf ⇑ c

**(WF-String)**
T = TypeString(state_opt)    state_opt ∈ {⊥, `View`, `Managed`}
────────────────────────────────────────────────────────────────
Γ ⊢ T wf


**(WF-Bytes)**
T = TypeBytes(state_opt)    state_opt ∈ {⊥, `View`, `Managed`}
───────────────────────────────────────────────────────────────
Γ ⊢ T wf

**(WF-Ptr)**
T = TypePtr(T_0, state_opt)    state_opt ∈ {⊥, `Valid`, `Null`, `Expired`}    Γ ⊢ T_0 wf
────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ T wf

**(WF-ModalState)**
T = TypeModalState(p, S)    p ∈ dom(Σ.Types)    Σ.Types[p] = `modal` M    S ∈ States(M)
─────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ T wf

**(WF-RawPtr)**
T = TypeRawPtr(q, T_0)    Γ ⊢ T_0 wf
────────────────────────────────────────
Γ ⊢ T wf

**Static Semantics**

**(Modal-WF)**
n ≥ 1    ∀ i ∈ 1..n, S_i unique    ∀ i, Γ ⊢ Payload(M, S_i) wf    ∀ i, S_i ≠ M
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ `modal` M { @S_1 ... @S_n } wf

**(Modal-NoStates-Err)**
n = 0    c = Code(Modal-NoStates-Err)
────────────────────────────────────────────
Γ ⊢ `modal` M { @S_1 ... @S_n } wf ⇑ c

**(Modal-DupState-Err)**
¬ Distinct([S_1, …, S_n])    c = Code(Modal-DupState-Err)
────────────────────────────────────────────────────────────────────
Γ ⊢ `modal` M { @S_1 ... @S_n } wf ⇑ c

**(Modal-StateName-Err)**
∃ i. S_i = M    c = Code(Modal-StateName-Err)
──────────────────────────────────────────────────────────────
Γ ⊢ `modal` M { @S_1 ... @S_n } wf ⇑ c

**(State-Specific-WF)**
S ∈ States(M)
──────────────────────
Γ ⊢ M@S wf

PayloadNames(M, S) = [ f | ⟨f, _⟩ ∈ Payload(M, S) ]
PayloadNameSet(M, S) = Set(PayloadNames(M, S))

**(T-Modal-State-Intro)**
Σ.Types[path] = `modal` M    S ∈ States(M)    path ∉ {["File"], ["DirIter"]}    PayloadNameSet(M, S) = FieldInitSet(fields)    Distinct(FieldInitNames(fields))    ∀ ⟨f, e⟩ ∈ fields, PayloadMap(M, S)(f) = T_f ∧ Γ; R; L ⊢ e ⇐ T_f ⊣ ∅
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ RecordExpr(ModalStateRef(path, S), fields) : TypeModalState(path, S)

**(Modal-Incomparable)**
S_A ≠ S_B
────────────────────────────────────────────
Γ ⊢ M@S_A not<: M@S_B ∧ Γ ⊢ M@S_B not<: M@S_A

#### 5.4.1. Built-in Modal Type `Region` (Cursive0)

["Region"] ∈ dom(Σ.Types)
States(`Region`) = { `@Active`, `@Frozen`, `@Freed` }

RegionPayloadFields = [⟨`handle`, TypePrim("usize")⟩]

Payload(`Region`, `@Active`) = RegionPayloadFields    Payload(`Region`, `@Frozen`) = RegionPayloadFields    Payload(`Region`, `@Freed`) = RegionPayloadFields

RegionProcs = {`Region::new_scoped`, `Region::alloc`, `Region::reset_unchecked`, `Region::freeze`, `Region::thaw`, `Region::free_unchecked`}

RegionProcSig(`Region::new_scoped`) = ⟨[⟨⊥, `options`, TypePath(["RegionOptions"])⟩], TypeModalState(["Region"], `@Active`)⟩
RegionProcSig(`Region::alloc`) = ⟨[⟨⊥, `self`, TypePerm(`unique`, TypeModalState(["Region"], `@Active`))⟩, ⟨⊥, `value`, T⟩], T_{π_Region(`self`)}⟩    (T ∈ Type)
RegionProcSig(`Region::reset_unchecked`) = ⟨[⟨⊥, `self`, TypePerm(`unique`, TypeModalState(["Region"], `@Active`))⟩], TypeModalState(["Region"], `@Active`)⟩
RegionProcSig(`Region::freeze`) = ⟨[⟨⊥, `self`, TypePerm(`unique`, TypeModalState(["Region"], `@Active`))⟩], TypeModalState(["Region"], `@Frozen`)⟩
RegionProcSig(`Region::thaw`) = ⟨[⟨⊥, `self`, TypePerm(`unique`, TypeModalState(["Region"], `@Frozen`))⟩], TypeModalState(["Region"], `@Active`)⟩
RegionProcSig(`Region::free_unchecked`) = ⟨[⟨⊥, `self`, TypePerm(`unique`, TypeUnion([TypeModalState(["Region"], `@Active`), TypeModalState(["Region"], `@Frozen`)]))⟩], TypeModalState(["Region"], `@Freed`)⟩

ProvType(T, π) = T_π
BaseType(T_π) = T    ProvOf(T_π) = π

`Bitcopy` ∉ Implements(TypePath(["Region"]))

**(Region-Unchecked-Unsafe-Err)**
Γ; R; L ⊢ base : T    StripPerm(T) = TypeModalState(["Region"], S)    S ∈ {`Active`, `Frozen`}    name ∈ {"reset_unchecked", "free_unchecked"}    ¬ UnsafeSpan(span(MethodCall(base, name, args)))    c = Code(Region-Unchecked-Unsafe-Err)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ MethodCall(base, name, args) ⇑ c

### 5.5. State-Specific Fields

ModalFieldVisible(m, p) ⇔ Σ.Types[p] = `modal` M ∧ ModuleOfPath(ModalPath(M)) = m

**(T-Modal-Field)**
Γ; R; L ⊢ e : TypeModalState(p, S)    Σ.Types[p] = `modal` M    PayloadMap(M, S)(f) = T    ModalFieldVisible(m, p)
────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ e.f : T

**(T-Modal-Field-Perm)**
Γ; R; L ⊢ e : TypePerm(p', TypeModalState(p, S))    Σ.Types[p] = `modal` M    PayloadMap(M, S)(f) = T    ModalFieldVisible(m, p)
────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ e.f : TypePerm(p', T)

**(Modal-Field-Missing)**
Γ; R; L ⊢ e : TypeModalState(p, S)    Σ.Types[p] = `modal` M    PayloadMap(M, S)(f) undefined    c = Code(Modal-Field-Missing)
────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ e.f ⇑ c

**(Modal-Field-General-Err)**
Γ; R; L ⊢ e : T    StripPerm(T) = TypePath(p)    Σ.Types[p] = `modal` M    c = Code(Modal-Field-General-Err)
────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ e.f ⇑ c

**(Modal-Field-NotVisible)**
Γ; R; L ⊢ e : TypeModalState(p, S)    Σ.Types[p] = `modal` M    PayloadMap(M, S)(f) = T    ¬ ModalFieldVisible(m, p)    c = Code(Modal-Field-NotVisible)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ e.f ⇑ c

### 5.6. Transitions and State-Specific Methods

Methods(S) = [ m | m ∈ StateMembers(S) ∧ ∃ vis, name, params, ret, body, span, doc. m = StateMethodDecl(vis, name, params, ret, body, span, doc) ]
Transitions(S) = [ t | t ∈ StateMembers(S) ∧ ∃ vis, name, params, target, body, span, doc. t = TransitionDecl(vis, name, params, target, body, span, doc) ]
StateMethodNames(S) = [ m.name | m ∈ Methods(S) ]
TransitionNames(S) = [ t.name | t ∈ Transitions(S) ]

**(StateMethod-Dup)**
¬ Distinct(StateMethodNames(S))    c = Code(StateMethod-Dup)
────────────────────────────────────────────────────────────
Γ ⊢ S ⇑ c

**(Transition-Dup)**
¬ Distinct(TransitionNames(S))    c = Code(Transition-Dup)
──────────────────────────────────────────────────────────
Γ ⊢ S ⇑ c

LookupStateMethod(S, name) = m ⇔ m ∈ Methods(S) ∧ m.name = name
LookupStateMethod(S, name) = ⊥ ⇔ ¬ ∃ m ∈ Methods(S). m.name = name
LookupTransition(S, name) = t ⇔ t ∈ Transitions(S) ∧ t.name = name
LookupTransition(S, name) = ⊥ ⇔ ¬ ∃ t ∈ Transitions(S). t.name = name
StateMemberVisible(mod, M, vis) ⇔ vis ∈ {`public`, `internal`} ∨ (vis ∈ {`private`, `protected`} ∧ ModuleOfPath(ModalPath(M)) = mod)
MethodSig(M, S, m).recv = TypePerm(`const`, M@S)
MethodSig(M, S, m).params = m.params
MethodSig(M, S, m).ret = ReturnType(m)
TransitionSig(M, S_src, t).recv = TypePerm(`unique`, M@S_src)
TransitionSig(M, S_src, t).params = t.params
S_tgt = t.target
TransitionSig(M, S_src, t).ret = M@S_tgt
TransitionSig(M, S_src, t).target = S_tgt
TransitionSig(M, S_src, t).mode = `move`

**State Method and Transition Well-Formedness.**

**(WF-State-Method)**
self ∉ ParamNames(md.params)    Distinct(ParamNames(md.params))    ∀ ⟨_, _, T_i⟩ ∈ md.params, Γ ⊢ T_i wf    (md.return_type_opt = ⊥ ∨ Γ ⊢ md.return_type_opt wf)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ md : StateMethodOK(M, S)

**(WF-Transition)**
self ∉ ParamNames(tr.params)    Distinct(ParamNames(tr.params))    ∀ ⟨_, _, T_i⟩ ∈ tr.params, Γ ⊢ T_i wf    tr.target ∈ States(M)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ tr : TransitionOK(M, S_src)

**(Transition-Target-Err)**
tr.target ∉ States(M)    c = Code(Transition-Target-Err)
──────────────────────────────────────────────────────────────
Γ ⊢ tr ⇑ c

**(T-Modal-Transition)**
Γ; R; L ⊢ e_self : `unique` M@S_src    LookupTransition(S_src, t) = tr    StateMemberVisible(mod, M, tr.vis)    TransitionSig(M, S_src, tr).params = ps    TransitionSig(M, S_src, tr).target = S_tgt    Γ; R; L ⊢ ArgsOk(ps, args)    RecvArgOk(e_self, `move`)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ e_self ~> t(args) : M@S_tgt

**(Transition-Source-Err)**
Γ; R; L ⊢ e_self : T    (PermOf(T) ≠ `unique` ∨ StripPerm(T) ≠ TypeModalState(_, _))    c = Code(Transition-Source-Err)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ e_self ~> t(args) ⇑ c

**(Transition-NotVisible)**
Γ; R; L ⊢ e_self : `unique` M@S_src    LookupTransition(S_src, t) = tr    ¬ StateMemberVisible(mod, M, tr.vis)    c = Code(Transition-NotVisible)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ e_self ~> t(args) ⇑ c


**(T-Modal-Transition-Body)**
Σ.Types[p] = `modal` M    S_src ∈ States(M)    tr ∈ Transitions(S_src)    tr.body = body    tr.target = S_tgt    S_tgt ∈ States(M)    Γ_0 = PushScope(Γ)    IntroAll(Γ_0, [⟨`self`, TypePerm(`unique`, TypeModalState(p, S_src))⟩] ++ ParamBinds(tr.params)) ⇓ Γ_1    Γ_1; TypeModalState(p, S_tgt); ⊥ ⊢ body : T_b    Γ ⊢ T_b <: TypeModalState(p, S_tgt)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ tr : TransitionBodyOK(p, S_src)

**(Transition-Body-Err)**
Σ.Types[p] = `modal` M    S_src ∈ States(M)    tr ∈ Transitions(S_src)    tr.body = body    tr.target = S_tgt    S_tgt ∈ States(M)    Γ_0 = PushScope(Γ)    IntroAll(Γ_0, [⟨`self`, TypePerm(`unique`, TypeModalState(p, S_src))⟩] ++ ParamBinds(tr.params)) ⇓ Γ_1    Γ_1; TypeModalState(p, S_tgt); ⊥ ⊢ body : T_b    ¬(Γ ⊢ T_b <: TypeModalState(p, S_tgt))    c = Code(Transition-Body-Err)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ tr ⇑ c

**(T-Modal-Method)**
Γ; R; L ⊢ e : P_caller M@S    PermSub(P_caller, `const`)    LookupStateMethod(S, m) = md    StateMemberVisible(mod, M, md.vis)    MethodSig(M, S, md).params = ps    Γ; R; L ⊢ ArgsOk(ps, args)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ e ~> m(args) : ReturnType(md)

**(Modal-Method-NotFound)**
Γ; R; L ⊢ e : P_caller M@S    LookupStateMethod(S, m) undefined    c = Code(Modal-Method-NotFound)
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ e ~> m(args) ⇑ c

**(Modal-Method-NotVisible)**
Γ; R; L ⊢ e : P_caller M@S    LookupStateMethod(S, m) = md    ¬ StateMemberVisible(mod, M, md.vis)    c = Code(Modal-Method-NotVisible)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ e ~> m(args) ⇑ c

**(T-Modal-Method-Body)**
Σ.Types[p] = `modal` M    S ∈ States(M)    md ∈ Methods(S)    md.body = body    Γ_0 = PushScope(Γ)    IntroAll(Γ_0, [⟨`self`, TypePerm(`const`, TypeModalState(p, S))⟩] ++ ParamBinds(md.params)) ⇓ Γ_1    R_m = ReturnType(md)    Γ_1; R_m; ⊥ ⊢ body : T_b    Γ ⊢ T_b <: R_m    (R_m ≠ TypePrim("()") ⇒ ExplicitReturn(body))
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ md : StateMethodBodyOK(p, S)

### 5.7. Modal Widening (`widen`)

WIDEN_LARGE_PAYLOAD_THRESHOLD_BYTES = 256

**(T-Modal-Widen)**
Γ; R; L ⊢ e : TypeModalState(p, S)    Σ.Types[p] = `modal` M    S ∈ States(M)    Γ ⊢ WarnWidenLargePayload(e, p, S) ⇓ ok
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ `widen` e : TypePath(p)

**(T-Modal-Widen-Perm)**
Γ; R; L ⊢ e : TypePerm(p', TypeModalState(p, S))    Σ.Types[p] = `modal` M    S ∈ States(M)    Γ ⊢ WarnWidenLargePayload(e, p, S) ⇓ ok
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ `widen` e : TypePerm(p', TypePath(p))

**(Widen-AlreadyGeneral)**
Γ; R; L ⊢ e : T    StripPerm(T) = TypePath(p)    Σ.Types[p] = `modal` M    c = Code(Widen-AlreadyGeneral)
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ `widen` e ⇑ c

**(Widen-NonModal)**
Γ; R; L ⊢ e : T    StripPerm(T) = U    U ≠ TypeModalState(_, _)    ¬ ∃ p, M. (U = TypePath(p) ∧ Σ.Types[p] = `modal` M)    c = Code(Widen-NonModal)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ `widen` e ⇑ c

**Niche-Layout-Compatible Conditions**

NicheCompatible(M, S) ⇔ NicheApplies(M) ∧ PayloadState(M) = S ∧ sizeof(M@S) = sizeof(M) ∧ alignof(M@S) = alignof(M)

WidenWarnCond(p, S) ⇔ Σ.Types[p] = `modal` M ∧ sizeof(M@S) > WIDEN_LARGE_PAYLOAD_THRESHOLD_BYTES ∧ ¬ NicheCompatible(M, S)

**(Warn-Widen-LargePayload)**
WidenWarnCond(p, S)    sp = span(Unary("widen", e))    Γ ⊢ Emit(W-SYS-4010, sp)
────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ WarnWidenLargePayload(e, p, S) ⇓ ok

**(Warn-Widen-Ok)**
¬ WidenWarnCond(p, S)
───────────────────────────────────────────────
Γ ⊢ WarnWidenLargePayload(e, p, S) ⇓ ok

**Size Relationship**

sizeof(M@S) ≤ sizeof(M)

### 5.8. String and Bytes Types and States

States(`string`) = { `@Managed`, `@View` }
States(`bytes`) = { `@Managed`, `@View` }

**Modal Widening.**

S ∈ {`@Managed`, `@View`}
──────────────────────────────────────
Γ ⊢ string@S <: string
S ∈ {`@Managed`, `@View`}
────────────────────────────────────
Γ ⊢ bytes@S <: bytes

StringBytesBuiltinTable =
{
 ⟨`string::from`, [⟨⊥, `source`, TypeString(`@View`)⟩, ⟨⊥, `heap`, TypeDynamic(`HeapAllocator`)⟩], TypeUnion([TypeString(`@Managed`), TypePath(["AllocationError"])])⟩,
 ⟨`string::as_view`, [⟨⊥, `self`, TypePerm(`const`, TypeString(`@Managed`))⟩], TypeString(`@View`)⟩,
 ⟨`string::to_managed`, [⟨⊥, `self`, TypePerm(`const`, TypeString(`@View`))⟩, ⟨⊥, `heap`, TypeDynamic(`HeapAllocator`)⟩], TypeUnion([TypeString(`@Managed`), TypePath(["AllocationError"])])⟩,
 ⟨`string::clone_with`, [⟨⊥, `self`, TypePerm(`const`, TypeString(`@Managed`))⟩, ⟨⊥, `heap`, TypeDynamic(`HeapAllocator`)⟩], TypeUnion([TypeString(`@Managed`), TypePath(["AllocationError"])])⟩,
 ⟨`string::append`, [⟨⊥, `self`, TypePerm(`unique`, TypeString(`@Managed`))⟩, ⟨⊥, `data`, TypeString(`@View`)⟩, ⟨⊥, `heap`, TypeDynamic(`HeapAllocator`)⟩], TypeUnion([TypePrim("()"), TypePath(["AllocationError"])])⟩,
 ⟨`string::length`, [⟨⊥, `self`, TypePerm(`const`, TypeString(`@View`))⟩], TypePrim("usize")⟩,
 ⟨`string::is_empty`, [⟨⊥, `self`, TypePerm(`const`, TypeString(`@View`))⟩], TypePrim("bool")⟩,
 ⟨`bytes::with_capacity`, [⟨⊥, `cap`, TypePrim("usize")⟩, ⟨⊥, `heap`, TypeDynamic(`HeapAllocator`)⟩], TypeUnion([TypeBytes(`@Managed`), TypePath(["AllocationError"])])⟩,
 ⟨`bytes::from_slice`, [⟨⊥, `data`, TypePerm(`const`, TypeSlice(TypePrim("u8")))⟩, ⟨⊥, `heap`, TypeDynamic(`HeapAllocator`)⟩], TypeUnion([TypeBytes(`@Managed`), TypePath(["AllocationError"])])⟩,
 ⟨`bytes::as_view`, [⟨⊥, `self`, TypePerm(`const`, TypeBytes(`@Managed`))⟩], TypeBytes(`@View`)⟩,
 ⟨`bytes::to_managed`, [⟨⊥, `self`, TypePerm(`const`, TypeBytes(`@View`))⟩, ⟨⊥, `heap`, TypeDynamic(`HeapAllocator`)⟩], TypeUnion([TypeBytes(`@Managed`), TypePath(["AllocationError"])])⟩,
 ⟨`bytes::view`, [⟨⊥, `data`, TypePerm(`const`, TypeSlice(TypePrim("u8")))⟩], TypeBytes(`@View`)⟩,
 ⟨`bytes::view_string`, [⟨⊥, `data`, TypeString(`@View`)⟩], TypeBytes(`@View`)⟩,
 ⟨`bytes::append`, [⟨⊥, `self`, TypePerm(`unique`, TypeBytes(`@Managed`))⟩, ⟨⊥, `data`, TypeBytes(`@View`)⟩, ⟨⊥, `heap`, TypeDynamic(`HeapAllocator`)⟩], TypeUnion([TypePrim("()"), TypePath(["AllocationError"])])⟩,
 ⟨`bytes::length`, [⟨⊥, `self`, TypePerm(`const`, TypeBytes(`@View`))⟩], TypePrim("usize")⟩,
 ⟨`bytes::is_empty`, [⟨⊥, `self`, TypePerm(`const`, TypeBytes(`@View`))⟩], TypePrim("bool")⟩
}
StringBytesBuiltinSig(method) = ⟨params, ret⟩ ⇔ ⟨method, params, ret⟩ ∈ StringBytesBuiltinTable

ByteSeq = List(`u8`)
SB = ⟨StrBuf, BytesBuf, BytesCap⟩
StrBuf : `string@Managed` ⇀ ByteSeq
BytesBuf : `bytes@Managed` ⇀ ByteSeq
BytesCap : `bytes@Managed` ⇀ `usize`

ViewBytes : (`string@View` ∪ `bytes@View`) → ByteSeq
ByteSeqOf(SB, v) =
 StrBuf(v)    if v:`string@Managed`
 BytesBuf(v)  if v:`bytes@Managed`
 ViewBytes(v) if v:`string@View` or v:`bytes@View`
ByteLen(SB, v) = |ByteSeqOf(SB, v)|

SliceBytes(data) = [b | b ∈ data]

StringBytesJudg = {
 StringFrom(SB, source, heap) ⇓ (r, SB'),
 StringAsView(SB, self) ⇓ v,
 StringToManaged(SB, self, heap) ⇓ (r, SB'),
 StringCloneWith(SB, self, heap) ⇓ (r, SB'),
 StringAppend(SB, self, data, heap) ⇓ (r, SB'),
 StringLength(SB, self) ⇓ n,
 StringIsEmpty(SB, self) ⇓ b,
 BytesWithCapacity(SB, cap, heap) ⇓ (r, SB'),
 BytesFromSlice(SB, data, heap) ⇓ (r, SB'),
 BytesAsView(SB, self) ⇓ v,
 BytesToManaged(SB, self, heap) ⇓ (r, SB'),
 BytesView(SB, data) ⇓ v,
 BytesViewString(SB, data) ⇓ v,
 BytesAppend(SB, self, data, heap) ⇓ (r, SB'),
 BytesLength(SB, self) ⇓ n,
 BytesIsEmpty(SB, self) ⇓ b
}


**(StringFrom-Ok)**
r = v    SB' = ⟨StrBuf[v ↦ ByteSeqOf(SB, source)], BytesBuf, BytesCap⟩
────────────────────────────────────────────────────────────────────────────
Γ ⊢ StringFrom(SB, source, heap) ⇓ (r, SB')

**(StringFrom-Err)**
AllocErrorVal(r)    SB' = SB
────────────────────────────────────────────────────────────────────────────
Γ ⊢ StringFrom(SB, source, heap) ⇓ (r, SB')

**(StringAsView-Ok)**
ByteSeqOf(SB, v) = ByteSeqOf(SB, self)
──────────────────────────────────────────────
Γ ⊢ StringAsView(SB, self) ⇓ v

**(StringToManaged-Ok)**
r = v    SB' = ⟨StrBuf[v ↦ ByteSeqOf(SB, self)], BytesBuf, BytesCap⟩
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ StringToManaged(SB, self, heap) ⇓ (r, SB')

**(StringToManaged-Err)**
AllocErrorVal(r)    SB' = SB
────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ StringToManaged(SB, self, heap) ⇓ (r, SB')

**(StringCloneWith-Ok)**
r = v    SB' = ⟨StrBuf[v ↦ ByteSeqOf(SB, self)], BytesBuf, BytesCap⟩
────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ StringCloneWith(SB, self, heap) ⇓ (r, SB')

**(StringCloneWith-Err)**
AllocErrorVal(r)    SB' = SB
────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ StringCloneWith(SB, self, heap) ⇓ (r, SB')

**(StringAppend-Ok)**
r = ()    StrBuf' = StrBuf[self ↦ ByteSeqOf(SB, self) ++ ByteSeqOf(SB, data)]    SB' = ⟨StrBuf', BytesBuf, BytesCap⟩
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ StringAppend(SB, self, data, heap) ⇓ (r, SB')

**(StringAppend-Err)**
AllocErrorVal(r)    SB' = SB
────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ StringAppend(SB, self, data, heap) ⇓ (r, SB')

**(StringLength)**
n = ByteLen(SB, self)
───────────────────────────────────────
Γ ⊢ StringLength(SB, self) ⇓ n

**(StringIsEmpty)**
b = (ByteLen(SB, self) = 0)
────────────────────────────────────────
Γ ⊢ StringIsEmpty(SB, self) ⇓ b

**(BytesWithCapacity-Ok)**
r = v    BytesBuf' = BytesBuf[v ↦ []]    BytesCap' = BytesCap[v ↦ cap']    cap' ≥ cap    SB' = ⟨StrBuf, BytesBuf', BytesCap'⟩
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BytesWithCapacity(SB, cap, heap) ⇓ (r, SB')

**(BytesWithCapacity-Err)**
AllocErrorVal(r)    SB' = SB
────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BytesWithCapacity(SB, cap, heap) ⇓ (r, SB')

**(BytesFromSlice-Ok)**
r = v    BytesBuf' = BytesBuf[v ↦ SliceBytes(data)]    SB' = ⟨StrBuf, BytesBuf', BytesCap⟩
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BytesFromSlice(SB, data, heap) ⇓ (r, SB')

**(BytesFromSlice-Err)**
AllocErrorVal(r)    SB' = SB
────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BytesFromSlice(SB, data, heap) ⇓ (r, SB')

**(BytesAsView-Ok)**
ByteSeqOf(SB, v) = ByteSeqOf(SB, self)
────────────────────────────────────────────
Γ ⊢ BytesAsView(SB, self) ⇓ v

**(BytesToManaged-Ok)**
r = v    BytesBuf' = BytesBuf[v ↦ ByteSeqOf(SB, self)]    SB' = ⟨StrBuf, BytesBuf', BytesCap⟩
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BytesToManaged(SB, self, heap) ⇓ (r, SB')

**(BytesToManaged-Err)**
AllocErrorVal(r)    SB' = SB
──────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BytesToManaged(SB, self, heap) ⇓ (r, SB')

**(BytesView-Ok)**
ByteSeqOf(SB, v) = SliceBytes(data)
──────────────────────────────────────────
Γ ⊢ BytesView(SB, data) ⇓ v

**(BytesViewString-Ok)**
ByteSeqOf(SB, v) = ByteSeqOf(SB, data)
───────────────────────────────────────────
Γ ⊢ BytesViewString(SB, data) ⇓ v

**(BytesAppend-Ok)**
r = ()    BytesBuf' = BytesBuf[self ↦ ByteSeqOf(SB, self) ++ ByteSeqOf(SB, data)]    SB' = ⟨StrBuf, BytesBuf', BytesCap⟩
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BytesAppend(SB, self, data, heap) ⇓ (r, SB')

**(BytesAppend-Err)**
AllocErrorVal(r)    SB' = SB
────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BytesAppend(SB, self, data, heap) ⇓ (r, SB')

**(BytesLength)**
n = ByteLen(SB, self)
──────────────────────────────────────
Γ ⊢ BytesLength(SB, self) ⇓ n

**(BytesIsEmpty)**
b = (ByteLen(SB, self) = 0)
───────────────────────────────────────
Γ ⊢ BytesIsEmpty(SB, self) ⇓ b

### 5.9. Capabilities and Context (Cursive0)


#### 5.9.1. Capability Access

CapClass = {`FileSystem`, `HeapAllocator`}
CapType(Cl) = TypeDynamic(Cl)

CapMethodSig(`FileSystem`, name) = ⟨params, ret⟩ ⇔ ⟨name, recv, params, ret⟩ ∈ FileSystemInterface
CapMethodSig(`HeapAllocator`, name) = ⟨params, ret⟩ ⇔ ⟨name, recv, params, ret⟩ ∈ HeapAllocatorInterface
CapRecv(`FileSystem`, name) = recv ⇔ ⟨name, recv, params, ret⟩ ∈ FileSystemInterface
CapRecv(`HeapAllocator`, name) = recv ⇔ ⟨name, recv, params, ret⟩ ∈ HeapAllocatorInterface


#### 5.9.2. `FileSystem` Capability Class

BuiltinTypes_FS = {`File`, `DirIter`, `DirEntry`, `FileKind`, `IoError`}

FileSystemInterface =
{
 ⟨"open_read", ~, [⟨⊥, `path`, TypeString(`@View`)⟩], TypeUnion([TypeModalState(["File"], `@Read`), TypePath(["IoError"])])⟩,
 ⟨"open_write", ~, [⟨⊥, `path`, TypeString(`@View`)⟩], TypeUnion([TypeModalState(["File"], `@Write`), TypePath(["IoError"])])⟩,
 ⟨"open_append", ~, [⟨⊥, `path`, TypeString(`@View`)⟩], TypeUnion([TypeModalState(["File"], `@Append`), TypePath(["IoError"])])⟩,
 ⟨"create_write", ~, [⟨⊥, `path`, TypeString(`@View`)⟩], TypeUnion([TypeModalState(["File"], `@Write`), TypePath(["IoError"])])⟩,
 ⟨"read_file", ~, [⟨⊥, `path`, TypeString(`@View`)⟩], TypeUnion([TypeString(`@Managed`), TypePath(["IoError"])])⟩,
 ⟨"read_bytes", ~, [⟨⊥, `path`, TypeString(`@View`)⟩], TypeUnion([TypeBytes(`@Managed`), TypePath(["IoError"])])⟩,
 ⟨"write_file", ~, [⟨⊥, `path`, TypeString(`@View`)⟩, ⟨⊥, `data`, TypeBytes(`@View`)⟩], TypeUnion([TypePrim("()"), TypePath(["IoError"])])⟩,
 ⟨"write_stdout", ~, [⟨⊥, `data`, TypeString(`@View`)⟩], TypeUnion([TypePrim("()"), TypePath(["IoError"])])⟩,
 ⟨"write_stderr", ~, [⟨⊥, `data`, TypeString(`@View`)⟩], TypeUnion([TypePrim("()"), TypePath(["IoError"])])⟩,
 ⟨"exists", ~, [⟨⊥, `path`, TypeString(`@View`)⟩], TypePrim("bool")⟩,
 ⟨"remove", ~, [⟨⊥, `path`, TypeString(`@View`)⟩], TypeUnion([TypePrim("()"), TypePath(["IoError"])])⟩,
 ⟨"open_dir", ~, [⟨⊥, `path`, TypeString(`@View`)⟩], TypeUnion([TypeModalState(["DirIter"], `@Open`), TypePath(["IoError"])])⟩,
 ⟨"create_dir", ~, [⟨⊥, `path`, TypeString(`@View`)⟩], TypeUnion([TypePrim("()"), TypePath(["IoError"])])⟩,
 ⟨"ensure_dir", ~, [⟨⊥, `path`, TypeString(`@View`)⟩], TypeUnion([TypePrim("()"), TypePath(["IoError"])])⟩,
 ⟨"kind", ~, [⟨⊥, `path`, TypeString(`@View`)⟩], TypeUnion([TypePath(["FileKind"]), TypePath(["IoError"])])⟩,
 ⟨"restrict", ~, [⟨⊥, `path`, TypeString(`@View`)⟩], TypeDynamic(`FileSystem`)⟩
}

Variants(`FileKind`) = [`File`, `Dir`, `Other`]

Implements(`FileKind`) = [`Bitcopy`]

Fields(`DirEntry`) = [⟨`name`, TypeString(`@Managed`)⟩, ⟨`path`, TypeString(`@Managed`)⟩, ⟨`kind`, TypePath(["FileKind"])⟩]

**DirIter Modal Type.**

States(`DirIter`) = {`@Open`, `@Closed`}

Payload(`DirIter`, `@Open`) = [⟨`handle`, `usize`⟩]
Payload(`DirIter`, `@Closed`) = []

DirIterStateMembers =
{
 ⟨`@Open`, "next", `method`, [], TypeUnion([TypePath(["DirEntry"]), TypePrim("()"), TypePath(["IoError"])])⟩,
 ⟨`@Open`, "close", `transition`, [], TypeModalState(["DirIter"], `@Closed`)⟩
}

**File Modal Type.**

States(`File`) = {`@Read`, `@Write`, `@Append`, `@Closed`}

Payload(`File`, `@Read`) = [⟨`handle`, `usize`⟩]
Payload(`File`, `@Write`) = [⟨`handle`, `usize`⟩]
Payload(`File`, `@Append`) = [⟨`handle`, `usize`⟩]
Payload(`File`, `@Closed`) = []

FileStateMembers =
{
 ⟨`@Read`, "read_all", `method`, [], TypeUnion([TypeString(`@Managed`), TypePath(["IoError"])])⟩,
 ⟨`@Read`, "read_all_bytes", `method`, [], TypeUnion([TypeBytes(`@Managed`), TypePath(["IoError"])])⟩,
 ⟨`@Read`, "close", `transition`, [], TypeModalState(["File"], `@Closed`)⟩,
 ⟨`@Write`, "write", `method`, [⟨⊥, `data`, TypeBytes(`@View`)⟩], TypeUnion([TypePrim("()"), TypePath(["IoError"])])⟩,
 ⟨`@Write`, "flush", `method`, [], TypeUnion([TypePrim("()"), TypePath(["IoError"])])⟩,
 ⟨`@Write`, "close", `transition`, [], TypeModalState(["File"], `@Closed`)⟩,
 ⟨`@Append`, "write", `method`, [⟨⊥, `data`, TypeBytes(`@View`)⟩], TypeUnion([TypePrim("()"), TypePath(["IoError"])])⟩,
 ⟨`@Append`, "flush", `method`, [], TypeUnion([TypePrim("()"), TypePath(["IoError"])])⟩,
 ⟨`@Append`, "close", `transition`, [], TypeModalState(["File"], `@Closed`)⟩
}

Variants(`IoError`) = [`NotFound`, `PermissionDenied`, `AlreadyExists`, `InvalidPath`, `Busy`, `IoFailure`]

Implements(`IoError`) = [`Bitcopy`]

**(Record-FileDir-Err)**
path ∈ {["File"], ["DirIter"]}    c = Code(Record-FileDir-Err)
────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ RecordExpr(ModalStateRef(path, S), fields) ⇑ c

#### 5.9.3. `HeapAllocator` Capability Class

HeapAllocatorInterface =
{
 ⟨"with_quota", ~!, [⟨⊥, `size`, TypePrim("usize")⟩], TypeDynamic(`HeapAllocator`)⟩,
 ⟨"alloc_raw", ~, [⟨⊥, `count`, TypePrim("usize")⟩], TypeRawPtr(`mut`, TypePrim("u8"))⟩,
 ⟨"dealloc_raw", ~, [⟨⊥, `ptr`, TypeRawPtr(`mut`, TypePrim("u8"))⟩, ⟨⊥, `count`, TypePrim("usize")⟩], TypePrim("()")⟩
}

**(AllocRaw-Unsafe-Err)**
Γ; R; L ⊢ base : TypeDynamic(`HeapAllocator`)    ¬ UnsafeSpan(span(MethodCall(base, "alloc_raw", args)))    c = Code(AllocRaw-Unsafe-Err)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ MethodCall(base, "alloc_raw", args) ⇑ c

**(DeallocRaw-Unsafe-Err)**
Γ; R; L ⊢ base : TypeDynamic(`HeapAllocator`)    ¬ UnsafeSpan(span(MethodCall(base, "dealloc_raw", args)))    c = Code(DeallocRaw-Unsafe-Err)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ; R; L ⊢ MethodCall(base, "dealloc_raw", args) ⇑ c

Variants(`AllocationError`) = [`OutOfMemory`, `QuotaExceeded`]
VariantPayload(`AllocationError`, `OutOfMemory`) = TuplePayload([TypePrim("usize")])
VariantPayload(`AllocationError`, `QuotaExceeded`) = TuplePayload([TypePrim("usize")])
AllocErrorVal(r) ⇔ ∃ s. r = EnumValue(["AllocationError", "OutOfMemory"], TuplePayload([s])) ∨ r = EnumValue(["AllocationError", "QuotaExceeded"], TuplePayload([s]))

#### 5.9.4. `Context` Record (Cursive0)

BuiltinRecord ⊇ {`Context`, `System`}

Fields(`Context`) = [⟨`fs`, TypeDynamic(`FileSystem`)⟩, ⟨`heap`, TypeDynamic(`HeapAllocator`)⟩, ⟨`sys`, TypePath(["System"])⟩]

Implements(`Context`) = [`Bitcopy`]
Implements(`System`) = [`Bitcopy`]

SystemInterface =
{
 ⟨"exit", [⟨⊥, `code`, TypePrim("i32")⟩], TypePrim("!")⟩,
 ⟨"get_env", [⟨⊥, `key`, TypeString(`@View`)⟩], TypeUnion([TypeString(⊥), TypePrim("()")])⟩
}
SystemMethodSig(name) = ⟨params, ret⟩ ⇔ ⟨name, params, ret⟩ ∈ SystemInterface
### 5.10. Enum Discriminant Defaults

Variants(E) = E.variants
disc_opt(v) = v.discriminant_opt

DiscValue(tok) = IntValue(tok)

DiscOf(v, n) =
 n    if disc_opt(v) = ⊥
 DiscValue(tok)    if disc_opt(v) = tok
DiscSeq([], n) = []
DiscSeq(v::vs, n) = [DiscOf(v, n)] ++ DiscSeq(vs, DiscOf(v, n) + 1)

EnumDiscriminants(E) ⇓ ds ⇔ ds = DiscSeq(Variants(E), 0) ∧ Distinct(ds) ∧ ∀ d ∈ ds. d ≥ 0

**(Enum-Disc-NotInt)**
∃ v ∈ Variants(E). disc_opt(v) = tok    tok.kind ≠ IntLiteral    c = Code(Enum-Disc-NotInt)
────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EnumDiscriminants(E) ⇑ c

**(Enum-Disc-Invalid)**
∃ v ∈ Variants(E). disc_opt(v) = tok    DiscValue(tok) undefined    c = Code(Enum-Disc-Invalid)
──────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EnumDiscriminants(E) ⇑ c

**(Enum-Disc-Negative)**
∃ v ∈ Variants(E). disc_opt(v) = tok    DiscValue(tok) = d    d < 0    c = Code(Enum-Disc-Negative)
───────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EnumDiscriminants(E) ⇑ c

**(Enum-Disc-Dup)**
ds = DiscSeq(Variants(E), 0)    ¬ Distinct(ds)    c = Code(Enum-Disc-Dup)
───────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EnumDiscriminants(E) ⇑ c

MaxDisc(E) = max(ds) ⇔ EnumDiscriminants(E) ⇓ ds

DiscType(E) =
 `u8`    if 0 ≤ MaxDisc(E) ≤ 255
 `u16`   if 256 ≤ MaxDisc(E) ≤ 65,535
 `u32`   if 65,536 ≤ MaxDisc(E) ≤ 4,294,967,295
 `u64`   otherwise


### 5.11. Foundational Classes (Cursive0)

**Class Signatures (built-in).**

```cursive
class Drop {
    procedure drop(~!)
}

class Bitcopy { }

class Clone {
    procedure clone(~) -> Self
}
```

BitcopyDropJudg = {Γ ⊢ T : BitcopyDropOk}

ImplementsBitcopy(T) ⇔ `Bitcopy` ∈ Implements(T)
ImplementsDrop(T) ⇔ `Drop` ∈ Implements(T)
ImplementsClone(T) ⇔ `Clone` ∈ Implements(T)

**(BitcopyDrop-Ok)**
¬(ImplementsBitcopy(T) ∧ ImplementsDrop(T))    (ImplementsBitcopy(T) ⇒ ImplementsClone(T))    (ImplementsBitcopy(T) ⇒ ∀ f : T_f ∈ Fields(T). BitcopyType(T_f))
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ T : BitcopyDropOk

**(BitcopyDrop-Conflict)**
ImplementsBitcopy(T) ∧ ImplementsDrop(T)    c = Code(BitcopyDrop-Conflict)
─────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ T : BitcopyDropOk ⇑ c

**(Bitcopy-Clone-Missing)**
ImplementsBitcopy(T)    ¬ ImplementsClone(T)    c = Code(Bitcopy-Clone-Missing)
──────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ T : BitcopyDropOk ⇑ c

**(Bitcopy-Field-NonBitcopy)**
ImplementsBitcopy(T)    ∃ f : T_f ∈ Fields(T). ¬ BitcopyType(T_f)    c = Code(Bitcopy-Field-NonBitcopy)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ T : BitcopyDropOk ⇑ c

BitcopyTypeCore(T) ⇔
 false    if T = TypePerm(`unique`, _)
 BuiltinBitcopyType(T_0) ∨ Γ ⊢ T_0 <: `Bitcopy`    if T = TypePerm(p, T_0) ∧ p ≠ `unique`
 BuiltinBitcopyType(T) ∨ Γ ⊢ T <: `Bitcopy`    otherwise

BuiltinBitcopyType(T) ⇔
 T = TypePrim(t) ∧ t ∈ PrimTypes_C0 ∨
 T = TypePtr(U, s) ∨
 T = TypeRawPtr(q, U) ∨
 T = TypeSlice(U) ∨
 T = TypeFunc(ps, R) ∨
 T = TypeDynamic(Cl) ∨
 T = TypeRange ∨
 T = TypeString(`@View`) ∨
 T = TypeBytes(`@View`)

BuiltinDropType(T) ⇔ T = TypeString(`@Managed`) ∨ T = TypeBytes(`@Managed`)

BuiltinCloneType(T) ⇔ BuiltinBitcopyType(T)

### 5.12. Initialization Planning

**Module Prefix Resolution.**
P = Project(Γ)
m = CurrentModule(Γ)
Modules = P.modules
PathPrefix(path, pref) ⇔ ∃ rest. path = pref ++ rest

**Alias Expansion.**

**(AliasExpand-None)**
path = a::rest    a ∉ dom(AliasMap(m))
──────────────────────────────────────────────────────────────
Γ ⊢ AliasExpand(path, AliasMap(m)) ⇓ path

**(AliasExpand-Yes)**
path = a::rest    a ∈ dom(AliasMap(m))    AliasMap(m)[a] = p_a
────────────────────────────────────────────────────────────────────────
Γ ⊢ AliasExpand(path, AliasMap(m)) ⇓ p_a ++ rest

**(ModulePrefix-Direct)**
Γ ⊢ AliasExpand(path, AliasMap(m)) ⇓ path'    ∃ p ∈ Modules, PathPrefix(path', p)    p = argmax_{q ∈ Modules, PathPrefix(path', q)} |q|
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ModulePrefix(path, Modules, AliasMap(m)) ⇓ p

**(ModulePrefix-None)**
Γ ⊢ AliasExpand(path, AliasMap(m)) ⇓ path'    ¬ ∃ p ∈ Modules. PathPrefix(path', p)
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ModulePrefix(path, Modules, AliasMap(m)) ↑

env = ⟨self, Modules, AliasMap(m), UsingValueMap, UsingTypeMap⟩

**(Reachable-Edge)**
(u, v) ∈ E
──────────────────────────
Γ ⊢ Reachable(u, v, E)

**(Reachable-Step)**
(u, w) ∈ E    Γ ⊢ Reachable(w, v, E)
────────────────────────────────────
Γ ⊢ Reachable(u, v, E)

**Type References.**
FullPath(path, name) = path ++ [name]
EnumPath(path) = p ⇔ SplitLast(path) = (p, n)
VariantName(path) = n ⇔ SplitLast(path) = (p, n)

TypeRefsJudg = {TypeRefsTy, TypeRefsRef, TypeRefsExpr, TypeRefsPat}
Modules = env.Modules
Alias = env.Alias
UsingTypeMap = env.UsingTypeMap

**(TypeRef-Path)**
|path| ≥ 2    Γ ⊢ ModulePrefix(path, Modules, Alias) ⇓ mp    mp ≠ env.self
────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TypeRefsTy(TypePath(path), env) ⇓ {mp}

**(TypeRef-Using)**
path = [name]    name ∈ dom(UsingTypeMap)    UsingTypeMap[name] ≠ env.self
────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TypeRefsTy(TypePath(path), env) ⇓ {UsingTypeMap[name]}

**(TypeRef-Path-Local)**
(|path| ≠ 1 ∨ (path = [name] ∧ name ∉ dom(UsingTypeMap)))    (Γ ⊢ ModulePrefix(path, Modules, Alias) ⇑ ∨ Γ ⊢ ModulePrefix(path, Modules, Alias) ⇓ env.self)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TypeRefsTy(TypePath(path), env) ⇓ ∅

**(TypeRef-Dynamic)**
Γ ⊢ TypeRefsTy(TypePath(path), env) ⇓ T
────────────────────────────────────────────────────────────
Γ ⊢ TypeRefsTy(TypeDynamic(path), env) ⇓ T

**(TypeRef-ModalState)**
Γ ⊢ TypeRefsTy(TypePath(path), env) ⇓ T
──────────────────────────────────────────────────────────────────
Γ ⊢ TypeRefsTy(TypeModalState(path, state), env) ⇓ T

**(TypeRef-Perm)**
Γ ⊢ TypeRefsTy(base, env) ⇓ T
────────────────────────────────────────────────────────
Γ ⊢ TypeRefsTy(TypePerm(perm, base), env) ⇓ T

**(TypeRef-Prim)**
────────────────────────────────────────────────────────
Γ ⊢ TypeRefsTy(TypePrim(_), env) ⇓ ∅

**(TypeRef-Tuple)**
∀ i, Γ ⊢ TypeRefsTy(t_i, env) ⇓ T_i
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TypeRefsTy(TypeTuple([t_1, …, t_n]), env) ⇓ ⋃_{i=1}^n T_i

**(TypeRef-Array)**
Γ ⊢ TypeRefsTy(elem, env) ⇓ T_e    Γ ⊢ TypeRefsExpr(size_expr, env) ⇓ T_s
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TypeRefsTy(TypeArray(elem, size_expr), env) ⇓ T_e ∪ T_s

**(TypeRef-Slice)**
Γ ⊢ TypeRefsTy(elem, env) ⇓ T
────────────────────────────────────────────────────────
Γ ⊢ TypeRefsTy(TypeSlice(elem), env) ⇓ T

**(TypeRef-Union)**
∀ i, Γ ⊢ TypeRefsTy(t_i, env) ⇓ T_i
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TypeRefsTy(TypeUnion([t_1, …, t_n]), env) ⇓ ⋃_{i=1}^n T_i

**(TypeRef-Func)**
∀ i, params_i = ⟨m_i, t_i⟩    Γ ⊢ TypeRefsTy(t_i, env) ⇓ T_i    Γ ⊢ TypeRefsTy(ret, env) ⇓ T_r
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TypeRefsTy(TypeFunc([params_1, …, params_n], ret), env) ⇓ (⋃_{i=1}^n T_i) ∪ T_r

**(TypeRef-String)**
────────────────────────────────────────────────────────
Γ ⊢ TypeRefsTy(TypeString(_), env) ⇓ ∅

**(TypeRef-Bytes)**
───────────────────────────────────────────────────────
Γ ⊢ TypeRefsTy(TypeBytes(_), env) ⇓ ∅

**(TypeRef-Ptr)**
Γ ⊢ TypeRefsTy(elem, env) ⇓ T
────────────────────────────────────────────────────
Γ ⊢ TypeRefsTy(TypePtr(elem, _), env) ⇓ T

**(TypeRef-RawPtr)**
Γ ⊢ TypeRefsTy(elem, env) ⇓ T
────────────────────────────────────────────────────────
Γ ⊢ TypeRefsTy(TypeRawPtr(_, elem), env) ⇓ T

**(TypeRef-Range)**
──────────────────────────────────────────────────────
Γ ⊢ TypeRefsTy(TypeRange, env) ⇓ ∅

**(TypeRef-Ref-Path)**
Γ ⊢ TypeRefsTy(TypePath(path), env) ⇓ T
────────────────────────────────────────────────────────────
Γ ⊢ TypeRefsRef(TypePath(path), env) ⇓ T

**(TypeRef-Ref-ModalState)**
Γ ⊢ TypeRefsTy(TypeModalState(path, state), env) ⇓ T
────────────────────────────────────────────────────────────────
Γ ⊢ TypeRefsRef(ModalStateRef(path, state), env) ⇓ T

**(TypeRef-RecordExpr)**
Γ ⊢ TypeRefsRef(r, env) ⇓ T_t    Γ ⊢ TypeRefsExprs(fields, env) ⇓ T_e
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TypeRefsExpr(RecordExpr(r, fields), env) ⇓ T_t ∪ T_e

**(TypeRef-EnumLiteral)**
Γ ⊢ TypeRefsTy(TypePath(EnumPath(path)), env) ⇓ T_t    Γ ⊢ TypeRefsEnumPayload(payload_opt, env) ⇓ T_p
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TypeRefsExpr(EnumLiteral(path, payload_opt), env) ⇓ T_t ∪ T_p

**(TypeRef-QualBrace)**
Γ ⊢ TypeRefsTy(TypePath(FullPath(path, name)), env) ⇓ T_t    Γ ⊢ TypeRefsExprs(fields, env) ⇓ T_f
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TypeRefsExpr(QualifiedApply(path, name, Brace(fields)), env) ⇓ T_t ∪ T_f

**(TypeRef-Cast)**
Γ ⊢ TypeRefsExpr(e, env) ⇓ T_e    Γ ⊢ TypeRefsTy(ty, env) ⇓ T_t
────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TypeRefsExpr(Cast(e, ty), env) ⇓ T_e ∪ T_t

**(TypeRef-Transmute)**
Γ ⊢ TypeRefsExpr(e, env) ⇓ T_e    Γ ⊢ TypeRefsTy(t_1, env) ⇓ T_1    Γ ⊢ TypeRefsTy(t_2, env) ⇓ T_2
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TypeRefsExpr(TransmuteExpr(t_1, t_2, e), env) ⇓ T_e ∪ T_1 ∪ T_2

TypeRefsExprRules = {TypeRef-RecordExpr, TypeRef-EnumLiteral, TypeRef-QualBrace, TypeRef-Cast, TypeRef-Transmute, TypeRef-Expr-Sub}
NoSpecificTypeRefsExpr(e) ⇔ ¬ ∃ r ∈ TypeRefsExprRules \ {TypeRef-Expr-Sub}. PremisesHold(r, e)

**(TypeRef-Expr-Sub)**
NoSpecificTypeRefsExpr(e)    Children_LTR(e) = [e_1, …, e_n]    ∀ i, Γ ⊢ TypeRefsExpr(e_i, env) ⇓ T_i
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TypeRefsExpr(e, env) ⇓ ⋃_{i=1}^n T_i

**(TypeRef-RecordPattern)**
Γ ⊢ TypeRefsTy(TypePath(tp), env) ⇓ T_t    Γ ⊢ TypeRefsFields(fields, env) ⇓ T_f
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TypeRefsPat(RecordPattern(tp, fields), env) ⇓ T_t ∪ T_f

**(TypeRef-EnumPattern)**
Γ ⊢ TypeRefsTy(TypePath(tp), env) ⇓ T_t    Γ ⊢ TypeRefsPayload(payload, env) ⇓ T_p
─────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TypeRefsPat(EnumPattern(tp, _, payload), env) ⇓ T_t ∪ T_p

**(TypeRef-LiteralPattern)**
──────────────────────────────────────────────────────────────
Γ ⊢ TypeRefsPat(LiteralPattern(_), env) ⇓ ∅

**(TypeRef-WildcardPattern)**
───────────────────────────────────────────────────────────
Γ ⊢ TypeRefsPat(WildcardPattern, env) ⇓ ∅

**(TypeRef-IdentifierPattern)**
───────────────────────────────────────────────────────────────
Γ ⊢ TypeRefsPat(IdentifierPattern(_), env) ⇓ ∅

**(TypeRef-TypedPattern)**
────────────────────────────────────────────────────────────
Γ ⊢ TypeRefsPat(TypedPattern(_, _), env) ⇓ ∅

**(TypeRef-TuplePattern)**
∀ i, Γ ⊢ TypeRefsPat(p_i, env) ⇓ T_i
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TypeRefsPat(TuplePattern([p_1, …, p_n]), env) ⇓ ⋃_{i=1}^n T_i

**(TypeRef-ModalPattern-None)**
────────────────────────────────────────────────────────────
Γ ⊢ TypeRefsPat(ModalPattern(_, ⊥), env) ⇓ ∅

**(TypeRef-ModalPattern-Record)**
Γ ⊢ TypeRefsFields(fields, env) ⇓ T
────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TypeRefsPat(ModalPattern(_, ModalRecordPayload(fields)), env) ⇓ T

**(TypeRef-RangePattern)**
Γ ⊢ TypeRefsPat(p_l, env) ⇓ T_l    Γ ⊢ TypeRefsPat(p_h, env) ⇓ T_h
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TypeRefsPat(RangePattern(_, p_l, p_h), env) ⇓ T_l ∪ T_h

**(TypeRef-Field-Explicit)**
Γ ⊢ TypeRefsPat(p, env) ⇓ T
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TypeRefsPat(⟨name, pattern_opt=p, span⟩, env) ⇓ T

**(TypeRef-Field-Implicit)**
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TypeRefsPat(⟨name, pattern_opt=⊥, span⟩, env) ⇓ ∅

**(TypeRefsExprs-Empty)**
────────────────────────────────────────────────────────
Γ ⊢ TypeRefsExprs([], env) ⇓ ∅

**(TypeRefsExprs-Cons)**
f = ⟨name, e⟩    Γ ⊢ TypeRefsExpr(e, env) ⇓ T_e    Γ ⊢ TypeRefsExprs(fs, env) ⇓ T_f
────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TypeRefsExprs(f::fs, env) ⇓ T_e ∪ T_f

**(TypeRefsEnumPayload-None)**
──────────────────────────────────────────────────────────────
Γ ⊢ TypeRefsEnumPayload(⊥, env) ⇓ ∅

**(TypeRefsEnumPayload-Tuple)**
∀ i, Γ ⊢ TypeRefsExpr(e_i, env) ⇓ T_i
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TypeRefsEnumPayload(Paren([e_1, …, e_n]), env) ⇓ ⋃_{i=1}^n T_i

**(TypeRefsEnumPayload-Record)**
Γ ⊢ TypeRefsExprs(fields, env) ⇓ T
────────────────────────────────────────────────────────────
Γ ⊢ TypeRefsEnumPayload(Brace(fields), env) ⇓ T

**(TypeRefsFields-Empty)**
────────────────────────────────────────────────────────
Γ ⊢ TypeRefsFields([], env) ⇓ ∅

**(TypeRefsFields-Cons)**
Γ ⊢ TypeRefsPat(f, env) ⇓ T_f    Γ ⊢ TypeRefsFields(fs, env) ⇓ T_s
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TypeRefsFields(f::fs, env) ⇓ T_f ∪ T_s

**(TypeRefsPayload-None)**
────────────────────────────────────────────────────────
Γ ⊢ TypeRefsPayload(⊥, env) ⇓ ∅

**(TypeRefsPayload-Tuple)**
∀ i, Γ ⊢ TypeRefsPat(p_i, env) ⇓ T_i
────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TypeRefsPayload(TuplePayloadPattern([p_1, …, p_n]), env) ⇓ ⋃_{i=1}^n T_i

**(TypeRefsPayload-Record)**
Γ ⊢ TypeRefsFields(fields, env) ⇓ T
───────────────────────────────────────────────────────────────────────────────
Γ ⊢ TypeRefsPayload(RecordPayloadPattern(fields), env) ⇓ T

**Value References.**

UsingValueMap = env.UsingValueMap
ValueRefsJudg = {ValueRefs, ValueRefsArgs, ValueRefsFields}

**(ValueRef-Ident)**
name ∈ dom(UsingValueMap)    UsingValueMap[name] ≠ env.self
──────────────────────────────────────────────────────────────
Γ ⊢ ValueRefs(Identifier(name), env) ⇓ {UsingValueMap[name]}

**(ValueRef-Ident-Local)**
name ∉ dom(UsingValueMap)
────────────────────────────────────────────────────
Γ ⊢ ValueRefs(Identifier(name), env) ⇓ ∅

**(ValueRef-Qual)**
Γ ⊢ ModulePrefix(path, Modules, Alias) ⇓ mp    mp ≠ env.self
────────────────────────────────────────────────────────────────────────────
Γ ⊢ ValueRefs(QualifiedName(path, _), env) ⇓ {mp}

**(ValueRef-Qual-Local)**
Γ ⊢ ModulePrefix(path, Modules, Alias) ⇑ ∨ Γ ⊢ ModulePrefix(path, Modules, Alias) ⇓ env.self
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ValueRefs(QualifiedName(path, _), env) ⇓ ∅

**(ValueRef-QualApply)**
Γ ⊢ ModulePrefix(path, Modules, Alias) ⇓ mp    mp ≠ env.self    Γ ⊢ ValueRefsArgs(args, env) ⇓ V_a
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ValueRefs(QualifiedApply(path, _, Paren(args)), env) ⇓ {mp} ∪ V_a

**(ValueRef-QualApply-Local)**
(Γ ⊢ ModulePrefix(path, Modules, Alias) ⇑ ∨ Γ ⊢ ModulePrefix(path, Modules, Alias) ⇓ env.self)    Γ ⊢ ValueRefsArgs(args, env) ⇓ V_a
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ValueRefs(QualifiedApply(path, _, Paren(args)), env) ⇓ V_a

**(ValueRef-QualApply-Brace)**
Γ ⊢ ValueRefsFields(fields, env) ⇓ V_f
────────────────────────────────────────────────────────────────────────────
Γ ⊢ ValueRefs(QualifiedApply(path, _, Brace(fields)), env) ⇓ V_f

ValueRefsRules = {ValueRef-Ident, ValueRef-Ident-Local, ValueRef-Qual, ValueRef-Qual-Local, ValueRef-QualApply, ValueRef-QualApply-Local, ValueRef-QualApply-Brace, ValueRef-Expr-Sub}
NoSpecificValueRefsExpr(e) ⇔ ¬ ∃ r ∈ ValueRefsRules \ {ValueRef-Expr-Sub}. PremisesHold(r, e)

**(ValueRef-Expr-Sub)**
NoSpecificValueRefsExpr(e)    Children_LTR(e) = [e_1, …, e_n]    ∀ i, Γ ⊢ ValueRefs(e_i, env) ⇓ V_i
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ValueRefs(e, env) ⇓ ⋃_{i=1}^n V_i

**(ValueRefsArgs-Empty)**
────────────────────────────────────────────────────────
Γ ⊢ ValueRefsArgs([], env) ⇓ ∅

**(ValueRefsArgs-Cons)**
a = ⟨moved, e, span⟩    Γ ⊢ ValueRefs(e, env) ⇓ V_e    Γ ⊢ ValueRefsArgs(args, env) ⇓ V_a
────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ValueRefsArgs(a::args, env) ⇓ V_e ∪ V_a

**(ValueRefsFields-Empty)**
────────────────────────────────────────────────────────
Γ ⊢ ValueRefsFields([], env) ⇓ ∅

**(ValueRefsFields-Cons)**
f = ⟨name, e⟩    Γ ⊢ ValueRefs(e, env) ⇓ V_e    Γ ⊢ ValueRefsFields(fs, env) ⇓ V_f
───────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ValueRefsFields(f::fs, env) ⇓ V_e ∪ V_f

**Dependency Sets.**

env_m = ⟨self=m, Modules=P.modules, Alias_m, UsingValueMap_m, UsingTypeMap_m⟩
Alias_m = AliasMap(m)
UsingValueMap_m = UsingValueMap(m)
UsingTypeMap_m = UsingTypeMap(m)
ASTModule(P, m) = ModuleMap(P, m)

TypeOptSet(⊥) = ∅
TypeOptSet(T) = {T}
ParamTypeSet(params) = { t | ∃ mode, name. ⟨mode, name, t⟩ ∈ params }
RecvTypeSet(ReceiverExplicit(_, t)) = {t}
RecvTypeSet(ReceiverShorthand(_)) = ∅
ClassPathTypeSet(paths) = { TypePath(p) | p ∈ paths }

RecordFieldTypeSet(members) = { t | ∃ vis, name, init, span, doc. FieldDecl(vis, name, t, init, span, doc) ∈ members }
RecordMethodRecvTypes(members) = { t | ∃ vis, ov, name, recv, params, ret, body, span, doc. MethodDecl(vis, ov, name, recv, params, ret, body, span, doc) ∈ members ∧ t ∈ RecvTypeSet(recv) }
RecordMethodParamTypes(members) = { t | ∃ vis, ov, name, recv, params, ret, body, span, doc. MethodDecl(vis, ov, name, recv, params, ret, body, span, doc) ∈ members ∧ t ∈ ParamTypeSet(params) }
RecordMethodRetTypes(members) = { t | ∃ vis, ov, name, recv, params, ret, body, span, doc. MethodDecl(vis, ov, name, recv, params, ret, body, span, doc) ∈ members ∧ t ∈ TypeOptSet(ret) }
RecordMemberTypeSet(members) = RecordFieldTypeSet(members) ∪ RecordMethodRecvTypes(members) ∪ RecordMethodParamTypes(members) ∪ RecordMethodRetTypes(members)

ClassFieldTypeSet(items) = { t | ∃ vis, name, span, doc. ClassFieldDecl(vis, name, t, span, doc) ∈ items }
ClassMethodRecvTypes(items) = { t | ∃ vis, name, recv, params, ret, body, span, doc. ClassMethodDecl(vis, name, recv, params, ret, body, span, doc) ∈ items ∧ t ∈ RecvTypeSet(recv) }
ClassMethodParamTypes(items) = { t | ∃ vis, name, recv, params, ret, body, span, doc. ClassMethodDecl(vis, name, recv, params, ret, body, span, doc) ∈ items ∧ t ∈ ParamTypeSet(params) }
ClassMethodRetTypes(items) = { t | ∃ vis, name, recv, params, ret, body, span, doc. ClassMethodDecl(vis, name, recv, params, ret, body, span, doc) ∈ items ∧ t ∈ TypeOptSet(ret) }
ClassItemTypeSet(items) = ClassFieldTypeSet(items) ∪ ClassMethodRecvTypes(items) ∪ ClassMethodParamTypes(items) ∪ ClassMethodRetTypes(items)

VariantPayloadTypeSet(⊥) = ∅
VariantPayloadTypeSet(TuplePayload(tys)) = { t | t ∈ tys }
VariantPayloadTypeSet(RecordPayload(fields)) = { t | ∃ vis, name, init, span, doc. FieldDecl(vis, name, t, init, span, doc) ∈ fields }
EnumVariantTypeSet(variants) = { t | ∃ name, payload, disc, span, doc. VariantDecl(name, payload, disc, span, doc) ∈ variants ∧ t ∈ VariantPayloadTypeSet(payload) }

TypePos_Static(P, m) = { t | ∃ vis, mut, bind, span, doc. ⟨StaticDecl, vis, mut, bind, span, doc⟩ ∈ ASTModule(P, m).items ∧ bind.type_opt = t ∧ t ≠ ⊥ }
TypePos_Proc(P, m) = { t | ∃ vis, name, params, ret, body, span, doc. ⟨ProcedureDecl, vis, name, params, ret, body, span, doc⟩ ∈ ASTModule(P, m).items ∧ t ∈ (ParamTypeSet(params) ∪ TypeOptSet(ret)) }
TypePos_Record(P, m) = { t | ∃ vis, name, impls, members, span, doc. ⟨RecordDecl, vis, name, impls, members, span, doc⟩ ∈ ASTModule(P, m).items ∧ t ∈ (ClassPathTypeSet(impls) ∪ RecordMemberTypeSet(members)) }
TypePos_Enum(P, m) = { t | ∃ vis, name, impls, variants, span, doc. ⟨EnumDecl, vis, name, impls, variants, span, doc⟩ ∈ ASTModule(P, m).items ∧ t ∈ (ClassPathTypeSet(impls) ∪ EnumVariantTypeSet(variants)) }
TypePos_Modal(P, m) = { t | ∃ vis, name, impls, states, span, doc. ⟨ModalDecl, vis, name, impls, states, span, doc⟩ ∈ ASTModule(P, m).items ∧ t ∈ ClassPathTypeSet(impls) }
TypePos_Class(P, m) = { t | ∃ vis, name, supers, items, span, doc. ⟨ClassDecl, vis, name, supers, items, span, doc⟩ ∈ ASTModule(P, m).items ∧ t ∈ (ClassPathTypeSet(supers) ∪ ClassItemTypeSet(items)) }
TypePos_Alias(P, m) = { t | ∃ vis, name, ty, span, doc. ⟨TypeAliasDecl, vis, name, ty, span, doc⟩ ∈ ASTModule(P, m).items ∧ t = ty }
TypePositions(P, m) = TypePos_Static(P, m) ∪ TypePos_Proc(P, m) ∪ TypePos_Record(P, m) ∪ TypePos_Enum(P, m) ∪ TypePos_Modal(P, m) ∪ TypePos_Class(P, m) ∪ TypePos_Alias(P, m)

ArraySizeExprs(P, m) = { e | ∃ elem. TypeArray(elem, e) ∈ TypePositions(P, m) }
EnumDiscriminantExprs(P, m) = { e | ∃ vis, name, impls, variants, span, doc. ⟨EnumDecl, vis, name, impls, variants, span, doc⟩ ∈ ASTModule(P, m).items ∧ ∃ v. v = VariantDecl(_, _, e, _, _) ∈ variants ∧ e ≠ ⊥ }
TypePosExprs(P, m) = ArraySizeExprs(P, m) ∪ EnumDiscriminantExprs(P, m)

Elems(v) =
 {v}    if v ∈ ASTNode
 {x | x ∈ v ∧ x ∈ ASTNode}    if v ∈ [_]
 ∅    if v = ⊥
 ∅    otherwise
Child(x, y) ⇔ ∃ C, a_1, …, a_k. x = C(a_1, …, a_k) ∧ y ∈ ⋃_{i=1}^k Elems(a_i)
E_child = { (x, y) | Child(x, y) }
Subnode(x, y) ⇔ x = y ∨ Γ ⊢ Reachable(x, y, E_child)
ExprNodes(P, m) = { e | e ∈ Expr ∧ Subnode(ASTModule(P, m), e) }
PatNodes(P, m) = { p | p ∈ Pattern ∧ Subnode(ASTModule(P, m), p) }
ExprNodesOf(x) = { e | e ∈ Expr ∧ Subnode(x, e) }

TypeDeps(P, m) = { n | ∃ t ∈ TypePositions(P, m). Γ ⊢ TypeRefsTy(t, env_m) ⇓ T ∧ n ∈ T } ∪ { n | ∃ p ∈ PatNodes(P, m). Γ ⊢ TypeRefsPat(p, env_m) ⇓ T ∧ n ∈ T } ∪ { n | ∃ e ∈ (ExprNodes(P, m) ∪ TypePosExprs(P, m)). Γ ⊢ TypeRefsExpr(e, env_m) ⇓ T ∧ n ∈ T }

StaticInitExprs(P, m) = { init | ∃ vis, mut, bind, span, doc. ⟨StaticDecl, vis, mut, bind, span, doc⟩ ∈ ASTModule(P, m).items ∧ bind.init = init }
RecordFieldInitExprs(P, m) = { init | ∃ vis, name, impls, members, span, doc. ⟨RecordDecl, vis, name, impls, members, span, doc⟩ ∈ ASTModule(P, m).items ∧ ∃ f. f = FieldDecl(_, _, _, init, _, _) ∈ members ∧ init ≠ ⊥ }
ProcBodies(P, m) = { body | ∃ vis, name, params, ret, body, span, doc. ⟨ProcedureDecl, vis, name, params, ret, body, span, doc⟩ ∈ ASTModule(P, m).items }
RecordMethodBodies(P, m) = { body | ∃ vis, name, impls, members, span, doc. ⟨RecordDecl, vis, name, impls, members, span, doc⟩ ∈ ASTModule(P, m).items ∧ ∃ md. md = MethodDecl(_, _, _, _, _, _, body, _, _) ∈ members }
ClassMethodBodies(P, m) = { body | ∃ vis, name, supers, items, span, doc. ⟨ClassDecl, vis, name, supers, items, span, doc⟩ ∈ ASTModule(P, m).items ∧ ∃ md. md = ClassMethodDecl(_, _, _, _, _, body, _, _) ∈ items ∧ body ≠ ⊥ }

ValueDepsEager(P, m) = { n | ∃ e ∈ StaticInitExprs(P, m). Γ ⊢ ValueRefs(e, env_m) ⇓ V ∧ n ∈ V }
ValueDepsLazy(P, m) = { n | ∃ e ∈ RecordFieldInitExprs(P, m) ∪ ⋃_{b ∈ (ProcBodies(P, m) ∪ RecordMethodBodies(P, m) ∪ ClassMethodBodies(P, m))} ExprNodesOf(b). Γ ⊢ ValueRefs(e, env_m) ⇓ V ∧ n ∈ V }

**Dependency Graph.**

V = Modules

E_type = {(m, n) | n ∈ TypeDeps(P, m)}
E_val^{eager} = {(m, n) | n ∈ ValueDepsEager(P, m)}
E_val^{lazy} = {(m, n) | n ∈ ValueDepsLazy(P, m)}

G = ⟨V, E_type, E_val^{eager}, E_val^{lazy}⟩
G_e = ⟨V, E_val^{eager}⟩

**(WF-Acyclic-Eager)**
∀ v ∈ V, ¬ Reachable(v, v, E_val^{eager})
───────────────────────────────────────────────────────────
Γ ⊢ G_e : DAG

## 6. Phase 4: Code Generation

### 6.0. Codegen Model and Judgments

ArtifactsOf(P) = Set(Objs) ∪ Set(IRs) ∪ {Exe} ⇔ Γ ⊢ OutputPipeline(P) ⇓ (Objs, IRs, Exe)
IRTarget = "LLVM-21.1.8"
ObjTarget = "COFF"
LLVMValid_21.1.8(L) ⇔ L ∈ LLVMIR_21.1.8
∀ IR, L. Γ ⊢ LowerIR(IR) ⇓ L ⇒ LLVMValid_21.1.8(L)

CodegenJudg = {CodegenProject, CodegenModule, CodegenItem, CodegenExpr, CodegenStmt, CodegenBlock, CodegenPlace}

IRDefined(IR) ⇔ ∀ σ. ∃ out, σ'. ExecIRSigma(IR, σ) ⇓ (out, σ')

CodegenExprValCorrect ⇔ ∀ e, IR, v, σ, v', σ'. (Γ ⊢ CodegenExpr(e) ⇓ ⟨IR, v⟩ ∧ Γ ⊢ EvalSigma(e, σ) ⇓ (Val(v'), σ')) ⇒ (ExecIRSigma(IR, σ) ⇓ (Val(v'), σ') ∧ v = v')
CodegenExprCtrlCorrect ⇔ ∀ e, IR, v, σ, κ, σ'. (Γ ⊢ CodegenExpr(e) ⇓ ⟨IR, v⟩ ∧ Γ ⊢ EvalSigma(e, σ) ⇓ (Ctrl(κ), σ')) ⇒ (ExecIRSigma(IR, σ) ⇓ (Ctrl(κ), σ'))
CodegenStmtCorrect ⇔ ∀ s, IR, σ, sout, σ'. (Γ ⊢ CodegenStmt(s) ⇓ IR ∧ Γ ⊢ ExecSigma(s, σ) ⇓ (sout, σ')) ⇒ (ExecIRSigma(IR, σ) ⇓ (sout, σ'))
CodegenBlockValCorrect ⇔ ∀ b, IR, v, σ, v', σ'. (Γ ⊢ CodegenBlock(b) ⇓ ⟨IR, v⟩ ∧ Γ ⊢ EvalBlockSigma(b, σ) ⇓ (Val(v'), σ')) ⇒ (ExecIRSigma(IR, σ) ⇓ (Val(v'), σ') ∧ v = v')
CodegenBlockCtrlCorrect ⇔ ∀ b, IR, v, σ, κ, σ'. (Γ ⊢ CodegenBlock(b) ⇓ ⟨IR, v⟩ ∧ Γ ⊢ EvalBlockSigma(b, σ) ⇓ (Ctrl(κ), σ')) ⇒ (ExecIRSigma(IR, σ) ⇓ (Ctrl(κ), σ'))

CodegenCorrect ⇔ CodegenExprValCorrect ∧ CodegenExprCtrlCorrect ∧ CodegenStmtCorrect ∧ CodegenBlockValCorrect ∧ CodegenBlockCtrlCorrect
CodegenUndefined ⇔ ∃ e, IR, v. Γ ⊢ CodegenExpr(e) ⇓ ⟨IR, v⟩ ∧ ¬ IRDefined(IR) ∨ ∃ s, IR. Γ ⊢ CodegenStmt(s) ⇓ IR ∧ ¬ IRDefined(IR) ∨ ∃ b, IR, v. Γ ⊢ CodegenBlock(b) ⇓ ⟨IR, v⟩ ∧ ¬ IRDefined(IR)
CodegenUndefined ⇒ OutsideConformance

IRDecls = [IRDecl]
ModuleIR = IRDecls

**(CG-Project)**
Γ ⊢ OutputPipeline(P) ⇓ (Objs, IRs, Exe)
─────────────────────────────────────────────────────────────────────
Γ ⊢ CodegenProject(P) ⇓ (Set(Objs) ∪ Set(IRs) ∪ {Exe})

Items(P, m) = ASTModule(P, m).items

**(CG-Module)**
Items(Project(Γ), m) = [i_1, …, i_k]    ∀ j, Γ ⊢ CodegenItem(i_j) ⇓ ds_j    Γ ⊢ InitFn(m) ⇓ sym_init    Γ ⊢ DeinitFn(m) ⇓ sym_deinit    Γ ⊢ Lower-StaticInit(m) ⇓ IR_init    Γ ⊢ Lower-StaticDeinit(m) ⇓ IR_deinit
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ CodegenModule(m) ⇓ ds_1 ++ … ++ ds_k ++ [ProcIR(sym_init, [PanicOutParam], TypePrim("()"), IR_init), ProcIR(sym_deinit, [PanicOutParam], TypePrim("()"), IR_deinit)]

Γ ⊢ CodegenItem(item) ⇓ ds ⇒ ds ∈ IRDecls
ProcIR : Symbol × [Param] × Type × IR → IRDecl

PanicOutParam = ⟨`move`, PanicOutName, PanicOutType⟩
CodegenParams(params) = params ++ [PanicOutParam]

MethodParams(R, m) = [⟨RecvMode(m.receiver), `self`, RecvType(Self_R, m.receiver)⟩] ++ m.params
ClassMethodParams(Cl, m) = [⟨RecvMode(m.receiver), `self`, RecvType(SelfVar, m.receiver)⟩] ++ m.params

ParamList_T(T, params) = [⟨mode_i, name_i, SubstSelf(T, ty_i)⟩ | ⟨mode_i, name_i, ty_i⟩ ∈ params]
ClassMethodParams_T(T, m) = [⟨RecvMode(m.receiver), `self`, RecvType(T, m.receiver)⟩] ++ ParamList_T(T, m.params)

StateMethodParams(M, S, md) = [⟨⊥, `self`, TypePerm(`const`, TypeModalState(ModalPath(M), S))⟩] ++ md.params
TransitionParams(M, S, tr) = [⟨`move`, `self`, TypePerm(`unique`, TypeModalState(ModalPath(M), S))⟩] ++ tr.params

StateList(M) = [s | s ∈ M.states]
DefaultImpl : Type × ClassMethodDecl → ASTItem
DefaultUserSet(m) = { T | Γ ⊢ T uses default m }
DefaultUserList(m) = sort_{≺_{type}}(DefaultUserSet(m))

**(CG-Item-Using)**
item = UsingDecl(_)
────────────────────────────────────────────
Γ ⊢ CodegenItem(item) ⇓ []

**(CG-Item-TypeAlias)**
item = TypeAliasDecl(_)
────────────────────────────────────────────
Γ ⊢ CodegenItem(item) ⇓ []

**(CG-Item-Procedure-Main)**
item = ProcedureDecl(vis, "main", params, ret_opt, body, span, doc)    Project(Γ) = P    Executable(P)    MainSigOk(item)    R = ProcReturn(ret_opt)    Γ ⊢ EmitInitPlan(P) ⇓ IR_init    Γ ⊢ LowerBlock(body) ⇓ ⟨IR_body, v⟩    Γ ⊢ Mangle(item) ⇓ sym    params' = CodegenParams(params)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ CodegenItem(item) ⇓ [ProcIR(sym, params', R, SeqIR(IR_init, IR_body))]

**(CG-Item-Procedure)**
item = ProcedureDecl(vis, name, params, ret_opt, body, span, doc)    Project(Γ) = P    (name ≠ "main" ∨ ¬ Executable(P))    R = ProcReturn(ret_opt)    Γ ⊢ LowerBlock(body) ⇓ ⟨IR, v⟩    Γ ⊢ Mangle(item) ⇓ sym    params' = CodegenParams(params)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ CodegenItem(item) ⇓ [ProcIR(sym, params', R, IR)]

**(CG-Item-Static)**
item = StaticDecl(_)    Γ ⊢ EmitGlobal(item) ⇓ ds
──────────────────────────────────────────────────────────
Γ ⊢ CodegenItem(item) ⇓ ds

**(CG-Item-Record)**
item = RecordDecl(vis, name, implements, members, span, doc)    R = item    Methods(R) = [m_1, …, m_k]    ∀ i, Γ ⊢ CodegenItem(m_i) ⇓ ds_i
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ CodegenItem(item) ⇓ ds_1 ++ … ++ ds_k

**(CG-Item-Method)**
m ∈ Methods(R)    params' = MethodParams(R, m)    R_m = ReturnType(m)    m.body = body    Γ ⊢ LowerBlock(body) ⇓ ⟨IR, v⟩    Γ ⊢ Mangle(m) ⇓ sym    params'' = CodegenParams(params')
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ CodegenItem(m) ⇓ [ProcIR(sym, params'', R_m, IR)]

**(CG-Item-Modal)**
item = ModalDecl(vis, name, implements, states, span, doc)    M = item    ∀ S ∈ StateList(M), ∀ md ∈ Methods(S), Γ ⊢ CodegenItem(md) ⇓ ds_{S,md}    ∀ S ∈ StateList(M), ∀ tr ∈ Transitions(S), Γ ⊢ CodegenItem(tr) ⇓ ds_{S,tr}
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ CodegenItem(item) ⇓ ++_{S ∈ StateList(M)} ( ++_{md ∈ Methods(S)} ds_{S,md} ++ ++_{tr ∈ Transitions(S)} ds_{S,tr} )

**(CG-Item-StateMethod)**
S ∈ StateList(M)    md ∈ Methods(S)    params' = StateMethodParams(M, S, md)    R_m = ReturnType(md)    md.body = body    Γ ⊢ LowerBlock(body) ⇓ ⟨IR, v⟩    Γ ⊢ Mangle(md) ⇓ sym    params'' = CodegenParams(params')
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ CodegenItem(md) ⇓ [ProcIR(sym, params'', R_m, IR)]

**(CG-Item-Transition)**
S ∈ StateList(M)    tr ∈ Transitions(S)    params' = TransitionParams(M, S, tr)    TransitionSig(M, S, tr).target = S_t    tr.body = body    Γ ⊢ LowerBlock(body) ⇓ ⟨IR, v⟩    Γ ⊢ Mangle(tr) ⇓ sym    params'' = CodegenParams(params')
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ CodegenItem(tr) ⇓ [ProcIR(sym, params'', TypeModalState(ModalPath(M), S_t), IR)]

**(CG-Item-Class)**
item = ClassDecl(vis, name, supers, items, span, doc)    Cl = item    ClassMethods(Cl) = [m_1, …, m_k]    ∀ i, Γ ⊢ CodegenItem(m_i) ⇓ ds_i
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ CodegenItem(item) ⇓ ds_1 ++ … ++ ds_k

**(CG-Item-ClassMethod-Abstract)**
m ∈ ClassMethods(Cl)    m.body_opt = ⊥
────────────────────────────────────────────
Γ ⊢ CodegenItem(m) ⇓ []

**(CG-Item-ClassMethod-Body)**
m ∈ ClassMethods(Cl)    m.body_opt = body    DefaultUserList(m) = [T_1, …, T_k]    ∀ i, Γ_i = Γ[SelfVar ↦ T_i]    params_i = ClassMethodParams_T(T_i, m)    R_i = ReturnType_T(T_i, m)    Γ_i ⊢ LowerBlock(body) ⇓ ⟨IR_i, v_i⟩    Γ ⊢ Mangle(DefaultImpl(T_i, m)) ⇓ sym_i    params_i' = CodegenParams(params_i)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ CodegenItem(m) ⇓ [ProcIR(sym_1, params_1', R_1, IR_1), …, ProcIR(sym_k, params_k', R_k, IR_k)]

**(CG-Item-Enum)**
item = EnumDecl(_)
────────────────────────────────────────────
Γ ⊢ CodegenItem(item) ⇓ []

**(CG-Item-ErrorItem)**
item = ErrorItem(_)
────────────────────────────────────────────
Γ ⊢ CodegenItem(item) ⇓ []
∃ item, ds. item = ErrorItem(_) ∧ Γ ⊢ CodegenItem(item) ⇓ ds ⇒ OutsideConformance

**(CG-Expr)**
Γ ⊢ LowerExpr(e) ⇓ ⟨IR, v⟩
────────────────────────────────────────────
Γ ⊢ CodegenExpr(e) ⇓ ⟨IR, v⟩

**(CG-Stmt)**
Γ ⊢ LowerStmt(s) ⇓ IR
──────────────────────────────
Γ ⊢ CodegenStmt(s) ⇓ IR

**(CG-Block)**
Γ ⊢ LowerBlock(b) ⇓ ⟨IR, v⟩
─────────────────────────────────────────────
Γ ⊢ CodegenBlock(b) ⇓ ⟨IR, v⟩

**(CG-Place)**
Γ ⊢ LowerPlace(p) ⇓ l
────────────────────────────────
Γ ⊢ CodegenPlace(p) ⇓ l


### 6.1. Layout and Representation

#### 6.1.1. Primitive Layout and Encoding

PtrSize = 8
PointerSize = PtrSize
PtrAlign = 8

PrimSize("i8") = 1
PrimSize("i16") = 2
PrimSize("i32") = 4
PrimSize("i64") = 8
PrimSize("i128") = 16
PrimSize("u8") = 1
PrimSize("u16") = 2
PrimSize("u32") = 4
PrimSize("u64") = 8
PrimSize("u128") = 16
PrimSize("f16") = 2
PrimSize("f32") = 4
PrimSize("f64") = 8
PrimSize("bool") = 1
PrimSize("char") = 4
PrimSize("usize") = PtrSize
PrimSize("isize") = PtrSize
PrimSize("()") = 0
PrimSize("!") = 0

PrimAlign("i8") = 1
PrimAlign("i16") = 2
PrimAlign("i32") = 4
PrimAlign("i64") = 8
PrimAlign("i128") = 16
PrimAlign("u8") = 1
PrimAlign("u16") = 2
PrimAlign("u32") = 4
PrimAlign("u64") = 8
PrimAlign("u128") = 16
PrimAlign("f16") = 2
PrimAlign("f32") = 4
PrimAlign("f64") = 8
PrimAlign("bool") = 1
PrimAlign("char") = 4
PrimAlign("usize") = PtrAlign
PrimAlign("isize") = PtrAlign
PrimAlign("()") = 1
PrimAlign("!") = 1

LayoutJudg = {sizeof, alignof, layout}

**(Size-Prim)**
T = TypePrim(name)    PrimSize(name) = n
────────────────────────────────────────
Γ ⊢ sizeof(T) = n

**(Align-Prim)**
T = TypePrim(name)    PrimAlign(name) = a
──────────────────────────────────────────
Γ ⊢ alignof(T) = a

**(Layout-Prim)**
T = TypePrim(name)    PrimSize(name) = n    PrimAlign(name) = a
────────────────────────────────────────────────────────────
Γ ⊢ layout(T) ⇓ ⟨n, a⟩

**Encoding.**

LEBytes(v, n) = LE(v mod 2^{8n}, n)
FloatBits_t(v) = IEEE754Bits(t, v)
EncodeConstJudg = {EncodeConst}
BoolByte(false) = 0x00
BoolByte(true) = 0x01

**(Encode-Bool)**
LiteralValue(lit, TypePrim("bool")) = b
──────────────────────────────────────────────────────────────
Γ ⊢ EncodeConst(TypePrim("bool"), lit) ⇓ LEBytes(BoolByte(b), 1)

**(Encode-Char)**
LiteralValue(lit, TypePrim("char")) = c
──────────────────────────────────────────────────────────────
Γ ⊢ EncodeConst(TypePrim("char"), lit) ⇓ LEBytes(c, 4)

**(Encode-Int)**
lit.kind = IntLiteral    T = TypePrim(t)    t ∈ IntTypes    v = LiteralValue(lit, T)    x = IntValValue(v)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EncodeConst(T, lit) ⇓ LEBytes(x, sizeof(T))

**(Encode-Float)**
lit.kind = FloatLiteral    T = TypePrim(t)    t ∈ FloatTypes    v = LiteralValue(lit, T)    x = FloatValValue(v)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EncodeConst(T, lit) ⇓ LEBytes(FloatBits_t(x), sizeof(T))

**(Encode-Unit)**
T = TypePrim("()")
────────────────────────────────────────────
Γ ⊢ EncodeConst(T, lit) ⇓ []

**(Encode-Never)**
T = TypePrim("!")
────────────────────────────────────────────
Γ ⊢ EncodeConst(T, lit) ⇓ []

**(Encode-RawPtr-Null)**
lit.kind = NullLiteral    T = TypeRawPtr(q, U)
───────────────────────────────────────────────────────────────
Γ ⊢ EncodeConst(T, lit) ⇓ LEBytes(0, sizeof(T))

**Validity.**

ValidValueJudg = {ValidValue}

**(Valid-Bool)**
ValidValue(TypePrim("bool"), bits) ⇔ bits ∈ {[0x00], [0x01]}

**(Valid-Char)**
ValidValue(TypePrim("char"), bits) ⇔ ∃ c. LEBytes(c, 4) = bits ∧ c ∈ UnicodeScalar

**(Valid-Scalar)**
ScalarTypes = {"i8", "i16", "i32", "i64", "i128", "u8", "u16", "u32", "u64", "u128", "f16", "f32", "f64", "usize", "isize"}
∀ t ∈ ScalarTypes. ValidValue(TypePrim(t), bits) ⇔ |bits| = PrimSize(t)

**(Valid-Unit)**
ValidValue(TypePrim("()"), bits) ⇔ bits = []

**(Valid-Never)**
ValidValue(TypePrim("!"), bits) ⇔ false

#### 6.1.2. Permission, Pointer, and Function Layout

**(Layout-Perm)**
Γ ⊢ layout(T) ⇓ L
────────────────────────────────────────
Γ ⊢ layout(TypePerm(p, T)) ⇓ L

**(Size-Perm)**
Γ ⊢ sizeof(T) = n
────────────────────────────────────────
Γ ⊢ sizeof(TypePerm(p, T)) = n

**(Align-Perm)**
Γ ⊢ alignof(T) = a
────────────────────────────────────────
Γ ⊢ alignof(TypePerm(p, T)) = a

**(Size-Ptr)**
T = TypePtr(T_0, s)
────────────────────────────────
Γ ⊢ sizeof(T) = PtrSize

**(Align-Ptr)**
T = TypePtr(T_0, s)
────────────────────────────────
Γ ⊢ alignof(T) = PtrAlign

**(Layout-Ptr)**
T = TypePtr(T_0, s)
────────────────────────────────────────────
Γ ⊢ layout(T) ⇓ ⟨PtrSize, PtrAlign⟩

**(Size-RawPtr)**
T = TypeRawPtr(q, T_0)
────────────────────────────────
Γ ⊢ sizeof(T) = PtrSize

**(Align-RawPtr)**
T = TypeRawPtr(q, T_0)
────────────────────────────────
Γ ⊢ alignof(T) = PtrAlign

**(Layout-RawPtr)**
T = TypeRawPtr(q, T_0)
────────────────────────────────────────────
Γ ⊢ layout(T) ⇓ ⟨PtrSize, PtrAlign⟩

**(Size-Func)**
T = TypeFunc(params, R)
────────────────────────────────
Γ ⊢ sizeof(T) = PtrSize

**(Align-Func)**
T = TypeFunc(params, R)
────────────────────────────────
Γ ⊢ alignof(T) = PtrAlign

**(Layout-Func)**
T = TypeFunc(params, R)
────────────────────────────────────────────
Γ ⊢ layout(T) ⇓ ⟨PtrSize, PtrAlign⟩

#### 6.1.3. Record Layout Without `[[layout(C)]]`

AlignUp(x, a) = ⌈x/a⌉ × a    where a > 0
Offsets([]) = []
Offsets(fields) = [offset_1, …, offset_n] ⇔ fields = [⟨f_1, T_1⟩, …, ⟨f_n, T_n⟩] ∧ n ≥ 1 ∧ offset_1 = 0 ∧ ∀ i ∈ {2, …, n}. offset_i = AlignUp(offset_{i-1} + sizeof(T_{i-1}), alignof(T_i))
RecordAlign([]) = 1
RecordAlign(fields) = max_{i ∈ {1, …, n}}(alignof(T_i)) ⇔ fields = [⟨f_1, T_1⟩, …, ⟨f_n, T_n⟩] ∧ n ≥ 1
RecordSize([]) = 0
RecordSize(fields) = AlignUp(offset_n + sizeof(T_n), RecordAlign(fields)) ⇔ fields = [⟨f_1, T_1⟩, …, ⟨f_n, T_n⟩] ∧ n ≥ 1 ∧ Offsets(fields) = [offset_1, …, offset_n]
RecordLayoutJudg = {RecordLayout}

**(Layout-Record-Empty)**
────────────────────────────────────────────────────────
Γ ⊢ RecordLayout([]) ⇓ ⟨0, 1, []⟩

**(Layout-Record-Cons)**
n ≥ 1    offsets = [offset_1, …, offset_n]    align = RecordAlign(fields)    size = RecordSize(fields)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ RecordLayout(fields) ⇓ ⟨size, align, offsets⟩

**(Size-Record)**
T = TypePath(p)    RecordDecl(p) = R    Fields(R) = fields    RecordLayout(fields) ⇓ ⟨size, _, _⟩
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ sizeof(T) = size

**(Align-Record)**
T = TypePath(p)    RecordDecl(p) = R    Fields(R) = fields    RecordLayout(fields) ⇓ ⟨_, align, _⟩
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ alignof(T) = align

**(Layout-Record)**
T = TypePath(p)    RecordDecl(p) = R    Fields(R) = fields    RecordLayout(fields) ⇓ ⟨size, align, _⟩
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ layout(T) ⇓ ⟨size, align⟩

FieldOffset(fields, f_i) = offset_i ⇔ fields = [⟨f_1, T_1⟩, …, ⟨f_n, T_n⟩] ∧ 1 ≤ i ≤ n ∧ Offsets(fields) = [offset_1, …, offset_n]

**Type Aliases.**
AliasBody(p) = ty ⇔ Σ.Types[p] = TypeAliasDecl(vis, name, ty, span, doc)

**(Size-Alias)**
T = TypePath(p)    AliasBody(p) = ty    Γ ⊢ sizeof(ty) = size
───────────────────────────────────────────────────────────────
Γ ⊢ sizeof(T) = size

**(Align-Alias)**
T = TypePath(p)    AliasBody(p) = ty    Γ ⊢ alignof(ty) = align
────────────────────────────────────────────────────────────────
Γ ⊢ alignof(T) = align

**(Layout-Alias)**
T = TypePath(p)    AliasBody(p) = ty    Γ ⊢ layout(ty) ⇓ ⟨size, align⟩
────────────────────────────────────────────────────────────────────────────
Γ ⊢ layout(T) ⇓ ⟨size, align⟩

#### 6.1.4. Union Layout and Discriminants

##### 6.1.4.1. Niche Optimization (Cursive0)

**Niche Sets.**

NicheSet(T) ⊆ { bits | ¬ ValidValue(T, bits) }
NicheSet(T) = {LEBytes(0, PtrSize)} ⇔ ∃ U. T = TypePtr(U, `Valid`)
NicheSet(T) = ∅ ⇔ ¬ ∃ U. T = TypePtr(U, `Valid`)

BitsToUInt(bits) = v ⇔ LEBytes(v, |bits|) = bits
bits_1 ≺_u bits_2 ⇔ ∃ v_1, v_2. BitsToUInt(bits_1) = v_1 ∧ BitsToUInt(bits_2) = v_2 ∧ v_1 < v_2
NicheOrder(T) = sort_{≺_u}(NicheSet(T))
NicheCount(T) = |NicheSet(T)|

**Valid Pointer Non-Zero Invariant.**

ValidValue(TypePtr(T, `Valid`), bits) ⇔ |bits| = PtrSize ∧ bits ≠ LEBytes(0, PtrSize)
ValidValue(TypePtr(T, `Null`), bits) ⇔ bits = LEBytes(0, PtrSize)
ValidValue(TypePtr(T, `Expired`), bits) ⇔ |bits| = PtrSize
ValidValue(TypePtr(T, ⊥), bits) ⇔ |bits| = PtrSize
ValidValue(TypeRawPtr(q, T), bits) ⇔ |bits| = PtrSize
ValidValue(T, bits) ⇔ T ∉ {TypePrim(_), TypePtr(_, _), TypeRawPtr(_, _)} ∧ ∃ v. ValueBits(T, v) = bits

**Union Niche Encoding.**

U = TypeUnion([T_1, …, T_n])

**Type Ordering (Cursive0).**

PathOrderKey(p) = ⟨Fold(p), p⟩
ArrayLen(e) = n ⇔ Γ ⊢ ConstLen(e) ⇓ n

TagKey(`prim`) = 0
TagKey(`tuple`) = 1
TagKey(`array`) = 2
TagKey(`slice`) = 3
TagKey(`func`) = 4
TagKey(`path`) = 5
TagKey(`modal_state`) = 6
TagKey(`string`) = 7
TagKey(`bytes`) = 8
TagKey(`dynamic`) = 9
TagKey(`ptr`) = 10
TagKey(`rawptr`) = 11
TagKey(`union`) = 12
TagKey(`perm`) = 13
TagKey(`range`) = 14

PermKey(`const`) = 0
PermKey(`unique`) = 1
PtrStateKey(⊥) = 0
PtrStateKey(`Valid`) = 1
PtrStateKey(`Null`) = 2
PtrStateKey(`Expired`) = 3
QualKey(`imm`) = 0
QualKey(`mut`) = 1
ModeKey(⊥) = 0
ModeKey(`move`) = 1
StateKey(`View`) = 0
StateKey(`Managed`) = 1
StateKey(⊥) = 2

TypeKey(TypePrim(name)) = ⟨TagKey(`prim`), name⟩
TypeKey(TypeRange) = ⟨TagKey(`range`)⟩
TypeKey(TypeTuple([T_1, …, T_n])) = ⟨TagKey(`tuple`), n, TypeKey(T_1), …, TypeKey(T_n)⟩
TypeKey(TypeArray(T, e)) = ⟨TagKey(`array`), TypeKey(T), ArrayLen(e)⟩
TypeKey(TypeSlice(T)) = ⟨TagKey(`slice`), TypeKey(T)⟩
TypeKey(TypeFunc([⟨m_1, T_1⟩, …, ⟨m_n, T_n⟩], R)) = ⟨TagKey(`func`), n, ModeKey(m_1), TypeKey(T_1), …, ModeKey(m_n), TypeKey(T_n), TypeKey(R)⟩
TypeKey(TypePath(p)) = ⟨TagKey(`path`), PathOrderKey(p)⟩
TypeKey(TypeModalState(p, S)) = ⟨TagKey(`modal_state`), PathOrderKey(p), S⟩
TypeKey(TypeString(st)) = ⟨TagKey(`string`), StateKey(st)⟩
TypeKey(TypeBytes(st)) = ⟨TagKey(`bytes`), StateKey(st)⟩
TypeKey(TypeDynamic(p)) = ⟨TagKey(`dynamic`), PathOrderKey(p)⟩
TypeKey(TypePtr(T, s)) = ⟨TagKey(`ptr`), PtrStateKey(s), TypeKey(T)⟩
TypeKey(TypeRawPtr(q, T)) = ⟨TagKey(`rawptr`), QualKey(q), TypeKey(T)⟩
TypeKey(TypeUnion([T_1, …, T_n])) = ⟨TagKey(`union`), Sort([TypeKey(T_1), …, TypeKey(T_n)])⟩
TypeKey(TypePerm(p, T)) = ⟨TagKey(`perm`), PermKey(p), TypeKey(T)⟩

Key = { TypeKey(T) | T ∈ Type }
KeyList = { [k_1, …, k_n] | ∀ i. k_i ∈ Key }
a ≺_{atom} b ⇔ (a, b ∈ ℕ ∧ a < b) ∨ (a, b ∈ String ∧ Utf8LexLess(a, b)) ∨ (a, b ∈ Key ∧ a ≺_{key} b) ∨ (a, b ∈ KeyList ∧ a ≺_{keylist} b)
LexLess_{≺}(L_1, L_2) ⇔ (∃ k. 0 ≤ k < |L_1| ∧ 0 ≤ k < |L_2| ∧ (∀ i. 0 ≤ i < k ⇒ L_1[i] = L_2[i]) ∧ L_1[k] ≺ L_2[k]) ∨ (|L_1| < |L_2| ∧ ∀ i. 0 ≤ i < |L_1| ⇒ L_1[i] = L_2[i])
k_1 ≺_{key} k_2 ⇔ LexLess_{≺_{atom}}(k_1, k_2)
L_1 ≺_{keylist} L_2 ⇔ LexLess_{≺_{key}}(L_1, L_2)
Sorted_{≺}(L) ⇔ ∀ i, j. 0 ≤ i < j < |L| ⇒ ¬(L[j] ≺ L[i])
Sort(L) = L' ⇔ Permutation(L', L) ∧ Sorted_{≺_{key}}(L')
T_1 ≺_{type} T_2 ⇔ TypeKey(T_1) ≺_{key} TypeKey(T_2)

MemberList(U) = Sort(Members(U))
MemberIndex(U, T) = i ⇔ MemberList(U)[i] ≡ T
UnionDiscValue(U, T) = i ⇔ MemberIndex(U, T) = i
EmptyMember(T) ⇔ T ≡ TypePrim("()")
EmptyList(U) = [MemberList(U)[i] | 0 ≤ i < |MemberList(U)| ∧ EmptyMember(MemberList(U)[i])]
PayloadMember(U) = T_p ⇔ ∃ j. MemberList(U)[j] ≡ T_p ∧ NicheCount(T_p) > 0 ∧ (∀ i. 0 ≤ i < |MemberList(U)| ∧ i ≠ j ⇒ EmptyMember(MemberList(U)[i])) ∧ NicheCount(T_p) ≥ |MemberList(U)| - 1
NicheApplies(U) ⇔ ∃ T_p. PayloadMember(U) = T_p

**ValueBits.**

FieldValueList(fs, f) = v ⇔ ⟨f, v⟩ ∈ fs
StructBits([T_1, …, T_n], [v_1, …, v_n], [o_1, …, o_n], size) = bits ⇔ |bits| = size ∧ ∀ i. ValueBits(T_i, v_i) = b_i ∧ bits[o_i..o_i+|b_i|) = b_i ∧ ∀ j. (∀ i. j ∉ [o_i, o_i+|b_i|)) ⇒ bits[j] = 0x00
PadBytes(b, size) = bits ⇔ |bits| = size ∧ bits[0..|b|) = b ∧ ∀ i. |b| ≤ i < size ⇒ bits[i] = 0x00

ValueBits(TypePrim("bool"), v) = bits ⇔ (v = BoolVal(true) ∧ bits = [0x01]) ∨ (v = BoolVal(false) ∧ bits = [0x00])
ValueBits(TypePrim("char"), v) = bits ⇔ v = CharVal(u) ∧ LEBytes(u, 4) = bits
ValueBits(TypePrim("()"), v) = bits ⇔ v = UnitVal ∧ bits = []
ValueBits(TypePrim(t), v) = bits ⇔ t ∈ IntTypes ∧ v = IntVal(t, x) ∧ LEBytes(x, sizeof(TypePrim(t))) = bits
ValueBits(TypePrim(t), v) = bits ⇔ t ∈ FloatTypes ∧ v = FloatVal(t, x) ∧ LEBytes(IEEE754Bits(t, x), sizeof(TypePrim(t))) = bits
ValueBits(TypePerm(p, T), v) = bits ⇔ ValueBits(T, v) = bits

ValueBits(TypePtr(T, `Valid`), v) = bits ⇔ v = PtrVal(`Valid`, addr) ∧ addr ≠ 0x0 ∧ bits = LEBytes(addr, PtrSize)
ValueBits(TypePtr(T, `Null`), v) = bits ⇔ v = PtrVal(`Null`, addr) ∧ addr = 0x0 ∧ bits = LEBytes(addr, PtrSize)
ValueBits(TypePtr(T, `Expired`), v) = bits ⇔ v = PtrVal(`Expired`, addr) ∧ bits = LEBytes(addr, PtrSize)
ValueBits(TypePtr(T, ⊥), v) = bits ⇔ ∃ s. s ∈ PtrStateSet ∧ ValueBits(TypePtr(T, s), v) = bits
ValueBits(TypeRawPtr(q, T), v) = bits ⇔ v = RawPtr(q, addr) ∧ bits = LEBytes(addr, PtrSize)

ValueBits(TypeTuple([T_1, …, T_n]), (v_1, …, v_n)) = bits ⇔ TupleLayout([T_1, …, T_n]) ⇓ ⟨size, _, offsets⟩ ∧ StructBits([T_1, …, T_n], [v_1, …, v_n], offsets, size) = bits
ValueBits(TypeArray(T, e), [v_0, …, v_{n-1}]) = bits ⇔ ArrayLen(e) = n ∧ s = sizeof(T) ∧ |bits| = n × s ∧ ∀ i. 0 ≤ i < n ⇒ (ValueBits(T, v_i) = b_i ∧ bits[i × s .. i × s + |b_i|) = b_i)
ValueBits(TypeSlice(T), SliceValue(v, r)) = bits ⇔ SliceBounds(r, Len(v)) = (start, end) ∧ n = end - start ∧ ∃ addr. ValueBits(TypeRawPtr(`imm`, T), RawPtr(`imm`, addr)) = b_ptr ∧ ValueBits(TypePrim("usize"), IntVal("usize", n)) = b_len ∧ bits = b_ptr ++ b_len
ValueBits(TypeRange, r) = bits ⇔ RangeValFields(r) = [⟨`kind`, v_k⟩, ⟨`lo`, v_l⟩, ⟨`hi`, v_h⟩] ∧ RangeFields = [⟨`kind`, T_k⟩, ⟨`lo`, T_l⟩, ⟨`hi`, T_h⟩] ∧ RecordLayout(RangeFields) ⇓ ⟨size, _, offsets⟩ ∧ StructBits([T_k, T_l, T_h], [v_k, v_l, v_h], offsets, size) = bits

ValueBits(TypeModalState(p, S), v) = bits ⇔ Σ.Types[p] = `modal` M ∧ S ∈ States(M) ∧ v = RecordValue(ModalStateRef(p, S), fs) ∧ Payload(M, S) = fields ∧ RecordLayout(fields) ⇓ ⟨size, _, offsets⟩ ∧ fields = [⟨f_1, T_1⟩, …, ⟨f_n, T_n⟩] ∧ (∀ i. FieldValue(RecordValue(ModalStateRef(p, S), fs), f_i) = v_i) ∧ StructBits([T_1, …, T_n], [v_1, …, v_n], offsets, size) = bits

EnumPayloadBits(E, name, ⊥) = bits ⇔ (∃ v ∈ Variants(E). v.name = name ∧ VariantPayloadOpt(v) = ⊥) ∧ PadBytes([], PayloadSize(E)) = bits
EnumPayloadBits(E, name, TuplePayload([v_1, …, v_k])) = bits ⇔ (∃ v ∈ Variants(E). v.name = name ∧ VariantPayloadOpt(v) = TuplePayload([T_1, …, T_k])) ∧ ValueBits(TypeTuple([T_1, …, T_k]), (v_1, …, v_k)) = b ∧ PadBytes(b, PayloadSize(E)) = bits
EnumPayloadBits(E, name, RecordPayload(fs)) = bits ⇔ (∃ v ∈ Variants(E). v.name = name ∧ VariantPayloadOpt(v) = RecordPayload(fields)) ∧ RecordLayout(fields) ⇓ ⟨size, _, offsets⟩ ∧ fields = [⟨f_1, T_1⟩, …, ⟨f_n, T_n⟩] ∧ (∀ i. FieldValueList(fs, f_i) = v_i) ∧ StructBits([T_1, …, T_n], [v_1, …, v_n], offsets, size) = b ∧ PadBytes(b, PayloadSize(E)) = bits

ValueBits(TypePath(p), v) = bits ⇔ AliasBody(p) = ty ∧ ValueBits(ty, v) = bits
ValueBits(TypePath(p), v) = bits ⇔ RecordDecl(p) = R ∧ v = RecordValue(TypePath(p), fs) ∧ Fields(R) = fields ∧ RecordLayout(fields) ⇓ ⟨size, _, offsets⟩ ∧ fields = [⟨f_1, T_1⟩, …, ⟨f_n, T_n⟩] ∧ (∀ i. FieldValue(RecordValue(TypePath(p), fs), f_i) = v_i) ∧ StructBits([T_1, …, T_n], [v_1, …, v_n], offsets, size) = bits
ValueBits(TypePath(p), v) = bits ⇔ EnumDecl(p) = E ∧ v = EnumValue(path, payload) ∧ EnumPath(path) = p ∧ name = VariantName(path) ∧ EnumDisc(E, name) = d ∧ EnumPayloadBits(E, name, payload) = payload_bits ∧ EnumDiscType(E) = D ∧ D = TypePrim(t) ∧ ValueBits(D, IntVal(t, d)) = disc_bits ∧ TaggedBits(disc_bits, payload_bits, sizeof(D), PayloadSize(E), PayloadAlign(E), EnumSize(E)) = bits
ValueBits(TypePath(p), v) = bits ⇔ Σ.Types[p] = `modal` M ∧ v = ⟨S, v_s⟩ ∧ ModalBits(M, S, v_s) = bits

ValueBits(TypeUnion(U), v) = bits ⇔ ∃ T. Member(T, TypeUnion(U)) ∧ UnionBits(U, T, v) = bits

ValueBits(TypeDynamic(Cl), v) = bits ⇔ v = Dyn(Cl, RawPtr(`imm`, addr), T) ∧ sym = ScopedSym(VTableDecl(T, Cl)) ∧ addr_vt = AddrOfSym(sym) ∧ RecordLayout(DynFields(Cl)) ⇓ ⟨size, _, offsets⟩ ∧ StructBits([TypeRawPtr(`imm`, TypePrim("()")), TypeRawPtr(`imm`, TypePath(["VTable"]))], [RawPtr(`imm`, addr), RawPtr(`imm`, addr_vt)], offsets, size) = bits
ValueBits(TypeString(st), v) = bits ⇔ ValueType(v) = TypeString(st) ∧ |bits| = sizeof(TypeString(st))
ValueBits(TypeBytes(st), v) = bits ⇔ ValueType(v) = TypeBytes(st) ∧ |bits| = sizeof(TypeBytes(st))

ValueBits(T, v) = bits ⇒ ValidValue(T, bits)
UnionNicheBits(U, T, v) = bits ⇔ NicheApplies(U) ∧ PayloadMember(U) = T_p ∧ ((T ≡ T_p ∧ ValueBits(T_p, v) = bits ∧ bits ∉ NicheSet(T_p)) ∨ (∃ i. EmptyList(U)[i] ≡ T ∧ v = () ∧ NicheOrder(T_p)[i] = bits))

**Union Layout.**

k = |MemberList(U)| - 1
UnionDiscType(U) = DiscType(k)
PayloadSize(U) = max_{T ∈ MemberList(U)}(sizeof(T))
PayloadAlign(U) = max_{T ∈ MemberList(U)}(alignof(T))
UnionAlign(U) = max(alignof(UnionDiscType(U)), PayloadAlign(U))
UnionSize(U) = AlignUp(sizeof(UnionDiscType(U)) + PayloadSize(U), UnionAlign(U))
UnionLayoutJudg = {UnionLayout}

**(Layout-Union-Niche)**
NicheApplies(U)    PayloadMember(U) = T_p    Γ ⊢ layout(T_p) ⇓ ⟨size, align⟩
──────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ UnionLayout(U) ⇓ ⟨size, align, ⊥, layout(T_p)⟩

**(Layout-Union-Tagged)**
¬ NicheApplies(U)    size = UnionSize(U)    align = UnionAlign(U)
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ UnionLayout(U) ⇓ ⟨size, align, UnionDiscType(U), PayloadSize(U)⟩

**(Size-Union)**
T = TypeUnion([T_1, …, T_n])    UnionLayout(T) ⇓ ⟨size, _, _, _⟩
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ sizeof(T) = size

**(Align-Union)**
T = TypeUnion([T_1, …, T_n])    UnionLayout(T) ⇓ ⟨_, align, _, _⟩
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ alignof(T) = align

**(Layout-Union)**
T = TypeUnion([T_1, …, T_n])    UnionLayout(T) ⇓ ⟨size, align, _, _⟩
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ layout(T) ⇓ ⟨size, align⟩

PayloadBits(U, T, v) = bits ⇔ ValueBits(T, v) = b ∧ |bits| = PayloadSize(U) ∧ bits[0..|b|) = b
TaggedBits(disc_bits, payload_bits, disc_size, payload_size, payload_align, size) = bits ⇔ |bits| = size ∧ payload_off = AlignUp(disc_size, payload_align) ∧ bits[0..disc_size) = disc_bits ∧ bits[payload_off..payload_off + payload_size) = payload_bits

**Informative.** TaggedBits constrains only the discriminant and payload ranges; bytes outside those ranges are unconstrained.

UnionTaggedBits(U, T, v) = bits ⇔ ¬ NicheApplies(U) ∧ UnionDiscType(U) = D ∧ UnionDiscValue(U, T) = d ∧ ValueBits(D, d) = disc_bits ∧ PayloadBits(U, T, v) = payload_bits ∧ TaggedBits(disc_bits, payload_bits, sizeof(D), PayloadSize(U), PayloadAlign(U), UnionSize(U)) = bits
UnionBits(U, T, v) = bits ⇔ UnionNicheBits(U, T, v) = bits ∨ UnionTaggedBits(U, T, v) = bits

**Modal Niche Encoding.**

SingleFieldPayload(M, S) = T ⇔ Payload(M, S) = [⟨f, T⟩]
EmptyState(M, S) ⇔ Payload(M, S) = []
PayloadState(M) = S_p ⇔ S_p ∈ States(M) ∧ SingleFieldPayload(M, S_p) = T_p ∧ NicheCount(T_p) > 0 ∧ (∀ S ∈ States(M). S ≠ S_p ⇒ EmptyState(M, S)) ∧ NicheCount(T_p) ≥ |States(M)| - 1
NicheApplies(M) ⇔ ∃ S_p. PayloadState(M) = S_p
EmptyStates(M) = [ S ∈ States(M) | EmptyState(M, S) ]
EmptyRecordVal(v) ⇔ ∃ tr. v = RecordValue(tr, [])
ModalNicheBits(M, S, v) = bits ⇔ NicheApplies(M) ∧ PayloadState(M) = S_p ∧ SingleFieldPayload(M, S_p) = T_p ∧ ((S = S_p ∧ ValueBits(T_p, v) = bits ∧ bits ∉ NicheSet(T_p)) ∨ (∃ i. EmptyStates(M)[i] = S ∧ (v = () ∨ EmptyRecordVal(v)) ∧ NicheOrder(T_p)[i] = bits))
ModalBits(M, S, v) = bits ⇔ ModalNicheBits(M, S, v) = bits ∨ ModalTaggedBits(M, S, v) = bits

ModalPayloadSize(M) = max_{S ∈ States(M)}(StateSize(M, S))
ModalPayloadAlign(M) = max_{S ∈ States(M)}(StateAlign(M, S))
StateRecordBits(M, S, v) = b ⇔ Payload(M, S) = fields ∧ RecordLayout(fields) ⇓ ⟨size, _, offsets⟩ ∧ fields = [⟨f_1, T_1⟩, …, ⟨f_n, T_n⟩] ∧ ((n = 0 ∧ (v = () ∨ EmptyRecordVal(v)) ∧ b = []) ∨ (n > 0 ∧ v = RecordValue(tr, fs) ∧ (∀ i. FieldValue(RecordValue(tr, fs), f_i) = v_i) ∧ StructBits([T_1, …, T_n], [v_1, …, v_n], offsets, size) = b))
ModalPayloadBits(M, S, v) = bits ⇔ StateRecordBits(M, S, v) = b ∧ PadBytes(b, ModalPayloadSize(M)) = bits

Modal tagged layout is fully defined; all bytes outside the discriminant and payload ranges MUST be zero.
ModalTaggedBits(M, S, v) = bits ⇔ ¬ NicheApplies(M) ∧ ModalDiscType(M) = D ∧ StateIndex(M, S) = i ∧ ValueBits(D, i) = disc_bits ∧ ModalPayloadBits(M, S, v) = payload_bits ∧ TaggedBits(disc_bits, payload_bits, sizeof(D), ModalPayloadSize(M), ModalPayloadAlign(M), ModalSize(M)) = bits ∧ payload_off = AlignUp(sizeof(D), ModalPayloadAlign(M)) ∧ ∀ j. 0 ≤ j < |bits| ∧ j ∉ [0, sizeof(D)) ∧ j ∉ [payload_off, payload_off + ModalPayloadSize(M)) ⇒ bits[j] = 0x00



#### 6.1.5. String and Bytes Layout

**`string@Managed` Representation**

StringManagedFields = [⟨`pointer`, TypePtr(TypePrim("u8"), `Valid`)⟩, ⟨`length`, TypePrim("usize")⟩, ⟨`capacity`, TypePrim("usize")⟩]
StringManagedOffsets = [0, PtrSize, 2 × PtrSize]
RecordLayout(StringManagedFields) = ⟨3 × PtrSize, PtrAlign, StringManagedOffsets⟩
sizeof(`string@Managed`) = 3 × PtrSize

**`string@View` Representation**

StringViewFields = [⟨`pointer`, TypePtr(TypePerm(`const`, TypePrim("u8")), `Valid`)⟩, ⟨`length`, TypePrim("usize")⟩]
StringViewOffsets = [0, PtrSize]
RecordLayout(StringViewFields) = ⟨2 × PtrSize, PtrAlign, StringViewOffsets⟩
sizeof(`string@View`) = 2 × PtrSize

**String Layout Rules.**

**(Size-String-Managed)**
T = TypeString(`@Managed`)
──────────────────────────────────────────────────
Γ ⊢ sizeof(T) = 3 × PtrSize

**(Align-String-Managed)**
T = TypeString(`@Managed`)
───────────────────────────────────────
Γ ⊢ alignof(T) = PtrAlign

**(Layout-String-Managed)**
T = TypeString(`@Managed`)
────────────────────────────────────────────
Γ ⊢ layout(T) ⇓ ⟨3 × PtrSize, PtrAlign⟩

**(Size-String-View)**
T = TypeString(`@View`)
────────────────────────────────────────
Γ ⊢ sizeof(T) = 2 × PtrSize

**(Align-String-View)**
T = TypeString(`@View`)
────────────────────────────────────
Γ ⊢ alignof(T) = PtrAlign

**(Layout-String-View)**
T = TypeString(`@View`)
───────────────────────────────────────────
Γ ⊢ layout(T) ⇓ ⟨2 × PtrSize, PtrAlign⟩

**`bytes@Managed` Representation**

BytesManagedFields = [⟨`pointer`, TypePtr(TypePrim("u8"), `Valid`)⟩, ⟨`length`, TypePrim("usize")⟩, ⟨`capacity`, TypePrim("usize")⟩]
BytesManagedOffsets = [0, PtrSize, 2 × PtrSize]
RecordLayout(BytesManagedFields) = ⟨3 × PtrSize, PtrAlign, BytesManagedOffsets⟩
sizeof(`bytes@Managed`) = 3 × PtrSize

**`bytes@View` Representation**

BytesViewFields = [⟨`pointer`, TypePtr(TypePerm(`const`, TypePrim("u8")), `Valid`)⟩, ⟨`length`, TypePrim("usize")⟩]
BytesViewOffsets = [0, PtrSize]
RecordLayout(BytesViewFields) = ⟨2 × PtrSize, PtrAlign, BytesViewOffsets⟩
sizeof(`bytes@View`) = 2 × PtrSize

**Bytes Layout Rules.**

**(Size-Bytes-Managed)**
T = TypeBytes(`@Managed`)
─────────────────────────────────────────
Γ ⊢ sizeof(T) = 3 × PtrSize

**(Align-Bytes-Managed)**
T = TypeBytes(`@Managed`)
────────────────────────────────────
Γ ⊢ alignof(T) = PtrAlign

**(Layout-Bytes-Managed)**
T = TypeBytes(`@Managed`)
───────────────────────────────────────────
Γ ⊢ layout(T) ⇓ ⟨3 × PtrSize, PtrAlign⟩

**(Size-Bytes-View)**
T = TypeBytes(`@View`)
────────────────────────────────────────
Γ ⊢ sizeof(T) = 2 × PtrSize

**(Align-Bytes-View)**
T = TypeBytes(`@View`)
────────────────────────────────────
Γ ⊢ alignof(T) = PtrAlign

**(Layout-Bytes-View)**
T = TypeBytes(`@View`)
───────────────────────────────────────────
Γ ⊢ layout(T) ⇓ ⟨2 × PtrSize, PtrAlign⟩

**(Size-String-Modal)**
T = TypeString(⊥)    Γ ⊢ ModalLayout(`string`) ⇓ ⟨size, align, _, _⟩
───────────────────────────────────────────────────────────────
Γ ⊢ sizeof(T) = size

**(Align-String-Modal)**
T = TypeString(⊥)    Γ ⊢ ModalLayout(`string`) ⇓ ⟨size, align, _, _⟩
────────────────────────────────────────────────────────────────
Γ ⊢ alignof(T) = align

**(Size-Bytes-Modal)**
T = TypeBytes(⊥)    Γ ⊢ ModalLayout(`bytes`) ⇓ ⟨size, align, _, _⟩
───────────────────────────────────────────────────────────────
Γ ⊢ sizeof(T) = size

**(Align-Bytes-Modal)**
T = TypeBytes(⊥)    Γ ⊢ ModalLayout(`bytes`) ⇓ ⟨size, align, _, _⟩
────────────────────────────────────────────────────────────────
Γ ⊢ alignof(T) = align


#### 6.1.6. Aggregate Layouts (Tuples, Arrays, Slices, Ranges, Enums)

**Tuples.**

TupleFields([T_1, …, T_n]) = [⟨0, T_1⟩, …, ⟨n-1, T_n⟩]
TupleLayoutJudg = {TupleLayout}

**(Layout-Tuple-Empty)**
────────────────────────────────────────────────────────
Γ ⊢ TupleLayout([]) ⇓ ⟨0, 1, []⟩

**(Layout-Tuple-Cons)**
n ≥ 1    TupleFields([T_1, …, T_n]) = fields    RecordLayout(fields) ⇓ ⟨size, align, offsets⟩
────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TupleLayout([T_1, …, T_n]) ⇓ ⟨size, align, offsets⟩

**(Size-Tuple)**
TupleLayout([T_1, …, T_n]) ⇓ ⟨size, _, _⟩
──────────────────────────────────────────────────────────────
Γ ⊢ sizeof(TypeTuple([T_1, …, T_n])) = size

**(Align-Tuple)**
TupleLayout([T_1, …, T_n]) ⇓ ⟨_, align, _⟩
───────────────────────────────────────────────────────────────
Γ ⊢ alignof(TypeTuple([T_1, …, T_n])) = align

**(Layout-Tuple)**
TupleLayout([T_1, …, T_n]) ⇓ ⟨size, align, _⟩
───────────────────────────────────────────────────────────────
Γ ⊢ layout(TypeTuple([T_1, …, T_n])) ⇓ ⟨size, align⟩

**Arrays.**

**(Size-Array)**
Γ ⊢ ConstLen(e) ⇓ n    Γ ⊢ sizeof(T_0) = s
────────────────────────────────────────────────────────────────
Γ ⊢ sizeof(TypeArray(T_0, e)) = n × s

**(Align-Array)**
Γ ⊢ alignof(T_0) = a
────────────────────────────────────────────────────────
Γ ⊢ alignof(TypeArray(T_0, e)) = a

**(Layout-Array)**
Γ ⊢ sizeof(TypeArray(T_0, e)) = size    Γ ⊢ alignof(TypeArray(T_0, e)) = align
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ layout(TypeArray(T_0, e)) ⇓ ⟨size, align⟩

**Slices.**

**(Size-Slice)**
────────────────────────────────────────────
Γ ⊢ sizeof(T) = 2 × PtrSize

**(Align-Slice)**
──────────────────────────────────────────
Γ ⊢ alignof(T) = PtrAlign

**(Layout-Slice)**
────────────────────────────────────────────────────────
Γ ⊢ layout(T) ⇓ ⟨2 × PtrSize, PtrAlign⟩

**Ranges.**

RangeTag(`To`) = 0
RangeTag(`ToInclusive`) = 1
RangeTag(`Full`) = 2
RangeTag(`From`) = 3
RangeTag(`Exclusive`) = 4
RangeTag(`Inclusive`) = 5
RangeFields = [⟨`kind`, TypePrim("u8")⟩, ⟨`lo`, TypePrim("usize")⟩, ⟨`hi`, TypePrim("usize")⟩]
OptVal(v_opt) =
 IntVal("usize", 0)    if v_opt = ⊥
 v_opt    otherwise
RangeValFields(RangeVal(kind, lo_opt, hi_opt)) = [⟨`kind`, IntVal("u8", RangeTag(kind))⟩, ⟨`lo`, OptVal(lo_opt)⟩, ⟨`hi`, OptVal(hi_opt)⟩]
RangeLayoutJudg = {RangeLayout}

**(Layout-Range)**
RecordLayout(RangeFields) ⇓ ⟨size, align, offsets⟩
────────────────────────────────────────────────────────────
Γ ⊢ RangeLayout() ⇓ ⟨size, align, offsets⟩

**(Size-Range)**
Γ ⊢ RangeLayout() ⇓ ⟨size, _, _⟩
───────────────────────────────────────────────
Γ ⊢ sizeof(TypeRange) = size

**(Align-Range)**
Γ ⊢ RangeLayout() ⇓ ⟨_, align, _⟩
────────────────────────────────────────────────
Γ ⊢ alignof(TypeRange) = align

**(Layout-Range-SizeAlign)**
Γ ⊢ RangeLayout() ⇓ ⟨size, align, _⟩
────────────────────────────────────────────────
Γ ⊢ layout(TypeRange) ⇓ ⟨size, align⟩

**Enums.**

EnumDiscType(E) = DiscType(E)
VariantPayloadOpt(v) = payload_opt ⇔ v = ⟨name, payload_opt, disc_opt, span, doc_opt⟩
VariantSize(v) = 0 ⇔ VariantPayloadOpt(v) = ⊥
VariantAlign(v) = 1 ⇔ VariantPayloadOpt(v) = ⊥
VariantSize(v) = s ⇔ VariantPayloadOpt(v) = TuplePayload([T_1, …, T_k]) ∧ TupleLayout([T_1, …, T_k]) ⇓ ⟨s, a, _⟩
VariantAlign(v) = a ⇔ VariantPayloadOpt(v) = TuplePayload([T_1, …, T_k]) ∧ TupleLayout([T_1, …, T_k]) ⇓ ⟨s, a, _⟩
VariantSize(v) = s ⇔ VariantPayloadOpt(v) = RecordPayload(fields) ∧ RecordLayout(fields) ⇓ ⟨s, a, _⟩
VariantAlign(v) = a ⇔ VariantPayloadOpt(v) = RecordPayload(fields) ∧ RecordLayout(fields) ⇓ ⟨s, a, _⟩
PayloadSize(E) = max_{v ∈ Variants(E)}(VariantSize(v))
PayloadAlign(E) = max_{v ∈ Variants(E)}(VariantAlign(v))
EnumAlign(E) = max(alignof(EnumDiscType(E)), PayloadAlign(E))
EnumSize(E) = AlignUp(sizeof(EnumDiscType(E)) + PayloadSize(E), EnumAlign(E))
EnumLayoutJudg = {EnumLayout}

**(Layout-Enum-Tagged)**
size = EnumSize(E)    align = EnumAlign(E)
────────────────────────────────────────────────────────────────
Γ ⊢ EnumLayout(E) ⇓ ⟨size, align, EnumDiscType(E), PayloadSize(E)⟩

**(Size-Enum)**
T = TypePath(p)    EnumDecl(p) = E    EnumLayout(E) ⇓ ⟨size, _, _, _⟩
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ sizeof(T) = size

**(Align-Enum)**
T = TypePath(p)    EnumDecl(p) = E    EnumLayout(E) ⇓ ⟨_, align, _, _⟩
─────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ alignof(T) = align

**(Layout-Enum)**
T = TypePath(p)    EnumDecl(p) = E    EnumLayout(E) ⇓ ⟨size, align, _, _⟩
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ layout(T) ⇓ ⟨size, align⟩

#### 6.1.7. Modal Layout (Codegen)

ModalDiscType(M) = DiscType(|States(M)| - 1)
StateSize(M, S) = s ⇔ RecordLayout(Payload(M, S)) ⇓ ⟨s, a, _⟩
StateAlign(M, S) = a ⇔ RecordLayout(Payload(M, S)) ⇓ ⟨s, a, _⟩
ModalAlign(M) = max(alignof(ModalDiscType(M)), max_{S ∈ States(M)}(StateAlign(M, S)))
ModalSize(M) = AlignUp(sizeof(ModalDiscType(M)) + max_{S ∈ States(M)}(StateSize(M, S)), ModalAlign(M))
ModalLayoutJudg = {ModalLayout}

**(Layout-Modal-Niche)**
NicheApplies(M)    PayloadState(M) = S_p    SingleFieldPayload(M, S_p) = T_p    Γ ⊢ layout(T_p) ⇓ ⟨size, align⟩
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ModalLayout(M) ⇓ ⟨size, align, ⊥, layout(T_p)⟩

**(Layout-Modal-Tagged)**
¬ NicheApplies(M)    size = ModalSize(M)    align = ModalAlign(M)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ModalLayout(M) ⇓ ⟨size, align, ModalDiscType(M), max_{S ∈ States(M)}(StateSize(M, S))⟩

**(Size-Modal)**
T = TypePath(p)    Σ.Types[p] = `modal` M    ModalLayout(M) ⇓ ⟨size, _, _, _⟩
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ sizeof(T) = size

**(Align-Modal)**
T = TypePath(p)    Σ.Types[p] = `modal` M    ModalLayout(M) ⇓ ⟨_, align, _, _⟩
───────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ alignof(T) = align

**(Layout-Modal)**
T = TypePath(p)    Σ.Types[p] = `modal` M    ModalLayout(M) ⇓ ⟨size, align, _, _⟩
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ layout(T) ⇓ ⟨size, align⟩

**(Size-ModalState)**
T = TypeModalState(p, S)    Σ.Types[p] = `modal` M    S ∈ States(M)    StateSize(M, S) = size
───────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ sizeof(T) = size

**(Align-ModalState)**
T = TypeModalState(p, S)    Σ.Types[p] = `modal` M    S ∈ States(M)    StateAlign(M, S) = align
───────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ alignof(T) = align

**(Layout-ModalState)**
T = TypeModalState(p, S)    Σ.Types[p] = `modal` M    S ∈ States(M)    StateSize(M, S) = size    StateAlign(M, S) = align
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ layout(T) ⇓ ⟨size, align⟩

#### 6.1.8. Dynamic Class Object Layout

DynFields(Cl) = [⟨`data`, TypeRawPtr(`imm`, TypePrim("()"))⟩, ⟨`vtable`, TypeRawPtr(`imm`, TypePath(["VTable"]))⟩]
DynLayoutJudg = {DynLayout}

**(Layout-DynamicClass)**
───────────────────────────────────────────────────────────────────────────────
Γ ⊢ DynLayout(Cl) ⇓ ⟨2 × PtrSize, PtrAlign, DynFields(Cl)⟩

**(Size-DynamicClass)**
T = TypeDynamic(Cl)
────────────────────────────────────────────
Γ ⊢ sizeof(T) = 2 × PtrSize

**(Align-DynamicClass)**
T = TypeDynamic(Cl)
────────────────────────────────────
Γ ⊢ alignof(T) = PtrAlign

### 6.2. ABI Lowering (Cursive0)

#### 6.2.1. Default Calling Convention

**DefaultCallingConvention.**
CallConvDefault = Cursive0ABI


#### 6.2.2. ABI Type Lowering

ABIType = { ⟨size, align⟩ | size ∈ ℕ ∧ align ∈ ℕ }
ABITyJudg = {ABITy}

**(ABI-Prim)**
Γ ⊢ sizeof(TypePrim(name)) = s    Γ ⊢ alignof(TypePrim(name)) = a
────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ABITy(TypePrim(name)) ⇓ ⟨s, a⟩

**(ABI-Perm)**
Γ ⊢ ABITy(T) ⇓ τ
──────────────────────────────────────────
Γ ⊢ ABITy(TypePerm(p, T)) ⇓ τ

**(ABI-Ptr)**
T = TypePtr(U, s)
──────────────────────────────────────────────
Γ ⊢ ABITy(T) ⇓ ⟨PtrSize, PtrAlign⟩

**(ABI-RawPtr)**
T = TypeRawPtr(q, U)
──────────────────────────────────────────────
Γ ⊢ ABITy(T) ⇓ ⟨PtrSize, PtrAlign⟩

**(ABI-Func)**
T = TypeFunc(params, R)
──────────────────────────────────────────────
Γ ⊢ ABITy(T) ⇓ ⟨PtrSize, PtrAlign⟩

**(ABI-Alias)**
T = TypePath(p)    AliasBody(p) = ty    Γ ⊢ ABITy(ty) ⇓ τ
────────────────────────────────────────────────────────────
Γ ⊢ ABITy(T) ⇓ τ

**(ABI-Record)**
T = TypePath(p)    RecordDecl(p) = R    Fields(R) = fields    RecordLayout(fields) ⇓ ⟨size, align, _⟩
──────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ABITy(T) ⇓ ⟨size, align⟩

**(ABI-Tuple)**
TupleLayout([T_1, …, T_n]) ⇓ ⟨size, align, _⟩
───────────────────────────────────────────────────────────────────────────────
Γ ⊢ ABITy(TypeTuple([T_1, …, T_n])) ⇓ ⟨size, align⟩

**(ABI-Array)**
Γ ⊢ sizeof(TypeArray(T, e)) = size    Γ ⊢ alignof(TypeArray(T, e)) = align
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ABITy(TypeArray(T, e)) ⇓ ⟨size, align⟩

**(ABI-Slice)**
T = TypeSlice(U)
──────────────────────────────────────────────
Γ ⊢ ABITy(T) ⇓ ⟨2 × PtrSize, PtrAlign⟩

**(ABI-Range)**
Γ ⊢ sizeof(TypeRange) = size    Γ ⊢ alignof(TypeRange) = align
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ABITy(TypeRange) ⇓ ⟨size, align⟩

**(ABI-Enum)**
T = TypePath(p)    EnumDecl(p) = E    EnumLayout(E) ⇓ ⟨size, align, _, _⟩
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ABITy(T) ⇓ ⟨size, align⟩

**(ABI-Union)**
T = TypeUnion([T_1, …, T_n])    UnionLayout(T) ⇓ ⟨size, align, _, _⟩
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ABITy(T) ⇓ ⟨size, align⟩

**(ABI-Modal)**
T = TypePath(p)    Σ.Types[p] = `modal` M    ModalLayout(M) ⇓ ⟨size, align, _, _⟩
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ABITy(T) ⇓ ⟨size, align⟩

**(ABI-Dynamic)**
Γ ⊢ DynLayout(Cl) ⇓ ⟨size, align, _⟩
──────────────────────────────────────────────────────────────
Γ ⊢ ABITy(TypeDynamic(Cl)) ⇓ ⟨size, align⟩

**(ABI-StringBytes)**
T ∈ {TypeString(`@View`), TypeString(`@Managed`), TypeBytes(`@View`), TypeBytes(`@Managed`)}    Γ ⊢ sizeof(T) = size    Γ ⊢ alignof(T) = align
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ABITy(T) ⇓ ⟨size, align⟩

#### 6.2.3. ABI Parameter and Return Passing

PassKind = {`ByValue`, `ByRef`, `SRet`}
ByValMax = 2 × PtrSize
ByValAlign = PtrAlign
ByValOk(T) ⇔ Γ ⊢ sizeof(T) = n ∧ Γ ⊢ alignof(T) = a ∧ n ≤ ByValMax ∧ a ≤ ByValAlign
ABIParamJudg = {ABIParam}
ABIRetJudg = {ABIRet}
ABICallJudg = {ABICall}

**(ABI-Param-ByRef-Alias)**
mode = ⊥    Γ ⊢ sizeof(T) = n
──────────────────────────────────────────
Γ ⊢ ABIParam(mode, T) ⇓ `ByRef`

**(ABI-Param-ByValue-Move)**
mode = `move`    Γ ⊢ sizeof(T) = 0 ∨ ByValOk(T)
────────────────────────────────────────────────────────
Γ ⊢ ABIParam(mode, T) ⇓ `ByValue`

**(ABI-Param-ByRef-Move)**
mode = `move`    Γ ⊢ sizeof(T) = n    n > 0    ¬ ByValOk(T)
─────────────────────────────────────────────────────────────
Γ ⊢ ABIParam(mode, T) ⇓ `ByRef`

**(ABI-Ret-ByValue)**
Γ ⊢ sizeof(T) = 0 ∨ ByValOk(T)
────────────────────────────────────────────
Γ ⊢ ABIRet(T) ⇓ `ByValue`

**(ABI-Ret-ByRef)**
Γ ⊢ sizeof(T) = n    n > 0    ¬ ByValOk(T)
──────────────────────────────────────────
Γ ⊢ ABIRet(T) ⇓ `SRet`

**(ABI-Call)**
∀ i, Γ ⊢ ABIParam(m_i, T_i) ⇓ k_i    Γ ⊢ ABIRet(R) ⇓ k_r
────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ABICall([⟨m_1, T_1⟩, …, ⟨m_n, T_n⟩], R) ⇓ ⟨[k_1, …, k_n], k_r, (k_r = `SRet`)⟩

**Panic Out-Parameter (Cursive0).**

PanicRecordFields = [⟨`panic`, TypePrim("bool")⟩, ⟨`code`, TypePrim("u32")⟩]
PanicRecordLayout = RecordLayout(PanicRecordFields)
PanicRecordFieldsOf(PanicRecord) = PanicRecordFields
PanicRecordLayoutOf(PanicRecord) = PanicRecordLayout

PanicOutType = TypeRawPtr(`mut`, PanicRecord)
PanicOutName = "__panic"

NeedsPanicOut(callee) ⇔ callee ≠ RecordCtor(_) ∧ callee ≠ EntrySym ∧ RuntimeSig(callee) undefined.

PanicOutParams(params, callee) =
 params ++ [⟨`move`, PanicOutName, PanicOutType⟩]    if NeedsPanicOut(callee)
 params    otherwise

#### 6.2.4. Call Lowering for Procedures and Methods

LowerCallJudg = {MethodSymbol, BuiltinMethodSym, LowerMethodCall, LowerArgs, LowerRecvArg}
ModalStateOf(T) = TypeModalState(p, S) ⇔ StripPerm(T) = TypeModalState(p, S)
BuiltinCapClass = {`FileSystem`, `HeapAllocator`}

**(MethodSymbol-Record)**
LookupMethod(T, name) = m    m = MethodDecl(_)    Γ ⊢ Mangle(m) ⇓ sym
───────────────────────────────────────────────────────────────────────────
Γ ⊢ MethodSymbol(T, name) ⇓ sym

**(MethodSymbol-Default)**
LookupMethod(T, name) = m    m = ClassMethodDecl(_)    m.body_opt ≠ ⊥    Γ ⊢ Mangle(DefaultImpl(T, m)) ⇓ sym
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ MethodSymbol(T, name) ⇓ sym

**(MethodSymbol-ModalState-Method)**
ModalStateOf(T) = TypeModalState(p, S)    LookupStateMethod(S, name) = md    Γ ⊢ Mangle(md) ⇓ sym
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ MethodSymbol(T, name) ⇓ sym

**(MethodSymbol-ModalState-Transition)**
ModalStateOf(T) = TypeModalState(p, S)    LookupTransition(S, name) = tr    Γ ⊢ Mangle(tr) ⇓ sym
────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ MethodSymbol(T, name) ⇓ sym

**(BuiltinMethodSym-FileSystem)**
BuiltinSym(`FileSystem`::name) ⇓ sym
──────────────────────────────────────────────────────────
Γ ⊢ BuiltinMethodSym(`FileSystem`, name) ⇓ sym

**(BuiltinMethodSym-HeapAllocator)**
BuiltinSym(`HeapAllocator`::name) ⇓ sym
─────────────────────────────────────────────────────────────
Γ ⊢ BuiltinMethodSym(`HeapAllocator`, name) ⇓ sym

**(Lower-Args-Empty)**
──────────────────────────────────────────────────────
Γ ⊢ LowerArgs([], []) ⇓ ⟨ε, []⟩

**(Lower-Args-Cons-Move)**
Γ ⊢ LowerExpr(MovedArg(moved, e)) ⇓ ⟨IR_e, v⟩    Γ ⊢ LowerArgs(ps, as) ⇓ ⟨IR_a, vec_v⟩
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerArgs([⟨`move`, x, T_p⟩] ++ ps, [⟨moved, e, _⟩] ++ as) ⇓ ⟨SeqIR(IR_e, IR_a), [v] ++ vec_v⟩

**(Lower-Args-Cons-Ref)**
Γ ⊢ LowerAddrOf(e) ⇓ ⟨IR_e, addr⟩    Γ ⊢ LowerArgs(ps, as) ⇓ ⟨IR_a, vec_v⟩
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerArgs([⟨⊥, x, T_p⟩] ++ ps, [⟨moved, e, _⟩] ++ as) ⇓ ⟨SeqIR(IR_e, IR_a), [Ptr@Valid(addr)] ++ vec_v⟩

**(Lower-RecvArg-Move)**
base = MoveExpr(p)    Γ ⊢ LowerExpr(base) ⇓ ⟨IR_b, v_self⟩
────────────────────────────────────────────────────────────────────
Γ ⊢ LowerRecvArg(base) ⇓ ⟨IR_b, v_self⟩

**(Lower-RecvArg-Ref)**
base ≠ MoveExpr(_)    Γ ⊢ LowerAddrOf(base) ⇓ ⟨IR_b, addr⟩
────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerRecvArg(base) ⇓ ⟨IR_b, Ptr@Valid(addr)⟩

**(Lower-MethodCall-Static-PanicOut)**
Γ ⊢ LowerRecvArg(base) ⇓ ⟨IR_b, v_self⟩    Γ ⊢ LowerArgs(m.params, args) ⇓ ⟨IR_a, vec_v⟩    T = ExprType(base)    T ≠ TypeDynamic(_)    LookupMethod(T, name) = m    MethodSymbol(T, name) ⇓ sym    NeedsPanicOut(sym)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerMethodCall(MethodCall(base, name, args)) ⇓ ⟨SeqIR(IR_b, IR_a, CallIR(sym, [v_self] ++ vec_v ++ [PanicOutName]), PanicCheck), v_call⟩

**(Lower-MethodCall-Static-NoPanicOut)**
Γ ⊢ LowerRecvArg(base) ⇓ ⟨IR_b, v_self⟩    Γ ⊢ LowerArgs(m.params, args) ⇓ ⟨IR_a, vec_v⟩    T = ExprType(base)    T ≠ TypeDynamic(_)    LookupMethod(T, name) = m    MethodSymbol(T, name) ⇓ sym    ¬ NeedsPanicOut(sym)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerMethodCall(MethodCall(base, name, args)) ⇓ ⟨SeqIR(IR_b, IR_a, CallIR(sym, [v_self] ++ vec_v)), v_call⟩

**(Lower-MethodCall-Capability)**
Γ ⊢ LowerRecvArg(base) ⇓ ⟨IR_b, v_self⟩    Γ ⊢ LowerArgs(m.params, args) ⇓ ⟨IR_a, vec_v⟩    ExprType(base) = TypeDynamic(Cl)    Cl ∈ BuiltinCapClass    LookupClassMethod(Cl, name) = m    Γ ⊢ BuiltinMethodSym(Cl, name) ⇓ sym
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerMethodCall(MethodCall(base, name, args)) ⇓ ⟨SeqIR(IR_b, IR_a, CallIR(sym, [v_self] ++ vec_v)), v_call⟩

**(Lower-MethodCall-Dynamic)**
Γ ⊢ LowerExpr(base) ⇓ ⟨IR_b, v_self⟩    Γ ⊢ LowerArgs(m.params, args) ⇓ ⟨IR_a, vec_v⟩    ExprType(base) = TypeDynamic(Cl)    Cl ∉ BuiltinCapClass    LookupClassMethod(Cl, name) = m    Γ ⊢ LowerDynCall(v_self, name, vec_v ++ [PanicOutName]) ⇓ IR_d
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerMethodCall(MethodCall(base, name, args)) ⇓ ⟨SeqIR(IR_b, IR_a, IR_d), v_call⟩

SeqIR() = ε
SeqIR(IR) = IR
SeqIR(IR_1, …, IR_n) = SeqIR(IR_1, SeqIR(IR_2, …, IR_n))    (n ≥ 2)

### 6.3. Symbols, Mangling, and Linkage

#### 6.3.1. Symbol Names and Mangling

MangleJudg = {Mangle}
VTableDecl(T, Cl) constructor
LiteralData(kind, contents) constructor
DefaultImpl(T, m) constructor

Join(sep, []) = "\""
Join(sep, [s]) = s
Join(sep, [s_1, …, s_n]) = s_1 ++ sep ++ Join(sep, [s_2, …, s_n])    (n ≥ 2)
PathSig(p) = mangle(PathString(p))

ItemPath(it) = PathOfModule(ModuleOf(it)) ++ [name] ⇔ it = ProcedureDecl(vis, name, params, ret_opt, body, span, doc)
ItemPath(m) = RecordPath(R) ++ [m.name] ⇔ m ∈ Methods(R)
ItemPath(m) = ClassPath(Cl) ++ [m.name] ⇔ m ∈ ClassMethods(Cl)
ItemPath(m) = ModalPath(M) ++ [S] ++ [m.name] ⇔ S ∈ States(M) ∧ m ∈ Methods(S)
ItemPath(tr) = ModalPath(M) ++ [S] ++ [tr.name] ⇔ S ∈ States(M) ∧ tr ∈ Transitions(S)
ItemPath(it) = PathOfModule(ModuleOf(it)) ++ [StaticName(binding)] ⇔ it = StaticDecl(vis, mut, binding, span, doc) ∧ StaticName(binding) ≠ ⊥
ItemPath(StaticBinding(StaticDecl(vis, mut, binding, span, doc), x)) = PathOfModule(ModuleOf(StaticDecl(vis, mut, binding, span, doc))) ++ [x]
ItemPath(VTableDecl(T, Cl)) = ["vtable"] ++ PathOfType(T) ++ ["cl"] ++ ClassPath(Cl)
ItemPath(DefaultImpl(T, m)) = ["default"] ++ PathOfType(T) ++ ["cl"] ++ ClassPath(Cl) ++ [m.name] ⇔ m ∈ ClassMethods(Cl)

TypeStateName(`View`) = "view"
TypeStateName(`Managed`) = "managed"
PathOfType(TypePrim(name)) = ["prim", name]
PathOfType(TypeString(st)) = ["string", TypeStateName(st)]
PathOfType(TypeBytes(st)) = ["bytes", TypeStateName(st)]
PathOfType(TypePath(p)) = p
PathOfType(TypeModalState(p, S)) = p ++ [S]
PathOfType(T) = ⊥ ⇔ T ∉ {TypePrim(_), TypeString(_), TypeBytes(_), TypePath(_), TypeModalState(_, _)}
ClassPath(Cl) = p ⇔ Σ.Classes[p] = Cl

**Literal Identity.**

FNVOffset64 = 14695981039346656037
FNVPrime64 = 1099511628211
FNV1a64([]) = FNVOffset64
FNV1a64([b_1, …, b_n]) = h_n ⇔ h_0 = FNVOffset64 ∧ ∀ i ∈ 0..n-1. h_{i+1} = ((h_i ⊕ b_{i+1}) × FNVPrime64) mod 2^64
Hex64(h) = Join("\"", [Hex2(b_1), …, Hex2(b_8)]) ⇔ rev(LEBytes(h, 8)) = [b_1, …, b_8]
LiteralID(kind, contents) = mangle(kind) ++ "_" ++ Hex64(FNV1a64(contents))

**Mangle Rules.**

ScopedSym(item) = PathSig(ItemPath(item))

**(Mangle-Proc)**
item = ProcedureDecl(vis, name, params, ret_opt, body, span, doc)    name ≠ "main"
─────────────────────────────────────────────────────────────────────────────
Γ ⊢ Mangle(item) ⇓ ScopedSym(item)

**(Mangle-Main)**
item = ProcedureDecl(vis, "main", params, ret_opt, body, span, doc)    MainSigOk(item)
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ Mangle(item) ⇓ ScopedSym(item)

**(Mangle-Record-Method)**
item = MethodDecl(vis, override, name, receiver, params, ret_opt, body, span, doc_opt)
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ Mangle(item) ⇓ ScopedSym(item)

**(Mangle-Class-Method)**
item = ClassMethodDecl(vis, name, receiver, params, ret_opt, body_opt, span, doc_opt)
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ Mangle(item) ⇓ ScopedSym(item)

**(Mangle-State-Method)**
item = StateMethodDecl(vis, name, params, ret_opt, body, span, doc_opt)
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ Mangle(item) ⇓ ScopedSym(item)

**(Mangle-Transition)**
item = TransitionDecl(vis, name, params, target, body, span, doc_opt)
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ Mangle(item) ⇓ ScopedSym(item)

**(Mangle-Static)**
item = StaticDecl(vis, mut, binding, span, doc)    StaticName(binding) ≠ ⊥
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ Mangle(item) ⇓ ScopedSym(item)

**(Mangle-StaticBinding)**
item = StaticBinding(StaticDecl(_, _, binding, _, _), x)
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ Mangle(item) ⇓ ScopedSym(item)

**(Mangle-VTable)**
item = VTableDecl(T, Cl)
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ Mangle(item) ⇓ ScopedSym(item)

**(Mangle-Literal)**
item = LiteralData(kind, contents)    LiteralID(kind, contents) = id
──────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Mangle(item) ⇓ PathSig(["cursive", "runtime", "literal", id])

**(Mangle-DefaultImpl)**
item = DefaultImpl(T, m)
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ Mangle(item) ⇓ ScopedSym(item)

#### 6.3.4. Linkage for Generated Symbols

LinkageKind = {`internal`, `external`}
LinkageJudg = {Linkage}

**(Linkage-UserItem)**
item ∈ {ProcedureDecl, StaticDecl, MethodDecl}    Vis(item) ∈ {`public`, `internal`}    Γ ⊢ Mangle(item) ⇓ sym
─────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Linkage(sym) ⇓ `external`

**(Linkage-UserItem-Internal)**
item ∈ {ProcedureDecl, StaticDecl, MethodDecl}    Vis(item) = `private`    Γ ⊢ Mangle(item) ⇓ sym
────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Linkage(sym) ⇓ `internal`

**(Linkage-StaticBinding)**
item = StaticBinding(StaticDecl(vis, _, _, _, _), x)    vis ∈ {`public`, `internal`}    Γ ⊢ Mangle(item) ⇓ sym
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Linkage(sym) ⇓ `external`

**(Linkage-StaticBinding-Internal)**
item = StaticBinding(StaticDecl(vis, _, _, _, _), x)    vis = `private`    Γ ⊢ Mangle(item) ⇓ sym
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Linkage(sym) ⇓ `internal`

**(Linkage-ClassMethod)**
item = ClassMethodDecl(vis, name, receiver, params, ret_opt, body_opt, span, doc_opt)    body_opt ≠ ⊥    Vis(item) ∈ {`public`, `internal`, `protected`}    Γ ⊢ Mangle(item) ⇓ sym
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Linkage(sym) ⇓ `external`

**(Linkage-ClassMethod-Internal)**
item = ClassMethodDecl(vis, name, receiver, params, ret_opt, body_opt, span, doc_opt)    body_opt ≠ ⊥    Vis(item) = `private`    Γ ⊢ Mangle(item) ⇓ sym
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Linkage(sym) ⇓ `internal`

**(Linkage-StateMethod)**
item = StateMethodDecl(vis, name, params, ret_opt, body, span, doc_opt)    Vis(item) ∈ {`public`, `internal`, `protected`}    Γ ⊢ Mangle(item) ⇓ sym
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Linkage(sym) ⇓ `external`

**(Linkage-StateMethod-Internal)**
item = StateMethodDecl(vis, name, params, ret_opt, body, span, doc_opt)    Vis(item) = `private`    Γ ⊢ Mangle(item) ⇓ sym
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Linkage(sym) ⇓ `internal`

**(Linkage-Transition)**
item = TransitionDecl(vis, name, params, target, body, span, doc_opt)    Vis(item) ∈ {`public`, `internal`, `protected`}    Γ ⊢ Mangle(item) ⇓ sym
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Linkage(sym) ⇓ `external`

**(Linkage-Transition-Internal)**
item = TransitionDecl(vis, name, params, target, body, span, doc_opt)    Vis(item) = `private`    Γ ⊢ Mangle(item) ⇓ sym
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Linkage(sym) ⇓ `internal`

**(Linkage-InitFn)**
InitFn(m) ⇓ sym
──────────────────────────────────────
Γ ⊢ Linkage(sym) ⇓ `internal`

**(Linkage-DeinitFn)**
DeinitFn(m) ⇓ sym
────────────────────────────────────────
Γ ⊢ Linkage(sym) ⇓ `internal`

**(Linkage-VTable)**
Mangle(VTableDecl(T, Cl)) ⇓ sym
────────────────────────────────────────────
Γ ⊢ Linkage(sym) ⇓ `internal`

**(Linkage-LiteralData)**
Mangle(LiteralData(kind, contents)) ⇓ sym
─────────────────────────────────────────────────
Γ ⊢ Linkage(sym) ⇓ `internal`

**(Linkage-DropGlue)**
DropGlueSym(T) ⇓ sym
─────────────────────────────────────────
Γ ⊢ Linkage(sym) ⇓ `internal`

**(Linkage-DefaultImpl)**
item = DefaultImpl(T, m)    Vis(m) ∈ {`public`, `internal`, `protected`}    Γ ⊢ Mangle(item) ⇓ sym
──────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Linkage(sym) ⇓ `external`

**(Linkage-DefaultImpl-Internal)**
item = DefaultImpl(T, m)    Vis(m) = `private`    Γ ⊢ Mangle(item) ⇓ sym
─────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Linkage(sym) ⇓ `internal`

**(Linkage-PanicSym)**
PanicSym ⇓ sym
────────────────────────────────
Γ ⊢ Linkage(sym) ⇓ `internal`

**(Linkage-RegionSym)**
RegionSym(proc) ⇓ sym
─────────────────────────────────────
Γ ⊢ Linkage(sym) ⇓ `internal`

**(Linkage-BuiltinSym)**
BuiltinSym(method) ⇓ sym
──────────────────────────────────────
Γ ⊢ Linkage(sym) ⇓ `internal`

**(Linkage-EntrySym)**
EntrySym ⇓ sym
────────────────────────────────
Γ ⊢ Linkage(sym) ⇓ `external`

### 6.4. Expression Lowering and Evaluation Order

EvalOrderJudg = {Children_LTR}

**(ArgsExprs-Empty)**
ArgsExprs([]) = []

**(ArgsExprs-Cons)**
ArgsExprs([⟨moved, e, span⟩] ++ rest) = [e] ++ ArgsExprs(rest)

**(FieldExprs-Empty)**
FieldExprs([]) = []

**(FieldExprs-Cons)**
FieldExprs([⟨f, e⟩] ++ rest) = [e] ++ FieldExprs(rest)

**(OptExprs-None)**
OptExprs(⊥, ⊥) = []

**(OptExprs-Lo)**
OptExprs(e, ⊥) = [e]

**(OptExprs-Hi)**
OptExprs(⊥, e) = [e]

**(OptExprs-Both)**
OptExprs(e_1, e_2) = [e_1, e_2]

LowerExprJudg = {LowerExpr, LowerUnOp, LowerBinOp, LowerCast, LowerList, LowerFieldInits, LowerOpt, LowerReadPlace, LowerWritePlace, LowerMovePlace, LowerAddrOf, LowerPlace}

**(EvalOrder-Literal)** `Children_LTR(Literal(â„“)) = []`.

**(EvalOrder-PtrNull)** `Children_LTR(PtrNullExpr) = []`.
  
**(EvalOrder-Ident)** `Children_LTR(Identifier(x)) = []`.
  
**(EvalOrder-Path)** `Children_LTR(Path(path, name)) = []`.
  
**(EvalOrder-Tuple)** `Children_LTR(TupleExpr(es)) = es`.
  
**(EvalOrder-Array)** `Children_LTR(ArrayExpr(es)) = es`.
  
**(EvalOrder-Record)** `Children_LTR(RecordExpr(tr, fields)) = FieldExprs(fields)`.
  
**(EvalOrder-Enum-Unit)** `Children_LTR(EnumLiteral(path, âŠ¥)) = []`.
  
**(EvalOrder-Enum-Tuple)** `Children_LTR(EnumLiteral(path, Paren(es))) = es`.
  
**(EvalOrder-Enum-Record)** `Children_LTR(EnumLiteral(path, Brace(fields))) = FieldExprs(fields)`.
  
**(EvalOrder-FieldAccess)** `Children_LTR(FieldAccess(base, f)) = [base]`.
  
**(EvalOrder-TupleAccess)** `Children_LTR(TupleAccess(base, i)) = [base]`.
  
**(EvalOrder-IndexAccess)** `Children_LTR(IndexAccess(base, idx)) = [base, idx]`.
  
**(EvalOrder-Call)** `Children_LTR(Call(callee, args)) = [callee] ++ ArgsExprs(args)`.
  
**(EvalOrder-MethodCall)** `Children_LTR(MethodCall(base, name, args)) = [base] ++ ArgsExprs(args)`.
  
**(EvalOrder-Unary)** `Children_LTR(Unary(op, e)) = [e]`.
  
**(EvalOrder-Binary)** `Children_LTR(Binary(op, e_1, e_2)) = [e_1, e_2]`.
  
**(EvalOrder-Cast)** `Children_LTR(Cast(e, T)) = [e]`.
  
**(EvalOrder-Transmute)** `Children_LTR(TransmuteExpr(T_1, T_2, e)) = [e]`.
  
**(EvalOrder-Propagate)** `Children_LTR(Propagate(e)) = [e]`.
  
**(EvalOrder-Range)** `Children_LTR(Range(kind, lo_opt, hi_opt)) = OptExprs(lo_opt, hi_opt)`.
  
**(EvalOrder-If)** `Children_LTR(IfExpr(cond, b1, b2)) = [cond]`.
  
**(EvalOrder-Match)** `Children_LTR(MatchExpr(scrut, arms)) = [scrut]`.
  
**(EvalOrder-Loop)** `Children_LTR(LoopInfinite(body)) = [body]`, `Children_LTR(LoopConditional(cond, body)) = [cond, body]`, `Children_LTR(LoopIter(pat, ty_opt, iter, body)) = [iter, body]`.
  
**(EvalOrder-Block)** `Children_LTR(BlockExpr(stmts, tail_opt)) = []`.
  
**(EvalOrder-UnsafeBlock)** `Children_LTR(UnsafeBlockExpr(b)) = []`.
  
**(EvalOrder-Move)** `Children_LTR(MoveExpr(p)) = []`.
  
**(EvalOrder-AddressOf)** `Children_LTR(AddressOf(p)) = []`.
  
**(EvalOrder-Deref)** `Children_LTR(Deref(e)) = [e]`.
  
**(EvalOrder-Alloc)** `Children_LTR(AllocExpr(r_opt, e)) = [e]`.

RetType(Γ) = R ⇔ ProcRet(Γ) = R

**(Lower-Expr-Correctness)**
∀ σ, Γ ⊢ EvalSigma(e, σ) ⇓ (out, σ') ⇒ ExecIRSigma(IR, σ) ⇓ (out, σ')
──────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(e) ⇓ ⟨IR, v⟩

ExprForms0 = {Literal(_), PtrNullExpr, Identifier(_), Path(_, _), ErrorExpr(_), TupleExpr(_), ArrayExpr(_), RecordExpr(_, _), EnumLiteral(_, _), FieldAccess(_, _), TupleAccess(_, _), IndexAccess(_, _), Call(_, _), MethodCall(_, _, _), Unary(_, _), Binary(_, _, _), Cast(_, _), TransmuteExpr(_, _, _), Propagate(_), Range(_, _, _), IfExpr(_, _, _), MatchExpr(_, _), LoopInfinite(_), LoopConditional(_, _), LoopIter(_, _, _, _), BlockExpr(_, _), UnsafeBlockExpr(_), MoveExpr(_), AddressOf(_), Deref(_), AllocExpr(_, _)}
LowerExprTotal(Γ) ⇔ ∀ e. e ∈ ExprForms0 ⇒ ∃ IR, v. Γ ⊢ LowerExpr(e) ⇓ ⟨IR, v⟩

ExecIRJudg = {ExecIRSigma, MoveStateSigma}

**(ExecIR-ReadVar)**
LookupVal(σ, x) = v
──────────────────────────────────────────────────────────────
ExecIRSigma(ReadVarIR(x), σ) ⇓ (Val(v), σ)

**(ExecIR-ReadPath)**
LookupValPath(σ, path, name) = v
──────────────────────────────────────────────────────────────────────
ExecIRSigma(ReadPathIR(path, name), σ) ⇓ (Val(v), σ)

**(ExecIR-StoreVar)**
Γ ⊢ WritePlaceSigma(Identifier(x), v, σ) ⇓ (sout, σ')
─────────────────────────────────────────────────────────────────────────────
ExecIRSigma(StoreVarIR(x, v), σ) ⇓ (sout, σ')

**(ExecIR-StoreVarNoDrop)**
Γ ⊢ WritePlaceSubSigma(Identifier(x), v, σ) ⇓ (sout, σ')
──────────────────────────────────────────────────────────────────────────────────
ExecIRSigma(StoreVarNoDropIR(x, v), σ) ⇓ (sout, σ')

**(ExecIR-BindVar)**
BindVal(σ, x, v) ⇓ (σ', b)
─────────────────────────────────────────────────────────────
ExecIRSigma(BindVarIR(x, v), σ) ⇓ (ok, σ')

**(ExecIR-ReadPtr)**
Γ ⊢ ReadPtrSigma(v_ptr, σ) ⇓ (out, σ')
──────────────────────────────────────────────────────────────
ExecIRSigma(ReadPtrIR(v_ptr), σ) ⇓ (out, σ')

**(ExecIR-WritePtr)**
Γ ⊢ WritePtrSigma(v_ptr, v, σ) ⇓ (sout, σ')
────────────────────────────────────────────────────────────────
ExecIRSigma(WritePtrIR(v_ptr, v), σ) ⇓ (sout, σ')

AllocTarget(σ, ⊥) = ActiveTarget(σ)
AllocTarget(σ, r) = ResolveTarget(σ, r)

**(ExecIR-Alloc)**
AllocTarget(σ, r_opt) = r    RegionAlloc(σ, r, v) ⇓ (σ', v')
──────────────────────────────────────────────────────────────────────────────
ExecIRSigma(AllocIR(r_opt, v), σ) ⇓ (Val(v'), σ')

**(MoveState-Root)**
PlaceRoot(p) = x    FieldHead(p) = ⊥    LookupBind(σ, x) = b    SetState(σ, b, Moved) ⇓ σ'
──────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ MoveStateSigma(p, σ) ⇓ σ'

**(MoveState-Field)**
PlaceRoot(p) = x    FieldHead(p) = f    LookupBind(σ, x) = b    BindState(σ, b) = s    PM(s, f) = s'    SetState(σ, b, s') ⇓ σ'
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ MoveStateSigma(p, σ) ⇓ σ'

**(ExecIR-MoveState)**
Γ ⊢ MoveStateSigma(p, σ) ⇓ σ'
──────────────────────────────────────────────────────────────
ExecIRSigma(MoveStateIR(p), σ) ⇓ (ok, σ')

**(ExecIR-Return)**
────────────────────────────────────────────────────────────────────────
ExecIRSigma(ReturnIR(v), σ) ⇓ (Ctrl(Return(v)), σ)

**(ExecIR-Result)**
────────────────────────────────────────────────────────────────────────
ExecIRSigma(ResultIR(v), σ) ⇓ (Ctrl(Result(v)), σ)

**(ExecIR-Break)**
────────────────────────────────────────────────────────────────────────────
ExecIRSigma(BreakIR(v_opt), σ) ⇓ (Ctrl(Break(v_opt)), σ)

**(ExecIR-Continue)**
────────────────────────────────────────────────────────────────────────
ExecIRSigma(ContinueIR, σ) ⇓ (Ctrl(Continue), σ)

**(ExecIR-Defer)**
AppendCleanup(σ, DeferBlock(b)) ⇓ σ'
──────────────────────────────────────────────────────────────
ExecIRSigma(DeferIR(b), σ) ⇓ (ok, σ')

ExecBlockBodyIRSigma(IR_s, IR_t, σ) ⇓ (out, σ') ⇔ ExecIRSigma(IR_s, σ) ⇓ (sout, σ_1) ∧ ((sout = ok ∧ IR_t = ε ∧ out = Val(()) ∧ σ' = σ_1) ∨ (sout = ok ∧ ExecIRSigma(IR_t, σ_1) ⇓ (out, σ')) ∨ (sout = Ctrl(Result(v)) ∧ out = Val(v) ∧ σ' = σ_1) ∨ (sout = Ctrl(κ) ∧ κ ≠ Result(_) ∧ out = Ctrl(κ) ∧ σ' = σ_1))
Γ ⊢ ExecInScopeIRSigma(IR_b, σ, scope) ⇓ (out, σ') ⇔ CurrentScopeId(σ) = scope ∧ ExecIRSigma(IR_b, σ) ⇓ (out, σ')
Γ ⊢ ExecBlockBindIRSigma(pat, v, IR_b, σ) ⇓ (out', σ'') ⇔ BindPatternVal(pat, v) ⇓ B ∧ BindOrder(pat, B) = binds ∧ BlockEnter(σ, binds) ⇓ (σ_1, scope) ∧ ExecIRSigma(IR_b, σ_1) ⇓ (out, σ_2) ∧ BlockExit(σ_2, scope, out) ⇓ (out', σ'')

**(ExecIR-If-True)**
v_c = true    ExecIRSigma(IR_t, σ) ⇓ (out, σ')
──────────────────────────────────────────────────────────────────────────────────────────
ExecIRSigma(IfIR(v_c, IR_t, v_t, IR_f, v_f), σ) ⇓ (out, σ')

**(ExecIR-If-False)**
v_c = false    ExecIRSigma(IR_f, σ) ⇓ (out, σ')
──────────────────────────────────────────────────────────────────────────────────────────
ExecIRSigma(IfIR(v_c, IR_t, v_t, IR_f, v_f), σ) ⇓ (out, σ')

**(ExecIR-Block)**
BlockEnter(σ, []) ⇓ (σ_1, scope)    ExecBlockBodyIRSigma(IR_s, IR_t, σ_1) ⇓ (out, σ_2)    BlockExit(σ_2, scope, out) ⇓ (out', σ_3)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
ExecIRSigma(BlockIR(IR_s, IR_t, v_t), σ) ⇓ (out', σ_3)

**(ExecIR-Match)**
Γ ⊢ MatchArmsSigma(arms, v_s, σ) ⇓ (out, σ')
───────────────────────────────────────────────────────────────
ExecIRSigma(MatchIR(v_s, arms), σ) ⇓ (out, σ')

**(ExecIR-Loop-Infinite-Step)**
ExecIRSigma(IR_b, σ) ⇓ (Val(v), σ_1)    ExecIRSigma(LoopIR(LoopInfinite, IR_b, v_b), σ_1) ⇓ (out, σ_2)
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
ExecIRSigma(LoopIR(LoopInfinite, IR_b, v_b), σ) ⇓ (out, σ_2)

**(ExecIR-Loop-Infinite-Continue)**
ExecIRSigma(IR_b, σ) ⇓ (Ctrl(Continue), σ_1)    ExecIRSigma(LoopIR(LoopInfinite, IR_b, v_b), σ_1) ⇓ (out, σ_2)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
ExecIRSigma(LoopIR(LoopInfinite, IR_b, v_b), σ) ⇓ (out, σ_2)

**(ExecIR-Loop-Infinite-Break)**
ExecIRSigma(IR_b, σ) ⇓ (Ctrl(Break(v_opt)), σ_1)    v = BreakVal(v_opt)
────────────────────────────────────────────────────────────────────────────────────────────────────────────
ExecIRSigma(LoopIR(LoopInfinite, IR_b, v_b), σ) ⇓ (Val(v), σ_1)

**(ExecIR-Loop-Infinite-Ctrl)**
ExecIRSigma(IR_b, σ) ⇓ (Ctrl(κ), σ_1)    κ ∈ {Return(_), Panic, Abort}
───────────────────────────────────────────────────────────────────────────────────────────────────────
ExecIRSigma(LoopIR(LoopInfinite, IR_b, v_b), σ) ⇓ (Ctrl(κ), σ_1)

**(ExecIR-Loop-Cond-False)**
ExecIRSigma(IR_c, σ) ⇓ (Val(false), σ_1)
──────────────────────────────────────────────────────────────────────────────────────────
ExecIRSigma(LoopIR(LoopConditional, IR_c, v_c, IR_b, v_b), σ) ⇓ (Val(()), σ_1)

**(ExecIR-Loop-Cond-True-Step)**
ExecIRSigma(IR_c, σ) ⇓ (Val(true), σ_1)    ExecIRSigma(IR_b, σ_1) ⇓ (Val(v), σ_2)    ExecIRSigma(LoopIR(LoopConditional, IR_c, v_c, IR_b, v_b), σ_2) ⇓ (out, σ_3)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
ExecIRSigma(LoopIR(LoopConditional, IR_c, v_c, IR_b, v_b), σ) ⇓ (out, σ_3)

**(ExecIR-Loop-Cond-Continue)**
ExecIRSigma(IR_c, σ) ⇓ (Val(true), σ_1)    ExecIRSigma(IR_b, σ_1) ⇓ (Ctrl(Continue), σ_2)    ExecIRSigma(LoopIR(LoopConditional, IR_c, v_c, IR_b, v_b), σ_2) ⇓ (out, σ_3)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
ExecIRSigma(LoopIR(LoopConditional, IR_c, v_c, IR_b, v_b), σ) ⇓ (out, σ_3)

**(ExecIR-Loop-Cond-Break)**
ExecIRSigma(IR_c, σ) ⇓ (Val(true), σ_1)    ExecIRSigma(IR_b, σ_1) ⇓ (Ctrl(Break(v_opt)), σ_2)    v = BreakVal(v_opt)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
ExecIRSigma(LoopIR(LoopConditional, IR_c, v_c, IR_b, v_b), σ) ⇓ (Val(v), σ_2)

**(ExecIR-Loop-Cond-Ctrl)**
ExecIRSigma(IR_c, σ) ⇓ (Ctrl(κ), σ_1)
─────────────────────────────────────────────────────────────────────────────────────────────
ExecIRSigma(LoopIR(LoopConditional, IR_c, v_c, IR_b, v_b), σ) ⇓ (Ctrl(κ), σ_1)

**(ExecIR-Loop-Cond-Body-Ctrl)**
ExecIRSigma(IR_c, σ) ⇓ (Val(true), σ_1)    ExecIRSigma(IR_b, σ_1) ⇓ (Ctrl(κ), σ_2)    κ ∈ {Return(_), Panic, Abort}
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
ExecIRSigma(LoopIR(LoopConditional, IR_c, v_c, IR_b, v_b), σ) ⇓ (Ctrl(κ), σ_2)

LoopIterIRJudg = {Γ ⊢ LoopIterExecIRSigma(pat, IR_b, it, σ) ⇓ (out, σ')}

**(ExecIR-Loop-Iter)**
ExecIRSigma(IR_i, σ) ⇓ (Val(v_iter), σ_1)    IterInit(v_iter) ⇓ it    Γ ⊢ LoopIterExecIRSigma(pat, IR_b, it, σ_1) ⇓ (out, σ_2)
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
ExecIRSigma(LoopIR(LoopIter, pat, ty_opt, IR_i, v_iter, IR_b, v_b), σ) ⇓ (out, σ_2)

**(ExecIR-Loop-Iter-Ctrl)**
ExecIRSigma(IR_i, σ) ⇓ (Ctrl(κ), σ_1)
───────────────────────────────────────────────────────────────────────────────────────────────
ExecIRSigma(LoopIR(LoopIter, pat, ty_opt, IR_i, v_iter, IR_b, v_b), σ) ⇓ (Ctrl(κ), σ_1)

**(LoopIterIR-Done)**
IterNext(it) ⇓ (⊥, it')
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ LoopIterExecIRSigma(pat, IR_b, it, σ) ⇓ (Val(()), σ)

**(LoopIterIR-Step-Val)**
IterNext(it) ⇓ (v, it')    Γ ⊢ ExecBlockBindIRSigma(pat, v, IR_b, σ) ⇓ (Val(v_b), σ_1)    Γ ⊢ LoopIterExecIRSigma(pat, IR_b, it', σ_1) ⇓ (out, σ_2)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LoopIterExecIRSigma(pat, IR_b, it, σ) ⇓ (out, σ_2)

**(LoopIterIR-Step-Continue)**
IterNext(it) ⇓ (v, it')    Γ ⊢ ExecBlockBindIRSigma(pat, v, IR_b, σ) ⇓ (Ctrl(Continue), σ_1)    Γ ⊢ LoopIterExecIRSigma(pat, IR_b, it', σ_1) ⇓ (out, σ_2)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LoopIterExecIRSigma(pat, IR_b, it, σ) ⇓ (out, σ_2)

**(LoopIterIR-Step-Break)**
IterNext(it) ⇓ (v, it')    Γ ⊢ ExecBlockBindIRSigma(pat, v, IR_b, σ) ⇓ (Ctrl(Break(v_opt)), σ_1)    v' = BreakVal(v_opt)
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LoopIterExecIRSigma(pat, IR_b, it, σ) ⇓ (Val(v'), σ_1)

**(LoopIterIR-Step-Ctrl)**
IterNext(it) ⇓ (v, it')    Γ ⊢ ExecBlockBindIRSigma(pat, v, IR_b, σ) ⇓ (Ctrl(κ), σ_1)    κ ∈ {Return(_), Panic, Abort}
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LoopIterExecIRSigma(pat, IR_b, it, σ) ⇓ (Ctrl(κ), σ_1)

**(ExecIR-Region)**
RegionNew(σ, v_o) ⇓ (σ_1, r, scope)    BindRegionAlias(σ_1, alias_opt, r) ⇓ σ_2    Γ ⊢ ExecInScopeIRSigma(IR_b, σ_2, scope) ⇓ (out, σ_3)    RegionRelease(σ_3, r, scope, out) ⇓ (out', σ_4)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
ExecIRSigma(RegionIR(v_o, alias_opt, IR_b, v_b), σ) ⇓ (StmtOutOf(out'), σ_4)

**(ExecIR-Frame-Implicit)**
ActiveTarget(σ) = r    FrameEnter(σ, r) ⇓ (σ_1, F, scope, mark)    Γ ⊢ ExecInScopeIRSigma(IR_b, σ_1, scope) ⇓ (out, σ_2)    FrameReset(σ_2, r, scope, mark, out) ⇓ (out', σ_3)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
ExecIRSigma(FrameIR(⊥, IR_b, v_b), σ) ⇓ (StmtOutOf(out'), σ_3)

**(ExecIR-Frame-Explicit)**
RegionHandleOf(v_r) = h    ResolveTarget(σ, h) = r_t    FrameEnter(σ, r_t) ⇓ (σ_1, F, scope, mark)    Γ ⊢ ExecInScopeIRSigma(IR_b, σ_1, scope) ⇓ (out, σ_2)    FrameReset(σ_2, r_t, scope, mark, out) ⇓ (out', σ_3)
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
ExecIRSigma(FrameIR(v_r, IR_b, v_b), σ) ⇓ (StmtOutOf(out'), σ_3)

**Lowering Helpers.**

**(LowerList-Empty)**
──────────────────────────────────────────────
Γ ⊢ LowerList([]) ⇓ ⟨ε, []⟩

**(LowerList-Cons)**
Γ ⊢ LowerExpr(e) ⇓ ⟨IR_e, v⟩    Γ ⊢ LowerList(es) ⇓ ⟨IR_s, vec_v⟩
──────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerList(e::es) ⇓ ⟨SeqIR(IR_e, IR_s), [v] ++ vec_v⟩

**(LowerFieldInits-Empty)**
────────────────────────────────────────────────────
Γ ⊢ LowerFieldInits([]) ⇓ ⟨ε, []⟩

**(LowerFieldInits-Cons)**
Γ ⊢ LowerExpr(e) ⇓ ⟨IR_e, v⟩    Γ ⊢ LowerFieldInits(fs) ⇓ ⟨IR_s, vec_f⟩
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerFieldInits([⟨f, e⟩] ++ fs) ⇓ ⟨SeqIR(IR_e, IR_s), [⟨f, v⟩] ++ vec_f⟩

**(LowerOpt-None)**
─────────────────────────────────────────────
Γ ⊢ LowerOpt(⊥) ⇓ ⟨ε, ⊥⟩

**(LowerOpt-Some)**
Γ ⊢ LowerExpr(e) ⇓ ⟨IR_e, v⟩
──────────────────────────────────────────
Γ ⊢ LowerOpt(e) ⇓ ⟨IR_e, v⟩

IsRangeExpr(e) ⇔ ExprType(e) = TypeRange

**Expression Lowering.** The rules below define `LowerExpr`.

**(Lower-Expr-Literal)**
T = ExprType(Literal(ℓ))    LiteralValue(ℓ, T) = v
──────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(Literal(ℓ)) ⇓ ⟨ε, v⟩

**(Lower-Expr-PtrNull)**
───────────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(PtrNullExpr) ⇓ ⟨ε, Ptr@Null(0x0)⟩

**(Lower-Expr-Ident-Local)**
Γ ⊢ ResolveValueName(x) ⇓ ent    ent.origin_opt = ⊥    Γ ⊢ LowerReadPlace(Identifier(x)) ⇓ ⟨IR, v⟩
────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(Identifier(x)) ⇓ ⟨IR, v⟩

**(Lower-Expr-Ident-Path)**
Γ ⊢ ResolveValueName(x) ⇓ ent    ent.origin_opt = mp    name = (ent.target_opt if present, else x)    PathOfModule(mp) = path
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(Identifier(x)) ⇓ ⟨ReadPathIR(path, name), v⟩

**(Lower-Expr-Path)**
──────────────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(Path(path, name)) ⇓ ⟨ReadPathIR(path, name), v⟩

**(Lower-Expr-Error)**
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(ErrorExpr(span)) ⇓ ⟨LowerPanic(ErrorExpr(span)), v_unreach⟩

**(Lower-Expr-Tuple)**
Γ ⊢ LowerList(es) ⇓ ⟨IR, vec_v⟩
──────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(TupleExpr(es)) ⇓ ⟨IR, (v_1, …, v_n)⟩

**(Lower-Expr-Array)**
Γ ⊢ LowerList(es) ⇓ ⟨IR, vec_v⟩
──────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(ArrayExpr(es)) ⇓ ⟨IR, [v_1, …, v_n]⟩

**(Lower-Expr-Record)**
Γ ⊢ LowerFieldInits(fields) ⇓ ⟨IR, vec_f⟩
─────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(RecordExpr(tr, fields)) ⇓ ⟨IR, RecordValue(tr, vec_f)⟩

**(Lower-Expr-Enum-Unit)**
──────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(EnumLiteral(path, ⊥)) ⇓ ⟨ε, EnumValue(path, ⊥)⟩

**(Lower-Expr-Enum-Tuple)**
Γ ⊢ LowerList(es) ⇓ ⟨IR, vec_v⟩
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(EnumLiteral(path, Paren(es))) ⇓ ⟨IR, EnumValue(path, TuplePayload(vec_v))⟩

**(Lower-Expr-Enum-Record)**
Γ ⊢ LowerFieldInits(fields) ⇓ ⟨IR, vec_f⟩
──────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(EnumLiteral(path, Brace(fields))) ⇓ ⟨IR, EnumValue(path, RecordPayload(vec_f))⟩

**(Lower-Expr-FieldAccess)**
Γ ⊢ LowerExpr(base) ⇓ ⟨IR_b, v_b⟩    FieldValue(v_b, f) = v_f
──────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(FieldAccess(base, f)) ⇓ ⟨IR_b, v_f⟩

**(Lower-Expr-TupleAccess)**
Γ ⊢ LowerExpr(base) ⇓ ⟨IR_b, v_b⟩    TupleValue(v_b, i) = v_i
──────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(TupleAccess(base, i)) ⇓ ⟨IR_b, v_i⟩

**(Lower-Expr-Index-Scalar)**
Γ ⊢ LowerExpr(base) ⇓ ⟨IR_b, v_b⟩    Γ ⊢ LowerExpr(idx) ⇓ ⟨IR_i, v_i⟩    ExprType(idx) = TypePrim("usize")    Γ ⊢ CheckIndex(Len(v_b), v_i) ⇓ ok    IndexValue(v_b, v_i) = v_e
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(IndexAccess(base, idx)) ⇓ ⟨SeqIR(IR_b, IR_i), v_e⟩

**(Lower-Expr-Index-Scalar-OOB)**
Γ ⊢ LowerExpr(base) ⇓ ⟨IR_b, v_b⟩    Γ ⊢ LowerExpr(idx) ⇓ ⟨IR_i, v_i⟩    ExprType(idx) = TypePrim("usize")    ¬(0 ≤ v_i < Len(v_b))    Γ ⊢ LowerPanic(Bounds) ⇓ IR_k
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(IndexAccess(base, idx)) ⇓ ⟨SeqIR(IR_b, IR_i, IR_k), v_unreach⟩

**(Lower-Expr-Index-Range)**
Γ ⊢ LowerExpr(base) ⇓ ⟨IR_b, v_b⟩    Γ ⊢ LowerExpr(idx) ⇓ ⟨IR_i, v_r⟩    IsRangeExpr(idx)    Γ ⊢ CheckRange(Len(v_b), v_r) ⇓ ok    SliceValue(v_b, v_r) = v_s
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(IndexAccess(base, idx)) ⇓ ⟨SeqIR(IR_b, IR_i), v_s⟩

**(Lower-Expr-Index-Range-OOB)**
Γ ⊢ LowerExpr(base) ⇓ ⟨IR_b, v_b⟩    Γ ⊢ LowerExpr(idx) ⇓ ⟨IR_i, v_r⟩    IsRangeExpr(idx)    SliceBounds(v_r, Len(v_b)) = ⊥    Γ ⊢ LowerPanic(Bounds) ⇓ IR_k
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(IndexAccess(base, idx)) ⇓ ⟨SeqIR(IR_b, IR_i, IR_k), v_unreach⟩

**(Lower-Expr-Call-PanicOut)**
Γ ⊢ LowerExpr(callee) ⇓ ⟨IR_c, v_c⟩    Γ ⊢ LowerArgs(Params(Call(callee, args)), args) ⇓ ⟨IR_a, vec_v⟩    NeedsPanicOut(callee)
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(Call(callee, args)) ⇓ ⟨SeqIR(IR_c, IR_a, CallIR(v_c, vec_v ++ [PanicOutName]), PanicCheck), v_call⟩

**(Lower-Expr-Call-NoPanicOut)**
Γ ⊢ LowerExpr(callee) ⇓ ⟨IR_c, v_c⟩    Γ ⊢ LowerArgs(Params(Call(callee, args)), args) ⇓ ⟨IR_a, vec_v⟩    ¬ NeedsPanicOut(callee)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(Call(callee, args)) ⇓ ⟨SeqIR(IR_c, IR_a, CallIR(v_c, vec_v)), v_call⟩

**(Lower-Expr-MethodCall)**
Γ ⊢ LowerMethodCall(MethodCall(base, name, args)) ⇓ ⟨IR, v_call⟩
─────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(MethodCall(base, name, args)) ⇓ ⟨IR, v_call⟩

**(Lower-Expr-Unary)**
Γ ⊢ LowerUnOp(op, e) ⇓ ⟨IR, v⟩
───────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(Unary(op, e)) ⇓ ⟨IR, v⟩

**(Lower-Expr-Bin-And)**
Γ ⊢ LowerExpr(e_1) ⇓ ⟨IR_1, v_1⟩    Γ ⊢ LowerExpr(e_2) ⇓ ⟨IR_2, v_2⟩
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(Binary("&&", e_1, e_2)) ⇓ ⟨SeqIR(IR_1, IfIR(v_1, IR_2, v_2, ε, false)), v_and⟩

**(Lower-Expr-Bin-Or)**
Γ ⊢ LowerExpr(e_1) ⇓ ⟨IR_1, v_1⟩    Γ ⊢ LowerExpr(e_2) ⇓ ⟨IR_2, v_2⟩
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(Binary("||", e_1, e_2)) ⇓ ⟨SeqIR(IR_1, IfIR(v_1, ε, true, IR_2, v_2)), v_or⟩

**(Lower-Expr-Binary)**
op ∉ {"&&", "||"}    Γ ⊢ LowerBinOp(op, e_1, e_2) ⇓ ⟨IR, v⟩
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(Binary(op, e_1, e_2)) ⇓ ⟨IR, v⟩

**(Lower-Expr-Cast)**
Γ ⊢ LowerCast(e, T) ⇓ ⟨IR, v⟩
───────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(Cast(e, T)) ⇓ ⟨IR, v⟩

**(Lower-Expr-Transmute)**
Γ ⊢ LowerTransmute(T_1, T_2, e) ⇓ ⟨IR, v⟩
────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(TransmuteExpr(T_1, T_2, e)) ⇓ ⟨IR, v⟩

**(Lower-Expr-Propagate-Success)**
Γ ⊢ LowerExpr(e) ⇓ ⟨IR_e, v⟩    U = ExprType(e)    SuccessMember(RetType(Γ), U) = T_s    UnionCase(v) = ⟨T_s, v_s⟩
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(Propagate(e)) ⇓ ⟨IR_e, v_s⟩

**(Lower-Expr-Propagate-Return)**
Γ ⊢ LowerExpr(e) ⇓ ⟨IR_e, v⟩    U = ExprType(e)    SuccessMember(RetType(Γ), U) = T_s    UnionCase(v) = ⟨T_e, v_e⟩    T_e ≠ T_s
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(Propagate(e)) ⇓ ⟨SeqIR(IR_e, ReturnIR(v_e)), v_unreach⟩

**(Lower-Expr-Range)**
Γ ⊢ LowerRangeExpr(Range(kind, lo_opt, hi_opt)) ⇓ ⟨IR, v⟩
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(Range(kind, lo_opt, hi_opt)) ⇓ ⟨IR, v⟩

**(Lower-Expr-If)**
Γ ⊢ LowerExpr(cond) ⇓ ⟨IR_c, v_c⟩    Γ ⊢ LowerBlock(b_1) ⇓ ⟨IR_1, v_1⟩    Γ ⊢ LowerBlock(b_2) ⇓ ⟨IR_2, v_2⟩
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(IfExpr(cond, b_1, b_2)) ⇓ ⟨SeqIR(IR_c, IfIR(v_c, IR_1, v_1, IR_2, v_2)), v_if⟩

**(Lower-Expr-Match)**
Γ ⊢ LowerMatch(scrut, arms) ⇓ ⟨IR, v⟩
────────────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(MatchExpr(scrut, arms)) ⇓ ⟨IR, v⟩

**(Lower-Expr-LoopInf)**
Γ ⊢ LowerLoop(LoopInfinite(body)) ⇓ ⟨IR, v⟩
─────────────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(LoopInfinite(body)) ⇓ ⟨IR, v⟩

**(Lower-Expr-LoopCond)**
Γ ⊢ LowerLoop(LoopConditional(cond, body)) ⇓ ⟨IR, v⟩
────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(LoopConditional(cond, body)) ⇓ ⟨IR, v⟩

**(Lower-Expr-LoopIter)**
Γ ⊢ LowerLoop(LoopIter(pat, ty_opt, iter, body)) ⇓ ⟨IR, v⟩
────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(LoopIter(pat, ty_opt, iter, body)) ⇓ ⟨IR, v⟩

**(Lower-Expr-Block)**
Γ ⊢ LowerBlock(BlockExpr(stmts, tail_opt)) ⇓ ⟨IR, v⟩
────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(BlockExpr(stmts, tail_opt)) ⇓ ⟨IR, v⟩

**(Lower-Expr-UnsafeBlock)**
Γ ⊢ LowerBlock(b) ⇓ ⟨IR, v⟩
─────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(UnsafeBlockExpr(b)) ⇓ ⟨IR, v⟩

**(Lower-Expr-Move)**
Γ ⊢ LowerMovePlace(p) ⇓ ⟨IR, v⟩
──────────────────────────────────────────────────
Γ ⊢ LowerExpr(MoveExpr(p)) ⇓ ⟨IR, v⟩

**(Lower-Expr-AddressOf)**
Γ ⊢ LowerAddrOf(p) ⇓ ⟨IR, addr⟩
────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(AddressOf(p)) ⇓ ⟨IR, Ptr@Valid(addr)⟩

**(Lower-Expr-Deref)**
Γ ⊢ LowerExpr(e) ⇓ ⟨IR_e, v_ptr⟩    Γ ⊢ LowerRawDeref(v_ptr) ⇓ ⟨IR_d, v⟩
──────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(Deref(e)) ⇓ ⟨SeqIR(IR_e, IR_d), v⟩

**(Lower-Expr-Alloc)**
Γ ⊢ LowerExpr(e) ⇓ ⟨IR_e, v⟩
────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerExpr(AllocExpr(r_opt, e)) ⇓ ⟨SeqIR(IR_e, AllocIR(r_opt, v)), v_alloc⟩

**Operator and Cast Lowering.**

OpPanicReason(op, v) = r
OpPanicReason(op, v_1, v_2) = r
OpPanicReason(op, v) =
 Overflow    if op = "-"
 Other       otherwise
OpPanicReason(op, v_1, v_2) =
 DivZero     if op ∈ {"/", "%"} ∧ IntValue(v_1, t) ∧ IntValue(v_2, t) ∧ v_2 = 0
 Overflow    if op ∈ {"/", "%"} ∧ IntValue(v_1, t) ∧ IntValue(v_2, t) ∧ v_2 ≠ 0
 Shift       if op ∈ ShiftOps
 Overflow    if op ∈ {"+", "-", "*", "**"} ∧ IntValue(v_1, t) ∧ IntValue(v_2, t)
 Other       otherwise

**(Lower-UnOp-Ok)**
Γ ⊢ LowerExpr(e) ⇓ ⟨IR_e, v⟩    UnOp(op, v) ⇓ v'
────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerUnOp(op, e) ⇓ ⟨IR_e, v'⟩

**(Lower-UnOp-Panic)**
Γ ⊢ LowerExpr(e) ⇓ ⟨IR_e, v⟩    UnOp(op, v) undefined    OpPanicReason(op, v) = r    Γ ⊢ LowerPanic(r) ⇓ IR_k
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerUnOp(op, e) ⇓ ⟨SeqIR(IR_e, IR_k), v_unreach⟩

**(Lower-BinOp-Ok)**
Γ ⊢ LowerExpr(e_1) ⇓ ⟨IR_1, v_1⟩    Γ ⊢ LowerExpr(e_2) ⇓ ⟨IR_2, v_2⟩    BinOp(op, v_1, v_2) ⇓ v
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerBinOp(op, e_1, e_2) ⇓ ⟨SeqIR(IR_1, IR_2), v⟩

**(Lower-BinOp-Panic)**
Γ ⊢ LowerExpr(e_1) ⇓ ⟨IR_1, v_1⟩    Γ ⊢ LowerExpr(e_2) ⇓ ⟨IR_2, v_2⟩    BinOp(op, v_1, v_2) undefined    OpPanicReason(op, v_1, v_2) = r    Γ ⊢ LowerPanic(r) ⇓ IR_k
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerBinOp(op, e_1, e_2) ⇓ ⟨SeqIR(IR_1, IR_2, IR_k), v_unreach⟩

**(Lower-Cast)**
Γ ⊢ LowerExpr(e) ⇓ ⟨IR, v⟩    S = ExprType(e)    CastVal(S, T, v) ⇓ v'
───────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerCast(e, T) ⇓ ⟨IR, v'⟩

**(Lower-Cast-Panic)**
Γ ⊢ LowerExpr(e) ⇓ ⟨IR, v⟩    S = ExprType(e)    CastVal(S, T, v) undefined    Γ ⊢ LowerPanic(Cast) ⇓ IR_k
─────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerCast(e, T) ⇓ ⟨SeqIR(IR, IR_k), v_unreach⟩

PlaceForms0 = {Identifier(_), FieldAccess(_, _), TupleAccess(_, _), IndexAccess(_, _), Deref(_)}
LowerPlaceTotal(Γ) ⇔ ∀ p. p ∈ PlaceForms0 ⇒ ∃ l. Γ ⊢ LowerPlace(p) ⇓ l
LowerPlacePreserve(Γ) ⇔
 ∀ p, l. Γ ⊢ LowerPlace(p) ⇓ l ⇒ (∀ σ, out, σ'. Γ ⊢ ReadPlaceSigma(p, σ) ⇓ (out, σ') ⇒ Γ ⊢ ReadPlaceSigma(l, σ) ⇓ (out, σ')) ∧ (∀ σ, v, sout, σ'. Γ ⊢ WritePlaceSigma(p, v, σ) ⇓ (sout, σ') ⇒ Γ ⊢ WritePlaceSigma(l, v, σ) ⇓ (sout, σ'))

**Place Lowering Rules.**

**(Lower-Place-Ident)**
────────────────────────────────────────────────────────────
Γ ⊢ LowerPlace(Identifier(x)) ⇓ Identifier(x)

**(Lower-Place-Field)**
Γ ⊢ LowerPlace(p) ⇓ l
─────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerPlace(FieldAccess(p, f)) ⇓ FieldAccess(l, f)

**(Lower-Place-Tuple)**
Γ ⊢ LowerPlace(p) ⇓ l
─────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerPlace(TupleAccess(p, i)) ⇓ TupleAccess(l, i)

**(Lower-Place-Index)**
Γ ⊢ LowerPlace(p) ⇓ l
────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerPlace(IndexAccess(p, idx)) ⇓ IndexAccess(l, idx)

**(Lower-Place-Deref)**
Γ ⊢ LowerPlace(p) ⇓ l
────────────────────────────────────────────────────────────
Γ ⊢ LowerPlace(Deref(p)) ⇓ Deref(l)

**Place Access Lowering.**

DropOnAssignRoot(p) ⇔ PlaceRoot(p) = x ∧ ((Γ ⊢ ResolveValueName(x) ⇓ ent ∧ ent.origin_opt = ⊥ ∧ BindInfo(x).mov = immov ∧ BindInfo(x).resp = resp) ∨ (Γ ⊢ ResolveValueName(x) ⇓ ent ∧ ent.origin_opt = mp ∧ name = (ent.target_opt if present, else x) ∧ path = PathOfModule(mp) ∧ StaticBindInfo(path, name).mov = immov ∧ StaticBindInfo(path, name).resp = resp))

**(Lower-ReadPlace-Ident-Local)**
Γ ⊢ ResolveValueName(x) ⇓ ent    ent.origin_opt = ⊥
─────────────────────────────────────────────────────────────────────
Γ ⊢ LowerReadPlace(Identifier(x)) ⇓ ⟨ReadVarIR(x), v⟩

**(Lower-ReadPlace-Ident-Path)**
Γ ⊢ ResolveValueName(x) ⇓ ent    ent.origin_opt = mp    name = (ent.target_opt if present, else x)    PathOfModule(mp) = path
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerReadPlace(Identifier(x)) ⇓ ⟨ReadPathIR(path, name), v⟩

**(Lower-ReadPlace-Field)**
Γ ⊢ LowerReadPlace(p) ⇓ ⟨IR_p, v_p⟩    FieldValue(v_p, f) = v_f
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerReadPlace(FieldAccess(p, f)) ⇓ ⟨IR_p, v_f⟩

**(Lower-ReadPlace-Tuple)**
Γ ⊢ LowerReadPlace(p) ⇓ ⟨IR_p, v_p⟩    TupleValue(v_p, i) = v_i
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerReadPlace(TupleAccess(p, i)) ⇓ ⟨IR_p, v_i⟩

**(Lower-ReadPlace-Index-Scalar)**
Γ ⊢ LowerReadPlace(p) ⇓ ⟨IR_p, v_p⟩    Γ ⊢ LowerExpr(idx) ⇓ ⟨IR_i, v_i⟩    ExprType(idx) = TypePrim("usize")    Γ ⊢ CheckIndex(Len(v_p), v_i) ⇓ ok    IndexValue(v_p, v_i) = v_e
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerReadPlace(IndexAccess(p, idx)) ⇓ ⟨SeqIR(IR_p, IR_i), v_e⟩

**(Lower-ReadPlace-Index-Scalar-OOB)**
Γ ⊢ LowerReadPlace(p) ⇓ ⟨IR_p, v_p⟩    Γ ⊢ LowerExpr(idx) ⇓ ⟨IR_i, v_i⟩    ExprType(idx) = TypePrim("usize")    ¬(0 ≤ v_i < Len(v_p))    Γ ⊢ LowerPanic(Bounds) ⇓ IR_k
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerReadPlace(IndexAccess(p, idx)) ⇓ ⟨SeqIR(IR_p, IR_i, IR_k), v_unreach⟩

**(Lower-ReadPlace-Index-Range)**
Γ ⊢ LowerReadPlace(p) ⇓ ⟨IR_p, v_p⟩    Γ ⊢ LowerExpr(idx) ⇓ ⟨IR_i, v_r⟩    IsRangeExpr(idx)    Γ ⊢ CheckRange(Len(v_p), v_r) ⇓ ok    SliceValue(v_p, v_r) = v_s
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerReadPlace(IndexAccess(p, idx)) ⇓ ⟨SeqIR(IR_p, IR_i), v_s⟩

**(Lower-ReadPlace-Index-Range-OOB)**
Γ ⊢ LowerReadPlace(p) ⇓ ⟨IR_p, v_p⟩    Γ ⊢ LowerExpr(idx) ⇓ ⟨IR_i, v_r⟩    IsRangeExpr(idx)    SliceBounds(v_r, Len(v_p)) = ⊥    Γ ⊢ LowerPanic(Bounds) ⇓ IR_k
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerReadPlace(IndexAccess(p, idx)) ⇓ ⟨SeqIR(IR_p, IR_i, IR_k), v_unreach⟩

**(Lower-ReadPlace-Deref)**
Γ ⊢ LowerReadPlace(p) ⇓ ⟨IR_p, v_ptr⟩    Γ ⊢ LowerRawDeref(v_ptr) ⇓ ⟨IR_d, v⟩
──────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerReadPlace(Deref(p)) ⇓ ⟨SeqIR(IR_p, IR_d), v⟩

**(Lower-AddrOf-Ident-Local)**
Γ ⊢ ResolveValueName(x) ⇓ ent    ent.origin_opt = ⊥
─────────────────────────────────────────────────────────────────────
Γ ⊢ LowerAddrOf(Identifier(x)) ⇓ ⟨ε, AddrOfBind(x)⟩

**(Lower-AddrOf-Ident-Path)**
Γ ⊢ ResolveValueName(x) ⇓ ent    ent.origin_opt = mp    name = (ent.target_opt if present, else x)    PathOfModule(mp) = path    StaticSymPath(path, name) = sym    StaticAddr(path, name) = addr    IR_p = CheckPoison(m) if ProcModule(sym) = m, otherwise ε
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerAddrOf(Identifier(x)) ⇓ ⟨IR_p, addr⟩

**(Lower-AddrOf-Field)**
Γ ⊢ LowerAddrOf(p) ⇓ ⟨IR_p, addr⟩    T_b = ExprType(p)    FieldAddr(T_b, addr, f) = addr'
─────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerAddrOf(FieldAccess(p, f)) ⇓ ⟨IR_p, addr'⟩

**(Lower-AddrOf-Tuple)**
Γ ⊢ LowerAddrOf(p) ⇓ ⟨IR_p, addr⟩    T_b = ExprType(p)    TupleAddr(T_b, addr, i) = addr'
────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerAddrOf(TupleAccess(p, i)) ⇓ ⟨IR_p, addr'⟩


**(Lower-AddrOf-Index)**
Γ ⊢ LowerAddrOf(p) ⇓ ⟨IR_p, addr⟩    Γ ⊢ LowerReadPlace(p) ⇓ ⟨IR_r, v_p⟩    Γ ⊢ LowerExpr(idx) ⇓ ⟨IR_i, v_i⟩    ExprType(idx) = TypePrim("usize")    T_b = ExprType(p)    Γ ⊢ CheckIndex(Len(v_p), v_i) ⇓ ok    IndexAddr(T_b, addr, v_i) = addr'
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerAddrOf(IndexAccess(p, idx)) ⇓ ⟨SeqIR(IR_p, IR_r, IR_i), addr'⟩

**(Lower-AddrOf-Index-OOB)**
Γ ⊢ LowerAddrOf(p) ⇓ ⟨IR_p, addr⟩    Γ ⊢ LowerReadPlace(p) ⇓ ⟨IR_r, v_p⟩    Γ ⊢ LowerExpr(idx) ⇓ ⟨IR_i, v_i⟩    ExprType(idx) = TypePrim("usize")    ¬(0 ≤ v_i < Len(v_p))    Γ ⊢ LowerPanic(Bounds) ⇓ IR_k
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerAddrOf(IndexAccess(p, idx)) ⇓ ⟨SeqIR(IR_p, IR_r, IR_i, IR_k), v_unreach⟩

**(Lower-AddrOf-Deref)**
Γ ⊢ LowerReadPlace(p) ⇓ ⟨IR_p, v_ptr⟩    PtrType(v_ptr) = TypePtr(T, `Valid`)    PtrAddr(v_ptr) = addr
────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerAddrOf(Deref(p)) ⇓ ⟨IR_p, addr⟩

**(Lower-AddrOf-Deref-Null)**
Γ ⊢ LowerReadPlace(p) ⇓ ⟨IR_p, v_ptr⟩    PtrType(v_ptr) = TypePtr(T, `Null`)    PtrAddr(v_ptr) = addr    Γ ⊢ LowerPanic(NullDeref) ⇓ IR_k
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerAddrOf(Deref(p)) ⇓ ⟨SeqIR(IR_p, IR_k), addr⟩

**(Lower-AddrOf-Deref-Expired)**
Γ ⊢ LowerReadPlace(p) ⇓ ⟨IR_p, v_ptr⟩    PtrType(v_ptr) = TypePtr(T, `Expired`)    PtrAddr(v_ptr) = addr    Γ ⊢ LowerPanic(ExpiredDeref) ⇓ IR_k
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerAddrOf(Deref(p)) ⇓ ⟨SeqIR(IR_p, IR_k), addr⟩

**(Lower-AddrOf-Deref-Raw)**
Γ ⊢ LowerReadPlace(p) ⇓ ⟨IR_p, v_ptr⟩    PtrType(v_ptr) = TypeRawPtr(q, T)    PtrAddr(v_ptr) = addr
──────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerAddrOf(Deref(p)) ⇓ ⟨IR_p, addr⟩

**(Lower-WritePlace-Ident-Local)**
Γ ⊢ ResolveValueName(x) ⇓ ent    ent.origin_opt = ⊥
────────────────────────────────────────────────────────────────────
Γ ⊢ LowerWritePlace(Identifier(x), v) ⇓ StoreVarIR(x, v)

**(Lower-WritePlace-Ident-Path)**
Γ ⊢ ResolveValueName(x) ⇓ ent    ent.origin_opt = mp    name = (ent.target_opt if present, else x)    PathOfModule(mp) = path    StaticSymPath(path, name) = sym    IR_p = CheckPoison(m) if ProcModule(sym) = m, otherwise ε    IR_d = EmitDrop(StaticType(path, name), Load(@sym, StaticType(path, name))) if StaticBindInfo(path, name).resp = resp, otherwise ε
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerWritePlace(Identifier(x), v) ⇓ SeqIR(IR_p, IR_d, StoreGlobal(sym, v))

**(Lower-WritePlace-Field)**
Γ ⊢ LowerReadPlace(p) ⇓ ⟨IR_p, v_p⟩    FieldValue(v_p, f) = v_f    T_f = ExprType(FieldAccess(p, f))    IR_d = EmitDrop(T_f, v_f) if DropOnAssignRoot(p), otherwise ε    FieldUpdate(v_p, f, v) = v_p'    Γ ⊢ LowerWritePlaceSub(p, v_p') ⇓ IR_w
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerWritePlace(FieldAccess(p, f), v) ⇓ SeqIR(IR_p, IR_d, IR_w)

**(Lower-WritePlace-Tuple)**
Γ ⊢ LowerReadPlace(p) ⇓ ⟨IR_p, v_p⟩    TupleValue(v_p, i) = v_i    T_i = ExprType(TupleAccess(p, i))    IR_d = EmitDrop(T_i, v_i) if DropOnAssignRoot(p), otherwise ε    TupleUpdate(v_p, i, v) = v_p'    Γ ⊢ LowerWritePlaceSub(p, v_p') ⇓ IR_w
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerWritePlace(TupleAccess(p, i), v) ⇓ SeqIR(IR_p, IR_d, IR_w)

**(Lower-WritePlace-Index-Scalar)**
Γ ⊢ LowerReadPlace(p) ⇓ ⟨IR_p, v_p⟩    Γ ⊢ LowerExpr(idx) ⇓ ⟨IR_i, v_i⟩    ExprType(idx) = TypePrim("usize")    Γ ⊢ CheckIndex(Len(v_p), v_i) ⇓ ok    IndexValue(v_p, v_i) = v_e    T_e = ExprType(IndexAccess(p, idx))    IR_d = EmitDrop(T_e, v_e) if DropOnAssignRoot(p), otherwise ε    IndexUpdate(v_p, v_i, v) = v_p'    Γ ⊢ LowerWritePlaceSub(p, v_p') ⇓ IR_w
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerWritePlace(IndexAccess(p, idx), v) ⇓ SeqIR(IR_p, IR_i, IR_d, IR_w)

**(Lower-WritePlace-Index-Scalar-OOB)**
Γ ⊢ LowerReadPlace(p) ⇓ ⟨IR_p, v_p⟩    Γ ⊢ LowerExpr(idx) ⇓ ⟨IR_i, v_i⟩    ExprType(idx) = TypePrim("usize")    ¬(0 ≤ v_i < Len(v_p))    Γ ⊢ LowerPanic(Bounds) ⇓ IR_k
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerWritePlace(IndexAccess(p, idx), v) ⇓ SeqIR(IR_p, IR_i, IR_k)

**(Lower-WritePlace-Index-Range)**
Γ ⊢ LowerReadPlace(p) ⇓ ⟨IR_p, v_p⟩    Γ ⊢ LowerExpr(idx) ⇓ ⟨IR_i, v_r⟩    IsRangeExpr(idx)    Γ ⊢ CheckRange(Len(v_p), v_r) ⇓ ok    SliceBounds(v_r, Len(v_p)) = (start, end)    SliceLen(v) = n    n = end - start    SliceUpdate(v_p, start, v) ⇓ v_p'    Γ ⊢ LowerWritePlaceSub(p, v_p') ⇓ IR_w
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerWritePlace(IndexAccess(p, idx), v) ⇓ SeqIR(IR_p, IR_i, IR_w)

**(Lower-WritePlace-Index-Range-OOB)**
Γ ⊢ LowerReadPlace(p) ⇓ ⟨IR_p, v_p⟩    Γ ⊢ LowerExpr(idx) ⇓ ⟨IR_i, v_r⟩    IsRangeExpr(idx)    SliceBounds(v_r, Len(v_p)) = ⊥    Γ ⊢ LowerPanic(Bounds) ⇓ IR_k
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerWritePlace(IndexAccess(p, idx), v) ⇓ SeqIR(IR_p, IR_i, IR_k)

**(Lower-WritePlace-Index-Range-Len)**
Γ ⊢ LowerReadPlace(p) ⇓ ⟨IR_p, v_p⟩    Γ ⊢ LowerExpr(idx) ⇓ ⟨IR_i, v_r⟩    IsRangeExpr(idx)    SliceBounds(v_r, Len(v_p)) = (start, end)    SliceLen(v) = n    n ≠ end - start    Γ ⊢ LowerPanic(Bounds) ⇓ IR_k
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerWritePlace(IndexAccess(p, idx), v) ⇓ SeqIR(IR_p, IR_i, IR_k)

**(Lower-WritePlace-Deref)**
Γ ⊢ LowerReadPlace(p) ⇓ ⟨IR_p, v_ptr⟩
────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerWritePlace(Deref(p), v) ⇓ SeqIR(IR_p, WritePtrIR(v_ptr, v))

**Write-Subplace Lowering.**

**(LowerWriteSub-Ident-Local)**
Γ ⊢ ResolveValueName(x) ⇓ ent    ent.origin_opt = ⊥
────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerWritePlaceSub(Identifier(x), v) ⇓ StoreVarNoDropIR(x, v)

**(LowerWriteSub-Ident-Path)**
Γ ⊢ ResolveValueName(x) ⇓ ent    ent.origin_opt = mp    name = (ent.target_opt if present, else x)    PathOfModule(mp) = path    StaticSymPath(path, name) = sym    IR_p = CheckPoison(m) if ProcModule(sym) = m, otherwise ε
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerWritePlaceSub(Identifier(x), v) ⇓ SeqIR(IR_p, StoreGlobal(sym, v))

**(LowerWriteSub-Field)**
Γ ⊢ LowerReadPlace(p) ⇓ ⟨IR_p, v_p⟩    FieldUpdate(v_p, f, v) = v_p'    Γ ⊢ LowerWritePlaceSub(p, v_p') ⇓ IR_w
──────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerWritePlaceSub(FieldAccess(p, f), v) ⇓ SeqIR(IR_p, IR_w)

**(LowerWriteSub-Tuple)**
Γ ⊢ LowerReadPlace(p) ⇓ ⟨IR_p, v_p⟩    TupleUpdate(v_p, i, v) = v_p'    Γ ⊢ LowerWritePlaceSub(p, v_p') ⇓ IR_w
──────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerWritePlaceSub(TupleAccess(p, i), v) ⇓ SeqIR(IR_p, IR_w)

**(LowerWriteSub-Index-Scalar)**
Γ ⊢ LowerReadPlace(p) ⇓ ⟨IR_p, v_p⟩    Γ ⊢ LowerExpr(idx) ⇓ ⟨IR_i, v_i⟩    ExprType(idx) = TypePrim("usize")    Γ ⊢ CheckIndex(Len(v_p), v_i) ⇓ ok    IndexUpdate(v_p, v_i, v) = v_p'    Γ ⊢ LowerWritePlaceSub(p, v_p') ⇓ IR_w
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerWritePlaceSub(IndexAccess(p, idx), v) ⇓ SeqIR(IR_p, IR_i, IR_w)

**(LowerWriteSub-Index-Scalar-OOB)**
Γ ⊢ LowerReadPlace(p) ⇓ ⟨IR_p, v_p⟩    Γ ⊢ LowerExpr(idx) ⇓ ⟨IR_i, v_i⟩    ExprType(idx) = TypePrim("usize")    ¬(0 ≤ v_i < Len(v_p))    Γ ⊢ LowerPanic(Bounds) ⇓ IR_k
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerWritePlaceSub(IndexAccess(p, idx), v) ⇓ SeqIR(IR_p, IR_i, IR_k)

**(LowerWriteSub-Index-Range)**
Γ ⊢ LowerReadPlace(p) ⇓ ⟨IR_p, v_p⟩    Γ ⊢ LowerExpr(idx) ⇓ ⟨IR_i, v_r⟩    IsRangeExpr(idx)    Γ ⊢ CheckRange(Len(v_p), v_r) ⇓ ok    SliceBounds(v_r, Len(v_p)) = (start, end)    SliceLen(v) = n    n = end - start    SliceUpdate(v_p, start, v) ⇓ v_p'    Γ ⊢ LowerWritePlaceSub(p, v_p') ⇓ IR_w
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerWritePlaceSub(IndexAccess(p, idx), v) ⇓ SeqIR(IR_p, IR_i, IR_w)

**(LowerWriteSub-Index-Range-OOB)**
Γ ⊢ LowerReadPlace(p) ⇓ ⟨IR_p, v_p⟩    Γ ⊢ LowerExpr(idx) ⇓ ⟨IR_i, v_r⟩    IsRangeExpr(idx)    SliceBounds(v_r, Len(v_p)) = ⊥    Γ ⊢ LowerPanic(Bounds) ⇓ IR_k
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerWritePlaceSub(IndexAccess(p, idx), v) ⇓ SeqIR(IR_p, IR_i, IR_k)

**(LowerWriteSub-Index-Range-Len)**
Γ ⊢ LowerReadPlace(p) ⇓ ⟨IR_p, v_p⟩    Γ ⊢ LowerExpr(idx) ⇓ ⟨IR_i, v_r⟩    IsRangeExpr(idx)    SliceBounds(v_r, Len(v_p)) = (start, end)    SliceLen(v) = n    n ≠ end - start    Γ ⊢ LowerPanic(Bounds) ⇓ IR_k
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerWritePlaceSub(IndexAccess(p, idx), v) ⇓ SeqIR(IR_p, IR_i, IR_k)

**(LowerWriteSub-Deref)**
Γ ⊢ LowerReadPlace(p) ⇓ ⟨IR_p, v_ptr⟩
───────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerWritePlaceSub(Deref(p), v) ⇓ SeqIR(IR_p, WritePtrIR(v_ptr, v))

**(Lower-MovePlace)**
Γ ⊢ LowerReadPlace(p) ⇓ ⟨IR_p, v⟩
────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerMovePlace(p) ⇓ ⟨SeqIR(IR_p, MoveStateIR(p)), v⟩

### 6.5. Statement and Block Lowering

LowerStmtJudg = {LowerStmt, LowerStmtList, LowerBlock, LowerLoop}

**(Lower-Stmt-Correctness)**
∀ σ, Γ ⊢ ExecSigma(s, σ) ⇓ (sout, σ') ⇒ ExecIRSigma(IR, σ) ⇓ (sout, σ')
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerStmt(s) ⇓ IR

**(Lower-Block-Correctness)**
∀ σ, out, σ'. Γ ⊢ EvalBlockSigma(b, σ) ⇓ (out, σ') ⇒ (ExecIRSigma(IR, σ) ⇓ (out, σ') ∧ (out = Val(v') ⇒ v = v'))
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerBlock(b) ⇓ ⟨IR, v⟩

**(Lower-Loop-Correctness)**
∀ σ, Γ ⊢ EvalSigma(loop, σ) ⇓ (out, σ') ⇒ ExecIRSigma(IR, σ) ⇓ (out, σ')
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerLoop(loop) ⇓ ⟨IR, v⟩

StmtForms0 = {LetStmt(_), VarStmt(_), ShadowLetStmt(_, _, _), ShadowVarStmt(_, _, _), AssignStmt(_, _), CompoundAssignStmt(_, _, _), ExprStmt(_), DeferStmt(_), RegionStmt(_, _, _), FrameStmt(_, _), ReturnStmt(_), ResultStmt(_), BreakStmt(_), ContinueStmt, UnsafeBlockStmt(_), ErrorStmt(_)}
LowerStmtTotal(Γ) ⇔ ∀ s. s ∈ StmtForms0 ⇒ ∃ IR. Γ ⊢ LowerStmt(s) ⇓ IR

**(Lower-StmtList-Empty)**
──────────────────────────────────────────────
Γ ⊢ LowerStmtList([]) ⇓ ε

**(Lower-StmtList-Cons)**
Γ ⊢ LowerStmt(s) ⇓ IR_s    Γ ⊢ LowerStmtList(ss) ⇓ IR_r
─────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerStmtList(s::ss) ⇓ SeqIR(IR_s, IR_r)

BindingParts(binding) = ⟨pat, ty_opt, op, init, span⟩

**(Lower-Stmt-Let)**
BindingParts(binding) = ⟨pat, ty_opt, op, init, span⟩    Γ ⊢ LowerExpr(init) ⇓ ⟨IR_i, v⟩    Γ ⊢ LowerBindPattern(pat, v) ⇓ IR_b
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerStmt(LetStmt(binding)) ⇓ SeqIR(IR_i, IR_b)

**(Lower-Stmt-Var)**
BindingParts(binding) = ⟨pat, ty_opt, op, init, span⟩    Γ ⊢ LowerExpr(init) ⇓ ⟨IR_i, v⟩    Γ ⊢ LowerBindPattern(pat, v) ⇓ IR_b
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerStmt(VarStmt(binding)) ⇓ SeqIR(IR_i, IR_b)

**(Lower-Stmt-ShadowLet)**
Γ ⊢ LowerExpr(init) ⇓ ⟨IR_i, v⟩
──────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerStmt(ShadowLetStmt(x, ty_opt, init)) ⇓ SeqIR(IR_i, BindVarIR(x, v))

**(Lower-Stmt-ShadowVar)**
Γ ⊢ LowerExpr(init) ⇓ ⟨IR_i, v⟩
──────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerStmt(ShadowVarStmt(x, ty_opt, init)) ⇓ SeqIR(IR_i, BindVarIR(x, v))

**(Lower-Stmt-Assign)**
Γ ⊢ LowerExpr(expr) ⇓ ⟨IR_e, v⟩    Γ ⊢ LowerWritePlace(place, v) ⇓ IR_w
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerStmt(AssignStmt(place, expr)) ⇓ SeqIR(IR_e, IR_w)

**(Lower-Stmt-CompoundAssign)**
Γ ⊢ LowerReadPlace(place) ⇓ ⟨IR_p, v_p⟩    Γ ⊢ LowerExpr(expr) ⇓ ⟨IR_e, v_e⟩    BinOp(op, v_p, v_e) ⇓ v    Γ ⊢ LowerWritePlace(place, v) ⇓ IR_w
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerStmt(CompoundAssignStmt(place, op, expr)) ⇓ SeqIR(IR_p, IR_e, IR_w)


**(Lower-Stmt-Expr)**
Γ ⊢ LowerExpr(expr) ⇓ ⟨IR_e, v⟩
──────────────────────────────────────────────────────────────
Γ ⊢ LowerStmt(ExprStmt(expr)) ⇓ IR_e

**(Lower-Stmt-Defer)**
────────────────────────────────────────────────────────────
Γ ⊢ LowerStmt(DeferStmt(block)) ⇓ DeferIR(block)

**(Lower-Stmt-Region)**
opts = RegionOptsExpr(opts_opt)    Γ ⊢ LowerExpr(opts) ⇓ ⟨IR_o, v_o⟩    Γ ⊢ LowerBlock(block) ⇓ ⟨IR_b, v_b⟩
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerStmt(RegionStmt(opts_opt, alias_opt, block)) ⇓ SeqIR(IR_o, RegionIR(v_o, alias_opt, IR_b, v_b))

**(Lower-Stmt-Frame-Implicit)**
Γ ⊢ LowerBlock(block) ⇓ ⟨IR_b, v_b⟩
───────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerStmt(FrameStmt(⊥, block)) ⇓ FrameIR(⊥, IR_b, v_b)

**(Lower-Stmt-Frame-Explicit)**
Γ ⊢ LowerExpr(Identifier(r)) ⇓ ⟨IR_r, v_r⟩    Γ ⊢ LowerBlock(block) ⇓ ⟨IR_b, v_b⟩
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerStmt(FrameStmt(r, block)) ⇓ SeqIR(IR_r, FrameIR(v_r, IR_b, v_b))

**(Lower-Stmt-Return)**
Γ ⊢ LowerExpr(e) ⇓ ⟨IR_e, v⟩
──────────────────────────────────────────────────────────────────────
Γ ⊢ LowerStmt(ReturnStmt(e)) ⇓ SeqIR(IR_e, ReturnIR(v))

**(Lower-Stmt-Return-Unit)**
──────────────────────────────────────────────────────────────
Γ ⊢ LowerStmt(ReturnStmt(⊥)) ⇓ ReturnIR(())

**(Lower-Stmt-Result)**
Γ ⊢ LowerExpr(expr) ⇓ ⟨IR_e, v⟩
──────────────────────────────────────────────────────────────────────
Γ ⊢ LowerStmt(ResultStmt(expr)) ⇓ SeqIR(IR_e, ResultIR(v))

**(Lower-Stmt-Break)**
Γ ⊢ LowerExpr(e) ⇓ ⟨IR_e, v⟩
────────────────────────────────────────────────────────────────────
Γ ⊢ LowerStmt(BreakStmt(e)) ⇓ SeqIR(IR_e, BreakIR(v))

**(Lower-Stmt-Break-Unit)**
─────────────────────────────────────────────────────────────
Γ ⊢ LowerStmt(BreakStmt(⊥)) ⇓ BreakIR(⊥)

**(Lower-Stmt-Continue)**
────────────────────────────────────────────────────
Γ ⊢ LowerStmt(ContinueStmt) ⇓ ContinueIR

**(Lower-Stmt-UnsafeBlock)**
Γ ⊢ LowerBlock(block) ⇓ ⟨IR_b, v⟩
────────────────────────────────────────────────────────────────
Γ ⊢ LowerStmt(UnsafeBlockStmt(block)) ⇓ IR_b

**(Lower-Stmt-Error)**
────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerStmt(ErrorStmt(span)) ⇓ LowerPanic(ErrorStmt(span))

**Temporary Cleanup in Lowering.**

Let TempDropOrder(s) = [e_1, …, e_k]. For each i, let
Γ ⊢ LowerExpr(e_i) ⇓ ⟨IR_i, v_i⟩
denote the unique invocation of LowerExpr(e_i) in the derivation of
Γ ⊢ LowerStmt(s) ⇓ IR_s, and let ExprType(e_i) = T_i.

TempCleanupIR(s) =
 ε    if k = 0
 SeqIRList([EmitDrop(T_k, v_k), …, EmitDrop(T_1, v_1)])    otherwise

For s ∉ {ReturnStmt(_), ResultStmt(_), BreakStmt(_), ContinueStmt}, the lowering MUST produce
Γ ⊢ LowerStmt(s) ⇓ SeqIR(IR_s, TempCleanupIR(s)).

For control-flow statements, the lowering MUST emit temporary cleanup immediately before the control transfer:

Γ ⊢ LowerStmt(ReturnStmt(e)) ⇓ SeqIR(IR_e, TempCleanupIR(s), ReturnIR(v))
Γ ⊢ LowerStmt(ResultStmt(e)) ⇓ SeqIR(IR_e, TempCleanupIR(s), ResultIR(v))
Γ ⊢ LowerStmt(BreakStmt(e)) ⇓ SeqIR(IR_e, TempCleanupIR(s), BreakIR(v))
Γ ⊢ LowerStmt(BreakStmt(⊥)) ⇓ SeqIR(TempCleanupIR(s), BreakIR(⊥))
Γ ⊢ LowerStmt(ContinueStmt) ⇓ SeqIR(TempCleanupIR(s), ContinueIR)

BlockForms0 = {BlockExpr(_, _)}
LoopForms0 = {LoopInfinite(_), LoopConditional(_, _), LoopIter(_, _, _, _)}
LowerBlockTotal(Γ) ⇔ ∀ b. b ∈ BlockForms0 ⇒ ∃ IR, v. Γ ⊢ LowerBlock(b) ⇓ ⟨IR, v⟩
LowerLoopTotal(Γ) ⇔ ∀ l. l ∈ LoopForms0 ⇒ ∃ IR, v. Γ ⊢ LowerLoop(l) ⇓ ⟨IR, v⟩

**(Lower-Block-Tail)**
tail ≠ ⊥    Γ ⊢ LowerStmtList(stmts) ⇓ IR_s    Γ ⊢ LowerExpr(tail) ⇓ ⟨IR_t, v_t⟩
──────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerBlock(BlockExpr(stmts, tail)) ⇓ ⟨BlockIR(IR_s, IR_t, v_t), v_block⟩

**(Lower-Block-Unit)**
Γ ⊢ LowerStmtList(stmts) ⇓ IR_s
────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerBlock(BlockExpr(stmts, ⊥)) ⇓ ⟨BlockIR(IR_s, ε, ()), v_block⟩

**(Lower-Loop-Infinite)**
Γ ⊢ LowerBlock(body) ⇓ ⟨IR_b, v_b⟩
─────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerLoop(LoopInfinite(body)) ⇓ ⟨LoopIR(LoopInfinite, IR_b, v_b), v_loop⟩

**(Lower-Loop-Cond)**
Γ ⊢ LowerExpr(cond) ⇓ ⟨IR_c, v_c⟩    Γ ⊢ LowerBlock(body) ⇓ ⟨IR_b, v_b⟩
──────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerLoop(LoopConditional(cond, body)) ⇓ ⟨LoopIR(LoopConditional, IR_c, v_c, IR_b, v_b), v_loop⟩

**(Lower-Loop-Iter)**
Γ ⊢ LowerExpr(iter) ⇓ ⟨IR_i, v_iter⟩    Γ ⊢ LowerBlock(body) ⇓ ⟨IR_b, v_b⟩
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerLoop(LoopIter(pat, ty_opt, iter, body)) ⇓ ⟨LoopIR(LoopIter, pat, ty_opt, IR_i, v_iter, IR_b, v_b), v_loop⟩


### 6.6. Pattern Matching Lowering

PatternLowerJudg = {LowerBindPattern, LowerBindList, LowerMatch, TagOf}

**(Lower-Pat-Correctness)**
∀ v, Γ ⊢ MatchPattern(pat, v) ⇓ B ⇒ ExecIRSigma(IR, σ) ⇓ (ok, σ')
────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerBindPattern(pat, v) ⇓ IR
MatchValueCorrect(Γ, scrut, arms, v) ⇔ ∀ σ, v', σ'. Γ ⊢ EvalSigma(MatchExpr(scrut, arms), σ) ⇓ (Val(v'), σ') ⇒ v = v'

**(Lower-Match-Correctness)**
∀ σ, Γ ⊢ EvalSigma(MatchExpr(scrut, arms), σ) ⇓ (out, σ') ⇒ ExecIRSigma(IR, σ) ⇓ (out, σ')    MatchValueCorrect(Γ, scrut, arms, v)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerMatch(scrut, arms) ⇓ ⟨IR, v⟩

EnumValuePath(v) = path ⇔ v = EnumValue(path, payload)
VariantIndex(E, name) = i ⇔ Variants(E) = [v_0, …, v_k] ∧ v_i.name = name
EnumDisc(E, name) = d ⇔ EnumDiscriminants(E) ⇓ ds ∧ VariantIndex(E, name) = i ∧ ds[i] = d
StateIndex(M, S) = i ⇔ States(M) = [S_0, …, S_k] ∧ S_i = S

**(TagOf-Enum)**
EnumValuePath(v) = path    EnumPath(path) = p    T = TypePath(p)    EnumDecl(p) = E    VariantName(path) = name    EnumDisc(E, name) = d
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TagOf(v, T) ⇓ d

**(TagOf-Modal)**
v = ⟨S, v_S⟩    T = TypePath(p)    Σ.Types[p] = `modal` M    StateIndex(M, S) = i
───────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ TagOf(v, T) ⇓ i

LowerBindJudg = {LowerBindList, LowerBindPattern, LowerMatch}

**(Lower-BindList-Empty)**
──────────────────────────────────────────────
Γ ⊢ LowerBindList([]) ⇓ ε

**(Lower-BindList-Cons)**
Γ ⊢ LowerBindList(bs) ⇓ IR_r
──────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerBindList([⟨x, v⟩] ++ bs) ⇓ SeqIR(BindVarIR(x, v), IR_r)

**(Lower-Pat-General)**
Γ ⊢ MatchPattern(pat, v) ⇓ B    BindOrder(pat, B) = binds    Γ ⊢ LowerBindList(binds) ⇓ IR
────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerBindPattern(pat, v) ⇓ IR

**(Lower-Pat-Err)**
MatchPattern(pat, v) undefined
──────────────────────────────────────────────────────
Γ ⊢ LowerBindPattern(pat, v) ⇑

**(Lower-Match)**
Γ ⊢ LowerExpr(scrut) ⇓ ⟨IR_s, v_s⟩
────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerMatch(scrut, arms) ⇓ ⟨SeqIR(IR_s, MatchIR(v_s, arms)), v_match⟩


### 6.7. Globals and Initialization

GlobalsJudg = {EmitGlobal, InitFn, DeinitFn, Lower-StaticInit, Lower-StaticInitItem, Lower-StaticInitItems, InitCallIR, Lower-StaticDeinit, Lower-StaticDeinitNames, Lower-StaticDeinitItem, Lower-StaticDeinitItems, DeinitCallIR, EmitInitPlan, EmitDeinitPlan, EmitStringLit, EmitBytesLit, InitPanicHandle}

ConstInitJudg = {ConstInit}

Γ ⊢ ConstInit(e) ⇓ bytes ⇔ e = Literal(lit) ∧ Γ ⊢ EncodeConst(ExprType(e), lit) ⇓ bytes

StaticName(binding) =
 name    if binding = ⟨IdentifierPattern(name), ty_opt, op, init, span⟩
 name    if binding = ⟨TypedPattern(name, _), ty_opt, op, init, span⟩
 ⊥       otherwise

StaticBindTypes(binding) = B ⇔ binding = ⟨pat, ty_opt, op, init, _⟩ ∧ Γ ⊢ pat ⇐ BindType(binding) ⊣ B

StaticBindList(binding) = PatNames(pat) ⇔ binding = ⟨pat, _, _, _, _⟩

StaticBinding : StaticDecl × Name → StaticDecl

StaticSym(StaticDecl(_, _, binding, _, _), x) =
 Mangle(StaticDecl(_, _, binding, _, _))    if StaticName(binding) = x
 Mangle(StaticBinding(StaticDecl(_, _, binding, _, _), x))    otherwise

**(Emit-Static-Const)**
item = StaticDecl(vis, mut, binding, span, doc)    StaticName(binding) = name    binding = ⟨pat, ty_opt, op, init, _⟩    Γ ⊢ ConstInit(init) ⇓ bytes    Γ ⊢ Mangle(item) ⇓ sym
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EmitGlobal(item) ⇓ [GlobalConst(sym, bytes)]

**(Emit-Static-Init)**
item = StaticDecl(vis, mut, binding, span, doc)    StaticName(binding) = name    binding = ⟨pat, ty_opt, op, init, _⟩    Γ ⊢ ConstInit(init) ⇑    T = ExprType(init)    Γ ⊢ Mangle(item) ⇓ sym
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EmitGlobal(item) ⇓ [GlobalZero(sym, sizeof(T))]

**(Emit-Static-Multi)**
item = StaticDecl(vis, mut, binding, span, doc)    StaticName(binding) = ⊥    StaticBindTypes(binding) = B    StaticBindList(binding) = [x_1, …, x_k]    ∀ i, Γ ⊢ Mangle(StaticBinding(item, x_i)) ⇓ sym_i
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EmitGlobal(item) ⇓ [GlobalZero(sym_1, sizeof(B[x_1])), …, GlobalZero(sym_k, sizeof(B[x_k]))]

InitSym(m) = PathSig(["cursive", "runtime", "init"] ++ PathOfModule(m))

**(InitFn)**
InitSym(m) = sym
──────────────────────────────
Γ ⊢ InitFn(m) ⇓ sym

DeinitSym(m) = PathSig(["cursive", "runtime", "deinit"] ++ PathOfModule(m))

**(DeinitFn)**
DeinitSym(m) = sym
──────────────────────────────
Γ ⊢ DeinitFn(m) ⇓ sym

StaticItems(P, m) = [ item | item ∈ ASTModule(P, m).items ∧ item = StaticDecl(_, _, _, _, _) ]

StaticItemOf(path, name) = item ⇔ m = path ∧ item ∈ StaticItems(Project(Γ), m) ∧ item = StaticDecl(_, _, binding, _, _) ∧ name ∈ StaticBindList(binding) ∧ ∀ item'. (item' ∈ StaticItems(Project(Γ), m) ∧ item' = StaticDecl(_, _, binding', _, _) ∧ name ∈ StaticBindList(binding')) ⇒ item' = item

StaticSymPath(path, name) = StaticSym(item, name) ⇔ StaticItemOf(path, name) = item

StaticAddr(path, name) = addr ⇔ ∃ sym. StaticSymPath(path, name) = sym ∧ AddrOfSym(sym) = addr

AddrOfSym : Symbol → Addr

StaticType(path, name) = StaticBindTypes(binding)[name] ⇔ StaticItemOf(path, name) = StaticDecl(_, mut, binding, _, _)

StaticBindInfo(path, name) = BindInfoMap(λ U. RespOfInit(init), StaticBindTypes(binding), MovOf(op), mut)[name] ⇔ StaticItemOf(path, name) = StaticDecl(_, mut, binding, _, _) ∧ binding = ⟨_, _, op, init, _⟩

SeqIRList([]) = ε
SeqIRList([IR] ++ IRs) = SeqIR(IR, SeqIRList(IRs))

StaticStoreIR(item, []) = ε
StaticStoreIR(item, [⟨x, v⟩] ++ bs) = SeqIR(StoreGlobal(StaticSym(item, x), v), StaticStoreIR(item, bs))

**(Lower-StaticInit-Item)**
item = StaticDecl(vis, mut, binding, span, doc)    binding = ⟨pat, ty_opt, op, init, _⟩    Γ ⊢ LowerExpr(init) ⇓ ⟨IR_e, v⟩    Γ ⊢ MatchPattern(pat, v) ⇓ B    BindOrder(pat, B) = binds    Γ ⊢ InitPanicHandle(m) ⇓ IR_p
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Lower-StaticInitItem(m, item) ⇓ SeqIR(IR_e, StaticStoreIR(item, binds), IR_p)

**(Lower-StaticInitItems-Empty)**
──────────────────────────────────────────────────────────
Γ ⊢ Lower-StaticInitItems(m, []) ⇓ ε

**(Lower-StaticInitItems-Cons)**
Γ ⊢ Lower-StaticInitItem(m, item) ⇓ IR_i    Γ ⊢ Lower-StaticInitItems(m, items) ⇓ IR_r
────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Lower-StaticInitItems(m, [item] ++ items) ⇓ SeqIR(IR_i, IR_r)

**(Lower-StaticInit)**
StaticItems(Project(Γ), m) = items    Γ ⊢ Lower-StaticInitItems(m, items) ⇓ IR
──────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Lower-StaticInit(m) ⇓ IR

**(InitCallIR)**
Γ ⊢ InitFn(m) ⇓ sym
────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ InitCallIR(m) ⇓ SeqIR(CallIR(sym, [PanicOutName]), PanicCheck)

Rev([]) = []
Rev([x] ++ xs) = Rev(xs) ++ [x]

**(Lower-StaticDeinitNames-Empty)**
────────────────────────────────────────────────────────────────
Γ ⊢ Lower-StaticDeinitNames(path, item, []) ⇓ ε

**(Lower-StaticDeinitNames-Cons-Resp)**
StaticBindInfo(path, x).resp = resp    sym = StaticSym(item, x)    Γ ⊢ EmitDrop(StaticType(path, x), Load(@sym, StaticType(path, x))) ⇓ IR_d    Γ ⊢ Lower-StaticDeinitNames(path, item, xs) ⇓ IR_r
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Lower-StaticDeinitNames(path, item, [x] ++ xs) ⇓ SeqIR(IR_d, IR_r)

**(Lower-StaticDeinitNames-Cons-NoResp)**
StaticBindInfo(path, x).resp ≠ resp    Γ ⊢ Lower-StaticDeinitNames(path, item, xs) ⇓ IR_r
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Lower-StaticDeinitNames(path, item, [x] ++ xs) ⇓ IR_r

**(Lower-StaticDeinit-Item)**
item = StaticDecl(vis, mut, binding, span, doc)    binding = ⟨pat, _, _, _, _⟩    xs = Rev(StaticBindList(binding))    Γ ⊢ Lower-StaticDeinitNames(PathOfModule(m), item, xs) ⇓ IR
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Lower-StaticDeinitItem(m, item) ⇓ IR

**(Lower-StaticDeinitItems-Empty)**
────────────────────────────────────────────────────────────
Γ ⊢ Lower-StaticDeinitItems(m, []) ⇓ ε

**(Lower-StaticDeinitItems-Cons)**
Γ ⊢ Lower-StaticDeinitItem(m, item) ⇓ IR_i    Γ ⊢ Lower-StaticDeinitItems(m, items) ⇓ IR_r
────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Lower-StaticDeinitItems(m, [item] ++ items) ⇓ SeqIR(IR_i, IR_r)

**(Lower-StaticDeinit)**
StaticItems(Project(Γ), m) = items    Γ ⊢ Lower-StaticDeinitItems(m, Rev(items)) ⇓ IR
────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Lower-StaticDeinit(m) ⇓ IR

**(DeinitCallIR)**
Γ ⊢ DeinitFn(m) ⇓ sym
──────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ DeinitCallIR(m) ⇓ SeqIR(CallIR(sym, [PanicOutName]), PanicCheck)

**(EmitInitPlan)**
InitOrder = [m_1, …, m_k]    ∀ i, Γ ⊢ InitCallIR(m_i) ⇓ IR_i    IR_init = SeqIRList([IR_1, …, IR_k])
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EmitInitPlan(P) ⇓ IR_init

**(EmitInitPlan-Err)**
∃ m ∈ InitOrder. Γ ⊢ InitFn(m) ⇑
────────────────────────────────────────
Γ ⊢ EmitInitPlan(P) ⇑

**(EmitDeinitPlan)**
InitOrder = [m_1, …, m_k]    ∀ i, Γ ⊢ DeinitCallIR(m_i) ⇓ IR_i    IR_deinit = SeqIRList(Rev([IR_1, …, IR_k]))
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EmitDeinitPlan(P) ⇓ IR_deinit

**(EmitDeinitPlan-Err)**
∃ m ∈ InitOrder. Γ ⊢ DeinitFn(m) ⇑
──────────────────────────────────────────
Γ ⊢ EmitDeinitPlan(P) ⇑

### 6.8. Cleanup, Drop, and Unwinding

CleanupJudg = {EmitDrop, CleanupPlan, LowerPanic, PanicSym, ClearPanic, PanicCheck}

**(CleanupPlan)**
cs = CleanupList(scope)
────────────────────────────────────────────
Γ ⊢ CleanupPlan(scope) ⇓ cs

EmitDropSpec(Γ, T, v, IR) ⇔ ∀ σ, ExecIRSigma(IR, σ) ⇓ (out, σ') ∧ Γ ⊢ DropValue(T, v, ∅) ⇓ σ'.
Γ ⊢ EmitDrop(T, v) ⇓ IR ⇔ EmitDropSpec(Γ, T, v, IR).

PanicOutAddr(σ) = addr ⇔ LookupVal(σ, PanicOutName) = RawPtr(`mut`, addr)

PanicRecordOf(σ) = ⟨p, c⟩ ⇔ PanicOutAddr(σ) = addr ∧ ReadAddr(σ, FieldAddr(PanicRecord, addr, "panic")) = p ∧ ReadAddr(σ, FieldAddr(PanicRecord, addr, "code")) = c

WritePanicRecord(σ, p, c) ⇓ σ' ⇔ WriteAddr(σ, FieldAddr(PanicRecord, PanicOutAddr(σ), "panic"), p) ⇓ σ_1 ∧ WriteAddr(σ_1, FieldAddr(PanicRecord, PanicOutAddr(σ), "code"), c) ⇓ σ'

Γ ⊢ InitPanicHandle(m) ⇓ IR ⇔ ∀ σ. (PanicRecordOf(σ) = ⟨true, c⟩ ⇒ ∃ σ'. ExecIRSigma(IR, σ) ⇓ (Ctrl(Panic), σ') ∧ ExecIRSigma(SeqIR(SetPoison(m), LowerPanic(InitPanic(m))), σ) ⇓ (Ctrl(Panic), σ')) ∧ (PanicRecordOf(σ) = ⟨false, c⟩ ⇒ ExecIRSigma(IR, σ) ⇓ (Val(()), σ))

**(PanicSym)**
──────────────────────────────────────────────────────────
Γ ⊢ PanicSym ⇓ PathSig(["cursive", "runtime", "panic"])

PanicReason = {ErrorExpr(span), ErrorStmt(span), DivZero, Overflow, Shift, Bounds, Cast, NullDeref, ExpiredDeref, InitPanic(m), Other}.

PanicCode(ErrorExpr(_)) = 0x0001
PanicCode(ErrorStmt(_)) = 0x0002
PanicCode(DivZero) = 0x0003
PanicCode(Overflow) = 0x0004
PanicCode(Shift) = 0x0005
PanicCode(Bounds) = 0x0006
PanicCode(Cast) = 0x0007
PanicCode(NullDeref) = 0x0008
PanicCode(ExpiredDeref) = 0x0009
PanicCode(InitPanic(_)) = 0x000A
PanicCode(Other) = 0x00FF.

PanicSite = {DivZeroCheck, OverflowCheck, ShiftCheck, BoundsCheck, CastCheck, NullDerefCheck, ExpiredDerefCheck, ErrorExprSite(span), ErrorStmtSite(span), InitPanicSite(m), OtherSite}.
PanicReasonOf(DivZeroCheck) = DivZero
PanicReasonOf(OverflowCheck) = Overflow
PanicReasonOf(ShiftCheck) = Shift
PanicReasonOf(BoundsCheck) = Bounds
PanicReasonOf(CastCheck) = Cast
PanicReasonOf(NullDerefCheck) = NullDeref
PanicReasonOf(ExpiredDerefCheck) = ExpiredDeref
PanicReasonOf(ErrorExprSite(span)) = ErrorExpr(span)
PanicReasonOf(ErrorStmtSite(span)) = ErrorStmt(span)
PanicReasonOf(InitPanicSite(m)) = InitPanic(m)
PanicReasonOf(OtherSite) = Other

Γ ⊢ ClearPanic ⇓ IR ⇔ ∀ σ, ExecIRSigma(IR, σ) ⇓ (out, σ') ∧ WritePanicRecord(σ, false, 0) ⇓ σ'.

Γ ⊢ PanicCheck ⇓ IR ⇔ ∀ σ, (PanicRecordOf(σ) = ⟨true, c⟩ ⇒ ExecIRSigma(IR, σ) ⇓ (Ctrl(Panic), σ)) ∧ (PanicRecordOf(σ) = ⟨false, c⟩ ⇒ ExecIRSigma(IR, σ) ⇓ (Val(()), σ)).

Γ ⊢ LowerPanic(reason) ⇓ IR ⇔ ∀ σ. ∃ σ'. ExecIRSigma(IR, σ) ⇓ (Ctrl(Panic), σ') ∧ WritePanicRecord(σ, true, PanicCode(reason)) ⇓ σ'

### 6.9. Built-ins Runtime Interface

RuntimeIfcJudg = {RegionLayout, RegionSym, BuiltinSym}

**(RegionLayout)**
ModalLayout(`Region`) ⇓ ⟨size, align, disc, payload⟩
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ RegionLayout ⇓ ⟨size, align, [⟨`disc`, disc⟩, ⟨`payload`, payload⟩]⟩

**(RegionSym-NewScoped)**
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ RegionSym(`Region::new_scoped`) ⇓ PathSig(["cursive", "runtime", "region", "new_scoped"])

**(RegionSym-Alloc)**
──────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ RegionSym(`Region::alloc`) ⇓ PathSig(["cursive", "runtime", "region", "alloc"])

**(RegionSym-ResetUnchecked)**
────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ RegionSym(`Region::reset_unchecked`) ⇓ PathSig(["cursive", "runtime", "region", "reset_unchecked"])

**(RegionSym-Freeze)**
────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ RegionSym(`Region::freeze`) ⇓ PathSig(["cursive", "runtime", "region", "freeze"])

**(RegionSym-Thaw)**
──────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ RegionSym(`Region::thaw`) ⇓ PathSig(["cursive", "runtime", "region", "thaw"])

**(RegionSym-FreeUnchecked)**
────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ RegionSym(`Region::free_unchecked`) ⇓ PathSig(["cursive", "runtime", "region", "free_unchecked"])

**(BuiltinSym-FileSystem-OpenRead)**
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BuiltinSym(`FileSystem::open_read`) ⇓ PathSig(["cursive", "runtime", "fs", "open_read"])

**(BuiltinSym-FileSystem-OpenWrite)**
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BuiltinSym(`FileSystem::open_write`) ⇓ PathSig(["cursive", "runtime", "fs", "open_write"])

**(BuiltinSym-FileSystem-OpenAppend)**
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BuiltinSym(`FileSystem::open_append`) ⇓ PathSig(["cursive", "runtime", "fs", "open_append"])

**(BuiltinSym-FileSystem-CreateWrite)**
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BuiltinSym(`FileSystem::create_write`) ⇓ PathSig(["cursive", "runtime", "fs", "create_write"])

**(BuiltinSym-FileSystem-ReadFile)**
──────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BuiltinSym(`FileSystem::read_file`) ⇓ PathSig(["cursive", "runtime", "fs", "read_file"])

**(BuiltinSym-FileSystem-ReadBytes)**
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BuiltinSym(`FileSystem::read_bytes`) ⇓ PathSig(["cursive", "runtime", "fs", "read_bytes"])

**(BuiltinSym-FileSystem-WriteFile)**
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BuiltinSym(`FileSystem::write_file`) ⇓ PathSig(["cursive", "runtime", "fs", "write_file"])

**(BuiltinSym-FileSystem-WriteStdout)**
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BuiltinSym(`FileSystem::write_stdout`) ⇓ PathSig(["cursive", "runtime", "fs", "write_stdout"])

**(BuiltinSym-FileSystem-WriteStderr)**
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BuiltinSym(`FileSystem::write_stderr`) ⇓ PathSig(["cursive", "runtime", "fs", "write_stderr"])

**(BuiltinSym-FileSystem-Exists)**
────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BuiltinSym(`FileSystem::exists`) ⇓ PathSig(["cursive", "runtime", "fs", "exists"])

**(BuiltinSym-FileSystem-Remove)**
────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BuiltinSym(`FileSystem::remove`) ⇓ PathSig(["cursive", "runtime", "fs", "remove"])

**(BuiltinSym-FileSystem-OpenDir)**
──────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BuiltinSym(`FileSystem::open_dir`) ⇓ PathSig(["cursive", "runtime", "fs", "open_dir"])

**(BuiltinSym-FileSystem-CreateDir)**
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BuiltinSym(`FileSystem::create_dir`) ⇓ PathSig(["cursive", "runtime", "fs", "create_dir"])

**(BuiltinSym-FileSystem-EnsureDir)**
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BuiltinSym(`FileSystem::ensure_dir`) ⇓ PathSig(["cursive", "runtime", "fs", "ensure_dir"])

**(BuiltinSym-FileSystem-Kind)**
──────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BuiltinSym(`FileSystem::kind`) ⇓ PathSig(["cursive", "runtime", "fs", "kind"])

**(BuiltinSym-FileSystem-Restrict)**
──────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BuiltinSym(`FileSystem::restrict`) ⇓ PathSig(["cursive", "runtime", "fs", "restrict"])

**(BuiltinSym-HeapAllocator-WithQuota)**
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BuiltinSym(`HeapAllocator::with_quota`) ⇓ PathSig(["cursive", "runtime", "heap", "with_quota"])

**(BuiltinSym-HeapAllocator-AllocRaw)**
───────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BuiltinSym(`HeapAllocator::alloc_raw`) ⇓ PathSig(["cursive", "runtime", "heap", "alloc_raw"])

**(BuiltinSym-HeapAllocator-DeallocRaw)**
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BuiltinSym(`HeapAllocator::dealloc_raw`) ⇓ PathSig(["cursive", "runtime", "heap", "dealloc_raw"])

### 6.10. Dynamic Dispatch

DynDispatchJudg = {VTable, VSlot, DynPack, LowerDynCall}

VTableEligible(Cl) = [ m ∈ EffMethods(Cl) | vtable_eligible(m) ].

**(DispatchSym-Impl)**
LookupClassMethod(Cl, name) = m    MethodByName(T, name) = m'    Sig_T(T, m') = Sig_T(T, m)    Γ ⊢ Mangle(m') ⇓ sym
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ DispatchSym(T, Cl, name) ⇓ sym

**(DispatchSym-Default-None)**
LookupClassMethod(Cl, name) = m    MethodByName(T, name) = ⊥    m.body_opt ≠ ⊥    Γ ⊢ Mangle(DefaultImpl(T, m)) ⇓ sym
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ DispatchSym(T, Cl, name) ⇓ sym

**(DispatchSym-Default-Mismatch)**
LookupClassMethod(Cl, name) = m    MethodByName(T, name) = m'    Sig_T(T, m') ≠ Sig_T(T, m)    m.body_opt ≠ ⊥    Γ ⊢ Mangle(DefaultImpl(T, m)) ⇓ sym
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ DispatchSym(T, Cl, name) ⇓ sym

**(VTable-Order)**
VTableEligible(Cl) = [m_1, …, m_k]    ∀ i, DispatchSym(T, Cl, m_i.name) = sym_i
─────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ VTable(T, Cl) ⇓ [sym_1, …, sym_k]

**(VSlot-Entry)**
VTableEligible(Cl) = [m_0, …, m_{k-1}]    m_i.name = method.name
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ VSlot(Cl, method) ⇓ i

**(Lower-Dynamic-Form)**
IsPlace(e)    Γ ⊢ LowerAddrOf(e) ⇓ ⟨IR, addr⟩    T_e = ExprType(e)    T = StripPerm(T_e)    Γ ⊢ T <: Cl
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ DynPack(T, e) ⇓ ⟨RawPtr(`imm`, addr), VTable(T, Cl)⟩

**(Lower-DynCall)**
VSlot(Cl, name) ⇓ i
────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerDynCall(base, name, args) ⇓ SeqIR(CallVTable(base, i, args), PanicCheck)

### 6.11. Checks and Panic

ChecksJudg = {LowerRangeExpr, CheckIndex, CheckRange, LowerTransmute, LowerRawDeref}

**(Lower-Range-Full)**
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerRangeExpr(Range(`Full`, ⊥, ⊥)) ⇓ ⟨ε, RangeVal(`Full`, ⊥, ⊥)⟩
  
**(Lower-Range-To)**
Γ ⊢ LowerExpr(e) ⇓ ⟨IR_e, v⟩
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerRangeExpr(Range(`To`, ⊥, e)) ⇓ ⟨IR_e, RangeVal(`To`, ⊥, v)⟩

**(Lower-Range-ToInclusive)**
Γ ⊢ LowerExpr(e) ⇓ ⟨IR_e, v⟩
────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerRangeExpr(Range(`ToInclusive`, ⊥, e)) ⇓ ⟨IR_e, RangeVal(`ToInclusive`, ⊥, v)⟩
  
**(Lower-Range-From)**
Γ ⊢ LowerExpr(e) ⇓ ⟨IR_e, v⟩
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerRangeExpr(Range(`From`, e, ⊥)) ⇓ ⟨IR_e, RangeVal(`From`, v, ⊥)⟩
  
**(Lower-Range-Inclusive)**
Γ ⊢ LowerExpr(e_1) ⇓ ⟨IR_1, v_1⟩    Γ ⊢ LowerExpr(e_2) ⇓ ⟨IR_2, v_2⟩
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerRangeExpr(Range(`Inclusive`, e_1, e_2)) ⇓ ⟨SeqIR(IR_1, IR_2), RangeVal(`Inclusive`, v_1, v_2)⟩
  
**(Lower-Range-Exclusive)**
Γ ⊢ LowerExpr(e_1) ⇓ ⟨IR_1, v_1⟩    Γ ⊢ LowerExpr(e_2) ⇓ ⟨IR_2, v_2⟩
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerRangeExpr(Range(`Exclusive`, e_1, e_2)) ⇓ ⟨SeqIR(IR_1, IR_2), RangeVal(`Exclusive`, v_1, v_2)⟩

**(Check-Index-Ok)**
IndexNum(v_i) = i    0 ≤ i < L
─────────────────────────────────────────────
Γ ⊢ CheckIndex(L, v_i) ⇓ ok

**(Check-Index-Err)**
IndexNum(v_i) = i    ¬(0 ≤ i < L)
────────────────────────────────────────────
Γ ⊢ CheckIndex(L, v_i) ⇑

**(Check-Range-Ok)**
SliceBounds(r, L) defined
───────────────────────────────────────────
Γ ⊢ CheckRange(L, r) ⇓ ok

**(Check-Range-Err)**
SliceBounds(r, L) undefined
────────────────────────────────────────────
Γ ⊢ CheckRange(L, r) ⇑

**(Lower-Transmute)**
Γ ⊢ LowerExpr(e) ⇓ ⟨IR_e, v⟩    TransmuteVal(T_1, T_2, v) ⇓ v'
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerTransmute(T_1, T_2, e) ⇓ ⟨IR_e, v'⟩

**(Lower-Transmute-Err)**
Γ ⊢ LowerExpr(e) ⇓ ⟨IR_e, v⟩    TransmuteVal(T_1, T_2, v) undefined
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerTransmute(T_1, T_2, e) ⇑

**Raw Dereference Lowering.**

**(Lower-RawDeref-Safe)**
PtrType(v_ptr) = TypePtr(T, `Valid`)
────────────────────────────────────────────────────────────
Γ ⊢ LowerRawDeref(v_ptr) ⇓ ⟨ReadPtrIR(v_ptr), v⟩

**(Lower-RawDeref-Raw)**
PtrType(v_ptr) = TypeRawPtr(q, T)
────────────────────────────────────────────────────────────
Γ ⊢ LowerRawDeref(v_ptr) ⇓ ⟨ReadPtrIR(v_ptr), v⟩

**(Lower-RawDeref-Null)**
PtrType(v_ptr) = TypePtr(T, `Null`)    Γ ⊢ LowerPanic(NullDeref) ⇓ IR_k
────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerRawDeref(v_ptr) ⇓ ⟨IR_k, v_unreach⟩

**(Lower-RawDeref-Expired)**
PtrType(v_ptr) = TypePtr(T, `Expired`)    Γ ⊢ LowerPanic(ExpiredDeref) ⇓ IR_k
────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerRawDeref(v_ptr) ⇓ ⟨IR_k, v_unreach⟩

### 6.12. LLVM 21 Backend Requirements

#### 6.12.1. LLVM Module Header

LLVMHeader = [TargetDataLayout(LLVMDataLayout), TargetTriple(LLVMTriple)]

#### 6.12.2. Opaque Pointer Model (LLVM 21)

AddrSpace(T) =
 0                if T = TypePtr(U, s)
 0                if T = TypeRawPtr(q, U)
 0                if T = TypeFunc(params, R)
 AddrSpace(U)     if T = TypePerm(p, U) ∧ AddrSpace(U) defined
 ⊥                otherwise

LLVMPtrTy(T) = `ptr addrspace(AddrSpace(T))` when AddrSpace(T) defined

#### 6.12.3. LLVM Attribute Mapping (Permissions and Pointer State)

LLVMAttrJudg = {PtrStateOf(T) = s, LLVMPtrAttrs(T) ⇓ A, LLVMArgAttrs(T) ⇓ A}

**(PtrStateOf-Perm)**
PtrStateOf(T) = s
────────────────────────────────────────────
PtrStateOf(TypePerm(p, T)) = s

**(LLVM-PtrAttrs-Valid)**
StripPerm(T) = TypePtr(U, `Valid`)
───────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LLVMPtrAttrs(T) ⇓ {`nonnull`, `dereferenceable`(sizeof(U)), `align`(alignof(U)), `noundef`}

**(LLVM-PtrAttrs-Other)**
StripPerm(T) = TypePtr(U, s)    s ∈ {⊥, `Null`, `Expired`}
────────────────────────────────────────────────────────────────────────
Γ ⊢ LLVMPtrAttrs(T) ⇓ ∅

**(LLVM-PtrAttrs-RawPtr)**
StripPerm(T) = TypeRawPtr(q, U)
────────────────────────────────────────────────
Γ ⊢ LLVMPtrAttrs(T) ⇓ ∅

**(LLVM-ArgAttrs-Ptr)**
LLVMArgAttrsPtr(T) = (PermOf(T) = `unique` Sigma {`noalias`} : ∅) ∪ (PermOf(T) = `const` Sigma {`readonly`} : ∅)
StripPerm(T) ∈ {TypePtr(_, _), TypeFunc(_, _)}
──────────────────────────────────────────────────────────────
Γ ⊢ LLVMArgAttrs(T) ⇓ LLVMArgAttrsPtr(T)

**(LLVM-ArgAttrs-RawPtr)**
StripPerm(T) = TypeRawPtr(_, _)
──────────────────────────────────────────────
Γ ⊢ LLVMArgAttrs(T) ⇓ ∅

**(LLVM-ArgAttrs-NonPtr)**
StripPerm(T) ∉ {TypePtr(_, _), TypeRawPtr(_, _), TypeFunc(_, _)}
──────────────────────────────────────────────────────────────────────
Γ ⊢ LLVMArgAttrs(T) ⇓ ∅

NoEscapeParam(x) predicate
NoEscapeParam(x) ⇔ false
OptArgAttrs(x) ⊆ {`nocapture`} ∧ (`nocapture` ∈ OptArgAttrs(x) ⇒ NoEscapeParam(x))
LLVMArgAttrsExt(x, T) = LLVMArgAttrs(T) ∪ OptArgAttrs(x)

#### 6.12.4. UB and Poison Avoidance (LLVM 21)

LLVMInstrs(LLVMIR) = [i_0, …, i_n]
Opcode(i) = op
UsesOpcode(LLVMIR, op) ⇔ ∃ i ∈ LLVMInstrs(LLVMIR). Opcode(i) = op
Intrinsic(i) = name
UsesIntrinsic(LLVMIR, name) ⇔ ∃ i ∈ LLVMInstrs(LLVMIR). Intrinsic(i) = name
NoUndefPoison(LLVMIR) ⇔ ¬ UsesOpcode(LLVMIR, `undef`) ∧ ¬ UsesOpcode(LLVMIR, `poison`)
NoNSWNUW(LLVMIR) ⇔ ¬ UsesOpcode(LLVMIR, `nsw`) ∧ ¬ UsesOpcode(LLVMIR, `nuw`)
CheckedOverflow(LLVMIR) ⇔ ¬ UsesOpcode(LLVMIR, `add`) ∧ ¬ UsesOpcode(LLVMIR, `sub`) ∧ ¬ UsesOpcode(LLVMIR, `mul`) ∧ UsesIntrinsic(LLVMIR, `llvm.*.with.overflow`)
CheckedDivRem(LLVMIR) ⇔ UsesIntrinsic(LLVMIR, `llvm.sdiv.with.check`) ∧ UsesIntrinsic(LLVMIR, `llvm.udiv.with.check`)
CheckedShifts(LLVMIR) ⇔ UsesIntrinsic(LLVMIR, `llvm.shift.with.check`)
FrozenPoisonUses(LLVMIR) ⇔ UsesOpcode(LLVMIR, `freeze`)
InboundsGEP(LLVMIR) ⇔ ¬ UsesOpcode(LLVMIR, `getelementptr inbounds`) ∨ UsesOpcode(LLVMIR, `gep.inbounds.checked`)
LLVMUBSafe(LLVMIR) ⇔ NoUndefPoison(LLVMIR) ∧ CheckedOverflow(LLVMIR) ∧ CheckedDivRem(LLVMIR) ∧ CheckedShifts(LLVMIR) ∧ FrozenPoisonUses(LLVMIR) ∧ InboundsGEP(LLVMIR) ∧ NoNSWNUW(LLVMIR)

#### 6.12.5. Memory Intrinsics

Memmove(dst, src, n) = [`call` `llvm.memmove`(dst, src, n)]
MemcpyOverlapUnknown(dst, src, n) predicate
MemcpyOverlapUnknown(dst, src, n) ⇔ true
MemcpyAllowed(dst, src, n) ⇔ ¬ MemcpyOverlapUnknown(dst, src, n)
AggMemcpy(dst, src, n) =
 Memcpy(dst, src, n)     if MemcpyAllowed(dst, src, n)
 Memmove(dst, src, n)    otherwise
AggZero(dst, n) = Memset(dst, 0, n)
LifetimeOpt(T) ⊆ {`llvm.lifetime.start`(sizeof(T)), `llvm.lifetime.end`(sizeof(T))}

#### 6.12.6. Runtime and Builtâ€‘in Declarations

RuntimeDeclJudg = {RuntimeSig(sym) ⇓ ⟨params, ret⟩, BuiltinSig(method) ⇓ ⟨params, ret⟩, RuntimeDecls(S) ⇓ decls}

BuiltinSig(`FileSystem`::name) = ⟨[⟨⊥, `self`, TypePerm(CapRecv(`FileSystem`, name), TypeDynamic(`FileSystem`))⟩] ++ params, ret⟩ ⇔ CapMethodSig(`FileSystem`, name) = ⟨params, ret⟩
BuiltinSig(`HeapAllocator`::name) = ⟨[⟨⊥, `self`, TypePerm(CapRecv(`HeapAllocator`, name), TypeDynamic(`HeapAllocator`))⟩] ++ params, ret⟩ ⇔ CapMethodSig(`HeapAllocator`, name) = ⟨params, ret⟩
BuiltinSig(method) = ⟨params, ret⟩ ⇔ StringBytesBuiltinSig(method) = ⟨params, ret⟩

RuntimeSig(PanicSym) = ⟨[⟨⊥, `code`, TypePrim("u32")⟩], TypePrim("!")⟩
RuntimeSig(ContextInitSym) = ⟨[], TypePath(["Context"])⟩
RuntimeSig(StringDropSym) = ⟨[⟨`move`, `value`, TypeString(`@Managed`)⟩], TypePrim("()")⟩
RuntimeSig(BytesDropSym) = ⟨[⟨`move`, `value`, TypeBytes(`@Managed`)⟩], TypePrim("()")⟩
RuntimeSig(sym) = ⟨params, ret⟩ ⇔ sym = RegionSym(proc) ∧ RegionProcSig(proc) = ⟨params, ret⟩
RuntimeSig(sym) = ⟨params, ret⟩ ⇔ sym = BuiltinSym(method) ∧ BuiltinSig(method) = ⟨params, ret⟩

LLVMDecl : Symbol × Sig → LLVMDecl

**(RuntimeDecls)**
∀ sym ∈ S, RuntimeSig(sym) = ⟨params, ret⟩    LLVMCallSig(params, ret) ⇓ sig
─────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ RuntimeDecls(S) ⇓ [LLVMDecl(sym, sig) | sym ∈ S]
DeclAttrs : Symbol → AttrSet
DeclSyms(LLVMIR) = { sym | LLVMDecl(sym, _) ∈ LLVMIR ∨ LLVMDefine(sym, _, _) ∈ LLVMIR }
DeclAttrsOk(sym) ⇔ (sym = PanicSym ⇒ {`noreturn`, `nounwind`} ⊆ DeclAttrs(sym)) ∧ (sym ≠ PanicSym ⇒ `nounwind` ∈ DeclAttrs(sym))
RuntimeDeclsOk(decls) ⇔ ∀ sym ∈ DeclSyms(decls). DeclAttrsOk(sym)
RuntimeDeclsCover(LLVMIR, IR) ⇔ RuntimeRefs(IR) ⊆ DeclSyms(LLVMIR)

#### 6.12.7. LLVM Toolchain Version

LLVMToolchain = "21.1.8"

#### 6.12.8. LLVM Type Mapping

LLVMTyJudg = {LLVMTy(T) ⇓ τ}

LLVMZST = {}
Pad(n) =
 []        if n = 0
 [n × i8]  if n ≠ 0

LLVMPrim(name) =
 i8        if name ∈ {i8, u8}
 i16       if name ∈ {i16, u16}
 i32       if name ∈ {i32, u32}
 i64       if name ∈ {i64, u64}
 i128      if name ∈ {i128, u128}
 `half`    if name = f16
 `float`   if name = f32
 `double`  if name = f64
 i8        if name = `bool`
 i32       if name = `char`
 i64       if name ∈ {`usize`, `isize`}
 LLVMZST   if name ∈ {`()`, `!`}
 ⊥         otherwise

LLVMStruct([t_1, …, t_k]) = { t_1, …, t_k }
LLVMArray(n, t) = [n × t]
LLVMArrayConst(n, t, elems) constructor
SlicePtrTy(T) = LLVMPtrTy(TypeRawPtr(`imm`, T))

StructElems([], [], 0) = []
StructElems([⟨f_1, T_1⟩, …, ⟨f_n, T_n⟩], [o_1, …, o_n], size) = Pad(pad_1) ++ [τ_1] ++ … ++ Pad(pad_n) ++ [τ_n] ++ Pad(pad_tail)
pad_1 = o_1
pad_i = o_i - (o_{i-1} + sizeof(T_{i-1}))    for i = 2..n
pad_tail = size - (o_n + sizeof(T_n))
τ_i = LLVMTy(T_i)

TaggedElems(disc, payload_size, payload_align, size) = [LLVMTy(disc)] ++ Pad(pad_mid) ++ [LLVMArray(payload_size, i8)] ++ Pad(pad_tail)
disc_size = sizeof(disc)
payload_off = AlignUp(disc_size, payload_align)
pad_mid = payload_off - disc_size
pad_tail = size - (payload_off + payload_size)

**(LLVMTy-Prim)**
T = TypePrim(name)    LLVMPrim(name) = τ
────────────────────────────────────────────
Γ ⊢ LLVMTy(T) ⇓ τ

**(LLVMTy-Perm)**
Γ ⊢ LLVMTy(T) ⇓ τ
───────────────────────────────────────────────
Γ ⊢ LLVMTy(TypePerm(p, T)) ⇓ τ

**(LLVMTy-Ptr)**
T = TypePtr(U, s)    LLVMPtrTy(T) = τ
──────────────────────────────────────────────
Γ ⊢ LLVMTy(T) ⇓ τ

**(LLVMTy-RawPtr)**
T = TypeRawPtr(q, U)    LLVMPtrTy(T) = τ
──────────────────────────────────────────────
Γ ⊢ LLVMTy(T) ⇓ τ

**(LLVMTy-Func)**
T = TypeFunc(params, R)    LLVMPtrTy(T) = τ
──────────────────────────────────────────────
Γ ⊢ LLVMTy(T) ⇓ τ

**(LLVMTy-Alias)**
T = TypePath(p)    AliasBody(p) = ty    Γ ⊢ LLVMTy(ty) ⇓ τ
──────────────────────────────────────────────────────────────
Γ ⊢ LLVMTy(T) ⇓ τ

**(LLVMTy-Record)**
T = TypePath(p)    RecordDecl(p) = R    Fields(R) = fields    RecordLayout(fields) ⇓ ⟨size, _, offsets⟩    StructElems(fields, offsets, size) = elems
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LLVMTy(T) ⇓ LLVMStruct(elems)

**(LLVMTy-Tuple)**
TupleLayout([T_1, …, T_n]) ⇓ ⟨size, _, offsets⟩    StructElems([⟨0, T_1⟩, …, ⟨n-1, T_n⟩], offsets, size) = elems
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LLVMTy(TypeTuple([T_1, …, T_n])) ⇓ LLVMStruct(elems)

**(LLVMTy-Array)**
T = TypeArray(T_0, e)    Γ ⊢ ConstLen(e) ⇓ n    Γ ⊢ LLVMTy(T_0) ⇓ τ
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ LLVMTy(T) ⇓ LLVMArray(n, τ)

**(LLVMTy-Slice)**
T = TypeSlice(T_0)    Γ ⊢ LLVMTy(TypePrim("usize")) ⇓ τ_u
────────────────────────────────────────────────────────────────────────
Γ ⊢ LLVMTy(T) ⇓ LLVMStruct([SlicePtrTy(T_0), τ_u])

**(LLVMTy-Range)**
Γ ⊢ RangeLayout() ⇓ ⟨size, _, offsets⟩    StructElems(RangeFields, offsets, size) = elems
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LLVMTy(TypeRange) ⇓ LLVMStruct(elems)

**(LLVMTy-Enum)**
T = TypePath(p)    EnumDecl(p) = E    EnumLayout(E) ⇓ ⟨size, _, disc, payload_size⟩    payload_align = PayloadAlign(E)    TaggedElems(disc, payload_size, payload_align, size) = elems
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LLVMTy(T) ⇓ LLVMStruct(elems)

**(LLVMTy-Union-Niche)**
T = TypeUnion([T_1, …, T_n])    NicheApplies(T)    PayloadMember(T) = T_p    Γ ⊢ LLVMTy(T_p) ⇓ τ
────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LLVMTy(T) ⇓ τ

**(LLVMTy-Union-Tagged)**
T = TypeUnion([T_1, …, T_n])    UnionLayout(T) ⇓ ⟨size, _, disc, payload_size⟩    disc ≠ ⊥    payload_align = PayloadAlign(T)    TaggedElems(disc, payload_size, payload_align, size) = elems
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LLVMTy(T) ⇓ LLVMStruct(elems)

**(LLVMTy-Modal-Niche)**
T = TypePath(p)    Σ.Types[p] = `modal` M    NicheApplies(M)    PayloadState(M) = S_p    SingleFieldPayload(M, S_p) = T_p    Γ ⊢ LLVMTy(T_p) ⇓ τ
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LLVMTy(T) ⇓ τ

**(LLVMTy-Modal-Tagged)**
T = TypePath(p)    Σ.Types[p] = `modal` M    ModalLayout(M) ⇓ ⟨size, _, disc, payload_size⟩    disc ≠ ⊥    payload_align = max_{S ∈ States(M)}(StateAlign(M, S))    TaggedElems(disc, payload_size, payload_align, size) = elems
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LLVMTy(T) ⇓ LLVMStruct(elems)

**(LLVMTy-Modal-StringBytes)**
BaseModal(TypeString(⊥)) = `string`
BaseModal(TypeBytes(⊥)) = `bytes`
T ∈ {TypeString(⊥), TypeBytes(⊥)}    ModalLayout(BaseModal(T)) ⇓ ⟨size, _, disc, payload_size⟩    (disc = ⊥ ⇒ PayloadState(BaseModal(T)) = S_p ∧ SingleFieldPayload(BaseModal(T), S_p) = T_p ∧ Γ ⊢ LLVMTy(T_p) ⇓ τ)    (disc ≠ ⊥ ⇒ payload_align = max_{S ∈ States(BaseModal(T))}(StateAlign(BaseModal(T), S)) ∧ TaggedElems(disc, payload_size, payload_align, size) = elems)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LLVMTy(T) ⇓ (τ if disc = ⊥ else LLVMStruct(elems))

**(LLVMTy-ModalState)**
T = TypeModalState(p, S)    Σ.Types[p] = `modal` M    S ∈ States(M)    Payload(M, S) = fields    RecordLayout(fields) ⇓ ⟨size, _, offsets⟩    StructElems(fields, offsets, size) = elems
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LLVMTy(T) ⇓ LLVMStruct(elems)

**(LLVMTy-Dynamic)**
DynLayout(Cl) ⇓ ⟨_, _, [⟨`data`, T_d⟩, ⟨`vtable`, T_v⟩]⟩    Γ ⊢ LLVMTy(T_d) ⇓ τ_d    Γ ⊢ LLVMTy(T_v) ⇓ τ_v
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LLVMTy(TypeDynamic(Cl)) ⇓ LLVMStruct([τ_d, τ_v])

**(LLVMTy-StringView)**
T = TypeString(`@View`)    Γ ⊢ LLVMTy(TypePrim("usize")) ⇓ τ_u
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LLVMTy(T) ⇓ LLVMStruct([LLVMPtrTy(TypePtr(TypePerm(`const`, TypePrim("u8")), `Valid`)), τ_u])

**(LLVMTy-StringManaged)**
T = TypeString(`@Managed`)    Γ ⊢ LLVMTy(TypePrim("usize")) ⇓ τ_u
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LLVMTy(T) ⇓ LLVMStruct([LLVMPtrTy(TypePtr(TypePrim("u8"), `Valid`)), τ_u, τ_u])

**(LLVMTy-BytesView)**
T = TypeBytes(`@View`)    Γ ⊢ LLVMTy(TypePrim("usize")) ⇓ τ_u
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LLVMTy(T) ⇓ LLVMStruct([LLVMPtrTy(TypePtr(TypePerm(`const`, TypePrim("u8")), `Valid`)), τ_u])

**(LLVMTy-BytesManaged)**
T = TypeBytes(`@Managed`)    Γ ⊢ LLVMTy(TypePrim("usize")) ⇓ τ_u
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LLVMTy(T) ⇓ LLVMStruct([LLVMPtrTy(TypePtr(TypePrim("u8"), `Valid`)), τ_u, τ_u])

**(LLVMTy-Err)**
LLVMTy(T) undefined
──────────────────────────────
Γ ⊢ LLVMTy(T) ⇑

#### 6.12.9. LLVM IR Emission Pipeline

LLVMEmitJudg = {LowerIR(ModuleIR) ⇓ LLVMIR, EmitLLVM(LLVMIR) ⇓ bytes, EmitObj(LLVMIR) ⇓ bytes}

RuntimeSyms = {PanicSym, StringDropSym, BytesDropSym, ContextInitSym} ∪ {RegionSym(proc) | proc ∈ RegionProcs} ∪ {BuiltinSym(method) | method ∈ BuiltinMethods}
BuiltinMethods = StringBuiltins ∪ BytesBuiltins ∪ {`FileSystem`::name | ⟨name, recv, params, ret⟩ ∈ FileSystemInterface} ∪ {`HeapAllocator`::name | ⟨name, recv, params, ret⟩ ∈ HeapAllocatorInterface}
RefSyms : IR → 𝒫(Symbol)
RefSyms([]) = ∅
RefSyms([d] ++ ds) = RefSyms(d) ∪ RefSyms(ds)
RefSyms(ProcIR(_, _, _, IR)) = RefSyms(IR)
RefSyms(GlobalConst(_, _)) = ∅
RefSyms(GlobalZero(_, _)) = ∅
RefSyms(GlobalVTable(_, header, slots)) = { s | s ∈ header ∧ s ∈ Symbol } ∪ { s | s ∈ slots ∧ s ∈ Symbol }
RefSyms(EmitVTable(T, Cl)) = RefSyms(d) ⇔ Γ ⊢ EmitVTable(T, Cl) ⇓ d
RefSyms(EmitDropGlue(T)) = RefSyms(IR) ⇔ Γ ⊢ DropGlueIR(T) ⇓ IR
RefSyms(EmitLiteralData(_, _)) = ∅
RefSyms(ε) = ∅
RefSyms(SeqIR(IR_1, IR_2)) = RefSyms(IR_1) ∪ RefSyms(IR_2)
RefSyms(ReadVarIR(_)) = ∅
RefSyms(StoreVarIR(_, _)) = ∅
RefSyms(StoreVarNoDropIR(_, _)) = ∅
RefSyms(BindVarIR(_, _)) = ∅
RefSyms(ReadPtrIR(_)) = ∅
RefSyms(WritePtrIR(_, _)) = ∅
RefSyms(AllocIR(_, _)) = ∅
RefSyms(MoveStateIR(_)) = ∅
RefSyms(ReturnIR(_)) = ∅
RefSyms(ResultIR(_)) = ∅
RefSyms(BreakIR(_)) = ∅
RefSyms(ContinueIR) = ∅
RefSyms(DeferIR(_)) = ∅
RefSyms(ReadPathIR(path, name)) = {PathSym(path, name)} ∪ { sym | StaticSymPath(path, name) = sym }
RefSyms(StoreGlobal(sym, _)) = {sym}
RefSyms(CallIR(callee, _)) = { callee | callee ∈ Symbol }
RefSyms(IfIR(_, IR_t, _, IR_f, _)) = RefSyms(IR_t) ∪ RefSyms(IR_f)
RefSyms(BlockIR(IR_s, IR_t, _)) = RefSyms(IR_s) ∪ RefSyms(IR_t)
RefSyms(MatchIR(_, _)) = ∅
RefSyms(LoopIR(LoopInfinite, IR_b, _)) = RefSyms(IR_b)
RefSyms(LoopIR(LoopConditional, IR_c, _, IR_b, _)) = RefSyms(IR_c) ∪ RefSyms(IR_b)
RefSyms(LoopIR(LoopIter, _, _, IR_i, _, IR_b, _)) = RefSyms(IR_i) ∪ RefSyms(IR_b)
RefSyms(RegionIR(_, _, IR_b, _)) = RefSyms(IR_b)
RefSyms(FrameIR(_, IR_b, _)) = RefSyms(IR_b)
RefSyms(BranchIR(_)) = ∅
RefSyms(BranchIR(_, _, _)) = ∅
RefSyms(PhiIR(_, _, _)) = ∅
RefSyms(CallVTable(_, _, _)) = ∅
RefSyms(AddrOfIR(p)) = RefSyms(IR_p) ⇔ Γ ⊢ LowerAddrOf(p) ⇓ ⟨IR_p, addr⟩
RefSyms(ClearPanic) = RefSyms(IR) ⇔ Γ ⊢ ClearPanic ⇓ IR
RefSyms(PanicCheck) = RefSyms(IR) ⇔ Γ ⊢ PanicCheck ⇓ IR
RefSyms(CheckPoison(m)) = RefSyms(IR) ⇔ Γ ⊢ CheckPoison(m) ⇓ IR
RefSyms(LowerPanic(r)) = RefSyms(IR) ⇔ Γ ⊢ LowerPanic(r) ⇓ IR
RuntimeRefs(IR) = RefSyms(IR) ∩ RuntimeSyms
LiteralRef(IR, kind, bytes) predicate
LiteralRef(IR, kind, bytes) ⇔ LiteralDataSym(kind, bytes) ∈ RefSyms(IR)
LiteralRefs(IR) = {⟨kind, bytes⟩ | LiteralRef(IR, kind, bytes)}
VTableRefs(IR) = {(T, Cl) | DynPack(T, _) ∈ IR ∨ CallVTable(_, _, _) ∈ IR}

ExpandIR(IR) = IR ++ ((++_{(T, Cl) ∈ VTableRefs(IR)} [EmitDropGlue(T), EmitVTable(T, Cl)]) ) ++ ((++_{⟨kind, bytes⟩ ∈ LiteralRefs(IR)} [EmitLiteralData(kind, bytes)]) )

EmitKey(d) =
 ⟨`vtable`, T, Cl⟩    if d = EmitVTable(T, Cl)
 ⟨`drop`, T⟩          if d = EmitDropGlue(T)
 ⟨`lit`, kind, bytes⟩  if d = EmitLiteralData(kind, bytes)
 ⊥                    otherwise
EmitKeys(IR) = [EmitKey(d) | d ∈ IR ∧ EmitKey(d) ≠ ⊥]
UniqueEmits(IR) ⇔ NoDup(EmitKeys(IR))

**(LowerIR-Module)**
IR' = ExpandIR(IR)    IR' = [d_1, …, d_k]    ∀ i, Γ ⊢ LowerIRDecl(d_i) ⇓ ll_i    RuntimeDecls(RuntimeRefs(IR')) = ds    RuntimeDeclsOk(ds)    LLVMIR = LLVMHeader ++ ds ++ ll_1 ++ … ++ ll_k    LLVMUBSafe(LLVMIR)    RuntimeDeclsCover(LLVMIR, IR')    UniqueEmits(IR')
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIR(IR) ⇓ LLVMIR

**(LowerIR-Err)**
∃ i, Γ ⊢ LowerIRDecl(d_i) ⇑
────────────────────────────────
Γ ⊢ LowerIR(IR) ⇑

**(EmitLLVM-Ok)**
RenderLLVM(LLVMIR) = bytes
────────────────────────────────────────────
Γ ⊢ EmitLLVM(LLVMIR) ⇓ bytes

LLVMText_21 = { bytes | `llvm-as`_21 accepts bytes }
RenderLLVM(LLVMIR) = bytes ⇒ bytes ∈ LLVMText_21

**(EmitLLVM-Err)**
RenderLLVM(LLVMIR) ⇑
────────────────────────────────
Γ ⊢ EmitLLVM(LLVMIR) ⇑

**(EmitObj-Ok)**
LLVMEmitObj_21(LLVMIR) = bytes
───────────────────────────────────────────
Γ ⊢ EmitObj(LLVMIR) ⇓ bytes
LLVMEmitObj_21(LLVMIR) = bytes ⇔ LLVMObj_21(LLVMIR, LLVMHeader) = bytes

**(EmitObj-Err)**
LLVMEmitObj_21(LLVMIR) ⇑
──────────────────────────────
Γ ⊢ EmitObj(LLVMIR) ⇑

#### 6.12.10. IR Operation Lowering

LowerIRJudg = {LowerIRDecl(d) ⇓ ll, LowerIRInstr(op) ⇓ ll}

LLVMInstrList = [LLVMInstr]
Label(l) ∈ LLVMInstr
Br(l) ∈ LLVMInstr
BrCond(v, l_t, l_f) ∈ LLVMInstr
Phi(τ, inc, v) ∈ LLVMInstr
HasLabel(I, l) ⇔ Label(l) ∈ I
HasBrCond(I, v) ⇔ ∃ l_t, l_f. BrCond(v, l_t, l_f) ∈ I
HasPhi(I, v) ⇔ ∃ τ, inc. Phi(τ, inc, v) ∈ I
FreshLabel(Γ) predicate
FreshSSA(Γ) predicate
LLVMSSA = Name
LLVMLabel = Name
FreshLabel(Γ) ∈ LLVMLabel \ dom(Γ)
FreshSSA(Γ) ∈ LLVMSSA \ dom(Γ)

IfLabels(Γ) = ⟨l_t, l_f, l_m⟩ ∧ Distinct([l_t, l_f, l_m])

LLResult = {⟨I, v⟩ | I ∈ LLVMInstrList ∧ v ∈ LLVMSSA ∪ {⊥}}
SeqLL(⟨I_1, v_1⟩, ⟨I_2, v_2⟩) = ⟨I_1 ++ I_2, v_2⟩

**(LowerIRInstr-Empty)**
──────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(ε) ⇓ ⟨[], ⊥⟩

**(LowerIRInstr-Seq)**
Γ ⊢ LowerIRInstr(IR_1) ⇓ ll_1    Γ ⊢ LowerIRInstr(IR_2) ⇓ ll_2
──────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(SeqIR(IR_1, IR_2)) ⇓ SeqLL(ll_1, ll_2)

Load(slot, T) = [`load` LLVMTy(T), slot : LLVMPtrTy(T)]
Store(slot, v, T) = [`store` LLVMTy(T) v, slot : LLVMPtrTy(T)]
Memcpy(dst, src, n) = [`call` `llvm.memcpy`(dst, src, n)]
Memset(dst, 0, n) = [`call` `llvm.memset`(dst, 0, n)]
LoadVal(slot, T) ⇓ ⟨Load(slot, T), v⟩

LEValue(bytes) = ∑_{i=0}^{|bytes|-1} bytes[i] · 256^i
ByteInt(bytes) = i{8|bytes|} LEValue(bytes)

AllZero(bytes) ⇔ ∀ b ∈ bytes. b = 0x00
ByteArray(bytes) = LLVMArrayConst(|bytes|, i8, bytes)
ConstBytes(τ, bytes) = c ⇔ ∃ T. Γ ⊢ LLVMTy(T) ⇓ τ ∧ |bytes| = sizeof(T) ∧ c = ConstBytesCase(τ, bytes)
ConstBytesCase(τ, bytes) =
 `zeroinitializer`    if |bytes| = 0
 ByteArray(bytes)     if τ = LLVMArray(|bytes|, i8)
 ByteInt(bytes)       if τ = i{8|bytes|}
 `bitcast`(ByteInt(bytes) to τ)    if τ ∈ {`half`, `float`, `double`}
 `null`               if τ = LLVMPtrTy(U) ∧ AllZero(bytes)
 ⊥                    otherwise
LLVMGlobalZero(sym, τ, align) = LLVMGlobalConst(sym, τ, `zeroinitializer`, align)

LenLit(n) = IntLiteral(IntValue = n)
StaticType(sym) =
 TypeArray(TypePrim("u8"), LenLit(|bytes|))    if sym = Mangle(LiteralData(kind, bytes))
 StaticBindTypes(sym)                          otherwise
ProcModule(sym) = m ⇔ ∃ item, p. item = ProcedureDecl(_, _, _, _, _, _, _) ∧ ItemPath(item) = p ∧ sym = ScopedSym(item) ∧ ModuleOfPath(p) = m
SigOf(callee) =
 ⟨params, ret⟩    if callee = Mangle(d) ∧ d ∈ {ProcedureDecl, MethodDecl, DefaultImpl} ∧ Sig(d) = ⟨params, ret⟩
 RuntimeSig(sym)  if callee = sym ∧ RuntimeSig(sym) defined
 ⟨params, ret⟩    if ExprType(callee) = TypeFunc(params, ret)
 ⊥                otherwise
LoweredSigOf(callee) = ⟨params', ret⟩ ⇔ ⟨params, ret⟩ = SigOf(callee) ∧ params' = (NeedsPanicOut(callee) Sigma params ++ [PanicOutParam] : params)

ParamInitIR(sig, params) = ++_{⟨mode, x, T⟩ ∈ params} ParamInit(sig, params, x, mode, T)
ZeroValue(T) = `zeroinitializer` if sizeof(T) = 0
ParamInit(sig, params, x, mode, T) =
 Store(BindSlot(x), LLVMParam(sig, params, x), T)    if ABIParam(mode, T) = `ByValue` ∧ sizeof(T) > 0
 Store(BindSlot(x), ZeroValue(T), T)                 if ABIParam(mode, T) = `ByValue` ∧ sizeof(T) = 0
 ε                                                   if ABIParam(mode, T) = `ByRef`
ParamOrder(params) = [x_i | ⟨mode_i, x_i, T_i⟩ ∈ params ∧ (ABIParam(mode_i, T_i) = `ByRef` ∨ sizeof(T_i) > 0)]
ParamIndex(params, x) = i ⇔ ParamOrder(params)[i] = x
LLVMArgs(sig) = sig.llvm_params
LLVMArg(sig, i) = LLVMArgs(sig)[i]
i' = (sig.sretSigma Sigma ParamIndex(params, x) + 1 : ParamIndex(params, x))
LLVMParam(sig, params, x) = LLVMArg(sig, i')

**(LowerIRDecl-Proc-User)**
LLVMCallSig(params, R) ⇓ sig    ProcModule(sym) = m    IR_p = ParamInitIR(sig, params)    IR_0 = (NeedsPanicOut(sym) Sigma SeqIR(ClearPanic, IR) : IR)    IR' = SeqIR(IR_p, CheckPoison(m), IR_0)    Γ ⊢ LowerIRInstr(IR') ⇓ ll
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIRDecl(ProcIR(sym, params, R, IR)) ⇓ LLVMDefine(sym, sig, ll)

**(LowerIRDecl-Proc-Gen)**
LLVMCallSig(params, R) ⇓ sig    ProcModule(sym) undefined    IR_p = ParamInitIR(sig, params)    Γ ⊢ LowerIRInstr(SeqIR(IR_p, (NeedsPanicOut(sym) Sigma SeqIR(ClearPanic, IR) : IR))) ⇓ ll
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIRDecl(ProcIR(sym, params, R, IR)) ⇓ LLVMDefine(sym, sig, ll)

**(LowerIRDecl-GlobalConst)**
T = StaticType(sym)    Γ ⊢ LLVMTy(T) ⇓ τ    ConstBytes(τ, bytes) = c
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIRDecl(GlobalConst(sym, bytes)) ⇓ LLVMGlobalConst(sym, τ, c, alignof(T))

**(LowerIRDecl-GlobalZero)**
T = StaticType(sym)    Γ ⊢ LLVMTy(T) ⇓ τ    size = sizeof(T)
───────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIRDecl(GlobalZero(sym, size)) ⇓ LLVMGlobalZero(sym, τ, alignof(T))

**(LowerIRDecl-VTable)**
GlobalVTable(sym, header, slots) = d
──────────────────────────────────────────────────────────────
Γ ⊢ LowerIRDecl(d) ⇓ LLVMGlobalVTable(sym, header, slots)

**(Lower-AllocIR)**
RegionSym(`Region::alloc`) ⇓ sym    r = InnermostActiveRegion(Γ) if r_opt = ⊥, otherwise r_opt    Γ ⊢ LowerIRInstr(CallIR(sym, [r, v])) ⇓ ll
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(AllocIR(r_opt, v)) ⇓ ll

**(Lower-BindVarIR)**
Γ ⊢ BindSlot(x) ⇓ slot    TypeOf(x) = T_x
─────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(BindVarIR(x, v)) ⇓ ⟨[Store(slot, v, T_x)], ⊥⟩

**(Lower-ReadVarIR)**
Γ ⊢ BindSlot(x) ⇓ slot    TypeOf(x) = T_x    Γ ⊢ BindValid(x) ⇓ `Valid`
───────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(ReadVarIR(x)) ⇓ ⟨[Load(slot, T_x)], v⟩

**(Lower-ReadVarIR-Err)**
Γ ⊢ BindValid(x) ⇓ s    s ≠ `Valid`
───────────────────────────────────────────────
Γ ⊢ LowerIRInstr(ReadVarIR(x)) ⇑

PathSym(path, name) = PathSig(path ++ [name])
ProcSymbol(sym) ⇔ ∃ item. item ∈ {ProcedureDecl, MethodDecl, ClassMethodDecl, StateMethodDecl, TransitionDecl, DefaultImpl} ∧ Γ ⊢ Mangle(item) ⇓ sym

**(Lower-ReadPathIR-Static-User)**
StaticSymPath(path, name) = sym    ProcModule(sym) = m    T = StaticType(sym)    Γ ⊢ LowerIRInstr(CheckPoison(m)) ⇓ ⟨I_p, ⊥⟩
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(ReadPathIR(path, name)) ⇓ ⟨I_p ++ [Load(@sym, T)], v⟩

**(Lower-ReadPathIR-Static-Gen)**
StaticSymPath(path, name) = sym    ProcModule(sym) undefined    T = StaticType(sym)
───────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(ReadPathIR(path, name)) ⇓ ⟨[Load(@sym, T)], v⟩

**(Lower-ReadPathIR-Proc-User)**
sym = PathSym(path, name)    ProcSymbol(sym)    ProcModule(sym) = m    Γ ⊢ LowerIRInstr(CheckPoison(m)) ⇓ ⟨I_p, ⊥⟩
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(ReadPathIR(path, name)) ⇓ ⟨I_p, sym⟩

**(Lower-ReadPathIR-Proc-Gen)**
sym = PathSym(path, name)    ProcSymbol(sym)    ProcModule(sym) undefined
─────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(ReadPathIR(path, name)) ⇓ ⟨ε, sym⟩

**(Lower-ReadPathIR-Runtime)**
sym = PathSym(path, name)    RuntimeSig(sym) defined
────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(ReadPathIR(path, name)) ⇓ ⟨ε, sym⟩

**(Lower-ReadPathIR-Record)**
p = path ++ [name]    RecordDecl(p) = R    ModuleOfPath(p) = m    Γ ⊢ LowerIRInstr(CheckPoison(m)) ⇓ ⟨I_p, ⊥⟩
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(ReadPathIR(path, name)) ⇓ ⟨I_p, RecordCtor(p)⟩

**(Lower-StoreVarIR)**
Γ ⊢ BindSlot(x) ⇓ slot    TypeOf(x) = T_x    Γ ⊢ DropOnAssign(x, slot) ⇓ IR_d
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(StoreVarIR(x, v)) ⇓ ⟨IR_d ++ [Store(slot, v, T_x)], ⊥⟩

**(Lower-StoreVarNoDropIR)**
Γ ⊢ BindSlot(x) ⇓ slot    TypeOf(x) = T_x
──────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(StoreVarNoDropIR(x, v)) ⇓ ⟨[Store(slot, v, T_x)], ⊥⟩

**(Lower-MoveStateIR)**
x = PlaceRoot(p)    Γ ⊢ UpdateValid(x, MoveStateIR(p)) ⇓ v'
──────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(MoveStateIR(p)) ⇓ ⟨ε, ⊥⟩

**(Lower-StoreGlobal)**
T = StaticType(sym)    Γ ⊢ LLVMTy(T) ⇓ τ
────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(StoreGlobal(sym, v)) ⇓ ⟨[Store(@sym, v, T)], ⊥⟩

**(Lower-ReadPlaceIR)**
Γ ⊢ LowerReadPlace(p) ⇓ ⟨IR_p, v⟩    Γ ⊢ LowerIRInstr(IR_p) ⇓ ll
────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(ReadPlaceIR(p)) ⇓ ll

**(Lower-WritePlaceIR)**
Γ ⊢ LowerWritePlace(p, v) ⇓ IR_w    Γ ⊢ LowerIRInstr(IR_w) ⇓ ll
──────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(WritePlaceIR(p, v)) ⇓ ll

PtrType(v) = T ⇔ (∃ e, IR. Γ ⊢ LowerExpr(e) ⇓ ⟨IR, v⟩ ∧ T = ExprType(e)) ∨ (∃ p, IR. Γ ⊢ LowerReadPlace(p) ⇓ ⟨IR, v⟩ ∧ T = ExprType(p))
ValueType(v) = TypePrim("bool") ⇔ ∃ b. v = BoolVal(b)
ValueType(v) = TypePrim("char") ⇔ ∃ u. v = CharVal(u)
ValueType(UnitVal) = TypePrim("()")
ValueType(IntVal(t, x)) = TypePrim(t)
ValueType(FloatVal(t, v)) = TypePrim(t)
ValueType(PtrVal(s, addr)) = TypePtr(T, s) ⇔ T ∈ Type
ValueType(RawPtr(q, addr)) = TypeRawPtr(q, T) ⇔ T ∈ Type
ValueType((v_1, …, v_n)) = TypeTuple([T_1, …, T_n]) ⇔ ∀ i. ValueType(v_i) = T_i
ValueType([v_1, …, v_n]) = TypeArray(T, Literal(IntLiteral(n))) ⇔ ∀ i. ValueType(v_i) = T
ValueType(SliceValue(v, r)) = TypeSlice(T) ⇔ ValueType(v) = TypeArray(T, _) ∨ ValueType(v) = TypeSlice(T)
ValueType(RecordValue(TypePath(p), fs)) = TypePath(p)
ValueType(RecordValue(ModalStateRef(p, S), fs)) = TypeModalState(p, S)
ValueType(EnumValue(path, payload)) = TypePath(p) ⇔ EnumPath(path) = p
ValueType(RangeVal(k, lo, hi)) = TypeRange
ValueType(Dyn(Cl, RawPtr(`imm`, addr), T)) = TypeDynamic(Cl)
ValueType(v) = TypeString(`@View`) ⇔ v ∈ `string@View`
ValueType(v) = TypeString(`@Managed`) ⇔ v ∈ `string@Managed`
ValueType(v) = TypeBytes(`@View`) ⇔ v ∈ `bytes@View`
ValueType(v) = TypeBytes(`@Managed`) ⇔ v ∈ `bytes@Managed`
ValueType(v) = TypeString(⊥) ⇔ ValueType(v) = TypeString(`@View`) ∨ ValueType(v) = TypeString(`@Managed`)
ValueType(v) = TypeBytes(⊥) ⇔ ValueType(v) = TypeBytes(`@View`) ∨ ValueType(v) = TypeBytes(`@Managed`)
ValueType(v) = TypePath(p) ⇔ ∃ S, M. ValueType(v) = TypeModalState(p, S) ∧ Σ.Types[p] = `modal` M ∧ S ∈ States(M)
ValueType(v) = U ⇔ ∃ T. ValueType(v) = T ∧ Member(T, U)

**(Lower-ReadPtrIR)**
PtrType(v_ptr) = TypePtr(T, `Valid`)
──────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(ReadPtrIR(v_ptr)) ⇓ ⟨[Load(PtrAddr(v_ptr), T)], v⟩

**(Lower-ReadPtrIR-Raw)**
PtrType(v_ptr) = TypeRawPtr(q, T)
──────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(ReadPtrIR(v_ptr)) ⇓ ⟨[Load(PtrAddr(v_ptr), T)], v⟩

**(Lower-WritePtrIR)**
PtrType(v_ptr) = TypePtr(T, `Valid`)
────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(WritePtrIR(v_ptr, v)) ⇓ ⟨[Store(PtrAddr(v_ptr), v, T)], ⊥⟩

**(Lower-WritePtrIR-Null)**
PtrType(v_ptr) = TypePtr(T, `Null`)    Γ ⊢ LowerIRInstr(LowerPanic(NullDeref)) ⇓ ll
──────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(WritePtrIR(v_ptr, v)) ⇓ ll

**(Lower-WritePtrIR-Expired)**
PtrType(v_ptr) = TypePtr(T, `Expired`)    Γ ⊢ LowerIRInstr(LowerPanic(ExpiredDeref)) ⇓ ll
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(WritePtrIR(v_ptr, v)) ⇓ ll

**(Lower-WritePtrIR-Raw)**
PtrType(v_ptr) = TypeRawPtr(`mut`, T)
────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(WritePtrIR(v_ptr, v)) ⇓ ⟨[Store(PtrAddr(v_ptr), v, T)], ⊥⟩

**(Lower-WritePtrIR-Raw-Err)**
PtrType(v_ptr) = TypeRawPtr(`imm`, T)
──────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(WritePtrIR(v_ptr, v)) ⇑

**(Lower-AddrOfIR)**
Γ ⊢ LowerAddrOf(p) ⇓ ⟨IR_p, addr⟩    Γ ⊢ LowerIRInstr(IR_p) ⇓ ll
──────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(AddrOfIR(p)) ⇓ ll

**(Lower-CallIR-RecordCtor)**
CallTarget(callee) = RecordCtor(p)    args = []    RecordDefaultInits(p) = fields    Γ ⊢ LowerFieldInits(fields) ⇓ ⟨IR_f, vec_f⟩
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(CallIR(callee, args)) ⇓ ⟨IR_f, RecordValue(TypePath(p), vec_f)⟩

CallPoison(f) =
 CheckPoison(m)    if ProcModule(f) = m
 ε                if ProcModule(f) undefined

SRetAlloc(R) ⇓ ⟨[`alloca` LLVMTy(R)], p⟩

CallArgs(sig, params, args, R) ⇓ ⟨I_a, vec_a, p_ret⟩ ⇔
 I_a = ε ∧ vec_a = args ∧ p_ret = ⊥    if sig.sretSigma = false
 ∃ p. SRetAlloc(R) ⇓ ⟨I_s, p⟩ ∧ I_a = I_s ∧ vec_a = [p] ++ args ∧ p_ret = p    if sig.sretSigma = true

CallInstr(sig, f, vec_a) ⇓ ⟨[`call` sig f(vec_a)], v_c⟩ ⇔
 v_c = (sig.llvm_ret = `void` Sigma ⊥ : call_result)

CallResult(sig, R, p_ret, v_c) ⇓ ⟨I_r, v⟩ ⇔
 I_r = ε ∧ v = v_c    if sig.sretSigma = false
 LoadVal(p_ret, R) ⇓ ⟨I_r, v⟩    if sig.sretSigma = true

**(Lower-CallIR-Func)**
CallTarget(callee) = f    LoweredSigOf(f) = ⟨params, ret⟩    LLVMCallSig(params, ret) ⇓ sig    CallPoison(f) = IR_p    Γ ⊢ LowerIRInstr(IR_p) ⇓ ⟨I_p, ⊥⟩    CallArgs(sig, params, args, ret) ⇓ ⟨I_a, vec_a, p_ret⟩    CallInstr(sig, f, vec_a) ⇓ ⟨I_c, v_c⟩    CallResult(sig, ret, p_ret, v_c) ⇓ ⟨I_r, v_call⟩
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(CallIR(callee, args)) ⇓ ⟨I_p ++ I_a ++ I_c ++ I_r, v_call⟩

DynType(v) = TypeDynamic(Cl) ⇔ (∃ e, IR. Γ ⊢ LowerExpr(e) ⇓ ⟨IR, v⟩ ∧ ExprType(e) = TypeDynamic(Cl)) ∨ (∃ p, IR. Γ ⊢ LowerReadPlace(p) ⇓ ⟨IR, v⟩ ∧ ExprType(p) = TypeDynamic(Cl))
DynData(v) = FieldValue(v, `data`) and DynVTable(v) = FieldValue(v, `vtable`)
VTableSlotIndex(i) = i + 3
GEP(ptr, [i_0, …, i_k]) = v_gep
VTableSlotAddr(vt, i) = GEP(vt, [0, VTableSlotIndex(i)])
VTableSlot(vt, i) = Load(VTableSlotAddr(vt, i), TypeRawPtr(`imm`, TypePrim("()")))

**(Lower-CallVTable)**
DynType(base) = TypeDynamic(Cl)    v_d = DynData(base)    v_t = DynVTable(base)    v_s = VTableSlot(v_t, i)    Γ ⊢ LowerIRInstr(CallIR(v_s, [v_d] ++ args)) ⇓ ll
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(CallVTable(base, i, args)) ⇓ ll

**(LowerIRInstr-ClearPanic)**
Γ ⊢ ClearPanic ⇓ IR    Γ ⊢ LowerIRInstr(IR) ⇓ ll
────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(ClearPanic) ⇓ ll

**(LowerIRInstr-PanicCheck)**
Γ ⊢ PanicCheck ⇓ IR    Γ ⊢ LowerIRInstr(IR) ⇓ ll
────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(PanicCheck) ⇓ ll

**(LowerIRInstr-CheckPoison)**
Γ ⊢ CheckPoison(m) ⇓ IR    Γ ⊢ LowerIRInstr(IR) ⇓ ll
────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(CheckPoison(m)) ⇓ ll

**(LowerIRInstr-LowerPanic)**
Γ ⊢ LowerPanic(r) ⇓ IR    Γ ⊢ LowerIRInstr(IR) ⇓ ll
────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(LowerPanic(r)) ⇓ ll

IfPhi(v_t, v_f, l_t, l_f) ⇓ ⟨I_phi, v_phi⟩ ⇔
 I_phi = ε ∧ v_phi = ⊥    if v_t = ⊥ ∨ v_f = ⊥
 ∃ T, τ, inc. ValueType(v_t) = T ∧ ValueType(v_f) = T ∧ Γ ⊢ LLVMTy(T) ⇓ τ ∧ inc = [⟨v_t, l_t⟩, ⟨v_f, l_f⟩] ∧ I_phi = [Phi(τ, inc, v_phi)]    if v_t ≠ ⊥ ∧ v_f ≠ ⊥

IfLowerForm(I, v_c, v_t, v_f, v) ⇔ HasBrCond(I, v_c) ∧ ((v_t = ⊥ ∨ v_f = ⊥) ⇒ v = ⊥) ∧ ((v_t ≠ ⊥ ∧ v_f ≠ ⊥) ⇒ HasPhi(I, v))

**(Lower-IfIR)**
IfLabels(Γ) = ⟨l_t, l_f, l_m⟩    Γ ⊢ LowerIRInstr(IR_t) ⇓ ⟨I_t, v_t'⟩    Γ ⊢ LowerIRInstr(IR_f) ⇓ ⟨I_f, v_f'⟩    v_t' = v_t    v_f' = v_f    IfPhi(v_t, v_f, l_t, l_f) ⇓ ⟨I_phi, v⟩    I = [BrCond(v_c, l_t, l_f), Label(l_t)] ++ I_t ++ [Br(l_m), Label(l_f)] ++ I_f ++ [Br(l_m), Label(l_m)] ++ I_phi    IfLowerForm(I, v_c, v_t, v_f, v)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(IfIR(v_c, IR_t, v_t, IR_f, v_f)) ⇓ ⟨I, v⟩

BlockScope(IR_s, IR_t) = scope
BlockScope(IR_s, IR_t) = scope ⇔ (∃ σ, σ_1, σ_2, out, scope_0. BlockEnter(σ, []) ⇓ (σ_1, scope_0) ∧ ExecBlockBodyIRSigma(IR_s, IR_t, σ_1) ⇓ (out, σ_2)) ∧ (∀ σ, σ_1, σ_2, out, scope_0. BlockEnter(σ, []) ⇓ (σ_1, scope_0) ∧ ExecBlockBodyIRSigma(IR_s, IR_t, σ_1) ⇓ (out, σ_2) ⇒ CurrentScope(σ_2) = scope)
EmitCleanupSpec(cs, IR) ⇔ ∀ σ, Γ ⊢ Cleanup(cs, σ) ⇓ (c, σ') ⇒ (ExecIRSigma(IR, σ) ⇓ (out, σ') ∧ ((c = panic) ⇒ out = Ctrl(Panic)) ∧ ((c = ok) ⇒ out = Val(())))
Γ ⊢ EmitCleanup(cs) ⇓ IR ⇔ EmitCleanupSpec(cs, IR)

**(Lower-BlockIR)**
Γ ⊢ LowerIRInstr(IR_s) ⇓ ⟨I_s, ⊥⟩    Γ ⊢ LowerIRInstr(IR_t) ⇓ ⟨I_t, v_t'⟩    v_t' = v_t    BlockScope(IR_s, IR_t) = scope    Γ ⊢ CleanupPlan(scope) ⇓ cs    Γ ⊢ EmitCleanup(cs) ⇓ IR_c    Γ ⊢ LowerIRInstr(IR_c) ⇓ ⟨I_c, ⊥⟩
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(BlockIR(IR_s, IR_t, v_t)) ⇓ ⟨I_s ++ I_t ++ I_c, v_t⟩

LoopLowerForm(I, loop, v) predicate
LoopIRForm(loop) predicate
MatchLowerForm(I, match, v) predicate
MatchIRForm(match) predicate
RegionLowerForm(I, region, v) predicate
RegionIRForm(region) predicate
FrameLowerForm(I, frame, v) predicate
FrameIRForm(frame) predicate
LoopLowerForm(I, loop, v) ⇔ ⟨I, v⟩ ∈ LLResult
MatchLowerForm(I, match, v) ⇔ ⟨I, v⟩ ∈ LLResult
RegionLowerForm(I, region, v) ⇔ ⟨I, v⟩ ∈ LLResult
FrameLowerForm(I, frame, v) ⇔ ⟨I, v⟩ ∈ LLResult
LoopIRForm(loop) ⇔ (∃ IR_b, v_b. loop = LoopIR(LoopInfinite, IR_b, v_b)) ∨ (∃ IR_c, v_c, IR_b, v_b. loop = LoopIR(LoopConditional, IR_c, v_c, IR_b, v_b)) ∨ (∃ pat, ty_opt, IR_i, v_iter, IR_b, v_b. loop = LoopIR(LoopIter, pat, ty_opt, IR_i, v_iter, IR_b, v_b))
MatchIRForm(match) ⇔ ∃ v_s, arms. match = MatchIR(v_s, arms)
RegionIRForm(region) ⇔ ∃ v_o, alias_opt, IR_b, v_b. region = RegionIR(v_o, alias_opt, IR_b, v_b)
FrameIRForm(frame) ⇔ ∃ v_r, IR_b, v_b. frame = FrameIR(v_r, IR_b, v_b)

**(Lower-LoopIR)**
LoopIRForm(loop)    LoopLowerForm(I, loop, v)
────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(loop) ⇓ ⟨I, v⟩

**(Lower-MatchIR)**
MatchIRForm(match)    MatchLowerForm(I, match, v)
──────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(match) ⇓ ⟨I, v⟩

**(Lower-RegionIR)**
RegionIRForm(region)    RegionLowerForm(I, region, v)
──────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(region) ⇓ ⟨I, v⟩

**(Lower-FrameIR)**
FrameIRForm(frame)    FrameLowerForm(I, frame, v)
──────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(frame) ⇓ ⟨I, v⟩

BranchLowerForm(I, target) ⇔ Br(target) ∈ I
BranchLowerForm(I, v_c, t, f) ⇔ BrCond(v_c, t, f) ∈ I

**(Lower-BranchIR)**
BranchLowerForm(I, target)
────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(BranchIR(target)) ⇓ ⟨I, ⊥⟩
BranchLowerForm(I, v_c, t, f)
──────────────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(BranchIR(v_c, t, f)) ⇓ ⟨I, ⊥⟩

PhiLowerForm(I, T, inc, v) ⇔ Γ ⊢ LLVMTy(T) ⇓ τ ∧ I = [Phi(τ, inc, v)]

**(Lower-PhiIR)**
PhiLowerForm(I, T, inc, v)
──────────────────────────────────────────────────────────────
Γ ⊢ LowerIRInstr(PhiIR(T, inc, v)) ⇓ ⟨I, v⟩

**(LowerIRDecl-Err)**
Γ ⊢ LowerIRDecl(d) ⇑
────────────────────
Γ ⊢ LowerIRDecl(d) ⇑

**(LowerIRInstr-Err)**
Γ ⊢ LowerIRInstr(op) ⇑
──────────────────────
Γ ⊢ LowerIRInstr(op) ⇑

#### 6.12.11. Binding Storage and Validity

BindStorageJudg = {BindSlot(x) ⇓ slot, BindValid(x) ⇓ v, UpdateValid(x, op) ⇓ v', DropOnAssign(x, slot) ⇓ IR}
TypeOf(x) = T ⇔ Γ; R; L ⊢ Identifier(x) : T
BindInfo(x) = info ⇔ BindState(Γ) = 𝔅 ∧ Lookup_B(𝔅, x) = info

ProcParams(Γ) = params ⇔ Γ is lowering ProcIR(_, params, _, _)
ProcRet(Γ) = R ⇔ Γ is lowering ProcIR(_, _, R, _)
ProcSig(Γ) = sig ⇔ Γ ⊢ LLVMCallSig(ProcParams(Γ), ProcRet(Γ)) ⇓ sig
ParamEntry(params, x) = ⟨mode, T⟩ ⇔ ⟨mode, x, T⟩ ∈ params
AllocaSlot(T) = LLVMAlloca(LLVMTy(T))
BindState(Γ) = Γ.bind_state

**(BindValid-Sigma)**
BindState(Γ) = 𝔅    Lookup_B(𝔅, x) = ⟨s, _, _, _⟩
──────────────────────────────────────────────────────────────
Γ ⊢ BindValid(x) ⇓ s

**(BindSlot-Param-ByValue)**
ProcParams(Γ) = params    ParamEntry(params, x) = ⟨mode, T⟩    Γ ⊢ ABIParam(mode, T) ⇓ `ByValue`
───────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BindSlot(x) ⇓ AllocaSlot(T)

**(BindSlot-Param-ByRef)**
ProcParams(Γ) = params    ParamEntry(params, x) = ⟨mode, T⟩    Γ ⊢ ABIParam(mode, T) ⇓ `ByRef`    ProcSig(Γ) = sig
──────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BindSlot(x) ⇓ LLVMParam(sig, params, x)

**(BindSlot-Local)**
Γ ⊢ ResolveValueName(x) ⇓ ent    ent.origin_opt = ⊥    ParamEntry(ProcParams(Γ), x) undefined
──────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BindSlot(x) ⇓ AllocaSlot(TypeOf(x))

**(BindSlot-Static)**
Γ ⊢ ResolveValueName(x) ⇓ ent    ent.origin_opt = mp    name = (ent.target_opt if present, else x)    PathOfModule(mp) = path    StaticSymPath(path, name) = sym
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BindSlot(x) ⇓ @sym

**(UpdateValid-BindVar)**
──────────────────────────────────────────────────────────────
Γ ⊢ UpdateValid(x, BindVarIR(x, v)) ⇓ `Valid`

**(UpdateValid-StoreVar)**
──────────────────────────────────────────────────────────────
Γ ⊢ UpdateValid(x, StoreVarIR(x, v)) ⇓ `Valid`

**(UpdateValid-StoreVarNoDrop)**
Γ ⊢ BindValid(x) ⇓ s
──────────────────────────────────────────────────────────────────────
Γ ⊢ UpdateValid(x, StoreVarNoDropIR(x, v)) ⇓ s

**(UpdateValid-MoveRoot)**
op = MoveStateIR(p)    PlaceRoot(p) = x    FieldHead(p) = ⊥
──────────────────────────────────────────────────────────────────────
Γ ⊢ UpdateValid(x, op) ⇓ Moved

**(UpdateValid-PartialMove-Init)**
op = MoveStateIR(p)    PlaceRoot(p) = x    FieldHead(p) = f    BindValid(x) ⇓ `Valid`
─────────────────────────────────────────────────────────────────────────────────────────────
UpdateValid(x, op) ⇓ PartiallyMoved({f})

**(UpdateValid-PartialMove-Step)**
op = MoveStateIR(p)    PlaceRoot(p) = x    FieldHead(p) = f    BindValid(x) ⇓ PartiallyMoved(F)
────────────────────────────────────────────────────────────────────────────────────────────────
UpdateValid(x, op) ⇓ PartiallyMoved(F ∪ {f})

DropOnAssignApplicable(x) ⇔ BindInfo(x).mov = immov ∧ BindInfo(x).resp = resp
AddrAdd(addr, n) = addr + n
ElemType(T_b) = T ⇔ StripPerm(T_b) = TypeArray(T, _) ∨ StripPerm(T_b) = TypeSlice(T)
FieldAddr(T, addr, f) = AddrAdd(addr, FieldOffset(Fields(R), f))    when StripPerm(T) = TypePath(p) ∧ RecordDecl(p) = R
TupleAddr(T, addr, i) = AddrAdd(addr, FieldOffset(TupleFields([T_1, …, T_n]), i))    when StripPerm(T) = TypeTuple([T_1, …, T_n])

**(DropOnAssign-NotApplicable)**
¬ DropOnAssignApplicable(x)
──────────────────────────────────────────────────────────────
Γ ⊢ DropOnAssign(x, slot) ⇓ ε

FieldsRev(R) = rev(Fields(R))
FieldDropIR(slot, p, f, T) = EmitDrop(T, Load(FieldAddr(TypePath(p), slot, f), T))
FieldDropSeq(slot, p, F) = ++_{⟨f_i, T_i⟩ ∈ FieldsRev(RecordDecl(p)), f_i ∉ F} FieldDropIR(slot, p, f_i, T_i)

**(DropOnAssign-Record-Valid)**
DropOnAssignApplicable(x)    TypeOf(x) = TypePath(p)    RecordDecl(p) = R    BindValid(x) ⇓ `Valid`
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ DropOnAssign(x, slot) ⇓ EmitDrop(TypePath(p), Load(slot, TypePath(p)))

**(DropOnAssign-Record-Partial)**
DropOnAssignApplicable(x)    TypeOf(x) = TypePath(p)    RecordDecl(p) = R    BindValid(x) ⇓ PartiallyMoved(F)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ DropOnAssign(x, slot) ⇓ FieldDropSeq(slot, p, F)

**(DropOnAssign-Record-Moved)**
DropOnAssignApplicable(x)    TypeOf(x) = TypePath(p)    RecordDecl(p) = R    BindValid(x) ⇓ Moved
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ DropOnAssign(x, slot) ⇓ ε

**(DropOnAssign-Aggregate-Ok)**
DropOnAssignApplicable(x)    TypeOf(x) ∈ {TypeArray(_, _), TypeTuple(_), TypeUnion(_), TypeModalState(_, _)}    BindValid(x) ⇓ s    s ≠ Moved
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ DropOnAssign(x, slot) ⇓ EmitDrop(TypeOf(x), Load(slot, TypeOf(x)))

**(DropOnAssign-Aggregate-Moved)**
DropOnAssignApplicable(x)    TypeOf(x) ∈ {TypeArray(_, _), TypeTuple(_), TypeUnion(_), TypeModalState(_, _)}    BindValid(x) ⇓ Moved
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ DropOnAssign(x, slot) ⇓ ε

**(BindSlot-Err)**
BindSlot(x) undefined
────────────────────────────
Γ ⊢ BindSlot(x) ⇑

**(BindValid-Err)**
BindValid(x) undefined
─────────────────────────────
Γ ⊢ BindValid(x) ⇑

**(UpdateValid-Err)**
UpdateValid(x, op) undefined
────────────────────────────────
Γ ⊢ UpdateValid(x, op) ⇑

**(DropOnAssign-Err)**
DropOnAssign(x, slot) undefined
──────────────────────────────────
Γ ⊢ DropOnAssign(x, slot) ⇑

#### 6.12.12. Call ABI Mapping

LLVMCallJudg = {LLVMCallSig(params, ret) ⇓ sig, LLVMArgLower(x, T, k) ⇓ ll, LLVMRetLower(T, k) ⇓ ll}

SigLLVMParams(sig) = llvm_params
SigLLVMRet(sig) = llvm_ret
SigLLVMAttrs(sig) = attrs
SigSRet(sig) = sretSigma

**(LLVMArgLower-ByValue-PtrValid)**
Γ ⊢ LLVMTy(T) ⇓ τ    StripPerm(T) = TypePtr(U, `Valid`)
────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LLVMArgLower(x, T, `ByValue`) ⇓ ⟨τ, LLVMArgAttrsExt(x, T) ∪ LLVMPtrAttrs(T)⟩

**(LLVMArgLower-ByValue-Other)**
Γ ⊢ LLVMTy(T) ⇓ τ    StripPerm(T) ≠ TypePtr(_, `Valid`)
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LLVMArgLower(x, T, `ByValue`) ⇓ ⟨τ, LLVMArgAttrsExt(x, T)⟩

**(LLVMArgLower-ByRef)**
Γ ⊢ LLVMTy(T) ⇓ τ
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LLVMArgLower(x, T, `ByRef`) ⇓ ⟨LLVMPtrTy(TypePtr(TypePerm(`const`, T), `Valid`)), LLVMPtrAttrs(TypePtr(TypePerm(`const`, T), `Valid`)) ∪ LLVMArgAttrsExt(x, T)⟩

**(LLVMRetLower-ByValue-ZST)**
sizeof(T) = 0
──────────────────────────────────────────────
Γ ⊢ LLVMRetLower(T, `ByValue`) ⇓ `void`

**(LLVMRetLower-ByValue)**
Γ ⊢ LLVMTy(T) ⇓ τ    sizeof(T) > 0
─────────────────────────────────────────────
Γ ⊢ LLVMRetLower(T, `ByValue`) ⇓ τ

**(LLVMRetLower-SRet)**
Γ ⊢ LLVMTy(T) ⇓ τ
──────────────────────────────────────────
Γ ⊢ LLVMRetLower(T, `SRet`) ⇓ `void`

ArgInclude(k, T) ⇔ (k = `ByRef`) ∨ (k = `ByValue` ∧ sizeof(T) > 0)
LLVMArgList([⟨m_1, x_1, T_1⟩, …, ⟨m_n, x_n, T_n⟩], [k_1, …, k_n]) = [τ_i | ArgInclude(k_i, T_i) ∧ Γ ⊢ LLVMArgLower(x_i, T_i, k_i) ⇓ ⟨τ_i, A_i⟩]
LLVMAttrList([⟨m_1, x_1, T_1⟩, …, ⟨m_n, x_n, T_n⟩], [k_1, …, k_n]) = [A_i | ArgInclude(k_i, T_i) ∧ Γ ⊢ LLVMArgLower(x_i, T_i, k_i) ⇓ ⟨τ_i, A_i⟩]

**(LLVMCall-ByValue)**
⟨[k_1, …, k_n], k_r, sretSigma⟩ = ABICall([⟨m_1, T_1⟩, …, ⟨m_n, T_n⟩], R)    k_r = `ByValue`    ∀ i, Γ ⊢ LLVMArgLower(x_i, T_i, k_i) ⇓ ⟨τ_i, A_i⟩    Γ ⊢ LLVMRetLower(R, `ByValue`) ⇓ τ_r
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LLVMCallSig([⟨m_1, x_1, T_1⟩, …, ⟨m_n, x_n, T_n⟩], R) ⇓ ⟨LLVMArgList([⟨m_1, x_1, T_1⟩, …, ⟨m_n, x_n, T_n⟩], [k_1, …, k_n]), τ_r, LLVMAttrList([⟨m_1, x_1, T_1⟩, …, ⟨m_n, x_n, T_n⟩], [k_1, …, k_n]), false⟩

**(LLVMCall-SRet)**
⟨[k_1, …, k_n], k_r, sretSigma⟩ = ABICall([⟨m_1, T_1⟩, …, ⟨m_n, T_n⟩], R)    k_r = `SRet`    sret_param = LLVMPtrTy(TypePtr(TypePerm(`unique`, R), `Valid`))    A_sret = {`sret`, `noalias`} ∪ LLVMPtrAttrs(TypePtr(TypePerm(`unique`, R), `Valid`))    ∀ i, Γ ⊢ LLVMArgLower(x_i, T_i, k_i) ⇓ ⟨τ_i, A_i⟩    Γ ⊢ LLVMRetLower(R, `SRet`) ⇓ `void`
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LLVMCallSig([⟨m_1, x_1, T_1⟩, …, ⟨m_n, x_n, T_n⟩], R) ⇓ ⟨[sret_param] ++ LLVMArgList([⟨m_1, x_1, T_1⟩, …, ⟨m_n, x_n, T_n⟩], [k_1, …, k_n]), `void`, [A_sret] ++ LLVMAttrList([⟨m_1, x_1, T_1⟩, …, ⟨m_n, x_n, T_n⟩], [k_1, …, k_n]), true⟩

ByRefAccess(T) =
 `rw`    if PermOf(T) = `unique`
 `ro`    otherwise

**(LLVMArgLower-Err)**
LLVMArgLower(x, T, k) undefined
───────────────────────────────────────
Γ ⊢ LLVMArgLower(x, T, k) ⇑

**(LLVMRetLower-Err)**
LLVMRetLower(T, k) undefined
──────────────────────────────────
Γ ⊢ LLVMRetLower(T, k) ⇑

**(LLVMCall-Err)**
LLVMCallSig(params, ret) undefined
──────────────────────────────────────
Γ ⊢ LLVMCallSig(params, ret) ⇑

#### 6.12.13. VTable Emission

VTableJudg = {EmitVTable(T, Cl) ⇓ IRDecl, EmitDropGlue(T) ⇓ IRDecl, DropGlueSym(T) ⇓ sym}

DropGlueSym(T) = PathSig(["cursive", "runtime", "drop"] ++ PathOfType(T))
VTableHeader(T) = [sizeof(T), alignof(T), DropGlueSym(T)]
PtrTy = LLVMPtrTy(TypeRawPtr(`imm`, TypePrim("()")))
k = |VTable(T, Cl)|
VTableTy(Cl) = LLVMStruct([LLVMTy(TypePrim("usize")), LLVMTy(TypePrim("usize")), PtrTy] ++ [PtrTy]^k)
GlobalVTable : Symbol × Header × Slots → IRDecl
LLVMGlobalVTable : Symbol × Header × Slots → LLVMDecl

**(EmitVTable-Decl)**
Mangle(VTableDecl(T, Cl)) ⇓ sym
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EmitVTable(T, Cl) ⇓ GlobalVTable(sym, VTableHeader(T), VTable(T, Cl))

VTableSlots(T, Cl) = [DispatchSym(T, Cl, m.name) | m ∈ VTableEligible(Cl)]

DropGlueSpec(T, IR) ⇔ ∀ σ, addr, v. LookupVal(σ, "data") = RawPtr(`imm`, addr) ∧ ReadAddr(σ, addr) = v ⇒ (ExecIRSigma(IR, σ) ⇓ (out, σ') ∧ Γ ⊢ DropValue(T, v, ∅) ⇓ σ')
Γ ⊢ DropGlueIR(T) ⇓ IR ⇔ DropGlueSpec(T, IR)

**(EmitDropGlue-Decl)**
Γ ⊢ DropGlueSym(T) ⇓ sym    Γ ⊢ DropGlueIR(T) ⇓ IR_drop
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EmitDropGlue(T) ⇓ ProcIR(sym, [⟨`move`, `data`, TypeRawPtr(`imm`, TypePrim("()"))⟩, PanicOutParam], TypePrim("()"), IR_drop)

**(EmitVTable-Err)**
EmitVTable(T, Cl) undefined
──────────────────────────────
Γ ⊢ EmitVTable(T, Cl) ⇑

#### 6.12.14. Literal Data Emission

LiteralEmitJudg = {EmitLiteralData(kind, bytes) ⇓ IRDecl, EmitStringLit(lit) ⇓ sym, EmitBytesLit(lit) ⇓ sym}

LiteralDataSym(kind, bytes) = Mangle(LiteralData(kind, bytes))
StringBytes(lit) function
EscapeBytes(e) =
 EscapeValue(e)          if e = `"\u{"` h_1 … h_n `"}"`
 [EscapeValue(e)]        otherwise
StringBytesFrom(T, p, q) =
 []                                                        if p = q
 EscapeBytes(Lexeme(T, p, r)) ++ StringBytesFrom(T, r, q)   if p < q ∧ T[p] = `"\\"` ∧ EscapeMatch(T, p, r)
 EncodeUTF8(T[p]) ++ StringBytesFrom(T, p + 1, q)           if p < q ∧ T[p] ≠ `"\\"`
StringBytes(lit) = bytes ⇔ lit.kind = StringLiteral ∧ T = Lexeme(lit) ∧ StringBytesFrom(T, 1, |T|-1) = bytes
RawBytes(lit) = bytes ⇔ lit.kind = BytesLiteral ∧ lit.payload = bytes
RawBytes(lit) = StringBytes(lit) ⇔ lit.kind = StringLiteral

**(EmitLiteralData-Decl)**
Γ ⊢ Mangle(LiteralData(kind, bytes)) ⇓ sym
──────────────────────────────────────────────────────────────
Γ ⊢ EmitLiteralData(kind, bytes) ⇓ GlobalConst(sym, bytes)

**(EmitLiteral-String)**
StringBytes(lit) = bytes    Γ ⊢ Mangle(LiteralData("string", bytes)) ⇓ sym
────────────────────────────────────────────────────────────────────
Γ ⊢ EmitStringLit(lit) ⇓ sym
StringBytes(lit) = bytes ⇒ Utf8Valid(bytes)

**(EmitLiteral-Bytes)**
RawBytes(lit) = bytes    Γ ⊢ Mangle(LiteralData("bytes", bytes)) ⇓ sym
──────────────────────────────────────────────────────────────────
Γ ⊢ EmitBytesLit(lit) ⇓ sym
RawBytes(lit) undefined ⇒ EmitBytesLit(lit) undefined

**(EmitLiteral-Char)**
T = TypePrim("char")    Γ ⊢ EncodeConst(T, lit) ⇓ bytes
─────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EmitLiteralData("char", bytes) ⇓ GlobalConst(Mangle(LiteralData("char", bytes)), bytes)

**(EmitLiteral-Int)**
T = TypePrim(t)    t ∈ IntTypes    Γ ⊢ EncodeConst(T, lit) ⇓ bytes
──────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EmitLiteralData("int", bytes) ⇓ GlobalConst(Mangle(LiteralData("int", bytes)), bytes)

**(EmitLiteral-Float)**
T = TypePrim(t)    t ∈ FloatTypes    Γ ⊢ EncodeConst(T, lit) ⇓ bytes
────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EmitLiteralData("float", bytes) ⇓ GlobalConst(Mangle(LiteralData("float", bytes)), bytes)

**(EmitLiteral-Err)**
EmitLiteralData(kind, bytes) undefined
─────────────────────────────────────────
Γ ⊢ EmitLiteralData(kind, bytes) ⇑

#### 6.12.15. String/Bytes Built-ins

BuiltinSymJudg = {BuiltinSym(method) ⇓ sym}

StringBuiltins = {`string::from`, `string::as_view`, `string::to_managed`, `string::clone_with`, `string::append`, `string::length`, `string::is_empty`}
BytesBuiltins = {`bytes::with_capacity`, `bytes::from_slice`, `bytes::as_view`, `bytes::to_managed`, `bytes::view`, `bytes::view_string`, `bytes::append`, `bytes::length`, `bytes::is_empty`}
StringMethod(method) ⇔ ∃ name. method = `string::`name
BytesMethod(method) ⇔ ∃ name. method = `bytes::`name

BuiltinSym(`string::from`) = PathSig(["cursive", "runtime", "string", "from"])
BuiltinSym(`string::as_view`) = PathSig(["cursive", "runtime", "string", "as_view"])
BuiltinSym(`string::to_managed`) = PathSig(["cursive", "runtime", "string", "to_managed"])
BuiltinSym(`string::clone_with`) = PathSig(["cursive", "runtime", "string", "clone_with"])
BuiltinSym(`string::append`) = PathSig(["cursive", "runtime", "string", "append"])
BuiltinSym(`string::length`) = PathSig(["cursive", "runtime", "string", "length"])
BuiltinSym(`string::is_empty`) = PathSig(["cursive", "runtime", "string", "is_empty"])

BuiltinSym(`bytes::with_capacity`) = PathSig(["cursive", "runtime", "bytes", "with_capacity"])
BuiltinSym(`bytes::from_slice`) = PathSig(["cursive", "runtime", "bytes", "from_slice"])
BuiltinSym(`bytes::as_view`) = PathSig(["cursive", "runtime", "bytes", "as_view"])
BuiltinSym(`bytes::to_managed`) = PathSig(["cursive", "runtime", "bytes", "to_managed"])
BuiltinSym(`bytes::view`) = PathSig(["cursive", "runtime", "bytes", "view"])
BuiltinSym(`bytes::view_string`) = PathSig(["cursive", "runtime", "bytes", "view_string"])
BuiltinSym(`bytes::append`) = PathSig(["cursive", "runtime", "bytes", "append"])
BuiltinSym(`bytes::length`) = PathSig(["cursive", "runtime", "bytes", "length"])
BuiltinSym(`bytes::is_empty`) = PathSig(["cursive", "runtime", "bytes", "is_empty"])

**(BuiltinSym-String-Err)**
StringMethod(method)    method ∉ StringBuiltins
───────────────────────────────────────────────
Γ ⊢ BuiltinSym(method) ⇑

**(BuiltinSym-Bytes-Err)**
BytesMethod(method)    method ∉ BytesBuiltins
──────────────────────────────────────────────
Γ ⊢ BuiltinSym(method) ⇑

#### 6.12.16. Managed String/Bytes Drop Hooks

DropHookJudg = {StringDropSym ⇓ sym, BytesDropSym ⇓ sym}

**(StringDropSym-Decl)**
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ StringDropSym ⇓ PathSig(["cursive", "runtime", "string", "drop_managed"])

**(BytesDropSym-Decl)**
───────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ BytesDropSym ⇓ PathSig(["cursive", "runtime", "bytes", "drop_managed"])

**(StringDropSym-Err)**
StringDropSym undefined
──────────────────────────
Γ ⊢ StringDropSym ⇑

**(BytesDropSym-Err)**
BytesDropSym undefined
─────────────────────────
Γ ⊢ BytesDropSym ⇑

#### 6.12.17. Entrypoint and Context Construction

EntryJudg = {EntrySym ⇓ sym, ContextInitSym ⇓ sym, EntryStub(P) ⇓ IRDecl}

**(EntrySym-Decl)**
──────────────────────────────────────────────
Γ ⊢ EntrySym ⇓ PathSig(["main"])

**(ContextInitSym-Decl)**
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ContextInitSym ⇓ PathSig(["cursive", "runtime", "context_init"])

PanicRecordInit(σ) ⇔ PanicRecordOf(σ) = ⟨false, 0⟩
EntryStubSpec(P, IR_entry) ⇔ Executable(P) ∧ ∃ d, main_sym. MainDecls(P) = [d] ∧ Γ ⊢ Mangle(d) ⇓ main_sym ∧ ∀ σ. ∃ ctx, ret, c, σ_1, σ_2, σ_3, σ_4.
 ExecIRSigma(CallIR(ContextInitSym, []), σ) ⇓ (Val(ctx), σ_1) ∧ PanicRecordInit(σ_2) ∧ ExecIRSigma(CallIR(main_sym, [ctx, PanicOutName]), σ_2) ⇓ (Val(ret), σ_3) ∧
 (PanicRecordOf(σ_3) = ⟨true, c⟩ ⇒ ExecIRSigma(CallIR(PanicSym, [c]), σ_3) ⇓ (Ctrl(Panic), σ_4)) ∧
 (PanicRecordOf(σ_3) = ⟨false, c⟩ ⇒ ∃ IR_d. Γ ⊢ EmitDeinitPlan(P) ⇓ IR_d ∧ ExecIRSigma(IR_d, σ_3) ⇓ (Val(()), σ_4)) ∧
 (PanicRecordOf(σ_3) = ⟨true, c⟩ ⇒ ExecIRSigma(IR_entry, σ) ⇓ (Ctrl(Panic), σ_4)) ∧
 (PanicRecordOf(σ_3) = ⟨false, c⟩ ⇒ ExecIRSigma(IR_entry, σ) ⇓ (Val(ret), σ_4))

**(EntryStub-Decl)**
Γ ⊢ EntrySym ⇓ sym    EntryStubSpec(P, IR_entry)
──────────────────────────────────────────────────────────────────────
Γ ⊢ EntryStub(P) ⇓ ProcIR(sym, [], TypePrim("i32"), IR_entry)

**(EntrySym-Err)**
EntrySym undefined
────────────────────
Γ ⊢ EntrySym ⇑

**(EntryStub-Err)**
EntryStub(P) undefined
────────────────────────
Γ ⊢ EntryStub(P) ⇑

#### 6.12.18. Poisoning Instrumentation

PoisonJudg = {PoisonFlag(m) ⇓ sym, CheckPoison(m) ⇓ IR, SetPoison(m) ⇓ IR}

PoisonSet(m) = {m} ∪ {x | Reachable(x, m, E_val^eager)}

**(PoisonFlag-Decl)**
──────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ PoisonFlag(m) ⇓ PathSig(["cursive", "runtime", "poison"] ++ PathOfModule(m))

PoisonFlagDecl(m) = GlobalZero(PoisonFlag(m), sizeof(TypePrim("bool")))
StaticType(PoisonFlag(m)) = TypePrim("bool")

**(CheckPoison-Use)**
PoisonFlag(m) ⇓ sym
────────────────────────────────────
Γ ⊢ CheckPoison(m) ⇓ IR
Γ ⊢ CheckPoison(m) ⇓ IR ⇔ ∀ σ. (ReadAddr(σ, AddrOfSym(PoisonFlag(m))) ≠ 0 ⇒ ∃ σ'. ExecIRSigma(IR, σ) ⇓ (Ctrl(Panic), σ') ∧ ExecIRSigma(LowerPanic(InitPanic(m)), σ) ⇓ (Ctrl(Panic), σ')) ∧ (ReadAddr(σ, AddrOfSym(PoisonFlag(m))) = 0 ⇒ ExecIRSigma(IR, σ) ⇓ (Val(()), σ))

**(SetPoison-OnInitFail)**
PoisonSet(m) = {m_1, …, m_k}    ∀ i, PoisonFlag(m_i) ⇓ sym_i
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ SetPoison(m) ⇓ SeqIR(StoreGlobal(sym_1, 1), …, StoreGlobal(sym_k, 1))

**(PoisonFlag-Err)**
PoisonFlag(m) undefined
──────────────────────────
Γ ⊢ PoisonFlag(m) ⇑

**(CheckPoison-Err)**
CheckPoison(m) undefined
──────────────────────────
Γ ⊢ CheckPoison(m) ⇑

**(SetPoison-Err)**
SetPoison(m) undefined
───────────────────────
Γ ⊢ SetPoison(m) ⇑

## 7. Dynamic Semantics

### 7.1. Initialization Order and Poisoning

Vertices(G_e) = V ⇔ G_e = ⟨V, E⟩
Edges(G_e) = E ⇔ G_e = ⟨V, E⟩
Index(L, x) = i ⇔ 0 ≤ i < |L| ∧ L[i] = x
TopoOrder(G_e, L) ⇔ Distinct(L) ∧ Set(L) = Vertices(G_e) ∧ ∀ (u, v) ∈ Edges(G_e). Index(L, u) < Index(L, v)
Incomparable_{G_e}(u, v) ⇔ ¬ Reachable(u, v, Edges(G_e)) ∧ ¬ Reachable(v, u, Edges(G_e))
TopoTieBreak(G_e, L, P) ⇔ ∀ u, v ∈ Vertices(G_e). Incomparable_{G_e}(u, v) ∧ Index(P.modules, u) < Index(P.modules, v) ⇒ Index(L, u) < Index(L, v)
Cycle(G_e) ⇔ ∃ v ∈ Vertices(G_e). Reachable(v, v, Edges(G_e))

**(Topo-Ok)**
Project(Γ) = P    Γ ⊢ G_e : DAG    TopoOrder(G_e, L)    TopoTieBreak(G_e, L, P)
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Topo(G_e) ⇓ L

**(Topo-Cycle)**
Cycle(G_e)    c = Code(Topo-Cycle)
──────────────────────────────────────
Γ ⊢ Topo(G_e) ⇑ c

P = Project(Γ)
StaticInitOf(item) = init ⇔ item = StaticDecl(vis, mut, binding, span, doc) ∧ binding = ⟨pat, ty_opt, op, init, sp⟩
StaticInitOf(item) = ⊥ ⇔ item ∉ StaticDecl(_, _, _, _, _)
InitList(m) = [ init | item ∈ Items(P, m) ∧ StaticInitOf(item) = init ]

InitOrder(G_e) = L ⇔ Γ ⊢ Topo(G_e) ⇓ L
InitPlan(G_e) = ++_{m ∈ InitOrder(G_e)} InitList(m)

DeinitOrder(G_e) = rev(InitOrder(G_e))

StaticBindOrder(m) = ++_{item ∈ StaticItems(P, m), item = StaticDecl(vis, mut, binding, span, doc)} [⟨PathOfModule(m), x⟩ | x ∈ StaticBindList(binding)]

GlobalStaticOrder = ++_{m ∈ InitOrder(G_e)} StaticBindOrder(m)

DeinitList(P) = rev([ DropStatic(path, name) | ⟨path, name⟩ ∈ GlobalStaticOrder ∧ StaticBindInfo(path, name).resp = resp ])

Γ ⊢ Eval(e, σ) ⇓ v ⇔ ∃ σ'. Γ ⊢ EvalSigma(e, σ) ⇓ (Val(v), σ')
Γ ⊢ Eval(e, σ) ⇑ panic ⇔ ∃ σ'. Γ ⊢ EvalSigma(e, σ) ⇓ (Ctrl(Panic), σ')

**Initialization (Small-Step).**

InitState = {InitStart(G_e, L, σ), InitMod(L, mi, ii, P, σ), InitDone(σ), InitPanic(P, σ)}
InitItem(L, mi, ii) = e ⇔ mi < |L| ∧ L[mi] = m ∧ InitList(m)[ii] = e
InitLen(L, mi) = k ⇔ mi < |L| ∧ L[mi] = m ∧ |InitList(m)| = k

**(Init-Start)**
──────────────────────────────────────────────────────────────────────────
⟨InitStart(G_e, L, σ)⟩ → ⟨InitMod(L, 0, 0, ∅, σ)⟩

**(Init-Step)**
InitItem(L, mi, ii) = e    Γ ⊢ EvalSigma(e, σ) ⇓ (Val(v), σ')
────────────────────────────────────────────────────────────────────────────────────────────
⟨InitMod(L, mi, ii, P, σ)⟩ → ⟨InitMod(L, mi, ii + 1, P, σ')⟩

**(Init-Next-Module)**
InitLen(L, mi) = k    ii = k
──────────────────────────────────────────────────────────────
⟨InitMod(L, mi, ii, P, σ)⟩ → ⟨InitMod(L, mi + 1, 0, P, σ)⟩

**(Init-Panic)**
InitItem(L, mi, ii) = e    Γ ⊢ EvalSigma(e, σ) ⇓ (Ctrl(Panic), σ')    L[mi] = m    P' = P ∪ {m} ∪ {x | Reachable(x, m, E_val^eager)}
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨InitMod(L, mi, ii, P, σ)⟩ → ⟨InitPanic(P', σ')⟩

**(Init-Done)**
mi = |L|
────────────────────────────────────────────────
⟨InitMod(L, mi, ii, P, σ)⟩ → ⟨InitDone(σ)⟩

**Initialization (Big-Step).**

**(Init-Ok)**
⟨InitStart(G_e, InitOrder(G_e), σ)⟩ →* ⟨InitDone(σ')⟩
──────────────────────────────────────────────────────────────
Γ ⊢ Init(G_e, σ) ⇓ σ'

**(Init-Fail)**
⟨InitStart(G_e, InitOrder(G_e), σ)⟩ →* ⟨InitPanic(P, σ')⟩
──────────────────────────────────────────────────────────────
Γ ⊢ Init(G_e, σ) ⇑ panic(P)

**Deinitialization (Big-Step).**

**(Deinit-Ok)**
Γ ⊢ Cleanup(DeinitList(P), σ) ⇓ (ok, σ')
───────────────────────────────────────────────────────────
Γ ⊢ Deinit(P, σ) ⇓ σ'

**(Deinit-Panic)**
Γ ⊢ Cleanup(DeinitList(P), σ) ⇓ (panic, σ')
───────────────────────────────────────────────────────────
Γ ⊢ Deinit(P, σ) ⇑ panic

### 7.2. Modal Layout (Dynamic Semantics)

layout(M@S) = layout(`record` {Payload(M, S)})
Payload(M, S) = ∅ ⇒ sizeof(M@S) = 0
layout(M) =
 layout(T_p)    if NicheApplies(M) ∧ PayloadState(M) = S_p ∧ SingleFieldPayload(M, S_p) = T_p
 layout(`enum` {S_1(Payload(M, S_1)), …, S_n(Payload(M, S_n))})    otherwise
sizeof(M) =
 sizeof(T_p)    if NicheApplies(M) ∧ PayloadState(M) = S_p ∧ SingleFieldPayload(M, S_p) = T_p
 sizeof(Discriminant) + max_{S ∈ States(M)}(sizeof(M@S)) + Padding    otherwise
alignof(M) =
 alignof(T_p)    if NicheApplies(M) ∧ PayloadState(M) = S_p ∧ SingleFieldPayload(M, S_p) = T_p
 max(alignof(Discriminant), max_{S ∈ States(M)}(alignof(M@S)))    otherwise
ValueBits(M, ⟨S, v⟩) = bits ⇔ ModalBits(M, S, v) = bits

### 7.3. Modal Pattern Matching

MatchModalJudg = {MatchModal(p, v) ⇓ B}

**(Match-Modal-Empty)**
────────────────────────────────────────────────────────────
Γ ⊢ MatchModal(@S, ⟨S, v⟩) ⇓ ∅

**(Match-Modal-Record)**
Γ ⊢ MatchRecord(fs, v) ⇓ B
──────────────────────────────────────────────────────────────
Γ ⊢ MatchModal(@S{fs}, ⟨S, v⟩) ⇓ B

BindEnv = Ident ⇀ Value
Dom(B) = {x | x ∈ Ident ∧ B[x] defined}
B_1 ⊎ B_2 = B ⇔ Dom(B_1) ∩ Dom(B_2) = ∅ ∧ ∀ x. (x ∈ Dom(B_1) ⇒ B[x] = B_1[x]) ∧ (x ∈ Dom(B_2) ⇒ B[x] = B_2[x])
MatchPatJudg = {MatchPattern(p, v) ⇓ B}
PatType(LiteralPattern(lit)) =
 TypePrim(t)         if lit.kind = IntLiteral ∧ IntSuffix(lit) = t
 TypePrim("i32")     if lit.kind = IntLiteral ∧ IntSuffix(lit) = ⊥
 TypePrim(t)         if lit.kind = FloatLiteral ∧ FloatSuffix(lit) = t
 TypePrim("f64")     if lit.kind = FloatLiteral ∧ FloatSuffix(lit) = ⊥
 TypePrim("bool")    if lit.kind = BoolLiteral
 TypePrim("char")    if lit.kind = CharLiteral
 TypeString(`@View`) if lit.kind = StringLiteral
 ⊥                   if lit.kind = NullLiteral

**(Match-Wildcard)**
──────────────────────────────────────────────
Γ ⊢ MatchPattern(_, v) ⇓ ∅

**(Match-Ident)**
──────────────────────────────────────────────
Γ ⊢ MatchPattern(x, v) ⇓ {x ↦ v}

**(Match-Typed)**
UnionCase(v) = ⟨T', v'⟩
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ MatchPattern(x : T, v) ⇓ {x ↦ v'}    if Γ ⊢ T' ≡ T else ⊥

**(Match-Literal)**
T = PatType(ℓ)    LiteralValue(ℓ, T) = v
──────────────────────────────────────────────────────────────
Γ ⊢ MatchPattern(ℓ, v) ⇓ ∅

**(Match-Tuple)**
v = (v_1, …, v_n)    ∀ i, Γ ⊢ MatchPattern(p_i, v_i) ⇓ B_i    B = ⊎_i B_i
───────────────────────────────────────────────────────────────────────────────
Γ ⊢ MatchPattern((p_1, …, p_n), v) ⇓ B

**(Match-Record)**
Γ ⊢ MatchRecord(fs, v) ⇓ B
──────────────────────────────────────────────
Γ ⊢ MatchPattern(R{fs}, v) ⇓ B

**(Match-Enum-Unit)**
v = EnumValue(path', ⊥)    EnumPath(path') = path    VariantName(path') = name
───────────────────────────────────────────────────────────────────────────────
Γ ⊢ MatchPattern(EnumPattern(path, name, ⊥), v) ⇓ ∅

**(Match-Enum-Tuple)**
v = EnumValue(path', TuplePayload(vec_v))    EnumPath(path') = path    VariantName(path') = name    ∀ i, Γ ⊢ MatchPattern(p_i, v_i) ⇓ B_i    B = ⊎_i B_i
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ MatchPattern(EnumPattern(path, name, TuplePayloadPattern([p_1, …, p_n])), v) ⇓ B

**(Match-Enum-Record)**
v = EnumValue(path', RecordPayload(vec_f))    EnumPath(path') = path    VariantName(path') = name    Γ ⊢ MatchRecord(fs, RecordPayload(vec_f)) ⇓ B
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ MatchPattern(EnumPattern(path, name, RecordPayloadPattern(fs)), v) ⇓ B

**(Match-Modal-General)**
Γ ⊢ MatchModal(@S{fs}, ⟨S, v⟩) ⇓ B
──────────────────────────────────────────────────────────────
Γ ⊢ MatchPattern(@S{fs}, ⟨S, v⟩) ⇓ B

**(Match-Modal-State)**
Γ ⊢ MatchRecord(fs, v) ⇓ B
──────────────────────────────────────────────
Γ ⊢ MatchPattern(@S{fs}, v) ⇓ B

**(Match-Range)**
ConstPat(p_l) = v_l    ConstPat(p_h) = v_h    v_l ≤ v ≤ v_h
────────────────────────────────────────────────────────────────
Γ ⊢ MatchPattern(p_l `..=` p_h, v) ⇓ ∅

ConstPat(p_l) = v_l    ConstPat(p_h) = v_h    v_l ≤ v < v_h
───────────────────────────────────────────────
Γ ⊢ MatchPattern(p_l `..` p_h, v) ⇓ ∅

### 7.4. Deterministic Destruction and Unwinding (Cursive0)

Responsible(b) ⇔ BindInfo(b).resp = resp

CleanupItem ::= DropBinding(b) | DropStatic(path, name) | DeferBlock(b)
DropStatus = {ok, panic}
DropJudg = {DropAction(b) ⇓ σ', DropValue(T, v, F) ⇓ σ', DropStaticAction(path, name) ⇓ σ', DropActionOut(b) ⇓ (c, σ'), DropValueOut(T, v, F) ⇓ (c, σ'), DropStaticActionOut(path, name) ⇓ (c, σ')}
DropAction(b) ⇓ σ' ⇔ DropActionOut(b) ⇓ (ok, σ')
DropValue(T, v, F) ⇓ σ' ⇔ DropValueOut(T, v, F) ⇓ (ok, σ')
DropStaticAction(path, name) ⇓ σ' ⇔ DropStaticActionOut(path, name) ⇓ (ok, σ')
RecordType(T) ⇔ ∃ p. T = TypePath(p) ∧ RecordDecl(p) defined
DropCall(T, v, σ) ⇓ (out, σ') relation
¬ ImplementsDrop(T) ⇒ DropCall(T, v, σ) ⇓ (Val(()), σ)
ImplementsDrop(T) ∧ BuiltinDropType(T) ∧ T = TypeString(`@Managed`) ∧ Γ ⊢ StringDropSym ⇓ sym ∧ ExecIRSigma(CallIR(sym, [v]), σ) ⇓ (out, σ') ⇒ DropCall(T, v, σ) ⇓ (out, σ')
ImplementsDrop(T) ∧ BuiltinDropType(T) ∧ T = TypeBytes(`@Managed`) ∧ Γ ⊢ BytesDropSym ⇓ sym ∧ ExecIRSigma(CallIR(sym, [v]), σ) ⇓ (out, σ') ⇒ DropCall(T, v, σ) ⇓ (out, σ')
ImplementsDrop(T) ∧ ¬ BuiltinDropType(T) ∧ LookupMethod(StripPerm(T), "drop") = m ∧ BindParams(MethodParamsDecl(StripPerm(T), m), [v]) = binds ∧ BlockEnter(σ, binds) ⇓ (σ_1, scope) ∧ Γ ⊢ EvalBlockBodySigma(m.body, σ_1) ⇓ (out_1, σ_2) ∧ BlockExit(σ_2, scope, out_1) ⇓ (out_2, σ_3) ∧ ReturnOut(out_2) = out ⇒ DropCall(T, v, σ) ⇓ (out, σ_3)
ReleaseValue(T, v, σ) ⇓ σ' relation
ReleaseValue(T, v, σ) ⇓ σ' ⇔ σ' = σ
DropChildren(T, v, F) =
 [⟨T_i, v_i⟩ | ⟨f_i, T_i⟩ ∈ FieldsRev(R), f_i ∉ F, FieldValue(v, f_i) = v_i]    if T = TypePath(p) ∧ RecordDecl(p) = R
 [⟨T_i, v_i⟩ | T = TypeTuple([T_0, …, T_{n-1}]), i ∈ rev([0, …, n-1]), TupleValue(v, i) = v_i]    if T = TypeTuple(_)
 [⟨T_e, v_i⟩ | T = TypeArray(T_e, n), i ∈ rev([0, …, n-1]), IndexValue(v, i) = v_i]    if T = TypeArray(_, _)
 [⟨T', v'⟩ | UnionCase(v) = ⟨T', v'⟩]    if T = TypeUnion(_)
 [⟨TypeModalState(p, S), v_s⟩ | v = ⟨S, v_s⟩]    if T = TypePath(p) ∧ Σ.Types[p] = `modal` M
 [⟨T_i, v_i⟩ | ⟨f_i, T_i⟩ ∈ Payload(M, S), FieldValue(v, f_i) = v_i]    if T = TypeModalState(p, S) ∧ Σ.Types[p] = `modal` M
 []    otherwise
DropList([], σ) ⇓ (ok, σ)
DropList([⟨T, v⟩] ++ xs, σ) ⇓ (c, σ'') ⇔ DropValueOut(T, v, ∅) ⇓ (c_1, σ') ∧ (c_1 = panic ⇒ c = panic ∧ σ'' = σ') ∧ (c_1 = ok ⇒ DropList(xs, σ') ⇓ (c, σ''))

**(DropAction-Moved)**
BindState(σ, b) = Moved
──────────────────────────────────────────────
Γ ⊢ DropActionOut(b) ⇓ (ok, σ)

**(DropAction-Partial)**
BindState(σ, b) = PartiallyMoved(F)    Γ ⊢ DropValueOut(TypeOf(b), BindingValue(σ, b), F) ⇓ (c, σ')
────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ DropActionOut(b) ⇓ (c, σ')

**(DropAction-Valid)**
BindState(σ, b) = `Valid`    Γ ⊢ DropValueOut(TypeOf(b), BindingValue(σ, b), ∅) ⇓ (c, σ')
──────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ DropActionOut(b) ⇓ (c, σ')

**(DropStaticAction)**
StaticAddr(path, name) = addr    ReadAddr(σ, addr) = v    Γ ⊢ DropValueOut(StaticType(path, name), v, ∅) ⇓ (c, σ')
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ DropStaticActionOut(path, name) ⇓ (c, σ')

NonRecordFOk(T, F) ⇔ RecordType(T) ∨ F = ∅

**(DropValueOut-DropPanic)**
NonRecordFOk(T, F)    DropCall(T, v, σ) ⇓ (Ctrl(Panic), σ_1)
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ DropValueOut(T, v, F) ⇓ (panic, σ_1)

**(DropValueOut-ChildPanic)**
NonRecordFOk(T, F)    DropCall(T, v, σ) ⇓ (Val(()), σ_1)    DropList(DropChildren(T, v, F), σ_1) ⇓ (panic, σ_2)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ DropValueOut(T, v, F) ⇓ (panic, σ_2)

**(DropValueOut-Ok)**
NonRecordFOk(T, F)    DropCall(T, v, σ) ⇓ (Val(()), σ_1)    DropList(DropChildren(T, v, F), σ_1) ⇓ (ok, σ_2)    ReleaseValue(T, v, σ_2) ⇓ σ_3
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ DropValueOut(T, v, F) ⇓ (ok, σ_3)

**Cleanup (Small-Step).**

CleanupFlag = {ok, panic}
CleanupState = {CleanupLoop(scope, σ, c) | c ∈ CleanupFlag} ∪ {ExitDone(c, σ) | c ∈ CleanupFlag} ∪ {Abort}

**(Cleanup-Start)**
────────────────────────────────────────────────────────────────────
⟨ExitScope(scope, σ)⟩ → ⟨CleanupLoop(scope, σ, ok)⟩

**(Cleanup-Step-Drop-Ok)**
CleanupList(scope) = rest ++ [DropBinding(b)]    σ_1 = SetCleanupList(scope, rest, σ)    Γ ⊢ DropActionOut(b) ⇓ (ok, σ_2)
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨CleanupLoop(scope, σ, c)⟩ → ⟨CleanupLoop(scope, σ_2, c)⟩

**(Cleanup-Step-Drop-Panic)**
CleanupList(scope) = rest ++ [DropBinding(b)]    σ_1 = SetCleanupList(scope, rest, σ)    Γ ⊢ DropActionOut(b) ⇓ (panic, σ_2)    c = ok
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨CleanupLoop(scope, σ, c)⟩ → ⟨CleanupLoop(scope, σ_2, panic)⟩

**(Cleanup-Step-Drop-Abort)**
CleanupList(scope) = rest ++ [DropBinding(b)]    σ_1 = SetCleanupList(scope, rest, σ)    Γ ⊢ DropActionOut(b) ⇓ (panic, σ_2)    c = panic
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨CleanupLoop(scope, σ, c)⟩ → ⟨Abort⟩

**(Cleanup-Step-DropStatic-Ok)**
CleanupList(scope) = rest ++ [DropStatic(path, name)]    σ_1 = SetCleanupList(scope, rest, σ)    Γ ⊢ DropStaticActionOut(path, name) ⇓ (ok, σ_2)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨CleanupLoop(scope, σ, c)⟩ → ⟨CleanupLoop(scope, σ_2, c)⟩

**(Cleanup-Step-DropStatic-Panic)**
CleanupList(scope) = rest ++ [DropStatic(path, name)]    σ_1 = SetCleanupList(scope, rest, σ)    Γ ⊢ DropStaticActionOut(path, name) ⇓ (panic, σ_2)    c = ok
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨CleanupLoop(scope, σ, c)⟩ → ⟨CleanupLoop(scope, σ_2, panic)⟩

**(Cleanup-Step-DropStatic-Abort)**
CleanupList(scope) = rest ++ [DropStatic(path, name)]    σ_1 = SetCleanupList(scope, rest, σ)    Γ ⊢ DropStaticActionOut(path, name) ⇓ (panic, σ_2)    c = panic
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨CleanupLoop(scope, σ, c)⟩ → ⟨Abort⟩

**(Cleanup-Step-Defer-Ok)**
CleanupList(scope) = rest ++ [DeferBlock(b)]    σ_1 = SetCleanupList(scope, rest, σ)    Γ ⊢ EvalSigma(b, σ_1) ⇓ (Val(v), σ_2)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨CleanupLoop(scope, σ, c)⟩ → ⟨CleanupLoop(scope, σ_2, c)⟩

**(Cleanup-Step-Defer-Panic)**
CleanupList(scope) = rest ++ [DeferBlock(b)]    σ_1 = SetCleanupList(scope, rest, σ)    Γ ⊢ EvalSigma(b, σ_1) ⇓ (Ctrl(Panic), σ_2)    c = ok
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨CleanupLoop(scope, σ, c)⟩ → ⟨CleanupLoop(scope, σ_2, panic)⟩

**(Cleanup-Step-Defer-Abort)**
CleanupList(scope) = rest ++ [DeferBlock(b)]    σ_1 = SetCleanupList(scope, rest, σ)    Γ ⊢ EvalSigma(b, σ_1) ⇓ (Ctrl(Panic), σ_2)    c = panic
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨CleanupLoop(scope, σ, c)⟩ → ⟨Abort⟩

**(Cleanup-Done)**
CleanupList(scope) = []
────────────────────────────────────────────────────────────
⟨CleanupLoop(scope, σ, c)⟩ → ⟨ExitDone(c, σ)⟩

**Destruction (Big-Step).**

**(Destroy-Empty)**
────────────────────────────────────────────
Γ ⊢ Destroy([], σ) ⇓ σ

**(Destroy-Cons)**
Γ ⊢ DropAction(b) ⇓ σ_1    Γ ⊢ Destroy(bs, σ_1) ⇓ σ_2
───────────────────────────────────────────────────────────
Γ ⊢ Destroy(b::bs, σ) ⇓ σ_2

**Cleanup (Big-Step).**

CleanupJudg_Dyn = {Cleanup(cs, σ) ⇓ (c, σ')}

**(Cleanup-Empty)**
────────────────────────────────────────────────────
Γ ⊢ Cleanup([], σ) ⇓ (ok, σ)

**(Cleanup-Cons-Drop)**
Γ ⊢ DropActionOut(b) ⇓ (ok, σ_1)    Γ ⊢ Cleanup(cs, σ_1) ⇓ (c, σ_2)
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ Cleanup(DropBinding(b)::cs, σ) ⇓ (c, σ_2)

**(Cleanup-Cons-Drop-Panic)**
Γ ⊢ DropActionOut(b) ⇓ (panic, σ_1)    Γ ⊢ Cleanup(cs, σ_1) ⇓ (c, σ_2)
────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Cleanup(DropBinding(b)::cs, σ) ⇓ (panic, σ_2)

**(Cleanup-Cons-DropStatic)**
Γ ⊢ DropStaticActionOut(path, name) ⇓ (ok, σ_1)    Γ ⊢ Cleanup(cs, σ_1) ⇓ (c, σ_2)
────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Cleanup(DropStatic(path, name)::cs, σ) ⇓ (c, σ_2)

**(Cleanup-Cons-DropStatic-Panic)**
Γ ⊢ DropStaticActionOut(path, name) ⇓ (panic, σ_1)    Γ ⊢ Cleanup(cs, σ_1) ⇓ (c, σ_2)
────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Cleanup(DropStatic(path, name)::cs, σ) ⇓ (panic, σ_2)

**(Cleanup-Cons-Defer-Ok)**
Γ ⊢ EvalSigma(b, σ) ⇓ (Val(v), σ_1)    Γ ⊢ Cleanup(cs, σ_1) ⇓ (c, σ_2)
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ Cleanup(DeferBlock(b)::cs, σ) ⇓ (c, σ_2)

**(Cleanup-Cons-Defer-Panic)**
Γ ⊢ EvalSigma(b, σ) ⇓ (Ctrl(Panic), σ_1)    Γ ⊢ Cleanup(cs, σ_1) ⇓ (c, σ_2)
────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ Cleanup(DeferBlock(b)::cs, σ) ⇓ (panic, σ_2)

**Cleanup Scope (Big-Step).**

CleanupScopeJudg = {CleanupScope(scope, σ) ⇓ (c, σ')}

**(CleanupScope-From-SmallStep)**
⟨ExitScope(scope, σ)⟩ →* ⟨ExitDone(c, σ')⟩
──────────────────────────────────────────────────────────────
Γ ⊢ CleanupScope(scope, σ) ⇓ (c, σ')

**Unwinding.**

**(Unwind-Step)**
Γ ⊢ CleanupScope(f_1.scope, σ) ⇓ (ok, σ')
──────────────────────────────────────────────────────────────
⟨Unwind(f_1::fs, σ)⟩ → ⟨Unwind(fs, σ')⟩

**(Unwind-Abort)**
Γ ⊢ CleanupScope(f_1.scope, σ) ⇓ (panic, σ')
──────────────────────────────────────────────────────────────
⟨Unwind(f_1::fs, σ)⟩ → ⟨Abort⟩

**Region, Frame, and Allocation Semantics (Cursive0).**

**Dynamic Scope Stack.**

ScopeEntry = ⟨scope_id, cleanup, names, vals, states⟩
ScopeId(⟨sid, cleanup, names, vals, states⟩) = sid
ScopeCleanup(⟨sid, cleanup, names, vals, states⟩) = cleanup
ScopeNames(⟨sid, cleanup, names, vals, states⟩) = names
ScopeVals(⟨sid, cleanup, names, vals, states⟩) = vals
ScopeStates(⟨sid, cleanup, names, vals, states⟩) = states
ScopeStack(σ) ∈ [ScopeEntry]
CurrentScope(σ) = scope ⇔ ScopeStack(σ) = scope :: ss
CurrentScopeId(σ) = ScopeId(CurrentScope(σ))
ScopeEmpty(sid) = ⟨sid, [], ∅, ∅, ∅⟩
FreshScopeId(σ) = sid ⇒ ∀ s ∈ ScopeStack(σ). ScopeId(s) ≠ sid
UpdateScopeStack(σ, ss) = σ' ⇔ ScopeStack(σ') = ss ∧ AddrTags(σ') = AddrTags(σ) ∧ RegionStack(σ') = RegionStack(σ) ∧ PoisonedModules(σ') = PoisonedModules(σ)
PushScope_σ(σ) ⇓ (σ', scope) ⇔ scope = ScopeEmpty(sid) ∧ FreshScopeId(σ) = sid ∧ UpdateScopeStack(σ, scope :: ScopeStack(σ)) = σ'
PopScope_σ(σ) ⇓ (σ', scope) ⇔ ScopeStack(σ) = scope :: ss ∧ UpdateScopeStack(σ, ss) = σ'
AppendCleanup(σ, item) ⇓ σ' ⇔ ScopeStack(σ) = scope :: ss ∧ scope = ⟨sid, cleanup, names, vals, states⟩ ∧ scope' = ⟨sid, cleanup ++ [item], names, vals, states⟩ ∧ UpdateScopeStack(σ, scope' :: ss) = σ'
CleanupList(scope) = ScopeCleanup(scope)
ScopeById([], sid) = ⊥
ScopeById(scope :: ss, sid) =
 scope                 if ScopeId(scope) = sid
 ScopeById(ss, sid)    otherwise
ReplaceScopeById([], sid, scope') = ⊥
ReplaceScopeById(scope :: ss, sid, scope') =
 scope' :: ss                                if ScopeId(scope) = sid
 scope :: ReplaceScopeById(ss, sid, scope')  otherwise
SetCleanupList(scope, xs, σ) ⇓ σ' ⇔ sid = ScopeId(scope) ∧ scope' = ⟨sid, xs, ScopeNames(scope), ScopeVals(scope), ScopeStates(scope)⟩ ∧ ReplaceScopeById(ScopeStack(σ), sid, scope') = ss' ∧ UpdateScopeStack(σ, ss') = σ'

**Poisoned Modules.**

PoisonedModule(σ, path) ⇔ ∃ m. PathOfModule(m) = path ∧ ReadAddr(σ, AddrOfSym(PoisonFlag(m))) ≠ 0
PoisonedModules(σ) = {path | PoisonedModule(σ, path)}

**Dynamic Value Environment.**

Binding = ⟨scope_id, bind_id, name⟩
BindingValue = Value ∪ {Alias(addr) | addr ∈ Addr}
FreshBindId(σ) = b ⇒ ∀ x. ScopeNames(CurrentScope(σ))[x] defined ⇒ b ∉ ScopeNames(CurrentScope(σ))[x]
Last([a]) = a
Last(a::as) = Last(as)    (|as| > 0)
NearestScope([], x) = ⊥
NearestScope(scope :: ss, x) =
 scope                  if ScopeNames(scope)[x] defined
 NearestScope(ss, x)    otherwise
LookupBind(σ, x) = ⟨ScopeId(scope), b, x⟩ ⇔ NearestScope(ScopeStack(σ), x) = scope ∧ b = Last(ScopeNames(scope)[x])
BindingValue(σ, ⟨sid, bind_id, x⟩) = v ⇔ ScopeById(ScopeStack(σ), sid) = scope ∧ ScopeVals(scope)[bind_id] = v
BindState(σ, ⟨sid, bind_id, x⟩) = s ⇔ ScopeById(ScopeStack(σ), sid) = scope ∧ ScopeStates(scope)[bind_id] = s

**(LookupVal-Bind-Value)**
LookupBind(σ, x) = b    BindingValue(σ, b) = v
───────────────────────────────────────────────
LookupVal(σ, x) = v

**(LookupVal-Bind-Alias)**
LookupBind(σ, x) = b    BindingValue(σ, b) = Alias(addr)    ReadAddr(σ, addr) = v
───────────────────────────────────────────────────────────────────────────────────────────
LookupVal(σ, x) = v

**(LookupVal-Path)**
LookupBind(σ, x) undefined    Γ ⊢ ResolveValueName(x) ⇓ ent    ent.origin_opt = mp    name = (ent.target_opt if present, else x)    ¬ PoisonedModule(σ, PathOfModule(mp))    LookupValPath(σ, PathOfModule(mp), name) = v
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
LookupVal(σ, x) = v

**(LookupVal-RecordCtor)**
LookupBind(σ, x) undefined    Γ ⊢ ResolveValueName(x) ⇑    Γ ⊢ ResolveTypeName(x) ⇓ ent    ent.origin_opt = mp    name = (ent.target_opt if present, else x)    ¬ PoisonedModule(σ, PathOfModule(mp))    RecordDecl(FullPath(PathOfModule(mp), name)) = R
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
LookupVal(σ, x) = RecordCtor(FullPath(PathOfModule(mp), name))

**(LookupValPath-Static)**
Γ ⊢ ResolveQualified(path, name, ValueKind) ⇓ ent    ent.origin_opt = mp    path' = PathOfModule(mp)    name' = (ent.target_opt if present, else name)    ¬ PoisonedModule(σ, path')    StaticAddr(path', name') = addr    ReadAddr(σ, addr) = v
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
LookupValPath(σ, path, name) = v

**(LookupValPath-Proc)**
Γ ⊢ ResolveQualified(path, name, ValueKind) ⇓ ent    ent.origin_opt = mp    path' = PathOfModule(mp)    name' = (ent.target_opt if present, else name)    ¬ PoisonedModule(σ, path')    DeclOf(path', name') = proc
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
LookupValPath(σ, path, name) = ProcRef(proc)

**(LookupValPath-RecordCtor)**
Γ ⊢ ResolveQualified(path, name, ValueKind) ⇑    Γ ⊢ ResolveRecordPath(path, name) ⇓ p    SplitLast(p) = (mp, _)    ¬ PoisonedModule(σ, mp)
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
LookupValPath(σ, path, name) = RecordCtor(p)

LookupBind(σ, x) undefined ⇒ IllFormed(LookupBind(σ, x))
LookupVal(σ, x) undefined ⇒ IllFormed(LookupVal(σ, x))

ScopeValsUpdate(⟨sid, cleanup, names, vals, states⟩, bind_id, v) = ⟨sid, cleanup, names, vals[bind_id ↦ v], states⟩
ScopeStatesUpdate(⟨sid, cleanup, names, vals, states⟩, bind_id, s) = ⟨sid, cleanup, names, vals, states[bind_id ↦ s]⟩

UpdateVal(σ, ⟨sid, bind_id, x⟩, v) ⇓ σ' ⇔ (BindingValue(σ, ⟨sid, bind_id, x⟩) = Alias(addr) ∧ WriteAddr(σ, addr, v) ⇓ σ') ∨ (BindingValue(σ, ⟨sid, bind_id, x⟩) ≠ Alias(_) ∧ ScopeById(ScopeStack(σ), sid) = scope ∧ scope' = ScopeValsUpdate(scope, bind_id, v) ∧ ReplaceScopeById(ScopeStack(σ), sid, scope') = ss' ∧ UpdateScopeStack(σ, ss') = σ')
SetState(σ, ⟨sid, bind_id, x⟩, s) ⇓ σ' ⇔ ScopeById(ScopeStack(σ), sid) = scope ∧ scope' = ScopeStatesUpdate(scope, bind_id, s) ∧ ReplaceScopeById(ScopeStack(σ), sid, scope') = ss' ∧ UpdateScopeStack(σ, ss') = σ'

TypeOf(⟨sid, bind_id, x⟩) = TypeOf(x)
BindInfo(⟨sid, bind_id, x⟩) = BindInfo(x)

BindVal(σ, x, v) ⇓ (σ', b) ⇔ ScopeStack(σ) = scope :: ss ∧ scope = ⟨sid, cleanup, names, vals, states⟩ ∧ bind_id = FreshBindId(σ) ∧ names' = names[x ↦ (names[x] if present else []) ++ [bind_id]] ∧ vals' = vals[bind_id ↦ v] ∧ states' = states[bind_id ↦ `Valid`] ∧ scope' = ⟨sid, cleanup, names', vals', states'⟩ ∧ UpdateScopeStack(σ, scope' :: ss) = σ_1 ∧ b = ⟨sid, bind_id, x⟩ ∧ ((BindInfo(b).resp = resp ∧ AppendCleanup(σ_1, DropBinding(b)) ⇓ σ') ∨ (BindInfo(b).resp ≠ resp ∧ σ' = σ_1))

BindPatternVal(p, v) ⇓ B ⇔ Γ ⊢ MatchPattern(p, v) ⇓ B
BindOrder(p, B) = [⟨x, B[x]⟩ | x ∈ PatNames(p)]

**(BindList-Empty)**
────────────────────────────────────────────
BindList(σ, []) ⇓ (σ, [])

**(BindList-Cons)**
BindVal(σ, x, v) ⇓ (σ_1, b)    BindList(σ_1, xs) ⇓ (σ_2, bs)
──────────────────────────────────────────────────────────────────────────────────────
BindList(σ, [⟨x, v⟩] ++ xs) ⇓ (σ_2, b::bs)

BindPattern(σ, p, v) ⇓ (σ', bs) ⇔ BindPatternVal(p, v) ⇓ B ∧ BindOrder(p, B) = binds ∧ BindList(σ, binds) ⇓ (σ', bs)

**Runtime Region Stack.**

RegionEntry = ⟨tag, target, scope, mark_opt⟩
RegionTagOf(⟨tag, target, scope, mark_opt⟩) = tag
RegionTargetOf(⟨tag, target, scope, mark_opt⟩) = target
RegionScopeOf(⟨tag, target, scope, mark_opt⟩) = scope
RegionMarkOf(⟨tag, target, scope, mark_opt⟩) = mark_opt
RuntimeTag = {RegionTag(tag), ScopeTag(sid)}
RegionStack(σ) ∈ [RegionEntry]
AddrTags(σ) : Addr ⇀ RuntimeTag

**Region Values.**

RegionValue(S, h) = RecordValue(ModalStateRef([`Region`], S), [⟨`handle`, IntVal("usize", h)⟩])
RegionHandleOf(v) = h ⇔ v = RecordValue(ModalStateRef([`Region`], S), fs) ∧ ⟨`handle`, IntVal("usize", h)⟩ ∈ fs
RegionHandleOf(v) undefined ⇒ IllFormed(RegionHandleOf(v))

ResolveEntry([], r) = ⊥
ResolveEntry(e::es, r) =
 e                     if RegionTargetOf(e) = r
 ResolveEntry(es, r)   otherwise
ActiveEntry(σ) = e ⇔ RegionStack(σ) = e :: es
ActiveTarget(σ) = target ⇔ ActiveEntry(σ) = e ∧ RegionTargetOf(e) = target
ResolveTarget(σ, r) = target ⇔ ResolveEntry(RegionStack(σ), r) = e ∧ RegionTargetOf(e) = target
ResolveTag(σ, r) = tag ⇔ ResolveEntry(RegionStack(σ), r) = e ∧ RegionTagOf(e) = tag
FreshTag(σ) = tag ⇒ ∀ e ∈ RegionStack(σ). RegionTagOf(e) ≠ tag
FreshArena(σ) = r ⇒ ∀ e ∈ RegionStack(σ). RegionTargetOf(e) ≠ r

ActiveTarget(σ) undefined ⇒ IllFormed(ActiveTarget(σ))
ResolveTarget(σ, r) undefined ⇒ IllFormed(ResolveTarget(σ, r))

UpdateRegionStack(σ, rs) = σ' ⇔ RegionStack(σ') = rs ∧ ScopeStack(σ') = ScopeStack(σ) ∧ AddrTags(σ') = AddrTags(σ) ∧ PoisonedModules(σ') = PoisonedModules(σ)

RegionNew(σ, opts) ⇓ (σ', r, scope) ⇔ PushScope_σ(σ) ⇓ (σ_1, scope) ∧ FreshArena(σ) = r ∧ UpdateRegionStack(σ_1, ⟨r, r, scope, ⊥⟩ :: RegionStack(σ_1)) = σ'

RegionOpen(σ, opts) ⇓ (σ', r) ⇔ FreshArena(σ) = r ∧ UpdateRegionStack(σ, ⟨r, r, CurrentScopeId(σ), ⊥⟩ :: RegionStack(σ)) = σ'

FrameEnter(σ, r) ⇓ (σ', F, scope, mark) ⇔ PushScope_σ(σ) ⇓ (σ_1, scope) ∧ F = FreshTag(σ) ∧ mark = FrameMark(σ_1, r) ∧ UpdateRegionStack(σ_1, ⟨F, r, scope, mark⟩ :: RegionStack(σ_1)) = σ'

BindRegionAlias(σ, ⊥, r) ⇓ σ
BindRegionAlias(σ, x, r) ⇓ σ' ⇔ BindVal(σ, x, RegionValue(`@Active`, r)) ⇓ (σ', b)

DynPayloadAddr(v, addr) ⇔ v = Dyn(Cl, RawPtr(`imm`, addr), T)
RegionAlloc(σ, r, v) ⇓ (σ', v') ⇒ (ResolveTag(σ, r) = tag ∧ ∀ addr. DynPayloadAddr(v', addr) ⇒ AddrTags(σ')(addr) = RegionTag(tag))

FreshTags(σ, tags) ⇔ Distinct(tags) ∧ ∀ tag ∈ Set(tags). ∀ e ∈ RegionStack(σ). RegionTagOf(e) ≠ tag

RetagRegions([], r, tags) = [] ⇔ tags = []
RetagRegions(e::es, r, tags) =
 e' :: RetagRegions(es, r, tags')    if RegionTargetOf(e) = r ∧ tags = tag :: tags' ∧ e' = ⟨tag, RegionTargetOf(e), RegionScopeOf(e), RegionMarkOf(e)⟩
 e :: RetagRegions(es, r, tags)      otherwise

RegionReset(σ, r) ⇓ σ' ⇔ FreshTags(σ, tags) ∧ RetagRegions(RegionStack(σ), r, tags) = rs' ∧ UpdateRegionStack(σ, rs') = σ'

PopRegions([], r) = []
PopRegions(e::es, r) =
 PopRegions(es, r)    if RegionTargetOf(e) = r
 e :: PopRegions(es, r)    otherwise
RegionFree(σ, r) ⇓ σ' ⇔ PopRegions(RegionStack(σ), r) = rs' ∧ UpdateRegionStack(σ, rs') = σ'

**Region Procedures.**

RegionProcJudg = {RegionNewScoped(σ, opts) ⇓ (σ', v), RegionAllocProc(σ, v_r, v) ⇓ (σ', v'), RegionResetProc(σ, v_r) ⇓ (σ', v'), RegionFreezeProc(σ, v_r) ⇓ (σ', v'), RegionThawProc(σ, v_r) ⇓ (σ', v'), RegionFreeProc(σ, v_r) ⇓ (σ', v')}

**(Region-New-Scoped)**
RegionOpen(σ, opts) ⇓ (σ', r)    v = RegionValue(`@Active`, r)
───────────────────────────────────────────────────────────────────────
RegionNewScoped(σ, opts) ⇓ (σ', v)

**(Region-Alloc-Proc)**
RegionHandleOf(v_r) = h    ResolveTarget(σ, h) = r_t    RegionAlloc(σ, r_t, v) ⇓ (σ', v')
─────────────────────────────────────────────────────────────────────────────────────────────────────────
RegionAllocProc(σ, v_r, v) ⇓ (σ', v')

**(Region-Reset-Proc)**
RegionHandleOf(v_r) = h    RegionReset(σ, h) ⇓ σ'    v' = RegionValue(`@Active`, h)
──────────────────────────────────────────────────────────────────────────────────────────────
RegionResetProc(σ, v_r) ⇓ (σ', v')

**(Region-Freeze-Proc)**
RegionHandleOf(v_r) = h    v' = RegionValue(`@Frozen`, h)
───────────────────────────────────────────────────────────────
RegionFreezeProc(σ, v_r) ⇓ (σ, v')

**(Region-Thaw-Proc)**
RegionHandleOf(v_r) = h    v' = RegionValue(`@Active`, h)
──────────────────────────────────────────────────────────────
RegionThawProc(σ, v_r) ⇓ (σ, v')

**(Region-Free-Proc)**
RegionHandleOf(v_r) = h    RegionFree(σ, h) ⇓ σ'    v' = RegionValue(`@Freed`, h)
──────────────────────────────────────────────────────────────────────────────────────────────
RegionFreeProc(σ, v_r) ⇓ (σ', v')

PopRegion([], r) = ⊥
PopRegion(e::es, r) =
 es                    if RegionTargetOf(e) = r
 e :: PopRegion(es, r) otherwise
ReleaseArena(σ, r) ⇓ σ' ⇔ PopRegion(RegionStack(σ), r) = rs' ∧ UpdateRegionStack(σ, rs') = σ'
ResetArena(σ, r, mark) ⇓ σ' ⇔ PopRegion(RegionStack(σ), r) = rs' ∧ UpdateRegionStack(σ, rs') = σ'

FrameMark(σ, r) = |RegionStack(σ)|

**Control Outcomes.**

Ctrl = {Return(v), Result(v), Break(v_opt), Continue, Panic, Abort}
StmtOut = {ok} ∪ {Ctrl(κ) | κ ∈ Ctrl}
Outcome = {Val(v)} ∪ {Ctrl(κ) | κ ∈ Ctrl}
StmtOutOf(Val(v)) = ok
StmtOutOf(Ctrl(κ)) = Ctrl(κ)
BreakVal(⊥) = ()
BreakVal(v) = v

**Block Evaluation Helpers.**

BlockEnter(σ, binds) ⇓ (σ', scope) ⇔ PushScope_σ(σ) ⇓ (σ_1, scope) ∧ ∃ bs. BindList(σ_1, binds) ⇓ (σ', bs)

ExitOutcome(out, ok) = out
ExitOutcome(Ctrl(Abort), c) = Ctrl(Abort)
ExitOutcome(Ctrl(Panic), panic) = Ctrl(Abort)
ExitOutcome(out, panic) = Ctrl(Panic)    (out ≠ Ctrl(Panic) ∧ out ≠ Ctrl(Abort))

BlockExit(σ, scope, out) ⇓ (out', σ') ⇔ Γ ⊢ CleanupScope(scope, σ) ⇓ (c, σ_1) ∧ out' = ExitOutcome(out, c) ∧ ((out' = Ctrl(Abort) ∧ σ' = σ_1) ∨ (out' ≠ Ctrl(Abort) ∧ PopScope_σ(σ_1) ⇓ (σ', scope)))

EvalBlockBodySigma(BlockExpr(stmts, tail_opt), σ) ⇓ (out, σ') ⇔ Γ ⊢ ExecSeqSigma(stmts, σ) ⇓ (sout, σ_1) ∧ (
 (sout = ok ∧ tail_opt = e ∧ Γ ⊢ EvalSigma(e, σ_1) ⇓ (out, σ')) ∨
 (sout = ok ∧ tail_opt = ⊥ ∧ out = Val(()) ∧ σ' = σ_1) ∨
 (sout = Ctrl(Result(v)) ∧ out = Val(v) ∧ σ' = σ_1) ∨
 (sout = Ctrl(κ) ∧ κ ≠ Result(_) ∧ out = Ctrl(κ) ∧ σ' = σ_1)
)

EvalBlockSigma(b, σ) ⇓ (out', σ'') ⇔ BlockEnter(σ, []) ⇓ (σ_1, scope) ∧ EvalBlockBodySigma(b, σ_1) ⇓ (out, σ_2) ∧ BlockExit(σ_2, scope, out) ⇓ (out', σ'')

EvalBlockBindSigma(p, v, b, σ) ⇓ (out', σ'') ⇔ BindPatternVal(p, v) ⇓ B ∧ BindOrder(p, B) = binds ∧ BlockEnter(σ, binds) ⇓ (σ_1, scope) ∧ EvalBlockBodySigma(b, σ_1) ⇓ (out, σ_2) ∧ BlockExit(σ_2, scope, out) ⇓ (out', σ'')

EvalInScopeSigma(b, σ, scope) ⇓ (out, σ') ⇔ CurrentScopeId(σ) = scope ∧ EvalBlockBodySigma(b, σ) ⇓ (out, σ')

**Place Evaluation Helpers.**

**PlaceJudg.**
PlaceJudg = {Γ ⊢ ReadPlaceSigma(p, σ) ⇓ (out, σ'), Γ ⊢ WritePlaceSigma(p, v, σ) ⇓ (sout, σ'), Γ ⊢ WritePlaceSubSigma(p, v, σ) ⇓ (sout, σ'), Γ ⊢ MovePlaceSigma(p, σ) ⇓ (out, σ')}

DropOnAssign(b) ⇔ BindInfo(b).mov = immov ∧ BindInfo(b).resp = resp

DropOnAssignStatic(path, name) ⇔ StaticBindInfo(path, name).mov = immov ∧ StaticBindInfo(path, name).resp = resp

RootBinding(Sigma, p) =
 Local(b)    if LookupBind(Sigma, PlaceRoot(p)) = b
 Static(path, name)    if LookupBind(Sigma, PlaceRoot(p)) undefined ∧ Γ ⊢ ResolveValueName(PlaceRoot(p)) ⇓ ent ∧ ent.origin_opt = mp ∧ name = (ent.target_opt if present, else PlaceRoot(p)) ∧ path = PathOfModule(mp)

DropOnAssignRoot(Sigma, p) ⇔ (RootBinding(Sigma, p) = Local(b) ∧ DropOnAssign(b)) ∨ (RootBinding(Sigma, p) = Static(path, name) ∧ DropOnAssignStatic(path, name))

RootMoved(Sigma, p) ⇔ RootBinding(Sigma, p) = Local(b) ∧ BindState(Sigma, b) = Moved

**DropSubvalueJudg.**
DropSubvalueJudg = {Γ ⊢ DropSubvalue(p, T, v, σ) ⇓ σ'}

**(DropSubvalue-Do)**
DropOnAssignRoot(Sigma, p)    ¬ RootMoved(Sigma, p)    Γ ⊢ DropValue(T, v, ∅) ⇓ Sigma'
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ DropSubvalue(p, T, v, Sigma) ⇓ Sigma'

**(DropSubvalue-Skip)**
¬ DropOnAssignRoot(Sigma, p) ∨ RootMoved(Sigma, p)
────────────────────────────────────────────────────
Γ ⊢ DropSubvalue(p, T, v, Sigma) ⇓ Sigma


**(ReadPlace-Ident)**
LookupVal(σ, x) = v
────────────────────────────────────────────────────────────────────────────
Γ ⊢ ReadPlaceSigma(Identifier(x), σ) ⇓ (Val(v), σ)

**(ReadPlace-Ident-Poison)**
LookupBind(σ, x) undefined    Γ ⊢ ResolveValueName(x) ⇓ ent    ent.origin_opt = mp    PoisonedModule(σ, PathOfModule(mp))
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ReadPlaceSigma(Identifier(x), σ) ⇓ (Ctrl(Panic), σ)

**(WritePlace-Ident)**
LookupBind(σ, x) = b    (DropOnAssign(b) ⇒ Γ ⊢ DropAction(b) ⇓ σ_1)    (¬ DropOnAssign(b) ⇒ σ_1 = σ)    UpdateVal(σ_1, b, v) ⇓ σ_2    SetState(σ_2, b, `Valid`) ⇓ σ_3
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ WritePlaceSigma(Identifier(x), v, σ) ⇓ (ok, σ_3)

**(WritePlace-Ident-Path-Poison)**
Γ ⊢ ResolveValueName(x) ⇓ ent    ent.origin_opt = mp    PoisonedModule(σ, PathOfModule(mp))
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ WritePlaceSigma(Identifier(x), v, σ) ⇓ (Ctrl(Panic), σ)

**(WritePlace-Ident-Path)**
Γ ⊢ ResolveValueName(x) ⇓ ent    ent.origin_opt = mp    name = (ent.target_opt if present, else x)    PathOfModule(mp) = path    StaticAddr(path, name) = addr    (DropOnAssignStatic(path, name) ⇒ Γ ⊢ DropStaticAction(path, name) ⇓ σ_1)    (¬ DropOnAssignStatic(path, name) ⇒ σ_1 = σ)    WriteAddr(σ_1, addr, v) ⇓ σ'
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ WritePlaceSigma(Identifier(x), v, σ) ⇓ (ok, σ')

TupleValue((v_0, …, v_{n-1}), i) = v_i    (0 ≤ i < n)
TupleUpdate((v_0, …, v_{n-1}), i, v') = (v_0, …, v_{i-1}, v', v_{i+1}, …, v_{n-1})    (0 ≤ i < n)
FieldValue(RecordValue(tr, fs), f) = v ⇔ ⟨f, v⟩ ∈ fs
FieldUpdate(RecordValue(tr, fs), f, v') = RecordValue(tr, fs')    where fs' = [⟨f_i, v_i'⟩ | ⟨f_i, v_i⟩ ∈ fs ∧ v_i' = v' if f_i = f otherwise v_i]

Len([v_0, …, v_{n-1}]) = n
Len(SliceValue(v, r)) = end - start    (SliceBounds(r, Len(v)) = (start, end))
IndexNum(v) = i ⇔ v = IntVal("usize", i)
IndexValue([v_0, …, v_{n-1}], i) = v_i    (0 ≤ i < n)
IndexValue(SliceValue(v, r), i) = IndexValue(v, start + i)    (SliceBounds(r, Len(v)) = (start, end) ∧ 0 ≤ i < end - start)
IndexValue(v, v_i) = v_e ⇔ IndexNum(v_i) = i ∧ IndexValue(v, i) = v_e
IndexUpdate([v_0, …, v_{n-1}], i, v_e) = [v_0, …, v_{i-1}, v_e, v_{i+1}, …, v_{n-1}]    (0 ≤ i < n)
IndexUpdate(SliceValue(v_b, r), i, v_e) = SliceValue(v_b', r)    (SliceBounds(r, Len(v_b)) = (start, end) ∧ 0 ≤ i < end - start ∧ IndexUpdate(v_b, start + i, v_e) = v_b')
IndexUpdate(v, v_i, v_e) = v' ⇔ IndexNum(v_i) = i ∧ IndexUpdate(v, i, v_e) = v'
SliceValue(v, r) defined ⇔ SliceBounds(r, Len(v)) defined
SliceLen([v_0, …, v_{n-1}]) = n
SliceLen(SliceValue(v, r)) = end - start    (SliceBounds(r, Len(v)) = (start, end))
SliceElem(v, i) = IndexValue(v, i)    (IndexValue(v, i) defined)
SliceUpdate(v, start, v_rhs) ⇓ v' ⇔ n = SliceLen(v_rhs) ∧ ∃ v_0, …, v_n. v_0 = v ∧ ∀ i ∈ [0, n-1]. v_{i+1} = IndexUpdate(v_i, start + i, SliceElem(v_rhs, i)) ∧ v' = v_n

**PtrAddrJudg.**
PtrAddrJudg = {Γ ⊢ ReadPtrSigma(v_ptr, σ) ⇓ (out, σ'), Γ ⊢ WritePtrSigma(v_ptr, v, σ) ⇓ (sout, σ'), Γ ⊢ AddrOfSigma(p, σ) ⇓ (out, σ')}

AddrPrimJudg = {ReadAddr(σ, addr) = v, WriteAddr(σ, addr, v) ⇓ σ', FieldAddr(T, addr, f) = addr', TupleAddr(T, addr, i) = addr', IndexAddr(T_b, addr, i) = addr'}
IndexLen(Sigma, addr) = Len(v)    (ReadAddr(Sigma, addr) = v ∧ Len(v) defined)
IndexAddr(T_b, addr, i) = AddrAdd(addr, i × sizeof(ElemType(T_b)))    (ElemType(T_b) defined)
IndexAddr(T_b, addr, v_i) = addr' ⇔ IndexNum(v_i) = i ∧ IndexAddr(T_b, addr, i) = addr'
SliceLenFromAddr(σ, addr) = n ⇔ ReadAddr(σ, addr) = v ∧ SliceLen(v) = n

PtrStateSet = {`Valid`, `Null`, `Expired`}
RawQual = {`imm`, `mut`}
PtrAddr(Ptr@Valid(addr)) = addr
PtrAddr(Ptr@Null(addr)) = addr
PtrAddr(Ptr@Expired(addr)) = addr
PtrAddr(RawPtr(q, addr)) = addr

AddrTag(Sigma, addr) =
 ScopeTag(sid)    if addr = BindAddr(⟨sid, bind_id, x⟩)
 RegionTag(tag)   if AddrTags(Sigma)(addr) = RegionTag(tag)
 ⊥                otherwise
TagActive(Sigma, RegionTag(tag)) ⇔ ∃ e ∈ RegionStack(Sigma). RegionTagOf(e) = tag
TagActive(Sigma, ScopeTag(sid)) ⇔ ∃ e ∈ ScopeStack(Sigma). ScopeId(e) = sid
PtrState(Sigma, Ptr@Null(_)) = `Null`
PtrState(Sigma, Ptr@Expired(_)) = `Expired`
PtrState(Sigma, Ptr@Valid(addr)) = `Valid`
DynAddrState(Sigma, addr) =
 `Valid`    if AddrTag(Sigma, addr) = ⊥
 `Valid`    if AddrTag(Sigma, addr) = tag ≠ ⊥ ∧ TagActive(Sigma, tag)
 `Expired`  if AddrTag(Sigma, addr) = tag ≠ ⊥ ∧ ¬ TagActive(Sigma, tag)
BindAddr(⟨sid, bind_id, x⟩) ∈ Addr
AddrOfBind(b) =
 addr         if BindingValue(Sigma, b) = Alias(addr)
 BindAddr(b)  if BindingValue(Sigma, b) ≠ Alias(_)
AddrOfBind(x) = addr ⇔ ∃ b. LookupBind(σ, x) = b ∧ AddrOfBind(b) = addr

**(ReadPtr-Safe)**
PtrState(σ, v_ptr) = `Valid`    PtrAddr(v_ptr) = addr    ReadAddr(σ, addr) = v
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ ReadPtrSigma(v_ptr, σ) ⇓ (Val(v), σ)

**(WritePtr-Safe)**
PtrState(σ, v_ptr) = `Valid`    PtrAddr(v_ptr) = addr    WriteAddr(σ, addr, v) ⇓ σ'
────────────────────────────────────────────────────────────────────────────────
Γ ⊢ WritePtrSigma(v_ptr, v, σ) ⇓ (ok, σ')

**(ReadPtr-Null)**
PtrState(σ, v_ptr) = `Null`
──────────────────────────────────────────────────────────────
Γ ⊢ ReadPtrSigma(v_ptr, σ) ⇓ (Ctrl(Panic), σ)

**(ReadPtr-Expired)**
PtrState(σ, v_ptr) = `Expired`
────────────────────────────────────────────────────────────────
Γ ⊢ ReadPtrSigma(v_ptr, σ) ⇓ (Ctrl(Panic), σ)

**(WritePtr-Null)**
PtrState(σ, v_ptr) = `Null`
──────────────────────────────────────────────────────────────
Γ ⊢ WritePtrSigma(v_ptr, v, σ) ⇓ (Ctrl(Panic), σ)

**(WritePtr-Expired)**
PtrState(σ, v_ptr) = `Expired`
────────────────────────────────────────────────────────────────
Γ ⊢ WritePtrSigma(v_ptr, v, σ) ⇓ (Ctrl(Panic), σ)

**(ReadPtr-Raw)**
v_ptr = RawPtr(q, addr)    ReadAddr(σ, addr) = v
──────────────────────────────────────────────────────────────
Γ ⊢ ReadPtrSigma(v_ptr, σ) ⇓ (Val(v), σ)

**(WritePtr-Raw)**
v_ptr = RawPtr(`mut`, addr)    WriteAddr(σ, addr, v) ⇓ σ'
───────────────────────────────────────────────────────────────
Γ ⊢ WritePtrSigma(v_ptr, v, σ) ⇓ (ok, σ')

ReadPtrSigma(RawPtr(q, addr), σ) undefined ⇔ ReadAddr(σ, addr) undefined
WritePtrSigma(RawPtr(`imm`, addr), v, σ) undefined
DynamicUndefined(ReadPtrSigma(RawPtr(q, addr), σ)) ⇔ ReadPtrSigma(RawPtr(q, addr), σ) undefined
DynamicUndefined(WritePtrSigma(RawPtr(`imm`, addr), v, σ))


**(AddrOf-Ident)**
LookupBind(σ, x) = b    AddrOfBind(b) = addr
────────────────────────────────────────────────────────────────────────────
Γ ⊢ AddrOfSigma(Identifier(x), σ) ⇓ (Val(addr), σ)

**(AddrOf-Ident-Path-Poison)**
Γ ⊢ ResolveValueName(x) ⇓ ent    ent.origin_opt = mp    PoisonedModule(σ, PathOfModule(mp))
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ AddrOfSigma(Identifier(x), σ) ⇓ (Ctrl(Panic), σ)

**(AddrOf-Ident-Path)**
Γ ⊢ ResolveValueName(x) ⇓ ent    ent.origin_opt = mp    name = (ent.target_opt if present, else x)    PathOfModule(mp) = path    StaticAddr(path, name) = addr
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ AddrOfSigma(Identifier(x), σ) ⇓ (Val(addr), σ)

**(AddrOf-Field)**
Γ ⊢ AddrOfSigma(p, σ) ⇓ (Val(addr), σ_1)    T_b = ExprType(p)    FieldAddr(T_b, addr, f) = addr'
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ AddrOfSigma(FieldAccess(p, f), σ) ⇓ (Val(addr'), σ_1)

**(AddrOf-Field-Ctrl)**
Γ ⊢ AddrOfSigma(p, σ) ⇓ (Ctrl(κ), σ_1)
──────────────────────────────────────────────────────────────
Γ ⊢ AddrOfSigma(FieldAccess(p, f), σ) ⇓ (Ctrl(κ), σ_1)

**(AddrOf-Tuple)**
Γ ⊢ AddrOfSigma(p, σ) ⇓ (Val(addr), σ_1)    T_b = ExprType(p)    TupleAddr(T_b, addr, i) = addr'
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ AddrOfSigma(TupleAccess(p, i), σ) ⇓ (Val(addr'), σ_1)

**(AddrOf-Tuple-Ctrl)**
Γ ⊢ AddrOfSigma(p, σ) ⇓ (Ctrl(κ), σ_1)
──────────────────────────────────────────────────────────────
Γ ⊢ AddrOfSigma(TupleAccess(p, i), σ) ⇓ (Ctrl(κ), σ_1)

**(AddrOf-Index)**
**(AddrOfSigma-Index-Ok)**
Γ ⊢ AddrOfSigma(p, σ) ⇓ (Val(addr), σ_1)    Γ ⊢ EvalSigma(i, σ_1) ⇓ (Val(v_i), σ_2)    IndexLen(σ_2, addr) = L    Γ ⊢ CheckIndex(L, v_i) ⇓ ok    T_b = ExprType(p)    IndexAddr(T_b, addr, v_i) = addr'
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ AddrOfSigma(IndexAccess(p, i), σ) ⇓ (Val(addr'), σ_2)

**(AddrOfSigma-Index-OOB)**
Γ ⊢ AddrOfSigma(p, σ) ⇓ (Val(addr), σ_1)    Γ ⊢ EvalSigma(i, σ_1) ⇓ (Val(v_i), σ_2)    ¬ (0 ≤ v_i < IndexLen(σ_2, addr))
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ AddrOfSigma(IndexAccess(p, i), σ) ⇓ (Ctrl(Panic), σ_2)

**(AddrOf-Index-Ctrl-Base)**
Γ ⊢ AddrOfSigma(p, σ) ⇓ (Ctrl(κ), σ_1)
──────────────────────────────────────────────────────────────
Γ ⊢ AddrOfSigma(IndexAccess(p, i), σ) ⇓ (Ctrl(κ), σ_1)

**(AddrOf-Index-Ctrl-Idx)**
Γ ⊢ AddrOfSigma(p, σ) ⇓ (Val(addr), σ_1)    Γ ⊢ EvalSigma(i, σ_1) ⇓ (Ctrl(κ), σ_2)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ AddrOfSigma(IndexAccess(p, i), σ) ⇓ (Ctrl(κ), σ_2)

**(AddrOf-Deref-Safe)**
Γ ⊢ ReadPlaceSigma(p, σ) ⇓ (Val(v_ptr), σ_1)    PtrState(σ_1, v_ptr) = `Valid`    PtrAddr(v_ptr) = addr
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ AddrOfSigma(Deref(p), σ) ⇓ (Val(addr), σ_1)

**(AddrOf-Deref-Null)**
Γ ⊢ ReadPlaceSigma(p, σ) ⇓ (Val(v_ptr), σ_1)    PtrState(σ_1, v_ptr) = `Null`
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ AddrOfSigma(Deref(p), σ) ⇓ (Ctrl(Panic), σ_1)

**(AddrOf-Deref-Expired)**
Γ ⊢ ReadPlaceSigma(p, σ) ⇓ (Val(v_ptr), σ_1)    PtrState(σ_1, v_ptr) = `Expired`
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ AddrOfSigma(Deref(p), σ) ⇓ (Ctrl(Panic), σ_1)

**(AddrOf-Deref-Raw)**
Γ ⊢ ReadPlaceSigma(p, σ) ⇓ (Val(RawPtr(q, addr)), σ_1)
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ AddrOfSigma(Deref(p), σ) ⇓ (Val(addr), σ_1)

**(AddrOf-Deref-Ctrl)**
Γ ⊢ ReadPlaceSigma(p, σ) ⇓ (Ctrl(κ), σ_1)
──────────────────────────────────────────────────────────────
Γ ⊢ AddrOfSigma(Deref(p), σ) ⇓ (Ctrl(κ), σ_1)

**(ReadPlace-Field)**
Γ ⊢ ReadPlaceSigma(p, σ) ⇓ (Val(v_p), σ_1)    FieldValue(v_p, f) = v_f
─────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ReadPlaceSigma(FieldAccess(p, f), σ) ⇓ (Val(v_f), σ_1)

**(ReadPlace-Tuple)**
Γ ⊢ ReadPlaceSigma(p, σ) ⇓ (Val(v_p), σ_1)    TupleValue(v_p, i) = v_i
─────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ReadPlaceSigma(TupleAccess(p, i), σ) ⇓ (Val(v_i), σ_1)

**(ReadPlace-Index)**
Γ ⊢ ReadPlaceSigma(p, σ) ⇓ (Val(v_p), σ_1)    Γ ⊢ EvalSigma(i, σ_1) ⇓ (Val(v_i), σ_2)    IndexValue(v_p, v_i) = v_e
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ReadPlaceSigma(IndexAccess(p, i), σ) ⇓ (Val(v_e), σ_2)

**(ReadPlace-Index-OOB)**
Γ ⊢ ReadPlaceSigma(p, σ) ⇓ (Val(v_p), σ_1)    Γ ⊢ EvalSigma(i, σ_1) ⇓ (Val(v_i), σ_2)    ¬ (0 ≤ v_i < Len(v_p))
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ReadPlaceSigma(IndexAccess(p, i), σ) ⇓ (Ctrl(Panic), σ_2)

**(ReadPlace-Index-Range)**
Γ ⊢ ReadPlaceSigma(p, σ) ⇓ (Val(v_p), σ_1)    Γ ⊢ EvalSigma(i, σ_1) ⇓ (Val(v_r), σ_2)    SliceValue(v_p, v_r) = v_s
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ReadPlaceSigma(IndexAccess(p, i), σ) ⇓ (Val(v_s), σ_2)

**(ReadPlace-Index-Range-OOB)**
Γ ⊢ ReadPlaceSigma(p, σ) ⇓ (Val(v_p), σ_1)    Γ ⊢ EvalSigma(i, σ_1) ⇓ (Val(v_r), σ_2)    SliceBounds(v_r, Len(v_p)) = ⊥
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ReadPlaceSigma(IndexAccess(p, i), σ) ⇓ (Ctrl(Panic), σ_2)

**(ReadPlace-Deref)**
Γ ⊢ ReadPlaceSigma(p, σ) ⇓ (Val(v_ptr), σ_1)    Γ ⊢ ReadPtrSigma(v_ptr, σ_1) ⇓ (out, σ_2)
───────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ReadPlaceSigma(Deref(p), σ) ⇓ (out, σ_2)

**(WritePlace-Field)**
Γ ⊢ ReadPlaceSigma(p, σ) ⇓ (Val(v_p), σ_1)    FieldValue(v_p, f) = v_f    T_f = ExprType(FieldAccess(p, f))    Γ ⊢ DropSubvalue(p, T_f, v_f, σ_1) ⇓ σ_1'    FieldUpdate(v_p, f, v) = v_p'    Γ ⊢ WritePlaceSubSigma(p, v_p', σ_1') ⇓ (sout, σ_2)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ WritePlaceSigma(FieldAccess(p, f), v, σ) ⇓ (sout, σ_2)

**(WritePlace-Tuple)**
Γ ⊢ ReadPlaceSigma(p, σ) ⇓ (Val(v_p), σ_1)    TupleValue(v_p, i) = v_i    T_i = ExprType(TupleAccess(p, i))    Γ ⊢ DropSubvalue(p, T_i, v_i, σ_1) ⇓ σ_1'    TupleUpdate(v_p, i, v) = v_p'    Γ ⊢ WritePlaceSubSigma(p, v_p', σ_1') ⇓ (sout, σ_2)
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ WritePlaceSigma(TupleAccess(p, i), v, σ) ⇓ (sout, σ_2)

**(WritePlace-Index)**
Γ ⊢ ReadPlaceSigma(p, σ) ⇓ (Val(v_p), σ_1)    Γ ⊢ EvalSigma(i, σ_1) ⇓ (Val(v_i), σ_2)    IndexValue(v_p, v_i) = v_e    T_e = ExprType(IndexAccess(p, i))    Γ ⊢ DropSubvalue(p, T_e, v_e, σ_2) ⇓ σ_2'    IndexUpdate(v_p, v_i, v) = v_p'    Γ ⊢ WritePlaceSubSigma(p, v_p', σ_2') ⇓ (sout, σ_3)
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ WritePlaceSigma(IndexAccess(p, i), v, σ) ⇓ (sout, σ_3)

**(WritePlace-Index-OOB)**
Γ ⊢ ReadPlaceSigma(p, σ) ⇓ (Val(v_p), σ_1)    Γ ⊢ EvalSigma(i, σ_1) ⇓ (Val(v_i), σ_2)    ¬ (0 ≤ v_i < Len(v_p))
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ WritePlaceSigma(IndexAccess(p, i), v, σ) ⇓ (Ctrl(Panic), σ_2)

**(WritePlace-Index-Range)**
Γ ⊢ ReadPlaceSigma(p, σ) ⇓ (Val(v_p), σ_1)    Γ ⊢ EvalSigma(i, σ_1) ⇓ (Val(v_r), σ_2)    SliceBounds(v_r, Len(v_p)) = (start, end)    SliceLen(v) = n    n = end - start    SliceUpdate(v_p, start, v) ⇓ v_p'    Γ ⊢ WritePlaceSubSigma(p, v_p', σ_2) ⇓ (sout, σ_3)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ WritePlaceSigma(IndexAccess(p, i), v, σ) ⇓ (sout, σ_3)

**(WritePlace-Index-Range-OOB)**
Γ ⊢ ReadPlaceSigma(p, σ) ⇓ (Val(v_p), σ_1)    Γ ⊢ EvalSigma(i, σ_1) ⇓ (Val(v_r), σ_2)    SliceBounds(v_r, Len(v_p)) = ⊥
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ WritePlaceSigma(IndexAccess(p, i), v, σ) ⇓ (Ctrl(Panic), σ_2)

**(WritePlace-Index-Range-Len)**
Γ ⊢ ReadPlaceSigma(p, σ) ⇓ (Val(v_p), σ_1)    Γ ⊢ EvalSigma(i, σ_1) ⇓ (Val(v_r), σ_2)    SliceBounds(v_r, Len(v_p)) = (start, end)    SliceLen(v) = n    n ≠ end - start
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ WritePlaceSigma(IndexAccess(p, i), v, σ) ⇓ (Ctrl(Panic), σ_2)

**(WritePlace-Deref)**
Γ ⊢ ReadPlaceSigma(p, σ) ⇓ (Val(v_ptr), σ_1)    Γ ⊢ WritePtrSigma(v_ptr, v, σ_1) ⇓ (sout, σ_2)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ WritePlaceSigma(Deref(p), v, σ) ⇓ (sout, σ_2)

**(WriteSub-Ident)**
LookupBind(σ, x) = b    UpdateVal(σ, b, v) ⇓ σ'
──────────────────────────────────────────────────────────────
Γ ⊢ WritePlaceSubSigma(Identifier(x), v, σ) ⇓ (ok, σ')

**(WriteSub-Ident-Path-Poison)**
Γ ⊢ ResolveValueName(x) ⇓ ent    ent.origin_opt = mp    PoisonedModule(σ, PathOfModule(mp))
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ WritePlaceSubSigma(Identifier(x), v, σ) ⇓ (Ctrl(Panic), σ)

**(WriteSub-Ident-Path)**
Γ ⊢ ResolveValueName(x) ⇓ ent    ent.origin_opt = mp    name = (ent.target_opt if present, else x)    PathOfModule(mp) = path    StaticAddr(path, name) = addr    WriteAddr(σ, addr, v) ⇓ σ'
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ WritePlaceSubSigma(Identifier(x), v, σ) ⇓ (ok, σ')

**(WriteSub-Field)**
Γ ⊢ ReadPlaceSigma(p, σ) ⇓ (Val(v_p), σ_1)    FieldUpdate(v_p, f, v) = v_p'    Γ ⊢ WritePlaceSubSigma(p, v_p', σ_1) ⇓ (sout, σ_2)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ WritePlaceSubSigma(FieldAccess(p, f), v, σ) ⇓ (sout, σ_2)

**(WriteSub-Tuple)**
Γ ⊢ ReadPlaceSigma(p, σ) ⇓ (Val(v_p), σ_1)    TupleUpdate(v_p, i, v) = v_p'    Γ ⊢ WritePlaceSubSigma(p, v_p', σ_1) ⇓ (sout, σ_2)
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ WritePlaceSubSigma(TupleAccess(p, i), v, σ) ⇓ (sout, σ_2)

**(WriteSub-Index)**
Γ ⊢ ReadPlaceSigma(p, σ) ⇓ (Val(v_p), σ_1)    Γ ⊢ EvalSigma(i, σ_1) ⇓ (Val(v_i), σ_2)    IndexUpdate(v_p, v_i, v) = v_p'    Γ ⊢ WritePlaceSubSigma(p, v_p', σ_2) ⇓ (sout, σ_3)
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ WritePlaceSubSigma(IndexAccess(p, i), v, σ) ⇓ (sout, σ_3)

**(WriteSub-Index-OOB)**
Γ ⊢ ReadPlaceSigma(p, σ) ⇓ (Val(v_p), σ_1)    Γ ⊢ EvalSigma(i, σ_1) ⇓ (Val(v_i), σ_2)    ¬ (0 ≤ v_i < Len(v_p))
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ WritePlaceSubSigma(IndexAccess(p, i), v, σ) ⇓ (Ctrl(Panic), σ_2)

**(WriteSub-Index-Range)**
Γ ⊢ ReadPlaceSigma(p, σ) ⇓ (Val(v_p), σ_1)    Γ ⊢ EvalSigma(i, σ_1) ⇓ (Val(v_r), σ_2)    SliceBounds(v_r, Len(v_p)) = (start, end)    SliceLen(v) = n    n = end - start    SliceUpdate(v_p, start, v) ⇓ v_p'    Γ ⊢ WritePlaceSubSigma(p, v_p', σ_2) ⇓ (sout, σ_3)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ WritePlaceSubSigma(IndexAccess(p, i), v, σ) ⇓ (sout, σ_3)

**(WriteSub-Index-Range-OOB)**
Γ ⊢ ReadPlaceSigma(p, σ) ⇓ (Val(v_p), σ_1)    Γ ⊢ EvalSigma(i, σ_1) ⇓ (Val(v_r), σ_2)    SliceBounds(v_r, Len(v_p)) = ⊥
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ WritePlaceSubSigma(IndexAccess(p, i), v, σ) ⇓ (Ctrl(Panic), σ_2)

**(WriteSub-Index-Range-Len)**
Γ ⊢ ReadPlaceSigma(p, σ) ⇓ (Val(v_p), σ_1)    Γ ⊢ EvalSigma(i, σ_1) ⇓ (Val(v_r), σ_2)    SliceBounds(v_r, Len(v_p)) = (start, end)    SliceLen(v) = n    n ≠ end - start
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ WritePlaceSubSigma(IndexAccess(p, i), v, σ) ⇓ (Ctrl(Panic), σ_2)

**(WriteSub-Deref)**
Γ ⊢ ReadPlaceSigma(p, σ) ⇓ (Val(v_ptr), σ_1)    Γ ⊢ WritePtrSigma(v_ptr, v, σ_1) ⇓ (sout, σ_2)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ WritePlaceSubSigma(Deref(p), v, σ) ⇓ (sout, σ_2)

**(MovePlace-Whole)**
Γ ⊢ ReadPlaceSigma(p, σ) ⇓ (Val(v), σ_1)    FieldHead(p) = ⊥    LookupBind(σ_1, PlaceRoot(p)) = b    BindInfo(b).mov = mov    SetState(σ_1, b, Moved) ⇓ σ_2
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ MovePlaceSigma(p, σ) ⇓ (Val(v), σ_2)

**(MovePlace-Field)**
Γ ⊢ ReadPlaceSigma(p, σ) ⇓ (Val(v), σ_1)    FieldHead(p) = f    LookupBind(σ_1, PlaceRoot(p)) = b    BindInfo(b).mov = mov    BindState(σ_1, b) = s    PM(s, f) = s'    SetState(σ_1, b, s') ⇓ σ_2
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ MovePlaceSigma(p, σ) ⇓ (Val(v), σ_2)

**(MovePlace-Ctrl)**
Γ ⊢ ReadPlaceSigma(p, σ) ⇓ (Ctrl(κ), σ_1)
──────────────────────────────────────────────────────────────
Γ ⊢ MovePlaceSigma(p, σ) ⇓ (Ctrl(κ), σ_1)


**Stateful Expression Evaluation.**

**EvalJudg.**
EvalJudg = {Γ ⊢ EvalSigma(e, σ) ⇓ (out, σ')}

**Range Helpers.**

RangeVal = {RangeVal(k, lo_opt, hi_opt) | k ∈ RangeKind}
Inc(v) = v' ⇔ v = IntVal(t, x) ∧ x' = x + 1 ∧ InRange(x', t) ∧ v' = IntVal(t, x')
SliceBoundsRaw(RangeVal(Exclusive, s, e), L) = (start, end) ⇔ IndexNum(s) = start ∧ IndexNum(e) = end
SliceBoundsRaw(RangeVal(Inclusive, s, e), L) = (start, end) ⇔ IndexNum(s) = start ∧ Inc(e) = e' ∧ IndexNum(e') = end
SliceBoundsRaw(RangeVal(From, s, ⊥), L) = (start, L) ⇔ IndexNum(s) = start
SliceBoundsRaw(RangeVal(To, ⊥, e), L) = (0, end) ⇔ IndexNum(e) = end
SliceBoundsRaw(RangeVal(ToInclusive, ⊥, e), L) = (0, end) ⇔ Inc(e) = e' ∧ IndexNum(e') = end
SliceBoundsRaw(RangeVal(Full, ⊥, ⊥), L) = (0, L)
SliceBounds(r, L) = (start, end) ⇔ SliceBoundsRaw(r, L) = (start, end) ∧ 0 ≤ start ≤ end ≤ L
SliceBounds(r, L) = ⊥ ⇔ SliceBoundsRaw(r, L) = ⊥ ∨ (∃ start, end. SliceBoundsRaw(r, L) = (start, end) ∧ ¬ (0 ≤ start ≤ end ≤ L))

**EvalOptJudg.**
EvalOptJudg = {Γ ⊢ EvalOptSigma(e_opt, σ) ⇓ (out, σ')}

**(EvalOptSigma-None)**
───────────────────────────────────────────────────────────────
Γ ⊢ EvalOptSigma(⊥, σ) ⇓ (Val(⊥), σ)

**(EvalOptSigma-Some)**
Γ ⊢ EvalSigma(e, σ) ⇓ (Val(v), σ_1)
──────────────────────────────────────────────────────────────
Γ ⊢ EvalOptSigma(e, σ) ⇓ (Val(v), σ_1)

**(EvalOptSigma-Ctrl)**
Γ ⊢ EvalSigma(e, σ) ⇓ (Ctrl(κ), σ_1)
───────────────────────────────────────────────────────────────
Γ ⊢ EvalOptSigma(e, σ) ⇓ (Ctrl(κ), σ_1)

**(EvalSigma-Range)**
Γ ⊢ EvalOptSigma(lo_opt, σ_0) ⇓ (Val(v_lo), σ_1)    Γ ⊢ EvalOptSigma(hi_opt, σ_1) ⇓ (Val(v_hi), σ_2)    r = RangeVal(kind, v_lo, v_hi)
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(Range(kind, lo_opt, hi_opt), σ_0) ⇓ (Val(r), σ_2)

**(EvalSigma-Range-Ctrl)**
Γ ⊢ EvalOptSigma(lo_opt, σ_0) ⇓ (Ctrl(κ), σ_1)
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(Range(kind, lo_opt, hi_opt), σ_0) ⇓ (Ctrl(κ), σ_1)

**(EvalSigma-Range-Ctrl-Hi)**
Γ ⊢ EvalOptSigma(lo_opt, σ_0) ⇓ (Val(v_lo), σ_1)    Γ ⊢ EvalOptSigma(hi_opt, σ_1) ⇓ (Ctrl(κ), σ_2)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(Range(kind, lo_opt, hi_opt), σ_0) ⇓ (Ctrl(κ), σ_2)

**Value Constructors and Accessors.**

**Values.**
BoolVal(b) = b ⇔ b ∈ {true, false}
CharVal(u) = u ⇔ u ∈ UnicodeScalar
UnitVal = ()
IntVal(t, x) defined ⇔ t ∈ IntTypes ∧ InRange(x, t)
IntValType(IntVal(t, x)) = t
IntValValue(IntVal(t, x)) = x
FloatVal(t, v) defined ⇔ t ∈ FloatTypes ∧ v ∈ FloatValueSet(t)
FloatValType(FloatVal(t, v)) = t
FloatValValue(FloatVal(t, v)) = v
PtrVal(s, addr) defined ⇔ s ∈ PtrStateSet
Ptr@Valid(addr) = PtrVal(`Valid`, addr)
Ptr@Null(addr) = PtrVal(`Null`, addr)
Ptr@Expired(addr) = PtrVal(`Expired`, addr)
TupleVal = {(v_1, …, v_n) | n ≥ 0}
ArrayVal = {[v_1, …, v_n] | n ≥ 0}
ModalVal(S, v) = ⟨S, v⟩
Value = {BoolVal(b) | b ∈ {true, false}} ∪ {CharVal(u) | u ∈ UnicodeScalar} ∪ {UnitVal} ∪ {IntVal(t, x) | IntVal(t, x) defined} ∪ {FloatVal(t, v) | FloatVal(t, v) defined} ∪ {PtrVal(s, addr) | PtrVal(s, addr) defined} ∪ {RawPtr(q, addr)} ∪ TupleVal ∪ ArrayVal ∪ {RecordValue(tr, fs)} ∪ {EnumValue(path, payload)} ∪ RangeVal ∪ {SliceValue(v, r) | SliceValue(v, r) defined} ∪ {ModalVal(S, v)} ∪ {Dyn(Cl, RawPtr(`imm`, addr), T)} ∪ `string@Managed` ∪ `string@View` ∪ `bytes@Managed` ∪ `bytes@View`

**EvalListJudg.**
EvalListJudg = {Γ ⊢ EvalListSigma(es, σ) ⇓ (out, σ'), Γ ⊢ EvalFieldInitsSigma(fields, σ) ⇓ (out, σ')}

**(EvalListSigma-Empty)**
──────────────────────────────────────────────────────────────
Γ ⊢ EvalListSigma([], σ) ⇓ (Val([]), σ)

**(EvalListSigma-Cons)**
Γ ⊢ EvalSigma(e, σ) ⇓ (Val(v), σ_1)    Γ ⊢ EvalListSigma(es, σ_1) ⇓ (Val(vec_v), σ_2)
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalListSigma(e::es, σ) ⇓ (Val([v] ++ vec_v), σ_2)

**(EvalListSigma-Ctrl)**
Γ ⊢ EvalSigma(e, σ) ⇓ (Ctrl(κ), σ_1)
───────────────────────────────────────────────────────────────
Γ ⊢ EvalListSigma(e::es, σ) ⇓ (Ctrl(κ), σ_1)

**(EvalFieldInitsSigma-Empty)**
────────────────────────────────────────────────────────────────
Γ ⊢ EvalFieldInitsSigma([], σ) ⇓ (Val([]), σ)

**(EvalFieldInitsSigma-Cons)**
Γ ⊢ EvalSigma(e, σ) ⇓ (Val(v), σ_1)    Γ ⊢ EvalFieldInitsSigma(fs, σ_1) ⇓ (Val(vec_f), σ_2)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalFieldInitsSigma([⟨f, e⟩] ++ fs, σ) ⇓ (Val([⟨f, v⟩] ++ vec_f), σ_2)

**(EvalFieldInitsSigma-Ctrl)**
Γ ⊢ EvalSigma(e, σ) ⇓ (Ctrl(κ), σ_1)
───────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalFieldInitsSigma([⟨f, e⟩] ++ fs, σ) ⇓ (Ctrl(κ), σ_1)

BoolValue(lit) = true ⇔ lit.kind = BoolLiteral ∧ Lexeme(lit) = "true"
BoolValue(lit) = false ⇔ lit.kind = BoolLiteral ∧ Lexeme(lit) = "false"
CharValue(lit) = u ⇔ lit.kind = CharLiteral ∧ T = Lexeme(lit) ∧ StringBytesFrom(T, 1, |T|-1) = bytes ∧ DecodeUTF8(bytes) = [u]
LiteralValue(ℓ, TypePrim("bool")) = BoolVal(b) ⇔ ℓ.kind = BoolLiteral ∧ BoolValue(ℓ) = b
LiteralValue(ℓ, TypePrim("char")) = CharVal(c) ⇔ ℓ.kind = CharLiteral ∧ CharValue(ℓ) = c
LiteralValue(ℓ, TypeString(`@View`)) = v ⇔ ℓ.kind = StringLiteral ∧ ViewBytes(v) = StringBytes(ℓ)
LiteralValue(ℓ, TypePrim(t)) = IntVal(t, x) ⇔ ℓ.kind = IntLiteral ∧ t ∈ IntTypes ∧ x = IntValue(ℓ)
LiteralValue(ℓ, TypePrim(t)) = FloatVal(t, v) ⇔ ℓ.kind = FloatLiteral ∧ t ∈ FloatTypes ∧ v = FloatValue(ℓ)
LiteralValue(ℓ, TypeRawPtr(q, U)) = RawPtr(q, 0x0) ⇔ ℓ.kind = NullLiteral
EnumPayloadVal = {⊥, TuplePayload(vec_v), RecordPayload(vec_f)}
RecordValue(tr, fs) defined
EnumValue(path, payload) defined ⇔ payload ∈ EnumPayloadVal

**(EvalSigma-Literal)**
T = ExprType(Literal(ℓ))    LiteralValue(ℓ, T) = v
──────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(Literal(ℓ), σ) ⇓ (Val(v), σ)

**(EvalSigma-Ident-Poison)**
LookupBind(σ, x) undefined    Γ ⊢ ResolveValueName(x) ⇓ ent    ent.origin_opt = mp    PoisonedModule(σ, PathOfModule(mp))
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(Identifier(x), σ) ⇓ (Ctrl(Panic), σ)

**(EvalSigma-Ident-Poison-RecordCtor)**
LookupBind(σ, x) undefined    Γ ⊢ ResolveValueName(x) ⇑    Γ ⊢ ResolveTypeName(x) ⇓ ent    ent.origin_opt = mp    PoisonedModule(σ, PathOfModule(mp))
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(Identifier(x), σ) ⇓ (Ctrl(Panic), σ)

**(EvalSigma-Ident)**
LookupVal(σ, x) = v
──────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(Identifier(x), σ) ⇓ (Val(v), σ)

**(EvalSigma-Path-Poison)**
Γ ⊢ ResolveQualified(path, name, ValueKind) ⇓ ent    ent.origin_opt = mp    PoisonedModule(σ, PathOfModule(mp))
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(Path(path, name), σ) ⇓ (Ctrl(Panic), σ)

**(EvalSigma-Path-Poison-RecordCtor)**
Γ ⊢ ResolveQualified(path, name, ValueKind) ⇑    Γ ⊢ ResolveRecordPath(path, name) ⇓ p    SplitLast(p) = (mp, _)    PoisonedModule(σ, mp)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(Path(path, name), σ) ⇓ (Ctrl(Panic), σ)

**(EvalSigma-Path)**
LookupValPath(σ, path, name) = v
──────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(Path(path, name), σ) ⇓ (Val(v), σ)

**(EvalSigma-ErrorExpr)**
────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(ErrorExpr(_), σ) ⇓ (Ctrl(Panic), σ)

**(EvalSigma-Tuple)**
Γ ⊢ EvalListSigma(es, σ) ⇓ (Val(vec_v), σ_1)
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(TupleExpr(es), σ) ⇓ (Val((v_1, …, v_n)), σ_1)

**(EvalSigma-Tuple-Ctrl)**
Γ ⊢ EvalListSigma(es, σ) ⇓ (Ctrl(κ), σ_1)
───────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(TupleExpr(es), σ) ⇓ (Ctrl(κ), σ_1)

**(EvalSigma-Array)**
Γ ⊢ EvalListSigma(es, σ) ⇓ (Val(vec_v), σ_1)
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(ArrayExpr(es), σ) ⇓ (Val([v_1, …, v_n]), σ_1)

**(EvalSigma-Array-Ctrl)**
Γ ⊢ EvalListSigma(es, σ) ⇓ (Ctrl(κ), σ_1)
───────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(ArrayExpr(es), σ) ⇓ (Ctrl(κ), σ_1)

**(EvalSigma-Record)**
Γ ⊢ EvalFieldInitsSigma(fields, σ) ⇓ (Val(vec_f), σ_1)
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(RecordExpr(tr, fields), σ) ⇓ (Val(RecordValue(tr, vec_f)), σ_1)

**(EvalSigma-Record-Ctrl)**
Γ ⊢ EvalFieldInitsSigma(fields, σ) ⇓ (Ctrl(κ), σ_1)
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(RecordExpr(tr, fields), σ) ⇓ (Ctrl(κ), σ_1)

**(EvalSigma-Enum-Unit)**
──────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(EnumLiteral(path, ⊥), σ) ⇓ (Val(EnumValue(path, ⊥)), σ)

**(EvalSigma-Enum-Tuple)**
Γ ⊢ EvalListSigma(es, σ) ⇓ (Val(vec_v), σ_1)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(EnumLiteral(path, Paren(es)), σ) ⇓ (Val(EnumValue(path, TuplePayload(vec_v))), σ_1)

**(EvalSigma-Enum-Tuple-Ctrl)**
Γ ⊢ EvalListSigma(es, σ) ⇓ (Ctrl(κ), σ_1)
───────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(EnumLiteral(path, Paren(es)), σ) ⇓ (Ctrl(κ), σ_1)

**(EvalSigma-Enum-Record)**
Γ ⊢ EvalFieldInitsSigma(fields, σ) ⇓ (Val(vec_f), σ_1)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(EnumLiteral(path, Brace(fields)), σ) ⇓ (Val(EnumValue(path, RecordPayload(vec_f))), σ_1)

**(EvalSigma-Enum-Record-Ctrl)**
Γ ⊢ EvalFieldInitsSigma(fields, σ) ⇓ (Ctrl(κ), σ_1)
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(EnumLiteral(path, Brace(fields)), σ) ⇓ (Ctrl(κ), σ_1)

**(EvalSigma-FieldAccess)**
Γ ⊢ EvalSigma(base, σ) ⇓ (Val(v_b), σ_1)    FieldValue(v_b, f) = v_f
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(FieldAccess(base, f), σ) ⇓ (Val(v_f), σ_1)

**(EvalSigma-FieldAccess-Ctrl)**
Γ ⊢ EvalSigma(base, σ) ⇓ (Ctrl(κ), σ_1)
──────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(FieldAccess(base, f), σ) ⇓ (Ctrl(κ), σ_1)

**(EvalSigma-TupleAccess)**
Γ ⊢ EvalSigma(base, σ) ⇓ (Val(v_b), σ_1)    TupleValue(v_b, i) = v_i
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(TupleAccess(base, i), σ) ⇓ (Val(v_i), σ_1)

**(EvalSigma-TupleAccess-Ctrl)**
Γ ⊢ EvalSigma(base, σ) ⇓ (Ctrl(κ), σ_1)
──────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(TupleAccess(base, i), σ) ⇓ (Ctrl(κ), σ_1)

**(EvalSigma-Index)**
Γ ⊢ EvalSigma(base, σ) ⇓ (Val(v_b), σ_1)    Γ ⊢ EvalSigma(idx, σ_1) ⇓ (Val(v_i), σ_2)    IndexValue(v_b, v_i) = v_e
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(IndexAccess(base, idx), σ) ⇓ (Val(v_e), σ_2)

**(EvalSigma-Index-OOB)**
Γ ⊢ EvalSigma(base, σ) ⇓ (Val(v_b), σ_1)    Γ ⊢ EvalSigma(idx, σ_1) ⇓ (Val(v_i), σ_2)    IndexNum(v_i) = i    ¬ (0 ≤ i < Len(v_b))
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(IndexAccess(base, idx), σ) ⇓ (Ctrl(Panic), σ_2)

**(EvalSigma-Index-Range)**
Γ ⊢ EvalSigma(base, σ) ⇓ (Val(v_b), σ_1)    Γ ⊢ EvalSigma(idx, σ_1) ⇓ (Val(v_r), σ_2)    SliceValue(v_b, v_r) = v_s
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(IndexAccess(base, idx), σ) ⇓ (Val(v_s), σ_2)

**(EvalSigma-Index-Range-OOB)**
Γ ⊢ EvalSigma(base, σ) ⇓ (Val(v_b), σ_1)    Γ ⊢ EvalSigma(idx, σ_1) ⇓ (Val(v_r), σ_2)    SliceBounds(v_r, Len(v_b)) = ⊥
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(IndexAccess(base, idx), σ) ⇓ (Ctrl(Panic), σ_2)

**(EvalSigma-Index-Ctrl-Base)**
Γ ⊢ EvalSigma(base, σ) ⇓ (Ctrl(κ), σ_1)
──────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(IndexAccess(base, idx), σ) ⇓ (Ctrl(κ), σ_1)

**(EvalSigma-Index-Ctrl-Idx)**
Γ ⊢ EvalSigma(base, σ) ⇓ (Val(v_b), σ_1)    Γ ⊢ EvalSigma(idx, σ_1) ⇓ (Ctrl(κ), σ_2)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(IndexAccess(base, idx), σ) ⇓ (Ctrl(κ), σ_2)

**Unary, Binary, Cast, Transmute, Propagate.**

RetType(Γ) ∈ Type
OpJudg = {UnOp(op, v) ⇓ v', BinOp(op, v_1, v_2) ⇓ v}
NumericValue(v, t) ⇔ ValueType(v) = TypePrim(t) ∧ t ∈ NumericTypes
IntValue(v, t) ⇔ ValueType(v) = TypePrim(t) ∧ t ∈ IntTypes
FloatValue(v, t) ⇔ ValueType(v) = TypePrim(t) ∧ t ∈ FloatTypes
SignedIntValue(v) ⇔ ∃ t. t ∈ SignedIntTypes ∧ ValueType(v) = TypePrim(t)
SignedTypeOf(v) = t ⇔ t ∈ SignedIntTypes ∧ ValueType(v) = TypePrim(t)
U32Value(v) ⇔ ValueType(v) = TypePrim("u32")
EqValue(v_1, v_2) ⇔ ∃ T. ValueType(v_1) = T ∧ ValueType(v_2) = T ∧ EqType(T)
OrdValue(v_1, v_2) ⇔ ∃ T. ValueType(v_1) = T ∧ ValueType(v_2) = T ∧ OrdType(T)
IsNaN(t, v) ⇔ t ∈ FloatTypes ∧ v = FloatVal(t, x) ∧ IEEE754Encode(t, x) = CanonicalNaNBits(t)
OrdScalar(v) = x ⇔ (∃ t. v = IntVal(t, x) ∧ t ∈ IntTypes) ∨ (∃ u. v = CharVal(u) ∧ x = u)
Cmp("==", v_1, v_2) = b ⇔ EqValue(v_1, v_2) ∧ ((∃ t. FloatValue(v_1, t) ∧ FloatValue(v_2, t) ∧ (IsNaN(t, v_1) ∨ IsNaN(t, v_2)) ∧ b = false) ∨ (¬ ∃ t. FloatValue(v_1, t) ∧ FloatValue(v_2, t) ∧ (IsNaN(t, v_1) ∨ IsNaN(t, v_2)) ∧ b = (v_1 = v_2)))
Cmp("!=", v_1, v_2) = b ⇔ EqValue(v_1, v_2) ∧ ((∃ t. FloatValue(v_1, t) ∧ FloatValue(v_2, t) ∧ (IsNaN(t, v_1) ∨ IsNaN(t, v_2)) ∧ b = true) ∨ (¬ ∃ t. FloatValue(v_1, t) ∧ FloatValue(v_2, t) ∧ (IsNaN(t, v_1) ∨ IsNaN(t, v_2)) ∧ b = (v_1 ≠ v_2)))
Cmp("<", v_1, v_2) = b ⇔ OrdValue(v_1, v_2) ∧ ((∃ t. FloatValue(v_1, t) ∧ FloatValue(v_2, t) ∧ ((IsNaN(t, v_1) ∨ IsNaN(t, v_2)) ∧ b = false) ∨ (¬ (IsNaN(t, v_1) ∨ IsNaN(t, v_2)) ∧ x_1 = FloatValValue(v_1) ∧ x_2 = FloatValValue(v_2) ∧ b = (x_1 < x_2))) ∨ (∃ x_1, x_2. OrdScalar(v_1) = x_1 ∧ OrdScalar(v_2) = x_2 ∧ b = (x_1 < x_2)))
Cmp("<=", v_1, v_2) = b ⇔ OrdValue(v_1, v_2) ∧ ((∃ t. FloatValue(v_1, t) ∧ FloatValue(v_2, t) ∧ ((IsNaN(t, v_1) ∨ IsNaN(t, v_2)) ∧ b = false) ∨ (¬ (IsNaN(t, v_1) ∨ IsNaN(t, v_2)) ∧ x_1 = FloatValValue(v_1) ∧ x_2 = FloatValValue(v_2) ∧ b = (x_1 ≤ x_2))) ∨ (∃ x_1, x_2. OrdScalar(v_1) = x_1 ∧ OrdScalar(v_2) = x_2 ∧ b = (x_1 ≤ x_2)))
Cmp(">", v_1, v_2) = b ⇔ OrdValue(v_1, v_2) ∧ ((∃ t. FloatValue(v_1, t) ∧ FloatValue(v_2, t) ∧ ((IsNaN(t, v_1) ∨ IsNaN(t, v_2)) ∧ b = false) ∨ (¬ (IsNaN(t, v_1) ∨ IsNaN(t, v_2)) ∧ x_1 = FloatValValue(v_1) ∧ x_2 = FloatValValue(v_2) ∧ b = (x_1 > x_2))) ∨ (∃ x_1, x_2. OrdScalar(v_1) = x_1 ∧ OrdScalar(v_2) = x_2 ∧ b = (x_1 > x_2)))
Cmp(">=", v_1, v_2) = b ⇔ OrdValue(v_1, v_2) ∧ ((∃ t. FloatValue(v_1, t) ∧ FloatValue(v_2, t) ∧ ((IsNaN(t, v_1) ∨ IsNaN(t, v_2)) ∧ b = false) ∨ (¬ (IsNaN(t, v_1) ∨ IsNaN(t, v_2)) ∧ x_1 = FloatValValue(v_1) ∧ x_2 = FloatValValue(v_2) ∧ b = (x_1 ≥ x_2))) ∨ (∃ x_1, x_2. OrdScalar(v_1) = x_1 ∧ OrdScalar(v_2) = x_2 ∧ b = (x_1 ≥ x_2)))
BitAt(u, i) = b ⇔ b ∈ {0, 1} ∧ ∃ q, r. u = q · 2^{i + 1} + b · 2^i + r ∧ 0 ≤ r < 2^i
BitNot(v) = v' ⇔ ∃ t, x, w, u, u'. v = IntVal(t, x) ∧ w = IntWidth(t) ∧ u = ToUnsigned(w, x) ∧ u' = (2^w - 1) - u ∧ ((t ∈ SignedIntTypes ∧ v' = IntVal(t, ToSigned(w, u'))) ∨ (t ∈ UnsignedIntTypes ∧ v' = IntVal(t, u')))
BitOp(op, t, v_1, v_2) = v ⇔ v_1 = IntVal(t, x_1) ∧ v_2 = IntVal(t, x_2) ∧ w = IntWidth(t) ∧ u_1 = ToUnsigned(w, x_1) ∧ u_2 = ToUnsigned(w, x_2) ∧ u = ∑_{i=0}^{w-1} b_i 2^i ∧ ∀ i. 0 ≤ i < w ⇒ ((op = "&" ∧ b_i = BitAt(u_1, i) · BitAt(u_2, i)) ∨ (op = "|" ∧ b_i = max(BitAt(u_1, i), BitAt(u_2, i))) ∨ (op = "^" ∧ b_i = (BitAt(u_1, i) + BitAt(u_2, i)) mod 2)) ∧ ((t ∈ SignedIntTypes ∧ v = IntVal(t, ToSigned(w, u))) ∨ (t ∈ UnsignedIntTypes ∧ v = IntVal(t, u)))
ShiftOp(op, t, v_1, v_2) = v ⇔ v_1 = IntVal(t, x_1) ∧ v_2 = IntVal("u32", n) ∧ w = IntWidth(t) ∧ 0 ≤ n < w ∧ u_1 = ToUnsigned(w, x_1) ∧ ((op = "<<" ∧ u = (u_1 · 2^n) mod 2^w) ∨ (op = ">>" ∧ u = ⌊u_1 / 2^n⌋)) ∧ ((t ∈ SignedIntTypes ∧ v = IntVal(t, ToSigned(w, u))) ∨ (t ∈ UnsignedIntTypes ∧ v = IntVal(t, u)))
PowInt(x, n) = y ⇔ n ∈ ℕ ∧ ((n = 0 ∧ y = 1) ∨ (n > 0 ∧ y = x · PowInt(x, n - 1)))
PowFloat(t, x_1, x_2) = x ⇔ t ∈ FloatTypes ∧ x_1 ∈ FloatValueSet(t) ∧ x_2 ∈ FloatValueSet(t) ∧ x is the IEEE 754-2019 pow result of x_1, x_2 in format FloatFormat(t)
IEEEArith(op, t, v_1, v_2) = v ⇔ v_1 = FloatVal(t, x_1) ∧ v_2 = FloatVal(t, x_2) ∧ op ∈ ArithOps ∧ ((op ∈ {"+", "-", "*", "/"} ∧ x is the IEEE 754-2019 result of applying op to x_1, x_2 in format FloatFormat(t)) ∨ (op = "%" ∧ x is the IEEE 754-2019 remainder of x_1, x_2 in format FloatFormat(t)) ∨ (op = "**" ∧ PowFloat(t, x_1, x_2) = x)) ∧ v = FloatVal(t, x)
∀ t ∈ FloatTypes, v_1, v_2, op ∈ ArithOps. ∃ v. IEEEArith(op, t, v_1, v_2) = v
UnOp("!", false) ⇓ true
UnOp("!", true) ⇓ false
UnOp("!", v) ⇓ v' ⇔ IntValue(v, t) ∧ v' = BitNot(v)
UnOp("-", v) ⇓ v' ⇔ v = IntVal(t, x) ∧ t ∈ SignedIntTypes ∧ x' = -x ∧ InRange(x', t) ∧ v' = IntVal(t, x')
UnOp("-", v) ⇓ v' ⇔ v = FloatVal(t, x) ∧ v' = FloatVal(t, -x)
UnOp("widen", v) ⇓ v
BinOp(op, v_1, v_2) ⇓ v ⇔ op ∈ ArithOps ∧ NumericValue(v_1, t) ∧ NumericValue(v_2, t) ∧ ArithEval(op, t, v_1, v_2) ⇓ v
BinOp(op, v_1, v_2) ⇓ v ⇔ op ∈ BitOps ∧ IntValue(v_1, t) ∧ IntValue(v_2, t) ∧ BitEval(op, t, v_1, v_2) ⇓ v
BinOp(op, v_1, v_2) ⇓ v ⇔ op ∈ ShiftOps ∧ IntValue(v_1, t) ∧ U32Value(v_2) ∧ ShiftEval(op, t, v_1, v_2) ⇓ v
BinOp(op, v_1, v_2) ⇓ v ⇔ op ∈ {"==", "!="} ∧ EqValue(v_1, v_2) ∧ v = Cmp(op, v_1, v_2)
BinOp(op, v_1, v_2) ⇓ v ⇔ op ∈ {"<", "<=", ">", ">="} ∧ OrdValue(v_1, v_2) ∧ v = Cmp(op, v_1, v_2)
ArithEval(op, t, v_1, v_2) ⇓ v ⇔ t ∈ IntTypes ∧ v_1 = IntVal(t, x_1) ∧ v_2 = IntVal(t, x_2) ∧ ((op ∈ {"+", "-", "*"} ∧ x = x_1 op x_2) ∨ (op ∈ {"/", "%"} ∧ x_2 ≠ 0 ∧ x = x_1 op x_2) ∨ (op = "**" ∧ x_2 ≥ 0 ∧ PowInt(x_1, x_2) = x)) ∧ InRange(x, t) ∧ v = IntVal(t, x)
ArithEval(op, t, v_1, v_2) ⇓ v ⇔ t ∈ FloatTypes ∧ v = IEEEArith(op, t, v_1, v_2)
BitEval(op, t, v_1, v_2) ⇓ v ⇔ t ∈ IntTypes ∧ v = BitOp(op, t, v_1, v_2)
ShiftEval(op, t, v_1, v_2) ⇓ v ⇔ t ∈ IntTypes ∧ v = ShiftOp(op, t, v_1, v_2)

**(EvalSigma-Unary)**
Γ ⊢ EvalSigma(e, σ) ⇓ (Val(v), σ_1)    UnOp(op, v) ⇓ v'
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(Unary(op, e), σ) ⇓ (Val(v'), σ_1)

**(EvalSigma-Unary-Panic)**
Γ ⊢ EvalSigma(e, σ) ⇓ (Val(v), σ_1)    UnOp(op, v) undefined
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(Unary(op, e), σ) ⇓ (Ctrl(Panic), σ_1)

**(EvalSigma-Unary-Ctrl)**
Γ ⊢ EvalSigma(e, σ) ⇓ (Ctrl(κ), σ_1)
──────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(Unary(op, e), σ) ⇓ (Ctrl(κ), σ_1)

**(EvalSigma-Bin-And-False)**
Γ ⊢ EvalSigma(e_1, σ) ⇓ (Val(false), σ_1)
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(Binary("&&", e_1, e_2), σ) ⇓ (Val(false), σ_1)

**(EvalSigma-Bin-And-True)**
Γ ⊢ EvalSigma(e_1, σ) ⇓ (Val(true), σ_1)    Γ ⊢ EvalSigma(e_2, σ_1) ⇓ (out, σ_2)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(Binary("&&", e_1, e_2), σ) ⇓ (out, σ_2)

**(EvalSigma-Bin-Or-True)**
Γ ⊢ EvalSigma(e_1, σ) ⇓ (Val(true), σ_1)
─────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(Binary("||", e_1, e_2), σ) ⇓ (Val(true), σ_1)

**(EvalSigma-Bin-Or-False)**
Γ ⊢ EvalSigma(e_1, σ) ⇓ (Val(false), σ_1)    Γ ⊢ EvalSigma(e_2, σ_1) ⇓ (out, σ_2)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(Binary("||", e_1, e_2), σ) ⇓ (out, σ_2)

**(EvalSigma-Binary)**
op ∉ {"&&", "||"}    Γ ⊢ EvalSigma(e_1, σ) ⇓ (Val(v_1), σ_1)    Γ ⊢ EvalSigma(e_2, σ_1) ⇓ (Val(v_2), σ_2)    BinOp(op, v_1, v_2) ⇓ v
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(Binary(op, e_1, e_2), σ) ⇓ (Val(v), σ_2)

**(EvalSigma-Binary-Panic)**
op ∉ {"&&", "||"}    Γ ⊢ EvalSigma(e_1, σ) ⇓ (Val(v_1), σ_1)    Γ ⊢ EvalSigma(e_2, σ_1) ⇓ (Val(v_2), σ_2)    BinOp(op, v_1, v_2) undefined
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(Binary(op, e_1, e_2), σ) ⇓ (Ctrl(Panic), σ_2)

**(EvalSigma-Bin-Ctrl-L)**
Γ ⊢ EvalSigma(e_1, σ) ⇓ (Ctrl(κ), σ_1)
──────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(Binary(op, e_1, e_2), σ) ⇓ (Ctrl(κ), σ_1)

**(EvalSigma-Bin-Ctrl-R)**
op ∉ {"&&", "||"}    Γ ⊢ EvalSigma(e_1, σ) ⇓ (Val(v_1), σ_1)    Γ ⊢ EvalSigma(e_2, σ_1) ⇓ (Ctrl(κ), σ_2)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(Binary(op, e_1, e_2), σ) ⇓ (Ctrl(κ), σ_2)

ExprType : Expr → Type
R = RetType(Γ)

CastValJudg = {CastVal(S, T, v) ⇓ v'}
UnsignedIntTypes = {`u8`, `u16`, `u32`, `u64`, `u128`, `usize`}
IntWidth(`i8`) = 8    IntWidth(`i16`) = 16    IntWidth(`i32`) = 32    IntWidth(`i64`) = 64    IntWidth(`i128`) = 128
IntWidth(`u8`) = 8    IntWidth(`u16`) = 16    IntWidth(`u32`) = 32    IntWidth(`u64`) = 64    IntWidth(`u128`) = 128
IntWidth(`isize`) = 8 × PointerSize    IntWidth(`usize`) = 8 × PointerSize
Mod_w(x) = x mod 2^w
ToSigned(w, x) = y ⇔ y ∈ [-2^{w-1}, 2^{w-1}-1] ∧ y ≡ x mod 2^w
ToUnsigned(w, x) = y ⇔ y ∈ [0, 2^w-1] ∧ y ≡ x mod 2^w
CodePoint : `char` → ℕ
IsScalar(u) ⇔ u ∈ CharValueRange
IntToFloat(t, x) function
FloatToFloat(s, t, v) function
Trunc(v) function
CharOf(u) = u ⇔ IsScalar(u)
CodePoint(CharOf(u)) = u    (IsScalar(u))
IEEE754Bits(t, v) = bits ⇔ v ∈ FloatValueSet(t) ∧ IEEE754Encode(t, v) = bits
IntToFloat(t, x) = v ⇔ v ∈ NonNaNValueSet(t) ∧ ∀ v' ∈ NonNaNValueSet(t). |v - x| < |v' - x| ∨ (|v - x| = |v' - x| ∧ EvenSignificandLSB(t, v))
FloatToFloat(s, t, v) = v' ⇔ IEEE754Encode(s, v) = CanonicalNaNBits(s) ∧ v' = CanonicalNaN(t)
FloatToFloat(s, t, v) = v' ⇔ IEEE754Encode(s, v) ≠ CanonicalNaNBits(s) ∧ v' ∈ NonNaNValueSet(t) ∧ ∀ u ∈ NonNaNValueSet(t). |v' - v| < |u - v| ∨ (|v' - v| = |u - v| ∧ EvenSignificandLSB(t, v'))
Trunc(v) =
 ⌊v⌋    if v ≥ 0
 ⌈v⌉    if v < 0

**(CastVal-Id)**
StripPerm(S) = StripPerm(T)
────────────────────────────────────────
CastVal(S, T, v) ⇓ v

**(CastVal-Int-Int-Signed)**
S' = StripPerm(S)    T' = StripPerm(T)    S' = TypePrim(s)    T' = TypePrim(t)    s ∈ IntTypes    t ∈ SignedIntTypes    v = IntVal(s, x)    w = IntWidth(t)    x' = ToSigned(w, x)    v' = IntVal(t, x')
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
CastVal(S, T, v) ⇓ v'

**(CastVal-Int-Int-Unsigned)**
S' = StripPerm(S)    T' = StripPerm(T)    S' = TypePrim(s)    T' = TypePrim(t)    s ∈ IntTypes    t ∈ UnsignedIntTypes    v = IntVal(s, x)    w = IntWidth(t)    x' = ToUnsigned(w, x)    v' = IntVal(t, x')
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
CastVal(S, T, v) ⇓ v'

**(CastVal-Int-Float)**
S' = StripPerm(S)    T' = StripPerm(T)    S' = TypePrim(s)    T' = TypePrim(t)    s ∈ IntTypes    t ∈ FloatTypes    v = IntVal(s, x)    v' = FloatVal(t, IntToFloat(t, x))
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
CastVal(S, T, v) ⇓ v'

**(CastVal-Float-Float)**
S' = StripPerm(S)    T' = StripPerm(T)    S' = TypePrim(s)    T' = TypePrim(t)    s ∈ FloatTypes    t ∈ FloatTypes    v = FloatVal(s, x)    v' = FloatVal(t, FloatToFloat(s, t, x))
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
CastVal(S, T, v) ⇓ v'

**(CastVal-Float-Int)**
S' = StripPerm(S)    T' = StripPerm(T)    S' = TypePrim(s)    T' = TypePrim(t)    s ∈ FloatTypes    t ∈ IntTypes    v = FloatVal(s, x)    x' = Trunc(x)    InRange(x', t)    v' = IntVal(t, x')
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
CastVal(S, T, v) ⇓ v'

**(CastVal-Bool-Int)**
S' = StripPerm(S)    T' = StripPerm(T)    S' = TypePrim("bool")    T' = TypePrim(t)    t ∈ IntTypes    v' =
 IntVal(t, 0)    if v = false
 IntVal(t, 1)    if v = true
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
CastVal(S, T, v) ⇓ v'

**(CastVal-Int-Bool)**
S' = StripPerm(S)    T' = StripPerm(T)    S' = TypePrim(t)    t ∈ IntTypes    T' = TypePrim("bool")    v = IntVal(t, x)    v' =
 false    if x = 0
 true     if x ≠ 0
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
CastVal(S, T, v) ⇓ v'

**(CastVal-Char-U32)**
S' = StripPerm(S)    T' = StripPerm(T)    S' = TypePrim("char")    T' = TypePrim("u32")    v' = IntVal("u32", CodePoint(v))
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
CastVal(S, T, v) ⇓ v'

**(CastVal-U32-Char)**
S' = StripPerm(S)    T' = StripPerm(T)    S' = TypePrim("u32")    T' = TypePrim("char")    v = IntVal("u32", x)    IsScalar(x)    v' = CharVal(CharOf(x))
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
CastVal(S, T, v) ⇓ v'

**(EvalSigma-Cast)**
Γ ⊢ EvalSigma(e, σ) ⇓ (Val(v), σ_1)    S = ExprType(e)    CastVal(S, T, v) ⇓ v'
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(Cast(e, T), σ) ⇓ (Val(v'), σ_1)

**(EvalSigma-Cast-Panic)**
Γ ⊢ EvalSigma(e, σ) ⇓ (Val(v), σ_1)    S = ExprType(e)    CastVal(S, T, v) undefined
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(Cast(e, T), σ) ⇓ (Ctrl(Panic), σ_1)

**(EvalSigma-Cast-Ctrl)**
Γ ⊢ EvalSigma(e, σ) ⇓ (Ctrl(κ), σ_1)
──────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(Cast(e, T), σ) ⇓ (Ctrl(κ), σ_1)

TransmuteVal(S, T, v) ⇓ v' ⇔ ValueBits(S, v) = bits ∧ ValueBits(T, v') = bits

**(EvalSigma-Transmute)**
Γ ⊢ EvalSigma(e, σ) ⇓ (Val(v), σ_1)    S = t_1    T = t_2    TransmuteVal(S, T, v) ⇓ v'
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(TransmuteExpr(t_1, t_2, e), σ) ⇓ (Val(v'), σ_1)

**(EvalSigma-Transmute-Ctrl)**
Γ ⊢ EvalSigma(e, σ) ⇓ (Ctrl(κ), σ_1)
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(TransmuteExpr(t_1, t_2, e), σ) ⇓ (Ctrl(κ), σ_1)

UnionCaseJudg = {UnionCase(v) = ⟨T, v_T⟩}
UnionCase(v) = ⟨T, v_T⟩ ⇔ ∃ U, bits. ValueBits(TypeUnion(U), v) = bits ∧ UnionBits(U, T, v_T) = bits

**(EvalSigma-Propagate-Success)**
Γ ⊢ EvalSigma(e, σ) ⇓ (Val(v), σ_1)    U = ExprType(e)    SuccessMember(RetType(Γ), U) = T_s    UnionCase(v) = ⟨T_s, v_s⟩
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(Propagate(e), σ) ⇓ (Val(v_s), σ_1)

**(EvalSigma-Propagate-Error)**
Γ ⊢ EvalSigma(e, σ) ⇓ (Val(v), σ_1)    U = ExprType(e)    SuccessMember(RetType(Γ), U) = T_s    UnionCase(v) = ⟨T_e, v_e⟩    T_e ≠ T_s
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(Propagate(e), σ) ⇓ (Ctrl(Return(v_e)), σ_1)

**(EvalSigma-Propagate-Ctrl)**
Γ ⊢ EvalSigma(e, σ) ⇓ (Ctrl(κ), σ_1)
──────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(Propagate(e), σ) ⇓ (Ctrl(κ), σ_1)

**Conditionals and Match.**

**(EvalSigma-If-True)**
Γ ⊢ EvalSigma(cond, σ) ⇓ (Val(true), σ_1)    Γ ⊢ EvalSigma(then_block, σ_1) ⇓ (out, σ_2)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(IfExpr(cond, then_block, else_opt), σ) ⇓ (out, σ_2)

**(EvalSigma-If-False-None)**
Γ ⊢ EvalSigma(cond, σ) ⇓ (Val(false), σ_1)    else_opt = ⊥
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(IfExpr(cond, then_block, else_opt), σ) ⇓ (Val(()), σ_1)

**(EvalSigma-If-False-Some)**
Γ ⊢ EvalSigma(cond, σ) ⇓ (Val(false), σ_1)    else_opt = e    Γ ⊢ EvalSigma(e, σ_1) ⇓ (out, σ_2)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(IfExpr(cond, then_block, else_opt), σ) ⇓ (out, σ_2)

**(EvalSigma-If-Ctrl)**
Γ ⊢ EvalSigma(cond, σ) ⇓ (Ctrl(κ), σ_1)
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(IfExpr(cond, then_block, else_opt), σ) ⇓ (Ctrl(κ), σ_1)

ArmResult = {Match(out), NoMatch}
MatchArmJudg = {Γ ⊢ MatchArmSigma(arm, v, σ) ⇓ (res, σ')}

**(EvalArmBody-Block)**
body = b    Γ ⊢ EvalBlockSigma(b, σ) ⇓ (out, σ')
──────────────────────────────────────────────────────────────
Γ ⊢ EvalArmBodySigma(body, σ) ⇓ (out, σ')

**(EvalArmBody-Expr)**
body = e    Γ ⊢ EvalSigma(e, σ) ⇓ (out, σ')
────────────────────────────────────────────────────────────
Γ ⊢ EvalArmBodySigma(body, σ) ⇓ (out, σ')

**(MatchArm-Fail)**
Γ ⊢ MatchPattern(pat, v) undefined
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ MatchArmSigma(⟨pat, guard_opt, body⟩, v, σ) ⇓ (NoMatch, σ)

**(MatchArm-Guard-False)**
Γ ⊢ MatchPattern(pat, v) ⇓ B    BindOrder(pat, B) = binds    BlockEnter(σ, binds) ⇓ (σ_1, scope)    guard_opt = g    Γ ⊢ EvalSigma(g, σ_1) ⇓ (Val(false), σ_2)    BlockExit(σ_2, scope, Val(())) ⇓ (out', σ_3)
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ MatchArmSigma(⟨pat, guard_opt, body⟩, v, σ) ⇓ (NoMatch, σ_3)

**(MatchArm-Guard-True)**
Γ ⊢ MatchPattern(pat, v) ⇓ B    BindOrder(pat, B) = binds    BlockEnter(σ, binds) ⇓ (σ_1, scope)    guard_opt = g    Γ ⊢ EvalSigma(g, σ_1) ⇓ (Val(true), σ_2)    Γ ⊢ EvalArmBodySigma(body, σ_2) ⇓ (out, σ_3)    BlockExit(σ_3, scope, out) ⇓ (out', σ_4)
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ MatchArmSigma(⟨pat, guard_opt, body⟩, v, σ) ⇓ (Match(out'), σ_4)

**(MatchArm-NoGuard)**
Γ ⊢ MatchPattern(pat, v) ⇓ B    BindOrder(pat, B) = binds    BlockEnter(σ, binds) ⇓ (σ_1, scope)    guard_opt = ⊥    Γ ⊢ EvalArmBodySigma(body, σ_1) ⇓ (out, σ_2)    BlockExit(σ_2, scope, out) ⇓ (out', σ_3)
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ MatchArmSigma(⟨pat, guard_opt, body⟩, v, σ) ⇓ (Match(out'), σ_3)

**(MatchArm-Ctrl)**
Γ ⊢ MatchPattern(pat, v) ⇓ B    BindOrder(pat, B) = binds    BlockEnter(σ, binds) ⇓ (σ_1, scope)    guard_opt = g    Γ ⊢ EvalSigma(g, σ_1) ⇓ (Ctrl(κ), σ_2)    BlockExit(σ_2, scope, Ctrl(κ)) ⇓ (out', σ_3)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ MatchArmSigma(⟨pat, guard_opt, body⟩, v, σ) ⇓ (Match(out'), σ_3)

**(MatchArms-Head)**
Γ ⊢ MatchArmSigma(a, v, σ) ⇓ (Match(out), σ_1)
──────────────────────────────────────────────────────────────
Γ ⊢ MatchArmsSigma(a::as, v, σ) ⇓ (out, σ_1)

**(MatchArms-Tail)**
Γ ⊢ MatchArmSigma(a, v, σ) ⇓ (NoMatch, σ_1)    Γ ⊢ MatchArmsSigma(as, v, σ_1) ⇓ (out, σ_2)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ MatchArmsSigma(a::as, v, σ) ⇓ (out, σ_2)

**(EvalSigma-Match)**
Γ ⊢ EvalSigma(scrutinee, σ) ⇓ (Val(v), σ_1)    Γ ⊢ MatchArmsSigma(arms, v, σ_1) ⇓ (out, σ_2)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(MatchExpr(scrutinee, arms), σ) ⇓ (out, σ_2)

**(EvalSigma-Match-Ctrl)**
Γ ⊢ EvalSigma(scrutinee, σ) ⇓ (Ctrl(κ), σ_1)
───────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(MatchExpr(scrutinee, arms), σ) ⇓ (Ctrl(κ), σ_1)

**Pointer and Move Expressions.**

**(EvalSigma-PtrNull)**
──────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(PtrNullExpr, σ) ⇓ (Val(Ptr@Null(0x0)), σ)

**(EvalSigma-AddressOf)**
Γ ⊢ AddrOfSigma(p, σ) ⇓ (Val(addr), σ_1)
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(AddressOf(p), σ) ⇓ (Val(Ptr@Valid(addr)), σ_1)

**(EvalSigma-AddressOf-Ctrl)**
Γ ⊢ AddrOfSigma(p, σ) ⇓ (Ctrl(κ), σ_1)
────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(AddressOf(p), σ) ⇓ (Ctrl(κ), σ_1)

**(EvalSigma-Deref)**
Γ ⊢ EvalSigma(e, σ) ⇓ (Val(v_ptr), σ_1)    Γ ⊢ ReadPtrSigma(v_ptr, σ_1) ⇓ (out, σ_2)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(Deref(e), σ) ⇓ (out, σ_2)

**(EvalSigma-Deref-Ctrl)**
Γ ⊢ EvalSigma(e, σ) ⇓ (Ctrl(κ), σ_1)
──────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(Deref(e), σ) ⇓ (Ctrl(κ), σ_1)

**(EvalSigma-Move)**
Γ ⊢ MovePlaceSigma(p, σ) ⇓ (out, σ_1)
──────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(MoveExpr(p), σ) ⇓ (out, σ_1)

**Call and Method Application.**

CallJudg = {Γ ⊢ EvalArgsSigma(params, args, σ) ⇓ (out, σ'), Γ ⊢ EvalRecvSigma(base, mode, σ) ⇓ (out, σ'), Γ ⊢ ApplyRegionProc(name, vec_v, σ) ⇓ (out, σ'), Γ ⊢ ApplyProcSigma(proc, vec_v, σ) ⇓ (out, σ'), Γ ⊢ ApplyRecordCtorSigma(p, σ) ⇓ (out, σ'), Γ ⊢ ApplyMethodSigma(base, name, v_self, v_arg, vec_v, σ) ⇓ (out, σ')}
CallTarget(ProcRef(proc)) = proc
CallTarget(RecordCtor(p)) = RecordCtor(p)
MethodTarget(Dyn(Cl, RawPtr(`imm`, addr), T), name) = Dispatch(T, Cl, name)
MethodTarget(v_self, name) = m ∧ m.body = ⊥ ∧ ¬ ∃ vec_v, v. Γ ⊢ PrimCall(MethodOwner(m), MethodName(m), v_self, vec_v) ⇓ v ⇒ IllFormed(MethodTarget(v_self, name))
ArgVal = {v, Alias(addr)}
BindParams([⟨mode_1, x_1, T_1⟩, …, ⟨mode_n, x_n, T_n⟩], [v_1, …, v_n]) = [⟨x_1, v_1⟩, …, ⟨x_n, v_n⟩]
RecordDefaultInits(p) = [⟨f_1, e_1⟩, …, ⟨f_n, e_n⟩] ⇔ RecordDecl(p) = R ∧ Fields(R) = [⟨vis_1, f_1, T_1, e_1, span_1, doc_1⟩, …, ⟨vis_n, f_n, T_n, e_n, span_n, doc_n⟩] ∧ ∀ i. e_i ≠ ⊥
ReturnOut(Val(v)) = Val(v)
ReturnOut(Ctrl(Return(v))) = Val(v)
ReturnOut(Ctrl(Panic)) = Ctrl(Panic)
ReturnOut(Ctrl(Abort)) = Ctrl(Abort)
ReturnOut(Ctrl(Break(v_opt))) = ⊥
ReturnOut(Ctrl(Continue)) = ⊥
ReturnOut(out) = ⊥ ⇒ IllFormed(ReturnOut(out))
RecvArgMode(base) = `move` ⇔ ∃ p. base = MoveExpr(p)
RecvArgMode(base) = ⊥ ⇔ ¬ ∃ p. base = MoveExpr(p)

**(EvalArgsSigma-Empty)**
──────────────────────────────────────────────────────────────
Γ ⊢ EvalArgsSigma([], [], σ) ⇓ (Val([]), σ)

**(EvalArgsSigma-Cons-Move)**
Γ ⊢ EvalSigma(MovedArg(moved, e), σ) ⇓ (Val(v), σ_1)    Γ ⊢ EvalArgsSigma(ps, as, σ_1) ⇓ (Val(vec_v), σ_2)
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalArgsSigma([⟨`move`, x, T_p⟩] ++ ps, [⟨moved, e, _⟩] ++ as, σ) ⇓ (Val([v] ++ vec_v), σ_2)

**(EvalArgsSigma-Cons-Ref)**
Γ ⊢ AddrOfSigma(e, σ) ⇓ (Val(addr), σ_1)    Γ ⊢ EvalArgsSigma(ps, as, σ_1) ⇓ (Val(vec_v), σ_2)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalArgsSigma([⟨⊥, x, T_p⟩] ++ ps, [⟨moved, e, _⟩] ++ as, σ) ⇓ (Val([Alias(addr)] ++ vec_v), σ_2)

**(EvalArgsSigma-Ctrl-Move)**
Γ ⊢ EvalSigma(MovedArg(moved, e), σ) ⇓ (Ctrl(κ), σ_1)
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalArgsSigma([⟨`move`, x, T_p⟩] ++ ps, [⟨moved, e, _⟩] ++ as, σ) ⇓ (Ctrl(κ), σ_1)

**(EvalArgsSigma-Ctrl-Ref)**
Γ ⊢ AddrOfSigma(e, σ) ⇓ (Ctrl(κ), σ_1)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalArgsSigma([⟨⊥, x, T_p⟩] ++ ps, [⟨moved, e, _⟩] ++ as, σ) ⇓ (Ctrl(κ), σ_1)

**(EvalRecvSigma-Move)**
mode = `move`    Γ ⊢ EvalSigma(base, σ) ⇓ (Val(v_self), σ_1)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalRecvSigma(base, mode, σ) ⇓ (Val(⟨v_self, v_self⟩), σ_1)

**(EvalRecvSigma-Ref-Dyn)**
mode = ⊥    Γ ⊢ AddrOfSigma(base, σ) ⇓ (Val(addr), σ_1)    ReadAddr(σ_1, addr) = Dyn(Cl, RawPtr(`imm`, addr_d), T)    DynAddrState(σ_1, addr_d) = `Valid`
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalRecvSigma(base, mode, σ) ⇓ (Val(⟨Dyn(Cl, RawPtr(`imm`, addr_d), T), Alias(addr_d)⟩), σ_1)

**(EvalRecvSigma-Ref-Dyn-Expired)**
mode = ⊥    Γ ⊢ AddrOfSigma(base, σ) ⇓ (Val(addr), σ_1)    ReadAddr(σ_1, addr) = Dyn(Cl, RawPtr(`imm`, addr_d), T)    DynAddrState(σ_1, addr_d) = `Expired`
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalRecvSigma(base, mode, σ) ⇓ (Ctrl(Panic), σ_1)

**(EvalRecvSigma-Ref)**
mode = ⊥    Γ ⊢ AddrOfSigma(base, σ) ⇓ (Val(addr), σ_1)    ReadAddr(σ_1, addr) = v_self    ¬ (∃ Cl, addr_d, T. v_self = Dyn(Cl, RawPtr(`imm`, addr_d), T))
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalRecvSigma(base, mode, σ) ⇓ (Val(⟨v_self, Alias(addr)⟩), σ_1)

**(EvalRecvSigma-Ctrl-Move)**
mode = `move`    Γ ⊢ EvalSigma(base, σ) ⇓ (Ctrl(κ), σ_1)
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalRecvSigma(base, mode, σ) ⇓ (Ctrl(κ), σ_1)

**(EvalRecvSigma-Ctrl-Ref)**
mode = ⊥    Γ ⊢ AddrOfSigma(base, σ) ⇓ (Ctrl(κ), σ_1)
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalRecvSigma(base, mode, σ) ⇓ (Ctrl(κ), σ_1)

RegionProcParams(name) = params ⇔ RegionProcSig(`Region::`name) = ⟨params, ret⟩

**(ApplyRegionProc-NewScoped)**
name = `new_scoped`    vec_v = [opts]    RegionNewScoped(σ, opts) ⇓ (σ', v)
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ApplyRegionProc(name, vec_v, σ) ⇓ (Val(v), σ')

**(ApplyRegionProc-Alloc)**
name = `alloc`    vec_v = [v_r, v]    RegionAllocProc(σ, v_r, v) ⇓ (σ', v')
───────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ApplyRegionProc(name, vec_v, σ) ⇓ (Val(v'), σ')

**(ApplyRegionProc-Reset)**
name = `reset_unchecked`    vec_v = [v_r]    RegionResetProc(σ, v_r) ⇓ (σ', v')
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ApplyRegionProc(name, vec_v, σ) ⇓ (Val(v'), σ')

**(ApplyRegionProc-Freeze)**
name = `freeze`    vec_v = [v_r]    RegionFreezeProc(σ, v_r) ⇓ (σ', v')
───────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ApplyRegionProc(name, vec_v, σ) ⇓ (Val(v'), σ')

**(ApplyRegionProc-Thaw)**
name = `thaw`    vec_v = [v_r]    RegionThawProc(σ, v_r) ⇓ (σ', v')
────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ApplyRegionProc(name, vec_v, σ) ⇓ (Val(v'), σ')

**(ApplyRegionProc-Free)**
name = `free_unchecked`    vec_v = [v_r]    RegionFreeProc(σ, v_r) ⇓ (σ', v')
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ApplyRegionProc(name, vec_v, σ) ⇓ (Val(v'), σ')

**(ApplyProcSigma)**
BindParams(proc.params, vec_v) = binds    BlockEnter(σ, binds) ⇓ (σ_1, scope)    Γ ⊢ EvalBlockBodySigma(proc.body, σ_1) ⇓ (out, σ_2)    BlockExit(σ_2, scope, out) ⇓ (out', σ_3)
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ApplyProcSigma(proc, vec_v, σ) ⇓ (ReturnOut(out'), σ_3)

**(ApplyRecordCtorSigma)**
RecordDefaultInits(p) = fields    Γ ⊢ EvalFieldInitsSigma(fields, σ) ⇓ (Val(vec_f), σ_1)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ApplyRecordCtorSigma(p, σ) ⇓ (Val(RecordValue(TypePath(p), vec_f)), σ_1)

**(ApplyRecordCtorSigma-Ctrl)**
RecordDefaultInits(p) = fields    Γ ⊢ EvalFieldInitsSigma(fields, σ) ⇓ (Ctrl(κ), σ_1)
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ApplyRecordCtorSigma(p, σ) ⇓ (Ctrl(κ), σ_1)

**(ApplyMethodSigma-Prim)**
m = MethodTarget(v_self, name)    MethodOwner(m) = owner    MethodName(m) = name    Γ ⊢ PrimCall(owner, name, v_self, vec_v) ⇓ v
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ApplyMethodSigma(base, name, v_self, v_arg, vec_v, σ) ⇓ (Val(v), σ)

**(ApplyMethodSigma)**
m = MethodTarget(v_self, name)    BindParams(RecvParams(base, name), [v_arg] ++ vec_v) = binds    BlockEnter(σ, binds) ⇓ (σ_1, scope)    Γ ⊢ EvalBlockBodySigma(m.body, σ_1) ⇓ (out, σ_2)    BlockExit(σ_2, scope, out) ⇓ (out', σ_3)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ApplyMethodSigma(base, name, v_self, v_arg, vec_v, σ) ⇓ (ReturnOut(out'), σ_3)

**(EvalSigma-Call-RegionProc)**
Γ ⊢ EvalSigma(callee, σ) ⇓ (Val(ProcRef([`Region`], name)), σ_1)    RegionProcParams(name) = params    Γ ⊢ EvalArgsSigma(params, args, σ_1) ⇓ (Val(vec_v), σ_2)    Γ ⊢ ApplyRegionProc(name, vec_v, σ_2) ⇓ (out, σ_3)
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(Call(callee, args), σ) ⇓ (out, σ_3)

**(EvalSigma-Call-RegionProc-Ctrl-Args)**
Γ ⊢ EvalSigma(callee, σ) ⇓ (Val(ProcRef([`Region`], name)), σ_1)    RegionProcParams(name) = params    Γ ⊢ EvalArgsSigma(params, args, σ_1) ⇓ (Ctrl(κ), σ_2)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(Call(callee, args), σ) ⇓ (Ctrl(κ), σ_2)

**(EvalSigma-Call-Proc)**
Γ ⊢ EvalSigma(callee, σ) ⇓ (Val(v_c), σ_1)    proc = CallTarget(v_c)    Γ ⊢ EvalArgsSigma(proc.params, args, σ_1) ⇓ (Val(vec_v), σ_2)    Γ ⊢ ApplyProcSigma(proc, vec_v, σ_2) ⇓ (out, σ_3)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(Call(callee, args), σ) ⇓ (out, σ_3)

**(EvalSigma-Call-Record)**
Γ ⊢ EvalSigma(callee, σ) ⇓ (Val(v_c), σ_1)    Γ ⊢ EvalArgsSigma([], args, σ_1) ⇓ (Val(vec_v), σ_2)    vec_v = []    RecordCtor(p) = CallTarget(v_c)    Γ ⊢ ApplyRecordCtorSigma(p, σ_2) ⇓ (out, σ_3)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(Call(callee, args), σ) ⇓ (out, σ_3)

**(EvalSigma-Call-Ctrl)**
Γ ⊢ EvalSigma(callee, σ) ⇓ (Ctrl(κ), σ_1)
──────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(Call(callee, args), σ) ⇓ (Ctrl(κ), σ_1)

**(EvalSigma-Call-Ctrl-Args)**
Γ ⊢ EvalSigma(callee, σ) ⇓ (Val(v_c), σ_1)    proc = CallTarget(v_c)    Γ ⊢ EvalArgsSigma(proc.params, args, σ_1) ⇓ (Ctrl(κ), σ_2)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(Call(callee, args), σ) ⇓ (Ctrl(κ), σ_2)

**(EvalSigma-MethodCall)**
mode = RecvArgMode(base)    Γ ⊢ EvalRecvSigma(base, mode, σ) ⇓ (Val(⟨v_self, v_arg⟩), σ_1)    m = MethodTarget(v_self, name)    Γ ⊢ EvalArgsSigma(m.params, args, σ_1) ⇓ (Val(vec_v), σ_2)    Γ ⊢ ApplyMethodSigma(base, name, v_self, v_arg, vec_v, σ_2) ⇓ (out, σ_3)
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(MethodCall(base, name, args), σ) ⇓ (out, σ_3)

**(EvalSigma-MethodCall-Ctrl)**
mode = RecvArgMode(base)    Γ ⊢ EvalRecvSigma(base, mode, σ) ⇓ (Ctrl(κ), σ_1)
────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(MethodCall(base, name, args), σ) ⇓ (Ctrl(κ), σ_1)

**(EvalSigma-MethodCall-Ctrl-Args)**
mode = RecvArgMode(base)    Γ ⊢ EvalRecvSigma(base, mode, σ) ⇓ (Val(⟨v_self, v_arg⟩), σ_1)    m = MethodTarget(v_self, name)    Γ ⊢ EvalArgsSigma(m.params, args, σ_1) ⇓ (Ctrl(κ), σ_2)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(MethodCall(base, name, args), σ) ⇓ (Ctrl(κ), σ_2)

**(EvalSigma-Alloc-Implicit)**
Γ ⊢ EvalSigma(e, σ) ⇓ (Val(v), σ_1)    ActiveTarget(σ_1) = r    RegionAlloc(σ_1, r, v) ⇓ (σ_2, v')
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(AllocExpr(⊥, e), σ) ⇓ (Val(v'), σ_2)

**(EvalSigma-Alloc-Implicit-Ctrl)**
Γ ⊢ EvalSigma(e, σ) ⇓ (Ctrl(κ), σ_1)
──────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(AllocExpr(⊥, e), σ) ⇓ (Ctrl(κ), σ_1)

**(EvalSigma-Alloc-Explicit)**
Γ ⊢ EvalSigma(e, σ) ⇓ (Val(v), σ_1)    LookupVal(σ_1, r) = v_r    RegionHandleOf(v_r) = h    ResolveTarget(σ_1, h) = r_t    RegionAlloc(σ_1, r_t, v) ⇓ (σ_2, v')
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(AllocExpr(r, e), σ) ⇓ (Val(v'), σ_2)

**(EvalSigma-Alloc-Explicit-Ctrl)**
Γ ⊢ EvalSigma(e, σ) ⇓ (Ctrl(κ), σ_1)
────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(AllocExpr(r, e), σ) ⇓ (Ctrl(κ), σ_1)

**(EvalSigma-Block)**
Γ ⊢ EvalBlockSigma(BlockExpr(stmts, tail_opt), σ) ⇓ (out, σ')
──────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(BlockExpr(stmts, tail_opt), σ) ⇓ (out, σ')

**(EvalSigma-UnsafeBlock)**
Γ ⊢ EvalBlockSigma(b, σ) ⇓ (out, σ')
────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(UnsafeBlockExpr(b), σ) ⇓ (out, σ')

**Loop Iteration Helpers.**

IterJudg = {IterInit(v) ⇓ it, IterNext(it) ⇓ (opt(v), it')}
Iter = {⟨v, i⟩ | Len(v) defined ∧ i ∈ ℕ}
IterInit(v) ⇓ ⟨v, 0⟩ ⇔ Len(v) defined
IterNext(⟨v, i⟩) ⇓ (⊥, ⟨v, i⟩) ⇔ ¬ (0 ≤ i < Len(v))
IterNext(⟨v, i⟩) ⇓ (v_i, ⟨v, i + 1⟩) ⇔ 0 ≤ i < Len(v) ∧ IndexValue(v, i) = v_i

LoopIterJudg = {Γ ⊢ LoopIterExec(p, b, it, σ) ⇓ (out, σ')}

**(EvalSigma-Loop-Infinite-Step)**
Γ ⊢ EvalSigma(body, σ) ⇓ (Val(v), σ_1)    Γ ⊢ EvalSigma(LoopInfinite(body), σ_1) ⇓ (out, σ_2)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(LoopInfinite(body), σ) ⇓ (out, σ_2)

**(EvalSigma-Loop-Infinite-Continue)**
Γ ⊢ EvalSigma(body, σ) ⇓ (Ctrl(Continue), σ_1)    Γ ⊢ EvalSigma(LoopInfinite(body), σ_1) ⇓ (out, σ_2)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(LoopInfinite(body), σ) ⇓ (out, σ_2)

**(EvalSigma-Loop-Infinite-Break)**
Γ ⊢ EvalSigma(body, σ) ⇓ (Ctrl(Break(v_opt)), σ_1)    v = BreakVal(v_opt)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(LoopInfinite(body), σ) ⇓ (Val(v), σ_1)

**(EvalSigma-Loop-Infinite-Ctrl)**
Γ ⊢ EvalSigma(body, σ) ⇓ (Ctrl(κ), σ_1)    κ ∈ {Return(_), Panic, Abort}
─────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(LoopInfinite(body), σ) ⇓ (Ctrl(κ), σ_1)

**(EvalSigma-Loop-Cond-False)**
Γ ⊢ EvalSigma(cond, σ) ⇓ (Val(false), σ_1)
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(LoopConditional(cond, body), σ) ⇓ (Val(()), σ_1)

**(EvalSigma-Loop-Cond-True-Step)**
Γ ⊢ EvalSigma(cond, σ) ⇓ (Val(true), σ_1)    Γ ⊢ EvalSigma(body, σ_1) ⇓ (Val(v), σ_2)    Γ ⊢ EvalSigma(LoopConditional(cond, body), σ_2) ⇓ (out, σ_3)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(LoopConditional(cond, body), σ) ⇓ (out, σ_3)

**(EvalSigma-Loop-Cond-Continue)**
Γ ⊢ EvalSigma(cond, σ) ⇓ (Val(true), σ_1)    Γ ⊢ EvalSigma(body, σ_1) ⇓ (Ctrl(Continue), σ_2)    Γ ⊢ EvalSigma(LoopConditional(cond, body), σ_2) ⇓ (out, σ_3)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(LoopConditional(cond, body), σ) ⇓ (out, σ_3)

**(EvalSigma-Loop-Cond-Break)**
Γ ⊢ EvalSigma(cond, σ) ⇓ (Val(true), σ_1)    Γ ⊢ EvalSigma(body, σ_1) ⇓ (Ctrl(Break(v_opt)), σ_2)    v = BreakVal(v_opt)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(LoopConditional(cond, body), σ) ⇓ (Val(v), σ_2)

**(EvalSigma-Loop-Cond-Ctrl)**
Γ ⊢ EvalSigma(cond, σ) ⇓ (Ctrl(κ), σ_1)
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(LoopConditional(cond, body), σ) ⇓ (Ctrl(κ), σ_1)

**(EvalSigma-Loop-Cond-Body-Ctrl)**
Γ ⊢ EvalSigma(cond, σ) ⇓ (Val(true), σ_1)    Γ ⊢ EvalSigma(body, σ_1) ⇓ (Ctrl(κ), σ_2)    κ ∈ {Return(_), Panic, Abort}
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(LoopConditional(cond, body), σ) ⇓ (Ctrl(κ), σ_2)

**(EvalSigma-Loop-Iter)**
Γ ⊢ EvalSigma(iter, σ) ⇓ (Val(v_iter), σ_1)    IterInit(v_iter) ⇓ it    Γ ⊢ LoopIterExec(pat, body, it, σ_1) ⇓ (out, σ_2)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(LoopIter(pat, ty_opt, iter, body), σ) ⇓ (out, σ_2)

**(EvalSigma-Loop-Iter-Ctrl)**
Γ ⊢ EvalSigma(iter, σ) ⇓ (Ctrl(κ), σ_1)
───────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(LoopIter(pat, ty_opt, iter, body), σ) ⇓ (Ctrl(κ), σ_1)

**(LoopIter-Done)**
IterNext(it) ⇓ (⊥, it')
──────────────────────────────────────────────────────────────
Γ ⊢ LoopIterExec(pat, body, it, σ) ⇓ (Val(()), σ)

**(LoopIter-Step-Val)**
IterNext(it) ⇓ (v, it')    Γ ⊢ EvalBlockBindSigma(pat, v, body, σ) ⇓ (Val(v_b), σ_1)    Γ ⊢ LoopIterExec(pat, body, it', σ_1) ⇓ (out, σ_2)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LoopIterExec(pat, body, it, σ) ⇓ (out, σ_2)

**(LoopIter-Step-Continue)**
IterNext(it) ⇓ (v, it')    Γ ⊢ EvalBlockBindSigma(pat, v, body, σ) ⇓ (Ctrl(Continue), σ_1)    Γ ⊢ LoopIterExec(pat, body, it', σ_1) ⇓ (out, σ_2)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LoopIterExec(pat, body, it, σ) ⇓ (out, σ_2)

**(LoopIter-Step-Break)**
IterNext(it) ⇓ (v, it')    Γ ⊢ EvalBlockBindSigma(pat, v, body, σ) ⇓ (Ctrl(Break(v_opt)), σ_1)    v' = BreakVal(v_opt)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LoopIterExec(pat, body, it, σ) ⇓ (Val(v'), σ_1)

**(LoopIter-Step-Ctrl)**
IterNext(it) ⇓ (v, it')    Γ ⊢ EvalBlockBindSigma(pat, v, body, σ) ⇓ (Ctrl(κ), σ_1)    κ ∈ {Return(_), Panic, Abort}
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ LoopIterExec(pat, body, it, σ) ⇓ (Ctrl(κ), σ_1)

**Stateful Small-Step (Expressions).**

ExprState = {⟨e, σ⟩, ⟨Val(v), σ⟩, ⟨Ctrl(κ), σ⟩}
TerminalExpr(⟨Val(v), σ⟩)
TerminalExpr(⟨Ctrl(κ), σ⟩)

**(StepSigma-Pure)**
⟨e⟩ → ⟨e'⟩
────────────────────────────────
⟨e, σ⟩ → ⟨e', σ⟩

**(StepSigma-Alloc-Implicit)**
Γ ⊢ EvalSigma(e, σ) ⇓ (Val(v), σ_1)    ActiveTarget(σ_1) = r    RegionAlloc(σ_1, r, v) ⇓ (σ_2, v')
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨AllocExpr(⊥, e), σ⟩ → ⟨Val(v'), σ_2⟩

**(StepSigma-Alloc-Implicit-Ctrl)**
Γ ⊢ EvalSigma(e, σ) ⇓ (Ctrl(κ), σ_1)
────────────────────────────────────────────────────────────
⟨AllocExpr(⊥, e), σ⟩ → ⟨Ctrl(κ), σ_1⟩

**(StepSigma-Alloc-Explicit)**
Γ ⊢ EvalSigma(e, σ) ⇓ (Val(v), σ_1)    LookupVal(σ_1, r) = v_r    RegionHandleOf(v_r) = h    ResolveTarget(σ_1, h) = r_t    RegionAlloc(σ_1, r_t, v) ⇓ (σ_2, v')
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨AllocExpr(r, e), σ⟩ → ⟨Val(v'), σ_2⟩

**(StepSigma-Alloc-Explicit-Ctrl)**
Γ ⊢ EvalSigma(e, σ) ⇓ (Ctrl(κ), σ_1)
──────────────────────────────────────────────
⟨AllocExpr(r, e), σ⟩ → ⟨Ctrl(κ), σ_1⟩

**(StepSigma-Block)**
Γ ⊢ EvalBlockSigma(BlockExpr(stmts, tail_opt), σ) ⇓ (out, σ')
────────────────────────────────────────────────────────────────────────────
⟨BlockExpr(stmts, tail_opt), σ⟩ → ⟨out, σ'⟩

**(StepSigma-UnsafeBlock)**
Γ ⊢ EvalBlockSigma(b, σ) ⇓ (out, σ')
────────────────────────────────────────────────────────
⟨UnsafeBlockExpr(b), σ⟩ → ⟨out, σ'⟩

**(StepSigma-Loop)**
Γ ⊢ EvalSigma(ℓ, σ) ⇓ (out, σ')    ℓ ∈ {LoopInfinite(_), LoopConditional(_, _), LoopIter(_, _, _, _)}
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨ℓ, σ⟩ → ⟨out, σ'⟩

**(StepSigma-Stateful-Other)**
Γ ⊢ EvalSigma(e, σ) ⇓ (out, σ')    e ∉ {AllocExpr(_, _), BlockExpr(_, _), UnsafeBlockExpr(_), LoopInfinite(_), LoopConditional(_, _), LoopIter(_, _, _, _)}
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨e, σ⟩ → ⟨out, σ'⟩

**Statement Execution (Cursive0).**

ExecJudg = {Γ ⊢ ExecSigma(s, σ) ⇓ (sout, σ'), Γ ⊢ ExecSeqSigma(ss, σ) ⇓ (sout, σ')}

**(ExecSeq-Empty)**
──────────────────────────────────────────────
Γ ⊢ ExecSeqSigma([], σ) ⇓ (ok, σ)

**(ExecSeq-Cons-Ok)**
Γ ⊢ ExecSigma(s, σ) ⇓ (ok, σ_1)    Γ ⊢ ExecSeqSigma(ss, σ_1) ⇓ (sout, σ_2)
──────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ExecSeqSigma(s::ss, σ) ⇓ (sout, σ_2)

**(ExecSeq-Cons-Ctrl)**
Γ ⊢ ExecSigma(s, σ) ⇓ (Ctrl(κ), σ_1)
───────────────────────────────────────────────────────────────
Γ ⊢ ExecSeqSigma(s::ss, σ) ⇓ (Ctrl(κ), σ_1)

BindingForm(binding) = ⟨pat, ty_opt, op, init, _⟩

**(ExecSigma-Let)**
BindingForm(binding) = ⟨pat, ty_opt, op, init, _⟩    Γ ⊢ EvalSigma(init, σ) ⇓ (Val(v), σ_1)    BindPattern(σ_1, pat, v) ⇓ (σ_2, bs)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ExecSigma(LetStmt(binding), σ) ⇓ (ok, σ_2)

**(ExecSigma-Let-Ctrl)**
BindingForm(binding) = ⟨pat, ty_opt, op, init, _⟩    Γ ⊢ EvalSigma(init, σ) ⇓ (Ctrl(κ), σ_1)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ExecSigma(LetStmt(binding), σ) ⇓ (Ctrl(κ), σ_1)

**(ExecSigma-Var)**
BindingForm(binding) = ⟨pat, ty_opt, op, init, _⟩    Γ ⊢ EvalSigma(init, σ) ⇓ (Val(v), σ_1)    BindPattern(σ_1, pat, v) ⇓ (σ_2, bs)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ExecSigma(VarStmt(binding), σ) ⇓ (ok, σ_2)

**(ExecSigma-Var-Ctrl)**
BindingForm(binding) = ⟨pat, ty_opt, op, init, _⟩    Γ ⊢ EvalSigma(init, σ) ⇓ (Ctrl(κ), σ_1)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ExecSigma(VarStmt(binding), σ) ⇓ (Ctrl(κ), σ_1)

**(ExecSigma-ShadowLet)**
Γ ⊢ EvalSigma(init, σ) ⇓ (Val(v), σ_1)    BindVal(σ_1, x, v) ⇓ (σ_2, b)
────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ExecSigma(ShadowLetStmt(x, ty_opt, init), σ) ⇓ (ok, σ_2)

**(ExecSigma-ShadowLet-Ctrl)**
Γ ⊢ EvalSigma(init, σ) ⇓ (Ctrl(κ), σ_1)
────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ExecSigma(ShadowLetStmt(x, ty_opt, init), σ) ⇓ (Ctrl(κ), σ_1)

**(ExecSigma-ShadowVar)**
Γ ⊢ EvalSigma(init, σ) ⇓ (Val(v), σ_1)    BindVal(σ_1, x, v) ⇓ (σ_2, b)
────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ExecSigma(ShadowVarStmt(x, ty_opt, init), σ) ⇓ (ok, σ_2)

**(ExecSigma-ShadowVar-Ctrl)**
Γ ⊢ EvalSigma(init, σ) ⇓ (Ctrl(κ), σ_1)
────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ExecSigma(ShadowVarStmt(x, ty_opt, init), σ) ⇓ (Ctrl(κ), σ_1)

**(ExecSigma-Assign)**
Γ ⊢ EvalSigma(e, σ) ⇓ (Val(v), σ_1)    Γ ⊢ WritePlaceSigma(p, v, σ_1) ⇓ (sout, σ_2)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ExecSigma(AssignStmt(p, e), σ) ⇓ (sout, σ_2)

**(ExecSigma-Assign-Ctrl)**
Γ ⊢ EvalSigma(e, σ) ⇓ (Ctrl(κ), σ_1)
────────────────────────────────────────────────────────────────
Γ ⊢ ExecSigma(AssignStmt(p, e), σ) ⇓ (Ctrl(κ), σ_1)


**(ExecSigma-CompoundAssign)**
Γ ⊢ ReadPlaceSigma(p, σ) ⇓ (Val(v_p), σ_1)    Γ ⊢ EvalSigma(e, σ_1) ⇓ (Val(v_e), σ_2)    BinOp(op, v_p, v_e) ⇓ v    Γ ⊢ WritePlaceSigma(p, v, σ_2) ⇓ (sout, σ_3)
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ExecSigma(CompoundAssignStmt(p, op, e), σ) ⇓ (sout, σ_3)

**(ExecSigma-CompoundAssign-Left-Ctrl)**
Γ ⊢ ReadPlaceSigma(p, σ) ⇓ (Ctrl(κ), σ_1)
───────────────────────────────────────────────────────────────────────────────
Γ ⊢ ExecSigma(CompoundAssignStmt(p, op, e), σ) ⇓ (Ctrl(κ), σ_1)

**(ExecSigma-CompoundAssign-Right-Ctrl)**
Γ ⊢ ReadPlaceSigma(p, σ) ⇓ (Val(v_p), σ_1)    Γ ⊢ EvalSigma(e, σ_1) ⇓ (Ctrl(κ), σ_2)
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ExecSigma(CompoundAssignStmt(p, op, e), σ) ⇓ (Ctrl(κ), σ_2)

**(ExecSigma-ExprStmt)**
Γ ⊢ EvalSigma(e, σ) ⇓ (out, σ_1)
────────────────────────────────────────────────────────────────
Γ ⊢ ExecSigma(ExprStmt(e), σ) ⇓ (StmtOutOf(out), σ_1)

**(ExecSigma-UnsafeStmt)**
Γ ⊢ EvalSigma(b, σ) ⇓ (out, σ_1)
──────────────────────────────────────────────────────────────────────
Γ ⊢ ExecSigma(UnsafeBlockStmt(b), σ) ⇓ (StmtOutOf(out), σ_1)

**(ExecSigma-Defer)**
AppendCleanup(σ, DeferBlock(b)) ⇓ σ'
──────────────────────────────────────────────────────────────
Γ ⊢ ExecSigma(DeferStmt(b), σ) ⇓ (ok, σ')

opts = RegionOptsExpr(opts_opt)

**(ExecSigma-Region)**
Γ ⊢ EvalSigma(opts, σ) ⇓ (Val(v_o), σ_1)    RegionNew(σ_1, v_o) ⇓ (σ_2, r, scope)    BindRegionAlias(σ_2, alias_opt, r) ⇓ σ_3    Γ ⊢ EvalInScopeSigma(b, σ_3, scope) ⇓ (out, σ_4)    RegionRelease(σ_4, r, scope, out) ⇓ (out', σ_5)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ExecSigma(RegionStmt(opts_opt, alias_opt, b), σ) ⇓ (StmtOutOf(out'), σ_5)

**(ExecSigma-Region-Ctrl)**
Γ ⊢ EvalSigma(opts, σ) ⇓ (Ctrl(κ), σ_1)
───────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ExecSigma(RegionStmt(opts_opt, alias_opt, b), σ) ⇓ (Ctrl(κ), σ_1)

RegionRelease(Σ, r, scope, out) ⇓ (out', Σ') ⇔ Γ ⊢ CleanupScope(scope, Σ) ⇓ (c, Σ_1) ∧ out' = ExitOutcome(out, c) ∧ ((out' = Ctrl(Abort) ∧ Σ' = Σ_1) ∨ (out' ≠ Ctrl(Abort) ∧ ReleaseArena(Σ_1, r) ⇓ Σ_2 ∧ PopScope_σ(Σ_2) ⇓ (Σ', scope)))

**(ExecSigma-Frame-Implicit)**
ActiveTarget(σ) = r    FrameEnter(σ, r) ⇓ (σ_1, F, scope, mark)    Γ ⊢ EvalInScopeSigma(b, σ_1, scope) ⇓ (out, σ_2)    FrameReset(σ_2, r, scope, mark, out) ⇓ (out', σ_3)
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ExecSigma(FrameStmt(⊥, b), σ) ⇓ (StmtOutOf(out'), σ_3)

**(ExecSigma-Frame-Explicit)**
LookupVal(σ, r) = v_r    RegionHandleOf(v_r) = h    ResolveTarget(σ, h) = r_t    FrameEnter(σ, r_t) ⇓ (σ_1, F, scope, mark)    Γ ⊢ EvalInScopeSigma(b, σ_1, scope) ⇓ (out, σ_2)    FrameReset(σ_2, r_t, scope, mark, out) ⇓ (out', σ_3)
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ExecSigma(FrameStmt(r, b), σ) ⇓ (StmtOutOf(out'), σ_3)

FrameReset(Σ, r, scope, mark, out) ⇓ (out', Σ') ⇔ Γ ⊢ CleanupScope(scope, Σ) ⇓ (c, Σ_1) ∧ out' = ExitOutcome(out, c) ∧ ((out' = Ctrl(Abort) ∧ Σ' = Σ_1) ∨ (out' ≠ Ctrl(Abort) ∧ ResetArena(Σ_1, r, mark) ⇓ Σ_2 ∧ PopScope_σ(Σ_2) ⇓ (Σ', scope)))

**(ExecSigma-Return)**
Γ ⊢ EvalSigma(e, σ) ⇓ (Val(v), σ_1)
──────────────────────────────────────────────────────────
Γ ⊢ ExecSigma(ReturnStmt(e), σ) ⇓ (Ctrl(Return(v)), σ_1)

**(ExecSigma-Return-Unit)**
───────────────────────────────────────────────────────
Γ ⊢ ExecSigma(ReturnStmt(⊥), σ) ⇓ (Ctrl(Return(())), σ)

**(ExecSigma-Return-Ctrl)**
Γ ⊢ EvalSigma(e, σ) ⇓ (Ctrl(κ), σ_1)
───────────────────────────────────────────────────────
Γ ⊢ ExecSigma(ReturnStmt(e), σ) ⇓ (Ctrl(κ), σ_1)

**(ExecSigma-Result)**
Γ ⊢ EvalSigma(e, σ) ⇓ (Val(v), σ_1)
──────────────────────────────────────────────────────────
Γ ⊢ ExecSigma(ResultStmt(e), σ) ⇓ (Ctrl(Result(v)), σ_1)

**(ExecSigma-Result-Ctrl)**
Γ ⊢ EvalSigma(e, σ) ⇓ (Ctrl(κ), σ_1)
───────────────────────────────────────────────────────
Γ ⊢ ExecSigma(ResultStmt(e), σ) ⇓ (Ctrl(κ), σ_1)

**(ExecSigma-Break)**
Γ ⊢ EvalSigma(e, σ) ⇓ (Val(v), σ_1)
──────────────────────────────────────────────────────────
Γ ⊢ ExecSigma(BreakStmt(e), σ) ⇓ (Ctrl(Break(v)), σ_1)

**(ExecSigma-Break-Unit)**
───────────────────────────────────────────────────────
Γ ⊢ ExecSigma(BreakStmt(⊥), σ) ⇓ (Ctrl(Break(⊥)), σ)

**(ExecSigma-Break-Ctrl)**
Γ ⊢ EvalSigma(e, σ) ⇓ (Ctrl(κ), σ_1)
───────────────────────────────────────────────────────
Γ ⊢ ExecSigma(BreakStmt(e), σ) ⇓ (Ctrl(κ), σ_1)

**(ExecSigma-Continue)**
──────────────────────────────────────────────────────────
Γ ⊢ ExecSigma(ContinueStmt, σ) ⇓ (Ctrl(Continue), σ)

**(ExecSigma-Error)**
──────────────────────────────────────────────────────────
Γ ⊢ ExecSigma(ErrorStmt(_), σ) ⇓ (Ctrl(Panic), σ)

**Stateful Small-Step (Statements).**

ExecState = {Exec(s, σ), ExecSeq(ss, σ), ExecCtrl(κ, σ), ExecDone(σ), RegionBody(r, scope, b, σ), RegionExit(r, scope, out, σ), FrameBody(r, scope, mark, b, σ), FrameExit(r, scope, mark, out, σ)}

**(Step-Exec-Other-Ok)**
s ∉ {DeferStmt(_), RegionStmt(_, _, _), FrameStmt(_, _)}    Γ ⊢ ExecSigma(s, σ) ⇓ (ok, σ')
────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨Exec(s, σ)⟩ → ⟨ExecDone(σ')⟩

**(Step-Exec-Other-Ctrl)**
s ∉ {DeferStmt(_), RegionStmt(_, _, _), FrameStmt(_, _)}    Γ ⊢ ExecSigma(s, σ) ⇓ (Ctrl(κ), σ')
─────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨Exec(s, σ)⟩ → ⟨ExecCtrl(κ, σ')⟩

**(Step-ExecSeq-Ok)**
Γ ⊢ ExecSeqSigma(ss, σ) ⇓ (ok, σ')
────────────────────────────────────────────────────────
⟨ExecSeq(ss, σ)⟩ → ⟨ExecDone(σ')⟩

**(Step-ExecSeq-Ctrl)**
Γ ⊢ ExecSeqSigma(ss, σ) ⇓ (Ctrl(κ), σ')
─────────────────────────────────────────────────────────
⟨ExecSeq(ss, σ)⟩ → ⟨ExecCtrl(κ, σ')⟩

**(Step-Exec-Defer)**
AppendCleanup(σ, DeferBlock(b)) ⇓ σ'
──────────────────────────────────────────────────────
⟨Exec(DeferStmt(b), σ)⟩ → ⟨ExecDone(σ')⟩

**(Step-Exec-Region-Enter)**
opts = RegionOptsExpr(opts_opt)    Γ ⊢ EvalSigma(opts, σ) ⇓ (Val(v_o), σ_1)    RegionNew(σ_1, v_o) ⇓ (σ_2, r, scope)    BindRegionAlias(σ_2, alias_opt, r) ⇓ σ_3
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨Exec(RegionStmt(opts_opt, alias_opt, b), σ)⟩ → ⟨RegionBody(r, scope, b, σ_3)⟩

**(Step-Exec-Region-Enter-Ctrl)**
opts = RegionOptsExpr(opts_opt)    Γ ⊢ EvalSigma(opts, σ) ⇓ (Ctrl(κ), σ_1)
────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨Exec(RegionStmt(opts_opt, alias_opt, b), σ)⟩ → ⟨ExecCtrl(κ, σ_1)⟩

**(Step-Exec-Region-Body)**
Γ ⊢ EvalInScopeSigma(b, σ, scope) ⇓ (out, σ_1)
────────────────────────────────────────────────────────────────────────────
⟨RegionBody(r, scope, b, σ)⟩ → ⟨RegionExit(r, scope, out, σ_1)⟩

**(Step-Exec-Region-Exit-Ok)**
RegionRelease(σ, r, scope, out) ⇓ (out', σ')    StmtOutOf(out') = ok
────────────────────────────────────────────────────────────────────────────────────────
⟨RegionExit(r, scope, out, σ)⟩ → ⟨ExecDone(σ')⟩

**(Step-Exec-Region-Exit-Ctrl)**
RegionRelease(σ, r, scope, out) ⇓ (out', σ')    StmtOutOf(out') = Ctrl(κ)
───────────────────────────────────────────────────────────────────────────────────────────
⟨RegionExit(r, scope, out, σ)⟩ → ⟨ExecCtrl(κ, σ')⟩

**(Step-Exec-Frame-Enter-Implicit)**
ActiveTarget(σ) = r    FrameEnter(σ, r) ⇓ (σ_1, F, scope, mark)
────────────────────────────────────────────────────────────────────────────────
⟨Exec(FrameStmt(⊥, b), σ)⟩ → ⟨FrameBody(r, scope, mark, b, σ_1)⟩

**(Step-Exec-Frame-Enter-Explicit)**
LookupVal(σ, r) = v_r    RegionHandleOf(v_r) = h    ResolveTarget(σ, h) = r_t    FrameEnter(σ, r_t) ⇓ (σ_1, F, scope, mark)
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
⟨Exec(FrameStmt(r, b), σ)⟩ → ⟨FrameBody(r_t, scope, mark, b, σ_1)⟩

**(Step-Exec-Frame-Body)**
Γ ⊢ EvalInScopeSigma(b, σ, scope) ⇓ (out, σ_1)
────────────────────────────────────────────────────────────────────────────
⟨FrameBody(r, scope, mark, b, σ)⟩ → ⟨FrameExit(r, scope, mark, out, σ_1)⟩

**(Step-Exec-Frame-Exit-Ok)**
FrameReset(σ, r, scope, mark, out) ⇓ (out', σ')    StmtOutOf(out') = ok
──────────────────────────────────────────────────────────────────────────────────────
⟨FrameExit(r, scope, mark, out, σ)⟩ → ⟨ExecDone(σ')⟩

**(Step-Exec-Frame-Exit-Ctrl)**
FrameReset(σ, r, scope, mark, out) ⇓ (out', σ')    StmtOutOf(out') = Ctrl(κ)
─────────────────────────────────────────────────────────────────────────────────────────
⟨FrameExit(r, scope, mark, out, σ)⟩ → ⟨ExecCtrl(κ, σ')⟩

ConstPat(p) = v ⇔ p = LiteralPattern(ℓ) ∧ v = LiteralValue(ℓ, PatType(p))

**MatchRecord.**
MatchRecordJudg = {Γ ⊢ MatchRecord(fs, v) ⇓ B}

**(MatchRecord-Empty)**
──────────────────────────────────────────────
Γ ⊢ MatchRecord([], v) ⇓ ∅

**(MatchRecord-Cons-Implicit)**
FieldValue(v, f) = v_f    Γ ⊢ MatchRecord(fs, v) ⇓ B
──────────────────────────────────────────────────────────────────────
Γ ⊢ MatchRecord([f] ++ fs, v) ⇓ (B ⊎ {f ↦ v_f})

**(MatchRecord-Cons-Explicit)**
FieldValue(v, f) = v_f    Γ ⊢ MatchPattern(p, v_f) ⇓ B_1    Γ ⊢ MatchRecord(fs, v) ⇓ B_2
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ MatchRecord([⟨f, p⟩] ++ fs, v) ⇓ (B_1 ⊎ B_2)

### 7.5. String Literal Semantics

StringLiteralVal(lit) = v ⇔ LiteralValue(lit, TypeString(`@View`)) = v

**String Literal Storage.**
For any string literal `lit`, evaluation MUST allocate `StringBytes(lit)` in static, read-only storage. The resulting `string@View` value MUST reference that storage and MUST have length `|StringBytes(lit)|`. The backing storage MUST have static duration and MUST NOT be deallocated. See Â§6.1.5 for the `string@View` layout.
<!-- Source: "Literal content is allocated in static, read-only memory at compilation ... A string@View value is constructed with pointer to static memory and byte length ... String literals have static storage duration; backing memory is never deallocated." -->

### 7.6. Dynamic Class Objects

DynValue(Cl, addr, T) = Dyn(Cl, RawPtr(`imm`, addr), T)
DynLayout(Cl, addr, T) = ⟨RawPtr(`imm`, addr), VTable(T, Cl)⟩

**(Eval-Dynamic-Form)**
IsPlace(e)    Γ ⊢ AddrOfSigma(e, σ) ⇓ (Val(addr), σ_1)    T_e = ExprType(e)    T = StripPerm(T_e)    Γ ⊢ T <: Cl
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(e `as` TypeDynamic(Cl), σ) ⇓ (Val(Dyn(Cl, RawPtr(`imm`, addr), T)), σ_1)

**(Eval-Dynamic-Form-Ctrl)**
Γ ⊢ AddrOfSigma(e, σ) ⇓ (Ctrl(κ), σ_1)
──────────────────────────────────────────────────────────────────────────────
Γ ⊢ EvalSigma(e `as` TypeDynamic(Cl), σ) ⇓ (Ctrl(κ), σ_1)

**Dynamic Dispatch.**

Dispatch(T, Cl, name) = m' ⇔ m = LookupClassMethod(Cl, name) ∧ MethodByName(T, name) = m' ∧ Sig_T(T, m') = Sig_T(T, m)
Dispatch(T, Cl, name) = m ⇔ m = LookupClassMethod(Cl, name) ∧ (MethodByName(T, name) = ⊥ ∨ (∃ m'. MethodByName(T, name) = m' ∧ Sig_T(T, m') ≠ Sig_T(T, m))) ∧ m.body ≠ ⊥
Dispatch(T, Cl, name) = ⊥ ⇔ m = LookupClassMethod(Cl, name) ∧ (MethodByName(T, name) = ⊥ ∨ (∃ m'. MethodByName(T, name) = m' ∧ Sig_T(T, m') ≠ Sig_T(T, m))) ∧ m.body = ⊥


### 7.7. FileSystem and File Operations

**Primitive Relations.**

FSJudg = {FSOpenRead(fs, path) ⇓ r, FSOpenWrite(fs, path) ⇓ r, FSOpenAppend(fs, path) ⇓ r, FSCreateWrite(fs, path) ⇓ r, FSReadFile(fs, path) ⇓ r, FSReadBytes(fs, path) ⇓ r, FSWriteFile(fs, path, data) ⇓ r, FSWriteStdout(fs, data) ⇓ r, FSWriteStderr(fs, data) ⇓ r, FSExists(fs, path) ⇓ b, FSRemove(fs, path) ⇓ r, FSOpenDir(fs, path) ⇓ r, FSCreateDir(fs, path) ⇓ r, FSEnsureDir(fs, path) ⇓ r, FSKind(fs, path) ⇓ r, FSRestrict(fs, path) ⇓ fs', FileReadAll(handle) ⇓ r, FileReadAllBytes(handle) ⇓ r, FileWrite(handle, data) ⇓ r, FileFlush(handle) ⇓ r, FileClose(handle) ⇓ ok, DirNext(handle) ⇓ r, DirClose(handle) ⇓ ok}
FSResType(FSOpenRead) = `File@Read` | `IoError`
FSResType(FSOpenWrite) = `File@Write` | `IoError`
FSResType(FSOpenAppend) = `File@Append` | `IoError`
FSResType(FSCreateWrite) = `File@Write` | `IoError`
FSResType(FSReadFile) = `string@Managed` | `IoError`
FSResType(FSReadBytes) = `bytes@Managed` | `IoError`
FSResType(FSWriteFile) = `()` | `IoError`
FSResType(FSWriteStdout) = `()` | `IoError`
FSResType(FSWriteStderr) = `()` | `IoError`
FSResType(FSExists) = `bool`
FSResType(FSRemove) = `()` | `IoError`
FSResType(FSOpenDir) = `DirIter@Open` | `IoError`
FSResType(FSCreateDir) = `()` | `IoError`
FSResType(FSEnsureDir) = `()` | `IoError`
FSResType(FSKind) = `FileKind` | `IoError`
FSResType(FSRestrict) = `$FileSystem`
FSResType(FileReadAll) = `string@Managed` | `IoError`
FSResType(FileReadAllBytes) = `bytes@Managed` | `IoError`
FSResType(FileWrite) = `()` | `IoError`
FSResType(FileFlush) = `()` | `IoError`
FSResType(FileClose) = `ok`
FSResType(DirNext) = `DirEntry` | `()` | `IoError`
FSResType(DirClose) = `ok`

Handle = ℕ
Entry ::= FileEntry(bytes) | DirEntry(names) | OtherEntry
FSState = ⟨entries, handles, diriters, flushed, failmap⟩
Entries(⟨entries, handles, diriters, flushed, failmap⟩) = entries
Handles(⟨entries, handles, diriters, flushed, failmap⟩) = handles
DirIters(⟨entries, handles, diriters, flushed, failmap⟩) = diriters
FlushedSet(⟨entries, handles, diriters, flushed, failmap⟩) = flushed
FailMap(⟨entries, handles, diriters, flushed, failmap⟩) = failmap
EntryKind(ω, path) =
 `File`  if Entries(ω)[path] = FileEntry(_)
 `Dir`   if Entries(ω)[path] = DirEntry(_)
 `Other` if Entries(ω)[path] = OtherEntry
 `Other` otherwise
FileBytes(ω, path) = bytes ⇔ Entries(ω)[path] = FileEntry(bytes)
DirNames(ω, path) = names ⇔ Entries(ω)[path] = DirEntry(names)
HandleStateOf(ω, h) =
 Handles(ω)[h].state  if Handles(ω)[h] defined
 `Closed`             otherwise
HandlePos(ω, h) =
 Handles(ω)[h].pos  if Handles(ω)[h] defined
 0                  otherwise
HandleLen(ω, h) =
 Handles(ω)[h].len  if Handles(ω)[h] defined
 0                  otherwise
HandlePath(ω, h) =
 Handles(ω)[h].path  if Handles(ω)[h] defined
 "\""                otherwise
DirIterFS(ω, h) =
 DirIters(ω)[h].fs  if DirIters(ω)[h] defined
 ⊥                  otherwise
DirIterPath(ω, h) =
 DirIters(ω)[h].path  if DirIters(ω)[h] defined
 "\""                 otherwise
DirIterEntries(ω, h) =
 DirIters(ω)[h].entries  if DirIters(ω)[h] defined
 []                      otherwise
DirIterPos(ω, h) =
 DirIters(ω)[h].pos  if DirIters(ω)[h] defined
 0                    otherwise
DirIterOpen(ω, h) ⇔ DirIters(ω)[h] defined
Flushed(ω, h) ⇔ h ∈ FlushedSet(ω)
FSJudg_ω = {FSOpenRead(fs, path, ω) ⇓ (r, ω'), FSOpenWrite(fs, path, ω) ⇓ (r, ω'), FSOpenAppend(fs, path, ω) ⇓ (r, ω'), FSCreateWrite(fs, path, ω) ⇓ (r, ω'), FSReadFile(fs, path, ω) ⇓ (r, ω'), FSReadBytes(fs, path, ω) ⇓ (r, ω'), FSWriteFile(fs, path, data, ω) ⇓ (r, ω'), FSWriteStdout(fs, data, ω) ⇓ (r, ω'), FSWriteStderr(fs, data, ω) ⇓ (r, ω'), FSExists(fs, path, ω) ⇓ (b, ω'), FSRemove(fs, path, ω) ⇓ (r, ω'), FSOpenDir(fs, path, ω) ⇓ (r, ω'), FSCreateDir(fs, path, ω) ⇓ (r, ω'), FSEnsureDir(fs, path, ω) ⇓ (r, ω'), FSKind(fs, path, ω) ⇓ (r, ω')}
FileJudg_ω = {FileReadAll(h, ω) ⇓ (r, ω'), FileReadAllBytes(h, ω) ⇓ (r, ω'), FileWrite(h, data, ω) ⇓ (r, ω'), FileFlush(h, ω) ⇓ (r, ω'), FileClose(h, ω) ⇓ (ok, ω')}
DirJudg_ω = {DirNext(h, ω) ⇓ (r, ω'), DirClose(h, ω) ⇓ (ok, ω')}

FSOpenRead(fs, path) ⇓ r ⇔ ∃ ω, ω'. FSOpenRead(fs, path, ω) ⇓ (r, ω')
FSOpenWrite(fs, path) ⇓ r ⇔ ∃ ω, ω'. FSOpenWrite(fs, path, ω) ⇓ (r, ω')
FSOpenAppend(fs, path) ⇓ r ⇔ ∃ ω, ω'. FSOpenAppend(fs, path, ω) ⇓ (r, ω')
FSCreateWrite(fs, path) ⇓ r ⇔ ∃ ω, ω'. FSCreateWrite(fs, path, ω) ⇓ (r, ω')
FSReadFile(fs, path) ⇓ r ⇔ ∃ ω, ω'. FSReadFile(fs, path, ω) ⇓ (r, ω')
FSReadBytes(fs, path) ⇓ r ⇔ ∃ ω, ω'. FSReadBytes(fs, path, ω) ⇓ (r, ω')
FSWriteFile(fs, path, data) ⇓ r ⇔ ∃ ω, ω'. FSWriteFile(fs, path, data, ω) ⇓ (r, ω')
FSWriteStdout(fs, data) ⇓ r ⇔ ∃ ω, ω'. FSWriteStdout(fs, data, ω) ⇓ (r, ω')
FSWriteStderr(fs, data) ⇓ r ⇔ ∃ ω, ω'. FSWriteStderr(fs, data, ω) ⇓ (r, ω')
FSExists(fs, path) ⇓ b ⇔ ∃ ω, ω'. FSExists(fs, path, ω) ⇓ (b, ω')
FSRemove(fs, path) ⇓ r ⇔ ∃ ω, ω'. FSRemove(fs, path, ω) ⇓ (r, ω')
FSOpenDir(fs, path) ⇓ r ⇔ ∃ ω, ω'. FSOpenDir(fs, path, ω) ⇓ (r, ω')
FSCreateDir(fs, path) ⇓ r ⇔ ∃ ω, ω'. FSCreateDir(fs, path, ω) ⇓ (r, ω')
FSEnsureDir(fs, path) ⇓ r ⇔ ∃ ω, ω'. FSEnsureDir(fs, path, ω) ⇓ (r, ω')
FSKind(fs, path) ⇓ r ⇔ ∃ ω, ω'. FSKind(fs, path, ω) ⇓ (r, ω')
FileReadAll(h) ⇓ r ⇔ ∃ ω, ω'. FileReadAll(h, ω) ⇓ (r, ω')
FileReadAllBytes(h) ⇓ r ⇔ ∃ ω, ω'. FileReadAllBytes(h, ω) ⇓ (r, ω')
FileWrite(h, data) ⇓ r ⇔ ∃ ω, ω'. FileWrite(h, data, ω) ⇓ (r, ω')
FileFlush(h) ⇓ r ⇔ ∃ ω, ω'. FileFlush(h, ω) ⇓ (r, ω')
FileClose(h) ⇓ ok ⇔ ∃ ω, ω'. FileClose(h, ω) ⇓ (ok, ω')
DirNext(h) ⇓ r ⇔ ∃ ω, ω'. DirNext(h, ω) ⇓ (r, ω')
DirClose(h) ⇓ ok ⇔ ∃ ω, ω'. DirClose(h, ω) ⇓ (ok, ω')

**Restriction Semantics.**

RestrictPath(base, path) = p ⇔ ¬ AbsPath(path) ∧ b = Canon(Normalize(base)) ∧ b ≠ ⊥ ∧ p = Canon(Normalize(Join(b, path))) ∧ p ≠ ⊥ ∧ prefix(p, b)
RestrictPath(base, path) = ⊥ ⇔ AbsPath(path) ∨ Canon(Normalize(base)) = ⊥ ∨ Canon(Normalize(Join(Canon(Normalize(base)), path))) = ⊥ ∨ ¬ prefix(Canon(Normalize(Join(Canon(Normalize(base)), path))), Canon(Normalize(base)))
FSOp = {FSOpenRead, FSOpenWrite, FSOpenAppend, FSCreateWrite, FSReadFile, FSReadBytes, FSWriteFile, FSWriteStdout, FSWriteStderr, FSExists, FSRemove, FSOpenDir, FSCreateDir, FSEnsureDir, FSKind}
FSRestrict(fs, base) ⇓ fs' ∧ Op ∈ FSOp ∧ RestrictPath(base, p) = q ⇒ Op(fs', p) = Op(fs, q)
FSRestrict(fs, base) ⇓ fs' ∧ Op ∈ FSOp ∧ RestrictPath(base, p) = ⊥ ∧ Op ≠ FSExists ⇒ Op(fs', p) = IoError::InvalidPath
FSRestrict(fs, base) ⇓ fs' ∧ RestrictPath(base, p) = ⊥ ⇒ FSExists(fs', p) = false

**IoError Mapping.**

FSPathOp_0 = {FSOpenRead, FSOpenWrite, FSOpenAppend, FSCreateWrite, FSReadFile, FSReadBytes, FSExists, FSRemove, FSOpenDir, FSCreateDir, FSEnsureDir, FSKind}
FSPathOp_1 = {FSWriteFile}
FSRequiresExisting = {FSOpenRead, FSOpenWrite, FSOpenAppend, FSReadFile, FSReadBytes, FSOpenDir, FSKind, FSRemove}
PathInvalid(fs, path, ω) ⇔ Canon(Normalize(path)) = ⊥
EntryExists(ω, path) ⇔ Entries(ω)[path] defined
EntryKind(ω, path) ∈ {`File`, `Dir`, `Other`}
PermissionDenied(fs, path, Op, ω) ⇔ FailMap(ω)[⟨Op, path⟩] = IoError::PermissionDenied
Busy(fs, path, Op, ω) ⇔ FailMap(ω)[⟨Op, path⟩] = IoError::Busy
OtherFailure(fs, path, Op, ω) ⇔ FailMap(ω)[⟨Op, path⟩] = IoError::IoFailure

Op ∈ FSPathOp_0 ∧ PathInvalid(fs, path, ω) ⇒ Op(fs, path, ω) ⇓ (IoError::InvalidPath, ω)
Op ∈ FSPathOp_1 ∧ PathInvalid(fs, path, ω) ⇒ Op(fs, path, data, ω) ⇓ (IoError::InvalidPath, ω)
Op ∈ FSRequiresExisting ∧ ¬ EntryExists(ω, path) ⇒ Op(fs, path, ω) ⇓ (IoError::NotFound, ω)
PermissionDenied(fs, path, Op, ω) ⇒ Op(fs, path, ω) ⇓ (IoError::PermissionDenied, ω)
Op = FSCreateWrite ∧ EntryExists(ω, path) ⇒ Op(fs, path, ω) ⇓ (IoError::AlreadyExists, ω)
Op ∈ {FSCreateDir, FSEnsureDir} ∧ EntryExists(ω, path) ∧ EntryKind(ω, path) ≠ `Dir` ⇒ Op(fs, path, ω) ⇓ (IoError::AlreadyExists, ω)
Op = FSOpenDir ∧ EntryExists(ω, path) ∧ EntryKind(ω, path) ≠ `Dir` ⇒ Op(fs, path, ω) ⇓ (IoError::InvalidPath, ω)
Busy(fs, path, Op, ω) ⇒ Op(fs, path, ω) ⇓ (IoError::Busy, ω)
OtherFailure(fs, path, Op, ω) ⇒ Op(fs, path, ω) ⇓ (IoError::IoFailure, ω)

FSReadFile(fs, path, ω) ⇓ (r, ω') ∧ FSReadBytes(fs, path, ω) ⇓ (bytes, ω'') ∧ ¬ Utf8Valid(bytes) ⇒ r = IoError::IoFailure
FileReadAll(h, ω) ⇓ (r, ω') ∧ FileReadAllBytes(h, ω) ⇓ (bytes, ω'') ∧ ¬ Utf8Valid(bytes) ⇒ r = IoError::IoFailure

FSExists(fs, path, ω) ⇓ (true, ω') ⇒ EntryExists(ω, path) ∧ ¬ PathInvalid(fs, path, ω)
FSExists(fs, path, ω) ⇓ (false, ω') ⇒ PathInvalid(fs, path, ω) ∨ ¬ EntryExists(ω, path)

**File and Directory Operation Semantics.**

HandleState = {`OpenRead`, `OpenWrite`, `OpenAppend`, `Closed`}
HandleOpen(ω, h) ⇔ HandleStateOf(ω, h) ≠ `Closed`
HandleMode(ω, h) =
 `Read`    if HandleStateOf(ω, h) = `OpenRead`
 `Write`   if HandleStateOf(ω, h) = `OpenWrite`
 `Append`  if HandleStateOf(ω, h) = `OpenAppend`
FileLenAt(ω, path) =
 ByteLen(bytes)  if Entries(ω)[path] = FileEntry(bytes)
 0               otherwise
ByteLen(data) =
 |data|       if data ∈ Bytes
 |Utf8(data)| if data ∈ String
 0            otherwise

LexBytes(b_1, b_2) ⇔ (∃ k. 0 ≤ k < min(|b_1|, |b_2|) ∧ (∀ i < k. b_1[i] = b_2[i]) ∧ b_1[k] < b_2[k]) ∨ (|b_1| < |b_2| ∧ ∀ i < |b_1|. b_1[i] = b_2[i])
EntryKey(name) = CaseFold(NFC(name))
EntryOrder(a, b) ⇔ LexBytes(Utf8(EntryKey(a)), Utf8(EntryKey(b))) ∨ (EntryKey(a) = EntryKey(b) ∧ LexBytes(Utf8(a), Utf8(b)))
DirSnapshot(fs, path, ω) =
 DirNames(ω, path)  if Entries(ω)[path] = DirEntry(_)
 []                 otherwise
DirEntries(fs, path, ω) = sort_{EntryOrder}(DirSnapshot(fs, path, ω))
∀ name ∈ DirSnapshot(fs, path, ω). name ≠ "." ∧ name ≠ ".."

FSOpenRead(fs, path, ω) ⇓ (`File@Read`{`handle`: h}, ω') ⇒ HandleStateOf(ω', h) = `OpenRead` ∧ HandlePos(ω', h) = 0 ∧ HandlePath(ω', h) = path ∧ HandleLen(ω', h) = FileLenAt(ω, path)
FSOpenWrite(fs, path, ω) ⇓ (`File@Write`{`handle`: h}, ω') ⇒ HandleStateOf(ω', h) = `OpenWrite` ∧ HandlePos(ω', h) = 0 ∧ HandlePath(ω', h) = path ∧ HandleLen(ω', h) = FileLenAt(ω, path)
FSOpenAppend(fs, path, ω) ⇓ (`File@Append`{`handle`: h}, ω') ⇒ HandleStateOf(ω', h) = `OpenAppend` ∧ HandlePos(ω', h) = FileLenAt(ω, path) ∧ HandlePath(ω', h) = path ∧ HandleLen(ω', h) = FileLenAt(ω, path)
FSCreateWrite(fs, path, ω) ⇓ (`File@Write`{`handle`: h}, ω') ⇒ HandleStateOf(ω', h) = `OpenWrite` ∧ HandlePos(ω', h) = 0 ∧ HandlePath(ω', h) = path ∧ HandleLen(ω', h) = 0

FSReadFile(fs, path, ω) ⇓ (r, ω') ⇔ ∃ h, ω_1, ω_2. FSOpenRead(fs, path, ω) ⇓ (`File@Read`{`handle`: h}, ω_1) ∧ FileReadAll(h, ω_1) ⇓ (r, ω_2) ∧ FileClose(h, ω_2) ⇓ (ok, ω')
FSReadBytes(fs, path, ω) ⇓ (r, ω') ⇔ ∃ h, ω_1, ω_2. FSOpenRead(fs, path, ω) ⇓ (`File@Read`{`handle`: h}, ω_1) ∧ FileReadAllBytes(h, ω_1) ⇓ (r, ω_2) ∧ FileClose(h, ω_2) ⇓ (ok, ω')

¬ HandleOpen(ω, h) ⇒ FileReadAll(h, ω) ⇓ (IoError::IoFailure, ω)
¬ HandleOpen(ω, h) ⇒ FileReadAllBytes(h, ω) ⇓ (IoError::IoFailure, ω)
¬ HandleOpen(ω, h) ⇒ FileWrite(h, data, ω) ⇓ (IoError::IoFailure, ω)
¬ HandleOpen(ω, h) ⇒ FileFlush(h, ω) ⇓ (IoError::IoFailure, ω)

FileReadAll(h, ω) ⇓ (r, ω') ∧ r ≠ IoError::IoFailure ⇒ HandlePos(ω', h) = HandleLen(ω, h)
FileReadAllBytes(h, ω) ⇓ (r, ω') ∧ r ≠ IoError::IoFailure ⇒ HandlePos(ω', h) = HandleLen(ω, h)

FileWrite(h, data, ω) ⇓ (ok, ω') ⇒ HandleOpen(ω, h) ∧ (HandleMode(ω, h) = `Append` ⇒ HandlePos(ω', h) = HandleLen(ω, h) + ByteLen(data)) ∧ (HandleMode(ω, h) ≠ `Append` ⇒ HandlePos(ω', h) = HandlePos(ω, h) + ByteLen(data))
FileWrite(h, data, ω) ⇓ (ok, ω') ⇒ HandleLen(ω', h) = max(HandleLen(ω, h), HandlePos(ω', h))

FileFlush(h, ω) ⇓ (ok, ω') ⇒ Flushed(ω', h)
FileClose(h, ω) ⇓ (ok, ω') ⇒ HandleStateOf(ω', h) = `Closed`

FSOpenDir(fs, path, ω) ⇓ (`DirIter@Open`{`handle`: h}, ω') ⇒ DirIterOpen(ω', h) ∧ DirIterFS(ω', h) = fs ∧ DirIterPath(ω', h) = path ∧ DirIterEntries(ω', h) = DirEntries(fs, path, ω) ∧ DirIterPos(ω', h) = 0

¬ DirIterOpen(ω, h) ⇒ DirNext(h, ω) ⇓ (IoError::IoFailure, ω)
DirIterOpen(ω, h) ∧ DirIterPos(ω, h) = i ∧ i ≥ |DirIterEntries(ω, h)| ⇒ DirNext(h, ω) ⇓ ((), ω)
DirIterOpen(ω, h) ∧ DirIterPos(ω, h) = i ∧ i < |DirIterEntries(ω, h)| ∧ name = DirIterEntries(ω, h)[i] ∧ path = Join(DirIterPath(ω, h), name) ∧ FSKind(DirIterFS(ω, h), path, ω) ⇓ (k, ω_1) ⇒ DirNext(h, ω) ⇓ (`DirEntry`{`path`: path, `name`: name, `kind`: k}, ω_2) ∧ DirIterPos(ω_2, h) = i + 1

DirClose(h, ω) ⇓ (ok, ω') ⇒ ¬ DirIterOpen(ω', h)

**Handle Extraction.**

HandleOf(v) = h ⇔ v = `File@Read`{`handle`: h} ∨ v = `File@Write`{`handle`: h} ∨ v = `File@Append`{`handle`: h}
DirHandleOf(v) = h ⇔ v = `DirIter@Open`{`handle`: h}

**Primitive Method Application.**

MethodName(MethodDecl(_, _, name, _, _, _, _, _, _)) = name
MethodName(ClassMethodDecl(_, name, _, _, _, _, _, _)) = name
MethodName(StateMethodDecl(_, name, _, _, _, _, _)) = name
MethodName(TransitionDecl(_, name, _, _, _, _, _)) = name
MethodOwner(m) = owner ⇔ ∃ T. MethodByName(T, MethodName(m)) = m ∧ owner = T
PrimCallJudg = {PrimCall(Owner, name, v_self, args) ⇓ v}

**(Prim-FS-OpenRead)**
Γ ⊢ FSOpenRead(v_fs, p) ⇓ r
──────────────────────────────────────────────────────────────────
Γ ⊢ PrimCall(`FileSystem`, `open_read`, v_fs, [p]) ⇓ r

**(Prim-FS-OpenWrite)**
Γ ⊢ FSOpenWrite(v_fs, p) ⇓ r
───────────────────────────────────────────────────────────────────
Γ ⊢ PrimCall(`FileSystem`, `open_write`, v_fs, [p]) ⇓ r

**(Prim-FS-OpenAppend)**
Γ ⊢ FSOpenAppend(v_fs, p) ⇓ r
────────────────────────────────────────────────────────────────────
Γ ⊢ PrimCall(`FileSystem`, `open_append`, v_fs, [p]) ⇓ r

**(Prim-FS-CreateWrite)**
Γ ⊢ FSCreateWrite(v_fs, p) ⇓ r
──────────────────────────────────────────────────────────────────────
Γ ⊢ PrimCall(`FileSystem`, `create_write`, v_fs, [p]) ⇓ r

**(Prim-FS-ReadFile)**
Γ ⊢ FSReadFile(v_fs, p) ⇓ r
──────────────────────────────────────────────────────────────────
Γ ⊢ PrimCall(`FileSystem`, `read_file`, v_fs, [p]) ⇓ r

**(Prim-FS-ReadBytes)**
Γ ⊢ FSReadBytes(v_fs, p) ⇓ r
───────────────────────────────────────────────────────────────────
Γ ⊢ PrimCall(`FileSystem`, `read_bytes`, v_fs, [p]) ⇓ r

**(Prim-FS-WriteFile)**
Γ ⊢ FSWriteFile(v_fs, p, d) ⇓ r
──────────────────────────────────────────────────────────────────────
Γ ⊢ PrimCall(`FileSystem`, `write_file`, v_fs, [p, d]) ⇓ r

**(Prim-FS-WriteStdout)**
Γ ⊢ FSWriteStdout(v_fs, d) ⇓ r
──────────────────────────────────────────────────────────────────────
Γ ⊢ PrimCall(`FileSystem`, `write_stdout`, v_fs, [d]) ⇓ r

**(Prim-FS-WriteStderr)**
Γ ⊢ FSWriteStderr(v_fs, d) ⇓ r
──────────────────────────────────────────────────────────────────────
Γ ⊢ PrimCall(`FileSystem`, `write_stderr`, v_fs, [d]) ⇓ r

**(Prim-FS-Exists)**
Γ ⊢ FSExists(v_fs, p) ⇓ b
────────────────────────────────────────────────────────────────
Γ ⊢ PrimCall(`FileSystem`, `exists`, v_fs, [p]) ⇓ b

**(Prim-FS-Remove)**
Γ ⊢ FSRemove(v_fs, p) ⇓ r
────────────────────────────────────────────────────────────────
Γ ⊢ PrimCall(`FileSystem`, `remove`, v_fs, [p]) ⇓ r

**(Prim-FS-OpenDir)**
Γ ⊢ FSOpenDir(v_fs, p) ⇓ r
──────────────────────────────────────────────────────────────────
Γ ⊢ PrimCall(`FileSystem`, `open_dir`, v_fs, [p]) ⇓ r

**(Prim-FS-CreateDir)**
Γ ⊢ FSCreateDir(v_fs, p) ⇓ r
────────────────────────────────────────────────────────────────────
Γ ⊢ PrimCall(`FileSystem`, `create_dir`, v_fs, [p]) ⇓ r

**(Prim-FS-EnsureDir)**
Γ ⊢ FSEnsureDir(v_fs, p) ⇓ r
────────────────────────────────────────────────────────────────────
Γ ⊢ PrimCall(`FileSystem`, `ensure_dir`, v_fs, [p]) ⇓ r

**(Prim-FS-Kind)**
Γ ⊢ FSKind(v_fs, p) ⇓ r
──────────────────────────────────────────────────────────────
Γ ⊢ PrimCall(`FileSystem`, `kind`, v_fs, [p]) ⇓ r

**(Prim-FS-Restrict)**
Γ ⊢ FSRestrict(v_fs, p) ⇓ v_fs'
──────────────────────────────────────────────────────────────────────
Γ ⊢ PrimCall(`FileSystem`, `restrict`, v_fs, [p]) ⇓ v_fs'

**(Prim-File-ReadAll)**
HandleOf(v) = h    Γ ⊢ FileReadAll(h) ⇓ r
───────────────────────────────────────────────────────────────
Γ ⊢ PrimCall(`File@Read`, `read_all`, v, []) ⇓ r

**(Prim-File-ReadAllBytes)**
HandleOf(v) = h    Γ ⊢ FileReadAllBytes(h) ⇓ r
────────────────────────────────────────────────────────────────────
Γ ⊢ PrimCall(`File@Read`, `read_all_bytes`, v, []) ⇓ r

**(Prim-File-Write)**
HandleOf(v) = h    Γ ⊢ FileWrite(h, d) ⇓ r
────────────────────────────────────────────────────────────
Γ ⊢ PrimCall(`File@Write`, `write`, v, [d]) ⇓ r

**(Prim-File-Flush)**
HandleOf(v) = h    Γ ⊢ FileFlush(h) ⇓ r
────────────────────────────────────────────────────────────
Γ ⊢ PrimCall(`File@Write`, `flush`, v, []) ⇓ r

**(Prim-File-Write-Append)**
HandleOf(v) = h    Γ ⊢ FileWrite(h, d) ⇓ r
─────────────────────────────────────────────────────────────
Γ ⊢ PrimCall(`File@Append`, `write`, v, [d]) ⇓ r

**(Prim-File-Flush-Append)**
HandleOf(v) = h    Γ ⊢ FileFlush(h) ⇓ r
─────────────────────────────────────────────────────────────
Γ ⊢ PrimCall(`File@Append`, `flush`, v, []) ⇓ r

**(Prim-File-Close-Read)**
HandleOf(v) = h    Γ ⊢ FileClose(h) ⇓ ok
──────────────────────────────────────────────────────────────
Γ ⊢ PrimCall(`File@Read`, `close`, v, []) ⇓ `File@Closed`{}

**(Prim-File-Close-Write)**
HandleOf(v) = h    Γ ⊢ FileClose(h) ⇓ ok
───────────────────────────────────────────────────────────────
Γ ⊢ PrimCall(`File@Write`, `close`, v, []) ⇓ `File@Closed`{}

**(Prim-File-Close-Append)**
HandleOf(v) = h    Γ ⊢ FileClose(h) ⇓ ok
───────────────────────────────────────────────────────────────
Γ ⊢ PrimCall(`File@Append`, `close`, v, []) ⇓ `File@Closed`{}

**(Prim-Dir-Next)**
DirHandleOf(v) = h    Γ ⊢ DirNext(h) ⇓ r
─────────────────────────────────────────────────────────────
Γ ⊢ PrimCall(`DirIter@Open`, `next`, v, []) ⇓ r

**(Prim-Dir-Close)**
DirHandleOf(v) = h    Γ ⊢ DirClose(h) ⇓ ok
──────────────────────────────────────────────────────────────
Γ ⊢ PrimCall(`DirIter@Open`, `close`, v, []) ⇓ `DirIter@Closed`{}

**(ApplyMethod-Prim)**
MethodOwner(m) = owner    MethodName(m) = name    Γ ⊢ PrimCall(owner, name, v_self, vec_v) ⇓ v
────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ ApplyMethod(m, v_self, vec_v) ⇓ v

**(ApplyMethod-Prim-Step)**
MethodOwner(m) = owner    MethodName(m) = name    Γ ⊢ PrimCall(owner, name, v_self, vec_v) ⇓ v
─────────────────────────────────────────────────────────────────────────────────────────────────────
⟨ApplyMethod(m, v_self, vec_v)⟩ → ⟨v⟩

### 7.8. Interpreter Entrypoint (Project-Level)

**Interpreter Judgments.**

InterpJudg = {Γ ⊢ ContextInitSigma(σ) ⇓ (Val(v_ctx), σ'), Γ ⊢ InterpretProject(P, σ) ⇓ (out, σ'), Γ ⊢ InterpretProject(P, σ) ⇑ panic(P_s)}
ContextValue(v) ⇔ ∃ bits. ValueBits(TypePath(["Context"]), v) = bits

**(ContextInitSigma)**
ContextValue(v_ctx)
────────────────────────────────────────────────────────────
Γ ⊢ ContextInitSigma(σ) ⇓ (Val(v_ctx), σ)

**(Interpret-Project-Ok)**
Executable(P)    MainDecls(P) = [d]    MainSigOk(d)    Γ ⊢ ContextInitSigma(σ) ⇓ (Val(v_ctx), σ_0)    Γ ⊢ Init(G_e, σ_0) ⇓ σ_1    Γ ⊢ ApplyProcSigma(d, [v_ctx], σ_1) ⇓ (Val(v), σ_2)    Γ ⊢ Deinit(P, σ_2) ⇓ σ_3
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ InterpretProject(P, σ) ⇓ (Val(v), σ_3)

**(Interpret-Project-Init-Panic)**
Executable(P)    MainDecls(P) = [d]    MainSigOk(d)    Γ ⊢ ContextInitSigma(σ) ⇓ (Val(v_ctx), σ_0)    Γ ⊢ Init(G_e, σ_0) ⇑ panic(P_s)
─────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ InterpretProject(P, σ) ⇑ panic(P_s)

**(Interpret-Project-Main-Ctrl)**
Executable(P)    MainDecls(P) = [d]    MainSigOk(d)    Γ ⊢ ContextInitSigma(σ) ⇓ (Val(v_ctx), σ_0)    Γ ⊢ Init(G_e, σ_0) ⇓ σ_1    Γ ⊢ ApplyProcSigma(d, [v_ctx], σ_1) ⇓ (Ctrl(κ), σ_2)    κ ∈ {Panic, Abort}
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ InterpretProject(P, σ) ⇓ (Ctrl(κ), σ_2)

**(Interpret-Project-Deinit-Panic)**
Executable(P)    MainDecls(P) = [d]    MainSigOk(d)    Γ ⊢ ContextInitSigma(σ) ⇓ (Val(v_ctx), σ_0)    Γ ⊢ Init(G_e, σ_0) ⇓ σ_1    Γ ⊢ ApplyProcSigma(d, [v_ctx], σ_1) ⇓ (Val(v), σ_2)    Γ ⊢ Deinit(P, σ_2) ⇑ panic
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Γ ⊢ InterpretProject(P, σ) ⇑ panic
## 8. Appendix A - Diagnostic Codes

### 8.0. DiagIdâ€“Code Map

DiagTable = {`E-PRJ`, `E-MOD`, `E-OUT`, `E-SRC`, `E-CNF`, `E-UNS`, `E-MEM`, `W-MOD`, `W-SRC`, `E-TYP`, `W-SYS`, `E-SEM`, `W-SEM`}
DiagRow = ⟨code, sev, det, cond, ids⟩
RowCode(⟨code, sev, det, cond, ids⟩) = code
RowSev(⟨code, sev, det, cond, ids⟩) = sev
RowDet(⟨code, sev, det, cond, ids⟩) = det
RowCond(⟨code, sev, det, cond, ids⟩) = cond
RowIds(⟨code, sev, det, cond, ids⟩) = ids
SeverityColumn(c) = sev ⇔ ∃ row ∈ DiagRows. RowCode(row) = c ∧ RowSev(row) = sev
ConditionColumn(c) = cond ⇔ ∃ row ∈ DiagRows. RowCode(row) = c ∧ RowCond(row) = cond
TableRows(t) = {row | row appears in table t}
DiagRows = ⋃_{t ∈ DiagTable} TableRows(t)
C0Code(id) = c ⇔ ∃ row ∈ DiagRows. id ∈ RowIds(row) ∧ RowCode(row) = c
C0Code(id) = ⊥ ⇔ ¬ ∃ row ∈ DiagRows. id ∈ RowIds(row)

### 8.1. E-PRJ (Project)

| Code         | Severity | Detection    | Condition                                                                            | DiagId                                                                                             |
| ------------ | -------- | ------------ | ------------------------------------------------------------------------------------ | -------------------------------------------------------------------------------------------------- |
| `E-PRJ-0101` | Error    | Compile-time | `Cursive.toml` not found at project root                                             | Parse-Manifest-Missing                                                                             |
| `E-PRJ-0102` | Error    | Compile-time | `Cursive.toml` is not valid TOML                                                     | Parse-Manifest-Err                                                                                 |
| `E-PRJ-0103` | Error    | Compile-time | Missing required `assembly` table, empty assembly list, required keys, or required key type | WF-Assembly-Table-Err, WF-Assembly-Count-Err, WF-Assembly-Required-Types-Err                       |
| `E-PRJ-0104` | Error    | Compile-time | Unknown key in `assembly` table or unknown top-level key                             | WF-Assembly-Keys-Err, WF-TopKeys-Err                                                               |
| `E-PRJ-0201` | Error    | Compile-time | `assembly.kind` is not in `{ "executable", "library" }`                              | WF-Assembly-Kind-Err                                                                               |
| `E-PRJ-0202` | Error    | Compile-time | Duplicate `assembly.name` values                                                     | WF-Assembly-Name-Dup                                                                               |
| `E-PRJ-0203` | Error    | Compile-time | `assembly.name` is not a valid identifier                                            | WF-Assembly-Name-Err                                                                               |
| `E-PRJ-0204` | Error    | Compile-time | `emit_ir` has invalid value or type                                                  | WF-Assembly-EmitIR-Err, WF-Assembly-EmitIRType-Err                                                 |
| `E-PRJ-0205` | Error    | Compile-time | Assembly selection failed (missing target or target not found)                       | Assembly-Select-Err                                                                                |
| `E-PRJ-0301` | Error    | Compile-time | `assembly.root` or `out_dir` has invalid type, is absolute, or resolves outside root | WF-Assembly-Root-Path-Err, WF-Assembly-OutDir-Path-Err, WF-Assembly-OutDirType-Err, WF-RelPath-Err |
| `E-PRJ-0302` | Error    | Compile-time | `assembly.root` does not exist or is not a directory                                 | WF-Source-Root-Err                                                                                 |
| `E-PRJ-0303` | Error    | Compile-time | Relative path derivation failed during deterministic ordering (file or directory)    | FileOrder-Rel-Fail, DirSeq-Rel-Fail                                                                |
| `E-PRJ-0304` | Error    | Compile-time | Path canonicalization or module path derivation failed due to filesystem error       | Disc-Rel-Fail, Resolve-Canonical-Err                                                               |
| `E-PRJ-0305` | Error    | Compile-time | Directory enumeration failed during module discovery                                 | DirSeq-Read-Err                                                                                    |

### 8.2. E-MOD (Module)

| Code         | Severity | Detection    | Condition                                                       | DiagId                                                                                                                                 |
| ------------ | -------- | ------------ | --------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------- |
| `E-MOD-1104` | Error    | Compile-time | Module path collision after NFC + case folding                  | WF-Module-Path-Collision, Disc-Collision                                                                                               |
| `E-MOD-1105` | Error    | Compile-time | Module path component is a reserved keyword                     | WF-Module-Path-Reserved                                                                                                                |
| `E-MOD-1106` | Error    | Compile-time | Module path component is not a valid identifier                 | WF-Module-Path-Ident-Err                                                                                                               |
| `E-MOD-1204` | Error    | Compile-time | Using path does not resolve to a module or item                 | Resolve-Using-None                                                                                                                     |
| `E-MOD-1205` | Error    | Compile-time | Attempt to `public using` a non-public item                     | Using-Path-Item-Public-Err, Using-List-Public-Err                                                                                      |
| `E-MOD-1206` | Error    | Compile-time | Duplicate item in a `using` list                                | Using-List-Dup                                                                                                                         |
| `E-MOD-1207` | Error    | Compile-time | Cannot access a non-public item from this scope                 | Access-Err                                                                                                                             |
| `E-MOD-1208` | Error    | Compile-time | Using path is ambiguous between module and item                 | Resolve-Using-Ambig                                                                                                                    |
| `E-MOD-1301` | Error    | Compile-time | Unresolved name: identifier not found in any accessible scope   | ResolveExpr-Ident-Err, ResolveQual-Name-Err, ResolveQual-Apply-Err, ResolveQual-Apply-Brace-Err, Expr-Unresolved-Err                   |
| `E-MOD-1302` | Error    | Compile-time | Duplicate declaration in module scope                           | Collect-Dup, Names-Step-Dup                                                                                                            |
| `E-MOD-1303` | Error    | Compile-time | Shadowing without `shadow` keyword                              | Intro-Shadow-Required                                                                                                                  |
| `E-MOD-1304` | Error    | Compile-time | Unresolved module: path prefix did not resolve to a module      | ResolveModulePath-Err                                                                                                                  |
| `E-MOD-1306` | Error    | Compile-time | Unnecessary `shadow` keyword: no binding is being shadowed      | Shadow-Unnecessary                                                                                                                     |
| `E-MOD-1307` | Error    | Compile-time | Ambiguous method resolution; disambiguation required            | LookupMethod-Ambig                                                                                                                     |
| `E-MOD-1401` | Error    | Compile-time | Cyclic module dependency detected in eager initializers         | Topo-Cycle                                                                                                                             |
| `E-MOD-2401` | Error    | Compile-time | Reassignment of immutable `let` binding                         | Assign-Immutable-Err                                                                                                                   |
| `E-MOD-2402` | Error    | Compile-time | Type annotation incompatible with inferred type                 | WF-StaticDecl-Ann-Mismatch, T-LetStmt-Ann-Mismatch, T-VarStmt-Ann-Mismatch, T-ShadowLetStmt-Ann-Mismatch, T-ShadowVarStmt-Ann-Mismatch |
| `E-MOD-2411` | Error    | Compile-time | Missing move expression at call site for transferring parameter | B-ArgPass-Move-Missing                                                                                                                 |
| `E-MOD-2430` | Error    | Compile-time | Multiple `main` procedures defined                              | Main-Multiple                                                                                                                          |
| `E-MOD-2431` | Error    | Compile-time | Invalid `main` signature                                        | Main-Signature-Err                                                                                                                     |
| `E-MOD-2432` | Error    | Compile-time | `main` is generic (has type parameters)                         | Main-Generic-Err                                                                                                                       |
| `E-MOD-2433` | Error    | Compile-time | Module-scope `var` declaration with `public` visibility         | StaticVisOk-Err                                                                                                                        |
| `E-MOD-2434` | Error    | Compile-time | Missing `main` procedure                                        | Main-Missing                                                                                                                           |
| `E-MOD-2440` | Error    | Compile-time | `protected` used on top-level declaration                       | Protected-TopLevel-Err                                                                                                                 |

### 8.3. E-OUT (Output and Linking)

| Code         | Severity | Detection    | Condition                                                                | DiagId                                                                           |
| ------------ | -------- | ------------ | ------------------------------------------------------------------------ | -------------------------------------------------------------------------------- |
| `E-OUT-0401` | Error    | Compile-time | Failed to create output directory                                        | Out-Dirs-Err                                                                     |
| `E-OUT-0402` | Error    | Compile-time | Failed to emit object file (codegen or write)                            | Out-Obj-Err, EmitObj-Err                                                         |
| `E-OUT-0403` | Error    | Compile-time | Failed to emit IR/bitcode (codegen, assemble, tool resolution, or write) | Out-IR-Err, EmitLLVM-Err                                                         |
| `E-OUT-0404` | Error    | Compile-time | Linker invocation failed                                                 | Out-Link-Fail                                                                    |
| `E-OUT-0405` | Error    | Compile-time | Required linker tool not found                                           | Out-Link-NotFound                                                                |
| `E-OUT-0406` | Error    | Compile-time | Output path collision detected                                           | Out-Obj-Collision, Out-IR-Collision                                              |
| `E-OUT-0407` | Error    | Compile-time | Runtime library missing or unreadable                                    | Out-Link-Runtime-Missing                                                         |
| `E-OUT-0408` | Error    | Compile-time | Runtime library missing required symbol(s)                               | Out-Link-Runtime-Incompatible                                                    |
| `E-OUT-0410` | Error    | Compile-time | LLVM type mapping failed                                                 | LLVMTy-Err                                                                       |
| `E-OUT-0411` | Error    | Compile-time | LLVM IR lowering failed                                                  | LowerIR-Err, LowerIRDecl-Err, LowerIRInstr-Err                                   |
| `E-OUT-0412` | Error    | Compile-time | Binding storage/validity lowering failed                                 | BindSlot-Err, BindValid-Err, UpdateValid-Err, DropOnAssign-Err                   |
| `E-OUT-0413` | Error    | Compile-time | LLVM call ABI lowering failed                                            | LLVMCall-Err, LLVMArgLower-Err, LLVMRetLower-Err                                 |
| `E-OUT-0414` | Error    | Compile-time | VTable emission failed                                                   | EmitVTable-Err                                                                   |
| `E-OUT-0415` | Error    | Compile-time | Literal data emission failed                                             | EmitLiteral-Err                                                                  |
| `E-OUT-0416` | Error    | Compile-time | Runtime built-in symbol resolution failed                                | BuiltinSym-String-Err, BuiltinSym-Bytes-Err, StringDropSym-Err, BytesDropSym-Err |
| `E-OUT-0417` | Error    | Compile-time | Entrypoint or context construction lowering failed                       | EntrySym-Err, EntryStub-Err, EmitInitPlan-Err, EmitDeinitPlan-Err                |
| `E-OUT-0418` | Error    | Compile-time | Poisoning instrumentation failed                                         | PoisonFlag-Err, CheckPoison-Err, SetPoison-Err                                   |

### 8.4. E-SRC (Source)

| Code         | Severity | Detection    | Condition                                                    | DiagId                                   |
| ------------ | -------- | ------------ | ------------------------------------------------------------ | ---------------------------------------- |
| `E-SRC-0101` | Error    | Compile-time | Invalid UTF-8 byte sequence                                  | Step-Decode-Err                          |
| `E-SRC-0102` | Error    | Compile-time | Failed to read source file                                   | ReadBytes-Err                            |
| `E-SRC-0103` | Error    | Compile-time | Embedded BOM found after the first position                  | Step-EmbeddedBOM-Err                     |
| `E-SRC-0104` | Error    | Compile-time | Forbidden control character or null byte                     | Step-Prohibited-Err                      |
| `E-SRC-0301` | Error    | Compile-time | Unterminated string literal                                  | Lex-String-Unterminated                  |
| `E-SRC-0302` | Error    | Compile-time | Invalid escape sequence                                      | Lex-String-BadEscape, Lex-Char-BadEscape |
| `E-SRC-0303` | Error    | Compile-time | Invalid character literal                                    | Lex-Char-Invalid, Lex-Char-Unterminated  |
| `E-SRC-0304` | Error    | Compile-time | Malformed numeric literal                                    | Lex-Numeric-Err                          |
| `E-SRC-0306` | Error    | Compile-time | Unterminated block comment                                   | Block-Comment-Unterminated               |
| `E-SRC-0307` | Error    | Compile-time | Invalid Unicode in identifier                                | Lex-Ident-InvalidUnicode                 |
| `E-SRC-0308` | Error    | Compile-time | Lexically sensitive Unicode character outside `unsafe` block | LexSecure-Err                            |
| `E-SRC-0309` | Error    | Compile-time | Tokenization failed to classify a character sequence         | Max-Munch-Err                            |
| `E-SRC-0510` | Error    | Compile-time | Missing statement terminator                                 | Missing-Terminator-Err                   |
| `E-SRC-0520` | Error    | Compile-time | Generic syntax error (unexpected token)                      | Parse-Syntax-Err                         |

### 8.5. E-CNF (Conformance / Limits)

| Code         | Severity | Detection    | Condition                                                                                   | DiagId                                                                                    |
| ------------ | -------- | ------------ | ------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------- |
| `E-CNF-0401` | Error    | Compile-time | Reserved keyword used as identifier                                                         | Validate-Module-Keyword-Err                                                               |
| `E-CNF-0402` | Error    | Compile-time | Reserved namespace `cursive.*` used by user code                                            | Validate-ModulePath-Reserved-Err, Intro-Reserved-Cursive-Err, Shadow-Reserved-Cursive-Err |
| `E-CNF-0403` | Error    | Compile-time | Primitive type name shadowed at module scope                                                | Validate-Module-Prim-Shadow-Err                                                           |
| `E-CNF-0404` | Error    | Compile-time | Shadowing of `Self`, `Drop`, `Bitcopy`, `Clone`, `string`, `bytes`, `Modal`, `Region`, `RegionOptions`, or `Context`    | Validate-Module-Special-Shadow-Err                                                        |
| `E-CNF-0405` | Error    | Compile-time | Shadowing of async type alias (`Async`, `Future`, `Sequence`, `Stream`, `Pipe`, `Exchange`) | Validate-Module-Async-Shadow-Err                                                          |
| `E-CNF-0406` | Error    | Compile-time | User declaration uses `gen_` prefix                                                         | Intro-Reserved-Gen-Err, Shadow-Reserved-Gen-Err                                           |

### 8.6. E-UNS (Unsupported Constructs)

| Code         | Severity | Detection    | Condition                                              | DiagId                   |
| ------------ | -------- | ------------ | ------------------------------------------------------ | ------------------------ |
| `E-UNS-0101` | Error    | Compile-time | Unsupported construct in Cursive0 subset               | Unsupported-Construct    |
| `E-UNS-0102` | Error    | Compile-time | Array index must be a compile-time constant            | Index-Array-NonConst-Err |
| `E-UNS-0103` | Error    | Compile-time | Array index out of bounds                              | Index-Array-OOB-Err      |
| `E-UNS-0104` | Error    | Compile-time | `transmute` source and target alignments differ        | T-Transmute-AlignEq      |
| `E-UNS-0105` | Error    | Compile-time | `override` used with no concrete procedure to override | Override-NoConcrete      |
| `E-UNS-0106` | Error    | Compile-time | Conflicting procedure signatures from multiple classes | EffMethods-Conflict      |
| `E-UNS-0107` | Error    | Compile-time | Non-`Bitcopy` place expression used as value           | ValueUse-NonBitcopyPlace |
| `E-UNS-0108` | Error    | Compile-time | Range expression used as index in Cursive0 subset      | Range-NonIndex-Err       |
| `E-UNS-0110` | Error    | Compile-time | `import` declaration used                              | WF-Import-Unsupported    |
| `E-UNS-0111` | Error    | Compile-time | `[[unwind]]` attribute used                            | WF-Unwind-Unsupported    |
| `E-UNS-0112` | Error    | Compile-time | `extern` block or foreign declaration used             | Parse-Extern-Unsupported |
| `E-UNS-0113` | Error    | Compile-time | Attribute syntax used                                  | WF-Attr-Unsupported      |

### 8.7. E-MEM (Memory)

| Code         | Severity | Detection    | Condition                                                            | DiagId                                                                                   |
| ------------ | -------- | ------------ | -------------------------------------------------------------------- | ---------------------------------------------------------------------------------------- |
| `E-MEM-1206` | Error    | Compile-time | Named region not found for allocation                                | Alloc-Region-NotFound-Err                                                                |
| `E-MEM-1207` | Error    | Compile-time | `frame` used with no active region in scope                          | Frame-NoActiveRegion-Err                                                                 |
| `E-MEM-1208` | Error    | Compile-time | `r.frame` target is not in `Region@Active` state                     | Frame-Target-NotActive-Err                                                               |
| `E-MEM-3001` | Error    | Compile-time | Read or move of a binding in Moved or PartiallyMoved state           | B-Place-Moved-Err, B-Move-Whole-Moved-Err, B-Move-Field-Moved-Err                        |
| `E-MEM-3003` | Error    | Compile-time | Reassignment of immutable binding                                    | B-Assign-Immutable-Err                                                                   |
| `E-MEM-3004` | Error    | Compile-time | Partial move from binding without `unique` permission                | B-Move-Field-NonUnique-Err                                                               |
| `E-MEM-3005` | Error    | Compile-time | Explicit call to `Drop::drop` method                                 | Drop-Call-Err, Drop-Call-Err-Dyn                                                         |
| `E-MEM-3006` | Error    | Compile-time | Attempt to move from immovable binding (`:=`)                        | B-Move-Whole-Immovable-Err, B-Move-Field-Immovable-Err                                   |
| `E-MEM-3007` | Error    | Compile-time | `unique` binding from place expression requires explicit `move`      | B-LetVar-UniqueNonMove-Err, B-ShadowLet-UniqueNonMove-Err, B-ShadowVar-UniqueNonMove-Err |
| `E-MEM-3020` | Error    | Compile-time | Value with shorter-lived provenance escapes to longer-lived location | Prov-Escape-Err                                                                          |
| `E-MEM-3021` | Error    | Compile-time | Region allocation `^` outside region scope                           | Alloc-Implicit-NoRegion-Err                                                              |
| `E-MEM-3030` | Error    | Compile-time | Unsafe operation outside block                                       | Transmute-Unsafe-Err                                                                     |
| `E-MEM-3030` | Error    | Compile-time | Unsafe operation outside block                                       | AllocRaw-Unsafe-Err                                                                      |
| `E-MEM-3030` | Error    | Compile-time | Unsafe operation outside block                                       | DeallocRaw-Unsafe-Err                                                                    |
| `E-MEM-3030` | Error    | Compile-time | Unsafe operation outside block                                       | Region-Unchecked-Unsafe-Err                                                              |

### 8.8. W-MOD (Module Warnings)

| Code         | Severity | Detection    | Condition                                                      |
| ------------ | -------- | ------------ | -------------------------------------------------------------- |
| `W-MOD-1101` | Warning  | Compile-time | Potential module path collision on case-insensitive filesystem |

### 8.9. W-SRC (Source Warnings)

| Code         | Severity | Detection    | Condition                                                   |
| ------------ | -------- | ------------ | ----------------------------------------------------------- |
| `W-SRC-0101` | Warning  | Compile-time | UTF-8 BOM present at the start of the file                  |
| `W-SRC-0301` | Warning  | Compile-time | Leading zeros in decimal literal                            |
| `W-SRC-0308` | Warning  | Compile-time | Lexically sensitive Unicode character within `unsafe` block |

### 8.10. E-TYP (Types)

| Code         | Severity | Detection    | Condition                                                                                              | DiagId                                                                                                                                   |
| ------------ | -------- | ------------ | ------------------------------------------------------------------------------------------------------ | ---------------------------------------------------------------------------------------------------------------------------------------- |
| `E-TYP-1101` | Error    | Compile-time | `shared` permission not supported in Cursive0                                                          | Perm-Shared-Unsupported                                                                                                                  |
| `E-TYP-1505` | Error    | Compile-time | Missing required type annotation at module scope                                                       | WF-StaticDecl-MissingType, WF-ProcedureDecl-MissingReturnType                                                                            |
| `E-TYP-1530` | Error    | Compile-time | Type inference failed; unable to determine type                                                        | T-LetStmt-Infer-Err, T-VarStmt-Infer-Err, T-ShadowLetStmt-Infer-Err, T-ShadowVarStmt-Infer-Err, PtrNull-Infer-Err, NullLiteral-Infer-Err |
| `E-TYP-1506` | Error    | Compile-time | Type alias cycle detected                                                                              | TypeAlias-Recursive-Err                                                                                                                  |
| `E-TYP-1507` | Error    | Compile-time | Procedure with non-unit return type requires explicit return statement                                 | WF-ProcBody-ExplicitReturn-Err                                                                                                           |
| `E-TYP-1601` | Error    | Compile-time | Mutation through `const` path                                                                          | B-Assign-Const-Err                                                                                                                       |
| `E-TYP-1602` | Error    | Compile-time | `unique` exclusion violation (aliasing or inactive use)                                                | B-Place-Unique-Err                                                                                                                       |
| `E-TYP-1603` | Error    | Compile-time | Non-`move` argument must be a place expression                                                         | Call-Arg-NotPlace                                                                                                                        |
| `E-TYP-1605` | Error    | Compile-time | Receiver permission incompatible with caller                                                           | MethodCall-RecvPerm-Err                                                                                                                  |
| `E-TYP-1801` | Error    | Compile-time | Tuple index out of bounds                                                                              | TupleIndex-OOB                                                                                                                           |
| `E-TYP-1802` | Error    | Compile-time | Tuple index is not a compile-time constant integer literal                                             | TupleIndex-NonConst                                                                                                                      |
| `E-TYP-1803` | Error    | Compile-time | Tuple arity mismatch in assignment or pattern                                                          | Pat-Tuple-Arity-Err, Pat-Tuple-R-Arity-Err                                                                                               |
| `E-TYP-1810` | Error    | Compile-time | Array length is not a compile-time constant                                                            | ConstLen-Err                                                                                                                             |
| `E-TYP-1812` | Error    | Compile-time | Array index expression has non-`usize` type                                                            | Index-Array-NonUsize, AddrOf-Index-Array-NonUsize                                                                                        |
| `E-TYP-1820` | Error    | Compile-time | Slice index expression has non-`usize` type                                                            | Index-Slice-NonUsize, AddrOf-Index-Slice-NonUsize                                                                                        |
| `E-TYP-1821` | Error    | Compile-time | Direct slice indexing not permitted in Cursive0 subset                                                 | Index-Slice-Direct-Err                                                                                                                   |
| `E-TYP-1901` | Error    | Compile-time | Duplicate field name in record declaration                                                             | WF-Record-DupField                                                                                                                       |
| `E-TYP-1902` | Error    | Compile-time | Missing field initializer in record literal                                                            | Record-FieldInit-Missing                                                                                                                 |
| `E-TYP-1903` | Error    | Compile-time | Duplicate field initializer in record literal                                                          | Record-FieldInit-Dup                                                                                                                     |
| `E-TYP-1904` | Error    | Compile-time | Access to nonexistent field                                                                            | FieldAccess-Unknown, Record-Field-Unknown                                                                                                |
| `E-TYP-1905` | Error    | Compile-time | Access to field not visible in current scope                                                           | FieldAccess-NotVisible, Record-Field-NotVisible                                                                                          |
| `E-TYP-1906` | Error    | Compile-time | Field visibility exceeds record visibility                                                             | FieldVisOk-Err                                                                                                                           |
| `E-TYP-1907` | Error    | Compile-time | Non-`Bitcopy` field requires move source expression                                                    | Record-Field-NonBitcopy-Move                                                                                                             |
| `E-TYP-1911` | Error    | Compile-time | Default record construction requires default initializer for every field                               | Record-Default-Init-Err                                                                                                                  |
| `E-TYP-1912` | Error    | Compile-time | Explicit receiver type must be `Self` for record methods                                               | Record-Method-RecvSelf-Err                                                                                                               |
| `E-TYP-1920` | Error    | Compile-time | Enum discriminant is not an integer literal                                                            | Enum-Disc-NotInt                                                                                                                         |
| `E-TYP-1921` | Error    | Compile-time | Enum discriminant literal is invalid                                                                   | Enum-Disc-Invalid                                                                                                                        |
| `E-TYP-1922` | Error    | Compile-time | Enum discriminant must be non-negative                                                                 | Enum-Disc-Negative                                                                                                                       |
| `E-TYP-1923` | Error    | Compile-time | Duplicate enum discriminant value                                                                      | Enum-Disc-Dup                                                                                                                            |
| `E-TYP-2050` | Error    | Compile-time | Modal type declares zero states                                                                        | Modal-NoStates-Err                                                                                                                       |
| `E-TYP-2051` | Error    | Compile-time | Duplicate state name within modal type                                                                 | Modal-DupState-Err                                                                                                                       |
| `E-TYP-2052` | Error    | Compile-time | Field access for field not present in current state's payload                                          | Modal-Field-Missing                                                                                                                      |
| `E-TYP-2053` | Error    | Compile-time | Method invocation for method not available in current state                                            | Modal-Method-NotFound                                                                                                                    |
| `E-TYP-2054` | Error    | Compile-time | State name collides with modal type name                                                               | Modal-StateName-Err                                                                                                                      |
| `E-TYP-2055` | Error    | Compile-time | Transition body returns value not matching declared target state                                       | Transition-Body-Err                                                                                                                      |
| `E-TYP-2056` | Error    | Compile-time | Transition invoked on value not of declared source state                                               | Transition-Source-Err                                                                                                                    |
| `E-TYP-2057` | Error    | Compile-time | Direct field access on general modal type without pattern matching                                     | Modal-Field-General-Err                                                                                                                  |
| `E-TYP-2058` | Error    | Compile-time | Duplicate field name in modal state payload                                                            | Modal-Payload-DupField                                                                                                                   |
| `E-TYP-2059` | Error    | Compile-time | Transition target state not declared in modal type                                                     | Transition-Target-Err                                                                                                                    |
| `E-TYP-2060` | Error    | Compile-time | Non-exhaustive match on general modal type                                                             | Match-Modal-NonExhaustive                                                                                                                |
| `E-TYP-2061` | Error    | Compile-time | Duplicate method name in modal state                                                                   | StateMethod-Dup                                                                                                                          |
| `E-TYP-2062` | Error    | Compile-time | Duplicate transition name in modal state                                                               | Transition-Dup                                                                                                                           |
| `E-TYP-2063` | Error    | Compile-time | State member visibility exceeds modal visibility                                                       | StateMemberVisOk-Err                                                                                                                     |
| `E-TYP-2064` | Error    | Compile-time | State member not visible in current scope                                                              | Modal-Field-NotVisible, Transition-NotVisible, Modal-Method-NotVisible                                                                   |
| `E-TYP-2070` | Error    | Compile-time | Implicit widening on non-niche-layout-compatible type                                                  | Chk-Subsumption-Modal-NonNiche                                                                                                           |
| `E-TYP-2071` | Error    | Compile-time | `widen` applied to non-modal type                                                                      | Widen-NonModal                                                                                                                           |
| `E-TYP-2072` | Error    | Compile-time | `widen` applied to already-general modal type                                                          | Widen-AlreadyGeneral                                                                                                                     |
| `E-TYP-2073` | Error    | Compile-time | Record literal whose type is `File@S` or `DirIter@S` for any state `S` in the corresponding modal type | Record-FileDir-Err                                                                                                                       |
| `E-TYP-2101` | Error    | Compile-time | Dereference of pointer in `@Null` state                                                                | Deref-Null                                                                                                                               |
| `E-TYP-2102` | Error    | Compile-time | Dereference of pointer in `@Expired` state                                                             | Deref-Expired                                                                                                                            |
| `E-TYP-2103` | Error    | Compile-time | Dereference of raw pointer outside `unsafe`                                                            | Deref-Raw-Unsafe                                                                                                                         |
| `E-TYP-2104` | Error    | Compile-time | Address-of applied to non-place expression                                                             | AddrOf-NonPlace                                                                                                                          |
| `E-MEM-3031` | Error    | Compile-time | `transmute` source and target sizes differ                                                             | T-Transmute-SizeEq                                                                                                                       |
| `E-TYP-2201` | Error    | Compile-time | Union type has fewer than two member types                                                             | WF-Union-TooFew                                                                                                                          |
| `E-TYP-2202` | Error    | Compile-time | Direct access on union value without pattern matching                                                  | Union-DirectAccess-Err                                                                                                                   |
| `E-TYP-2402` | Error    | Compile-time | Implementing type missing required field                                                               | Impl-Field-Missing                                                                                                                       |
| `E-TYP-2404` | Error    | Compile-time | Implementing field has incompatible type                                                               | Impl-Field-Type-Err                                                                                                                      |
| `E-TYP-2406` | Error    | Compile-time | Conflicting field names from multiple classes                                                          | EffFields-Conflict                                                                                                                       |
| `E-TYP-2408` | Error    | Compile-time | Duplicate abstract field name in class                                                                 | Class-AbstractField-Dup                                                                                                                  |
| `E-TYP-2500` | Error    | Compile-time | Duplicate procedure name in class                                                                      | Class-Method-Dup                                                                                                                         |
| `E-TYP-2501` | Error    | Compile-time | `override` used on abstract procedure implementation                                                   | Override-Abstract-Err                                                                                                                    |
| `E-TYP-2502` | Error    | Compile-time | Missing `override` on concrete procedure replacement                                                   | Override-Missing-Err                                                                                                                     |
| `E-TYP-2503` | Error    | Compile-time | Type does not implement required procedure from class or has incompatible signature                    | Impl-Missing-Method, Impl-Sig-Err, Impl-Sig-Err-Concrete                                                                                 |
| `E-TYP-2505` | Error    | Compile-time | Name conflict among class members                                                                      | Class-Name-Conflict                                                                                                                      |
| `E-TYP-2506` | Error    | Compile-time | Coherence violation: duplicate class implementation                                                    | Impl-Dup, Impl-Duplicate-Class-Err                                                                                                       |
| `E-TYP-2508` | Error    | Compile-time | Cyclic superclass dependency detected                                                                  | Superclass-Cycle                                                                                                                         |
| `E-TYP-2509` | Error    | Compile-time | Superclass bound refers to undefined class                                                             | Superclass-Undefined                                                                                                                     |
| `E-TYP-2541` | Error    | Compile-time | Dynamic class type created from non-dispatchable class                                                 | Dynamic-NonDispatchable                                                                                                                  |
| `E-TYP-2621` | Error    | Compile-time | Type implements both `Bitcopy` and `Drop`                                                              | BitcopyDrop-Conflict                                                                                                                     |
| `E-TYP-2622` | Error    | Compile-time | `Bitcopy` type has non-`Bitcopy` field                                                                 | Bitcopy-Field-NonBitcopy                                                                                                                 |
| `E-TYP-2623` | Error    | Compile-time | Type implementing `Bitcopy` does not implement `Clone`                                                              | Bitcopy-Clone-Missing                                                                                                                    |

### 8.11. W-SYS (System Warnings)

| Code         | Severity | Detection    | Condition                                               |
| ------------ | -------- | ------------ | ------------------------------------------------------- |
| `W-SYS-4010` | Warning  | Compile-time | Modal widening involves large payload copy (>256 bytes) |

### 8.12. E-SEM (Semantics)

| Code         | Severity | Detection    | Condition                                            | DiagId                                            |
| ------------ | -------- | ------------ | ---------------------------------------------------- | ------------------------------------------------- |
| `E-SEM-2524` | Error    | Compile-time | Tuple access on non-tuple                            | TupleAccess-NotTuple                              |
| `E-SEM-2527` | Error    | Compile-time | Indexing applied to non-indexable type               | Index-NonIndexable                                |
| `E-SEM-2531` | Error    | Compile-time | Callee expression is not of FUNCTION type            | Call-Callee-NotFunc                               |
| `E-SEM-2532` | Error    | Compile-time | Argument count mismatch                              | Call-ArgCount-Err                                 |
| `E-SEM-2533` | Error    | Compile-time | Argument type incompatible with parameter type       | Call-ArgType-Err                                  |
| `E-SEM-2534` | Error    | Compile-time | `move` argument required but not provided            | Call-Move-Missing                                 |
| `E-SEM-2535` | Error    | Compile-time | `move` argument provided but parameter is not `move` | Call-Move-Unexpected                              |
| `E-SEM-2536` | Error    | Compile-time | Method not found for receiver type                   | LookupMethod-NotFound, LookupClassMethod-NotFound |
| `E-SEM-2705` | Error    | Compile-time | `match` expression is not exhaustive for union type  | Match-Union-NonExhaustive                         |
| `E-SEM-2711` | Error    | Compile-time | Refutable pattern in irrefutable context (`let`)     | Let-Refutable-Pattern-Err                         |
| `E-SEM-2713` | Error    | Compile-time | Duplicate binding identifier within single pattern   | Pat-Dup-Err, Pat-Dup-R-Err                        |
| `E-SEM-2721` | Error    | Compile-time | Range pattern bounds are not compile-time constants  | RangePattern-NonConst                             |
| `E-SEM-2722` | Error    | Compile-time | Range pattern start exceeds end (empty range)        | RangePattern-Empty                                |
| `E-SEM-2731` | Error    | Compile-time | Record pattern references non-existent field         | RecordPattern-UnknownField                        |
| `E-SEM-3011` | Error    | Compile-time | Method defined outside of type context               | Method-Context-Err                                |
| `E-SEM-3012` | Error    | Compile-time | Duplicate method name in type                        | Record-Method-Dup                                 |
| `E-SEM-3131` | Error    | Compile-time | Assignment target is not a place expression          | Assign-NotPlace                                   |
| `E-SEM-3132` | Error    | Compile-time | Assignment through `const` permission                | Assign-Const-Err                                  |
| `E-SEM-3133` | Error    | Compile-time | Assignment type mismatch                             | Assign-Type-Err                                   |
| `E-SEM-3151` | Error    | Compile-time | Defer block has non-unit type                        | Defer-NonUnit-Err                                 |
| `E-SEM-3152` | Error    | Compile-time | Non-local control flow in defer block                | Defer-NonLocal-Err                                |
| `E-SEM-3161` | Error    | Compile-time | `return` type mismatch with procedure                | Return-Type-Err                                   |
| `E-SEM-3162` | Error    | Compile-time | `break` outside `loop`                               | Break-Outside-Loop                                |
| `E-SEM-3163` | Error    | Compile-time | `continue` outside `loop`                            | Continue-Outside-Loop                             |
| `E-SEM-3164` | Error    | Compile-time | `result` type mismatch with block                    | BlockInfo-Res-Err                                 |
| `E-SEM-3165` | Error    | Compile-time | `return` at module scope                             | Return-At-Module-Err                              |

### 8.13. W-SEM (Semantic Warnings)

| Code         | Severity | Detection    | Condition                               | DiagId                  |
| ------------ | -------- | ------------ | --------------------------------------- | ----------------------- |
| `W-SEM-1001` | Warning  | Compile-time | Unreachable code after result statement | Warn-Result-Unreachable |

## 9. Appendix B - Notation and Glossary

### 9.1. Notation Conventions
F(x_1, …, x_n) = F^(n)(x_1, …, x_n)
n ≠ m ⇒ F^(n) ≠ F^(m)

### 9.2. Helper Functions and Relations
UnicodeNFC_15.0.0 and UnicodeCaseFold_15.0.0 are defined by the Unicode Standard 15.0.0 and are normative for this document
LLVMText_21 and LLVMObj_21 are defined by LLVM 21 tool acceptance for textual IR and object emission respectively

**HelperRef.**
HelperRef(Fold) = `"2.3.2"`
HelperRef(DirSeq) = `"2.3.3"`
HelperRef(Modules) = `"2.4"`
HelperRef(ModuleList) = `"2.5"`
HelperRef(OutputRoot) = `"2.5"`
HelperRef(OutputPaths) = `"2.5"`
HelperRef(SearchDirs) = `"2.6"`
HelperRef(ResolveTool) = `"2.6"`
HelperRef(AssembleIR) = `"2.6"`
HelperRef(IdKey) = `"3.1.6"`
HelperRef(IdEq) = `"3.1.6"`
HelperRef(CaseFold) = `"3.1.6"`
HelperRef(Span) = `"1.6"`
HelperRef(SpanOf) = `"1.6"`
HelperRef(ClampSpan) = `"1.6"`
HelperRef(AliasExpand) = `"5.12"`
HelperRef(ModulePrefix) = `"5.12"`
HelperRef(Reachable) = `"5.12"`
HelperRef(TypeRefsTy) = `"5.12"`
HelperRef(TypeRefsRef) = `"5.12"`
HelperRef(TypeRefsExpr) = `"5.12"`
HelperRef(TypeRefsPat) = `"5.12"`
HelperRef(ValueRefs) = `"5.12"`
HelperRef(ValueRefsArgs) = `"5.12"`
HelperRef(ValueRefsFields) = `"5.12"`
HelperRef(Topo) = `"7.1"`
HelperRef(EntryKey) = `"7.7"`
HelperRef(PathOrderKey) = `"6.1.4.1"`
HelperRef(TypeKey) = `"6.1.4.1"`
HelperRef(≺_type) = `"6.1.4.1"`
HelperRef(Members) = `"5.2.7"`
HelperRef(MemberList) = `"6.1.4.1"`
HelperRef(DistinctMembers) = `"6.1.4.1"`
HelperRef(SingleFieldPayload) = `"6.1.4.1"`
HelperRef(PayloadState) = `"6.1.4.1"`
HelperRef(EmptyState) = `"6.1.4.1"`
HelperRef(NicheSet) = `"6.1.4.1"`
HelperRef(NicheCount) = `"6.1.4.1"`
HelperRef(NicheOrder) = `"6.1.4.1"`
HelperRef(NicheApplies) = `"6.1.4.1"`
HelperRef(StaticJudgments) = `"1.2"`
HelperRef(PremisesHold) = `"1.2"`
HelperRef(PermSyntax) = `"5.2.2"`
HelperRef(Behavior) = `"1.2"`
HelperRef(ResourceExhaustion) = `"1.3"`
HelperRef(OutsideConformance) = `"1.3"`
HelperRef(SpanRange) = `"1.6.1"`
HelperRef(SeverityColumn) = `"8"`
HelperRef(ConditionColumn) = `"8"`
HelperRef(HasError) = `"1.6.3"`
HelperRef(CompileStatus) = `"1.6.3"`
HelperRef(NoDiag) = `"1.6.4"`
HelperRef(HostPrim) = `"1.7"`
HelperRef(FSPrim) = `"1.7"`
HelperRef(FilePrim) = `"1.7"`
HelperRef(DirPrim) = `"1.7"`
HelperRef(MapsToDiagOrRuntime) = `"1.7"`
HelperRef(WinSep) = `"2.1"`
HelperRef(AbsPath) = `"2.1"`
HelperRef(DriveRooted) = `"2.1"`
HelperRef(UNC) = `"2.1"`
HelperRef(RootRelative) = `"2.1"`
HelperRef(FoldPath) = `"2.3"`
HelperRef(FileKey) = `"2.3"`
HelperRef(DirKey) = `"2.3"`
HelperRef(Basename) = `"2.3"`
HelperRef(FileExt) = `"2.1"`
HelperRef(Utf8LexLess) = `"2.3"`
HelperRef(BMap) = `"2.5"`
HelperRef(Hex2) = `"2.5"`
HelperRef(Concat) = `"2.5"`
HelperRef(Overwrites) = `"2.5"`
HelperRef(IsDir) = `"2.5"`
Overwrites(p, b) ⇔ ∃ fs, ω, ω'. FSWriteFile(fs, p, b, ω) ⇓ (ok, ω')
IsDir(p) ⇔ ∃ fs, ω, ω'. FSKind(fs, p, ω) ⇓ (`Dir`, ω')

**Auxiliary String/Sequence Operators.**
At(s, i) = s[i]
StartsWith(s, p) ⇔ s[0..|p|) = p
EndsWith(s, p) ⇔ s[|s| - |p|..|s|) = p
Remove(s, c) = [ s[i] | 0 ≤ i < |s| ∧ s[i] ≠ c ]
Concat([]) = `"\""`
Concat([s]) = s
Concat(s::ss) = s ++ Concat(ss)    (|ss| > 0)
HexDigit(0) = `"0"` … HexDigit(9) = `"9"` … HexDigit(15) = `"f"`
Hex2(b) = HexDigit(⌊b/16⌋) ++ HexDigit(b mod 16)


**Hex Values.**
HexDigitValue(`'0'`) = 0    HexDigitValue(`'1'`) = 1    HexDigitValue(`'2'`) = 2    HexDigitValue(`'3'`) = 3    HexDigitValue(`'4'`) = 4    HexDigitValue(`'5'`) = 5    HexDigitValue(`'6'`) = 6    HexDigitValue(`'7'`) = 7    HexDigitValue(`'8'`) = 8    HexDigitValue(`'9'`) = 9
HexDigitValue(`'a'`) = 10    HexDigitValue(`'b'`) = 11    HexDigitValue(`'c'`) = 12    HexDigitValue(`'d'`) = 13    HexDigitValue(`'e'`) = 14    HexDigitValue(`'f'`) = 15
HexDigitValue(`'A'`) = 10    HexDigitValue(`'B'`) = 11    HexDigitValue(`'C'`) = 12    HexDigitValue(`'D'`) = 13    HexDigitValue(`'E'`) = 14    HexDigitValue(`'F'`) = 15
HexValue(h_1…h_n) = ∑_{k=1}^{n} HexDigitValue(h_k) · 16^(n-k)
DecDigitValue(`'0'`) = 0    DecDigitValue(`'1'`) = 1    DecDigitValue(`'2'`) = 2    DecDigitValue(`'3'`) = 3    DecDigitValue(`'4'`) = 4    DecDigitValue(`'5'`) = 5    DecDigitValue(`'6'`) = 6    DecDigitValue(`'7'`) = 7    DecDigitValue(`'8'`) = 8    DecDigitValue(`'9'`) = 9
OctDigitValue(`'0'`) = 0    OctDigitValue(`'1'`) = 1    OctDigitValue(`'2'`) = 2    OctDigitValue(`'3'`) = 3    OctDigitValue(`'4'`) = 4    OctDigitValue(`'5'`) = 5    OctDigitValue(`'6'`) = 6    OctDigitValue(`'7'`) = 7
BinDigitValue(`'0'`) = 0    BinDigitValue(`'1'`) = 1
DecValue(d_1…d_n) = ∑_{k=1}^{n} DecDigitValue(d_k) · 10^(n-k)
OctValue(d_1…d_n) = ∑_{k=1}^{n} OctDigitValue(d_k) · 8^(n-k)
BinValue(d_1…d_n) = ∑_{k=1}^{n} BinDigitValue(d_k) · 2^(n-k)

**Rule-Section Map.**
SectionId(r) ∈ String
RulesIn(Σ) = { r | SectionId(r) ∈ Σ }

**EmitList.**
EmitList([]) = ok
EmitList([d] ++ ds) = (Γ ⊢ Emit(d)) ∧ EmitList(ds)

**ArgMax.**
argmax_{x ∈ C} g(x) = x* ⇔ x* ∈ C ∧ ∀ y ∈ C. g(x*) ≥ g(y) ∧ (∀ z ∈ C. g(z) = g(x*) ⇒ z = x*)








