#pragma once
#include "three_sat/three_sat.hpp"
#include <istream>

/// Reads one complete formula from a stream, without solving it.
/// Format: p 3sat N M, followed by exactly M triples of signed literals.
/// Whitespace and # comments are allowed. This is not DIMACS (no trailing zeros).
/// Reads through EOF; interactive callers must signal end-of-input.
/// Throws std::invalid_argument for malformed input, and std::runtime_error for I/O failure.
class ThreeSATReader {
public:
    ThreeSAT read(std::istream& input) const;
};
