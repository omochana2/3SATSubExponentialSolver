#pragma once
#include "three_sat/detail/paper_reduction.hpp"
#include "three_sat/detail/paper_schedule.hpp"

namespace three_sat::detail {
struct PaperPipelineStats {
    PaperCounterStats algebra;
    std::size_t sparse_leaves = 0;
    std::size_t occurrence_checks = 0;
    std::size_t copy_reductions = 0;
    std::size_t balanced_designs = 0;
    std::size_t small_input_leaves = 0;
    std::size_t asymptotic_leaves = 0;
    std::size_t sparsification_threshold = 0;
    Rational sparsification_exponent;
    bool small_input_sparsification = false;
    std::vector<std::string> parameter_failures;
};

// Default Section 2.8 route used by the public solver: disjoint sparse leaves,
// occurrence bound <=3, conditional-expectation schedules, then Sections 2.3-2.5.
// Lemma 4.8 is checked explicitly. Its small-input regime uses M=2 and r=Q=R=2;
// it runs the same mathematical stages without claiming the asymptotic bounds.
// A supplied stats object is reset at entry and records the regime actually used.
Integer count_paper_pipeline(const Cnf& formula, PaperPipelineStats* stats = nullptr);
} // namespace three_sat::detail
