# Project 02: Rank Queries on a Live Leaderboard

CS 253 / MA 253. Team **PEAKY BLINDERS**.

## Team and roles

| Name | Roll no. | GitHub | Role | Module folder | Derivation |
| --- | --- | --- | --- | --- | --- |
| Chavda Kalpeshbhai Prince | 250041011 | @princechavda8773 | Structure A owner | `src/structA/` | D1 |
| Saksham Bhatia | 250041034 | @SakshamBhatia07 | Structure B owner | `src/structB/` | D2 |
| Madhav Pachisia | 250041025 | @pachisiamadhav26-eng | Architect | `app/`, `include/` | D3 |
| Om Dehariya | 250041029 | @OmDehariya111 | Oracle owner | `oracle/` | D4 |
| Bianca Jain | 250041008 | @Bianca-the-sensational | Adversary | `adversary/` | D5 |
| Sumit | 250041038 | @sumitdobal04 | Analyst | `bench/`, `results/` | D6 |

## Summary

The application replays player score changes, competition-rank and position queries, and season split/merge commands. Structure A is an AVL tree augmented with subtree sizes; Structure B is a Fenwick tree over compressed scores with ordered ID buckets for ties. Both use the same `Board` interface and are checked against an independent sorted-array Oracle. In the recorded million-player experiments, Fenwick answered rank/select faster, while AVL split and separated-league rejoin were much faster; see [REPORT.md](REPORT.md) and the raw data for the exact workloads and spread.

## Requirements

- A C++17 compiler. Use **GCC 7 or newer** for the full build; GCC 6.3 cannot compile the adversarial generator.
- Python 3.8 or newer for the build, tests, and benchmark scripts.
- To regenerate charts, install the packages in `bench/requirements.txt` with `python -m pip install -r bench/requirements.txt`.
- A POSIX shell for `scripts/run_all.sh`, or PowerShell for `scripts/run_all.ps1`. The optional browser demo runs on Windows.

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

- The browser demo runs on Windows. The command-line program is the interface used for replay and testing.
- Fenwick score buckets can be slow to update when many players have the same score. Its split and merge operations rebuild counts; an AVL merge of overlapping score ranges may also rebuild.
- The timing results come from one machine and the recorded workloads. Cache events were sampled, and query-node visits were not measured.

## Before submission

Add the instructor's GitHub account as a collaborator, and resolve the missing checkpoint tags. If the course provides a different hidden-test command, update the documented CLI instructions to match.

## Tools and references

See [TOOLS.md](TOOLS.md) for declared assistance and [REPORT.md](REPORT.md) for references. Main readings include Adelson-Velsky and Landis on AVL trees, Fenwick's cumulative-frequency tree, and Blelloch, Ferizovic and Sun on ordered-set join.
