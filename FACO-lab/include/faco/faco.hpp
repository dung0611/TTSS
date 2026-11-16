#pragma once
#include "local_search.hpp"
#include "pheromone.hpp"
#include <functional>
#include <random>
namespace faco {
struct Config {
    int ants = 0, iterations = 5000, threads = 1, candidate_size = 16, backup_size = 64,
        min_new_edges = 8, ls_size = 20, initial_solutions = 0, repeat = 1, log_every = 4;
    std::uint64_t seed = 1;
    double rho = 0.5, beta = 1, p_best = 0.1, global_best_probability = 0.01;
    bool local_search = true, reference_bounds = false, verify = false, profile = false;
    std::string schedule = "dynamic", input, output_dir = "results", reference_source,
                reference_date;
    Cost reference_length = 0;
};
void validate_config(const Config &c);
int default_ant_count(int n);
std::mt19937_64 ant_rng(std::uint64_t seed, int phase, int iteration, int ant);
double uniform01(std::mt19937_64 &rng);
int uniform_index(std::mt19937_64 &rng, int n);
struct Construction {
    Tour tour;
    std::vector<int> checklist;
    int selected_edges = 0, new_edges = 0, copied_vertices = 0;
};
int select_next(const Problem &p, const Neighbors &nn, const Pheromone &pheromone,
                const std::vector<double> &heuristic, int from,
                const std::vector<unsigned char> &visited, std::mt19937_64 &rng);
Construction construct(const Problem &p, const Neighbors &nn, const Pheromone &ph,
                       const std::vector<double> &heuristic, const Source &source, int threshold,
                       std::mt19937_64 &rng);
struct History {
    int iteration;
    Cost best, iteration_best;
};
struct WorkerProfile {
    int thread = 0;
    std::uint64_t ants_completed = 0, aco_selected_edges = 0, copied_vertices = 0, ls_moves = 0;
    double ant_work_seconds = 0;
};
struct ParallelProfile {
    bool enabled = false;
    double ant_phase_seconds = 0, selection_seconds = 0, evaporation_seconds = 0,
           deposit_seconds = 0, clamp_seconds = 0, bookkeeping_seconds = 0;
    std::vector<WorkerProfile> workers;
};
struct Result {
    Tour tour;
    Cost best_length = 0;
    Config config;
    int actual_threads = 1, initial_count = 0;
    double preprocessing_seconds = 0, initialization_seconds = 0, search_seconds = 0,
           total_seconds = 0;
    std::vector<History> history;
    ParallelProfile parallel_profile;
};
using ProgressCallback = std::function<void(const History &, double)>;
Result solve(const Problem &p, Config c, const ProgressCallback &progress = {});
Config parse_options(int argc, char **argv);
void print_help();
void save_result(const Problem &p, const Result &r, int repeat_index);
} // namespace faco
