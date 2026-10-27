#pragma once
#include "three_sat/detail/paper_design.hpp"
#include <cstdint>

namespace three_sat::detail {
// Optional diagnostics make the algebraic path observable without changing the public
// solver API. They count work, not SAT models, and are accumulated across calls.
struct PaperCounterStats {
    std::uint64_t restricted_sums = 0;
    std::uint64_t measurement_tuples = 0;
    std::uint64_t local_assignments = 0;
    std::uint64_t interpolation_calls = 0;
    std::uint64_t nonconstant_forms = 0;
};

// Equation (2.9), computed using group sums (2.6) in Q[z]/(z^J).
TaylorPolynomial recover_taylor(const Cnf& formula, const PaperDesign& design,
                                int point, std::size_t precision,
                                PaperCounterStats* stats = nullptr);

// Fully algebraic leaf backend. There is no assignment-counting or DPLL fallback.
// The default uses small parameters in the general tensor builder. Exhaustive local
// sums make this a correctness implementation, not a subexponential-time guarantee.
Integer count_with_paper_pipeline(const Cnf& formula, PaperCounterStats* stats = nullptr);
// Use the caller's design and factor order, with arbitrary degrees, points and groups.
Integer count_with_paper_pipeline(const Cnf& formula, const PaperDesign& design,
                                  PaperCounterStats* stats = nullptr);
} // namespace three_sat::detail
