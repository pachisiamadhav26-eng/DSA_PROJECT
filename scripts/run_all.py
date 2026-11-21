"""Build, test, demonstrate both backends, rerun benchmarks, and regenerate figures."""

import argparse
import os
from pathlib import Path
import subprocess
import sys


ROOT = Path(__file__).resolve().parent.parent
SUFFIX = ".exe" if os.name == "nt" else ""


def run(command, output=None):
    with subprocess.Popen([str(x) for x in command], cwd=ROOT, stdout=subprocess.PIPE,
                          text=True) as process:
        lines = process.stdout.read()
        if process.wait():
            raise RuntimeError(f"{' '.join(map(str, command))} failed")
    if output is not None:
        output.write_text(lines, encoding="utf-8")
    else:
        print(lines, end="", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--quick", action="store_true")
    parser.add_argument("--compiler", default=os.environ.get("CXX", "g++"))
    args = parser.parse_args()
    run([sys.executable, "scripts/build.py", "--compiler", args.compiler])
    run([sys.executable, "scripts/test.py", *(["--quick"] if args.quick else [])])
    for backend in ("avl", "fenwick"):
        output = ROOT / "build" / ("demo-" + backend + ".txt")
        run([ROOT / "build/bin" / ("leaderboard" + SUFFIX), backend,
             ROOT / "app/examples/basic.trace"], output=output)
        if output.read_text(encoding="utf-8").splitlines() != (
                ROOT / "app/examples/basic.expected").read_text(encoding="utf-8").splitlines():
            raise RuntimeError(f"{backend} demo output mismatch")
        print("PASS demo", backend, flush=True)
    run([sys.executable, "bench/run_all.py", "--local", "--suite",
         "quick" if args.quick else "full", "--compiler", args.compiler])
    run([sys.executable, "results/plot.py", "results"])
    print("ALL PIPELINE STEPS PASSED", flush=True)


if __name__ == "__main__":
    main()
