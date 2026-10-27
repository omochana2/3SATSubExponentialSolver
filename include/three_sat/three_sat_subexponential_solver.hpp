#pragma once
#include "three_sat/three_sat_solver.hpp"

/// Exact counter using the integrated Section 2.8 reduction and algebraic pipeline.
/// The explicit small-input regime does not imply a subexponential runtime guarantee.
class ThreeSatSubexponentialSolver : public ThreeSatSolver {
public:
    using ThreeSatSolver::ThreeSatSolver;
    using ThreeSatSolver::count;
    std::uint64_t count(const ThreeSAT& formula) const override;
};

