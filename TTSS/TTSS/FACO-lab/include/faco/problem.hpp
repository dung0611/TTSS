#pragma once
#include <cstdint>
#include <istream>
#include <string>
#include <vector>
namespace faco {
using Cost = std::int64_t;
using Tour = std::vector<int>;
struct Point {
    double x, y;
};
struct Problem {
    std::string name;
    std::vector<Point> points;
    int size() const {
        return static_cast<int>(points.size());
    }
    Cost distance(int a, int b) const;
};
Problem read_tsplib(std::istream &in);
Problem load_tsplib(const std::string &path);
bool valid_tour(const Tour &tour, int n);
Cost tour_length(const Problem &problem, const Tour &tour);
} // namespace faco
