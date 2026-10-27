#include "three_sat/assignment.hpp"
#include <stdexcept>

// A fixed-size vector makes the declared variable domain independent of later writes.
Assignment::Assignment(std::size_t variable_count) : values_(variable_count, false) {}
std::size_t Assignment::variable_count() const noexcept { return values_.size(); }
void Assignment::set(std::size_t variable, bool value) {
    check_variable(variable);
    // Translate the public 1-based ID only after checking its bounds.
    values_[variable - 1] = value;
}
bool Assignment::get(std::size_t variable) const {
    check_variable(variable);
    return values_[variable - 1];
}
void Assignment::check_variable(std::size_t variable) const {
    // Both readers and writers share this guard, including the zero-variable case.
    if (variable == 0 || variable > values_.size()) {
        throw std::out_of_range("Variable must be in [1, variable_count].");
    }
}
