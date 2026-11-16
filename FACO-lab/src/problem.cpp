#include "faco/problem.hpp"
#include <cmath>
#include <fstream>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
namespace faco {
namespace {
std::string trim(const std::string &s) {
    const auto a = s.find_first_not_of(" \t\r\n");
    return a == std::string::npos ? "" : s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1);
}
} // namespace
Cost Problem::distance(int a, int b) const {
    const auto &u = points.at(a);
    const auto &v = points.at(b);
    return static_cast<Cost>(std::floor(std::hypot(u.x - v.x, u.y - v.y) + 0.5));
}
Problem read_tsplib(std::istream &in) {
    std::map<std::string, std::string> h;
    std::string line;
    bool section = false;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty())
            continue;
        if (line == "NODE_COORD_SECTION") {
            section = true;
            break;
        }
        auto sep = line.find(':');
        std::string key, value;
        if (sep != std::string::npos) {
            key = trim(line.substr(0, sep));
            value = trim(line.substr(sep + 1));
        } else {
            std::istringstream row(line);
            row >> key;
            std::getline(row, value);
            value = trim(value);
        }
        if (key == "COMMENT")
            continue;
        if (key == "EOF" || key.find("SECTION") != std::string::npos)
            throw std::runtime_error("Unsupported section: " + key);
        if (key != "NAME" && key != "TYPE" && key != "DIMENSION" && key != "EDGE_WEIGHT_TYPE" &&
            key != "EDGE_WEIGHT_FORMAT" && key != "NODE_COORD_TYPE" && key != "DISPLAY_DATA_TYPE")
            throw std::runtime_error("Unsupported TSPLIB header: " + key);
        if (!h.emplace(key, value).second)
            throw std::runtime_error("Duplicate TSPLIB header: " + key);
    }
    if (!section || h["TYPE"] != "TSP" || h["EDGE_WEIGHT_TYPE"] != "EUC_2D")
        throw std::runtime_error(
            "Only TYPE=TSP, EDGE_WEIGHT_TYPE=EUC_2D, NODE_COORD_SECTION supported");
    if (h.count("EDGE_WEIGHT_FORMAT") && h["EDGE_WEIGHT_FORMAT"] != "FUNCTION")
        throw std::runtime_error("Unsupported EDGE_WEIGHT_FORMAT");
    if (h.count("NODE_COORD_TYPE") && h["NODE_COORD_TYPE"] != "TWOD_COORDS")
        throw std::runtime_error("Only TWOD_COORDS supported");
    long long dim = 0;
    std::istringstream ds(h["DIMENSION"]);
    std::string extra;
    if (!(ds >> dim) || (ds >> extra) || dim < 2 || dim > 1000000)
        throw std::runtime_error("DIMENSION must be in [2,1000000]");
    Problem p{h.count("NAME") ? h["NAME"] : "unnamed",
              std::vector<Point>(static_cast<std::size_t>(dim))};
    std::vector<bool> seen(static_cast<std::size_t>(dim));
    int count = 0;
    bool eof = false;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty())
            continue;
        if (line == "EOF") {
            eof = true;
            break;
        }
        long long id = 0;
        Point pt{};
        std::istringstream row(line);
        if (!(row >> id >> pt.x >> pt.y) || (row >> extra) || id < 1 || id > dim ||
            !std::isfinite(pt.x) || !std::isfinite(pt.y) || std::abs(pt.x) > 1e12 ||
            std::abs(pt.y) > 1e12)
            throw std::runtime_error("Invalid coordinate row (IDs 1..DIMENSION, finite coordinates "
                                     "|x|,|y|<=1e12 required)");
        if (seen.at(static_cast<std::size_t>(id - 1)))
            throw std::runtime_error("Duplicate node ID");
        seen[static_cast<std::size_t>(id - 1)] = true;
        p.points[static_cast<std::size_t>(id - 1)] = pt;
        ++count;
    }
    if (count != dim)
        throw std::runtime_error("Missing or extra coordinate rows");
    if (eof)
        while (std::getline(in, line))
            if (!trim(line).empty())
                throw std::runtime_error("Unexpected data after EOF");
    return p;
}
Problem load_tsplib(const std::string &path) {
    std::ifstream in(path);
    if (!in)
        throw std::runtime_error("Cannot open input: " + path);
    return read_tsplib(in);
}
bool valid_tour(const Tour &t, int n) {
    if (static_cast<int>(t.size()) != n)
        return false;
    std::vector<bool> seen(static_cast<std::size_t>(n));
    for (int v : t) {
        if (v < 0 || v >= n || seen[v])
            return false;
        seen[v] = true;
    }
    return true;
}
Cost tour_length(const Problem &p, const Tour &t) {
    if (!valid_tour(t, p.size()))
        throw std::runtime_error("Not a Hamiltonian tour");
    Cost sum = 0;
    int prev = t.back();
    for (int v : t) {
        sum += p.distance(prev, v);
        prev = v;
    }
    return sum;
}
} // namespace faco
