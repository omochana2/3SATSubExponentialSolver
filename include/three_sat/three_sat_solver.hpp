#pragma once
#include "three_sat/three_sat.hpp"
#include <cstdint>
#include <optional>

/// Common solver API: owns a formula snapshot, caches its count, and reconstructs witnesses.
/// Derived classes implement only count(formula). Witness reconstruction uses that virtual
/// operation for every restricted formula, so the selected algorithm is used throughout.
/// The lazy cache is not synchronized; concurrent first calls require external locking.
class ThreeSatSolver {
public:
    explicit ThreeSatSolver(const ThreeSAT& formula);
    virtual ~ThreeSatSolver() = default;

    /// Count the constructor's formula, caching only successful results.
    /// This convenience overload delegates to the virtual count(formula) operation.
    std::uint64_t count() const;
    /// Count any formula, including unused declared variables. Implementations must not
    /// change the original formula or its cache. Throw on counts exceeding uint64_t.
    virtual std::uint64_t count(const ThreeSAT& formula) const = 0;

    /// Return the false-first witness, or nullopt for UNSAT. Uses at most n+1 oracle calls.
    /// This is a polynomial-time reduction to counting, not a polynomial-time SAT algorithm.
    std::optional<Assignment> get_satisfiable() const;
private:
    ThreeSAT formula_;
    mutable bool computed_ = false;
    mutable std::uint64_t result_ = 0;
};
