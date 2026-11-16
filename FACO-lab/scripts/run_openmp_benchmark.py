#!/usr/bin/env python3
"""Run a reproducible, serial OpenMP scaling experiment and save every artifact."""
import argparse
import csv
import hashlib
import json
import os
import platform
import re
import statistics
import subprocess
import sys
from datetime import datetime
from pathlib import Path


THREADS = [1, 2, 4, 8]
SEEDS = [11, 22, 33]
ALGORITHM = [
    "--ants", "128", "--iterations", "200", "--initial-solutions", "4",
    "--schedule", "dynamic", "--local-search", "on", "--candidate-size", "16",
    "--backup-size", "64", "--min-new-edges", "8", "--ls-size", "20",
    "--rho", "0.5", "--beta", "1", "--p-best", "0.1",
    "--gbest-probability", "0.01", "--bounds", "paper", "--verify", "off",
    "--log-every", "0", "--profile", "off",
]


def sha256(path):
    digest = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def read_release():
    values = {}
    try:
        for line in Path("/etc/os-release").read_text().splitlines():
            if "=" in line:
                key, value = line.split("=", 1)
                values[key] = value.strip('"')
    except OSError:
        pass
    return {"name": values.get("PRETTY_NAME", platform.platform()),
            "id": values.get("ID"), "version": values.get("VERSION_ID")}


def cpu_topology():
    physical = set()
    model_name = None
    try:
        blocks = Path("/proc/cpuinfo").read_text().strip().split("\n\n")
        for block in blocks:
            fields = {key.strip(): value.strip() for key, value in
                      (line.split(":", 1) for line in block.splitlines() if ":" in line)}
            model_name = model_name or fields.get("model name")
            if "physical id" in fields and "core id" in fields:
                physical.add((fields["physical id"].strip(), fields["core id"].strip()))
    except OSError:
        pass
    return {"physical_cores": len(physical) or None,
            "logical_cpus": os.cpu_count(), "processor": model_name or platform.processor()}


def q(value, probs):
    if len(value) == 1:
        return value[0]
    return statistics.quantiles(value, n=4, method="inclusive")[probs]


def build_metadata(binary, input_path, output_root, warmups, repeats, threads, workload):
    cache = binary.parent / "CMakeCache.txt"
    flags = binary.parent / "CMakeFiles" / "faco.dir" / "flags.make"
    cache_text = cache.read_text(errors="replace") if cache.exists() else ""
    flags_text = flags.read_text(errors="replace") if flags.exists() else ""
    def cache_value(name):
        match = re.search(rf"^{re.escape(name)}:[^=]+=([^\n]+)$", cache_text, re.M)
        return match.group(1).strip() if match else None
    compiler = cache_value("CMAKE_CXX_COMPILER") or "g++"
    compiler_version = subprocess.run([compiler, "--version"], text=True,
                                      capture_output=True, check=True).stdout.splitlines()[0]
    cmake_version = subprocess.run(["cmake", "--version"], text=True,
                                   capture_output=True, check=True).stdout.splitlines()[0]
    omp_macro = subprocess.run([compiler, "-fopenmp", "-dM", "-E", "-"], input="",
                               text=True, capture_output=True, check=True)
    omp_line = next((line for line in omp_macro.stdout.splitlines()
                     if line.startswith("#define _OPENMP ")), None)
    mem_kb = None
    try:
        for line in Path("/proc/meminfo").read_text().splitlines():
            if line.startswith("MemTotal:"):
                mem_kb = int(line.split()[1])
                break
    except OSError:
        pass
    return {
        "created_local": datetime.now().astimezone().isoformat(timespec="seconds"),
        "output_directory": str(output_root),
        "operating_system": read_release(), "platform": platform.platform(),
        "cpu": cpu_topology(), "ram_total_kib": mem_kb,
        "compiler": compiler_version, "cmake": cmake_version,
        "cmake_build_type": cache_value("CMAKE_BUILD_TYPE"),
        "faco_openmp": cache_value("FACO_OPENMP"),
        "openmp_macro": omp_line,
        "compile_flags": next((line.split("=", 1)[1].strip() for line in flags_text.splitlines()
                                if line.startswith("CXX_FLAGS =")), None),
        "binary": str(binary), "binary_sha256": sha256(binary),
        "input": str(input_path), "input_sha256": sha256(input_path),
        "threads_requested": threads, "seeds": SEEDS,
        "workload": {"ants": workload["ants"], "iterations": workload["iterations"],
                     "initial_solutions": workload["initial_solutions"],
                     "schedule": "dynamic", "local_search": workload["local_search"],
                     "candidate_size": 16, "backup_size": 64, "min_new_edges": 8,
                     "ls_size": 20, "rho": 0.5, "beta": 1.0, "p_best": 0.1,
                     "global_best_probability": 0.01, "bounds": "paper",
                     "verify": False, "log_every": 0, "profile": False},
        "warmups_per_thread_count": warmups, "measurement_repeats_per_seed_thread": repeats,
        "timing_policy": "Sequential executions; one excluded warm-up for each requested thread count; 5 measured repetitions per seed and thread count; run order rotates by repeat and seed.",
        "speedup_policy": "For each seed, take the median of repeated timings at each requested setting, divide the OpenMP 1-thread median by that paired setting median, then take the median of those per-seed ratios. Efficiency divides by the median actual OpenMP team size; it is not CPU utilization.",
    }


def main():
    global THREADS
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--binary", default="build-omp/faco")
    ap.add_argument("--input", default="data/uniform2048.tsp")
    ap.add_argument("--output-dir", help="New experiment directory; default includes a timestamp")
    ap.add_argument("--warmups", type=int, default=1)
    ap.add_argument("--repeats", type=int, default=5)
    ap.add_argument("--threads", type=int, nargs="+", default=THREADS)
    ap.add_argument("--ants", type=int, default=128)
    ap.add_argument("--iterations", type=int, default=200)
    ap.add_argument("--initial-solutions", type=int, default=4)
    ap.add_argument("--local-search", choices=["on", "off"], default="on")
    args = ap.parse_args()
    if args.warmups < 1 or args.repeats < 3:
        ap.error("at least one warm-up and three measured repetitions are required")
    if not args.threads or any(t < 1 for t in args.threads) or len(set(args.threads)) != len(args.threads):
        ap.error("thread counts must be unique positive integers")
    THREADS = args.threads
    workload = {"ants": args.ants, "iterations": args.iterations,
                "initial_solutions": args.initial_solutions,
                "local_search": args.local_search == "on"}
    binary = Path(args.binary).resolve()
    input_path = Path(args.input).resolve()
    if not binary.is_file() or not input_path.is_file():
        ap.error("binary and input must exist")
    cache = binary.parent / "CMakeCache.txt"
    cache_text = cache.read_text(errors="replace") if cache.exists() else ""
    if "FACO_OPENMP:BOOL=ON" not in cache_text or "CMAKE_BUILD_TYPE:STRING=Release" not in cache_text:
        ap.error("binary must come from a CMake Release build configured with FACO_OPENMP=ON")
    if args.output_dir:
        root = Path(args.output_dir).resolve()
    else:
        stamp = datetime.now().astimezone().strftime("%Y%m%d-%H%M%S")
        root = Path("results") / f"openmp-benchmark-{stamp}"
        root = root.resolve()
    if root.exists() and any(root.iterdir()):
        ap.error(f"refusing to overwrite non-empty experiment directory: {root}")
    root.mkdir(parents=True, exist_ok=True)
    metadata = build_metadata(binary, input_path, root, args.warmups, args.repeats,
                              THREADS, workload)
    (root / "metadata.json").write_text(json.dumps(metadata, ensure_ascii=False, indent=2) + "\n",
                                         encoding="utf-8")
    records = []

    def run_one(threads, seed, kind, repetition):
        label = "warmup" if kind == "warmup" else f"repeat{repetition:02d}"
        out = root / "runs" / f"seed{seed}" / f"threads{threads}" / label
        out.mkdir(parents=True, exist_ok=True)
        algorithm = ALGORITHM.copy()
        for option, value in (("--ants", args.ants), ("--iterations", args.iterations),
                              ("--initial-solutions", args.initial_solutions),
                              ("--local-search", args.local_search)):
            algorithm[algorithm.index(option) + 1] = str(value)
        command = [str(binary), "--input", str(input_path), *algorithm,
                   "--threads", str(threads), "--seed", str(seed),
                   "--output-dir", str(out)]
        (out / "command.json").write_text(json.dumps(command, ensure_ascii=False, indent=2) + "\n",
                                           encoding="utf-8")
        print(f"{kind}: seed={seed} requested_threads={threads} repetition={repetition}", flush=True)
        result = subprocess.run(command, text=True, capture_output=True, check=True)
        (out / "stdout.txt").write_text(result.stdout, encoding="utf-8")
        json_path, = out.glob("run-*.json")
        record = json.loads(json_path.read_text(encoding="utf-8"))
        stem = json_path.stem
        record.update({
            "run_kind": kind, "repetition": repetition,
            "requested_threads": record["requested_threads"],
            "actual_threads": record["threads"],
            "binary_sha256": metadata["binary_sha256"],
            "input_sha256": metadata["input_sha256"],
            "command": json.dumps(command, ensure_ascii=False),
            "run_json": str(json_path.relative_to(root)),
            "tour_path": str((out / f"{stem}.tour").relative_to(root)),
            "history_path": str((out / f"{stem}-history.csv").relative_to(root)),
        })
        records.append(record)

    # Warm each requested team size once; these observations are preserved but excluded from stats.
    for index, threads in enumerate(THREADS):
        run_one(threads, SEEDS[0], "warmup", index + 1)
    for repetition in range(1, args.repeats + 1):
        for seed_index, seed in enumerate(SEEDS):
            offset = (repetition - 1 + seed_index) % len(THREADS)
            order = THREADS[offset:] + THREADS[:offset]
            for threads in order:
                run_one(threads, seed, "measurement", repetition)

    raw_fields = ["run_kind", "repetition", "seed", "requested_threads", "actual_threads",
        "ants", "iterations", "initial_solutions", "schedule", "local_search",
                  "candidate_size", "backup_size", "min_new_edges", "ls_size", "rho", "beta",
                  "p_best", "global_best_probability", "bounds", "verify", "log_every",
                  "preprocessing_seconds", "initialization_seconds", "search_seconds", "total_seconds",
                  "best_length", "binary_sha256", "input_sha256", "command", "run_json",
                  "tour_path", "history_path"]
    with (root / "raw.csv").open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=raw_fields, extrasaction="ignore")
        writer.writeheader()
        writer.writerows(records)

    measured = [r for r in records if r["run_kind"] == "measurement"]
    summary = []
    per_seed = {}
    for threads in THREADS:
        group = [r for r in measured if r["requested_threads"] == threads]
        actuals = sorted({r["actual_threads"] for r in group})
        seed_medians = {}
        for seed in SEEDS:
            seed_group = [r for r in group if r["seed"] == seed]
            seed_medians[seed] = {
                "search": statistics.median(r["search_seconds"] for r in seed_group),
                "total": statistics.median(r["total_seconds"] for r in seed_group),
                "actual_threads": statistics.median(r["actual_threads"] for r in seed_group),
            }
        per_seed[threads] = seed_medians
        search_ratios, total_ratios = [], []
        for seed in SEEDS:
            search_ratios.append(per_seed[1][seed]["search"] / seed_medians[seed]["search"])
            total_ratios.append(per_seed[1][seed]["total"] / seed_medians[seed]["total"])
        search_values = [r["search_seconds"] for r in group]
        total_values = [r["total_seconds"] for r in group]
        actual_median = statistics.median(r["actual_threads"] for r in group)
        summary.append({
            "requested_threads": threads, "actual_threads_values": ";".join(map(str, actuals)),
            "actual_threads_median": actual_median, "runs": len(group),
            "best_length": group[0]["best_length"],
            "preprocessing_median": statistics.median(r["preprocessing_seconds"] for r in group),
            "initialization_median": statistics.median(r["initialization_seconds"] for r in group),
            "search_median": statistics.median(search_values), "search_q1": q(search_values, 0),
            "search_q3": q(search_values, 2), "search_iqr": q(search_values, 2) - q(search_values, 0),
            "total_median": statistics.median(total_values), "total_q1": q(total_values, 0),
            "total_q3": q(total_values, 2), "total_iqr": q(total_values, 2) - q(total_values, 0),
            "paired_search_speedup_median": statistics.median(search_ratios),
            "paired_search_efficiency_percent": 100 * statistics.median(search_ratios) / actual_median,
            "paired_total_speedup_median": statistics.median(total_ratios),
            "per_seed_search_speedups": json.dumps(search_ratios),
            "per_seed_total_speedups": json.dumps(total_ratios),
        })
    with (root / "summary.csv").open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(summary[0]))
        writer.writeheader()
        writer.writerows(summary)
    metadata["actual_threads_by_requested"] = {
        str(threads): sorted({r["actual_threads"] for r in measured
                              if r["requested_threads"] == threads}) for threads in THREADS}
    verifier = Path(__file__).with_name("verify_results.py")
    subprocess.run([sys.executable, str(verifier), "--input", str(input_path),
                    "--results", str(root)], check=True)
    metadata["verification"] = "PASS: independent tour, length, complete-history and matched-workload comparison"
    (root / "metadata.json").write_text(json.dumps(metadata, ensure_ascii=False, indent=2) + "\n",
                                         encoding="utf-8")
    plotter = Path(__file__).with_name("plot_openmp_benchmark.py")
    subprocess.run([sys.executable, str(plotter), "--results", str(root)], check=True)
    print(f"Benchmark complete: {root}", flush=True)
    print((root / "raw.csv"), flush=True)
    print((root / "summary.csv"), flush=True)


if __name__ == "__main__":
    main()
