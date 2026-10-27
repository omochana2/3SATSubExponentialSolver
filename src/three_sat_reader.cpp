#include "three_sat/three_sat_reader.hpp"
#include <charconv>
#include <cstdint>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

ThreeSAT ThreeSATReader::read(std::istream& input) const {
    // Strip comments before tokenization. Keep parsing independent of physical line breaks.
    std::string text, line;
    while (std::getline(input, line)) text += line.substr(0, line.find('#')) + '\n';
    if (input.bad() || (input.fail() && !input.eof())) {
        throw std::runtime_error("Failed to read formula stream");
    }
    std::istringstream tokens(text);
    auto next = [&]() {
        std::string token;
        if (!(tokens >> token)) throw std::invalid_argument("Unexpected end of formula");
        return token;
    };
    auto expect = [&](const char* keyword) {
        if (next() != keyword) throw std::invalid_argument(std::string("Expected: ") + keyword);
    };
    auto natural = [&]() {
        const auto token = next();
        std::uint64_t value = 0;
        const auto result = std::from_chars(token.data(), token.data() + token.size(), value);
        if (result.ec != std::errc{} || result.ptr != token.data() + token.size()) {
            throw std::invalid_argument("Invalid unsigned integer: " + token);
        }
        return value;
    };
    expect("p");
    expect("3sat");
    const auto variables = natural();
    const auto clauses = natural();
    // Validate before narrowing from the serialized unsigned value.
    if (variables > static_cast<std::uint64_t>(std::numeric_limits<int>::max())) {
        throw std::invalid_argument("Variable count exceeds supported literal IDs");
    }
    ThreeSAT formula(static_cast<std::size_t>(variables));
    for (std::uint64_t i = 0; i < clauses; ++i) {
        ThreeSAT::Clause clause{};
        for (int& literal : clause) {
            const auto token = next();
            const auto result = std::from_chars(token.data(), token.data() + token.size(), literal);
            if (result.ec != std::errc{} || result.ptr != token.data() + token.size()) {
                throw std::invalid_argument("Invalid literal: " + token);
            }
        }
        formula.add_clause(clause); // Domain validation stays in ThreeSAT.
    }
    std::string extra;
    if (tokens >> extra) throw std::invalid_argument("Unexpected trailing token: " + extra);
    return formula;
}
