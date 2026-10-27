#include "three_sat/detail/paper_reduction.hpp"
#include <limits>
#include <algorithm>
#include <stdexcept>

namespace three_sat::detail {
SparsificationParameters select_sparsification_parameters(Rational exponent) {
    if (exponent.get_den() == 0) throw std::invalid_argument("Invalid sparsification exponent.");
    exponent.canonicalize();
    if (exponent <= 0 || exponent > 1) throw std::invalid_argument("Sparsification exponent must be in (0,1].");
    // k=3 gives Gamma=16 and C=4(1+3M).
    for (std::size_t threshold = 16;;) {
        const Integer M(std::to_string(threshold));
        const Integer C = 4 * (1 + 3 * M);
        const Integer logarithm_argument = 4 * C * M - 1;
        const Integer h(std::to_string(mpz_sizeinbase(logarithm_argument.get_mpz_t(), 2)));
        if (32 * h <= exponent * M)
            return {threshold, C, 2 * (1 + M + M * M), exponent};
        if (threshold > std::numeric_limits<std::size_t>::max() / 2)
            throw std::overflow_error("Sparsification threshold exceeds size_t.");
        threshold *= 2;
    }
}

OccurrenceReduction reduce_occurrences(const Cnf& input) {
    const auto normalized = normalize(input);
    OccurrenceReduction result{Cnf{0, {}}, std::vector<std::vector<int>>(normalized.variable_count)};
    auto new_copy = [&](std::size_t variable) {
        if (result.formula.variable_count == static_cast<std::size_t>(std::numeric_limits<int>::max()))
            throw std::overflow_error("Occurrence reduction exceeds the literal ID range.");
        const int copy = static_cast<int>(++result.formula.variable_count);
        result.copies[variable].push_back(copy);
        return copy;
    };
    for (const auto& clause : normalized.clauses) {
        Clause replaced;
        for (int literal : clause) {
            const auto variable = static_cast<std::size_t>((literal < 0 ? -literal : literal) - 1);
            const int copy = new_copy(variable);
            replaced.push_back(literal < 0 ? -copy : copy);
        }
        result.formula.clauses.push_back(std::move(replaced));
    }
    for (std::size_t v = 0; v < normalized.variable_count; ++v) {
        if (result.copies[v].empty()) new_copy(v); // An unused variable still has two choices.
        const auto& copies = result.copies[v];
        if (copies.size() > 1) for (std::size_t i = 0; i < copies.size(); ++i)
            result.formula.clauses.push_back({-copies[i], copies[(i + 1) % copies.size()]});
    }
    return result;
}

OccurrenceReduction ensure_bounded_occurrences(const Cnf& input) {
    auto formula = normalize(input);
    std::vector<unsigned> occurrences(formula.variable_count);
    for (const auto& clause : formula.clauses) for (int literal : clause) {
        const auto v = static_cast<std::size_t>((literal < 0 ? -literal : literal) - 1);
        if (++occurrences[v] > 3) return reduce_occurrences(formula);
    }
    OccurrenceReduction result{std::move(formula), {}};
    for (std::size_t v = 0; v < result.formula.variable_count; ++v)
        result.copies.push_back({static_cast<int>(v + 1)});
    return result;
}

void visit_sparse_leaves(const Cnf& formula, std::size_t threshold,
                         const std::function<void(const Cnf&)>& visitor) {
    if (threshold < 2 || !visitor) throw std::invalid_argument("Invalid sparsifier threshold or visitor.");
    std::function<void(Cnf)> visit = [&](Cnf current) {
        current = normalize(std::move(current));
        const auto core = find_core_branch(current, threshold);
        if (!core) { visitor(current); return; }
        auto children = split_core(current, *core);
        visit(std::move(children.first));
        visit(std::move(children.second));
    };
    visit(formula);
}

Integer count_reduced_paper_pipeline(const Cnf& formula, std::size_t threshold,
                                     const DesignParameters& parameters, PaperCounterStats* stats) {
    Integer total = 0;
    visit_sparse_leaves(formula, threshold, [&](const Cnf& leaf) {
        const auto reduced = reduce_occurrences(leaf);
        const auto design = build_paper_design(reduced.formula, parameters);
        total += count_with_paper_pipeline(reduced.formula, design, stats);
    });
    return total;
}
} // namespace three_sat::detail
