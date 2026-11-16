#include "faco/faco.hpp"
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
int main(int argc, char **argv) {
    try {
        for (int i = 1; i < argc; ++i)
            if (std::string(argv[i]) == "--help" || std::string(argv[i]) == "-h") {
                faco::print_help();
                return 0;
            }
        auto config = faco::parse_options(argc, argv);
        auto problem = faco::load_tsplib(config.input);
        for (int r = 0; r < config.repeat; ++r) {
            auto run = config;
            run.seed += static_cast<unsigned>(r);
            int previous_iteration = 0;
            double previous_elapsed = 0;
            auto progress = [&run, &previous_iteration, &previous_elapsed](const faco::History &row,
                                                                           double elapsed) {
                std::ostringstream line;
                line << "[Vong " << row.iteration << '/' << run.iterations << "] seed=" << run.seed
                     << " iter_best=" << row.iteration_best << " best=" << row.best
                     << " elapsed=" << std::fixed << std::setprecision(3) << elapsed << 's'
                     << " block_ms=" << (elapsed - previous_elapsed) * 1000
                     << " rounds=" << row.iteration - previous_iteration;
                previous_iteration = row.iteration;
                previous_elapsed = elapsed;
                std::cout << line.str() << std::endl;
            };
            auto result = faco::solve(problem, run, progress);
            faco::save_result(problem, result, r);
            std::cout << "instance=" << problem.name << " seed=" << run.seed
                      << " threads=" << result.actual_threads << " best=" << result.best_length
                      << " preprocessing=" << result.preprocessing_seconds
                      << " initialization=" << result.initialization_seconds
                      << " search=" << result.search_seconds << " total=" << result.total_seconds
                      << " s\n";
            if (result.parallel_profile.enabled) {
                const auto &profile = result.parallel_profile;
                std::ostringstream report;
                report << std::fixed << std::setprecision(6)
                       << "Execution profile: threads=" << result.actual_threads
                       << " schedule=" << run.schedule << " chunk=1\n"
                       << "Phases(s): ants=" << profile.ant_phase_seconds
                       << " selection=" << profile.selection_seconds
                       << " evaporation=" << profile.evaporation_seconds
                       << " deposit=" << profile.deposit_seconds
                       << " clamp=" << profile.clamp_seconds
                       << " bookkeeping=" << profile.bookkeeping_seconds << '\n';
                for (const auto &worker : profile.workers)
                    report << "Thread " << worker.thread << ": ants=" << worker.ants_completed
                           << " ant_work_s=" << worker.ant_work_seconds
                           << " aco_edges=" << worker.aco_selected_edges
                           << " copied_vertices=" << worker.copied_vertices
                           << " ls_moves=" << worker.ls_moves << '\n';
                std::cout << report.str();
            }
        }
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}
