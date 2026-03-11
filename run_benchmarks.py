#!/usr/bin/env python3
import subprocess
import glob
import pandas as pd
import numpy as np

# === CONFIG ===
WARMUP_RUNS = 3
N_RUNS = 9
FILE_PREFIXES = ["xxx", "yyy", "zzz"]
BENCH_PATH = "./bin/bench"   # Path to your benchmark executable
TASKSET_MASK = "0x1"         # CPU core mask (e.g. 0x1 = core 0)

# Optional extra argument before suffix (e.g., "./bin/bench B suffix")
B = ""  # set to "" or e.g. "128" if needed

# === FUNCTIONS ===

def run_cmd(cmd):
    print(f"  → Executing: {cmd}")
    subprocess.run(cmd, shell=True, check=True)

def run_benchmarks():
    # --- Warmup runs ---
    print(f"Performing {WARMUP_RUNS} warmup runs (results ignored)...")
    for i in range(1, WARMUP_RUNS + 1):
        suffix = f"warmup{i}"
        cmd = f"taskset {TASKSET_MASK} {BENCH_PATH}"
        if B:
            cmd += f" {B}"
        cmd += f" {suffix}"
        run_cmd(cmd)
    print("Warmup complete.\n")

    # --- Actual measurement runs ---
    for i in range(1, N_RUNS + 1):
        suffix = f"run{i}"
        print(f"Running benchmark {i}/{N_RUNS} pinned to core mask {TASKSET_MASK} ...")
        cmd = f"taskset {TASKSET_MASK} {BENCH_PATH}"
        if B:
            cmd += f" {B}"
        cmd += f" {suffix}"
        run_cmd(cmd)

def merge_results():
    for prefix in FILE_PREFIXES:
        files = sorted(glob.glob(f"{prefix}.run*"))
        if not files:
            print(f"No files found for prefix {prefix}. Skipping.")
            continue

        dfs = []
        for f in files:
            df = pd.read_csv(f, delim_whitespace=True, comment="%", names=["n", "cycles"])
            dfs.append(df)

        merged = dfs[0][["n"]].copy()
        all_cycles = np.stack([df["cycles"].to_numpy() for df in dfs], axis=1)

        merged["median"] = np.median(all_cycles, axis=1).astype(int)
        merged["max"] = np.max(all_cycles, axis=1).astype(int)
        merged["min"] = np.min(all_cycles, axis=1).astype(int)

        out_file = f"{prefix}.merged"
        merged.to_csv(out_file, sep=" ", index=False, header=["n", "median", "max", "min"])
        print(f"✅ Wrote {out_file}")

def main():
    run_benchmarks()
    merge_results()

if __name__ == "__main__":
    main()
