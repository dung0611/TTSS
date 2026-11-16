#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

if [[ $# -lt 1 ]]; then
  echo "Usage: $0 {1|2|4|8|benchmark} [arguments passed to benchmark script]" >&2
  exit 2
fi

MODE="$1"
shift
if [[ "$MODE" == "benchmark" ]]; then
  exec python3 scripts/run_openmp_benchmark.py --binary build-omp/faco "$@"
fi

case "$MODE" in
  1|2|4|8) THREADS="$MODE" ;;
  *) echo "Thread count must be 1, 2, 4, 8, or benchmark." >&2; exit 2 ;;
esac

STAMP="$(date +%Y%m%d-%H%M%S)"
OUT="results/uniform2048-threads${THREADS}-${STAMP}"
exec ./build-omp/faco \
  --input data/uniform2048.tsp --ants 128 --iterations 200 \
  --threads "$THREADS" --seed 11 --initial-solutions 4 \
  --schedule dynamic --local-search on --log-every 4 --profile off \
  --output-dir "$OUT" "$@"
