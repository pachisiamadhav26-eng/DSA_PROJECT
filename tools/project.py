"""Build, exercise and benchmark the live leaderboard from a fresh clone."""

import argparse
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build" / "project"
SUFFIX = ".exe" if os.name == "nt" else ""
BACKENDS = ("src/structA/avl.cpp", "src/structB/fenwick.cpp")
REPLAY = ("app/replay.cpp", "app/player_table.cpp")


def executable(name):
    return BUILD / (name + SUFFIX)


def run(*args, **options):
    return subprocess.run([str(value) for value in args], cwd=ROOT,
                          check=True, text=True, **options)


def build(compiler):
    BUILD.mkdir(parents=True, exist_ok=True)
    programs = {
        "leaderboard": ("app/leaderboard.cpp", *REPLAY, *BACKENDS),
        "oracle": ("oracle/main.cpp",),
        "differential": ("oracle/differential.cpp", *BACKENDS),
        "avl_test": ("src/structA/tests/avl_test.cpp", BACKENDS[0]),
        "fenwick_test": ("src/structB/tests/fenwick_test.cpp", BACKENDS[1]),
        "player_table_test": ("app/tests/player_table_test.cpp", REPLAY[1]),
        "replay_test": ("app/tests/replay_test.cpp", *REPLAY),
    }
    if (ROOT / "adversary/generator.cpp").exists():
        programs["generator"] = ("adversary/generator.cpp",)
    if (ROOT / "bench/leaderboard_bench.cpp").exists():
        programs["leaderboard_bench"] = ("bench/leaderboard_bench.cpp", *BACKENDS)
    flags = ("-std=c++17", "-O2", "-Wall", "-Wextra", "-Wpedantic", "-Iinclude", "-Iapp")
    objects = {}
    for name, sources in programs.items():
        for source in sources:
            if source not in objects:
                obj = BUILD / (source.replace("/", "_") + ".o")
                run(compiler, *flags, "-c", source, "-o", obj)
                objects[source] = obj
        run(compiler, *(objects[source] for source in sources), "-o", executable(name))
    print(f"Built {len(programs)} executables in {BUILD.relative_to(ROOT)}", flush=True)


def demo():
    for backend in ("avl", "fenwick"):
        print(f"Sample replay with {backend}", flush=True)
        run(executable("leaderboard"), backend, "app/examples/basic.trace", "--audit")


def test(full):
    for name in ("player_table_test", "replay_test", "avl_test", "fenwick_test"):
        run(executable(name))
        print(f"PASS {name}", flush=True)

    for trace in sorted((ROOT / "oracle/tests").glob("*.trace")):
        expected = trace.with_suffix(".expected").read_text(encoding="utf-8")
        for backend in ("oracle", "avl", "fenwick"):
            command = (executable("oracle"), trace) if backend == "oracle" else (
                executable("leaderboard"), backend, trace, "--audit")
            actual = run(*command, capture_output=True).stdout
            if actual != expected:
                raise RuntimeError(f"{backend} output differs for {trace.name}")
        run(executable("differential"), "--trace", trace)
        print(f"PASS golden {trace.stem} with oracle, AVL and Fenwick", flush=True)

    for seed in (253, 42, 20261112):
        operations = (1000000 if seed == 253 else 100000) if full else 10000
        run(executable("differential"), "--seed", seed, "--ops", operations, "--max-live", 48)
    run(executable("differential"), "--scale", 1000000 if full else 10000)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", default=os.environ.get("CXX", "g++"))
    parser.add_argument("--skip-build", action="store_true")
    parser.add_argument("--full", action="store_true", help="use release-scale test and bench inputs")
    parser.add_argument("action", choices=("build", "demo", "test", "adversary", "bench", "all"))
    args = parser.parse_args()
    if args.action == "build" or not args.skip_build:
        build(args.compiler)
    if args.action in ("demo", "all"):
        demo()
    if args.action in ("test", "all"):
        test(args.full)
    if args.action in ("adversary", "all"):
        run(sys.executable, "adversary/run.py", "--no-build",
            "--players", 1000000 if args.full else 10000,
            "--revisions", 1000000 if args.full else 10000)
    if args.action in ("bench", "all"):
        run(sys.executable, "bench/run.py", "--compiler", args.compiler,
            *([] if args.full else ["--quick"]))


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(error, file=sys.stderr)
        sys.exit(1)
