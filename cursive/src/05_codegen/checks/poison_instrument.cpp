// =============================================================================
// MIGRATION MAPPING: poison_instrument.cpp
// =============================================================================
//
// SPEC REFERENCE: C0updated.md
//   - Section 7.1 Initialization Order and Poisoning
//   - SetPoison judgment (referenced in InitPanicHandle)
//   - Module poison propagation
//   - InitPanicHandle rule (line 16989)
//
// SOURCE FILE: cursive-bootstrap/src/04_codegen/checks.cpp
//   - Lines 18-72: PoisonSetFor helper
//   - Computes transitive closure of dependent modules
//   - Uses init_modules and init_eager_edges from context
//
// DEPENDENCIES:
//   - cursive/include/05_codegen/checks/poison_instrument.h
//   - cursive/include/05_codegen/ir/ir_model.h (IRSetPoison)
//   - cursive/include/00_core/symbols.h (StringOfPath)
//
// REFACTORING NOTES:
//   1. Poisoning marks modules as failed during init
//   2. PoisonSetFor computes which modules to poison
//   3. Uses graph reachability from module node
//   4. Prevents use of partially-initialized modules
//   5. Set includes module itself + transitive dependents
//
// POISON SET COMPUTATION:
//   PoisonSetFor(module, ctx):
//     1. Build module index map
//     2. Build outgoing edge graph from init_eager_edges
//     3. DFS from target module
//     4. Collect all reachable modules
//     5. Return module paths as strings
//
// GRAPH TRAVERSAL:
//   - init_modules: ordered list of all modules
//   - init_eager_edges: (from, to) dependency pairs
//   - Traverse outgoing edges from module
//   - Mark visited, collect reachable set
//
// USAGE IN INIT:
//   If module A's init panics:
//     1. Compute PoisonSetFor(A)
//     2. Mark all modules in set as poisoned
//     3. Any access to poisoned module -> panic
// =============================================================================

#include "cursive/05_codegen/checks/poison_instrument.h"

// TODO: Migrate implementation from cursive-bootstrap/src/04_codegen/checks.cpp

