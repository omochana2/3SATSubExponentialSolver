#include "three_sat/three_sat_naive_solver.hpp"
#include <limits>
#include <stdexcept>

namespace {
// Variables before 'variable' are fixed. Each recursive call overwrites its suffix,
// so explicit rollback is unnecessary. The two branches partition all extensions.
std::uint64_t enumerate(const ThreeSAT& formula, std::size_t variable, Assignment& assignment) {
    if (variable > formula.variable_count()) {
        return formula.is_satisfied_by(assignment) ? 1 : 0;
    }
    assignment.set(variable, false);
    const auto false_count = enumerate(formula, variable + 1, assignment);
    assignment.set(variable, true);
    const auto true_count = enumerate(formula, variable + 1, assignment);
    // Check before addition to prevent silent unsigned wraparound.
    if (true_count > std::numeric_limits<std::uint64_t>::max() - false_count) {
        throw std::overflow_error("Satisfying assignment count exceeds uint64_t.");
    }
    return false_count + true_count;
}
} // namespace

std::uint64_t ThreeSatNaiveSolver::count(const ThreeSAT& formula) const {
    Assignment assignment(formula.variable_count());
    return enumerate(formula, 1, assignment);
}
