# Fixed input/output cases

Each case is a pair of UTF-8 text files with the same relative path:

```text
in/basic/exclude_both_extremes.txt
out/basic/exclude_both_extremes.txt
```

## Input

```text
p 3sat 3 2
1 2 3
-1 -2 -3
```

`p 3sat N M` declares N variables and M clauses. Exactly M triples follow. Positive IDs mean a variable, negative IDs its negation. IDs must be in [1, N], and repeated literals are allowed. Whitespace and # comments are supported. The reader consumes the entire stream and rejects extra tokens. This is a project-specific format, not DIMACS: do not add clause-terminating zeros.

## Expected output

```text
count 6
```

`count` is the fixed expected model count. No expected witness is stored. CLI tests validate the returned assignment directly against the input formula, or require `witness none` when the fixed count is zero.

For UNSAT:

```text
count 0
```

For the zero-variable empty formula:

```text
count 1
```

Output files contain only the count line, with a final newline and no comments. LF and CRLF are both accepted; other whitespace differences are significant. Counts must fit in uint64_t.

## Add and run cases

1. Add `in/hard/my_case.txt` with the formula.
2. Add `out/hard/my_case.txt` with an independently established expected count.
3. Run `scripts/test.ps1` from the project root.

No C++ or CMake edits are needed. Both directory trees are searched recursively. Missing counterparts or an empty case set cause CMake configuration to fail. Avoid semicolons in filenames. Newly added files are discovered on the next configure/build, not by running CTest alone.

Every pair has two CTest entries. `SolverCase/<path>` runs five GoogleTest variants: Naive and Subexponential through the common base interface, plus three direct algebraic configurations (default, quadratic three-point, and balanced three-group). Direct variants bypass solver preprocessing and verify that interpolation actually runs. `CLI/<path>` runs the actual executable using the input file as stdin, compares the count line with the answer file, and uses `cli_case_tests` to verify the returned witness on the original formula. This check evaluates one assignment; it does not solve the formula again. No test generates expected counts using another algorithm.

```powershell
ctest --test-dir build/cmake -L solver_cases --output-on-failure
ctest --test-dir build/cmake -R 'CLI/basic/exclusive_or' --output-on-failure
```

All pairs run by default. Both naive counting and the fixed-design algebraic backend can be slow for large inputs; the direct backend intentionally does not benefit from solver preprocessing. Each case has a 65-second CTest timeout; the CLI subprocess has a 60-second timeout. Adjust these in `CMakeLists.txt` and `tests/run_cli_case.cmake` when intentionally running slower cases.

