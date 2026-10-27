#include "three_sat/detail/paper_design.hpp"
#include <algorithm>
#include <array>
#include <stdexcept>

namespace three_sat::detail {
namespace {
unsigned exchange(unsigned label, unsigned bit) {
    auto a = label / 4, d = label % 4;
    if ((a ^ d) & bit) { a ^= bit; d ^= bit; }
    return 4 * a + d;
}
int sign(unsigned a) { return ((a & 1) != 0) == ((a & 2) != 0) ? 1 : -1; }
}

PaperDesign build_paper_design(const Cnf& formula) {
    // Validate public internal inputs as well; normalization is otherwise the caller's job.
    (void)normalize(formula);
    PaperDesign result;
    result.variable_count = formula.variable_count;
    for (std::size_t v = 0; v < formula.variable_count; ++v)
        result.variable_labels.push_back(static_cast<unsigned>(v % 4));
    std::array<std::size_t, 16> used{};
    std::vector<std::array<int, 16>> positions;
    std::vector<std::vector<std::pair<unsigned, std::size_t>>> incidences;
    for (std::size_t f = 0; f < formula.clauses.size(); ++f) {
        // Start with d=11 so that small examples exercise nonconstant replacements.
        const unsigned d = static_cast<unsigned>((f + 3) % 4);
        result.factor_labels.push_back(d);
        incidences.emplace_back();
        for (int literal : formula.clauses[f]) {
            const auto v = static_cast<std::size_t>((literal < 0 ? -literal : literal) - 1);
            const unsigned label = 4 * result.variable_labels[v] + d;
            const auto copy = used[label]++;
            if (copy == positions.size()) {
                std::array<int, 16> empty;
                empty.fill(-1);
                positions.push_back(empty);
            }
            positions[copy][label] = static_cast<int>(v);
            incidences.back().push_back({label, copy});
        }
    }
    // C^B is the tensor of the two antisymmetric rows. It is zero except on the
    // four labels with a XOR d == 11. I-M=(I-Pi_1)(I-Pi_2) factors through this row.
    result.measurements.assign(positions.size(), std::vector<int>(formula.variable_count));
    for (std::size_t c = 0; c < positions.size(); ++c)
        for (unsigned a = 0; a < 4; ++a) {
            const int v = positions[c][4 * a + (a ^ 3)];
            if (v >= 0) result.measurements[c][static_cast<std::size_t>(v)] += sign(a);
        }
    for (const auto& factor : incidences) {
        result.forms.emplace_back();
        for (const auto& [label, copy] : factor) {
            const unsigned a = label / 4, d = label % 4;
            ReplacementForm form{static_cast<std::size_t>(positions[copy][label]),
                                 std::vector<Polynomial>(formula.variable_count, Polynomial(2)), copy,
                                 (a ^ d) == 3 ? sign(a) : 0};
            form.coordinates[form.variable][0] = 1;
            // Equation (4.10), M=Pi_1+Pi_2-Pi_1 Pi_2. A single exchange leaves
            // at most one mismatch, giving (1-T/s); a double exchange leaves none.
            // All constant terms combine to e_v, so only these slopes remain.
            for (unsigned bit : {1u, 2u}) {
                const unsigned other = bit == 1 ? 2 : 1;
                if (!((a ^ d) & other)) continue;
                const int v = positions[copy][exchange(label, bit)];
                if (v >= 0) form.coordinates[static_cast<std::size_t>(v)][1] -= Rational(1, other);
            }
            result.forms.back().push_back(std::move(form));
        }
    }
    for (int t : {1, 2}) {
        DesignPoint point{t, 2, {}, {}, {}, {}};
        const unsigned bit = t == 1 ? 1 : 2;
        for (auto a : result.variable_labels) point.variable_groups.push_back((a & bit) != 0);
        for (auto d : result.factor_labels) point.factor_groups.push_back((d & bit) != 0);
        point.measurements.assign(2, result.measurements);
        for (std::size_t g = 0; g < 2; ++g) for (auto& row : point.measurements[g])
            for (std::size_t v = 0; v < formula.variable_count; ++v)
                if (point.variable_groups[v] != g) row[v] = 0;
        complete_design_point(result, point);
        result.points.push_back(std::move(point));
    }
    return result;
}
} // namespace three_sat::detail
