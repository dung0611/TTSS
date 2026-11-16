#pragma once
#include "problem.hpp"
namespace faco {
// One sorted row: candidate prefix, then backup; LS uses an independent prefix.
struct Neighbors {
    int candidate_size = 0, backup_size = 0, ls_size = 0;
    std::vector<std::vector<int>> rows;
    Neighbors(const Problem &p, int candidates, int backup, int local_search);
};
Tour nearest_neighbor_tour(const Problem &p, const Neighbors &nn, int start);
} // namespace faco
