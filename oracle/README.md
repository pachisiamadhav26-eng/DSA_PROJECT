# Oracle and correctness tests

Owner: Om Dehariya. The independent C++17 sorted-vector model follows
`INTERFACE.md` and `include/leaderboard.hpp` on Madhav's branch.

## Build and test

From the repository root, with Git and a C++17 `g++` on PATH, run in PowerShell:

```powershell
git fetch origin
./oracle/run_tests.ps1 -Quick
./oracle/run_tests.ps1 -SkipBuild
```

The first test command builds the oracle, differential runner and actual app.
It checks golden fixtures in all three executables and exercises 10,000 commands
per seed. The second reuses the binaries and runs seed 253 for 1,000,000
commands, two other seeds for 100,000 commands each, and a million-player
test. Use `-MaxLive` to set the bounded generator's live population
limit (default 48). The runner prints the measured peak occupancy.

The default build resolves these verified revisions and writes the exact
hashes to `build/oracle/sources.txt`:

| Source | Commit |
| --- | --- |
| Architect's shared header and replay app | `a4dc4382b11d60ed85697fb9560574d24a8d2366` |
| Prince's production AVL | `03f3f57186f96bac6b2ad8f22acbedd7da2de153` |
| Saksham's production Fenwick and common audit declarations | `c921385a635deb2bd6ff8ca70ec67cf554752fed` |

The build exports dependencies to the ignored build directory and verifies
that the checked-in header matches Madhav's header. Future revisions can be
selected with `-ArchitectRef`, `-AvlRef` and `-FenwickRef` on `build.ps1`.
Then use `run_tests.ps1 -SkipBuild` to exercise those binaries. The compiler
can be set with `-Compiler PATH` on the build script.

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
- Section 3 of Madhav's interface rules out destination IDs that currently
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
