#include "three_sat/three_sat_reader.hpp"
#include "three_sat/three_sat_subexponential_solver.hpp"
#include <exception>
#include <iostream>

int main() {
    try {
        const auto formula = ThreeSATReader{}.read(std::cin);
        ThreeSatSubexponentialSolver solver(formula);
        // Complete computation before writing stdout, so errors do not leave partial answers.
        const auto count = solver.count();
        const auto witness = solver.get_satisfiable();
        std::cout << "count " << count << '\n' << "witness";
        if (witness) {
            for (std::size_t variable = 1; variable <= witness->variable_count(); ++variable) {
                std::cout << ' ' << (witness->get(variable) ? 1 : 0);
            }
        } else {
            std::cout << " none";
        }
        std::cout << '\n';
        return 0; // UNSAT is a valid result, not a program error.
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}

