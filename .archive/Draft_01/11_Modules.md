# Part 3 - Module System and Name Resolution

## 11. Modules, Assemblies, and Projects [module.overview]

This chapter defines Cursive's organizational units: modules, assemblies, and projects. It specifies how these units are discovered from the file system, configured via a manifest file, and resolved into the unique module paths that form the basis of Cursive's namespace and visibility systems.

### 11.1 Core Definitions [module.definitions]

> A **Project** is the top-level organizational unit, consisting of a collection of source files and a single manifest file, `Cursive.toml`, at its root. A project defines one or more assemblies.
> 
> An **Assembly** is a collection of modules that are compiled and distributed as a single unit. An assembly can be either a `library` or an `executable`. Each assembly is defined within the project manifest.
> 
> A **Module** is the fundamental unit of code organization and encapsulation. A module's contents are defined by the `.cursive` source files within a single directory, and its namespace is identified by a unique, filesystem-derived `module path`.

### 11.2 Module Discovery and Paths [module.discovery]

Cursive's module system is directly mapped to the filesystem to ensure predictability and local reasoning. The compiler discovers modules by traversing the directory structure from a set of defined source roots.

#### 11.2.1 The Folder-as-Module Rule

> Each directory within a declared source root that contains one or more `.cursive` files **MUST** be treated as a single module. All `.cursive` files located directly within that directory contribute their top-level declarations to that single module's namespace.

**_Explanation:_**
This rule simplifies module definition by avoiding the need for explicit module declaration files (e.g., `mod.rs` or `__init__.py`). The module's boundary is implicitly the directory's boundary.

**_Example:_**
Given the file structure:
```
/source/rendering/core.cursive
/source/rendering/shaders.cursive
```
Both `core.cursive` and `shaders.cursive` belong to the same module.

#### 11.2.2 Module Path Derivation

> The `module path` for a given module **MUST** be derived from its directory path relative to the assembly's source root directory. Directory separators in the path **MUST** be replaced by the scope resolution operator `::`.

**_Formal Rule:_**
$$ 
\frac{\text{root} = \text{resolve_path}(\text{assembly.root}) \quad \text{dir} = \text{resolve_path}(\text{module_dir}) \quad \text{rel_path} = \text{relative}(\text{dir}, \text{root})}{\text{module_path} = \text{replace}(\text{rel_path}, \text{PATH_SEPARATOR}, "::")} \tag{WF-Module-Path-Derivation}
$$ 

**_Explanation:_**
The compiler first resolves the absolute paths for the assembly's source root and the module's directory. It then computes the relative path and replaces the filesystem separator (e.g., `/` or `\`) with `::` to form the canonical module path.

**_Example:_**
- Assembly Root: `/project/source`
- Module Directory: `/project/source/graphics/vulkan`
- Relative Path: `graphics/vulkan`
- Resulting Module Path: `graphics::vulkan`

### 11.3 Project Manifest (`Cursive.toml`) [module.manifest]

Every project **MUST** be defined by a UTF-8 encoded manifest file named `Cursive.toml` at its root directory. The manifest **MUST** follow the TOML 1.0 syntax.

> An implementation **MUST** issue diagnostic `E-MOD-0101` if the `Cursive.toml` manifest is not found or is syntactically malformed.

#### 11.3.1 Manifest Schema

A conforming manifest provides the necessary metadata to discover source files and configure assemblies.

**_Syntax (Informative TOML Structure):_**
```toml
[project]
name = "<identifier>"
version = "<semver>"

[language]
version = "<semver>"

[paths]
<symbolic_name> = "<relative_path>"
#... more paths

[[assembly]]
name = "<identifier>"
type = "library" | "executable" # Optional, default: "library"
root = "<symbolic_name>"
path = "<relative_path_from_root>"
#... more assemblies
```

#### 11.3.2 Normative Sections

##### `[project]` Table
> The `[project]` table **MUST** be present and **MUST** contain the following keys:
> *   `name`: A string that **MUST** be a valid Cursive identifier (§9.3).
> *   `version`: A string that **MUST** be a valid Semantic Version 2.0.0 string.
> 
> A missing or invalid `[project]` table **MUST** trigger diagnostic `E-MOD-0107`.

##### `[language]` Table
> The `[language]` table **MUST** be present and **MUST** contain the following key:
> *   `version`: A string that **MUST** be a valid semantic version. The MAJOR version **MUST** be supported by the compiler, as specified in §7.1.

##### `[paths]` Table
> The `[paths]` table **MUST** be present and **MUST** contain at least one key-value pair.
> *   Each key **MUST** be a string that serves as a symbolic name for a source root.
> *   Each value **MUST** be a string containing a valid relative path from the project root to a source directory.
> 
> A missing, empty, or invalid `[paths]` table **MUST** trigger diagnostic `E-MOD-0102`.

##### `[[assembly]]` Table
> At least one `[[assembly]]` table **MUST** be defined. Each assembly table **MUST** contain the following keys:
> *   `name`: A string that **MUST** be a unique identifier for the assembly within the project.
> *   `root`: A string that **MUST** match a key defined in the `[paths]` table.
> *   `path`: A string specifying the relative path from the directory specified by `root` to the assembly's root module directory.
> 
> The following key is optional:
> *   `type`: A string, either `"library"` (default) or `"executable"`.
> 
> An assembly referencing a `root` not defined in `[paths]` **MUST** trigger diagnostic `E-MOD-0103`. Duplicate assembly names **MUST** trigger diagnostic `E-MOD-0108`.

### 11.4 Module Path Validity [module.paths]

Module paths are not arbitrary strings; they are sequences of identifiers and are subject to the same rules.

#### 11.4.1 Path Component as Identifier
> Each component of a module path **MUST** be a valid Cursive identifier as defined in §9.3.
> 
> **_Explanation:_**
> This constraint ensures that module paths can be used directly in qualified name expressions (e.g., `my_module::my_item`).
> 
> **_Diagnostic:_**
> If a directory or file name that forms a module path component is not a valid identifier (e.g., `01-utils`), implementations **MUST** issue diagnostic `E-MOD-0106`.

#### 11.4.2 Path Component Keyword Check
> A module path component **MUST NOT** be a reserved keyword.
> 
> **_Explanation:_**
> This constraint prevents ambiguity between module paths and language constructs (e.g., `import my_project::if`).
> 
> **_Diagnostic:_**
> If a directory or file name that forms a module path component is a reserved keyword from §9.2.1, implementations **MUST** issue diagnostic `E-MOD-0105`.

#### 11.4.3 Case-Sensitivity and Collisions
> On filesystems that are not case-sensitive, two file or directory names that differ only in case **MUST** be treated as ambiguous if they would resolve to the same module path component.
> 
> **_Explanation:_**
> On Windows and default macOS filesystems, `MyModule` and `mymodule` refer to the same directory. A project containing both is ill-formed because the canonical module path is ambiguous.
> 
> **_Diagnostic:_**
> An implementation **MUST** detect and report such collisions with diagnostic `E-MOD-0104`.
> 
> To ensure cross-platform portability, implementations **SHOULD** also issue a warning (`W-MOD-0101`) if two module paths are distinct on a case-sensitive filesystem but would collide on a case-insensitive one.

### 11.5 Diagnostics Summary [module.diagnostics]

This chapter introduces the following diagnostics in the `MOD` (Module System) category, which corresponds to the `SRC-11` feature bucket in Appendix B.

| Code       | Severity | Description                                                              |
| :--------- | :------- | :----------------------------------------------------------------------- |
| E-MOD-0101 | Error    | Manifest file `Cursive.toml` not found or syntactically malformed.       |
| E-MOD-0102 | Error    | `[paths]` table in manifest is missing, empty, or invalid.               |
| E-MOD-0103 | Error    | Assembly references a `root` that is not defined in the `[paths]` table. |
| E-MOD-0104 | Error    | Module path collision detected on a case-insensitive filesystem.         |
| E-MOD-0105 | Error    | Module path component is a reserved keyword.                             |
| E-MOD-0106 | Error    | Module path component is not a valid Cursive identifier.                 |
| E-MOD-0107 | Error    | `[project]` table or its required keys (`name`, `version`) are missing.  |
| E-MOD-0108 | Error    | Duplicate assembly name found in `Cursive.toml`.                         |
| W-MOD-0101 | Warning  | Potential module path collision on case-insensitive filesystems.         |
