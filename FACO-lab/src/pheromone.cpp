#include "faco/pheromone.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace faco {
Limits trail_limits(int n, int k, double p_best, double rho, Cost length, bool ls, bool ref) {
    if (n < 2 || k < 1 || k >= n || !(p_best > 0 && p_best < 1) || !(rho > 0 && rho < 1) ||
        length < 0)
        throw std::invalid_argument("Invalid trail-limit parameters");
    const double maximum = 1.0 / (static_cast<double>(std::max<Cost>(length, 1)) * (1 - rho));
    const double avg = ls ? static_cast<double>(k) : n / 2.0;
    const double root = std::pow(p_best, 1.0 / (ls && ref ? k : n));
    double minimum =
        avg <= 1 ? maximum : std::min(maximum, maximum * (1 - root) / ((avg - 1) * root));
    if (!std::isfinite(minimum) || minimum <= 0 || !std::isfinite(maximum) || maximum <= 0)
        throw std::invalid_argument("Non-finite pheromone bounds");
    return {minimum, maximum};
}
Pheromone::Pheromone(const Neighbors &neighbors, Limits bounds)
    : nn(neighbors), outside(bounds.maximum), limits(bounds),
      trails(neighbors.rows.size() * static_cast<std::size_t>(neighbors.candidate_size),
             bounds.maximum) {}
double Pheromone::at(int from, int slot) const {
    return trails[static_cast<std::size_t>(from) * nn.candidate_size + slot];
}
double Pheromone::get(int from, int to) const {
    for (int j = 0; j < nn.candidate_size; ++j)
        if (nn.rows[from][j] == to)
            return at(from, j);
    return outside;
}
void Pheromone::evaporate_slot(std::size_t slot, double rho) {
    trails[slot] *= rho;
}
void Pheromone::clamp_slot(std::size_t slot) {
    trails[slot] = std::clamp(trails[slot], limits.minimum, limits.maximum);
}
void Pheromone::deposit(const Source &s, Cost length, double rho) {
    const double delta = 1.0 / static_cast<double>(std::max<Cost>(length, 1));
    // Update each stored orientation if it exists; no missing reverse arc is allocated.
    for (int u : s.route) {
        const int adjacent[] = {s.successor(u), s.predecessor(u)};
        // n=2 has one distinct undirected edge: reinforce each stored orientation once.
        const int count = adjacent[0] == adjacent[1] ? 1 : 2;
        for (int side = 0; side < count; ++side) {
            const int v = adjacent[side];
            for (int j = 0; j < nn.candidate_size; ++j)
                if (nn.rows[u][j] == v) {
                    trails[static_cast<std::size_t>(u) * nn.candidate_size + j] += delta;
                    break;
                }
        }
    }
    outside = std::clamp(outside * rho, limits.minimum, limits.maximum);
}
bool Pheromone::in_bounds() const {
    auto valid = [&](double v) {
        return std::isfinite(v) && v >= limits.minimum && v <= limits.maximum;
    };
    return valid(outside) && std::all_of(trails.begin(), trails.end(), valid);
}
} // namespace faco
