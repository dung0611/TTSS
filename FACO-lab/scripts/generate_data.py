"""Original deterministic data, standard library only. No external TSP solver."""
import argparse
import random
from pathlib import Path

def generate(path, n, seed):
    rng = random.Random(seed)
    rows = [f"NAME : uniform{n}", "TYPE : TSP", f"COMMENT : Original synthetic coordinates; Python Random seed {seed}", f"DIMENSION : {n}", "EDGE_WEIGHT_TYPE : EUC_2D", "NODE_COORD_SECTION"]
    rows += [f"{i+1} {rng.randrange(100000)} {rng.randrange(100000)}" for i in range(n)]
    Path(path).write_text("\n".join(rows + ["EOF", ""]), encoding="utf-8")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", required=True)
    parser.add_argument("--dimension", type=int, default=2048)
    parser.add_argument("--seed", type=int, default=2026)
    args = parser.parse_args()
    if args.dimension < 2:
        parser.error("dimension must be >=2")
    generate(args.output, args.dimension, args.seed)
