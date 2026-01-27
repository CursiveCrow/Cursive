# Cursive0 Spec Test Suite Runner

A comprehensive test harness for the cursivec0 compiler, inspired by Rust's compiletest and LLVM's lit test infrastructure.

## Quick Start

```bash
# Run all tests
python run_tests.py

# Run with verbose output
python run_tests.py --verbose

# Run specific test category
python run_tests.py --filter "ui/E-TYP/*"

# Run a single test
python run_tests.py --filter "**/E-TYP-1901.cursive"

# Generate coverage report
python run_tests.py --coverage

# List all tests without running
python run_tests.py --list
```

## Test File Format

### Single-File Tests

Each test is a `.cursive` file with inline directives:

```cursive
// RUN: compile-fail
// DIAG: E-TYP-1901
// SPEC: 5.2.3
// DESC: Duplicate field name in record declaration

record Point {
    x: i32
    x: i32  //~ ERROR E-TYP-1901: Duplicate field name
}

public procedure main(move ctx: Context) -> i32 {
    return 0
}
```

### Header Directives

| Directive | Description |
|-----------|-------------|
| `// RUN: compile-pass` | Test must compile without errors |
| `// RUN: compile-fail` | Test must fail compilation |
| `// RUN: run-pass` | Test must compile and run (exit 0 by default) |
| `// RUN: run-fail` | Test must compile, run, and panic |
| `// DIAG: E-XXX-NNNN` | Expected diagnostic code |
| `// DIAG-NOT: E-XXX-NNNN` | Diagnostic must NOT appear |
| `// STDOUT: text` | Expected stdout contains text |
| `// EXIT: N` | Expected exit code (for run-pass) |
| `// SPEC: X.Y.Z` | Spec section reference |
| `// DESC: description` | Human-readable test description |
| `// SKIP: reason` | Skip test with reason |
| `// XFAIL: reason` | Expected failure (known bug) |

### Inline Directives

Inline directives specify expected diagnostics at specific lines:

```cursive
let x: i32 = "hello"  //~ ERROR E-TYP-2533: type mismatch
```

Directive format:
- `//~ ERROR E-XXX-NNNN` - Error expected on this line
- `//~ WARN W-XXX-NNNN` - Warning expected on this line
- `//~ INFO I-XXX-NNNN` - Info message expected on this line
- `//~^ ERROR E-XXX-NNNN` - Error expected on previous line
- `//~v ERROR E-XXX-NNNN` - Error expected on next line
- `//~2 ERROR E-XXX-NNNN` - Error expected 2 lines below

Message matching:
- `//~ ERROR E-XXX-NNNN: substring` - Message must contain "substring"

### Multi-File Project Tests

For tests requiring project structure, create a directory with `expect.toml`:

```
ui/E-PRJ/E-PRJ-0101-missing-manifest/
├── src/
│   └── main.cursive
└── expect.toml
```

`expect.toml` format:
```toml
diag = "E-PRJ-0101"
# or multiple:
diags = ["E-PRJ-0101", "E-MOD-1301"]
```

## Directory Structure

```
tests/
├── runner/
│   ├── run_tests.py      # Main test harness
│   ├── filecheck.py      # Inline directive parser
│   ├── config.toml       # Configuration
│   └── README.md         # This file
├── ui/                   # Compile-time diagnostic tests (~388 tests)
│   ├── E-PRJ/           # Project errors (15 codes)
│   ├── E-MOD/           # Module errors (33 codes)
│   ├── E-SRC/           # Source/lexing/parsing errors (15 codes)
│   ├── E-CNF/           # Conformance errors (6 codes)
│   ├── E-UNS/           # Unsupported constructs (8 codes)
│   ├── E-MEM/           # Memory/provenance errors (14 codes)
│   ├── E-CON/           # Concurrency/keys errors (74 codes)
│   ├── E-TYP/           # Type errors (110 codes)
│   ├── E-SEM/           # Semantic errors (46 codes)
│   ├── E-OUT/           # Output/linking errors (17 codes)
│   ├── E-SYS/           # System/FFI errors (12 codes)
│   ├── W-CNF/           # Conformance warnings
│   ├── W-CON/           # Concurrency warnings (16 codes)
│   ├── W-MOD/           # Module warnings
│   ├── W-SEM/           # Semantic warnings
│   ├── W-SRC/           # Source warnings
│   ├── W-SYS/           # System warnings
│   ├── W-TYP/           # Type warnings
│   └── I-CON/           # Concurrency info messages
├── run-pass/            # Compile and execute tests (~109 tests)
│   ├── primitives/      # Integer, float, bool, char, unit
│   ├── operators/       # Arithmetic, comparison, logical, bitwise
│   ├── control-flow/    # if, match, loop, break, continue
│   ├── bindings/        # let, var, :=, shadow
│   ├── procedures/      # calls, recursion, move params
│   ├── records/         # fields, methods, patterns
│   ├── enums/           # discriminants, payloads, match
│   ├── tuples/          # construction, indexing, destructure
│   ├── arrays/          # fixed-size, indexing, patterns
│   ├── slices/          # views
│   ├── unions/          # matching, propagation
│   ├── pointers/        # Ptr, address-of, deref
│   ├── permissions/     # const, unique
│   ├── modals/          # states, transitions, widen
│   ├── strings/         # string, bytes, states
│   ├── memory/          # regions, frames, alloc
│   ├── generics/        # type parameters, bounds
│   ├── classes/         # implementation, dispatch
│   ├── capabilities/    # Context, HeapAllocator
│   ├── parallelism/     # parallel, spawn, dispatch
│   ├── keys/            # hash blocks, acquisition
│   ├── async/           # suspension, composition
│   ├── ffi/             # extern, FfiSafe
│   ├── contracts/       # pre/post conditions
│   ├── closures/        # capture, escaping
│   ├── statics/         # module-level bindings
│   ├── unsafe/          # transmute, raw pointers
│   ├── types/           # aliases
│   └── defer/           # cleanup ordering
├── run-fail/            # Runtime panic tests (~13 tests)
│   ├── P-TYP/           # Type-related panics (2 codes)
│   ├── P-SEM/           # Semantic panics (5 codes)
│   └── panic-*.cursive  # Generic panic tests
├── codegen/             # IR verification tests (~4 tests)
│   ├── layout/          # Type layout verification
│   ├── abi/             # Calling convention tests
│   └── symbols/         # Symbol mangling tests
└── auxiliary/           # Shared test libraries
    └── runtime/         # Runtime library for tests
```

## Command-Line Options

```
--filter PATTERN    Only run tests matching glob pattern
--jobs N            Run N tests in parallel (default: CPU count)
--bless             Update expected outputs (baseline mode)
--verbose           Show detailed output for each test
--format FORMAT     Output format: human, junit, tap
--compiler PATH     Path to cursivec0.exe
--timeout SECONDS   Per-test timeout (default: 60)
--coverage          Generate diagnostic coverage report
--list              List all tests without running
```

## Output Formats

### Human (default)
```
Running 150 tests with 8 parallel jobs...

..F..S..x....
```

Legend:
- `.` = pass
- `F` = fail
- `S` = skip
- `x` = expected failure (xfail)
- `X` = unexpected pass (xpass)

### JUnit XML
```bash
python run_tests.py --format junit
# Creates junit-results.xml
```

### TAP (Test Anything Protocol)
```bash
python run_tests.py --format tap
```

## Coverage Report

```bash
python run_tests.py --coverage
```

Output:
```
DIAGNOSTIC CODE COVERAGE
========================
Codes hit: 145/200 (72.5%)

Missing coverage for:
  - E-CON-0150
  - E-CON-0151
  ...
```

## Writing New Tests

### 1. Compile-Fail Test (Diagnostic Test)

Create `ui/E-TYP/E-TYP-1901.cursive`:

```cursive
// RUN: compile-fail
// DIAG: E-TYP-1901
// SPEC: 5.2.3
// DESC: Duplicate field name in record declaration

record Point {
    x: i32
    x: i32  //~ ERROR E-TYP-1901
}

public procedure main(move ctx: Context) -> i32 {
    return 0
}
```

### 2. Run-Pass Test (Execution Test)

Create `run-pass/primitives/integers.cursive`:

```cursive
// RUN: run-pass
// EXIT: 0
// SPEC: 5.2.1
// DESC: All integer types compile and have correct sizes

public procedure main(move ctx: Context) -> i32 {
    let a: i8 = -128
    let b: u8 = 255
    return 0
}
```

### 3. Run-Fail Test (Panic Test)

Create `run-fail/P-SEM/P-SEM-2850.cursive`:

```cursive
// RUN: run-fail
// DIAG: P-SEM-2850
// SPEC: 14
// DESC: Contract predicate failed at runtime

public procedure div(x: i32, y: i32) -> i32
    where y != 0
{
    return x / y
}

public procedure main(move ctx: Context) -> i32 {
    return div(10, 0)  // Runtime panic: precondition failed
}
```

## Test Naming Convention

Tests should be named after the diagnostic code they test:
- `E-TYP-1901.cursive` - Tests error code E-TYP-1901
- `E-TYP-1901-variant.cursive` - Alternative test for same code

For run-pass tests, use descriptive names:
- `primitives/integers.cursive`
- `control-flow/match-union.cursive`
- `memory/region-allocation.cursive`

## Troubleshooting

### Test fails with "Executable not found"
The compiler may have failed silently or the output path changed. Check:
```bash
python run_tests.py --verbose --filter "your/test"
```

### Test times out
Increase the timeout:
```bash
python run_tests.py --timeout 120
```

### Can't find compiler
Specify the path explicitly:
```bash
python run_tests.py --compiler /path/to/cursivec0.exe
```
