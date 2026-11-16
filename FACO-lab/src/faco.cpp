#include "faco/faco.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <exception>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <thread>
#ifdef _OPENMP
#include <omp.h>
#endif
namespace faco {
namespace {
using Clock = std::chrono::steady_clock;
double seconds(Clock::time_point start) {
    return std::chrono::duration<double>(Clock::now() - start).count();
}
} // namespace
void validate_config(const Config &c) {
    if (c.ants < 0 || c.iterations < 1 || c.threads < 1 || c.candidate_size < 1 ||
        c.backup_size < 0 || c.min_new_edges < 1 || c.ls_size < 1 || c.initial_solutions < 0 ||
        c.repeat < 1 || c.log_every < 0)
        throw std::invalid_argument("Invalid integer option (ants/initial-solutions: 0=auto; "
                                    "backup-size/log-every: >=0; others: >=1)");
    if (!(c.rho > 0 && c.rho < 1) || !std::isfinite(c.beta) || c.beta < 0 ||
        !(c.p_best > 0 && c.p_best < 1) ||
        !(c.global_best_probability >= 0 && c.global_best_probability <= 1))
        throw std::invalid_argument(
            "Require 0<rho,p-best<1; beta>=0 finite; 0<=gbest-probability<=1");
    if (c.schedule != "static" && c.schedule != "dynamic")
        throw std::invalid_argument("schedule must be static or dynamic");
    if (c.reference_length < 0 ||
        (c.reference_length > 0 && (c.reference_source.empty() || c.reference_date.empty())))
        throw std::invalid_argument(
            "Reference length requires reference-source and reference-date");
#ifndef _OPENMP
    if (c.threads != 1)
        throw std::invalid_argument("This build has no OpenMP: --threads must be 1");
#endif
}
int default_ant_count(int n) {
    return std::max(64, static_cast<int>(std::ceil(4 * std::sqrt(n) / 64)) * 64);
}
std::mt19937_64 ant_rng(std::uint64_t seed, int phase, int iteration, int ant) {
    std::seed_seq seq{static_cast<std::uint32_t>(seed), static_cast<std::uint32_t>(seed >> 32),
                      static_cast<std::uint32_t>(phase), static_cast<std::uint32_t>(iteration),
                      static_cast<std::uint32_t>(ant)};
    return std::mt19937_64(seq);
}
double uniform01(std::mt19937_64 &rng) {
    return static_cast<double>(rng() >> 11) * 0x1.0p-53;
}
int uniform_index(std::mt19937_64 &rng, int n) {
    auto bound = static_cast<std::uint64_t>(n);
    auto threshold = (std::uint64_t(0) - bound) % bound;
    std::uint64_t value;
    do {
        value = rng();
    } while (value < threshold);
    return static_cast<int>(value % bound);
}
int select_next(const Problem &p, const Neighbors &nn, const Pheromone &ph,
                const std::vector<double> &h, int from, const std::vector<unsigned char> &seen,
                std::mt19937_64 &rng) {
    double sum = 0;
    int first = -1, last = -1;
    for (int j = 0; j < nn.candidate_size; ++j)
        if (!seen[nn.rows[from][j]]) {
            if (first < 0)
                first = nn.rows[from][j];
            last = nn.rows[from][j];
            sum += ph.at(from, j) * h[static_cast<std::size_t>(from) * nn.candidate_size + j];
        }
    if (first >= 0) {
        if (!(sum > 0) || !std::isfinite(sum))
            return first;
        const double target = uniform01(rng) * sum;
        double prefix = 0;
        for (int j = 0; j < nn.candidate_size; ++j)
            if (!seen[nn.rows[from][j]]) {
                prefix +=
                    ph.at(from, j) * h[static_cast<std::size_t>(from) * nn.candidate_size + j];
                if (target < prefix)
                    return nn.rows[from][j];
            }
        return last;
    }
    for (int j = nn.candidate_size; j < nn.candidate_size + nn.backup_size; ++j)
        if (!seen[nn.rows[from][j]])
            return nn.rows[from][j];
    int next = -1;
    for (int v = 0; v < p.size(); ++v)
        if (!seen[v] && (next < 0 || std::make_pair(p.distance(from, v), v) <
                                         std::make_pair(p.distance(from, next), next)))
            next = v;
    if (next < 0)
        throw std::logic_error("No unvisited node remains");
    return next;
}
Construction construct(const Problem &p, const Neighbors &nn, const Pheromone &ph,
                       const std::vector<double> &h, const Source &source, int threshold,
                       std::mt19937_64 &rng) {
    Construction a;
    const int n = p.size();
    std::vector<unsigned char> seen(n);
    int start = uniform_index(rng, n);
    a.tour.reserve(n);
    a.tour.push_back(start);
    seen[start] = 1;
    a.checklist.push_back(start);
    while (static_cast<int>(a.tour.size()) < n) {
        int u = a.tour.back(), v = select_next(p, nn, ph, h, u, seen, rng);
        a.tour.push_back(v);
        seen[v] = 1;
        ++a.selected_edges;
        if (!source.contains_edge(u, v)) {
            ++a.new_edges;
            a.checklist.push_back(v);
        }
        if (a.new_edges >= threshold) {
            const int before = static_cast<int>(a.tour.size());
            copy_source(source, v, a.tour, seen);
            a.copied_vertices += static_cast<int>(a.tour.size()) - before;
        }
    }
    return a;
}
Result solve(const Problem &p, Config c, const ProgressCallback &progress) {
    validate_config(c);
    if (p.size() < 2)
        throw std::invalid_argument("At least two cities required");
    if (c.ants == 0)
        c.ants = default_ant_count(p.size());
    if (c.initial_solutions == 0)
        c.initial_solutions = static_cast<int>(std::max(1u, std::thread::hardware_concurrency()));
    Result out;
    out.config = c;
    out.initial_count = c.initial_solutions;
#ifdef _OPENMP
    omp_set_dynamic(0);
    omp_set_num_threads(c.threads);
    omp_set_schedule(c.schedule == "dynamic" ? omp_sched_dynamic : omp_sched_static, 1);
#pragma omp parallel
    {
#pragma omp single
        out.actual_threads = omp_get_num_threads();
    }
#endif
    auto total = Clock::now(), stage = total;
    Neighbors nn(p, c.candidate_size, c.backup_size, c.ls_size);
    out.config.candidate_size = nn.candidate_size;
    out.config.backup_size = nn.backup_size;
    out.config.ls_size = nn.ls_size;
    std::vector<double> h(static_cast<std::size_t>(p.size()) * nn.candidate_size);
    for (int u = 0; u < p.size(); ++u)
        for (int j = 0; j < nn.candidate_size; ++j)
            h[static_cast<std::size_t>(u) * nn.candidate_size + j] = std::pow(
                1.0 / static_cast<double>(std::max<Cost>(1, p.distance(u, nn.rows[u][j]))), c.beta);
    out.preprocessing_seconds = seconds(stage);
    stage = Clock::now();
    Tour all(p.size());
    std::iota(all.begin(), all.end(), 0);
    Cost best = std::numeric_limits<Cost>::max();
    Tour best_tour;
    // Fixed initialization population independent of --threads. Serial by design.
    for (int i = 0; i < c.initial_solutions; ++i) {
        auto rng = ant_rng(c.seed, 0, 0, i);
        Tour t = nearest_neighbor_tour(p, nn, uniform_index(rng, p.size()));
        if (c.local_search) {
            two_opt(p, nn, t, all, c.verify);
            three_opt(p, nn, t, c.verify);
        }
        Cost length = tour_length(p, t);
        if (length < best) {
            best = length;
            best_tour = std::move(t);
        }
    }
    Source source(best_tour);
    Pheromone ph(nn, trail_limits(p.size(), nn.candidate_size, c.p_best, c.rho, best,
                                  c.local_search, c.reference_bounds));
    std::vector<Tour> tours(c.ants);
    std::vector<Cost> costs(c.ants);
    out.history.reserve(static_cast<std::size_t>(c.iterations) + 1);
    out.history.push_back({0, best, best});
    out.initialization_seconds = seconds(stage);
    stage = Clock::now();
    if (progress && c.log_every > 0)
        progress(out.history.back(), 0.0);
    // One OpenMP team executes all iterations. Without OpenMP this same block is serial.
    out.parallel_profile.enabled = c.profile;
    if (c.profile)
        out.parallel_profile.workers.resize(c.threads);
    const auto slots = static_cast<std::int64_t>(ph.trails.size());
    Cost source_length = best;
    Cost iteration_best_cost = best;
    auto phase_start = Clock::now();
    std::atomic<bool> failed{false};
    bool stop_after_iteration = false;
    std::exception_ptr error;
    auto record_failure = [&]() {
#ifdef _OPENMP
#pragma omp critical(faco_failure)
#endif
        {
            if (!error)
                error = std::current_exception();
        }
        failed.store(true);
    };
#ifdef _OPENMP
#pragma omp parallel default(none)                                                                 \
    shared(p, c, nn, ph, h, source, tours, costs, best, best_tour, out, progress, stage, slots,    \
               source_length, iteration_best_cost, phase_start, failed, error, record_failure,     \
               stop_after_iteration)
#endif
    {
        WorkerProfile worker;
#ifdef _OPENMP
        worker.thread = omp_get_thread_num();
#pragma omp single
        out.actual_threads = omp_get_num_threads();
#endif
        for (int iteration = 0; iteration < c.iterations; ++iteration) {
            // Barrier A: finish all ant tours before selecting the best one.
#ifdef _OPENMP
#pragma omp for schedule(runtime)
#endif
            for (int ant = 0; ant < c.ants; ++ant) {
                if (failed.load())
                    continue;
                try {
                    Clock::time_point started{};
                    if (c.profile)
                        started = Clock::now();
                    auto rng = ant_rng(c.seed, 1, iteration, ant);
                    auto a = construct(p, nn, ph, h, source, c.min_new_edges, rng);
                    const int moves =
                        c.local_search ? two_opt(p, nn, a.tour, a.checklist, c.verify) : 0;
                    costs[ant] = tour_length(p, a.tour);
                    if (c.profile) {
                        ++worker.ants_completed;
                        worker.aco_selected_edges += a.selected_edges;
                        worker.copied_vertices += a.copied_vertices;
                        worker.ls_moves += moves;
                    }
                    tours[ant] = std::move(a.tour);
                    if (c.profile)
                        worker.ant_work_seconds += seconds(started);
                } catch (...) {
                    record_failure();
                }
            }
            // The single's barrier publishes the new source and trail bounds.
#ifdef _OPENMP
#pragma omp single
#endif
            {
                Clock::time_point started{};
                if (c.profile) {
                    out.parallel_profile.ant_phase_seconds += seconds(phase_start);
                    started = Clock::now();
                }
                if (!failed.load()) {
                    try {
                        const int winner = static_cast<int>(
                            std::min_element(costs.begin(), costs.end()) - costs.begin());
                        if (costs[winner] < best) {
                            best = costs[winner];
                            best_tour = tours[winner];
                            ph.limits = trail_limits(p.size(), nn.candidate_size, c.p_best, c.rho,
                                                     best, c.local_search, c.reference_bounds);
                        }
                        auto source_rng = ant_rng(c.seed, 2, iteration, 0);
                        const bool global = uniform01(source_rng) < c.global_best_probability;
                        source = Source(global ? best_tour : tours[winner]);
                        source_length = global ? best : costs[winner];
                        iteration_best_cost = costs[winner];
                    } catch (...) {
                        record_failure();
                    }
                }
                if (c.profile) {
                    out.parallel_profile.selection_seconds += seconds(started);
                    phase_start = Clock::now();
                }
            }
            // Barrier B: evaporation must finish before serial deposition.
            const bool skip_evaporation = failed.load();
#ifdef _OPENMP
#pragma omp for schedule(static)
#endif
            for (std::int64_t slot = 0; slot < slots; ++slot)
                if (!skip_evaporation)
                    ph.evaporate_slot(static_cast<std::size_t>(slot), c.rho);
#ifdef _OPENMP
#pragma omp single
#endif
            {
                Clock::time_point started{};
                if (c.profile) {
                    out.parallel_profile.evaporation_seconds += seconds(phase_start);
                    started = Clock::now();
                }
                if (!failed.load()) {
                    try {
                        ph.deposit(source, source_length, c.rho);
                    } catch (...) {
                        record_failure();
                    }
                }
                if (c.profile) {
                    out.parallel_profile.deposit_seconds += seconds(started);
                    phase_start = Clock::now();
                }
            }
            // Barrier C: clamp every slot before any ant reads trails again.
            const bool skip_clamping = failed.load();
#ifdef _OPENMP
#pragma omp for schedule(static)
#endif
            for (std::int64_t slot = 0; slot < slots; ++slot)
                if (!skip_clamping)
                    ph.clamp_slot(static_cast<std::size_t>(slot));
#ifdef _OPENMP
#pragma omp single
#endif
            {
                Clock::time_point started{};
                if (c.profile) {
                    out.parallel_profile.clamp_seconds += seconds(phase_start);
                    started = Clock::now();
                }
                if (!failed.load()) {
                    try {
                        if (c.verify && !ph.in_bounds())
                            throw std::logic_error("Pheromone invariant failed");
                        out.history.push_back({iteration + 1, best, iteration_best_cost});
                        const int completed = iteration + 1;
                        if (progress && c.log_every > 0 &&
                            (completed % c.log_every == 0 || completed == c.iterations))
                            progress(out.history.back(), seconds(stage));
                    } catch (...) {
                        record_failure();
                    }
                }
                if (c.profile) {
                    out.parallel_profile.bookkeeping_seconds += seconds(started);
                    phase_start = Clock::now();
                }
                stop_after_iteration = failed.load();
            }
            // All threads encounter all barriers even on failure; rethrow after joining.
            if (stop_after_iteration)
                break;
        }
        if (c.profile)
            out.parallel_profile.workers[worker.thread] = worker;
    }
    if (error)
        std::rethrow_exception(error);
    if (c.profile)
        out.parallel_profile.workers.resize(out.actual_threads);

    out.search_seconds = seconds(stage);
    out.total_seconds = seconds(total);
    out.tour = std::move(best_tour);
    out.best_length = best;
    if (tour_length(p, out.tour) != best)
        throw std::logic_error("Final length mismatch");
    return out;
}
} // namespace faco
