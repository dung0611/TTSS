#pragma once
#include "neighbors.hpp"
namespace faco {
int two_opt(const Problem &p, const Neighbors &nn, Tour &tour, const std::vector<int> &checklist,
            bool verify = false);
// Initialization only: nearest-neighbor restricted 3-cut reconnections, all 7 nonidentity cases.
int three_opt(const Problem &p, const Neighbors &nn, Tour &tour, bool verify = false);
} // namespace faco
