# 3SAT

A C++17 library and command-line solver for exact 3SAT model counting and witness reconstruction. Tests use GoogleTest 1.18.0 and fixed input/output files.

## Layout

```text
include/three_sat/      Public class declarations and contracts
  assignment.hpp       Complete truth assignments
  three_sat.hpp        Formula representation and evaluation
  three_sat_solver.hpp           Abstract ThreeSatSolver and shared witness reconstruction
  three_sat_naive_solver.hpp     Exhaustive ThreeSatNaiveSolver
  three_sat_subexponential_solver.hpp  Algebraic ThreeSatSubexponentialSolver
  detail/paper_*.hpp    Exact polynomials, replacement design, and algebraic counter
  three_sat_reader.hpp           Stream input parser (ThreeSATReader)
src/                   Matching implementations and CLI main.cpp
examples/main.cpp      Programmatic API example
tests/                 GoogleTest suites and CLI comparison script
  cases/in/            Formula inputs (*.txt)
  cases/out/           Fixed expected outputs (matching relative paths)
  cases/README.md       Case format and authoring instructions
scripts/test.ps1        Configure, build, and test
build/                 Generated files, ignored by Git
CMakeLists.txt          Library, executables, and test registration
```

## Build and test

Requires GCC or another C++17 compiler, CMake, Ninja, and GMP's C++ development files for exact rational arithmetic. With MSYS2 UCRT64, install the dependencies once:


```bash
pacman -S mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-gmp
```

From PowerShell, with the UCRT64 bin directory on PATH:

```powershell
cd D:\omochan\cpp\3SAT
.\scripts\test.ps1
```

Or run the steps individually:

```powershell
cmake -S . -B build/cmake -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/cmake
ctest --test-dir build/cmake --output-on-failure
```

The first configure downloads pinned GoogleTest 1.18.0 from GitHub; internet access is needed. GoogleTest is built locally, with no global installation. Configure with `-DBUILD_TESTING=OFF` to omit tests and the download.

## Command-line usage

`src/main.cpp` builds as `build/cmake/three_sat.exe`. It reads one formula from stdin through EOF and prints the count and one witness to stdout. In PowerShell:

```powershell
Get-Content tests/cases/in/basic/exclude_both_extremes.txt | .\build\cmake\three_sat.exe
```

Input:

```text
p 3sat 3 2
1 2 3
-1 -2 -3
```

Output:

```text
count 6
witness 0 0 1
```

The header gives the number of variables and clauses. Each clause has exactly three signed literals; clauses are AND-ed and their literals OR-ed. Repeated literals are valid. Zero and out-of-range IDs are rejected. Whitespace and # comments are allowed. This is not DIMACS: no trailing zero belongs after a clause.

UNSAT prints `count 0` and `witness none`, with exit code 0. The zero-variable empty formula prints `count 1` and a bare `witness` line. Case answer files store only the count; the CLI still displays both results. Invalid input or computation errors print an error to stderr, leave stdout empty, and return exit code 1. Interactive input requires an EOF signal because the reader checks for trailing tokens.

## Library API

```cpp
#include "three_sat/three_sat_reader.hpp"
#include "three_sat/three_sat_naive_solver.hpp"
#include <sstream>

std::istringstream input("p 3sat 3 1\n1 2 3\n");
ThreeSAT formula = ThreeSATReader{}.read(input);
ThreeSatNaiveSolver solver(formula);
auto total = solver.count();              // 7
auto witness = solver.get_satisfiable(); // optional<Assignment>
```

The reader only parses input; it does not solve the formula. Formulas can also be built directly with `ThreeSAT(n)` and `add_clause({a, b, c})`. `Assignment(n)` starts with all values false and uses 1-based variable IDs. Invalid assignment access throws `std::out_of_range`; invalid literals and mismatched assignment sizes throw `std::invalid_argument`.

The solver owns a formula snapshot and caches its count. It counts unused declared variables too. `get_satisfiable()` returns a witness or nullopt. Witnesses prefer false at each successive variable. Concurrent first calls on the same solver require external synchronization.

## Solver implementations

`ThreeSatSolver` is abstract. `ThreeSatNaiveSolver` and `ThreeSatSubexponentialSolver` inherit it and override only `count(const ThreeSAT&) const`. Constructors are inherited; the witness method is nonvirtual and shared.

The zero-argument `count()` caches the count of the constructor's formula and delegates to the virtual overload. Witness reconstruction also calls that virtual overload on each restricted formula. Thus replacing the counting algorithm changes the complete reconstruction path without a factory, cloning, or another override. Calling `count(other_formula)` directly does not change the original formula or its cached answer.

```cpp
#include "three_sat/three_sat_subexponential_solver.hpp"

ThreeSatSubexponentialSolver implementation(formula);
const ThreeSatSolver& solver = implementation;
auto total = solver.count();
auto witness = solver.get_satisfiable();
```

The CLI uses `ThreeSatSubexponentialSolver`; the programmatic example above and `examples/main.cpp` use `ThreeSatNaiveSolver`. Every input/output fixture runs against both classes through the base interface. The subexponential class runs the paper's algebraic pipeline at its leaves, with fixed small design parameters. It does not currently provide a subexponential runtime guarantee. See [paper implementation notes](docs/paper_implementation.md) for the implemented equations and remaining work.

The internal engine supports arbitrary-degree replacement polynomials, multiple evaluation points, and variable group counts. The default CLI now uses the integrated Section 2.8 route: disjoint sparsification, ensuring at most three occurrences per variable, conditional-expectation schedules, construction, and algebraic counting. A formula already within the occurrence bound passes that stage unchanged. Lemma 4.8 is checked on the actual reduced instance; since the current variable-count domain fails its hypotheses, the explicit small-input regime uses M=2 and r=Q=R=2 with balanced schedules enabled. These settings do not claim the paper's asymptotic bounds. The dense constructor also has an explicit auxiliary-coordinate budget.

## Algorithm and limits

The naive solver enumerates all assignments: O(2^n * (m+1)) time and O(n) search memory, excluding formula storage. The algebraic solver normalizes clauses, propagates units, factors independent components, and applies disjoint core splits. Remaining leaves construct replacement linear forms, enumerate shared measurements and local group assignments, recover Taylor coefficients by inclusion-exclusion, and perform exact Hermite interpolation. It does not fall back to the naive solver or variable-by-variable counting. Its fixed design and exhaustive inner loops remain exponential and can be slower than naive search.

Cached counts take O(1). Shared witness reconstruction fixes variables in order, querying whether each false branch still has a positive count. This needs at most n+1 counting calls on formulas with at most m+n clauses. It is a polynomial-time reduction to a counting oracle.

The public count uses uint64_t and throws `std::overflow_error` if the answer does not fit. The algebraic backend uses GMP arbitrary-precision rationals and integers, including signed intermediate values. The outer nonnegative sums and products track overflow without losing zero times an overflowing factor. Tests cover representable boundary counts, overflow, and an unsatisfiable component with many unused variables. This implementation is intended for small instances.

## Tests and new cases

See [case instructions](tests/cases/README.md). Add matching `tests/cases/in/hard/name.txt` and `out/hard/name.txt` files, then rebuild. No C++ edits are needed. Missing counterparts fail configuration. Expected outputs are fixed data; no reference solver runs in tests.

GoogleTest covers assignments, formulas, parsing, snapshots, polynomial translation, design identities, general tensor construction, schedule potentials, reductions, and fixed model counts. Each of the 22 fixture pairs runs in five modes: both public solvers and three direct algebraic configurations (default, quadratic three-point, balanced three-group). Direct checks bypass solver preprocessing. Witness reconstruction has separate unit tests, and CLI tests evaluate the actual returned assignment on the original formula. Expected files still store only counts. The programmatic example remains separately tested. See the implementation notes for completed milestones and remaining asymptotic work.

```powershell
ctest --test-dir build/cmake -N
ctest --test-dir build/cmake -R ThreeSATReaderTest --output-on-failure
ctest --test-dir build/cmake -L solver_cases --output-on-failure
```

For API tests, add named `TEST` cases under `tests/`. Use `EXPECT_EQ` for values, `EXPECT_THROW` for errors, and `ASSERT_TRUE` before dereferencing an optional. These checks remain active in Release builds. See the [GoogleTest primer](https://google.github.io/googletest/primer.html).

