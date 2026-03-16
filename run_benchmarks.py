#!/usr/bin/env python3

import subprocess
import glob
import os
import pandas as pd
import numpy as np
import re
import argparse

# === CONFIG ===
WARUMUPS     = 0
RUNS         = 1
ALGOS        = ["fvs", "nest", "beta"]
SETS         = ["trie", "immer", "set"]
ROWS         = [0, 1, 2]
TASKSET_MASK = "0x1"         # CPU core mask (e.g. 0x1 = core 0)
OPT          = "opt"

ITERS = {
    "trie" : {
        0: 10,
        1: 10,
        2: 10,
    },
    "immer" : {
        0: 10,
        1: 10,
        2: 10,
    },
    "set" : {
        0: 10,
        1: 10,
        2: 9,
    },
}

parser = argparse.ArgumentParser()
parser.add_argument("--warmups",     action="store_true", help="use 3 warmups before measuring")
parser.add_argument("--runs",        action="store_true", help="measure 9 runs and compute median instead of just 1")
parser.add_argument("--all-iters",   action="store_true", help="by default only a limited number of n; this option includes all `n's")
parser.add_argument("--remove-llvm", action="store_true", help="removes *.ll files and recreates them during benchmarking; this takes a while!")
args = parser.parse_args()

if args.warmups:
    WARMUP = 3

if args.runs:
    RUNS = 9

if args.all_iters:
    ITERS = {
        "trie" : {
            0: 20,
            1: 20,
            2: 11,
        },
        "immer" : {
            0: 20,
            1: 13,
            2: 11,
        },
        "set" : {
            0: 15,
            1: 13,
            2: 10,
        },
    }

if args.remove_llvm:
    print("remove all *.ll files")
    for ll in glob.glob("*.ll"):
        os.remove(ll)

def run_cmd(cmd, capture_output=False):
    cmd = f"taskset {TASKSET_MASK} {cmd}"
    if not capture_output:
        cmd = f"ulimit -s unlimited && {cmd}"
    print(f" → Executing: '{cmd}'")
    return subprocess.run(cmd, shell=True, check=True, text=True, capture_output=capture_output).stderr

def run_mimir_benchmarks():
    for set in SETS:
        for row in ROWS:
            bench = f"release_{set}/bin/bench"
            iter  = ITERS[set][row]
            # --- Warmup runs ---
            print(f"Performing {WARUMUPS} warmup runs (results ignored)...")
            for i in range(1, WARUMUPS + 1):
                suffix = f"warmup{i}"
                cmd    = f"{bench} {iter} {row} {suffix}"
                run_cmd(cmd)
            print("Warmup complete.\n")

            # --- Actual measurement runs ---
            for i in range(1, RUNS + 1):
                print(f"Running benchmark {i}/{RUNS} pinned to core mask {TASKSET_MASK} ...")
                suffix = f"run{i}"
                cmd    = f"{bench} {iter} {row} {suffix}"
                run_cmd(cmd)

def extract_wall_time(opt_out, pass_name):
    # Regex to match a line from the "Pass execution timing report" section.
    # It captures the four numeric columns (User, System, User+System, Wall)
    # and the pass name that follows.
    pattern = re.compile(
        r'^\s*'                                         # leading whitespace
        r'(?:(\d+\.\d+)\s+\(\s*\d+(?:\.\d+)?%\)\s*)?'   # User time + percentage (is missing sometimes)
        r'(\d+\.\d+)\s+\(\s*\d+(?:\.\d+)?%\)\s*'        # System time
        r'(\d+\.\d+)\s+\(\s*\d+(?:\.\d+)?%\)\s*'        # User+System
        r'(\d+\.\d+)\s+\(\s*\d+(?:\.\d+)?%\)\s*'        # Wall time (this is group 4)
        r'(.*)$'                                        # Pass name (rest of the line)
    )

    for line in opt_out.splitlines():
        match = pattern.match(line)
        if match:
            wall_time_str = match.group(4)
            name_on_line  = match.group(5).strip()
            if name_on_line == pass_name:
                return int(float(wall_time_str) * 1000000)

    return 0

def run_llvm_benchmarks():
    for row in ROWS:
        lls = []
        for ll in glob.glob(f"{row}.*.ll"):
            # Extract the part between the first dot and the last ".ll"
            # Example: "0.16.ll" -> parts = ["0", "16", "ll"]
            parts = ll.split('.')
            if len(parts) == 3:
                num_str = parts[1]
                try:
                    num = int(num_str)
                    lls.append((num, ll))
                except ValueError:
                    print(f"Warning: '{num_str}' in {ll} is not an integer.")
            else:
                print(f"Warning: {ll} does not match expected format.")

        lls.sort(key = lambda num_ll: num_ll[0])

        for n, ll in lls:
            # dominance
            cmd = f"{OPT} -passes='require<domtree>' -disable-output -time-passes {ll}"

            print(f"Performing {WARUMUPS} warmup runs (results ignored)...")
            for _ in range(1, WARUMUPS + 1):
                run_cmd(cmd, capture_output=True)
            print("Warmup complete.\n")

            # --- Actual measurement runs ---
            for i in range(1, RUNS + 1):
                with open(f"{row}.dom.run{i}", "w" if n == 1 else "a") as f:
                    if n == 1:
                        f.write("% n ms\n")
                    opt_out = run_cmd(cmd, capture_output=True)
                    t = extract_wall_time(opt_out, "RequireAnalysisPass<llvm::DominatorTreeAnalysis, llvm::Function, llvm::AnalysisManager<Function>>")
                    f.write(f"{n} {t}\n")

            # inline

            cmd = f"{OPT} -passes='inline' -disable-output -time-passes {ll}"

            print(f"Performing {WARUMUPS} warmup runs (results ignored)...")
            for _ in range(1, WARUMUPS + 1):
                run_cmd(cmd, capture_output=True)
            print("Warmup complete.\n")

            # --- Actual measurement runs ---
            for i in range(1, RUNS + 1):
                with open(f"{row}.inl.run{i}", "w" if n == 1 else "a") as f:
                    opt_out = run_cmd(cmd, capture_output=True)
                    t = extract_wall_time(opt_out, "InlinerPass")
                    f.write(f"{n} {t}\n")

            # inline + optimize

            cmd = f"{OPT} -passes='inline,instcombine,early-cse,dce,unreachableblockelim' -disable-output -time-passes {ll}"
            print(f"Performing {WARUMUPS} warmup runs (results ignored)...")
            for _ in range(1, WARUMUPS + 1):
                run_cmd(cmd, capture_output=True)
            print("Warmup complete.\n")

            # --- Actual measurement runs ---
            for i in range(1, RUNS + 1):
                with open(f"{row}.opt.run{i}", "w" if n == 1 else "a") as f:
                    opt_out = run_cmd(cmd, capture_output=True)
                    t = 0
                    t += extract_wall_time(opt_out, "DCEPass")
                    t += extract_wall_time(opt_out, "EarlyCSEPass")
                    t += extract_wall_time(opt_out, "InlinerPass")
                    t += extract_wall_time(opt_out, "InstCombinePass")
                    t += extract_wall_time(opt_out, "UnreachableBlockElimPass")
                    f.write(f"{n} {t}\n")

def merge(prefix):
    g = f"{prefix}.run*"
    files = sorted(glob.glob(g))
    if not files:
        print(f"No files found for `{g}`. Skipping.")
        return

    dfs = []
    for f in files:
        df = pd.read_csv(f, sep='\\s+', comment="%", names=["n", "cycles"])
        dfs.append(df)

    merged = dfs[0][["n"]].copy()
    all_cycles = np.stack([df["cycles"].to_numpy() for df in dfs], axis=1)

    merged["median"] = np.median(all_cycles, axis=1).astype(int)
    merged["max"]    = np.max   (all_cycles, axis=1).astype(int)
    merged["min"]    = np.min   (all_cycles, axis=1).astype(int)

    out_file = f"{prefix}.merged"
    merged.to_csv(out_file, sep=" ", index=False, header=["n", "median", "max", "min"])
    print(f"✅ Wrote {out_file}")

def merge_mimir_results():
    for set in SETS:
        for row in ROWS:
            for algo in ALGOS:
                merge(f"{set}.{algo}.{row}")

def merge_llvm_results():
    for row in ROWS:
        for algo in ["dom", "inl", "opt"]:
            merge(f"{row}.{algo}")

def make_figure():
    os.chdir('tex')
    run_cmd('pdflatex bench.tex && pdflatex bench.tex')
    os.chdir('..')

def main():
    run_mimir_benchmarks()
    run_llvm_benchmarks()
    merge_mimir_results()
    merge_llvm_results()
    make_figure ()

if __name__ == "__main__":
    main()
