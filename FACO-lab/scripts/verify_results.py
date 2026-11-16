"""Independent verification of exported tours, lengths and iteration histories."""
import argparse
import csv
import json
import math
from pathlib import Path

def coordinates(path):
    points = {}
    started = False
    for raw in Path(path).read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if line == "NODE_COORD_SECTION":
            started = True
            continue
        if started:
            if line == "EOF":
                break
            if line:
                node, x, y = line.split()
                points[int(node)] = (float(x), float(y))
    return points

def verify(input_path, output_dir):
    points = coordinates(input_path)
    matched = {}
    counts_by_threads = {}
    seen = set()
    count = 0
    for path in sorted(Path(output_dir).rglob("run-s*.json")):
        record = json.loads(path.read_text(encoding="utf-8"))
        tour = record["tour_ids"]
        assert len(tour) == len(points) and set(tour) == set(points), path
        length = 0
        for a, b in zip(tour, tour[1:] + tour[:1]):
            u, v = points[a], points[b]
            length += math.floor(math.hypot(u[0] - v[0], u[1] - v[1]) + 0.5)
        assert length == record["best_length"], (path, length)
        with path.with_name(path.stem + "-history.csv").open(newline="", encoding="utf-8") as f:
            history = list(csv.DictReader(f))
        assert [int(row["iteration"]) for row in history] == list(range(record["iterations"] + 1)), path
        best = [int(row["best_length"]) for row in history]
        assert best[-1] == length and all(b <= a for a, b in zip(best, best[1:])), path
        assert all(int(row["best_length"]) <= int(row["iteration_best"]) for row in history), path
        tour_text = path.with_suffix(".tour").read_text(encoding="utf-8").split("TOUR_SECTION\n", 1)[1]
        assert [int(row) for row in tour_text.splitlines()[:-2]] == tour, path
        # All records in one directory must describe a matched workload.
        # Thread count and measurement repetition are intentionally excluded: this key
        # describes the algorithmic workload whose outputs must be bit-for-bit stable.
        key = tuple(record.get(field) for field in (
            "instance", "dimension", "seed", "ants", "iterations", "schedule",
            "local_search", "candidate_size", "backup_size", "min_new_edges", "ls_size",
            "initial_solutions", "bounds", "rho", "beta", "p_best",
            "global_best_probability"))
        value = (tour, history)
        assert key not in matched or matched[key] == value, (path, "RNG/history mismatch")
        matched[key] = value
        requested = record.get("requested_threads", record.get("threads"))
        actual = record.get("threads")
        counts_by_threads.setdefault(requested, set()).add(actual)
        seen.add((record["seed"], requested))
        count += 1
    assert count, "No run JSON found"
    requested_values = sorted(counts_by_threads)
    seeds = sorted({seed for seed, _ in seen})
    missing = [(seed, threads) for seed in seeds for threads in requested_values
               if (seed, threads) not in seen]
    assert not missing, f"Missing seed/thread configurations: {missing}"
    actual_text = ", ".join(f"requested {requested}: actual {sorted(actuals)}"
                             for requested, actuals in sorted(counts_by_threads.items()))
    print(f"PASS independent export verification: {count} tours; exact lengths, complete histories, "
          f"and identical tours/history for matched algorithm workloads.")
    print(f"Seeds: {seeds}; {actual_text}")
    return count

if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--input", required=True)
    ap.add_argument("--results", required=True)
    args = ap.parse_args()
    verify(args.input, args.results)
