#include "three_sat/three_sat_subexponential_solver.hpp"
#include "three_sat/detail/cnf.hpp"
#include "three_sat/detail/paper_pipeline.hpp"
#include <algorithm>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string>

namespace {
using three_sat::detail::Cnf;
using three_sat::detail::Clause;

// Saturate at 2^64, without confusing that value with zero. All operations are monotone
// on nonnegative counts. This preserves every representable result, including 0 times an
// overflowing component. Throw only when the final result cannot fit in the public API.
struct Count {
    std::uint64_t value;
    bool overflow = false;
    bool zero() const { return !overflow && value == 0; }
};
Count multiply(Count a, Count b) {
    if (a.zero() || b.zero()) return {0};
    if (a.overflow || b.overflow || b.value > std::numeric_limits<std::uint64_t>::max() / a.value)
        return {0, true};
    return {a.value * b.value};
}
Count power_of_two(std::size_t exponent) {
    return exponent >= 64 ? Count{0, true} : Count{std::uint64_t{1} << exponent};
}

// Counting-preserving preprocessing and disjoint sparsification feed algebraic leaves.
// remaining counts unfixed variables, while literal IDs stay in the original domain.
Count count_exact(Cnf formula, std::size_t remaining) {
    formula = three_sat::detail::normalize(std::move(formula));
    while (true) {
        if (formula.clauses.empty()) return power_of_two(remaining);
        if (formula.clauses.front().empty()) return {0};
        // Unit propagation is counting-preserving: each forced variable has one choice.
        if (formula.clauses.front().size() != 1) break;
        const int unit = formula.clauses.front()[0];
        std::vector<Clause> simplified;
        for (auto clause : formula.clauses) {
            if (std::binary_search(clause.begin(), clause.end(), unit)) continue;
            clause.erase(std::remove(clause.begin(), clause.end(), -unit), clause.end());
            simplified.push_back(std::move(clause));
        }
        formula.clauses = std::move(simplified);
        --remaining;
        formula = three_sat::detail::normalize(std::move(formula));
    }

    // Factor independent components, and retain 2^unused for every unfixed unused variable.
    std::map<int, int> parent;
    for (const auto& clause : formula.clauses) for (int literal : clause) {
        const int variable = literal < 0 ? -literal : literal;
        parent.emplace(variable, variable);
    }
    auto root = [&](int variable) {
        while (parent.at(variable) != variable) variable = parent.at(variable);
        return variable;
    };
    for (const auto& clause : formula.clauses) {
        const int first = clause[0] < 0 ? -clause[0] : clause[0];
        for (int literal : clause) {
            const int variable = literal < 0 ? -literal : literal;
            parent[root(variable)] = root(first);
        }
    }
    std::map<int, std::size_t> sizes;
    for (const auto& [variable, ignored] : parent) { (void)ignored; ++sizes[root(variable)]; }
    const auto free_factor = power_of_two(remaining - parent.size());
    if (sizes.size() > 1) {
        std::map<int, std::vector<Clause>> components;
        for (const auto& clause : formula.clauses) {
            const int variable = clause[0] < 0 ? -clause[0] : clause[0];
            components[root(variable)].push_back(clause);
        }
        Count product = free_factor;
        for (auto& [id, clauses] : components) {
            product = multiply(product, count_exact(Cnf{formula.variable_count, std::move(clauses)}, sizes.at(id)));
            if (product.zero()) break; // Even an overflowing earlier factor cannot change zero.
        }
        return product;
    }
    const auto active = parent.size();

    // Compact the active variable IDs. Removed/free variables have already been accounted
    // for above; they must not reappear as free inputs to the algebraic sum.
    std::map<int, int> compact;
    for (const auto& [variable, ignored] : parent) {
        (void)ignored;
        compact.emplace(variable, static_cast<int>(compact.size() + 1));
    }
    for (auto& clause : formula.clauses) for (auto& literal : clause)
        literal = literal < 0 ? -compact.at(-literal) : compact.at(literal);
    formula.variable_count = active;
    const auto exact = three_sat::detail::count_paper_pipeline(formula);
    const three_sat::detail::Integer maximum("18446744073709551615");
    const Count leaf = exact > maximum ? Count{0, true} : Count{std::stoull(exact.get_str())};
    return multiply(free_factor, leaf);
}
} // namespace

std::uint64_t ThreeSatSubexponentialSolver::count(const ThreeSAT& formula) const {
    // Exact simplifications precede the integrated Section 2.8 pipeline. Every
    // remaining component goes through sparsification, the occurrence bound,
    // balanced schedules and algebraic counting. The pipeline explicitly selects
    // the small-input regime when Lemma 4.8 fails; no subexponential claim is made.
    const auto result = count_exact(three_sat::detail::normalize(formula), formula.variable_count());
    if (result.overflow) throw std::overflow_error("Satisfying assignment count exceeds uint64_t.");
    return result.value;
}
