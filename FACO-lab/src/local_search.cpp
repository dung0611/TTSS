#include "faco/local_search.hpp"
#include <algorithm>
#include <array>
#include <deque>
#include <numeric>
#include <stdexcept>
namespace faco {
int two_opt(const Problem &p, const Neighbors &nn, Tour &t, const std::vector<int> &checklist,
            bool verify) {
    const int n = p.size();
    std::vector<int> pos(n);
    for (int i = 0; i < n; ++i)
        pos[t[i]] = i;
    std::deque<int> queue;
    std::vector<unsigned char> pending(n);
    auto enqueue = [&](int v) {
        if (!pending.at(v)) {
            pending[v] = 1;
            queue.push_back(v);
        }
    };
    for (int v : checklist)
        enqueue(v);
    int moves = 0;
    while (!queue.empty() && moves < n) {
        int a = queue.front();
        queue.pop_front();
        pending[a] = 0;
        Cost gain = 0;
        int left = 0, right = 0;
        std::array<int, 4> endpoints{};
        auto consider = [&](int i, int j) {
            if (i == j || (i + 1) % n == j || (j + 1) % n == i)
                return;
            int w = t[i], x = t[(i + 1) % n], y = t[j], z = t[(j + 1) % n];
            Cost diff = p.distance(w, x) + p.distance(y, z) - p.distance(w, y) - p.distance(x, z);
            if (diff > gain) {
                gain = diff;
                left = std::min(i, j) + 1;
                right = std::max(i, j);
                endpoints = {w, x, y, z};
            }
        };
        for (int j = 0; j < nn.ls_size; ++j) {
            int b = nn.rows[a][j];
            if (p.distance(a, t[(pos[a] + 1) % n]) > p.distance(a, b))
                consider(pos[a], pos[b]);
            else
                break;
        }
        for (int j = 0; j < nn.ls_size; ++j) {
            int b = nn.rows[a][j];
            if (p.distance(t[(pos[a] + n - 1) % n], a) > p.distance(a, b))
                consider((pos[a] + n - 1) % n, (pos[b] + n - 1) % n);
            else
                break;
        }
        if (gain > 0) {
            const Cost before = verify ? tour_length(p, t) : 0;
            std::reverse(t.begin() + left, t.begin() + right + 1);
            for (int j = left; j <= right; ++j)
                pos[t[j]] = j;
            for (int v : endpoints)
                enqueue(v);
            ++moves;
            if (verify && (tour_length(p, t) != before - gain))
                throw std::logic_error("2-opt delta mismatch");
        }
    }
    return moves;
}
int three_opt(const Problem &p, const Neighbors &nn, Tour &t, bool verify) {
    const int n = p.size();
    if (n < 6)
        return 0;
    std::vector<int> pos(n);
    int moves = 0;
    bool improved = true;
    while (improved && moves < n) {
        improved = false;
        for (int i = 0; i < n; ++i)
            pos[t[i]] = i;
        for (int i = 0; i < n && !improved; ++i) {
            int a = t[i];
            for (int bi = 0; bi < nn.ls_size && !improved; ++bi) {
                int b = nn.rows[a][bi];
                if (p.distance(a, b) > p.distance(a, t[(i + 1) % n]))
                    break;
                for (int ci = 0; ci < nn.ls_size && !improved; ++ci) {
                    int c = nn.rows[b][ci];
                    std::array<int, 3> cut{i, pos[b], pos[c]};
                    std::sort(cut.begin(), cut.end());
                    int x = cut[0], y = cut[1], z = cut[2];
                    if (y - x < 2 || z - y < 2 || n - z + x < 2)
                        continue;
                    // Keep cyclic segment A in its original orientation. Reorder/reverse B,C.
                    std::array<std::pair<int, int>, 2> seg{{{x + 1, y}, {y + 1, z}}};
                    Cost old = p.distance(t[x], t[x + 1]) + p.distance(t[y], t[y + 1]) +
                               p.distance(t[z], t[(z + 1) % n]);
                    for (int swap_order = 0; swap_order < 2 && !improved; ++swap_order)
                        for (int mask = 0; mask < 4 && !improved; ++mask) {
                            if (!swap_order && !mask)
                                continue;
                            auto s0 = seg[swap_order], s1 = seg[1 - swap_order];
                            int first0 = t[(mask & 1) ? s0.second : s0.first],
                                last0 = t[(mask & 1) ? s0.first : s0.second];
                            int first1 = t[(mask & 2) ? s1.second : s1.first],
                                last1 = t[(mask & 2) ? s1.first : s1.second];
                            Cost fresh = p.distance(t[x], first0) + p.distance(last0, first1) +
                                         p.distance(last1, t[(z + 1) % n]);
                            if (fresh >= old)
                                continue;
                            Cost before = verify ? tour_length(p, t) : 0;
                            Tour next;
                            next.reserve(n);
                            next.insert(next.end(), t.begin(), t.begin() + x + 1);
                            auto append = [&](std::pair<int, int> s, bool rev) {
                                if (rev)
                                    for (int q = s.second; q >= s.first; --q)
                                        next.push_back(t[q]);
                                else
                                    for (int q = s.first; q <= s.second; ++q)
                                        next.push_back(t[q]);
                            };
                            append(s0, (mask & 1) != 0);
                            append(s1, (mask & 2) != 0);
                            next.insert(next.end(), t.begin() + z + 1, t.end());
                            t = std::move(next);
                            ++moves;
                            improved = true;
                            if (verify && tour_length(p, t) != before - old + fresh)
                                throw std::logic_error("3-opt delta mismatch");
                        }
                }
            }
        }
    }
    return moves;
}
} // namespace faco
