"""CLI/output tests, standard library only; run against either compiled binary."""
import argparse
import csv
import json
import re
import subprocess
import tempfile
from pathlib import Path
from verify_results import verify

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--binary", required=True)
    a = ap.parse_args()
    binary = str(Path(a.binary).resolve())
    data = Path(__file__).resolve().parents[1] / "data" / "square4.tsp"
    base = [binary, "--input", str(data)]
    with tempfile.TemporaryDirectory(prefix="faco-integration-") as directory:
        cmd = base + ["--ants", "8", "--iterations", "10", "--initial-solutions", "2", "--seed", "3",
                      "--repeat", "2", "--output-dir", directory, "--verify", "on", "--reference-length", "40",
                      "--reference-source", "exhaustive square4 test", "--reference-date", "2026-11-10"]
        completed = subprocess.run(cmd, check=True, capture_output=True, text=True)
        assert [int(n) for n in re.findall(r"\[Vong (\d+)/10\]", completed.stdout)] == [0, 4, 8, 10] * 2
        assert verify(data, directory) == 2
        for path in Path(directory).glob("run-s*.json"):
            record = json.loads(path.read_text(encoding="utf-8"))
            assert record["reference_length"] == 40 and record["error_percent"] == 0
            with path.with_suffix(".csv").open(newline="", encoding="utf-8") as f:
                row, = csv.DictReader(f)
            for field in ("instance", "dimension", "ants", "iterations", "seed", "threads", "best_length"):
                assert str(record[field]) == row[field]
            assert record["total_seconds"] >= record["search_seconds"] >= 0
        failures = [["--rho", "1"], ["--rho", "nan"], ["--beta", "inf"], ["--ants", "-1"],
                    ["--iterations", "0"], ["--candidate-size", "0"], ["--schedule", "unknown"],
                    ["--bounds", "unknown"], ["--seed", "-1"], ["--reference-length", "40"],
                    ["--unknown", "1"], ["--ants"], ["--seed", str(2**64-1), "--repeat", "2"],
                    ["--log-every", "-1"], ["--log-every", "invalid"], ["--profile", "invalid"]]
        for args in failures:
            result = subprocess.run(base + args, capture_output=True, text=True)
            assert result.returncode != 0 and "Error:" in result.stderr, args
    progress_checks(binary, data)
    profile_checks(binary, data)
    print(f"PASS CLI integration: repeat/export/reference and {len(failures)} rejected option cases")

def progress_checks(binary, data):
    pattern = re.compile(r"\[Vong (\d+)/(\d+)\] seed=(\d+) iter_best=(\d+) best=(\d+) elapsed=([0-9.]+)s")
    baseline = None
    with tempfile.TemporaryDirectory(prefix="faco-progress-") as directory:
        for interval, expected in [(4, [0, 4, 8, 10]), (3, [0, 3, 6, 9, 10]), (0, [])]:
            out = Path(directory) / str(interval)
            cmd = [binary, "--input", str(data), "--ants", "8", "--iterations", "10",
                   "--initial-solutions", "2", "--seed", "3", "--log-every", str(interval),
                   "--output-dir", str(out)]
            run = subprocess.run(cmd, check=True, capture_output=True, text=True)
            logged = pattern.findall(run.stdout)
            assert [int(row[0]) for row in logged] == expected, run.stdout
            path, = out.glob("run-s*.json")
            record = json.loads(path.read_text(encoding="utf-8"))
            assert record["log_every"] == interval
            with path.with_name(path.stem + "-history.csv").open(newline="", encoding="utf-8") as f:
                history = list(csv.DictReader(f))
            elapsed = []
            for iteration, total, seed, iteration_best, best, seconds in logged:
                row = history[int(iteration)]
                assert int(total) == 10 and int(seed) == 3
                assert int(iteration_best) == int(row["iteration_best"])
                assert int(best) == int(row["best_length"])
                elapsed.append(float(seconds))
            assert elapsed == sorted(elapsed)
            value = (record["tour_ids"], history)
            assert baseline is None or baseline == value, "logging changed the search result"
            baseline = value
    print("PASS progress: every 4/3 iterations, final partial interval, disabled mode, exact history and RNG unchanged")

def profile_checks(binary, data):
    baseline = None
    with tempfile.TemporaryDirectory(prefix="faco-profile-") as directory:
        for enabled in ("off", "on"):
            out = Path(directory) / enabled
            run = subprocess.run([binary, "--input", str(data), "--ants", "8", "--iterations", "8",
                                  "--initial-solutions", "2", "--seed", "3", "--log-every", "0",
                                  "--profile", enabled, "--output-dir", str(out)],
                                 check=True, text=True, capture_output=True)
            path, = out.glob("run-s*.json")
            record = json.loads(path.read_text(encoding="utf-8"))
            with path.with_name(path.stem + "-history.csv").open(newline="", encoding="utf-8") as f:
                rows = list(csv.DictReader(f))
            value = (record["tour_ids"], rows)
            assert baseline is None or baseline == value
            baseline = value
            assert record["profiling"] == (enabled == "on")
            profile = record["profile"]
            if enabled == "off":
                assert profile is None and "Execution profile:" not in run.stdout
                continue
            assert "Phases(s):" in run.stdout and "Thread 0:" in run.stdout
            workers = profile["workers"]
            assert len(workers) == record["threads"] == 1
            assert sum(w["ants_completed"] for w in workers) == 64
            assert sum(w["aco_selected_edges"] + w["copied_vertices"] for w in workers) == 64 * 3
            for key, value in profile.items():
                if key != "workers":
                    assert value >= 0
    print("PASS profiling: phase/work counters, JSON/stdout, disabled mode and RNG invariance")

if __name__ == "__main__":
    main()
