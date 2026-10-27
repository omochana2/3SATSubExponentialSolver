#include "three_sat/three_sat_solver.hpp"
#include <gtest/gtest.h>
#include <stdexcept>
#include <type_traits>
#include <vector>

static_assert(std::is_abstract_v<ThreeSatSolver>);
static_assert(std::has_virtual_destructor_v<ThreeSatSolver>);

namespace {
// Fixed oracle responses test dispatch and caching without implementing another solver.
class RecordingSolver : public ThreeSatSolver {
public:
    using ThreeSatSolver::ThreeSatSolver;
    using ThreeSatSolver::count;
    mutable std::vector<ThreeSAT> calls;
    std::uint64_t count(const ThreeSAT& formula) const override {
        calls.push_back(formula);
        const std::uint64_t answers[] = {3, 1, 0};
        if (calls.size() > 3) throw std::runtime_error("Unexpected oracle call");
        return answers[calls.size() - 1];
    }
};

TEST(ThreeSatSolverTest, WitnessUsesDerivedCounterForEveryRestrictedFormula) {
    ThreeSAT formula(2);
    formula.add_clause({1, 2, 2});
    RecordingSolver implementation(formula);
    const ThreeSatSolver& solver = implementation;
    EXPECT_EQ(solver.count(), 3u);
    EXPECT_EQ(solver.count(), 3u);
    ASSERT_EQ(implementation.calls.size(), 1u); // Original count is cached.
    const auto witness = solver.get_satisfiable();
    ASSERT_TRUE(witness);
    EXPECT_FALSE(witness->get(1));
    EXPECT_TRUE(witness->get(2));
    ASSERT_EQ(implementation.calls.size(), 3u);
    // These are the exact two restricted formulas expected for this fixed example.
    const std::vector<ThreeSAT::Clause> first{{1, 2, 2}, {-1, -1, -1}};
    const std::vector<ThreeSAT::Clause> second{{1, 2, 2}, {-1, -1, -1}, {-2, -2, -2}};
    EXPECT_EQ(implementation.calls[1].clauses(), first);
    EXPECT_EQ(implementation.calls[2].clauses(), second);
    EXPECT_EQ(solver.count(), 3u);
    EXPECT_EQ(implementation.calls.size(), 3u);
}

class RetrySolver : public ThreeSatSolver {
public:
    using ThreeSatSolver::ThreeSatSolver;
    using ThreeSatSolver::count;
    mutable int calls = 0;
    std::uint64_t count(const ThreeSAT&) const override {
        if (++calls == 1) throw std::overflow_error("Simulated failure");
        return 1;
    }
};

TEST(ThreeSatSolverTest, FailedCountsAreNotCached) {
    RetrySolver solver(ThreeSAT(0));
    EXPECT_THROW(solver.count(), std::overflow_error);
    EXPECT_EQ(solver.count(), 1u);
    EXPECT_EQ(solver.count(), 1u);
    EXPECT_EQ(solver.calls, 2);
}
} // namespace
