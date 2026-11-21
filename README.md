# Project 02: Rank Queries on a Live Leaderboard

CS 253 / MA 253. Team name will be added when the team confirms it.

## Team and roles

| Name | Roll no. | GitHub | Role | Module folder | Derivation |
| --- | --- | --- | --- | --- | --- |
| Prince Chavda | 250041011 | @princechavda8773 | Structure A owner | `src/structA/` | D1 |
| Saksham Bhatia | 250041034 | @SakshamBhatia07 | Structure B owner | `src/structB/` | D2 |
| Madhav Pachisia | 250041025 | @pachisiamadhav26-eng | Architect | `app/`, `include/` | D3 |
| Om Dehariya | 250041029 | @OmDehariya111 | Oracle owner | `oracle/` | D4 |
| Bianca (surname to confirm) | 250041008 | @Bianca-the-sensational | Adversary | `adversary/` | D5 |
| Sumit (surname to confirm) | 250041038 | @sumitdobal04 | Analyst | `bench/`, `results/` | D6 |

The team name and two incomplete names require confirmation before the final tag.

## Summary

The application replays player score changes, competition-rank and position queries, and season split/merge commands. Structure A is an AVL tree augmented with subtree sizes; Structure B is a Fenwick tree over compressed scores with ordered ID buckets for ties. Both use the same `Board` interface and are checked against an independent sorted-array Oracle. In the recorded million-player experiments, Fenwick answered rank/select faster, while AVL split and separated-league rejoin were much faster; see [REPORT.md](REPORT.md) and the raw data for the exact workloads and spread.

## Requirements

- C++17 compiler: **GCC 7 or newer** for the complete build, including `adversary/generator.cpp`. Bianca validated GCC 14.2.0 on Windows. GCC 6.3 compiles the other targets but cannot parse the generator's structured bindings and fold expression.
- Python 3.8 or newer; tested with Python 3.13 on Windows. No external C++ container is used for the AVL tree, Fenwick tree, or application player table.
- Install plot and PDF dependencies: `python -m pip install -r bench/requirements.txt`.
- The Python build and test scripts use portable executable paths. `app/web/server.cpp` is Windows only. `scripts/run_all.sh` needs a POSIX shell; `scripts/run_all.ps1` is the Windows equivalent.
- Recorded formal measurements used MinGW GCC 6.3 for the unchanged AVL, Fenwick, and benchmark sources. Their exact compiler, flags, source hashes, and machine description are in `results/full-20261117-final/metadata.json`.

## Build

From a fresh clone at the repository root, with GCC 7+ on `PATH`:

```sh
python scripts/build.py
```

This builds the application, Oracle, differential checker, generator, four unit tests, benchmark runner, and (on Windows) web demo into `build/bin/`. On Windows, pass a compiler path if needed: `python scripts/build.py --compiler C:/msys64/ucrt64/bin/g++.exe`. `CMakeLists.txt` is an optional alternative for a recent CMake installation.

## Quick start

```powershell
./build/bin/leaderboard.exe avl app/examples/basic.trace
./build/bin/leaderboard.exe fenwick app/examples/basic.trace
```

On Linux or macOS, omit `.exe`. Both commands produce the same 12 lines in [app/examples/basic.expected](app/examples/basic.expected):

```text
OK
OK
OK
OK
RANK 2
PLAYER 8 90
PLAYERS 2 3 90 8 90
OK
SIZE 3
OK
OK
RANK 1
```

For the optional Windows browser demo, run `./build/bin/leaderboard_web.exe 25401` and visit `http://127.0.0.1:25401/`.

## Tests

```sh
python scripts/test.py --quick
python scripts/test.py
```

The full command runs one million differential operations with seed **253**, a separate one-million-player scale check, unit tests, invariant checks, all submitted Oracle and Adversary fixtures, and generated adversarial cases when the generator is built. A successful run ends with `ALL TESTS PASSED`. The quick form uses 10,000 operations and players. Tests are also in the top-level `tests/` and `oracle/`; the Oracle's detailed runner is `./oracle/run_tests.ps1` on Windows.

## Evaluation interface (hidden tests)

The CLI accepts a backend and a **trace file path**, not standard input:

```sh
build/bin/leaderboard avl path/to/ops.trace > avl.out
build/bin/leaderboard fenwick path/to/ops.trace > fenwick.out
```

On Windows, use `build/bin/leaderboard.exe`. Each UTF-8 trace line contains one command from [INTERFACE.md](INTERFACE.md), for example `ADD 0 5 100` then `RANK 0 5`. Board `0` starts empty. Each command produces one line such as `OK`, `RANK 1`, `PLAYER id score`, `PLAYERS count id score ...`, `SIZE count`, or `ERR code`. Scores are signed 64-bit integers; IDs and board numbers are unsigned. Positions start at one, higher scores come first, and tied players have the same competition rank but appear in increasing ID order. The course's exact hidden-test invocation has not yet been published; this is the current executable contract.

## Benchmarks and figures

```sh
./scripts/run_all.sh
```

On Windows use `./scripts/run_all.ps1`; `-Quick` runs a smaller rehearsal. The command builds, tests, runs the sample on both backends, executes the Analyst suite, and regenerates the 29 figures and `results/summary.csv` from `results/raw/`. The full suite includes five repetitions and million-player workloads; runtime depends on the machine. The original run settings and timings are in `results/full-20261117-final/metadata.json`. To regenerate only the committed figures from committed raw CSVs, run `python results/plot.py results`. The figure-to-CSV map is [results/figures/INDEX.md](results/figures/INDEX.md).

## Repository map

| Folder | Contents |
| --- | --- |
| `include/` | Shared `Board` API and backend audit helpers. |
| `src/structA/`, `src/structB/` | AVL and Fenwick implementations. |
| `app/` | Replay application, handwritten player table, examples, optional Windows web demo. |
| `oracle/` | Sorted-array reference, differential and invariant checks, golden traces. |
| `adversary/` | Generator, validation script, adversarial fixtures. |
| `bench/` | Harness, counters, profiling scripts and benchmark checks. |
| `tests/` | Unit tests for application and both structures. |
| `scripts/` | One-command build, test and full pipeline. |
| `results/raw/`, `results/figures/` | Five-run CSV observations, generated figures and their source map. |
| `docs/derivations/`, `docs/slides/` | Six individual derivation PDFs and presentation. |

## Results at a glance

At one million players and `U = 1,000,000` possible scores, the recorded five-run means were:

| Operation | AVL | Fenwick |
| --- | ---: | ---: |
| Rank | 939.02 ns/query | 342.74 ns/query |
| Select | 1,046.16 ns/query | 625.85 ns/query |
| Split near midpoint | 5.76 microseconds | 37.21 milliseconds |
| Rejoin separated divisions | 8.20 microseconds | 286.05 milliseconds |

These are measurements on one machine and one workload, not universal guarantees. Five-run spread, exact inputs, prediction comparisons, and cache-sampling limits appear in [REPORT.md](REPORT.md).

## Known limitations

- The generator needs GCC 7+; GCC 6.3 on Madhav's laptop cannot build it. The other integrated targets and quick checks were built there with GCC 6.3.
- `app/web/` is a Windows demonstration interface and is not the evaluation CLI.
- Team name, Bianca's and Sumit's full surnames, and instructor account await confirmation. Repository name, privacy setting, and checkpoint tags must be set by the team before submission.
- The instructor has not yet supplied a project-specific hidden-test command. If it differs from the documented trace-file CLI, the adapter and this section must be updated before the `final` tag.
- The cost model is symbolic; it predicts growth, not exact elapsed nanoseconds. Query-node visits were not instrumented, and cache recordings are sampled rather than exact miss totals.

## Tools and references

See [TOOLS.md](TOOLS.md) for declared assistance and [REPORT.md](REPORT.md) for references. Main readings include Adelson-Velsky and Landis on AVL trees, Fenwick's cumulative-frequency tree, and Blelloch, Ferizovic and Sun on ordered-set join.
