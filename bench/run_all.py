"""Build, check, and run the Analyst experiments."""

import argparse
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parent.parent


def run(script, *arguments):
    subprocess.run([sys.executable, script, *map(str, arguments)], cwd=ROOT, check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--suite", choices=["quick", "full"], default="quick")
    parser.add_argument("--compiler", default="g++")
    parser.add_argument("--local", action="store_true", help="Build an integrated checkout")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--seed", type=int, default=253)
    parser.add_argument("--trace", type=Path, action="append", default=[])
    parser.add_argument("--check-trace", action="store_true", help="Check small traces against Om's oracle")
    parser.add_argument("--machine-json", type=Path)
    args = parser.parse_args()

    build = ["--compiler", args.compiler, "--avl-tools", "--fenwick-tools"]
    if args.local:
        build.append("--local")
    run("bench/build.py", *build)
    run("bench/tests/check_runner.py")
    run("bench/tests/check_results.py")

    experiment = ["--suite", args.suite, "--seed", str(args.seed)]
    for name in ("output", "machine_json"):
        value = getattr(args, name)
        if value is not None:
            experiment += ["--" + name.replace("_", "-"), str(value)]
    for path in args.trace:
        experiment += ["--trace", str(path)]
    if args.check_trace:
        experiment.append("--check-trace")
    run("bench/run.py", *experiment)


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as error:
        sys.exit(error.returncode)
