#include "three_sat/detail/paper_counter.hpp"
#include <algorithm>
#include <functional>
#include <limits>
#include <stdexcept>

namespace three_sat::detail {
namespace {
// Once measurements are fixed, factors depend only on the variables of their assigned
// group. This is deliberately a streaming enumeration: no table of global assignments
// and no evaluation of the original CNF as a Boolean predicate is used here.
TaylorPolynomial restricted_sum(const Cnf& formula, const PaperDesign& design, const DesignPoint& point,
                               std::size_t precision, const std::vector<bool>& forced,
                               const std::vector<bool>& substitution, PaperCounterStats* stats) {
    if (stats) ++stats->restricted_sums;
    const auto n = formula.variable_count;
    std::vector<std::vector<std::size_t>> groups(point.group_count);
    for (std::size_t v = 0; v < n; ++v)
        groups[point.variable_groups[v]].push_back(v);
    std::vector<std::size_t> offsets(point.group_count + 1);
    std::vector<std::pair<Integer, Integer>> ranges;
    for (std::size_t g = 0; g < point.group_count; ++g) {
        for (const auto& row : point.measurements[g]) {
            ranges.emplace_back();
            for (auto v : groups[g]) {
                ranges.back().first += std::min(0, row[v]);
                ranges.back().second += std::max(0, row[v]);
            }
        }
        offsets[g + 1] = ranges.size();
    }
    std::vector<Integer> measurements(ranges.size());
    // Compute every coefficient of Delta P(z). Translating must precede truncation.
    // Corrections use 1_U, NOT the current assignment or its measurements.
    std::vector<std::vector<TaylorPolynomial>> corrections(design.forms.size());
    for (std::size_t f = 0; f < design.forms.size(); ++f) {
        for (const auto& form : design.forms[f]) {
            TaylorPolynomial correction(precision);
            for (std::size_t v = 0; v < n; ++v) if (substitution[v])
                correction += polynomial_taylor(form.coordinates[v], point.value, precision);
            correction.coefficients[0] = 0;
            corrections[f].push_back(std::move(correction));
        }
    }
    std::vector<bool> local(n);
    TaylorPolynomial answer(precision);
    auto evaluate_tuple = [&] {
        if (stats) ++stats->measurement_tuples;
        TaylorPolynomial tuple_product(precision, 1);
        for (std::size_t g = 0; g < point.group_count; ++g) {
            TaylorPolynomial group_sum(precision);
            std::function<void(std::size_t)> visit_local = [&](std::size_t index) {
                if (index != groups[g].size()) {
                    const auto v = groups[g][index];
                    if (!forced[v]) { local[v] = false; visit_local(index + 1); }
                    local[v] = true;
                    visit_local(index + 1);
                    local[v] = false;
                    return;
                }
                if (stats) ++stats->local_assignments;
                for (std::size_t c = 0; c < point.measurements[g].size(); ++c) {
                    Integer measured = 0;
                    for (auto v : groups[g]) if (local[v]) measured += point.measurements[g][c][v];
                    if (measured != measurements[offsets[g] + c]) return;
                }
                TaylorPolynomial product(precision, 1);
                for (std::size_t f = 0; f < formula.clauses.size(); ++f) {
                    if (point.factor_groups[f] != g) continue;
                    TaylorPolynomial unsatisfied(precision, 1);
                    for (std::size_t i = 0; i < formula.clauses[f].size(); ++i) {
                        const auto& evaluated = point.forms[f][i];
                        Rational value = 0;
                        for (std::size_t h = 0; h < point.group_count; ++h)
                            for (std::size_t row = 0; row < point.measurements[h].size(); ++row)
                                value += evaluated.measurement_weights[h][row] * measurements[offsets[h] + row];
                        for (auto v : groups[g]) if (local[v])
                            value += evaluated.local_coefficients[v];
                        TaylorPolynomial replacement = corrections[f][i];
                        replacement.coefficients[0] = value;
                        if (formula.clauses[f][i] > 0) {
                            TaylorPolynomial false_literal(precision, 1);
                            false_literal -= replacement;
                            unsatisfied *= false_literal;
                        } else {
                            unsatisfied *= replacement;
                        }
                    }
                    TaylorPolynomial clause(precision, 1);
                    clause -= unsatisfied;
                    product *= clause;
                }
                group_sum += product;
            };
            visit_local(0);
            tuple_product *= group_sum;
        }
        answer += tuple_product;
    };
    std::function<void(std::size_t)> visit_measurements = [&](std::size_t index) {
        if (index == measurements.size()) { evaluate_tuple(); return; }
        // Tighter row-specific bounds than [-D,D] omit only impossible tuples.
        for (Integer y = ranges[index].first; y <= ranges[index].second; ++y) {
            measurements[index] = y;
            visit_measurements(index + 1);
        }
    };
    visit_measurements(0);
    return answer;
}
}

TaylorPolynomial recover_taylor(const Cnf& formula, const PaperDesign& design, int point,
                                std::size_t precision, PaperCounterStats* stats) {
    validate_paper_design(formula, design);
    const auto found = std::find_if(design.points.begin(), design.points.end(),
                                    [&](const DesignPoint& candidate) { return candidate.value == point; });
    if (found == design.points.end()) throw std::invalid_argument("Evaluation point is absent from design.");
    TaylorPolynomial answer(precision);
    std::vector<bool> forced(formula.variable_count), substitution(formula.variable_count);
    std::vector<std::size_t> support;
    const auto max_support = std::min(precision - 1, formula.variable_count);
    // Enumerate E and U subset E separately. The two bit vectors have different roles:
    // E restricts the summation x; U fixes the formal correction input X.
    auto evaluate_support = [&] {
        std::function<void(std::size_t, bool)> visit_subsets = [&](std::size_t index, bool negative) {
            if (index == support.size()) {
                const auto sum = restricted_sum(formula, design, *found, precision, forced, substitution, stats);
                if (negative) answer -= sum; else answer += sum;
                return;
            }
            const auto v = support[index];
            substitution[v] = false;
            visit_subsets(index + 1, !negative); // v in E\U contributes a minus sign.
            substitution[v] = true;
            visit_subsets(index + 1, negative);
            substitution[v] = false;
        };
        visit_subsets(0, false);
    };
    std::function<void(std::size_t)> visit_supports = [&](std::size_t start) {
        evaluate_support();
        if (support.size() == max_support) return;
        for (auto v = start; v < formula.variable_count; ++v) {
            support.push_back(v); forced[v] = true;
            visit_supports(v + 1);
            forced[v] = false; support.pop_back();
        }
    };
    visit_supports(0);
    return answer;
}

Integer count_with_paper_pipeline(const Cnf& input, PaperCounterStats* stats) {
    const auto formula = normalize(input);
    const auto design = build_paper_design(formula, DesignParameters{});
    return count_with_paper_pipeline(formula, design, stats);
}

Integer count_with_paper_pipeline(const Cnf& formula, const PaperDesign& design, PaperCounterStats* stats) {
    validate_paper_design(formula, design);
    std::size_t degree_bound = 0;
    for (const auto& factor : design.forms) for (const auto& form : factor) {
        std::size_t degree = 0;
        for (const auto& coordinate : form.coordinates) degree = std::max(degree, polynomial_degree(coordinate));
        if (degree > std::numeric_limits<std::size_t>::max() - degree_bound)
            throw std::overflow_error("Polynomial degree exceeds size_t.");
        degree_bound += degree;
        if (stats && degree) ++stats->nonconstant_forms;
    }
    const auto precision = degree_bound / design.points.size() + 1; // Q*J > deg(H)
    std::vector<TaylorPolynomial> jets;
    std::vector<int> points;
    for (const auto& point : design.points) {
        points.push_back(point.value);
        jets.push_back(recover_taylor(formula, design, point.value, precision, stats));
    }
    if (stats) ++stats->interpolation_calls;
    const auto count = hermite_at_zero(points, jets, degree_bound);
    if (count.get_den() != 1 || count < 0)
        throw std::logic_error("Algebraic counter produced a nonintegral or negative model count.");
    return count.get_num();
}
} // namespace three_sat::detail
