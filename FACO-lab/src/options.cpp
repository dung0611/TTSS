#include "faco/faco.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
namespace faco {
namespace {
std::uint64_t integer(const std::string &s) {
    if (s.empty() || s.find_first_not_of("0123456789") != std::string::npos)
        throw std::invalid_argument("Expected unsigned integer: " + s);
    std::size_t end = 0;
    auto v = std::stoull(s, &end);
    if (end != s.size())
        throw std::invalid_argument("Invalid integer");
    return v;
}
int small(const std::string &s) {
    auto v = integer(s);
    if (v > static_cast<std::uint64_t>(std::numeric_limits<int>::max()))
        throw std::invalid_argument("Integer too large");
    return static_cast<int>(v);
}
double real(const std::string &s) {
    std::size_t end = 0;
    auto v = std::stod(s, &end);
    if (end != s.size())
        throw std::invalid_argument("Invalid real: " + s);
    return v;
}
bool boolean(const std::string &s) {
    if (s == "1" || s == "on")
        return true;
    if (s == "0" || s == "off")
        return false;
    throw std::invalid_argument("Expected 0/1 or off/on");
}
} // namespace
void print_help() {
    std::cout << "FACO C++17 / OpenMP (EUC_2D symmetric TSP)\n"
                 "--input FILE --output-dir DIR [results]\n"
                 "--ants N [0=paper auto] --iterations N [5000] --threads N [1]\n"
                 "--seed N [1; zero is a reproducible seed] --repeat N [1; seeds seed+r]\n"
                 "--rho R [0.5 retained fraction] --beta B [1] --p-best P [0.1]\n"
                 "--candidate-size N [16] --backup-size N [64] --min-new-edges N [8]\n"
                 "--local-search on|off [on] --ls-size N [20] --initial-solutions N [0=hardware "
                 "threads]\n"
                 "--schedule static|dynamic [dynamic] --bounds paper|reference [paper]\n"
                 "--gbest-probability P [0.01] --verify on|off [off]\n"
                 "--log-every N [4; 0 disables progress; also prints initial/final iteration]\n"
                 "--profile on|off [off; phase timings and per-thread ant workload]\n"
                 "--reference-length L --reference-source TEXT --reference-date YYYY-MM-DD\n";
}
Config parse_options(int argc, char **argv) {
    Config c;
    for (int i = 1; i < argc; ++i) {
        std::string key = argv[i];
        if (key == "--help" || key == "-h")
            continue;
        if (i + 1 >= argc)
            throw std::invalid_argument("Missing value: " + key);
        std::string value = argv[++i];
        if (key == "--input")
            c.input = value;
        else if (key == "--output-dir")
            c.output_dir = value;
        else if (key == "--ants")
            c.ants = small(value);
        else if (key == "--iterations")
            c.iterations = small(value);
        else if (key == "--log-every")
            c.log_every = small(value);
        else if (key == "--threads")
            c.threads = small(value);
        else if (key == "--seed")
            c.seed = integer(value);
        else if (key == "--repeat")
            c.repeat = small(value);
        else if (key == "--rho")
            c.rho = real(value);
        else if (key == "--beta")
            c.beta = real(value);
        else if (key == "--candidate-size")
            c.candidate_size = small(value);
        else if (key == "--backup-size")
            c.backup_size = small(value);
        else if (key == "--min-new-edges")
            c.min_new_edges = small(value);
        else if (key == "--local-search")
            c.local_search = boolean(value);
        else if (key == "--ls-size")
            c.ls_size = small(value);
        else if (key == "--initial-solutions")
            c.initial_solutions = small(value);
        else if (key == "--schedule")
            c.schedule = value;
        else if (key == "--bounds") {
            if (value != "paper" && value != "reference")
                throw std::invalid_argument("bounds must be paper or reference");
            c.reference_bounds = value == "reference";
        } else if (key == "--verify")
            c.verify = boolean(value);
        else if (key == "--profile")
            c.profile = boolean(value);
        else if (key == "--p-best")
            c.p_best = real(value);
        else if (key == "--gbest-probability")
            c.global_best_probability = real(value);
        else if (key == "--reference-length") {
            auto v = integer(value);
            if (v > static_cast<std::uint64_t>(std::numeric_limits<Cost>::max()))
                throw std::invalid_argument("Reference length too large");
            c.reference_length = static_cast<Cost>(v);
        } else if (key == "--reference-source")
            c.reference_source = value;
        else if (key == "--reference-date")
            c.reference_date = value;
        else
            throw std::invalid_argument("Unknown option: " + key);
    }
    if (c.input.empty())
        throw std::invalid_argument("--input is required");
    validate_config(c);
    if (c.seed > std::numeric_limits<std::uint64_t>::max() - static_cast<unsigned>(c.repeat - 1))
        throw std::invalid_argument("repeat would overflow seed");
    return c;
}
} // namespace faco
