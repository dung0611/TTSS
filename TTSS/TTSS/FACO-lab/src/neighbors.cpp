#include "faco/neighbors.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <queue>
#include <stdexcept>
namespace faco {
namespace {
// Independent balanced, immutable k-d tree. Ties use rounded EUC_2D then node ID.
class KDTree {
    const Problem &p;
    struct Node {
        int point, left = -1, right = -1, axis;
    };
    std::vector<Node> nodes;
    std::vector<int> ids;
    int build(int lo, int hi, int depth) {
        if (lo >= hi)
            return -1;
        int mid = lo + (hi - lo) / 2, axis = depth % 2;
        auto coord = [&](int v) { return axis ? p.points[v].y : p.points[v].x; };
        std::nth_element(ids.begin() + lo, ids.begin() + mid, ids.begin() + hi, [&](int a, int b) {
            return coord(a) < coord(b) || (coord(a) == coord(b) && a < b);
        });
        int index = static_cast<int>(nodes.size());
        nodes.push_back({ids[mid], -1, -1, axis});
        int left = build(lo, mid, depth + 1), right = build(mid + 1, hi, depth + 1);
        nodes[index].left = left;
        nodes[index].right = right;
        return index;
    }
    using Heap = std::priority_queue<std::pair<Cost, int>>;
    void query(int index, int target, int k, Heap &heap) const {
        if (index < 0)
            return;
        const auto &node = nodes[index];
        if (node.point != target) {
            auto item = std::make_pair(p.distance(target, node.point), node.point);
            if (static_cast<int>(heap.size()) < k)
                heap.push(item);
            else if (item < heap.top()) {
                heap.pop();
                heap.push(item);
            }
        }
        const auto &a = p.points[target];
        const auto &b = p.points[node.point];
        double delta = node.axis ? a.y - b.y : a.x - b.x;
        int near = delta < 0 ? node.left : node.right, far = delta < 0 ? node.right : node.left;
        query(near, target, k, heap);
        // Search equality too: another point may have the same rounded distance and a lower ID.
        if (static_cast<int>(heap.size()) < k ||
            std::floor(std::abs(delta) + 0.5) <= static_cast<double>(heap.top().first))
            query(far, target, k, heap);
    }

  public:
    explicit KDTree(const Problem &problem) : p(problem), ids(p.size()) {
        nodes.reserve(p.size());
        std::iota(ids.begin(), ids.end(), 0);
        build(0, p.size(), 0);
    }
    std::vector<int> nearest(int v, int k) const {
        Heap heap;
        query(0, v, k, heap);
        std::vector<int> result(k);
        for (int j = k - 1; j >= 0; --j) {
            result[j] = heap.top().second;
            heap.pop();
        }
        return result;
    }
};
} // namespace
Neighbors::Neighbors(const Problem &p, int candidates, int backup, int local_search) {
    if (candidates < 1 || backup < 0 || local_search < 1)
        throw std::invalid_argument("Invalid neighbor list sizes");
    candidate_size = std::min(candidates, p.size() - 1);
    backup_size = std::min(backup, p.size() - 1 - candidate_size);
    ls_size = std::min(local_search, p.size() - 1);
    int k = std::max(candidate_size + backup_size, ls_size);
    KDTree tree(p);
    rows.reserve(p.size());
    for (int v = 0; v < p.size(); ++v)
        rows.push_back(tree.nearest(v, k));
}
Tour nearest_neighbor_tour(const Problem &p, const Neighbors &nn, int start) {
    Tour t;
    t.reserve(p.size());
    std::vector<bool> seen(p.size());
    t.push_back(start);
    seen.at(start) = true;
    while (static_cast<int>(t.size()) < p.size()) {
        int u = t.back(), next = -1;
        for (int v : nn.rows[u])
            if (!seen[v]) {
                next = v;
                break;
            }
        if (next < 0)
            for (int v = 0; v < p.size(); ++v)
                if (!seen[v] && (next < 0 || std::make_pair(p.distance(u, v), v) <
                                                 std::make_pair(p.distance(u, next), next)))
                    next = v;
        t.push_back(next);
        seen[next] = true;
    }
    return t;
}
} // namespace faco
