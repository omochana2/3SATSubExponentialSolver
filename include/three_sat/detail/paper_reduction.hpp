#pragma once
#include "three_sat/detail/paper_counter.hpp"
#include <functional>

namespace three_sat::detail {
struct OccurrenceReduction {
    Cnf formula;
    // One-based copy IDs for each original variable, including unused variables.
    std::vector<std::vector<int>> copies;
};
// Section 2.8: one copy per occurrence, with a directed implication cycle forcing
// equality. This is a bijection on models, not just an equisatisfiable reduction.
OccurrenceReduction reduce_occurrences(const Cnf& formula);
// Identity when already bounded by three; otherwise use the same bijective copy reduction.
OccurrenceReduction ensure_bounded_occurrences(const Cnf& formula);

struct SparsificationParameters {
    std::size_t threshold;
    Integer depth_coefficient, occurrence_bound;
    Rational target_exponent;
};
// Exact Appendix A.3 search for k=3. Uses sigma/2 internally so the displayed
// 2*sigma exponent in that appendix is at most the caller's target_exponent.
SparsificationParameters select_sparsification_parameters(Rational target_exponent);

// Appendix A's disjoint leaves streamed depth-first. M is explicit; this routine
// does not claim a sigma exponent without a corresponding parameter selection.
void visit_sparse_leaves(const Cnf& formula, std::size_t threshold,
                         const std::function<void(const Cnf&)>& visitor);

// Optional general-input route: disjoint sparsification -> bounded occurrences ->
// parameterized design -> exact algebraic count. No leaf list or truth table is stored.
Integer count_reduced_paper_pipeline(const Cnf& formula, std::size_t threshold,
                                     const DesignParameters& parameters,
                                     PaperCounterStats* stats = nullptr);
} // namespace three_sat::detail
