# Cursive Language Specification

## Clause 10 — Memory Model, Regions, and Permissions

**Part**: X — Memory Model, Regions, and Permissions
**File**: 11-1_Memory-Model-Overview.md  
**Section**: §10.1 Memory Model Overview  
**Stable label**: [memory.overview]  
**Forward references**: §10.2 [memory.object], §10.3 [memory.region], §10.4 [memory.permission], §10.5 [memory.move], Clause 9 [generic], Clause 11 [contract]

---

### §10.1 Memory Model Overview [memory.overview]

#### §10.1.1 Overview and Design Goals

[1] Cursive achieves memory safety through compile-time enforcement of cleanup responsibility, permission-based access control, and region-scoped allocation—without garbage collection or borrow checking. The memory model ensures that well-typed programs cannot produce use-after-free errors, double-free errors, or dangling references.

[2] The model is built on three foundational principles:

- **Deterministic cleanup**: Resources are released at statically determined program points via RAII (Resource Acquisition Is Initialization)
- **Explicit transfers**: Cleanup responsibility transfers require the explicit `move` keyword at call sites
- **Zero runtime overhead**: All safety mechanisms operate at compile time with no runtime cost

#### §10.1.2 Two-Axis Orthogonal Design

[4] Cursive's memory model separates two independent concerns into orthogonal axes:

**Axis 1: Cleanup Responsibility** (§5.2.5 [decl.variable.operator]) — Determined by assignment operator (`=` vs `<-`)

**Axis 2: Permissions** (§11.4 [memory.permission]) — Access control qualifiers (`const`, `unique`, `shared`)

[5] These axes are completely orthogonal and may be combined freely. Complete binding operator semantics, including invalidation rules for non-responsible bindings, are specified in §5.2.5 [decl.variable.operator]. Complete permission semantics and the permission lattice are specified in §11.4 [memory.permission].

#### §10.1.3 No Garbage Collection

[7] Cursive does not employ garbage collection (§1.8.1).

#### §10.1.4 No Borrow Checker

[10] Cursive achieves memory safety without borrow checking (§1.8.2).


#### §10.1.6 Zero Runtime Overhead

[16] All memory safety mechanisms operate at compile time. The memory model guarantees:

- **No runtime checks**: Permission violations detected at compile time
- **No metadata**: Objects carry no runtime permission or responsibility tags
- **No reference counting**: Cleanup responsibility is static, not dynamic
- **No GC metadata**: No garbage collector infrastructure
- **Equivalent assembly**: Safe code compiles to identical assembly as unsafe code performing the same operations

[17] Field-level partitioning, unique enforcement, region escape analysis, and move tracking are purely compile-time analyses with zero runtime representation.

#### §10.1.7 Non-Responsible Binding Tracking Without Runtime Cost

[21] A key innovation in Cursive's memory model is tracking object lifetimes for non-responsible bindings **without runtime overhead**. The mechanism:

1. **Compile-time dependency tracking**: Non-responsible bindings (`<-`) recorded as dependent on their source bindings
2. **Parameter responsibility signal**: Procedure parameters declare whether they will destroy objects (`move` modifier)
3. **Invalidation propagation**: Move to responsible parameter → invalidate all non-responsible bindings to that object
4. **Validity preservation**: Pass to non-responsible parameter → non-responsible bindings remain valid

This provides the **safety of garbage collection** (no dangling pointers) with the **cost of manual management** (zero runtime overhead) by leveraging information already present in procedure signatures.

**Comparison to other approaches**:

| Approach                          | Safety | Cost       | How Achieved                 |
| --------------------------------- | ------ | ---------- | ---------------------------- |
| Garbage Collection                | ✅     | Runtime GC | Runtime pointer tracking     |
| Rust Borrow Checker               | ✅     | Zero       | Compile-time lifetime params |
| **Cursive Non-Responsible Bindings** | ✅     | **Zero**   | **Parameter responsibility** |

Cursive achieves safe non-responsible bindings through explicit parameter annotations rather than lifetime parameters or runtime tracking.

#### §10.1.8 Conformance Requirements

[22] Implementations shall guarantee zero runtime overhead for all safety mechanisms and reject programs with memory safety violations at compile time.

---

**Previous**: Clause 9 — Statements and Control Flow (§9.4 [stmt.order]) | **Next**: §11.2 Objects and Storage Duration (§11.2 [memory.object])
