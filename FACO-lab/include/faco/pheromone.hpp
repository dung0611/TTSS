#pragma once
#include "neighbors.hpp"
#include "tour.hpp"
namespace faco {
struct Limits {
    double minimum, maximum;
};
Limits trail_limits(int n, int k, double p_best, double rho, Cost length, bool ls,
                    bool reference_exponent);
class Pheromone {
    const Neighbors &nn;
    double outside;

  public:
    Limits limits;
    std::vector<double> trails;
    Pheromone(const Neighbors &neighbors, Limits bounds);
    double get(int from, int to) const;
    double at(int from, int slot) const;
    // Intermediate values may be below tau_min; clamp AFTER deposit to implement Eq. (2).
    void evaporate_slot(std::size_t slot, double rho);
    void deposit(const Source &source, Cost length, double rho);
    void clamp_slot(std::size_t slot);
    bool in_bounds() const;
};
} // namespace faco
