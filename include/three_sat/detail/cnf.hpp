#pragma once
#include "three_sat/three_sat.hpp"
#include <optional>
#include <utility>
#include <vector>

namespace three_sat::detail {
// Internal width-at-most-three CNF. Empty clauses represent false; no clauses means true.
// Declared variables are retained even if normalization removes all their occurrences.
using Clause = std::vector<int>;
struct Cnf {
    std::size_t variable_count;
    std::vector<Clause> clauses;
};

Cnf normalize(Cnf formula);
Cnf normalize(const ThreeSAT& formula);

struct CoreBranch {
    Clause core;
    std::size_t width;
};
// Appendix A.1: smallest clause width first, then largest proper core, then lexical order.
// threshold M >= 2 requires at least M^(width - core.size()) clauses sharing a core.
std::optional<CoreBranch> find_core_branch(const Cnf& normalized, std::size_t threshold);
// Appendix A.1, equation (A.2): (G AND H), (G AND all petals AND negated core literals).
// The two children have disjoint satisfying assignments and preserve their parent's count.
std::pair<Cnf, Cnf> split_core(const Cnf& normalized, const CoreBranch& branch);
} // namespace three_sat::detail
