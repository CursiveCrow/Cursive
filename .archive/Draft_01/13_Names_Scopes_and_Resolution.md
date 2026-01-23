## 13. Names, Scopes, and Resolution [names]

This chapter defines the rules for name resolution in Cursive. It specifies how identifiers are bound to entities within nested scopes and the algorithms used to look up these bindings.

### 13.1 Namespaces [names.namespaces]

Cursive organizes names into several distinct, non-interfering namespaces. This allows, for example, a type and a procedure to share the same identifier without conflict.

> An identifier's meaning is determined by both its spelling and the namespace it occupies. The primary namespaces are:
>
> 1.  **The Term Namespace**: Contains bindings for variables, constants, and procedures.
> 2.  **The Type Namespace**: Contains bindings for type declarations, such as records, enums, and modals.
> 3.  **The Module Namespace**: Contains bindings for modules, introduced via the filesystem structure and `import` declarations.
>
> The syntactic context in which an identifier appears determines which namespace is searched. For example, in a type annotation (`let x: T`), `T` is resolved in the type namespace. In a procedure call (`f()`), `f` is resolved in the term namespace.

### 13.2 Scopes [names.scopes]

A scope is a region of source text in which a name is valid. Scopes are lexically nested, and inner scopes have access to names declared in their containing outer scopes.

> The following scope kinds are defined, from outermost to innermost:
>
> 1.  **Universe Scope**: A conceptual outermost scope that contains all built-in types (e.g., `i32`, `bool`, `string`) and intrinsic procedures.
> 2.  **Module Scope**: Encompasses all source files within a single module. It contains all top-level declarations and `import` bindings from that module.
> 3.  **Procedure Scope**: Contains the parameters of a procedure.
> 4.  **Block Scope**: A scope created by a block of statements enclosed in braces `{...}`. It contains local `let` bindings.

### 13.3 Name Introduction and Shadowing [names.shadowing]

A name is introduced into a scope by a declaration.

> A declaration binds an identifier to an entity within the current scope and the appropriate namespace.
>
> It is a compile-time error to declare an identifier that is already bound within the **same scope and same namespace**.
>
> > **_Diagnostic:_**
> > An attempt to declare a name that is already declared in the current scope and namespace **MUST** trigger diagnostic `E-NAME-0102`.

#### 13.3.1 Shadowing

A declaration in an inner scope may use the same identifier as a declaration in an outer scope. This is called **shadowing**.

> When an identifier in an inner scope shares the same name and namespace as an identifier in an outer scope, the inner declaration **shadows** the outer one. Within the inner scope, any unqualified use of that identifier refers to the entity declared in the inner scope. The outer entity is inaccessible via unqualified lookup within the inner scope.

**_Example:_**
```cursive
let x: i32 = 10; // Outer 'x' in module scope

procedure test() {
    let y: i32 = 20; // 'y' in procedure scope
    
    if y > x { // 'x' resolves to the module-scope 'x'
        let x: string = "shadow"; // Inner 'x' shadows the outer 'x'
        print(x); // Prints "shadow"
    }
    
    print(x); // Prints 10, as the inner 'x' is out of scope
    
    let y: bool = false; // ERROR: 'y' is already declared in this scope.
                         // Triggers E-NAME-0102
}
```

### 13.4 Name Lookup (Resolution) [names.lookup]

Name lookup is the process of finding the entity to which an identifier refers. The process differs for unqualified and qualified names.

#### 13.4.1 Unqualified Name Lookup

> For an unqualified identifier, name lookup **MUST** proceed as follows:
>
> 1.  Search for a binding with the given name in the appropriate namespace within the current, innermost scope.
> 2.  If not found, repeat the search in the immediate parent scope.
> 3.  Continue this process, moving outwards through each lexical scope up to the module scope.
> 4.  If not found in the module scope, search the universe scope.
> 5.  If the name is not found after checking all scopes, the lookup fails.
>
> > **_Diagnostic:_**
> > A failed unqualified name lookup **MUST** trigger diagnostic `E-NAME-0101`.

#### 13.4.2 Qualified Name Lookup

> For a qualified identifier of the form `A::B::...::X`, name lookup **MUST** proceed as follows:
>
> 1.  The first component (`A`) **MUST** be resolved using unqualified name lookup in the module namespace. It must resolve to a module or an alias for one. If it fails, the entire lookup fails.
> 2.  Each subsequent component (`B`, ..., `X`) **MUST** be resolved as a `public` member of the entity resolved by the preceding component. The namespace for the lookup (term or type) is determined by the context of the entire qualified name.
> 3.  If any component in the path cannot be resolved, or if it resolves to an item that is not `public`, the lookup fails.
>
> > **_Diagnostics:_**
> > *   If the first component does not resolve to a module, `E-NAME-0104` **MUST** be issued.
> > *   If a subsequent component does not resolve to a public member of the preceding entity, `E-NAME-0105` **MUST** be issued.

### 13.5 Diagnostics Summary [names.diagnostics]

This chapter introduces the following diagnostics in the `NAME` category.

| Code       | Severity | Description                                                              |
| :--------- | :------- | :----------------------------------------------------------------------- |
| E-NAME-0101 | Error    | Unresolved name: cannot find identifier in any accessible scope.         |
| E-NAME-0102 | Error    | Duplicate name: the identifier is already declared in this scope.        |
| E-NAME-0104 | Error    | Unresolved module: the first part of a qualified path is not a module.   |
| E-NAME-0105 | Error    | Unresolved or private item in path: an item in a qualified path is not found or is not public. |
