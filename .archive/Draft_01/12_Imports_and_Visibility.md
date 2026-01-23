## 12. Imports and Visibility [module.import]

This chapter specifies the mechanisms for controlling visibility and managing namespaces across modules. It defines the unified `import` declaration, the visibility modifiers (`public`, `internal`, `private`), and the rules for re-exporting symbols from other modules.

### 12.1 The `import` Declaration [module.import.declaration]

The `import` declaration is the sole mechanism for making declarations from another module available in the current scope. Its behavior adapts based on its syntax, supporting both qualified and unqualified access.

#### 12.1.1 Syntax

> An `import` declaration **MUST** conform to one of the following syntactic forms:
>
> ```ebnf
> import_declaration ::= [ "public" ] "import" <import_clause> ";"
>
> import_clause ::= <module_path> [ "as" <identifier> ]
>                 | <qualified_path> "::" <import_specifier>
>                 | <qualified_path> "::" "{" <import_list> "}"
>
> import_specifier ::= <identifier> [ "as" <identifier> ]
>
> import_list ::= <import_specifier> ("," <import_specifier>)* ","?
> ```
>
> The `public` modifier **MUST NOT** be used with the `<module_path>` form of the `import_clause`. Doing so **MUST** trigger diagnostic `E-MOD-0206`.

### 12.2 Module Imports for Qualified Access [module.import.qualified]

This form of import makes an entire module's public API available under a namespace, but does not bring individual items into the local scope.

> An `import` declaration of the form `import <module_path> [as <alias>];` **MUST** make the target module available for **qualified access only**.
>
> 1.  If no alias is provided, public items from the imported module **MUST** be accessed using their full, qualified path (e.g., `external_module::utils::helper()`).
> 2.  If an alias is provided (e.g., `import ... as utils`), public items **MUST** be accessed using the alias as the qualifier (e.g., `utils::helper()`).
>
> The `module_path` **MUST** resolve to a module visible within the current assembly or its dependencies. An unresolved module path **MUST** be diagnosed with `E-MOD-0201`. An alias that conflicts with an existing name in the module scope **MUST** be diagnosed with `E-MOD-0203`.

**_Example:_**
```cursive
// Assuming 'crypto' is a dependency assembly
import crypto::sha256;
import crypto::aes as advanced_encryption;

procedure hash_data(data: [u8]): [u8] {
    // Qualified access using the full path
    let digest = sha256::hash(data);

    // Qualified access using the alias
    let encrypted = advanced_encryption::encrypt(data, get_key());
    
    result digest;
}
```

### 12.3 Item Imports for Unqualified Access [module.import.unqualified]

This form of import brings specific items from another module directly into the local scope, allowing them to be used without qualification.

> An `import` declaration that specifies one or more items (e.g., `import path::Item;` or `import path::{Item1, Item2};`) **MUST** bring those items into the local scope for unqualified access.
>
> **_Constraints:_**
> *   The path **MUST** resolve to a visible module, and the specified items **MUST** exist and be public within that module. An attempt to import a non-existent or non-public item **MUST** trigger diagnostic `E-MOD-0202`.
> *   An identifier brought into scope **MUST NOT** conflict with any other name in the current module scope. Such conflicts **MUST** be diagnosed with `E-MOD-0203`.
> *   Each `import_specifier` within a single `import_list` **MUST** be unique. A duplicate entry **MUST** trigger diagnostic `E-MOD-0205`.

**_Example:_**
```cursive
import crypto::sha256::hash;
import crypto::aes::{encrypt as aes_encrypt, decrypt as aes_decrypt};

procedure process_data(data: [u8]): [u8] {
    // Unqualified access is now possible
    let digest = hash(data);
    let encrypted = aes_encrypt(data, get_key());
    result digest;
}
```

### 12.4 Visibility and Re-exporting [module.visibility]

Visibility modifiers control which declarations are accessible outside of their defining module or assembly.

#### 12.4.1 Visibility Modifiers

> Every top-level declaration has a visibility level. If no modifier is specified, visibility defaults to `internal`.
>
> 1.  **`public`**: The declaration is visible to any module in any assembly that depends on it. Public items constitute the module's stable API.
> 2.  **`internal`**: (Default) The declaration is visible only to other modules within the **same assembly**. It is not accessible to external assemblies.
> 3.  **`private`**: The declaration is visible only within its **defining module** (i.e., within the same directory). It cannot be accessed from other modules, even within the same assembly.

#### 12.4.2 Re-exporting with `public import`

> An item import prefixed with the `public` modifier re-exports an item, making it part of the current module's public API.
>
> **_Semantics:_**
> The `public import` declaration brings an item into the local scope for unqualified access and simultaneously adds it to the current module's export set, as if it were declared with `public` in the current module.
>
> **_Constraint:_**
> An item can only be re-exported if it is `public` in its source module. An attempt to `public import` an `internal` or `private` item **MUST** be diagnosed with `E-MOD-0204`.

**_Example:_**
```cursive
// Assembly: "graphics"
// Module: graphics::shapes
// File: /source/graphics/shapes/mod.cursive

// This record is only visible inside the 'graphics' assembly.
internal record Circle { radius: f64 }

// This procedure is visible to any assembly.
public procedure make_square(size: f64): Square { ... }

---
// Assembly: "my_app"
// Module: my_app::drawing
// File: /source/my_app/drawing/mod.cursive

// Re-export 'make_square' as part of this module's public API.
public import graphics::shapes::make_square;

// ERROR: Cannot re-export an internal item from another assembly.
// public import graphics::shapes::Circle; // Triggers E-MOD-0204

procedure draw_shapes() {
    // 'make_square' is available locally due to the public import.
    let s = make_square(10.0);
}
```

### 12.5 Diagnostics Summary [module.diagnostics]

This chapter introduces the following diagnostics, which correspond to the `SRC-12` feature bucket in Appendix B.

| Code       | Severity | Description                                                              |
| :--------- | :------- | :----------------------------------------------------------------------- |
| E-MOD-0201 | Error    | `import` path does not resolve to a known module.                        |
| E-MOD-0202 | Error    | `import` path does not resolve to a visible item, or item is not public. |
| E-MOD-0203 | Error    | Name introduced by `import` conflicts with an existing item in scope.    |
| E-MOD-0204 | Error    | Attempt to `public import` a non-public item from another module.        |
| E-MOD-0205 | Error    | Duplicate item in an `import` list.                                      |
| E-MOD-0206 | Error    | The `public` modifier cannot be applied to a module-level `import`.      |