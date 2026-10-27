#include "three_sat/detail/paper_design.hpp"
#include <set>
#include <stdexcept>

namespace three_sat::detail {
namespace {
void require(bool condition) {
    if (!condition) throw std::invalid_argument("Invalid replacement design or row-space identity.");
}
void check_point_shape(const PaperDesign& design, const DesignPoint& point) {
    const auto n = design.variable_count;
    require(point.value != 0 && point.group_count > 0);
    require(point.variable_groups.size() == n && point.factor_groups.size() == design.forms.size());
    require(point.measurements.size() == point.group_count);
    for (auto group : point.variable_groups) require(group < point.group_count);
    for (auto group : point.factor_groups) require(group < point.group_count);
    for (std::size_t g = 0; g < point.group_count; ++g) for (const auto& row : point.measurements[g]) {
        require(row.size() == n);
        for (std::size_t v = 0; v < n; ++v) if (point.variable_groups[v] != g) require(row[v] == 0);
    }
    for (const auto& factor : design.forms) for (const auto& form : factor)
        require(form.coordinates.size() == n && form.variable < n);
}

// Solve C^T lambda = target. Dependent rows are allowed; free coefficients are zero.
std::vector<Rational> row_combination(const std::vector<std::vector<int>>& rows,
                                      const std::vector<Rational>& target) {
    const auto columns = rows.size(), n = target.size();
    std::vector<std::vector<Rational>> equations(n, std::vector<Rational>(columns + 1));
    for (std::size_t v = 0; v < n; ++v) {
        for (std::size_t j = 0; j < columns; ++j) equations[v][j] = rows[j][v];
        equations[v].back() = target[v];
    }
    std::vector<std::size_t> pivots;
    std::size_t rank = 0;
    for (std::size_t j = 0; j < columns && rank < n; ++j) {
        auto pivot = rank;
        while (pivot < n && equations[pivot][j] == 0) ++pivot;
        if (pivot == n) continue;
        std::swap(equations[rank], equations[pivot]);
        const Rational divisor = equations[rank][j];
        for (std::size_t k = j; k <= columns; ++k) equations[rank][k] /= divisor;
        for (std::size_t r = 0; r < n; ++r) if (r != rank) {
            const Rational scale = equations[r][j];
            for (std::size_t k = j; k <= columns; ++k) equations[r][k] -= scale * equations[rank][k];
        }
        pivots.push_back(j);
        ++rank;
    }
    for (std::size_t r = rank; r < n; ++r) require(equations[r].back() == 0);
    std::vector<Rational> result(columns);
    for (std::size_t r = 0; r < rank; ++r) result[pivots[r]] = equations[r].back();
    return result;
}
}

void complete_design_point(const PaperDesign& design, DesignPoint& point) {
    check_point_shape(design, point);
    // Keep an independent subset of the original integer rows. This does not change
    // their row space, and avoids enumerating redundant or identically zero measurements.
    for (auto& rows : point.measurements) {
        std::vector<std::vector<int>> basis;
        for (const auto& row : rows) {
            std::vector<Rational> target(row.begin(), row.end());
            try { (void)row_combination(basis, target); }
            catch (const std::invalid_argument&) { basis.push_back(row); }
        }
        rows = std::move(basis);
    }
    point.forms.clear();
    for (std::size_t f = 0; f < design.forms.size(); ++f) {
        point.forms.emplace_back();
        for (const auto& form : design.forms[f]) {
            PointForm evaluated;
            evaluated.local_coefficients.resize(design.variable_count);
            for (std::size_t g = 0; g < point.group_count; ++g) {
                std::vector<Rational> target(design.variable_count);
                for (std::size_t v = 0; v < design.variable_count; ++v) if (point.variable_groups[v] == g) {
                    const auto value = polynomial_value(form.coordinates[v], point.value);
                    if (g == point.factor_groups[f]) evaluated.local_coefficients[v] = value;
                    else target[v] = value;
                }
                evaluated.measurement_weights.push_back(row_combination(point.measurements[g], target));
            }
            point.forms.back().push_back(std::move(evaluated));
        }
    }
}

void validate_paper_design(const Cnf& formula, const PaperDesign& design) {
    (void)normalize(formula); // Validate literals without reordering the caller's factors.
    require(design.variable_count == formula.variable_count && design.forms.size() == formula.clauses.size());
    require(!design.points.empty());
    for (std::size_t f = 0; f < formula.clauses.size(); ++f) {
        require(design.forms[f].size() == formula.clauses[f].size());
        for (std::size_t i = 0; i < formula.clauses[f].size(); ++i) {
            const auto& form = design.forms[f][i];
            const int literal = formula.clauses[f][i];
            require(form.variable == static_cast<std::size_t>((literal < 0 ? -literal : literal) - 1));
            require(form.coordinates.size() == design.variable_count);
            for (std::size_t v = 0; v < design.variable_count; ++v)
                require(polynomial_value(form.coordinates[v], 0) == (v == form.variable ? 1 : 0));
        }
    }
    std::set<int> values;
    for (const auto& point : design.points) {
        check_point_shape(design, point);
        require(values.insert(point.value).second && point.forms.size() == design.forms.size());
        for (std::size_t f = 0; f < design.forms.size(); ++f) {
            require(point.forms[f].size() == design.forms[f].size());
            for (std::size_t i = 0; i < design.forms[f].size(); ++i) {
                const auto& evaluated = point.forms[f][i];
                require(evaluated.local_coefficients.size() == design.variable_count);
                require(evaluated.measurement_weights.size() == point.group_count);
                for (std::size_t g = 0; g < point.group_count; ++g)
                    require(evaluated.measurement_weights[g].size() == point.measurements[g].size());
                for (std::size_t v = 0; v < design.variable_count; ++v) {
                    const auto g = point.variable_groups[v];
                    Rational value = evaluated.local_coefficients[v];
                    if (g != point.factor_groups[f]) require(value == 0);
                    for (std::size_t row = 0; row < point.measurements[g].size(); ++row)
                        value += evaluated.measurement_weights[g][row] * point.measurements[g][row][v];
                    require(value == polynomial_value(design.forms[f][i].coordinates[v], point.value));
                }
            }
        }
    }
}
} // namespace three_sat::detail
