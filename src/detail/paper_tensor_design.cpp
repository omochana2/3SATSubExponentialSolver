#include "three_sat/detail/paper_design.hpp"
#include "three_sat/detail/paper_schedule.hpp"
#include <functional>
#include <limits>
#include <stdexcept>

namespace three_sat::detail {
namespace {
std::size_t bounded_power(std::size_t base, std::size_t exponent, std::size_t bound) {
    std::size_t result = 1;
    for (std::size_t i = 0; i < exponent; ++i) {
        if (result > bound / base) throw std::length_error("Auxiliary design exceeds its explicit coordinate budget.");
        result *= base;
    }
    return result;
}
struct Eigenvector { std::vector<int> row; std::size_t negative; };

Rational spectral_multiplier(std::size_t negatives, const DesignParameters& p) {
    // (4.7): (-1)^k f_I(k), using generalized binomial coefficients also for k<A.
    Rational binomial = 1, sum = 0;
    Integer power = 1;
    const Integer offset = Integer(std::to_string(negatives)) - Integer(std::to_string(p.interval_start));
    for (std::size_t j = 0; j < p.interval_length; ++j) {
        sum += power * binomial;
        binomial *= offset - Integer(std::to_string(j));
        binomial /= Integer(std::to_string(j + 1));
        power *= -2;
    }
    return ((negatives + p.interval_start) % 2) ? Rational(-sum) : sum;
}
}

PaperDesign build_paper_design(const Cnf& formula, const DesignParameters& p) {
    (void)normalize(formula);
    if (p.groups < 2 || !p.point_count || p.point_count > static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
        !p.interval_length || p.interval_start > p.point_count ||
        p.interval_length - 1 > p.point_count - p.interval_start)
        throw std::invalid_argument("Invalid tensor design parameters or spectral interval.");
    const auto K = bounded_power(p.groups, p.point_count, p.max_auxiliary_coordinates);
    if (K > p.max_auxiliary_coordinates / K)
        throw std::length_error("Auxiliary design exceeds its explicit coordinate budget.");
    const auto D = K * K;
    if (D > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        throw std::length_error("Projected integer measurement coefficients exceed supported dimensions.");
    std::vector<std::vector<std::size_t>> digits(K, std::vector<std::size_t>(p.point_count));
    for (std::size_t a = 0; a < K; ++a) {
        auto encoded = a;
        for (auto& digit : digits[a]) { digit = encoded % p.groups; encoded /= p.groups; }
    }
    // Orthogonal one-site eigenbasis: diagonal e_aa; symmetric e_ab+e_ba;
    // antisymmetric e_ab-e_ba. Tensor products diagonalize all commuting exchanges.
    std::vector<Eigenvector> local_basis;
    for (std::size_t a = 0; a < p.groups; ++a) {
        Eigenvector e{std::vector<int>(p.groups * p.groups), 0};
        e.row[a * p.groups + a] = 1;
        local_basis.push_back(std::move(e));
    }
    for (std::size_t a = 0; a < p.groups; ++a) for (auto d = a + 1; d < p.groups; ++d)
        for (int sign : {1, -1}) {
            Eigenvector e{std::vector<int>(p.groups * p.groups), sign == -1 ? 1u : 0u};
            e.row[a * p.groups + d] = 1; e.row[d * p.groups + a] = sign;
            local_basis.push_back(std::move(e));
        }
    std::vector<std::vector<Rational>> M(D, std::vector<Rational>(D));
    for (std::size_t u = 0; u < D; ++u) M[u][u] = 1;
    std::vector<std::vector<int>> tail_rows;
    std::vector<std::size_t> choices(p.point_count);
    std::function<void(std::size_t, std::size_t)> visit = [&](std::size_t t, std::size_t negatives) {
        if (t != p.point_count) {
            for (std::size_t j = 0; j < local_basis.size(); ++j) {
                choices[t] = j; visit(t + 1, negatives + local_basis[j].negative);
            }
            return;
        }
        if (negatives >= p.interval_start && negatives - p.interval_start < p.interval_length) return;
        std::vector<int> row(D, 1);
        std::vector<std::size_t> support;
        for (std::size_t u = 0; u < D; ++u) {
            for (std::size_t s = 0; s < p.point_count; ++s)
                row[u] *= local_basis[choices[s]].row[digits[u / K][s] * p.groups + digits[u % K][s]];
            if (row[u]) support.push_back(u);
        }
        // The tensor basis is orthogonal. I-M is the sum of its tail projectors
        // weighted by 1-(-1)^k f_I(k); each norm is the number of nonzero entries.
        const Rational weight = (1 - spectral_multiplier(negatives, p)) / Integer(std::to_string(support.size()));
        for (auto u : support) for (auto y : support) M[u][y] -= weight * row[u] * row[y];
        tail_rows.push_back(std::move(row));
    };
    visit(0, 0);

    PaperDesign result;
    result.variable_count = formula.variable_count;
    for (std::size_t v = 0; v < formula.variable_count; ++v) result.variable_labels.push_back(static_cast<unsigned>(v % K));
    for (std::size_t f = 0; f < formula.clauses.size(); ++f)
        result.factor_labels.push_back(static_cast<unsigned>((f + K - 1) % K));
    if (p.balanced_schedules) {
        const auto schedule = choose_balanced_schedules(formula, p.groups, p.point_count, K);
        result.variable_labels = schedule.variable_labels;
        result.factor_labels = schedule.factor_labels;
    }
    std::vector<std::size_t> used(D);
    std::vector<std::vector<int>> positions;
    std::vector<std::vector<std::pair<std::size_t, std::size_t>>> incidences;
    for (std::size_t f = 0; f < formula.clauses.size(); ++f) {
        incidences.emplace_back();
        for (int literal : formula.clauses[f]) {
            const auto v = static_cast<std::size_t>((literal < 0 ? -literal : literal) - 1);
            const auto label = result.variable_labels[v] * K + result.factor_labels[f];
            const auto copy = used[label]++;
            if (copy == positions.size()) positions.emplace_back(D, -1);
            positions[copy][label] = static_cast<int>(v);
            incidences.back().push_back({label, copy});
        }
    }
    for (const auto& copy : positions) for (const auto& row : tail_rows) {
        std::vector<int> projected(formula.variable_count);
        for (std::size_t y = 0; y < D; ++y) if (copy[y] >= 0) projected[copy[y]] += row[y];
        result.measurements.push_back(std::move(projected));
    }
    for (const auto& factor : incidences) {
        result.forms.emplace_back();
        for (const auto& [u, copy] : factor) {
            ReplacementForm form{static_cast<std::size_t>(positions[copy][u]),
                                 std::vector<Polynomial>(formula.variable_count, Polynomial(p.interval_length)), copy, 0};
            form.coordinates[form.variable][0] = 1;
            for (std::size_t y = 0; y < D; ++y) if (M[u][y] != 0 && positions[copy][y] >= 0) {
                Polynomial phi{1};
                for (std::size_t s = 0; s < p.point_count; ++s) if (digits[y / K][s] != digits[u % K][s]) {
                    Polynomial next(phi.size() + 1);
                    for (std::size_t j = 0; j < phi.size(); ++j) {
                        next[j] += phi[j];
                        next[j + 1] -= phi[j] / Integer(std::to_string(s + 1));
                    }
                    phi = std::move(next);
                }
                if (phi.size() > p.interval_length) throw std::logic_error("Tensor bandwidth identity failed.");
                // The constant parts of (4.10) cancel to e_v; only phi(T)-1 remains.
                for (std::size_t j = 1; j < phi.size(); ++j)
                    form.coordinates[positions[copy][y]][j] += M[u][y] * phi[j];
            }
            result.forms.back().push_back(std::move(form));
        }
    }
    for (std::size_t t = 0; t < p.point_count; ++t) {
        DesignPoint point{static_cast<int>(t + 1), p.groups, {}, {}, {}, {}};
        for (auto a : result.variable_labels) point.variable_groups.push_back(digits[a][t]);
        for (auto d : result.factor_labels) point.factor_groups.push_back(digits[d][t]);
        point.measurements.assign(p.groups, result.measurements);
        for (std::size_t g = 0; g < p.groups; ++g) for (auto& row : point.measurements[g])
            for (std::size_t v = 0; v < formula.variable_count; ++v) if (point.variable_groups[v] != g) row[v] = 0;
        complete_design_point(result, point);
        result.points.push_back(std::move(point));
    }
    validate_paper_design(formula, result);
    return result;
}
} // namespace three_sat::detail
