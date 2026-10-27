#include "three_sat/detail/cnf.hpp"
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <limits>
#include <map>
#include <stdexcept>

namespace three_sat::detail {
namespace {
bool contains(const Clause& clause, const Clause& subset) {
    return std::includes(clause.begin(), clause.end(), subset.begin(), subset.end());
}
} // namespace

Cnf normalize(Cnf formula) {
    if (formula.variable_count > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::invalid_argument("Too many CNF variables");
    }
    std::vector<Clause> canonical;
    for (auto clause : formula.clauses) {
        if (clause.size() > 3) throw std::invalid_argument("CNF width exceeds three");
        for (int literal : clause) {
            const auto wide = static_cast<std::int64_t>(literal);
            const auto id = static_cast<std::uint64_t>(wide < 0 ? -wide : wide);
            if (!id || id > formula.variable_count) throw std::invalid_argument("Invalid CNF literal");
        }
        std::sort(clause.begin(), clause.end());
        clause.erase(std::unique(clause.begin(), clause.end()), clause.end());
        bool tautology = false;
        for (int literal : clause) {
            if (std::binary_search(clause.begin(), clause.end(), -literal)) tautology = true;
        }
        if (!tautology) canonical.push_back(std::move(clause));
    }
    // Smaller clauses are processed first. Any superset is redundant in a conjunction.
    std::sort(canonical.begin(), canonical.end(), [](const Clause& a, const Clause& b) {
        return a.size() != b.size() ? a.size() < b.size() : a < b;
    });
    formula.clauses.clear();
    for (const auto& clause : canonical) {
        bool redundant = false;
        for (const auto& kept : formula.clauses) {
            if (contains(clause, kept)) { redundant = true; break; }
        }
        if (!redundant) formula.clauses.push_back(clause);
    }
    return formula;
}

Cnf normalize(const ThreeSAT& formula) {
    Cnf result{formula.variable_count(), {}};
    for (const auto& clause : formula.clauses()) result.clauses.emplace_back(clause.begin(), clause.end());
    return normalize(std::move(result));
}

std::optional<CoreBranch> find_core_branch(const Cnf& formula, std::size_t threshold) {
    if (threshold < 2) throw std::invalid_argument("Core threshold must be at least two");
    for (std::size_t width = 2; width <= 3; ++width) {
        for (std::size_t size = width - 1; size > 0; --size) {
            std::map<Clause, std::size_t> frequencies;
            for (const auto& clause : formula.clauses) {
                if (clause.size() != width) continue;
                for (unsigned mask = 1; mask < (1u << width) - 1; ++mask) {
                    Clause core;
                    for (std::size_t i = 0; i < width; ++i) if (mask & (1u << i)) core.push_back(clause[i]);
                    if (core.size() == size) ++frequencies[core];
                }
            }
            std::size_t required = 1;
            bool too_large = false;
            for (std::size_t j = 0; j < width - size; ++j) {
                // No core can occur more often than the number of clauses; also avoids overflow.
                if (required > formula.clauses.size() / threshold) { too_large = true; break; }
                required *= threshold;
            }
            if (too_large) continue;
            for (const auto& [core, count] : frequencies) {
                if (count >= required) return CoreBranch{core, width};
            }
        }
    }
    return std::nullopt;
}

std::pair<Cnf, Cnf> split_core(const Cnf& formula, const CoreBranch& branch) {
    if (branch.core.empty() || branch.core.size() >= branch.width || branch.width > 3 ||
        !std::is_sorted(branch.core.begin(), branch.core.end()) ||
        std::adjacent_find(branch.core.begin(), branch.core.end()) != branch.core.end()) {
        throw std::invalid_argument("Invalid core branch");
    }
    Cnf yes{formula.variable_count, {}}, no{formula.variable_count, {}};
    bool found = false;
    for (const auto& clause : formula.clauses) {
        if (clause.size() == branch.width && contains(clause, branch.core)) {
            found = true;
            Clause petal;
            std::set_difference(clause.begin(), clause.end(), branch.core.begin(), branch.core.end(),
                                std::back_inserter(petal));
            no.clauses.push_back(std::move(petal));
        } else {
            yes.clauses.push_back(clause);
            no.clauses.push_back(clause);
        }
    }
    if (!found) throw std::invalid_argument("Core does not occur in any selected clause");
    yes.clauses.push_back(branch.core);
    for (int literal : branch.core) no.clauses.push_back({-literal});
    return {normalize(std::move(yes)), normalize(std::move(no))};
}
} // namespace three_sat::detail
