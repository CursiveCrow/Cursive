# Cursive Language Specification

## Clause 11 — Contracts

**Clause**: 11 — Contracts
**File**: 12-9_Grammar-Reference.md
**Section**: §11.9 Grammar Reference
**Stable label**: [contract.grammar]  
**Forward references**: Annex A §A.7 [grammar.sequent], Annex A §A.8 [grammar.assertion], Annex A §A.9 [grammar.grant]

---

### §11.9 Grammar Reference [contract.grammar]

[1] This subsection consolidates grammar cross-references for the contract system. All grammar productions are normatively defined in Annex A.

- Sequent syntax: Annex A §A.7 [grammar.sequent]
- Grant syntax: Annex A §A.9 [grammar.grant]
- Predicate expressions: Annex A §A.8 [grammar.assertion]

[2] For sequent formation rules and semantic interpretation, see §11.2 [contract.sequent].

#### §11.9.1 Complete Example [contract.grammar.example]

**Example 11.9.1.1 (All grammar elements)**:

```cursive
// Grant declaration
public grant database_modify

// Contract with sequents
public contract Repository<T: Serializable> {
    type Storage = string@View

    procedure save(~, item: T): Self::Storage \/ Error
        [[ database_modify, alloc::heap |- true => true ]]

    procedure load(data: Self::Storage): T \/ Error
        [[ alloc::heap |- data.len() > 0 => true ]]
}

// Type with invariant implementing contract
record UserRepository: Repository<User> {
    data: [User],

    where {
        data.len() <= 10_000
    }

    type Storage = string@Managed

    procedure save(~, item: User): string@Managed \/ Error
        [[
            database_modify,
            alloc::heap
            |-
            item.id > 0
            =>
            match result {
                storage: string@Managed => storage.len() > 0,
                _: Error => true
            }
        ]]
    {
        // Implementation
    }

    procedure load(data: string@Managed): User \/ Error
        [[ alloc::heap |- data.len() > 0 => true ]]
    {
        // Implementation
    }
}

// Procedure using all features
[[verify(static)]]
procedure process_users(repo: unique UserRepository, count: usize)
    [[
        database_modify,
        alloc::heap
        |-
        count > 0,
        count <= 100
        =>
        repo.data.len() >= @old(repo.data.len())
    ]]
    where count <= 100  // Type invariant ensures this
{
    var i: usize = 0
    loop i < count where { i >= 0, i <= count } {
        let user = create_user(i)
        match repo.save(user) {
            _: string@Managed => { },
            err: Error => { panic("Save failed") }
        }
        i = i + 1
    }
}
```

---

**Previous**: §11.8 Verification Modes (§11.8 [contract.verification]) | **Next**: §11.10 Conformance and Diagnostics (§11.10 [contract.diagnostics])
