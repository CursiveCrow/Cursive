# Cursive Language Specification

## Clause 10 — Memory Model, Regions, and Permissions

**Part**: X — Memory Model, Regions, and Permissions
**File**: 11-6_Layout-and-Alignment.md  
**Section**: §10.6 Layout and Alignment  
**Stable label**: [memory.layout]  
**Forward references**: Clause 5 §5.5 [decl.type], Clause 7 §7.3 [type.composite], Clause 15 [interop]

---

### §10.6 Layout and Alignment [memory.layout]

#### §10.6.1 Overview

[1] This section provides normative rules for calculating sizes, alignments, field offsets, and padding for all type categories.

[2] Layout determines ABI compatibility (Clause 15), memory efficiency, and platform-specific representations. Alignment ensures correct hardware access and prevents undefined behavior on strict-alignment platforms.

#### §10.6.2 Alignment Rules [memory.layout.alignment]

##### §10.6.2.1 Alignment Definition

[3] The alignment of type `T`, denoted $\text{align}(T)$, is a power of two indicating that objects of type `T` must be placed at addresses divisible by that value.

$$
\text{align}(T) = 2^k \quad \text{where } k \in \mathbb{N} \cup \{0\}
$$

[4] Violating alignment produces undefined behavior [UB-ID: B.2.10] on platforms with strict alignment.

##### §10.6.2.2 Alignment by Type

[5] Table 11.6 specifies alignment for each type category:

**Table 11.6 — Alignment by type**

| Type Category       | Alignment Rule     | Example                        |
| ------------------- | ------------------ | ------------------------------ |
| `i8`, `u8`, `bool`  | 1 byte             | `align(i8) = 1`                |
| `i16`, `u16`        | 2 bytes            | `align(i16) = 2`               |
| `i32`, `u32`, `f32` | 4 bytes            | `align(i32) = 4`               |
| `i64`, `u64`, `f64` | 8 bytes            | `align(f64) = 8`               |
| `i128`, `u128`      | 8 or 16 bytes\*    | Implementation-defined         |
| `char`, `()`        | 1 byte             | Implementation-defined ≥ 1     |
| Pointers            | Word size          | `align(Ptr<T>) = align(usize)` |
| Tuples              | max(align(fields)) | `align((i8, i32)) = 4`         |
| Records             | max(align(fields)) | Based on field alignments      |
| Arrays              | align(element)     | `align([T; n]) = align(T)`     |

\*Implementations shall document whether `i128`/`u128` use 8 or 16-byte alignment.

##### §10.6.2.3 Explicit Alignment

[6] The `[[repr(align(N))]]` attribute forces minimum alignment:

$$
N \geq \text{align}_{\text{natural}}(T)
$$

[7] Violating this constraint (specifying alignment smaller than natural) produces diagnostic E11-601.

**Example 11.6.2.1 (Explicit alignment):**

```cursive
[[repr(align(64))]]
record CacheLine {
    data: [u8; 64],
}
// CacheLine objects guaranteed 64-byte alignment (cache-line aligned)
```

#### §10.6.3 Size and Layout [memory.layout.size]

##### §10.6.3.1 Size Calculation

[8] The size of type `T`, denoted $\text{size}(T)$, includes all fields plus padding to maintain alignment for arrays.

[9] For records, size is calculated:

$$
\text{size}(R) = \text{last\_field\_end} + \text{tail\_padding}
$$

where:

$$
\text{tail\_padding} = (-\text{last\_field\_end}) \bmod \text{align}(R)
$$

[10] This ensures $\text{size}(T)$ is always a multiple of $\text{align}(T)$.

##### §10.6.3.2 Field Layout

[11] Record fields are laid out in declaration order with padding inserted to satisfy alignment:

[ Given: Record `R { f₁: T₁, ..., fₙ: Tₙ }` ]

$$
\text{offset}(R, f_i) \equiv 0 \pmod{\text{align}(T_i)}
$$

[12] Each field offset is the minimum value satisfying the field's alignment constraint and following all previous fields.

**Example 11.6.3.1 (Field layout with padding):**

```cursive
record Example {
    a: u8,      // Offset 0, size 1
    // 3 bytes padding
    b: i32,     // Offset 4, size 4
    c: u8,      // Offset 8, size 1
    // 3 bytes tail padding
}
// size(Example) = 12, align(Example) = 4
```

#### §10.6.4 Representation Attributes [memory.layout.repr]

##### §10.6.4.1 Overview

[13] Representation attributes control memory layout. They modify size, alignment, and field ordering without changing type semantics.

##### §10.6.4.2 Packed Layout

[14] `[[repr(packed)]]` removes all padding, setting alignment to 1:

$$
\text{align}(\text{packed } R) = 1
$$

[15] Packed layouts may cause performance penalties on platforms preferring aligned access.

**Example 11.6.4.1 (Packed layout):**

```cursive
[[repr(packed)]]
record Packed {
    a: u8,      // Offset 0
    b: i32,     // Offset 1 (no padding!)
    c: u8,      // Offset 5
}
// size(Packed) = 6, align(Packed) = 1
```

##### §10.6.4.3 C-Compatible Layout

[16] `[[repr(C)]]` uses C-compatible field layout for FFI (Clause 15):

- Fields in declaration order
- Platform-specific padding rules (C ABI)
- Ensures interoperability with C code

##### §10.6.4.4 Transparent Layout

[17] `[[repr(transparent)]]` adopts the layout of a single non-zero-sized field. Valid only for single-field wrappers.

#### §10.6.5 Diagnostics

[18] Layout diagnostics:

| Code    | Condition                                         |
| ------- | ------------------------------------------------- |
| E11-601 | Explicit alignment smaller than natural alignment |
| E11-602 | Invalid repr attribute combination                |
| E11-603 | Transparent applied to multi-field type           |

#### §10.6.6 Conformance Requirements

[19] Implementations shall:

1. Calculate alignment as power of two per §11.6.2
2. Compute field offsets satisfying alignment constraints
3. Insert padding between fields and at struct end
4. Support repr attributes (packed, align, C, transparent)
5. Document platform-specific alignment for i128/u128
6. Provide sizeof and alignof compile-time queries
7. Ensure arrays maintain element alignment

---

**Previous**: §11.5 Move/Copy/Clone Semantics (§11.5 [memory.move]) | **Next**: §11.7 Aliasing and Uniqueness (§11.7 [memory.aliasing])
