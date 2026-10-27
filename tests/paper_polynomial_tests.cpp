#include "three_sat/detail/paper_polynomial.hpp"
#include <gtest/gtest.h>
#include <stdexcept>
using namespace three_sat::detail;

TEST(PaperPolynomialTest, ShiftsHighPowersBeforeTruncating) {
    // (2+z)^4 = 16+32z+24z^2+8z^3+z^4.
    const Polynomial polynomial{0, 0, 0, 0, 1, 0};
    EXPECT_EQ(polynomial_degree(polynomial), 4u);
    EXPECT_EQ(polynomial_value(polynomial, 2), 16);
    EXPECT_EQ(polynomial_taylor(polynomial, 2, 3).coefficients,
              (std::vector<Rational>{16, 32, 24}));
    EXPECT_EQ(polynomial_taylor({}, -3, 2).coefficients, (std::vector<Rational>{0, 0}));
}

TEST(PaperPolynomialTest, MultipliesExactlyAndDiscardsHighPowers) {
    TaylorPolynomial a(3, Rational(1, 2));
    a.coefficients[1] = 1;
    a.coefficients[2] = 2;
    a *= a;
    EXPECT_EQ(a.coefficients, (std::vector<Rational>{Rational(1, 4), 1, 3}));
    TaylorPolynomial b(3, Rational(1, 4));
    a -= b;
    EXPECT_EQ(a.coefficients[0], 0);
    a += b;
    EXPECT_EQ(a.coefficients[0], Rational(1, 4));
    EXPECT_THROW(a *= TaylorPolynomial(2), std::invalid_argument);
    EXPECT_THROW(TaylorPolynomial(0), std::invalid_argument);
}

TEST(PaperPolynomialTest, RecoversCubicFromTwoValuesAndTwoSlopes) {
    // H(T)=7-3T+2T^2+T^3. Taylor coefficients have no factorial scaling.
    TaylorPolynomial first(2), second(2);
    first.coefficients = {7, 4};
    second.coefficients = {17, 17};
    EXPECT_EQ(hermite_at_zero({1, 2}, {first, second}, 3), 7);
    EXPECT_THROW(hermite_at_zero({1, 1}, {first, second}, 3), std::invalid_argument);
    EXPECT_THROW(hermite_at_zero({1}, {first}, 3), std::invalid_argument);
}

TEST(PaperPolynomialTest, HandlesLargeRationalsAndRejectsInconsistentExtraData) {
    const Rational huge(Integer("123456789012345678901234567890123456789"));
    TaylorPolynomial first(2, huge), second(2, huge);
    EXPECT_EQ(hermite_at_zero({1, 2}, {first, second}, 0), huge);
    second.coefficients[1] = 1;
    EXPECT_THROW(hermite_at_zero({1, 2}, {first, second}, 0), std::invalid_argument);
}
