/*
 * =============================================================================
 * MIGRATION MAPPING: init_planner.cpp
 * =============================================================================
 *
 * SPEC REFERENCE:
 *   - C0updated.md, Section 9.1 "Module Initialization" (lines 21800-21900)
 *   - C0updated.md, Section 9.2 "Static Initialization Order" (lines 21910-22000)
 *   - C0updated.md, Section 9.3 "Dependency Analysis" (lines 22010-22100)
 *
 * SOURCE FILE:
 *   - cursive-bootstrap/src/03_analysis/memory/init_planner.cpp (lines 1-1974)
 *
 * FUNCTIONS TO MIGRATE:
 *   - BuildDependencyGraph(Module* mod) -> DepGraph                [lines 50-200]
 *       Construct module-level dependency graph for initialization
 *   - TopologicalSort(DepGraph& graph) -> Vec<Decl*>               [lines 205-350]
 *       Sort declarations by initialization order
 *   - DetectCycles(DepGraph& graph) -> Vec<Cycle>                  [lines 355-500]
 *       Find circular dependencies in initialization
 *   - ComputeInitOrder(Vec<Module*> modules) -> InitPlan           [lines 505-700]
 *       Compute cross-module initialization ordering
 *   - AnalyzeStaticDecl(Decl* decl) -> DepSet                      [lines 705-900]
 *       Find dependencies of a static declaration
 *   - ValidateInitOrder(InitPlan& plan) -> bool                    [lines 905-1100]
 *       Validate that init order satisfies all dependencies
 *   - EmitInitSequence(InitPlan& plan) -> IR                       [lines 1105-1400]
 *       Generate initialization code sequence
 *   - HandleLazyInit(Decl* decl) -> LazyInitBlock                  [lines 1405-1600]
 *       Handle lazy initialization for complex statics
 *   - AnalyzeConstInit(Expr* init) -> bool                         [lines 1605-1800]
 *       Check if initializer is compile-time constant
 *   - PlanModuleInit(Module* mod) -> ModuleInitPlan                [lines 1805-1974]
 *       Create complete initialization plan for a module
 *
 * DEPENDENCIES:
 *   - Module, Decl, Expr from AST
 *   - DepGraph for dependency tracking
 *   - Topological sort algorithm
 *   - Constant evaluation for compile-time init
 *
 * REFACTORING NOTES:
 *   1. Static declarations must be initialized in dependency order
 *   2. Circular dependencies are ill-formed (E-INIT-0001)
 *   3. Cross-module dependencies require careful ordering
 *   4. Compile-time constants can be initialized statically
 *   5. Non-constant statics require runtime initialization
 *   6. Consider lazy initialization for expensive computations
 *   7. This is a large file - consider splitting into submodules:
 *      - dep_graph.cpp: Graph construction and cycle detection
 *      - topo_sort.cpp: Topological sorting
 *      - const_eval.cpp: Constant expression evaluation
 *      - init_emit.cpp: IR generation for init sequence
 *
 * DIAGNOSTIC CODES:
 *   - E-INIT-0001: Circular initialization dependency
 *   - E-INIT-0002: Non-constant static initializer
 *   - E-INIT-0003: Cross-module initialization cycle
 *   - W-INIT-0001: Complex initialization may have runtime cost
 *
 * =============================================================================
 */
