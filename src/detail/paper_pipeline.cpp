#include "three_sat/detail/paper_pipeline.hpp"
#include <algorithm>

namespace three_sat::detail {
namespace {
Rational sparsification_exponent(std::size_t n) {
    // For k=3, Section 2.8 uses sigma of order (log n)^(-1/6).
    // Integer ceilings give an exact rational with that order and sigma in (0,1].
    std::size_t lambda = 0;
    for (auto value = n ? n - 1 : 0; value; value >>= 1) ++lambda;
    std::size_t root = 1;
    for (;;) {
        Integer power = 1;
        for (int i = 0; i < 6; ++i) power *= Integer(std::to_string(root));
        if (power >= Integer(std::to_string(lambda))) break;
        ++root;
    }
    return Rational(1, Integer(std::to_string(root)));
}
}

Integer count_paper_pipeline(const Cnf& input, PaperPipelineStats* output) {
    PaperPipelineStats local;
    auto& stats = output ? *output : local;
    stats = PaperPipelineStats{};
    const auto formula = normalize(input);
    stats.sparsification_exponent = sparsification_exponent(formula.variable_count);
    const auto theoretical_sparse = select_sparsification_parameters(stats.sparsification_exponent);
    const auto initial = select_asymptotic_parameters(formula);
    stats.small_input_sparsification = !initial.admissible;
    stats.sparsification_threshold = initial.admissible ? theoretical_sparse.threshold : 2;
    stats.parameter_failures = initial.failures;

    Integer total = 0;
    visit_sparse_leaves(formula, stats.sparsification_threshold, [&](const Cnf& leaf) {
        ++stats.sparse_leaves;
        const auto bounded = ensure_bounded_occurrences(leaf);
        ++stats.occurrence_checks;
        if (bounded.formula.variable_count != leaf.variable_count) ++stats.copy_reductions;

        // Recheck on the actual reduced instance. Copying changes n, b and w;
        // parameters of the original input must not be reused blindly.
        const auto selection = select_asymptotic_parameters(bounded.formula);
        DesignParameters parameters;
        if (selection.admissible) {
            parameters = selection.parameters;
            ++stats.asymptotic_leaves;
        } else {
            ++stats.small_input_leaves;
            for (const auto& failure : selection.failures)
                if (std::find(stats.parameter_failures.begin(), stats.parameter_failures.end(), failure) == stats.parameter_failures.end())
                    stats.parameter_failures.push_back(failure);
        }
        parameters.balanced_schedules = true;
        const auto design = build_paper_design(bounded.formula, parameters);
        ++stats.balanced_designs;
        total += count_with_paper_pipeline(bounded.formula, design, &stats.algebra);
    });
    return total;
}
} // namespace three_sat::detail
