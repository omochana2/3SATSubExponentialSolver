#include "three_sat/three_sat_reader.hpp"
#include <gtest/gtest.h>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

// Validate the actual executable's witness, without solving the input again and
// without storing a second expected witness in the fixture. The fixed count file
// determines whether a witness should exist; evaluating one assignment is linear.
TEST(CliCaseFileTest, ReturnedWitnessMatchesTheInputAndExpectedCount) {
    const char* input_path = std::getenv("THREE_SAT_INPUT");
    const char* expected_path = std::getenv("THREE_SAT_EXPECTED");
    const char* actual = std::getenv("THREE_SAT_CLI_OUTPUT");
    ASSERT_NE(input_path, nullptr);
    ASSERT_NE(expected_path, nullptr);
    ASSERT_NE(actual, nullptr);
    SCOPED_TRACE(input_path);
    std::ifstream input(input_path), expected_file(expected_path);
    ASSERT_TRUE(input.is_open());
    ASSERT_TRUE(expected_file.is_open());
    const auto formula = ThreeSATReader{}.read(input);
    std::string label, expected_count, actual_count;
    ASSERT_TRUE(static_cast<bool>(expected_file >> label >> expected_count));
    ASSERT_EQ(label, "count");
    std::istringstream output(actual);
    ASSERT_TRUE(static_cast<bool>(output >> label >> actual_count));
    ASSERT_EQ(label, "count");
    ASSERT_EQ(actual_count, expected_count);
    ASSERT_TRUE(static_cast<bool>(output >> label));
    ASSERT_EQ(label, "witness");
    if (expected_count == "0") {
        ASSERT_TRUE(static_cast<bool>(output >> label));
        ASSERT_EQ(label, "none");
    } else {
        Assignment witness(formula.variable_count());
        for (std::size_t v = 1; v <= formula.variable_count(); ++v) {
            ASSERT_TRUE(static_cast<bool>(output >> label)) << "Missing bit " << v;
            ASSERT_TRUE(label == "0" || label == "1");
            witness.set(v, label == "1");
        }
        EXPECT_TRUE(formula.is_satisfied_by(witness));
    }
    EXPECT_FALSE(static_cast<bool>(output >> label)) << "Unexpected trailing output";
}
