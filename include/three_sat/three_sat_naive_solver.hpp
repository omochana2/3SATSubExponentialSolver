#pragma once
#include "three_sat/three_sat_solver.hpp"

/// Exhaustive model counter: O(2^n * (m+1)) time and O(n) search memory.
class ThreeSatNaiveSolver : public ThreeSatSolver {
public:
    using ThreeSatSolver::ThreeSatSolver;
    // Keep the cached zero-argument overload visible despite the overload below.
    using ThreeSatSolver::count;
    std::uint64_t count(const ThreeSAT& formula) const override;
};
