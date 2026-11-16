# Rank queries on a live leaderboard

CS 253 / MA 253 data structures project. This project compares Prince's
augmented AVL tree and Saksham's compressed-score Fenwick tree through Madhav's
replay application and handwritten player table. Om's independent sorted-array
oracle checks both structures, and Bianca's generators supply difficult traces.
Sumit's fixed-board rank benchmark uses the same linked implementations.

Players sort by decreasing score and increasing ID within a tie. Scores
100, 90, 90, 70 have competition ranks 1, 2, 2, 4; positions are always unique.
Both backends support uint64 player IDs and int64 scores. Split assigns scores
at the cutoff to the high division. Split and merge consume their source IDs.
See `INTERFACE.md` for the complete command and error contract.

## Build and test

Requirements: CMake 3.16+, a C++17 compiler, and Python 3.8+ for tests.
No network, Git history, third-party C++ library, or Python package is needed.
From the project root:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 4
cmake -E chdir build ctest -C Release --output-on-failure
./build/bin/leaderboard avl app/examples/basic.trace
./build/bin/leaderboard fenwick app/examples/basic.trace
```

On Windows the executables have `.exe` suffixes. CMake places every configuration
in `build/bin`. Assertions stay enabled in release tests.

The `ctest` step above runs three million-command mixed traces and a scale test
with one million players and one million revisions. Allow about one to two
minutes for testing in a Release build. To run only the adversary suite:

```sh
python3 adversary/run.py --ops 1000000 --scale 1000000
```

Use `python` if that is your Python 3 executable. `--skip-build` reuses an
already-built tree; do not use it after changing C++ sources. The runner writes
command results and source hashes to `build/validation.json`.

For address/undefined-behavior checking with Clang or GCC:

```sh
cmake -S . -B build/sanitize -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
cmake --build build/sanitize --parallel 4
cmake -E chdir build/sanitize ctest --output-on-failure
```

## Project contents

- `src/structA/`: AVL insertion, deletion, rank, selection, range, split and merge.
- `src/structB/`: Fenwick counts, compressed scores and sorted tied-ID buckets.
- `app/`: handwritten open-addressing player table, replay and CLI.
- `oracle/`: independent reference/parser, backend invariant checks and reducer.
- `adversary/`: deterministic trace generation and integrated test runner.
- `bench/`: fixed 100-player rank microbenchmark.
- `docs/derivations/`: D1–D5 sources and PDFs. `D5.md` at the project root describes the workloads.
- `results/`: validation summary, benchmark output, and build environment.
- `REPORT.md`: design, complexity, and results.
- `TOOLS.md`: tool-use disclosure.

The CLI replays commands in memory. Fenwick collects all ADD and SET score
coordinates before replay. Updates can shift IDs within a tie bucket, and each
populated Fenwick board allocates arrays for the replay’s score domain.
