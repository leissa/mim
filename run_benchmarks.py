#!/usr/bin/env python3

import subprocess
import glob
import pandas as pd
import numpy as np

# === CONFIG ===
WARMUP_RUNS   = 3
N_RUNS        = 9
ALGOS         = ["fvs", "nest", "beta"]
# SETS          = ["trie", "immer", "set"]
SETS          = ["immer"]
ROWS          = [0, 1, 2]
TASKSET_MASK  = "0x1"         # CPU core mask (e.g. 0x1 = core 0)

def run_cmd(cmd):
    print(f"  → Executing: {cmd}")
    cmd = "ulimit -s unlimited && " + cmd
    subprocess.run(cmd, shell=True, check=True)

def run_benchmarks():
    for set in SETS:
        for row in ROWS:
            bench = f"release_{set}/bin/bench"
            # --- Warmup runs ---
            print(f"Performing {WARMUP_RUNS} warmup runs (results ignored)...")
            for i in range(1, WARMUP_RUNS + 1):
                suffix = f"warmup{i}"
                cmd    = f"taskset {TASKSET_MASK} {bench} {row} {suffix}"
                run_cmd(cmd)
            print("Warmup complete.\n")

            # --- Actual measurement runs ---
            for i in range(1, N_RUNS + 1):
                print(f"Running benchmark {i}/{N_RUNS} pinned to core mask {TASKSET_MASK} ...")
                suffix = f"run{i}"
                cmd    = f"taskset {TASKSET_MASK} {bench} {row} {suffix}"
                run_cmd(cmd)

def merge_results():
    for set in SETS:
        for row in ROWS:
            for algo in ALGOS:
                g = f"{set}.{algo}.{row}.run*"
                files = sorted(glob.glob(g))
                if not files:
                    print(f"No files found for `{g}`. Skipping.")
                    continue

                dfs = []
                for f in files:
                    df = pd.read_csv(f, sep='\\s+', comment="%", names=["n", "cycles"])
                    dfs.append(df)

                merged = dfs[0][["n"]].copy()
                all_cycles = np.stack([df["cycles"].to_numpy() for df in dfs], axis=1)

                merged["median"] = np.median(all_cycles, axis=1).astype(int)
                merged["max"]    = np.max   (all_cycles, axis=1).astype(int)
                merged["min"]    = np.min   (all_cycles, axis=1).astype(int)

                out_file = f"{set}.{algo}.{row}.merged"
                merged.to_csv(out_file, sep=" ", index=False, header=["n", "median", "max", "min"])
                print(f"✅ Wrote {out_file}")

def main():
    run_benchmarks()
    merge_results()

if __name__ == "__main__":
    main()
