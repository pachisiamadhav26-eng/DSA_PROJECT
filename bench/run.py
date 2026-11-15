"""Run the production benchmark and summarize its five-trial measurements."""

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
EXE = ROOT / "build" / "project" / ("leaderboard_bench.exe" if os.name == "nt" else "leaderboard_bench")


def run(*command, **options):
    subprocess.run([str(arg) for arg in command], cwd=ROOT, check=True, **options)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", default=os.environ.get("CXX", "g++"))
    parser.add_argument("--quick", action="store_true")
    parser.add_argument("--cache", action="store_true", help="also capture process totals with Linux perf")
    parser.add_argument("--output", type=Path, default=ROOT / "build" / "benchmark")
    parser.add_argument("--build", action="store_true", help="recompile before running")
    args = parser.parse_args()
    if args.build or not EXE.is_file():
        run(sys.executable, "tools/project.py", "--compiler", args.compiler, "build")
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    raw = output / "benchmark_trials.csv"
    with raw.open("wb") as stream:
        run(EXE, *(["--quick"] if args.quick else []), stdout=stream)
    run(sys.executable, "results/summarize.py", raw)
    if args.cache:
        perf = shutil.which("perf")
        if not perf:
            raise RuntimeError("Linux perf is required for --cache")
        for backend in ("avl", "fenwick"):
            with (output / f"profile-{backend}-queries.csv").open("wb") as stream:
                run(perf, "stat", "-r", "5", "-x", ",", "-e",
                    "cache-references,cache-misses,cycles,instructions",
                    "-o", output / f"cache-{backend}.csv", "--", EXE, "--profile", backend,
                    stdout=stream)
    print(f"Benchmark CSV and figures written to {output}", flush=True)


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(error, file=sys.stderr)
        sys.exit(1)
