#include "three_sat/three_sat_reader.hpp"
#include <gtest/gtest.h>
#include <sstream>
#include <stdexcept>

TEST(ThreeSATReaderTest, ReadsFormulaWithoutSolvingIt) {
    std::istringstream input("p 3sat 3 2\n1 2 3\n-1 -2 -3\n");
    const auto formula = ThreeSATReader{}.read(input);
    EXPECT_EQ(formula.variable_count(), 3u);
    ASSERT_EQ(formula.clauses().size(), 2u);
    EXPECT_EQ(formula.clauses()[0], (ThreeSAT::Clause{1, 2, 3}));
    EXPECT_EQ(formula.clauses()[1], (ThreeSAT::Clause{-1, -2, -3}));
}

TEST(ThreeSATReaderTest, AllowsCommentsWhitespaceAndEmptyFormula) {
    std::istringstream input("# comment\r\n p 3sat\t0 0 # empty\n\n");
    const auto formula = ThreeSATReader{}.read(input);
    EXPECT_EQ(formula.variable_count(), 0u);
    EXPECT_TRUE(formula.clauses().empty());
}

TEST(ThreeSATReaderTest, RejectsMalformedInput) {
    const char* invalid[] = {"", "p cnf 1 0", "p 3sat -1 0", "p 3sat 1 -1",
        "p 3sat 2147483648 0", "p 3sat 1 18446744073709551616",
        "p 3sat 1 1 1 1", "p 3sat 1 0 1 1 1", "p 3sat 1 1 0 1 1",
        "p 3sat 1 1 2 1 1", "p 3sat 1 1 -2147483648 1 1",
        "p 3sat 1 1 2147483648 1 1", "p 3sat 1 1 1x 1 1",
        "p 3sat 0 0 count 1"};
    for (const char* text : invalid) {
        SCOPED_TRACE(text);
        std::istringstream input(text);
        EXPECT_THROW(ThreeSATReader{}.read(input), std::invalid_argument);
    }
}

TEST(ThreeSATReaderTest, ReportsStreamFailure) {
    std::istringstream input;
    input.setstate(std::ios::badbit);
    EXPECT_THROW(ThreeSATReader{}.read(input), std::runtime_error);
}
