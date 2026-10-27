#include "three_sat/detail/paper_schedule.hpp"
#include <algorithm>
#include <limits>
#include <map>
#include <stdexcept>

namespace three_sat::detail {
namespace {
Integer number(std::size_t value) { return Integer(std::to_string(value)); }
struct IncidenceData {
    std::vector<std::pair<std::size_t, std::size_t>> edges;
    std::size_t b = 0, w = 0;
};
IncidenceData incidences(const Cnf& formula) {
    IncidenceData data;
    std::map<std::size_t, std::size_t> degree;
    for (std::size_t f = 0; f < formula.clauses.size(); ++f) {
        data.w = std::max(data.w, formula.clauses[f].size());
        for (int literal : formula.clauses[f]) {
            const auto v = static_cast<std::size_t>((literal < 0 ? -literal : literal) - 1);
            data.edges.push_back({f, v}); data.b = std::max(data.b, ++degree[v]);
        }
    }
    return data;
}
std::size_t ceil_log2(std::size_t value) {
    std::size_t result = 0;
    if (value) --value;
    while (value) { ++result; value >>= 1; }
    return result;
}
}

ScheduleResult choose_balanced_schedules(const Cnf& formula, std::size_t r, std::size_t Q, std::size_t limit) {
    (void)normalize(formula);
    if (r < 2 || !Q) throw std::invalid_argument("Invalid schedule dimensions.");
    std::size_t K = 1;
    for (std::size_t t = 0; t < Q; ++t) {
        if (K > limit / r) throw std::length_error("Schedule enumeration exceeds its explicit budget.");
        K *= r;
    }
    if (K > static_cast<std::size_t>(std::numeric_limits<int>::max())) throw std::length_error("Too many schedules.");
    std::vector<std::vector<std::size_t>> digits(K, std::vector<std::size_t>(Q));
    for (std::size_t a = 0; a < K; ++a) {
        auto code = a;
        for (auto& digit : digits[a]) { digit = code % r; code /= r; }
    }
    const auto data = incidences(formula);
    const auto M = data.edges.size(), n = formula.variable_count;
    std::vector<int> variables(n, -1), factors(formula.clauses.size(), -1);
    for (std::size_t f = 0; f < factors.size(); ++f) if (formula.clauses[f].empty()) factors[f] = 0;
    auto equality = [&](const std::vector<int>& assigned, std::size_t i, std::size_t j) -> Rational {
        if (i == j) return 1;
        if (assigned[i] < 0 || assigned[j] < 0) return Rational(1, number(K));
        return assigned[i] == assigned[j] ? 1 : 0;
    };
    auto potential = [&]() -> Rational {
        Rational phi = 0, xi = 0;
        if (M) {
            for (const auto& [f, v] : data.edges) for (const auto& [other_f, other_v] : data.edges)
                phi += equality(variables, v, other_v) * equality(factors, f, other_f);
            phi -= Rational(number(M) * number(M)) / (number(K) * number(K));
            phi /= number(M) * (number(data.b) + number(data.w));
        }
        if (n) {
            for (std::size_t t = 0; t < Q; ++t) for (std::size_t v = 0; v < n; ++v)
                for (std::size_t other = 0; other < n; ++other) {
                    if (v == other) xi += 1;
                    else if (variables[v] < 0 || variables[other] < 0) xi += Rational(1, number(r));
                    else if (digits[variables[v]][t] == digits[variables[other]][t]) xi += 1;
                }
            xi -= Rational(number(Q) * number(n) * number(n)) / number(r);
            xi /= number(Q) * number(n);
        }
        return phi + xi;
    };
    ScheduleResult result;
    result.initial_potential = potential();
    Rational previous = result.initial_potential;
    for (auto* list : {&variables, &factors}) for (auto& value : *list) if (value < 0) {
        Rational best;
        int winner = 0;
        for (std::size_t candidate = 0; candidate < K; ++candidate) {
            value = static_cast<int>(candidate);
            const Rational current = potential();
            if (!candidate || current < best) { best = current; winner = value; }
        }
        value = winner;
        if (best > previous) throw std::logic_error("Conditional expectation increased.");
        previous = best;
    }
    result.final_potential = previous;
    for (auto value : variables) result.variable_labels.push_back(static_cast<unsigned>(value));
    for (auto value : factors) result.factor_labels.push_back(static_cast<unsigned>(value));
    std::vector<std::size_t> loads(K * K);
    for (const auto& [f, v] : data.edges)
        result.maximum_label_load = std::max(result.maximum_label_load, ++loads[variables[v] * K + factors[f]]);
    for (std::size_t t = 0; t < Q; ++t) {
        std::vector<std::size_t> sizes(r);
        for (auto a : variables) result.maximum_group_size = std::max(result.maximum_group_size, ++sizes[digits[a][t]]);
    }
    const Integer D = number(K) * number(K);
    result.balance_hypotheses = n > 0 && M >= n && data.b && data.w &&
        2 * (number(data.b) + number(data.w)) * D * D <= number(M) &&
        2 * number(Q) * number(r) * number(r) <= number(n);
    if (result.balance_hypotheses && (number(result.maximum_label_load) * D > 2 * number(M) ||
        number(result.maximum_group_size) * number(r) > 2 * number(n)))
        throw std::logic_error("Balanced schedule bound failed.");
    return result;
}

ParameterSelection select_asymptotic_parameters(const Cnf& formula, unsigned beta) {
    (void)normalize(formula);
    if (!beta) throw std::invalid_argument("Lemma 4.8 requires beta >= 1.");
    ParameterSelection result;
    const auto n = formula.variable_count;
    const auto data = incidences(formula);
    if (n < 20) { result.failures.push_back("n < 20"); return result; }
    const auto lambda = ceil_log2(n), nu = ceil_log2(lambda);
    std::size_t r = 0, root = 0;
    while (number(r + 1) * number(r + 1) * number(nu) * number(nu) * number(nu) * number(nu) <= number(lambda)) ++r;
    while (root * root < lambda) ++root;
    const auto Q = lambda / (128 * nu);
    const Integer wide_R = (Integer(beta) + 3) * number(root);
    result.parameters.groups = r;
    result.parameters.point_count = Q;
    if (wide_R > number(std::numeric_limits<std::size_t>::max())) {
        result.failures.push_back("R exceeds size_t"); return result;
    }
    const auto R = static_cast<std::size_t>(std::stoull(wide_R.get_str()));
    result.parameters.interval_length = R;
    if (r < 2) result.failures.push_back("r < 2");
    if (Q <= R) result.failures.push_back("Q <= R");
    if (R < 32) result.failures.push_back("R < 32");
    if (6 * wide_R > number(Q)) result.failures.push_back("6R > Q");
    // For every supported machine-sized n, these early size tests fail. Return the
    // actual failed hypotheses rather than silently substituting small parameters.
    if (!result.failures.empty()) return result;
    auto check = [&](bool condition, const char* reason) { if (!condition) result.failures.push_back(reason); };
    const Integer M = number(data.edges.size()), b = number(data.b), w = number(data.w);
    check(data.b && data.w && number(n) <= M && M <= b * number(n), "Invalid incidence bounds n <= M <= bn");
    Integer incidence_bound = 1;
    for (unsigned j = 0; j < beta && incidence_bound < std::max(b, w); ++j) incidence_bound *= number(lambda);
    check(b <= incidence_bound && w <= incidence_bound, "b or w exceeds lambda^beta");
    Integer D = 1;
    for (std::size_t j = 0; j < 2 * Q; ++j) D *= number(r);
    check(2 * (b + w) * D * D <= M, "Label balance inequality fails");
    check(2 * number(Q) * number(r) * number(r) <= number(n), "Group balance inequality fails");
    const Integer numerator = number(Q) * (number(r) - 1) - number(r) * (wide_R - 1);
    if (numerator < 0) { result.failures.push_back("Negative spectral interval start"); return result; }
    const Integer start = numerator / (2 * number(r));
    result.parameters.interval_start = static_cast<std::size_t>(std::stoull(start.get_str()));
    check(start + wide_R - 1 <= number(Q), "Spectral interval exceeds [0,Q]");
    // Tail probability numerator over (2r)^Q, with no floating-point estimates.
    Integer tail = 0, binomial = 1, denominator = 1;
    for (std::size_t j = 0; j < Q; ++j) denominator *= 2 * number(r);
    for (std::size_t j = 0; j <= Q; ++j) {
        if (number(j) < start || number(j) >= start + wide_R) {
            Integer weight = binomial;
            for (std::size_t k = 0; k < j; ++k) weight *= number(r) - 1;
            for (auto k = j; k < Q; ++k) weight *= number(r) + 1;
            tail += weight;
        }
        if (j < Q) { binomial *= number(Q - j); binomial /= number(j + 1); }
    }
    check(tail * b * number(r) * number(r) * number(lambda) <= denominator, "Exact binomial tail exceeds epsilon");
    result.parameters.balanced_schedules = true;
    result.admissible = result.failures.empty();
    return result;
}
} // namespace three_sat::detail
