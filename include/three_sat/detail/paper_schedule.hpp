#pragma once
#include "three_sat/detail/paper_design.hpp"
#include <string>

namespace three_sat::detail {
struct ScheduleResult {
    std::vector<unsigned> variable_labels, factor_labels;
    Rational initial_potential, final_potential;
    std::size_t maximum_label_load = 0, maximum_group_size = 0;
    bool balance_hypotheses = false;
};
// Lemma 4.5: minimize the exact conditional expectation of Phi/[M(b+w)]+Xi/[Qn].
// Works as a deterministic heuristic even when the size hypotheses fail, but only
// claims the lemma's load bounds when balance_hypotheses is true.
ScheduleResult choose_balanced_schedules(const Cnf& formula, std::size_t groups,
                                        std::size_t points, std::size_t max_schedules = 256);

struct ParameterSelection {
    bool admissible = false;
    DesignParameters parameters;
    std::vector<std::string> failures;
};
// Exact Lemma 4.8 candidate selection for a machine-sized variable count. A failed
// size hypothesis is an explicit result, never permission to claim a runtime bound.
ParameterSelection select_asymptotic_parameters(const Cnf& formula, unsigned beta = 1);
} // namespace three_sat::detail
