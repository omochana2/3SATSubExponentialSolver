# Algebraic counting pipeline

Source: [The Exponential Time Hypothesis is False, October 25 revision](https://github.com/openai/math/blob/main/preprints/The-Exponential-Time-Hypothesis-is-False-October-25-2026/paper.pdf).

The Sections 2.3-2.5 engine now accepts arbitrary-degree replacement polynomials,
arbitrary distinct nonzero integer evaluation points, and any positive number
of groups. A general dense Section 4 tensor constructor, balanced schedule
construction, and the integrated Section 2.8 reduction route are available.
This does not establish the paper's subexponential runtime for this program.

## Implementation map

| Stage | Implementation | Status |
| --- | --- | --- |
| Polynomial evaluation and Taylor translation | paper_polynomial.cpp | Arbitrary degree; truncation after translation |
| Group sums, inclusion-exclusion, interpolation | paper_counter.cpp | Parameterized by a supplied design |
| Design invariants and row-space decomposition | paper_design_validation.cpp | Exact checks and elimination |
| Remark 4.4 reference construction | paper_design.cpp | Independent small constructor retained |
| General tensor construction (4.7)-(4.10) | paper_tensor_design.cpp | General r,Q,R and interval, within explicit resource budget |
| Conditional-expectation schedules | paper_schedule.cpp | Exact deterministic candidate selection |
| Lemma 4.8 parameter selection | paper_schedule.cpp | Exact candidates and rejection reasons; current input sizes fail early hypotheses |
| Appendix A sparsification | paper_reduction.cpp and cnf.cpp | Explicit M or exponent-based M selection; streamed leaves |
| Bounded-occurrence conversion | paper_reduction.cpp | Model-bijective copies and implication cycles |
| Default end-to-end orchestration | paper_pipeline.cpp | Sparsification, occurrence bound, parameter checks, balanced construction, counting |

## Default execution path

1. The reader parses stdin; main constructs ThreeSatSubexponentialSolver.
2. The solver normalizes, propagates units, accounts for unused variables, factors
   disconnected components, and compacts active variable IDs.
3. Each remaining component calls count_paper_pipeline. This streams disjoint
   sparse leaves, ensures each variable occurs at most three times, checks the
   parameters on the reduced formula, and builds a design with balanced schedules.
4. The engine computes group sums in Q[z]/(z^J), recovers Taylor jets by (2.9),
   and uses Hermite interpolation to recover H(0). Sparse leaf counts are added.
5. The result must be a nonnegative integer. Outer branches add disjoint counts,
   independent components multiply counts, and the public uint64_t range is checked.
6. Shared witness reconstruction invokes the same counter on restricted formulas.

Trivial true/false or forced formulas may finish during preprocessing. No
nontrivial leaf is handed to the naive counter or an ordinary variable split.
The general counter knows nothing about the tensor construction or its labels.

The default orchestrator computes an exact rational sigma of order
(log n)^(-1/6), and the corresponding theoretical sparsification threshold.
When the asymptotic parameter checks fail, it explicitly chooses the verification
regime M=2, r=Q=R=2 and I={0,1}. These are small-input settings, not a claim that
the theoretical threshold or Lemma 4.8 conditions have been met. Parameters are
rechecked after the occurrence stage because it can change the problem size.
Balanced schedules are enabled in either regime.

PaperPipelineStats records the threshold, regime, rejected hypotheses, sparse
leaves, occurrence checks, actual copy reductions, balanced designs and algebraic
stage counts. Unit tests verify this sequence instead of inferring it solely from
the final answer. Trivial preprocessing exits do not run the algebraic stages.

## General replacement and measurement interface

Each occurrence has one ordinary polynomial per input variable. At a design point,
its representation consists of local coefficients and linear combinations of
shared integer measurement rows. Validation checks dimensions, distinct points,
P(0)=e_v, partitions, and the full coefficient identity at every point.

complete_design_point solves off-group row-space systems exactly. It retains an
independent subset of the original integer rows, removing zero/dependent rows
without introducing rational measurements. Groups can have different numbers of
rows, including zero. Empty groups are supported.

The counter enumerates measurement tuples in row-specific integer ranges and
assignments within each group. Corrections use Delta P(z) applied to 1_U,
separately from the summation assignment x restricted by x_E=1. Equation (2.9)
combines all U subset E with |E|<J. The degree bound sums the maximum coordinate
degree of each occurrence; J=floor(degree/Q)+1 ensures QJ>degree. Polynomials are
translated before truncation, retaining contributions from their high powers.

Hermite rows contain binom(k,j)*t^(k-j). Rational Gaussian elimination checks extra
equations for consistency. GMP provides arbitrary-precision integers/rationals.
Enumeration and interpolation remain deliberately simple.

## General tensor construction

Labels (a,d) range over ([r]^Q)^2, with D=r^(2Q). The constructor tensors the
orthogonal one-site diagonal, symmetric and antisymmetric eigenvectors. A tensor
with k antisymmetric factors has multiplier (-1)^k f_I(k) in M. Tail vectors with
k outside I become shared measurement rows. Weighted tail projectors construct
I-M exactly, and equation (4.10) constructs the polynomial rows.

Occurrences occupy separate positions within auxiliary copies. Empty positions
are zero. Positions mapped to the same original variable are combined, never
counted as independent Boolean variables. Projected rows are restricted to each
group before exact row-space decomposition.

The dense constructor defaults to a maximum of 256 auxiliary coordinates.
Exceeding this explicit resource budget throws std::length_error; it does not
silently select other parameters. The budget is separate from theorem hypotheses.
The specialized Remark 4.4 constructor remains available for comparison tests.

## Balanced schedules and asymptotic checks

With balanced_schedules=true, construction uses Lemma 4.5's potential
Phi/[M(b+w)]+Xi/[Qn]. It enumerates candidate schedules and minimizes their exact
conditional expectations, fixing variables and then factors. Shared-variable and
shared-factor dependencies are accounted for. Every step checks nonincrease.

The schedule result reports initial/final potential, actual largest label load
and group size, and whether the balance hypotheses hold. When they hold, their
promised bounds are checked too. A small configuration can be a valid exact design
without satisfying the asymptotic balance hypotheses.

select_asymptotic_parameters implements Lemma 4.8's integer formulas and reports
failed hypotheses. The API bounds variable IDs by INT_MAX; lambda=ceil(log2(n))
is too small for that lemma's r,Q,R regime. No current input receives an admissible
asymptotic selection. Later balance/tail checks are implemented, but the successful
asymptotic path cannot be exercised within this input-size domain.

## Reduction stage and explicit lower-level entry point

visit_sparse_leaves streams disjoint Appendix A leaves depth-first.
select_sparsification_parameters chooses a power-of-two M exactly. For k=3,
Gamma=16 and C=4(1+3M). It uses half the caller's target exponent internally,
accounting for the appendix's final 2*sigma exponent. It reports the occurrence
bound 2(1+M+M^2).

reduce_occurrences replaces each occurrence by a copy and connects copies with a
directed implication cycle. Every active copy occurs at most three times. Unused
original variables retain one unused copy. Models correspond bijectively, so
there is no additional multiplicative factor.

The default orchestrator uses ensure_bounded_occurrences: it returns the normalized
formula and an identity copy map when all variables already occur at most three
times, and otherwise invokes reduce_occurrences. Both branches establish the same
postcondition and preserve the model count.

count_reduced_paper_pipeline remains a lower-level entry point with an explicitly
chosen threshold and design parameters. It always performs the copy construction;
the default count_paper_pipeline instead selects the regime and avoids copies when
the occurrence bound already holds. Copying can make small instances much more
expensive for exhaustive inner loops.

```cpp
#include "three_sat/detail/paper_reduction.hpp"
using namespace three_sat::detail;

Cnf formula{2, {{1, 2}, {-1, -2}}}; // XOR: two models
DesignParameters parameters;
parameters.balanced_schedules = true;
const auto sparse = select_sparsification_parameters(Rational(1, 2));
const Integer total = count_reduced_paper_pipeline(formula, sparse.threshold, parameters);
```

To supply a general design without the reductions:

```cpp
DesignParameters parameters;
parameters.point_count = 3;
parameters.interval_length = 3; // quadratic replacement forms
const auto design = build_paper_design(formula, parameters);
const Integer total = count_with_paper_pipeline(formula, design);
```

## Verification and remaining work

The fixed-design end-to-end milestone remains complete. All 22 fixed cases now run
in five modes: Naive, the public Subexponential solver, the default algebraic
backend, a quadratic three-point backend, and a balanced three-group backend.
The actual CLI's counts and returned assignments are also validated. Expected
counts remain fixed files; no reference algorithm generates them.

Dedicated tests cover the Remark 4.4 polynomial, exact known Taylor jets, cubic
forms at nonconsecutive/negative points, quadratic tensor designs, shifted spectral
intervals, multiple copies, UNSAT cancellation, malformed designs, conditional
expectations, balance hypotheses, exponent selection and the reduction route.

Still outstanding for a claim of the paper's asymptotic runtime:

- A representation capable of the required asymptotic sizes, plus validation of
  the successful asymptotic tail/balance regime. The current default route selects
  the explicit small-input regime because the numerical hypotheses fail.
- A bit-complexity and space audit of the actual data structures and arithmetic.
- Construction/enumeration resource management and practical optimization of dense
  matrices, repeated evaluations and interpolation.

These are executable correctness baselines. They can be slower than naive counting;
no subexponential runtime claim is made.
