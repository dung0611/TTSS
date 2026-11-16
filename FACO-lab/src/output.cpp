#include "faco/faco.hpp"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
namespace faco {
namespace {
std::string json_string(const std::string &s) {
    std::ostringstream out;
    out << '"';
    for (unsigned char ch : s) {
        if (ch == '"' || ch == '\\')
            out << '\\' << ch;
        else if (ch < 32)
            out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                << static_cast<unsigned>(ch) << std::dec;
        else
            out << ch;
    }
    out << '"';
    return out.str();
}
std::string csv_string(const std::string &s) {
    std::string out = "\"";
    for (char ch : s) {
        if (ch == '"')
            out += '"';
        out += ch;
    }
    return out + '"';
}
std::ofstream file(const std::filesystem::path &path) {
    std::ofstream out(path);
    if (!out)
        throw std::runtime_error("Cannot create output: " + path.string());
    out << std::setprecision(17);
    return out;
}
} // namespace
void save_result(const Problem &p, const Result &r, int repeat_index) {
    const auto &c = r.config;
    namespace fs = std::filesystem;
    fs::create_directories(c.output_dir);
    // Stable unique stem for this run; repeated identical commands deliberately replace their own
    // files.
    const std::string stem = "run-s" + std::to_string(c.seed) + "-t" +
                             std::to_string(r.actual_threads) + "-" + c.schedule + "-r" +
                             std::to_string(repeat_index);
    fs::path dir(c.output_dir);
    auto csv = file(dir / (stem + ".csv"));
    csv << "instance,dimension,ants,iterations,seed,threads,schedule,preprocessing_seconds,"
           "initialization_seconds,search_seconds,total_seconds,best_length,reference_length,error_"
           "percent\n";
    csv << csv_string(p.name) << ',' << p.size() << ',' << c.ants << ',' << c.iterations << ','
        << c.seed << ',' << r.actual_threads << ',' << c.schedule << ',' << r.preprocessing_seconds
        << ',' << r.initialization_seconds << ',' << r.search_seconds << ',' << r.total_seconds
        << ',' << r.best_length << ',';
    if (c.reference_length > 0)
        csv << c.reference_length;
    csv << ',';
    if (c.reference_length > 0)
        csv << 100.0 *
                   (static_cast<double>(r.best_length) - static_cast<double>(c.reference_length)) /
                   static_cast<double>(c.reference_length);
    csv << '\n';
    auto history = file(dir / (stem + "-history.csv"));
    history << "iteration,best_length,iteration_best\n";
    for (const auto &row : r.history)
        history << row.iteration << ',' << row.best << ',' << row.iteration_best << '\n';
    auto tour = file(dir / (stem + ".tour"));
    tour << "NAME : " << p.name << "\nTYPE : TOUR\nDIMENSION : " << p.size() << "\nTOUR_SECTION\n";
    for (int v : r.tour)
        tour << v + 1 << '\n';
    tour << "-1\nEOF\n";
    auto meta = file(dir / (stem + ".json"));
    meta << "{\n\"instance\":" << json_string(p.name) << ",\n\"input\":" << json_string(c.input)
         << ",\n\"dimension\":" << p.size() << ",\n\"ants\":" << c.ants
         << ",\n\"iterations\":" << c.iterations << ",\n\"log_every\":" << c.log_every
         << ",\n\"seed\":" << c.seed << ",\n\"requested_threads\":" << c.threads
         << ",\n\"threads\":" << r.actual_threads << ",\n\"schedule\":" << json_string(c.schedule)
         << ",\n\"rng\":\"mt19937_64 seed_seq(seed_lo,seed_hi,phase,iteration,ant)\",\n\"bounds\":"
         << json_string(c.reference_bounds ? "reference" : "paper")
         << ",\n\"candidate_size\":" << c.candidate_size << ",\n\"backup_size\":" << c.backup_size
         << ",\n\"ls_size\":" << c.ls_size << ",\n\"initial_solutions\":" << r.initial_count
         << ",\n\"local_search\":" << (c.local_search ? "true" : "false")
         << ",\n\"verify\":" << (c.verify ? "true" : "false")
         << ",\n\"min_new_edges\":" << c.min_new_edges << ",\n\"rho\":" << c.rho
         << ",\n\"beta\":" << c.beta << ",\n\"p_best\":" << c.p_best
         << ",\n\"global_best_probability\":" << c.global_best_probability
         << ",\n\"preprocessing_seconds\":" << r.preprocessing_seconds
         << ",\n\"initialization_seconds\":" << r.initialization_seconds
         << ",\n\"search_seconds\":" << r.search_seconds
         << ",\n\"total_seconds\":" << r.total_seconds << ",\n\"best_length\":" << r.best_length
         << ",\n\"reference_length\":";
    if (c.reference_length > 0)
        meta << c.reference_length;
    else
        meta << "null";
    meta << ",\n\"reference_source\":" << json_string(c.reference_source)
         << ",\n\"reference_date\":" << json_string(c.reference_date) << ",\n\"error_percent\":";
    if (c.reference_length > 0)
        meta << 100.0 *
                    (static_cast<double>(r.best_length) - static_cast<double>(c.reference_length)) /
                    static_cast<double>(c.reference_length);
    else
        meta << "null";
    meta << ",\n\"profiling\":" << (c.profile ? "true" : "false") << ",\n\"profile\":";
    const auto &profile = r.parallel_profile;
    if (profile.enabled) {
        meta << "{\n\"ant_phase_seconds\":" << profile.ant_phase_seconds
             << ",\n\"selection_seconds\":" << profile.selection_seconds
             << ",\n\"evaporation_seconds\":" << profile.evaporation_seconds
             << ",\n\"deposit_seconds\":" << profile.deposit_seconds
             << ",\n\"clamp_seconds\":" << profile.clamp_seconds
             << ",\n\"bookkeeping_seconds\":" << profile.bookkeeping_seconds << ",\n\"workers\":[";
        for (std::size_t i = 0; i < profile.workers.size(); ++i) {
            if (i)
                meta << ',';
            const auto &worker = profile.workers[i];
            meta << "{\"thread\":" << worker.thread
                 << ",\"ants_completed\":" << worker.ants_completed
                 << ",\"ant_work_seconds\":" << worker.ant_work_seconds
                 << ",\"aco_selected_edges\":" << worker.aco_selected_edges
                 << ",\"copied_vertices\":" << worker.copied_vertices
                 << ",\"ls_moves\":" << worker.ls_moves << '}';
        }
        meta << "]}";
    } else {
        meta << "null";
    }

    meta << ",\n\"tour_ids\":[";
    for (std::size_t j = 0; j < r.tour.size(); ++j) {
        if (j)
            meta << ',';
        meta << r.tour[j] + 1;
    }
    meta << "]\n}\n";
    csv.flush();
    history.flush();
    tour.flush();
    meta.flush();
    if (!csv || !history || !tour || !meta)
        throw std::runtime_error("Failed writing result files");
}
} // namespace faco
