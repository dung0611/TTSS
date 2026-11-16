#include "faco/tour.hpp"
#include <stdexcept>
namespace faco {
Source::Source(const Tour &t) : route(t), position(t.size()) {
    if (!valid_tour(t, static_cast<int>(t.size())))
        throw std::invalid_argument("Invalid source tour");
    for (int i = 0; i < static_cast<int>(t.size()); ++i)
        position[t[i]] = i;
}
int Source::successor(int v) const {
    return route[(position[v] + 1) % route.size()];
}
int Source::predecessor(int v) const {
    return route[(position[v] + route.size() - 1) % route.size()];
}
bool Source::contains_edge(int a, int b) const {
    return successor(a) == b || predecessor(a) == b;
}
void copy_source(const Source &s, int from, Tour &route, std::vector<unsigned char> &visited) {
    int cursor = from;
    while (!visited[s.successor(cursor)]) {
        cursor = s.successor(cursor);
        route.push_back(cursor);
        visited[cursor] = 1;
    }
    // cursor is the predecessor of the first blocked vertex, matching the iterator rollback.
    while (!visited[s.predecessor(cursor)]) {
        cursor = s.predecessor(cursor);
        route.push_back(cursor);
        visited[cursor] = 1;
    }
}
} // namespace faco
