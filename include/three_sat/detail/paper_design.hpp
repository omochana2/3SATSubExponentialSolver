#pragma once
#include "three_sat/detail/cnf.hpp"
#include "three_sat/detail/paper_polynomial.hpp"
#include <array>

namespace three_sat::detail {
struct ReplacementForm {
    std::size_t variable; // zero-based original variable, P(0) = its coordinate vector
    std::vector<Polynomial> coordinates; // coefficient polynomial of each Boolean input
    std::size_t copy; // provenance for tensor constructions; unused by the counter
    int tail_sign; // only the specialized Remark 4.4 builder uses this diagnostic
};
struct PointForm {
    std::vector<Rational> local_coefficients;
    // One linear combination of the shared rows for each group.
    std::vector<std::vector<Rational>> measurement_weights;
};
struct DesignPoint {
    int value;
    std::size_t group_count;
    std::vector<std::size_t> variable_groups, factor_groups;
    // [group][row][variable]; rows are zero outside their own group.
    std::vector<std::vector<std::vector<int>>> measurements;
    std::vector<std::vector<PointForm>> forms;
};
struct PaperDesign {
    std::size_t variable_count;
    std::vector<unsigned> variable_labels; // schedules encoded as base-r integers
    std::vector<unsigned> factor_labels;
    std::vector<std::vector<ReplacementForm>> forms;
    // Tensor tail rows per auxiliary copy, projected onto actual Boolean variables.
    // A coordinate not occupied by an incidence is zero, not a new variable.
    std::vector<std::vector<int>> measurements;
    std::vector<DesignPoint> points;
};

// Construct the per-point row-space decompositions by exact linear algebra. The
// caller supplies partitions, factor groups and projected shared measurement rows.
void complete_design_point(const PaperDesign& design, DesignPoint& point);
// Check dimensions, P(0)=e_v and every off-group row-space identity before counting.
void validate_paper_design(const Cnf& formula, const PaperDesign& design);

// Section 4, specialized to Remark 4.4: r=Q=R=2, I={0,1}.
// Uses all 16 labels (a,d), and enough copies for every incidence. The round-robin
// schedules are deterministic; they do NOT implement the asymptotic balancing lemma.
PaperDesign build_paper_design(const Cnf& formula);

struct DesignParameters {
    std::size_t groups = 2;
    std::size_t point_count = 2;
    std::size_t interval_start = 0;
    std::size_t interval_length = 2;
    // Explicit implementation resource bound, not a mathematical admissibility condition.
    std::size_t max_auxiliary_coordinates = 256;
    bool balanced_schedules = false;
};
// Dense exact Section 4 construction, including arbitrary spectral intervals.
// It rejects a construction exceeding the explicit resource budget; it never silently
// switches to a different design or claims the asymptotic hypotheses hold.
PaperDesign build_paper_design(const Cnf& formula, const DesignParameters& parameters);
} // namespace three_sat::detail
