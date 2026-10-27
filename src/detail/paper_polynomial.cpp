#include "three_sat/detail/paper_polynomial.hpp"
#include <algorithm>
#include <set>
#include <stdexcept>

namespace three_sat::detail {
std::size_t polynomial_degree(const Polynomial& polynomial) {
    auto size = polynomial.size();
    while (size > 1 && polynomial[size - 1] == 0) --size;
    return size ? size - 1 : 0;
}
Rational polynomial_value(const Polynomial& polynomial, const Rational& point) {
    Rational value = 0;
    for (auto it = polynomial.rbegin(); it != polynomial.rend(); ++it) value = value * point + *it;
    return value;
}
TaylorPolynomial polynomial_taylor(const Polynomial& polynomial, const Rational& point,
                                   std::size_t precision) {
    TaylorPolynomial result(precision), argument(precision, point);
    if (precision > 1) argument.coefficients[1] = 1;
    // Horner over the truncated ring includes all original coefficients.
    for (auto it = polynomial.rbegin(); it != polynomial.rend(); ++it) {
        result *= argument;
        result.coefficients[0] += *it;
    }
    return result;
}
TaylorPolynomial::TaylorPolynomial(std::size_t precision, Rational constant)
    : coefficients(precision) {
    if (!precision) throw std::invalid_argument("Taylor precision must be positive.");
    coefficients[0] = std::move(constant);
}
static void same_precision(const TaylorPolynomial& a, const TaylorPolynomial& b) {
    if (a.coefficients.size() != b.coefficients.size())
        throw std::invalid_argument("Taylor precisions differ.");
}
TaylorPolynomial& TaylorPolynomial::operator+=(const TaylorPolynomial& other) {
    same_precision(*this, other);
    for (std::size_t i = 0; i < coefficients.size(); ++i) coefficients[i] += other.coefficients[i];
    return *this;
}
TaylorPolynomial& TaylorPolynomial::operator-=(const TaylorPolynomial& other) {
    same_precision(*this, other);
    for (std::size_t i = 0; i < coefficients.size(); ++i) coefficients[i] -= other.coefficients[i];
    return *this;
}
TaylorPolynomial& TaylorPolynomial::operator*=(const TaylorPolynomial& other) {
    same_precision(*this, other);
    std::vector<Rational> product(coefficients.size());
    for (std::size_t i = 0; i < product.size(); ++i)
        for (std::size_t j = 0; i + j < product.size(); ++j)
            product[i + j] += coefficients[i] * other.coefficients[j];
    coefficients = std::move(product);
    return *this;
}

Rational hermite_at_zero(const std::vector<int>& points,
                         const std::vector<TaylorPolynomial>& jets, std::size_t degree_bound) {
    if (points.size() != jets.size() || points.empty() ||
        std::set<int>(points.begin(), points.end()).size() != points.size())
        throw std::invalid_argument("Hermite points must be distinct and match the jets.");
    const auto columns = degree_bound + 1;
    std::vector<std::vector<Rational>> equations;
    for (std::size_t p = 0; p < points.size(); ++p) {
        for (std::size_t j = 0; j < jets[p].coefficients.size(); ++j) {
            std::vector<Rational> row(columns + 1);
            Integer binomial = 1;
            Rational power = 1;
            for (std::size_t k = j; k < columns; ++k) {
                row[k] = Rational(binomial) * power;
                // binom(k+1,j) / binom(k,j) = (k+1)/(k+1-j).
                binomial *= Integer(std::to_string(k + 1));
                binomial /= Integer(std::to_string(k + 1 - j));
                power *= points[p];
            }
            row.back() = jets[p].coefficients[j];
            equations.push_back(std::move(row));
        }
    }
    if (equations.size() < columns) throw std::invalid_argument("Insufficient Hermite data.");
    // Exact elimination is deliberately simple. This can later be replaced with a faster
    // confluent interpolation algorithm without changing the counter's data flow.
    for (std::size_t c = 0; c < columns; ++c) {
        auto pivot = c;
        while (pivot < equations.size() && equations[pivot][c] == 0) ++pivot;
        if (pivot == equations.size()) throw std::invalid_argument("Singular Hermite data.");
        std::swap(equations[c], equations[pivot]);
        const Rational divisor = equations[c][c];
        for (std::size_t k = c; k <= columns; ++k) equations[c][k] /= divisor;
        for (std::size_t r = 0; r < equations.size(); ++r) if (r != c) {
            const Rational factor = equations[r][c];
            if (factor == 0) continue;
            for (std::size_t k = c; k <= columns; ++k) equations[r][k] -= factor * equations[c][k];
        }
    }
    for (std::size_t r = columns; r < equations.size(); ++r)
        if (equations[r].back() != 0) throw std::invalid_argument("Inconsistent Hermite data.");
    return equations[0].back();
}
} // namespace three_sat::detail
