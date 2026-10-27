#include "three_sat/three_sat_naive_solver.hpp"
#include <iostream>

int main() {
    ThreeSAT formula(3);
    // (x1 OR x2 OR x3) AND (!x1 OR !x2 OR !x3)
    formula.add_clause({1, 2, 3});
    formula.add_clause({-1, -2, -3});

    Assignment assignment(3);
    assignment.set(1, true);
    assignment.set(2, false);
    assignment.set(3, false);
    std::cout << "Assignment satisfies formula: " << std::boolalpha
              << formula.is_satisfied_by(assignment) << '\n';

    ThreeSatNaiveSolver counter(formula);
    std::cout << "Satisfying assignments: " << counter.count() << '\n';
    if (const auto solution = counter.get_satisfiable()) {
        for (std::size_t variable = 1; variable <= solution->variable_count(); ++variable) {
            std::cout << "x" << variable << " = " << solution->get(variable) << '\n';
        }
    } else {
        std::cout << "UNSAT\n";
    }
}

