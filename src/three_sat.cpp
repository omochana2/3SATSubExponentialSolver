#include "three_sat/three_sat.hpp"
#include <cstdint>
#include <limits>
#include <stdexcept>

ThreeSAT::ThreeSAT(std::size_t variable_count) : variable_count_(variable_count) {
    // Every declared variable must be expressible as a positive int literal.
    if (variable_count > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::invalid_argument("Too many variables for int literals.");
    }
}
std::size_t ThreeSAT::variable_count() const noexcept { return variable_count_; }
const std::vector<ThreeSAT::Clause>& ThreeSAT::clauses() const noexcept { return clauses_; }
void ThreeSAT::add_clause(const Clause& clause) {
    // Check the entire clause before mutating storage (strong validation guarantee).
    for (int literal : clause) {
        if (literal == 0 || variable_of(literal) > variable_count_) {
            throw std::invalid_argument("Literal must be a nonzero, valid variable index.");
        }
    }
    clauses_.push_back(clause);
}
bool ThreeSAT::is_satisfied_by(const Assignment& assignment) const {
    // Reject mismatches even for an empty formula; the variable domain is part of the API.
    if (assignment.variable_count() != variable_count_) {
        throw std::invalid_argument("Assignment and formula sizes must match.");
    }
    for (const auto& clause : clauses_) {
        bool satisfied = false;
        // OR the three signed literals to obtain this clause's truth value.
        for (int literal : clause) {
            const bool value = assignment.get(variable_of(literal));
            satisfied = satisfied || (literal > 0 ? value : !value);
        }
        // Clauses are AND-ed: one false clause is enough to reject this assignment.
        if (!satisfied) return false;
    }
    // With no clauses, the empty conjunction is true by definition.
    return true;
}
std::size_t ThreeSAT::variable_of(int literal) noexcept {
    // Widen before negating so INT_MIN is handled safely.
    const auto widened = static_cast<std::int64_t>(literal);
    return static_cast<std::size_t>(widened < 0 ? -widened : widened);
}
