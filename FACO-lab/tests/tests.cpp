#include "faco/faco.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stdexcept>
using namespace faco;
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x))                                                                                  \
            throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) +      \
                                     " " #x);                                                      \
    } while (false)
template <class F> void rejects(F f) {
    bool caught = false;
    try {
        f();
    } catch (const std::exception &) {
        caught = true;
    }
    CHECK(caught);
}
Problem square() {
    return {"square", {{0, 0}, {10, 0}, {10, 10}, {0, 10}}};
}
Problem random_problem(int n, int seed) {
    auto rng = ant_rng(seed, 7, 0, 0);
    Problem p{"random", {}};
    for (int i = 0; i < n; ++i)
        p.points.push_back({static_cast<double>(uniform_index(rng, 200)),
                            static_cast<double>(uniform_index(rng, 200))});
    return p;
}
void parser() {
    auto p = load_tsplib(std::string(FACO_DATA_DIR) + "/square4.tsp");
    CHECK(p.size() == 4);
    CHECK(p.distance(0, 2) == 14);
    Problem d{"round", {{0, 0}, {0.49, 0}, {0.5, 0}, {1.49, 0}, {1.5, 0}}};
    CHECK(d.distance(0, 1) == 0);
    CHECK(d.distance(0, 2) == 1);
    CHECK(d.distance(0, 3) == 1);
    CHECK(d.distance(0, 4) == 2);
    auto parse = [](const std::string &s) {
        std::istringstream in(s);
        return read_tsplib(in);
    };
    const std::string prefix =
        "TYPE:TSP\nDIMENSION:2\nEDGE_WEIGHT_TYPE:EUC_2D\nNODE_COORD_SECTION\n";
    auto order = parse(prefix + "2 3 4\n1 0 0\nEOF\n");
    CHECK(order.distance(0, 1) == 5);
    for (const auto &tail :
         {"1 0 0\n1 3 4\nEOF", "1 0 0\nEOF", "0 0 0\n2 3 4\nEOF", "1 0 0\n2 nan 0\nEOF",
          "1 0 0\n2 3 4 5\nEOF", "1 0 0\n2 3 4\nDISPLAY_DATA_SECTION"})
        rejects([&] { parse(prefix + tail); });
    rejects([&] {
        parse(
            "TYPE:ATSP\nDIMENSION:2\nEDGE_WEIGHT_TYPE:EUC_2D\nNODE_COORD_SECTION\n1 0 0\n2 3 4\n");
    });
    rejects([&] {
        parse("TYPE:TSP\nDIMENSION:2\nEDGE_WEIGHT_TYPE:ATT\nNODE_COORD_SECTION\n1 0 0\n2 3 4\n");
    });
}
void tours() {
    auto p = square();
    CHECK(tour_length(p, {0, 1, 2, 3}) == 40);
    CHECK(tour_length(p, {0, 2, 1, 3}) == 48);
    CHECK(!valid_tour({0, 1, 1, 3}, 4));
    CHECK(!valid_tour({0, 1, 2}, 4));
    CHECK(!valid_tour({0, 1, 2, 4}, 4));
    rejects([&] { tour_length(p, {0, 1}); });
    Source s({0, 1, 2, 3});
    CHECK(s.contains_edge(0, 1));
    CHECK(s.contains_edge(1, 0));
    CHECK(s.contains_edge(0, 3));
    CHECK(!s.contains_edge(0, 2));
    // Exhaustive optimum is only a quality reference; solver never calls this enumeration.
    Tour t{0, 1, 2, 3};
    Cost optimum = 1000;
    do {
        optimum = std::min(optimum, tour_length(p, t));
    } while (std::next_permutation(t.begin() + 1, t.end()));
    CHECK(optimum == 40);
}
void neighbors() {
    for (int seed = 0; seed < 8; ++seed) {
        auto p = random_problem(80, seed);
        p.points[1] = p.points[0];
        p.points[2] = {p.points[0].x + 0.4, p.points[0].y};
        Neighbors nn(p, 16, 30, 20);
        for (int u = 0; u < p.size(); ++u) {
            std::vector<std::pair<Cost, int>> brute;
            for (int v = 0; v < p.size(); ++v)
                if (v != u)
                    brute.push_back({p.distance(u, v), v});
            std::sort(brute.begin(), brute.end());
            for (std::size_t j = 0; j < nn.rows[u].size(); ++j)
                CHECK(nn.rows[u][j] == brute[j].second);
        }
        CHECK(valid_tour(nearest_neighbor_tour(p, nn, 0), p.size()));
    }
    Neighbors small(square(), 16, 64, 20);
    CHECK(small.candidate_size == 3);
    CHECK(small.backup_size == 0);
    CHECK(small.ls_size == 3);
}
void copying() {
    Source s({0, 1, 2, 3, 4, 5});
    Tour a{0, 2};
    std::vector<unsigned char> seen{1, 0, 1, 0, 0, 0};
    copy_source(s, 2, a, seen);
    CHECK((a == Tour{0, 2, 3, 4, 5}));
    Tour b{0, 3, 5};
    seen = {1, 0, 0, 1, 0, 1};
    copy_source(s, 5, b, seen);
    CHECK((b == Tour{0, 3, 5, 4})); // successor blocked => backward
    auto p = random_problem(30, 3);
    Neighbors nn(p, 4, 5, 8);
    Tour base(p.size());
    std::iota(base.begin(), base.end(), 0);
    Source source(base);
    Pheromone ph(nn, {0.01, 1});
    std::vector<double> h(p.size() * nn.candidate_size, 1);
    bool copied = false;
    for (int threshold : {1, 3, 8, 1000})
        for (int seed = 0; seed < 50; ++seed) {
            auto rng = ant_rng(seed, 1, 0, 0);
            auto a2 = construct(p, nn, ph, h, source, threshold, rng);
            CHECK(valid_tour(a2.tour, p.size()));
            CHECK(a2.selected_edges + a2.copied_vertices == p.size() - 1);
            copied |= a2.copied_vertices > 0;
            if (threshold == 1000)
                CHECK(a2.copied_vertices == 0);
        }
    CHECK(copied);
    Neighbors tiny(square(), 3, 0, 3);
    Pheromone tp(tiny, {0.1, 1});
    std::vector<double> th(12, 1);
    auto rng = ant_rng(1, 1, 0, 0);
    auto a3 = construct(square(), tiny, tp, th, Source({0, 1, 2, 3}), 100, rng);
    CHECK(valid_tour(a3.tour, 4));
}
void selection() {
    Problem p{"line", {{0, 0}, {10, 0}, {20, 0}, {30, 0}, {40, 0}}};
    Neighbors nn(p, 1, 1, 1);
    Pheromone ph(nn, {0.1, 1});
    std::vector<double> h(5, 1);
    auto rng = ant_rng(0, 0, 0, 0);
    std::vector<unsigned char> seen{1, 0, 0, 0, 0};
    CHECK(select_next(p, nn, ph, h, 0, seen, rng) == 1);
    seen[1] = 1;
    CHECK(select_next(p, nn, ph, h, 0, seen, rng) == 2);
    seen[2] = 1;
    CHECK(select_next(p, nn, ph, h, 0, seen, rng) == 3);
    seen[1] = 0;
    h.assign(5, 0);
    CHECK(select_next(p, nn, ph, h, 0, seen, rng) == 1);
}
void local_search() {
    auto p = square();
    Neighbors nn(p, 3, 0, 3);
    Tour t{0, 2, 1, 3};
    CHECK(two_opt(p, nn, t, {0, 1, 2, 3}, true) > 0);
    CHECK(tour_length(p, t) == 40);
    int three_moves = 0;
    for (int seed = 0; seed < 20; ++seed) {
        auto q = random_problem(14, seed);
        Neighbors n(q, 8, 5, 13);
        Tour a(q.size());
        std::iota(a.begin(), a.end(), 0);
        Cost before = tour_length(q, a);
        three_moves += three_opt(q, n, a, true);
        CHECK(valid_tour(a, q.size()));
        CHECK(tour_length(q, a) <= before);
    }
    CHECK(three_moves > 0);
    // A strict 3-opt improvement from a tour with no improving 2-opt move.
    Problem fixture{
        "three-opt-fixture",
        {{32, 63}, {20, 61}, {95, 87}, {73, 55}, {50, 23}, {57, 84}, {63, 57}, {16, 78}}};
    Tour local{0, 6, 5, 2, 3, 4, 1, 7};
    CHECK(tour_length(fixture, local) == 263);
    for (int i = 0; i < 8; ++i)
        for (int j = i + 2; j < 8; ++j) {
            if (i == 0 && j == 7)
                continue;
            CHECK(fixture.distance(local[i], local[(i + 1) % 8]) +
                      fixture.distance(local[j], local[(j + 1) % 8]) <=
                  fixture.distance(local[i], local[j]) +
                      fixture.distance(local[(i + 1) % 8], local[(j + 1) % 8]));
        }
    Neighbors full(fixture, 7, 0, 7);
    CHECK(three_opt(fixture, full, local, true) > 0);
    CHECK(tour_length(fixture, local) < 263);

    Problem demo{"six-city-test-fixture", {{0, 0}, {10, 0}, {20, 0},
                                             {20, 10}, {10, 10}, {0, 10}}};
    Source source({0, 1, 2, 3, 4, 5});
    Tour trace{0, 2};
    std::vector<unsigned char> seen{1, 0, 1, 0, 0, 0};
    copy_source(source, 2, trace, seen);
    trace.push_back(1);
    CHECK((trace == Tour{0, 2, 3, 4, 5, 1}));
    CHECK(tour_length(demo, trace) == 74);
    Neighbors demo_nn(demo, 5, 0, 5);
    CHECK(two_opt(demo, demo_nn, trace, {0, 2, 1}, true) == 1);
    CHECK((trace == Tour{0, 5, 4, 3, 2, 1}));
    CHECK(tour_length(demo, trace) == 60);
}
void pheromone() {
    auto a = trail_limits(100, 16, .1, .5, 1000, true, false),
         b = trail_limits(100, 16, .1, .5, 1000, true, true);
    CHECK(std::abs(a.maximum - .002) < 1e-15);
    CHECK(std::abs(a.minimum - .002 * (1 - std::pow(.1, .01)) / (15 * std::pow(.1, .01))) < 1e-15);
    CHECK(b.minimum > a.minimum);
    CHECK(trail_limits(2, 1, .1, .5, 0, true, false).minimum == 2);
    rejects([] { trail_limits(4, 3, .1, 1, 40, true, false); });
    rejects([] { trail_limits(4, 3, 0, .5, 40, true, false); });
    auto p = square();
    Neighbors nn(p, 3, 0, 3);
    Pheromone ph(nn, {1, 10});
    std::fill(ph.trails.begin(), ph.trails.end(), 1);
    Source source({0, 1, 2, 3});
    for (std::size_t i = 0; i < ph.trails.size(); ++i)
        ph.evaporate_slot(i, .5);
    ph.deposit(source, 2, .5);
    for (std::size_t i = 0; i < ph.trails.size(); ++i)
        ph.clamp_slot(i);
    CHECK(ph.get(0, 1) == 1);
    CHECK(ph.in_bounds()); // clamp(.5*1+.5)=1, not 1.5
    Problem line{"asym-list", {{0, 0}, {1, 0}, {3, 0}}};
    Neighbors asymmetric(line, 1, 0, 1);
    Pheromone q(asymmetric, {.01, 1});
    Source s({0, 1, 2});
    CHECK(asymmetric.rows[2][0] == 1);
    CHECK(asymmetric.rows[1][0] == 0);
    for (std::size_t i = 0; i < q.trails.size(); ++i)
        q.evaporate_slot(i, .5);
    q.deposit(s, 10, .5);
    for (std::size_t i = 0; i < q.trails.size(); ++i)
        q.clamp_slot(i);
    CHECK(std::abs(q.get(2, 1) - .6) < 1e-12);
    CHECK(std::abs(q.get(1, 2) - .5) < 1e-12);
    for (int j = 0; j < 500; ++j) {
        for (std::size_t i = 0; i < q.trails.size(); ++i)
            q.evaporate_slot(i, .5);
        q.deposit(s, 10, .5);
        for (std::size_t i = 0; i < q.trails.size(); ++i)
            q.clamp_slot(i);
        CHECK(q.in_bounds());
    }
}
void check_profile(const Problem &p, const Config &c, const Result &r) {
    const auto &profile = r.parallel_profile;
    CHECK(profile.enabled);
    CHECK(profile.workers.size() == static_cast<std::size_t>(r.actual_threads));
    std::uint64_t ants = 0, selected = 0, copied = 0;
    for (std::size_t i = 0; i < profile.workers.size(); ++i) {
        const auto &worker = profile.workers[i];
        CHECK(worker.thread == static_cast<int>(i));
        CHECK(std::isfinite(worker.ant_work_seconds) && worker.ant_work_seconds >= 0);
        ants += worker.ants_completed;
        selected += worker.aco_selected_edges;
        copied += worker.copied_vertices;
        if (c.schedule == "static") {
            const auto expected =
                static_cast<std::uint64_t>(c.ants / r.actual_threads +
                                           (worker.thread < c.ants % r.actual_threads)) *
                c.iterations;
            CHECK(worker.ants_completed == expected);
        }
    }
    CHECK(ants == static_cast<std::uint64_t>(c.ants) * c.iterations);
    CHECK(selected + copied == ants * static_cast<std::uint64_t>(p.size() - 1));
    double phases = 0;
    for (double value :
         {profile.ant_phase_seconds, profile.selection_seconds, profile.evaporation_seconds,
          profile.deposit_seconds, profile.clamp_seconds, profile.bookkeeping_seconds}) {
        CHECK(std::isfinite(value) && value >= 0);
        phases += value;
    }
    CHECK(phases <= r.search_seconds + 0.01);
}
void solver() {
    Config c;
    c.ants = 16;
    c.iterations = 25;
    c.initial_solutions = 3;
    c.verify = true;
    for (const auto &p :
         {square(), Problem{"zeros", {{0, 0}, {0, 0}, {0, 0}}}, Problem{"pair", {{0, 0}, {1, 0}}}})
        for (bool ls : {false, true}) {
            c.local_search = ls;
            auto r = solve(p, c);
            CHECK(valid_tour(r.tour, p.size()));
            CHECK(tour_length(p, r.tour) == r.best_length);
            CHECK(r.history.size() == 26);
            for (std::size_t j = 1; j < r.history.size(); ++j)
                CHECK(r.history[j].best <= r.history[j - 1].best);
        }
    CHECK(default_ant_count(2) == 64);
    CHECK(default_ant_count(318) == 128);
    c.local_search = true;
    auto p = random_problem(100, 42);
    auto one = solve(p, c);
    CHECK(!one.parallel_profile.enabled);
    c.profile = true;
    auto repeat = solve(p, c);
    CHECK(one.tour == repeat.tour);
    check_profile(p, c, repeat);
#ifdef _OPENMP
    for (int threads : {2, 4})
        for (const auto &schedule : {"static", "dynamic"}) {
            c.threads = threads;
            c.schedule = schedule;
            auto parallel = solve(p, c);
            check_profile(p, c, parallel);
            CHECK(one.tour == parallel.tour);
            CHECK(one.best_length == parallel.best_length);
            CHECK(parallel.actual_threads == threads);
            for (std::size_t i = 0; i < one.history.size(); ++i) {
                CHECK(one.history[i].best == parallel.history[i].best);
                CHECK(one.history[i].iteration_best == parallel.history[i].iteration_best);
            }
        }
#else
    c.threads = 2;
    rejects([&] { solve(p, c); });
    c.threads = 1;
#endif
    rejects([&] {
        solve(p, c, [](const History &row, double) {
            if (row.iteration >= 4)
                throw std::runtime_error("callback failure inside persistent team");
        });
    });
    auto recovered = solve(p, c);
    CHECK(recovered.tour == one.tour);
    check_profile(p, c, recovered);
    c.rho = 1;
    rejects([&] { solve(p, c); });
    c.rho = .5;
    c.beta = std::numeric_limits<double>::quiet_NaN();
    rejects([&] { solve(p, c); });
}
int main() {
    int failed = 0;
    std::vector<std::pair<std::string, std::function<void()>>> groups{
        {"parser/EUC_2D", parser},
        {"tours/exhaustive reference", tours},
        {"k-d tree vs brute force", neighbors},
        {"FACO copying", copying},
        {"selection fallback", selection},
        {"2-opt/3-opt", local_search},
        {"pheromone/formulas", pheromone},
        {"solver/RNG", solver}};
    for (const auto &group : groups)
        try {
            group.second();
            std::cout << "PASS " << group.first << '\n';
        } catch (const std::exception &e) {
            ++failed;
            std::cerr << "FAIL " << group.first << ": " << e.what() << '\n';
        }
    return failed ? 1 : 0;
}
