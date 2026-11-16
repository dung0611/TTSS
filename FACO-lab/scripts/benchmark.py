"""Run matched workloads and save raw data, timing/quality summary and system metadata."""
import argparse
import csv
import hashlib
import json
import os
import platform
import statistics
import subprocess
from pathlib import Path

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--binary", required=True)
    ap.add_argument("--sequential-binary")
    ap.add_argument("--input", required=True)
    ap.add_argument("--output-dir", default="results/benchmark")
    ap.add_argument("--threads", nargs="+", type=int, default=[1, 2, 4])
    ap.add_argument("--seeds", nargs="+", type=int, default=[11, 22, 33])
    ap.add_argument("--ants", type=int, default=128)
    ap.add_argument("--iterations", type=int, default=200)
    ap.add_argument("--initial-solutions", type=int, default=4)
    ap.add_argument("--schedule", choices=["static", "dynamic"], default="dynamic")
    ap.add_argument("--profile", action="store_true", help="collect instrumented per-phase and per-thread data")
    ap.add_argument("--reference-length", type=int)
    ap.add_argument("--reference-source")
    ap.add_argument("--reference-date")
    a = ap.parse_args()
    if any(t < 1 or t > (os.cpu_count() or 1) for t in a.threads) or 1 not in a.threads:
        ap.error("threads must include 1 and stay within detected logical CPUs")
    if a.reference_length and not (a.reference_source and a.reference_date):
        ap.error("reference length requires source/date")
    root = Path(a.output_dir).resolve()
    root.mkdir(parents=True, exist_ok=True)
    records = []
    workloads = [("openmp", a.binary, t) for t in a.threads]
    if a.sequential_binary:
        workloads.insert(0, ("sequential", a.sequential_binary, 1))
    for seed_index, seed in enumerate(a.seeds):
        # Rotate order across seeds to reduce systematic warmup/order bias.
        offset = seed_index % len(workloads)
        order = workloads[offset:] + workloads[:offset]
        for build, binary, threads in order:
            out = root / build / f"t{threads}" / f"seed{seed}"
            cmd = [str(Path(binary).resolve()), "--input", str(Path(a.input).resolve()),
                   "--ants", str(a.ants), "--iterations", str(a.iterations),
                   "--threads", str(threads), "--seed", str(seed), "--schedule", a.schedule,
                   "--initial-solutions", str(a.initial_solutions), "--log-every", "0", "--output-dir", str(out)]
            if a.profile:
                cmd += ["--profile", "on"]
            if a.reference_length:
                cmd += ["--reference-length", str(a.reference_length), "--reference-source", a.reference_source,
                        "--reference-date", a.reference_date]
            print("Running", build, "threads", threads, "seed", seed, flush=True)
            result = subprocess.run(cmd, check=True, text=True, capture_output=True)
            (out / "stdout.txt").write_text(result.stdout, encoding="utf-8")
            (out / "command.json").write_text(json.dumps(cmd, ensure_ascii=False, indent=2), encoding="utf-8")
            path, = out.glob("run-*.json")
            record = json.loads(path.read_text(encoding="utf-8"))
            record["build"] = build
            profile = record.get("profile") or {}
            for key in ("ant_phase_seconds", "selection_seconds", "evaporation_seconds", "deposit_seconds", "clamp_seconds", "bookkeeping_seconds"):
                record[key] = profile.get(key)
            records.append(record)
    # Validate output artifacts independently from the solver.
    for seed in a.seeds:
        same_seed = [r for r in records if r["seed"] == seed]
        assert all(r["tour_ids"] == same_seed[0]["tour_ids"] for r in same_seed), "RNG reproducibility mismatch"
    fields = ["build", "instance", "dimension", "ants", "iterations", "seed", "threads", "schedule",
              "preprocessing_seconds", "initialization_seconds", "search_seconds", "total_seconds", "best_length",
              "reference_length", "error_percent", "ant_phase_seconds", "selection_seconds",
              "evaporation_seconds", "deposit_seconds", "clamp_seconds", "bookkeeping_seconds"]
    with (root / "raw.csv").open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=fields, extrasaction="ignore")
        writer.writeheader()
        writer.writerows(records)
    baseline = {r["seed"]: r for r in records if r["build"] == "openmp" and r["threads"] == 1}
    summaries = []
    for build, _, threads in workloads:
        group = [r for r in records if r["build"] == build and r["threads"] == threads]
        speedups = [baseline[r["seed"]]["search_seconds"] / r["search_seconds"] for r in group]
        median_speedup = statistics.median(speedups)
        summaries.append(dict(build=build, threads=threads, runs=len(group),
                              preprocessing_median=statistics.median(r["preprocessing_seconds"] for r in group),
                              initialization_median=statistics.median(r["initialization_seconds"] for r in group),
                              search_median=statistics.median(r["search_seconds"] for r in group),
                              search_min=min(r["search_seconds"] for r in group),
                              search_max=max(r["search_seconds"] for r in group),
                              total_median=statistics.median(r["total_seconds"] for r in group),
                              paired_speedup_median=median_speedup, efficiency=median_speedup / threads,
                              best_min=min(r["best_length"] for r in group),
                              best_mean=statistics.mean(r["best_length"] for r in group),
                              best_max=max(r["best_length"] for r in group)))
    with (root / "summary.csv").open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=list(summaries[0]))
        writer.writeheader()
        writer.writerows(summaries)
    metadata = dict(platform=platform.platform(), logical_cpus=os.cpu_count(),
                    processor=platform.processor(), input_sha256=hashlib.sha256(Path(a.input).read_bytes()).hexdigest(),
                    arguments=vars(a), rng_verified_equal_tours=True,
                    timing_scope="steady_clock: preprocess=neighbors+heuristic; init=NN+LS+pheromone+state; search=iterations; total=sum including stage transitions; excludes input/output and thread-runtime bootstrap",
                    speedup="median over seeds of paired OpenMP 1-thread search time / p-thread search time")
    (root / "metadata.json").write_text(json.dumps(metadata, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps(summaries, indent=2))

if __name__ == "__main__":
    main()
