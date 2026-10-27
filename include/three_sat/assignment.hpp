#pragma once
#include <cstddef>
#include <vector>

/// A complete truth assignment for a fixed number of Boolean variables.
/// Public variable IDs are 1-based; every value initially equals false.
/// Copying an Assignment creates independent values. Zero variables are valid.
class Assignment {
public:
    explicit Assignment(std::size_t variable_count);
    std::size_t variable_count() const noexcept;
    /// Set/read a variable. Throws std::out_of_range for IDs outside [1, n].
    void set(std::size_t variable, bool value);
    bool get(std::size_t variable) const;
private:
    // Validate before subtracting 1: size_t would underflow for variable == 0.
    void check_variable(std::size_t variable) const;
    std::vector<bool> values_;
};
