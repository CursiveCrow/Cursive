## 14. Initialization [initialization]

This chapter defines the formal rules for the initialization of modules and module-level variable bindings. It specifies the construction of the module dependency graph, the requirement for acyclicity, and the runtime semantics of initialization order and failure.

### 14.1 Module Initialization [initialization.overview]

> Module initialization is the process of evaluating the initializers for all module-level `let` and `var` bindings within a given module. This process **MUST** occur at program startup, before the program's `main` entry point is executed.
> 
> The order of initialization is determined by the dependencies between modules. A module that depends on another **MUST** be initialized after its dependency.

### 14.2 The Module Dependency Graph [initialization.graph]

The relationships between modules are represented by a Module Dependency Graph (MDG).

> The MDG is a directed graph $G = (V, E)$, where:
> 
> 1.  $V$ is the set of all modules in the program and its compiled dependencies.
> 2.  $E$ is a set of directed edges $(A, B)$, where an edge from module $A$ to module $B$ signifies that $A$ depends on $B$.

**_Formal Rule:_**
An edge $(A, B)$ exists in $E$ if and only if module $A$ contains an `import` declaration that resolves to an item or module path within module $B$.
$$
\frac{ { 
    \text{decl} \in A \quad \text{decl} = \text{`import B::...;`} 
} }{ (A, B) \in E }
\tag{WF-Dep-Edge}
$$

### 14.3 Initialization Order and Cycles [initialization.order]

The initialization order of modules is derived from the topology of the MDG.

> The initialization order of modules **MUST** be a valid topological sort of the Module Dependency Graph.
> 
> Consequently, the MDG **MUST** be a Directed Acyclic Graph (DAG). An implementation **MUST** detect cycles in the MDG.

**_Formal Rule:_**
The judgment $\vdash G: \text{DAG}$ holds if the graph $G$ is acyclic. A graph is cyclic if any node is reachable from itself.
$$
\frac{ { 
    \forall v \in V, \neg \text{is\_reachable}(v, v, E) 
} }{ 
    \vdash G: \text{DAG} 
}
\tag{WF-Acyclic-Deps}
$$
Where $\text{is\_reachable}(u, v, E)$ is true if there is a path of one or more edges from $u$ to $v$ in the graph $(V, E)$.

> **_Diagnostic:_** If the MDG is not a DAG, the implementation **MUST** reject the program and issue diagnostic `E-MOD-1401`. The diagnostic message **SHOULD** include the sequence of modules that form the cycle.

### 14.4 Initialization Semantics [initialization.semantics]

The runtime **MUST** adhere to the initialization order computed at compile time.

> Before the program's `main` procedure is invoked, the runtime **MUST**:
> 
> 1.  Take the valid topological sort of modules computed by the compiler.
> 2.  For each module in the sorted list, execute the initializers for all module-level bindings in the order they appear within that module's source files.

### 14.5 Initialization Failure [initialization.failure]

> If the evaluation of any module-level initializer terminates with a panic, the initialization of that module is considered to have failed.
> 
> If the initialization of a module $M$ fails, the program state is considered "poisoned." Any subsequent attempt at runtime to access a binding from module $M$, or from any module that transitively depends on $M$, **MUST** also result in a panic.

**_Explanation:_**
This rule ensures that initialization failures are fail-stop and do not lead to undefined behavior from using partially initialized modules. The program is guaranteed to terminate predictably rather than continue in an inconsistent state.

### 14.6 Diagnostics Summary [initialization.diagnostics]

This chapter introduces the following diagnostics in the `MOD` (Module System) category.

| Code       | Severity | Description                               |
| :--------- | :------- | :---------------------------------------- |
| E-MOD-1401 | Error    | Cyclic module dependency detected.        |