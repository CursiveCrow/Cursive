# Arena as Modal Type: Implementation Summary

**Date**: November 8, 2025  
**Status**: ✅ Complete  
**Impact**: Major design improvement avoiding Rust lifetime complexity

---

## Overview

Successfully redesigned Cursive's region allocation system to use `Arena` as a built-in modal type rather than deferring region type parameters. This maintains "regions not lifetimes" philosophy while enabling advanced arena control patterns.

## Key Design Decision

**Rejected**: Region type parameters (`List<^r, T>`) - would reintroduce Rust lifetime complexity

**Adopted**: Arena as first-class modal type with explicit state machine

## The Arena Modal Type

```cursive
modal Arena {
    @Active {
        ptr: Ptr<u8>,
        capacity: usize,
        allocated: usize,
    }

    @Active::alloc<T>(~!, value: T) -> @Active
    @Active::alloc_array<T>(~!, count: usize) -> @Active
    @Active::reset(~!) -> @Active
    @Active::freeze(~) -> @Frozen
    @Active::free(~!) -> @Freed

    @Frozen {
        ptr: Ptr<u8>,
        allocated: usize,
    }

    @Frozen::thaw(~!) -> @Active
    @Frozen::free(~!) -> @Freed

    @Freed { }
}
```

## State Transition Graph

```
@Active ──reset()──> @Active    (bulk free, reuse memory)
   │
   │ freeze()
   ↓
@Frozen ──thaw()───> @Active    (resume allocation)
   │              │
   │ free()   free()
   ↓              ↓
@Freed <────────────            (terminal state)
```

## Syntax Extensions

### Named Arena Bindings

```cursive
// Anonymous arena (implicit):
region temp {
    let x = ^Value::new()
}

// Named arena binding:
region session as workspace {
    let x = workspace.alloc(Value::new())  // Explicit
    let y = ^Value::new()                   // Sugar (equivalent)
}
```

### Arena Parameters

```cursive
procedure fill_arena(workspace: Arena@Active, items: [Item])
    [[ alloc::region |- items.len() > 0 => true ]]
{
    loop item in items {
        workspace.alloc(transform(item))
    }
}
```

### Caret Operator Desugaring

```cursive
region temp as workspace {
    let x = ^Value::new()
}

// Desugars to:
region temp as workspace {
    let x = workspace.alloc(Value::new())
}
```

## Arena Pools

Multiple independent arenas via nesting:

```cursive
region persist as persistent {
    region temp as temporary {
        persistent.alloc(permanent_data())
        temporary.alloc(scratch_data())
    }
    // temp freed, persist still active
}
```

Advanced pools (standard library):

```cursive
let pool = ArenaPool::new(count: 4)
pool.scoped(|arenas| {
    // arenas: [Arena@Active]
    loop item in items {
        arenas[item.id % 4].alloc(process(item))
    }
})
```

## Files Modified

### Core Specifications

1. **Spec/11_Memory-Model-Regions-and-Permissions/11-3_Regions.md**

   - Added §11.3.2 Arena Modal Type (complete modal declaration)
   - Updated §11.3.3 to explain caret operator as sugar for arena.alloc()
   - Renumbered sections (§11.3.3 → §11.3.4, etc.)
   - Added §11.3.7 Arena Passing and Pools
   - Added E11-110 diagnostic for arena escape
   - Updated conformance requirements

2. **Spec/10_Generics-and-Behaviors/10-1_Region-Parameters.md**

   - Rewrote §10.1.7 "Future Extension" to "Design Decision: No Region Type Parameters"
   - Added comprehensive rationale explaining why type parameters would be Rust lifetimes in disguise
   - Documented Arena modal type as the correct approach

3. **Spec/07_Types/07-6_Modal-Types.md**

   - Added Arena to list of built-in modal types (alongside Ptr)
   - Updated examples to include arena lifecycle

4. **Spec/03_Basic-Concepts/03-4_Storage-Duration.md**
   - Added note explaining region storage implemented via Arena modal type

### Grammar and Diagnostics

5. **Spec/Annex/A_Grammar.md**

   - Added `region_expr` production with `('as' ident)?` clause
   - Added commented Arena modal declaration for reference

6. **Spec/Annex/E_Implementation-Diagnostics.md**
   - Added E11-110 diagnostic code for arena binding escape
   - Updated section references for renumbered sections

### Documentation

7. **Reviews/Comprehensive_Review_2025-11-08_Complete.md**
   - Added "Post-Review Updates" section documenting this implementation
   - Noted resolution of region type parameter concern

## Technical Benefits

### Avoids Rust Lifetime Complexity

**Rust lifetimes** (what we avoided):

- Type parameters: `<'a, 'b, T>`
- Variance rules for lifetime parameters
- Outlives bounds: `'a: 'b`
- Borrow checker entanglement
- Pervasive annotations

**Cursive Arena** (what we implemented):

- First-class modal values
- Explicit state machine
- Runtime arena management
- No type pollution
- Opt-in complexity

### Enables New Patterns

1. **Arena passing**: Pass arenas to helper procedures
2. **Explicit reset**: Reuse arena memory mid-scope
3. **Freeze/thaw**: Prevent allocation while keeping data accessible
4. **Arena pools**: Multiple independent arenas (nested or via library)

### Maintains Cursive Philosophy

✅ **Explicit over implicit**: `workspace.alloc()` or `^` (clear what's happening)
✅ **Regions not lifetimes**: Arenas are spatial (memory pools), not temporal (reference validity)
✅ **Modal types for lifecycle**: State machine tracks arena state
✅ **No type-level complexity**: Arena is value, not type parameter
✅ **Zero cost when unused**: Provenance metadata for simple cases, modal types when needed

## Example: Before and After

### Before (Deferred Feature)

```cursive
// Could only use implicit region:
region temp {
    let x = ^allocate()
    // No way to name or pass the region
}

// Unclear how to:
// - Pass region to helper procedures
// - Reset and reuse arena memory
// - Track arena lifecycle explicitly
```

### After (Arena Modal Type)

```cursive
// Named arena binding:
region session as workspace {
    helper_procedure(workspace)

    workspace.reset()  // Explicit bulk free and reuse

    more_allocations(workspace)
}

// Arena parameters:
procedure helper_procedure(arena: Arena@Active)
    [[ alloc::region |- true => true ]]
{
    loop item in get_items() {
        arena.alloc(process(item))
    }
}

// Arena lifecycle:
region batch as workspace {
    allocate_phase(workspace)      // Arena@Active
    let frozen = workspace.freeze() // Arena@Frozen
    validate_phase(frozen)
    let active = frozen.thaw()      // Arena@Active
    cleanup_phase(active)
}  // auto-freed
```

## Why This Is Superior

1. **No Rust lifetimes**: Arena is a value, not an abstract lifetime bound
2. **Modal states**: Lifecycle explicit through @Active/@Frozen/@Freed
3. **Passable**: Arena can be parameter (non-responsible binding semantics)
4. **Reusable**: `reset()` enables memory reuse without deallocation
5. **Flexible**: Freeze/thaw for phased processing
6. **Simple**: No variance, no subtyping, no generic complexity

## Comparison Table

| Feature        | Rust Lifetimes            | Cursive Arena Modal Type |
| -------------- | ------------------------- | ------------------------ |
| Syntax         | `<'a, T>`                 | `Arena@Active`           |
| Purpose        | Prevent dangling refs     | Manage allocation arenas |
| Type pollution | High (every type)         | Zero (arenas are values) |
| Complexity     | High (variance, outlives) | Low (modal states)       |
| Opt-in         | No (required)             | Yes (named bindings)     |
| Passable       | As bounds only            | As parameters            |
| Runtime ops    | No (compile-time)         | Yes (reset, freeze)      |

## Integration Points

The Arena modal type integrates with existing Cursive features:

- **Modal types (§7.6)**: Arena uses same state machine mechanism as FileHandle, Connection, etc.
- **Non-responsible bindings (§11.2)**: Arena bindings from `region ... as` are non-responsible
- **Escape analysis (§11.3.4)**: Arena bindings cannot escape (diagnostic E11-110)
- **RAII (§11.2.5)**: Arena automatically freed at region exit via defer
- **Provenance metadata (§11.3.4.2)**: Values allocated through arena get Region(r) provenance

## Conclusion

This implementation demonstrates that Cursive can provide sophisticated memory management (arena allocation, lifecycle control, pooling) **without resorting to type-level lifetime parameters**. The Arena modal type approach is:

- ✅ More explicit (arena operations visible)
- ✅ More flexible (runtime arena management)
- ✅ Simpler (no variance/subtyping rules)
- ✅ More Cursive-like (values and modal states, not type parameters)
- ✅ Distinctly different from Rust (arenas not lifetimes)

**Status**: This design is ready for inclusion in Cursive v1.0. No further work needed on region type parameters.
