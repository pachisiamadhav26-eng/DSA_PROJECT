# Oracle and correctness tests

Owner: Om Dehariya. The independent C++17 sorted-vector model follows
`INTERFACE.md` and `include/leaderboard.hpp` in this checkout.

## Build and test

From the repository root, with a C++17 `g++` on PATH, run in PowerShell:

```powershell
./oracle/run_tests.ps1 -Quick
./oracle/run_tests.ps1 -SkipBuild
```

The first test command builds the oracle, differential runner and actual app.
It checks golden fixtures in all three executables and exercises 10,000 commands
per seed. The second reuses the binaries and runs seed 253 for 1,000,000
commands, two other seeds for 100,000 commands each, and a million-player
test. Use `-MaxLive` to set the bounded generator's live population
limit (default 48). The runner prints the measured peak occupancy.

The default build compiles `src/structA/avl.cpp`, `src/structB/fenwick.cpp`,
and `app/` directly from this checkout with its shared headers. It requires
no dependency fetch or historical teammate commits. `build/oracle/sources.txt`
records SHA256 hashes and paths of the actual application and backend inputs.
The compiler can be set with `-Compiler PATH` on `oracle/build.ps1`.

The pinned revisions in older validation entries describe historical runs.
To reproduce a historical run, check out the commit named in that entry.
The current runner always tests the current checkout.

## What the checks establish

- The oracle uses binary search on a sorted array. `contains` matches both
  score and ID. Ties share competition rank and are selected by increasing ID
  within the score. Positions are one-based. Range bounds are inclusive.
- The differential runner creates the real AVL and Fenwick factories and
  compares outputs, ordered state, lookup state and exact membership to the
  independent model after each line. It calls each backend's structural audit.
  Fenwick receives all scores from ADD and SET before replay.
- The golden tests also call Madhav's executable twice, once per backend.
  This exercises the production parser and handwritten player table. The
  independent Session used in the differential runner uses test-only maps.
- Section 3 of `INTERFACE.md` rules out destination IDs that currently
  exist. The current app therefore allows reuse after a board is consumed.
  Blank lines are skipped, and identical board arguments use SAME_BOARD
  with the app's validation order. The error fixture checks these cases.
- The million-player test bulk-checks the reference, issues rank/select queries
  and compares each split partition and the joined board. Its full audits
  occur at phase boundaries. The bounded million-command run audits after
  every operation. These runs cover different workloads.
- The legacy checks and corruption tests in the historical validation
  record apply to the old standalone snapshots. Production audits are
  exercised through the current inspection functions.

## Code reading order

| File | What to explain |
| --- | --- |
| `include/leaderboard.hpp` | The shared Board operations. AVL, Fenwick and the oracle expose the same interface. |
| `oracle/sorted_array.hpp` | Keep a sorted vector. Find a key with binary search, move entries when inserting or erasing, and merge two ordered vectors. |
| `oracle/replay.hpp` | Parse a line, check the board and player's location, then call the Board operation. |
| `oracle/main.cpp` | Read a trace file or standard input and print each answer. |
| `oracle/trace.hpp` | Generate a reproducible command list, or read and write a trace file. |
| `oracle/differential.cpp` | Run each command on three implementations, compare answers and state, and reduce a failing trace. |
| `oracle/backend_checks.hpp` | Test exact membership, rotations, split/join and the million-player case. |
| `oracle/build.ps1`, `run_tests.ps1` | Build the current checkout and compare both real app modes to the golden fixtures. |
| [D4 source](../docs/derivations/D4.md) and [D4 PDF](../docs/derivations/D4_250041029.pdf) | The two required proofs about AVL subtree sizes and selection. |
| `docs/derivations/render_d4.py` | Build the submission PDF from its English source with ReportLab. |

`override` tells the compiler to check that a method matches the shared
Board interface. `unique_ptr` owns a board. `std::move` transfers that
ownership when split or merge changes which board ID owns it.

Try explaining these small changes aloud:

- If `select` uses `position` instead of `position - 1`, every answer
  moves one place and the last position goes outside the vector.
- If the rank search counts scores equal to the target, tied players get
  the wrong rank. A player's rank is one plus the number of strictly higher scores.
- If a successful split keeps its old board ID active, a later query can use
  a board the interface says has already been consumed.
- If a reducer drops a line without rerunning the trace, it can remove the
  command that caused the failure and save a passing trace.

```powershell
./build/oracle/oracle.exe oracle/tests/basic.trace
./build/oracle/differential.exe --trace oracle/tests/seasons.trace
```

For a discrepancy, retain `build/oracle/sources.txt` and the console output.
The differential runner saves `oracle-failure.trace` and
`oracle-failure.min.trace`. The second trace is 1-minimal for the reported
failure signature under command deletion. A crash outside ordinary C++
exception handling requires external debugging. The runner checks its
shrinker separately with a deliberately injected rank failure.
