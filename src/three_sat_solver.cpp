#include "three_sat/three_sat_solver.hpp"

ThreeSatSolver::ThreeSatSolver(const ThreeSAT& formula) : formula_(formula) {}
std::uint64_t ThreeSatSolver::count() const {
    if (!computed_) {
        // Publish the cache flag only after a successful count; exceptions leave it uncomputed.
        result_ = count(formula_);
        computed_ = true;
    }
    return result_;
}
std::optional<Assignment> ThreeSatSolver::get_satisfiable() const {
    // Establish that at least one extension exists before deciding any variable.
    if (count() == 0) return std::nullopt;
    // Keep the original immutable. This copy accumulates the chosen unit clauses.
    ThreeSAT restricted = formula_;
    Assignment solution(formula_.variable_count());
    // Self-reduction: at most n + 1 calls to the counting oracle.
    for (std::size_t variable = 1; variable <= formula_.variable_count(); ++variable) {
        const int literal = static_cast<int>(variable);
        ThreeSAT trial = restricted;
        // Repeated literals encode the unit clause !x in 3SAT.
        trial.add_clause({-literal, -literal, -literal});
        // Invariant: restricted is satisfiable. If its false branch has no models,
        // the true branch must have one; otherwise choose false deterministically.
        const bool value = count(trial) == 0;
        solution.set(variable, value);
        const int fixed = value ? literal : -literal;
        restricted.add_clause({fixed, fixed, fixed});
    }
    return solution;
}

