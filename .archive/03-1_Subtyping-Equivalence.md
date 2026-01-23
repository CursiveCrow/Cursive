# Cursive Language Specification
## Clause 7 — Types

**Part**: VII — Type System  
**File**: 07-1_Subtyping-Equivalence.md  
**Section**: 7.7 Type Relations *(deprecated file name; see §7.7)*  

**File**: `03-1_Subtyping-Equivalence.md`
**Version**: 2.0
**Status**: Normative
**Previous**: [07-1_Type-Foundations.md](07-1_Type-Foundations.md) | **Next**: [07-2_Primitive-Types.md](07-2_Primitive-Types.md)

---

## 3.1.1 Overview [subtyping.overview]

[1] This section specifies the subtyping relation and type equivalence rules for the Cursive type system.

**Definition 3.1.1** (*Subtyping and Equivalence Relations*):
The subtyping relation `τ <: υ` and type equivalence `τ ≡ υ` are binary relations on types where:
- Equivalence is reflexive, symmetric, and transitive
- Subtyping is reflexive and transitive (preorder)
- Equivalence implies subtyping: if `τ ≡ υ` then `τ <: υ`

[2] The subtyping relation determines when one type can safely substitute for another, while type equivalence determines when two type expressions denote the same type.

## 3.1.2 Syntax [subtyping.syntax]

[3] The subtyping relation shall be denoted `τ <: υ` (read "τ is a subtype of υ").

[4] Type equivalence shall be denoted `τ ≡ υ` (read "τ is equivalent to υ").

**Syntax:**
```
subtyping_judgment
    : type_expression '<:' type_expression
    ;

equivalence_judgment
    : type_expression '≡' type_expression
    ;
```

[5] A subtyping or equivalence judgment shall be well-formed if and only if:
    (5.1) both type expressions are well-formed types in the current type environment Γ, and
    (5.2) the types are of compatible kinds.

**Formal Well-Formedness Rule:**

$$
{\small
\dfrac{\Gamma \vdash \tau : \text{Type} \quad \Gamma \vdash \upsilon : \text{Type}}{\Gamma \vdash (\tau <: \upsilon) \text{ wf}}
}
\tag{WF-Subtype}
$$

[Cross-reference to complete grammar in Annex A §A.2]

---

## 3.1.3 Type Equivalence [subtyping.equivalence]

[6] Type equivalence shall be an equivalence relation (reflexive, symmetric, and transitive).

**Definition 3.1.2** (*Type Equivalence*):
Types τ₁ and τ₂ are equivalent (`τ₁ ≡ τ₂`) when they denote the same type after expanding aliases, substituting generic arguments, and normalizing associated types.

[7] The basic equivalence rules shall be:

**Equivalence Rules:**

$$
{\small
\dfrac{}{\tau \equiv \tau}
}
\tag{Equiv-Refl}
$$

$$
{\small
\dfrac{\tau_1 \equiv \tau_2}{\tau_2 \equiv \tau_1}
}
\tag{Equiv-Sym}
$$

$$
{\small
\dfrac{\tau_1 \equiv \tau_2 \quad \tau_2 \equiv \tau_3}{\tau_1 \equiv \tau_3}
}
\tag{Equiv-Trans}
$$

### 3.1.3.1 Type Alias Equivalence [subtyping.equivalence.alias]

[8] Type aliases shall be transparent. An alias is equivalent to its definition.

**Aliasing Rules:**

$$
{\small
\dfrac{\text{type } A = \tau}{A \equiv \tau}
}
\tag{Equiv-Alias}
$$

$$
{\small
\dfrac{\text{type } F\langle\alpha\rangle = \tau[\alpha]}{F\langle\upsilon\rangle \equiv \tau[\alpha \mapsto \upsilon]}
}
\tag{Equiv-Generic-Alias}
$$

**Example 1**: Type alias transparency
```cursive
type Coordinate = Point
let p: const Point = Point { x: 1.0, y: 2.0 }
let c: const Coordinate = p  // OK: Coordinate ≡ Point
```

**Example 2**: Generic type alias
```cursive
type Pair<T> = (T, T)
let p1: const Pair<i32> = (1, 2)
let p2: const (i32, i32) = p1  // OK: Pair<i32> ≡ (i32, i32)
```

### 3.1.3.2 Nominal vs Structural Equivalence [subtyping.equivalence.nominal-structural]

[9] Records, enums, modals, and predicates shall use nominal equivalence. Two types with identical structure but different names are not equivalent.

**Definition 3.1.3** (*Nominal Equivalence*):
Two nominal types are equivalent if and only if they have the same name and originate from the same type declaration.

**Example 3**: Nominal types are not structurally equivalent (invalid)
```cursive
record Point { x: f64, y: f64 }
record Vector { x: f64, y: f64 }
// Point ≢ Vector despite identical structure

let p: const Point = Point { x: 1.0, y: 2.0 }
// let v: Vector = p  // ERROR: Point ≢ Vector (nominal types)
```

[10] Tuples, arrays, slices, primitives, and function types shall use structural equivalence.

**Definition 3.1.4** (*Structural Equivalence*):
Two structural types are equivalent if and only if they have identical structure.

**Example 4**: Structural type equivalence
```cursive
let t1: const (i32, f64) = (42, 3.14)
let t2: const (i32, f64) = (100, 2.71)
// t1 and t2 have the same type: (i32, f64) ≡ (i32, f64)
```

### 3.1.3.3 Structural Equivalence Rules [subtyping.equivalence.structural-rules]

[11] Structural equivalence rules shall apply to composite types as follows:

**Structural Equivalence Rules:**

$$
{\small
\dfrac{\tau_1 \equiv \upsilon_1 \quad \ldots \quad \tau_n \equiv \upsilon_n}{(\tau_1, \ldots, \tau_n) \equiv (\upsilon_1, \ldots, \upsilon_n)}
}
\tag{Equiv-Tuple}
$$

$$
{\small
\dfrac{\tau \equiv \upsilon \quad n = m}{[\tau; n] \equiv [\upsilon; m]}
}
\tag{Equiv-Array}
$$

$$
{\small
\dfrac{\tau \equiv \upsilon}{[\tau] \equiv [\upsilon]}
}
\tag{Equiv-Slice}
$$

$$
{\small
\dfrac{\tau_1 \equiv \upsilon_1 \quad \ldots \quad \tau_n \equiv \upsilon_n \quad \tau_{\text{ret}} \equiv \upsilon_{\text{ret}} \quad \varepsilon_1 = \varepsilon_2}{(\tau_1, \ldots, \tau_n) \to \tau_{\text{ret}} ! \varepsilon_1 \equiv (\upsilon_1, \ldots, \upsilon_n) \to \upsilon_{\text{ret}} ! \varepsilon_2}
}
\tag{Equiv-Function}
$$

---

## 3.1.4 Subtyping Rules [subtyping.rules]

[12] The subtyping relation shall be a preorder (reflexive and transitive).

**Definition 3.1.5** (*Subtyping Relation*):
The subtyping relation `τ <: υ` (read "τ is a subtype of υ") indicates that a value of type τ can be used safely wherever a value of type υ is expected.

[13] No implicit coercions beyond subtyping shall be permitted.

### 3.1.4.1 Basic Subtyping Rules [subtyping.rules.basic]

[14] The basic subtyping axioms shall be:

**Subtyping Rules:**

$$
{\small
\dfrac{}{\tau <: \tau}
}
\tag{Sub-Refl}
$$

$$
{\small
\dfrac{\tau_1 <: \tau_2 \quad \tau_2 <: \tau_3}{\tau_1 <: \tau_3}
}
\tag{Sub-Trans}
$$

$$
{\small
\dfrac{}{! <: \tau}
}
\tag{Sub-Never}
$$

[15] The never type `!` shall be the bottom type, a subtype of all types.

**Example 5**: Never type subtyping
```cursive
function diverge(): ! {
    loop {
        // Infinite loop, never returns
    }
}

function example_never(): i32 {
    diverge()  // OK: ! <: i32
    // This code is unreachable but well-typed
}
```

### 3.1.4.2 Array and Slice Subtyping [subtyping.rules.array-slice]

[16] Fixed-size arrays shall coerce to slices of the same element type.

**Array-Slice Subtyping:**

$$
{\small
\dfrac{}{[\tau; n] <: [\tau]}
}
\tag{Sub-Array-Slice}
$$

**Example 6**: Array to slice subtyping
```cursive
let array: const [i32; 5] = [1, 2, 3, 4, 5]
let slice: const [i32] = array[..]  // OK: [i32; 5] <: [i32]
```

**Example 7**: Using array where slice expected
```cursive
function takes_slice(data: const [i32]) {
    loop item in data {
        // Process item
    }
}

let fixed: const [i32; 5] = [1, 2, 3, 4, 5]
takes_slice(fixed[..])  // OK: [i32; 5] <: [i32]
```

### 3.1.4.3 Function Subtyping [subtyping.rules.function]

[17] Function types shall be contravariant in parameters, covariant in return types, and covariant in grant sets.

**Definition 3.1.6** (*Function Subtyping*):
Function types exhibit the following variance properties:
- **Contravariant parameters**: A function accepting more general parameters is a subtype
- **Covariant return**: A function returning more specific results is a subtype
- **Covariant grants**: A function requiring fewer grants is a subtype (more specific)

**Function Subtyping Rule:**

$$
{\small
\dfrac{\upsilon_1 <: \tau_1 \quad \ldots \quad \upsilon_n <: \tau_n \quad \tau_{\text{ret}} <: \upsilon_{\text{ret}} \quad \varepsilon_1 \subseteq \varepsilon_2}{(\tau_1, \ldots, \tau_n) \to \tau_{\text{ret}} ! \varepsilon_1 <: (\upsilon_1, \ldots, \upsilon_n) \to \upsilon_{\text{ret}} ! \varepsilon_2}
}
\tag{Sub-Function}
$$

With variance properties:
- **Contravariant parameters**: $\upsilon_i <: \tau_i$
- **Covariant return**: $\tau_{\text{ret}} <: \upsilon_{\text{ret}}$
- **Covariant grants**: $\varepsilon_1 \subseteq \varepsilon_2$ (fewer grants = more specific)

**Example 8**: Function grant subtyping
```cursive
function pure_compute(x: i32): i32 {
    result x * 2
}

procedure io_compute(x: i32): i32
    sequent { [fs::write] |- true => true }
{
    println("Computing {}", x)
    result x * 2
}

// Type of pure_compute: (i32) -> i32 ! ∅
// Type of io_compute: (i32) -> i32 ! {fs::write}
//
// Subtyping: (i32) -> i32 ! ∅  <:  (i32) -> i32 ! {fs::write}
// Pure function (no grants) is subtype of function requiring grants
// (fewer grants = more specific)
```

---

## 3.1.5 Variance [subtyping.variance]

[18] Variance shall describe how subtyping of type parameters relates to subtyping of type constructors.

**Definition 3.1.7** (*Variance*):
A type constructor `F` is:
- **Covariant** in parameter `T` if `τ <: υ` implies `F⟨τ⟩ <: F⟨υ⟩`
- **Contravariant** in parameter `T` if `τ <: υ` implies `F⟨υ⟩ <: F⟨τ⟩`
- **Invariant** in parameter `T` if neither covariance nor contravariance holds

### 3.1.5.1 Variance Table [subtyping.variance.table]

[19] The following type constructors shall have the specified variance:

**Table 3.1.1**: Type Constructor Variance

| Type Constructor | Variance | Rationale |
|-----------------|----------|-----------|
| `[τ; n]` | Invariant in τ | Mutation requires exact type matching |
| `[τ]` | Invariant in τ | Mutation through slices requires type preservation |
| `(τ₁, ..., τₙ)` | Invariant in all τᵢ | Structural equality requires exact matching |
| Records | Invariant in all fields | Nominal types, no structural subtyping |
| Enums | Invariant in all variants | Nominal types, discriminant safety |
| Modals | Invariant in all states | State machine integrity |
| Function parameters | Contravariant | Parameter positions flip variance |
| Function returns | Covariant | Return positions preserve variance |
| Grants in functions | Covariant | Fewer grants = more specific |
| `Ptr⟨τ⟩@State` | Covariant in τ, Invariant in State | Safe pointer semantics |

> **Note**: Reference bindings (`let x: T <- value`) follow invariance for type safety but do not introduce new type constructors.

### 3.1.5.2 Variance Checking [subtyping.variance.checking]

[20] For a generic type `F⟨T⟩`, variance shall be computed as follows:

**Algorithm 3.1.1** (*Variance Computation*):
```
1. Examine all occurrences of T in the definition of F
2. For each occurrence:
   - In covariant position: contributes covariance
   - In contravariant position: contributes contravariance
   - In invariant position: forces invariance
3. Combine contributions:
   - All covariant → covariant
   - All contravariant → contravariant
   - Mixed or any invariant → invariant
```

**Example 9**: Variance in generic types
```cursive
// Function parameter contravariance
type Processor<T> = (T) -> ()

// If we had subtyping on T, then:
// Processor<SuperType> <: Processor<SubType>
// (contravariant in parameter position)

// Arrays are invariant
let int_array: const [i32; 3] = [1, 2, 3]
// Cannot treat as [i64; 3] even conceptually
```

---

## 3.1.6 Type Compatibility [subtyping.compatibility]

[21] Type τ shall be compatible with type υ in context C if either:
    (21.1) `τ <: υ` (subtyping), or
    (21.2) An explicit conversion from τ to υ is allowed in context C

**Definition 3.1.8** (*Type Compatibility*):
Type τ is compatible with type υ in context C if the conditions in paragraph [21] are satisfied.

[22] Cursive shall not perform implicit numeric promotions. All numeric conversions shall be explicit.

### 3.1.6.1 Compatibility Contexts [subtyping.compatibility.contexts]

[23] In assignment context `x = e`, the type of `e` shall be a subtype of the declared type of `x`.

[24] In function call context `f(e₁, ..., eₙ)`, each `eᵢ` shall have type that is a subtype of the corresponding parameter type.

[25] In return context `result e`, the type of `e` shall be a subtype of the function's return type.

### 3.1.6.2 Explicit Conversions [subtyping.compatibility.conversions]

[26] Type conversions shall use the `as` operator for explicit casting.

**Example 10**: Explicit numeric conversion required
```cursive
let x32: const i32 = 42
let x64: const i64 = x32 as i64  // Explicit cast required
```

**Example 11**: Implicit conversion not allowed (invalid)
```cursive
let x: const i32 = 42
// let z: i64 = x  // ERROR: no implicit conversion
```

> **Forward reference**: Complete conversion semantics in §5.8 (Type Conversions and Coercions).

---

## 3.1.7 Integration [subtyping.integration]

[27] The subtyping and equivalence rules integrate with the following language components:

**Cross-references:**
- Type formation: §3.0.2
- Type environments: §3.0.3
- Type conversions and coercions: §5.8
- Grant system: Part X (Grant System)
- Function types: §3.5
- Generic type constraints: Part VIII (Predicates and Type Constraints)
- Permission system interactions: Part IV §4.2

[28] The variance rules specified in paragraph [19] shall apply during generic type checking in Part IX (Generics and Parametric Polymorphism).

---

**Previous**: [07-1_Type-Foundations.md](07-1_Type-Foundations.md) | **Next**: [07-2_Primitive-Types.md](07-2_Primitive-Types.md)
