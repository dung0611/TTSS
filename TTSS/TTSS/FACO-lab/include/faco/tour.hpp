#pragma once
#include "problem.hpp"
namespace faco {
struct Source {
    Tour route;
    std::vector<int> position;
    explicit Source(const Tour &t);
    int successor(int v) const;
    int predecessor(int v) const;
    bool contains_edge(int a, int b) const;
};
// Forward until blocked; undo that failed successor; then walk backward.
void copy_source(const Source &source, int from, Tour &route, std::vector<unsigned char> &visited);
} // namespace faco
