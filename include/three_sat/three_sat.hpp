#pragma once
#include "three_sat/assignment.hpp"
#include <array>
#include <cstddef>
#include <vector>

/// A conjunction (AND) of clauses, each containing exactly three OR-ed literals.
/// Literal k means x_k; -k means NOT x_k. Zero is not a valid literal.
/// Repeated literals are allowed: {k, k, k} encodes the unit clause x_k.
class ThreeSAT {
public:
    using Clause = std::array<int, 3>;
    /// Throws std::invalid_argument if variable IDs cannot be represented by int.
    explicit ThreeSAT(std::size_t variable_count);
    std::size_t variable_count() const noexcept;
    /// Borrowed, read-only view. The formula must outlive the returned reference.
    const std::vector<Clause>& clauses() const noexcept;
    /// Rejects zero/out-of-range literals with std::invalid_argument.
    /// Validation finishes before insertion, so invalid input leaves the formula unchanged.
    void add_clause(const Clause& clause);
    /// Evaluates every clause against a complete assignment in O(m) time.
    /// An empty conjunction is true. Throws std::invalid_argument on a size mismatch.
    bool is_satisfied_by(const Assignment& assignment) const;
private:
    // Convert the sign-encoded literal to a variable ID without overflowing on INT_MIN.
    static std::size_t variable_of(int literal) noexcept;
    std::size_t variable_count_;
    std::vector<Clause> clauses_;
};
