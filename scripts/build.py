"""Build the integrated C++ application, tests, oracle, and benchmark."""

import argparse
import os
from pathlib import Path
import subprocess


ROOT = Path(__file__).resolve().parent.parent
BIN = ROOT / "build" / "bin"
SUFFIX = ".exe" if os.name == "nt" else ""


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", default=os.environ.get("CXX", "g++"))
    parser.add_argument("--skip-generator", action="store_true",
                        help="build the other targets when only an older local compiler is available")
    args = parser.parse_args()
    BIN.mkdir(parents=True, exist_ok=True)

    common = [args.compiler, "-std=c++17", "-O2", "-Wall", "-Wextra",
              "-Wpedantic", "-UNDEBUG", "-I", ROOT,
              "-I", ROOT / "include", "-I", ROOT / "app", "-I", ROOT / "oracle"]
    app = ["app/replay.cpp", "app/player_table.cpp"]
    backends = ["src/structA/avl.cpp", "src/structB/fenwick.cpp"]

    def build(name, sources, definitions=(), libraries=()):
        output = BIN / (name + SUFFIX)
        command = [*common, *definitions, *(ROOT / source for source in sources),
                   *libraries, "-o", output]
        print("Building", name, flush=True)
        subprocess.run([str(part) for part in command], cwd=ROOT, check=True)

    build("leaderboard", ["app/leaderboard.cpp", *app, *backends])
    build("oracle", ["oracle/main.cpp"])
    build("differential", ["oracle/differential.cpp", *backends])
    if not args.skip_generator:
        build("generator", ["adversary/generator.cpp"])
    build("player_table_test", ["tests/player_table_test.cpp", "app/player_table.cpp"])
    build("replay_test", ["tests/replay_test.cpp", *app])
    build("avl_test", ["tests/avl_test.cpp", *app])
    build("fenwick_test", ["tests/fenwick_test.cpp", "src/structB/fenwick.cpp"])
    build("bench_runner", ["bench/runner.cpp", *app, *backends],
          ["-DBENCH_HAS_AVL", "-DBENCH_HAS_FENWICK", "-DBENCH_ORACLE",
           "-DBENCH_AVL_TOOLS", "-DBENCH_FENWICK_TOOLS"],
          [] if os.name == "nt" else ["-pthread"])
    if os.name == "nt":
        build("leaderboard_web", ["app/web/server.cpp", *app, *backends],
              libraries=["-lws2_32"])
    print("Build complete:", BIN, flush=True)


if __name__ == "__main__":
    main()
