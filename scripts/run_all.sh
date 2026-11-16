#!/usr/bin/env sh
set -eu
cd "$(dirname "$0")/.."
build="${BUILD_DIR:-build}"
python="${PYTHON:-python3}"

cmake -S . -B "$build" -DCMAKE_BUILD_TYPE=Release
cmake --build "$build" --config Release --parallel 4
cmake -E chdir "$build" ctest -C Release --output-on-failure
"$build/bin/leaderboard" avl app/examples/basic.trace
"$build/bin/leaderboard" fenwick app/examples/basic.trace
"$build/bin/scaling_bench" > "$build/measurements.csv"
"$python" bench/plot.py results --collect "$build/measurements.csv"
