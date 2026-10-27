#pragma once
#include <gmpxx.h>
#include <vector>

namespace three_sat::detail {
using Rational = mpq_class;
using Integer = mpz_class;
// Ordinary polynomial in T, stored from constant to highest coefficient.
using Polynomial = std::vector<Rational>;
std::size_t polynomial_degree(const Polynomial& polynomial);
Rational polynomial_value(const Polynomial& polynomial, const Rational& point);

// An element of Q[z]/(z^J). Coefficients are Taylor coefficients, not derivatives.
// Keeping exactly J entries makes truncation explicit at every multiplication.
class TaylorPolynomial {
public:
    explicit TaylorPolynomial(std::size_t precision, Rational constant = 0);
    std::vector<Rational> coefficients;
    TaylorPolynomial& operator+=(const TaylorPolynomial& other);
    TaylorPolynomial& operator-=(const TaylorPolynomial& other);
    TaylorPolynomial& operator*=(const TaylorPolynomial& other);
};

// Exact binomial shift P(t+z), truncated only after translation. In particular,
// high powers of T can contribute to low Taylor coefficients at nonzero t.
TaylorPolynomial polynomial_taylor(const Polynomial& polynomial, const Rational& point,
                                   std::size_t precision);

// Recover H(0) from H(t+z) mod z^J for distinct integer points. The degree bound
// is on H, rather than on individual factors. Extra equations are checked too.
Rational hermite_at_zero(const std::vector<int>& points,
                         const std::vector<TaylorPolynomial>& jets,
                         std::size_t degree_bound);
} // namespace three_sat::detail
