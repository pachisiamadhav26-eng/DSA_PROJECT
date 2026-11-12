# Oracle and correctness tests

Owner: **Om Dehariya**. C++17, Project 2, rank queries on a live leaderboard.

This module provides a sorted-array reference, a replay executable, differential
tests against the team's actual AVL and Fenwick code, structural invariant
checks, and a reducer for failing command sequences. The editable D4 proof and
its PDF are in [`docs/derivations`](../docs/derivations/).

## Run from the repository root

With Git and a C++17 `g++` on PATH, in PowerShell:

```powershell
git fetch origin
./oracle/run_tests.ps1
```

This builds both executables, checks four hand-written golden fixtures, runs
seed 253 with **1,000,000 commands**, two additional seeds with **100,000 commands
each**, and checks **1,000,000 simultaneous
players plus 1,000,000 queries** and split/rejoin. For a short development run:

```powershell
./oracle/run_tests.ps1 -Quick
```

`-SkipBuild` reuses existing executables. An alternative compiler can be selected
with `./oracle/build.ps1 -Compiler C:/path/to/g++.exe`. Binaries and exported
dependencies live in ignored `build/oracle/`.

The build script exports the following immutable teammate sources using `git
show`; it does not switch branches or change their code:

| Dependency | Original branch | Commit |
| --- | --- | --- |
| `avl.cpp` | `prince-chavda` | `2db28739e291a07bf65c68b44db6a86fbd187b91` |
| `Fenvick.cpp` | `Saksham` | `de0ab032ce8599d1ef7c9be048377c23cd571327` |
| Shared header and basic example | `madhav` | `1e7434545e3809394bbc7dffa627eb9008102c0e` |

The shared header is included at its intended repository path and unchanged
in content, so it can coexist with the architect's contribution. The basic
fixture is copied into this module's test directory. To test later backend
commits, update the two pins in `build.ps1`, rebuild, and record the tested SHAs.
Do not assume passing tests of these snapshots validate untested future changes.

On Linux/macOS, export the same sources into `build/oracle/deps/` with `git show
COMMIT:FILE > build/oracle/deps/FILE`, then build with:

```sh
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic oracle/main.cpp -o build/oracle/oracle
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -I build/oracle/deps oracle/differential.cpp -o build/oracle/differential
```

## Files and responsibilities

| File | Purpose |
| --- | --- |
| `sorted_array.hpp` | Independent sorted vector implementing `leaderboard::Board` |
| `replay.hpp` | Strict command parsing and test-only board/player bookkeeping |
| `main.cpp` | Oracle-only replay executable; stdin or a trace filename |
| `legacy_adapters.hpp` | Bridge to standalone demos; full structural checkers |
| `differential.cpp` | Seeded generator, replay comparison, shrinker, self-tests and scale check |
| `tests/` | Fixed expected outputs, including errors and 64-bit boundaries |
| `build.ps1`, `run_tests.ps1` | Reproducible build and checks |

The oracle orders higher scores first and smaller IDs first within a tie.
Competition rank is `1 + count_strictly_higher(score)`; position is unique.
`select` is one-based and `range` is inclusive. Threshold equality enters the
high division. Split empties its source; merge empties its donor. The oracle
rejects duplicate IDs, missing exact keys, invalid positions/ranges, self-merge,
and cross-backend merge. Duplicate-ID merge checks happen before mutation.

The sorted vector uses O(n) space; insertion/deletion are O(n), rank/count is
O(log n), select O(1), and outputting q positions O(q). Split and merge take
O(n) and O(n+m) respectively, with expected linear duplicate-ID checking in
merge. The bulk constructor sorts once in O(n log n); this avoids quadratic
reference insertion when constructing the million-player scale test.

## Replay and failure reduction

The replay syntax matches `INTERFACE.md` on `madhav`: ADD, SET, REMOVE, RANK,
SELECT, RANGE, SIZE, SPLIT and MERGE. Board 0 starts empty. Each input line has
exactly one output, and malformed/blank lines produce `ERR BAD_COMMAND`.
Normal user errors produce `ERR code` without changing any board. The test
facade uses library maps; it is not the application's required handwritten hash.

```powershell
./build/oracle/oracle.exe oracle/tests/basic.trace
./build/oracle/differential.exe --seed 253 --ops 1000000 --max-live 128
./build/oracle/differential.exe --seed 42 --ops 10000 --dump build/oracle/seed42.trace
./build/oracle/differential.exe --trace build/oracle/seed42.trace
./build/oracle/differential.exe --scale 1000000
```

The generator uses `mt19937_64` and explicitly defined modulo choices, including
ties, score extremes, unchanged scores, removal/reinsertion, valid and invalid
queries, and threshold split/rejoin. It precollects all ADD/SET score coordinates
for Fenwick. A bounded live population allows full state and invariant audits
after **every command**; the default is 128 live players, not one million.
Separate full-scale testing uses a million players and audits at construction,
split and merge boundaries. These are correctness tests, not benchmark timings
or the application's million-revision performance workload.

On a mismatch or checker exception, exit status is 1; the failing prefix is
saved as `oracle-failure.trace`. Delta debugging removes chunks, then repeatedly
tries every single deletion while preserving the same backend/failure signature.
The result in `oracle-failure.min.trace` is **1-minimal**: deleting any one
remaining command no longer reproduces that signature. It is not guaranteed to
be the globally shortest sequence. Parameters are not numerically minimized.
Removed prerequisites become normal replay errors, so candidates stay replayable.
Unexpected process crashes/invalid pointers are not caught by the in-process
shrinker; use a debugger or sanitizer for those failures.

The shrinker's self-test injects an incorrect rank and confirms that the reduced
ADD/RANK trace still fails and every single deletion stops that failure. To
exercise file emission and a nonzero exit code manually:

```powershell
./build/oracle/differential.exe --trace oracle/tests/basic.trace --inject-fault
```

This command intentionally fails. Do not enable fault injection for normal runs.
CLI/setup errors return 2; successful checks return 0.

## What is checked

- AVL: global key bounds, unique IDs, cycle/shared-node detection, independently
  recomputed sizes/heights, and balance factors after every bounded replay command.
- Fenwick: descending unique coordinates, sorted unique bucket IDs, nonnegative
  tree counts, every block's expected sum, and every prefix sum. Complexity is
  O(n + U log U), including explicit prefix calls, with O(n+U) extra space.
- Application-facing state: board ownership, score lookup, sizes and exact full
  ordered contents agree with the independent sorted array. Query outputs agree.
- Fixed tests: all four rotation patterns, deletion, highly unequal-height joins,
  tie changes, overlapping league merges, empty divisions, error atomicity,
  boundary scores/IDs and deliberately corrupted metadata.

## Integration boundaries

The current teammate demos have `int` IDs/sizes and do not implement `Board`
factories. The test adapters reject IDs above INT_MAX explicitly; the standalone
oracle and `wide_ids` fixture support the architect's entire uint64 ID range.
The tests do not establish full-width ID support in the teammates' demos.

The adapters validate positional bounds and clear range output. Fenwick's count
adapter handles thresholds absent from the compressed domain using lower_bound;
the demo's direct `rank(score)` requires an existing coordinate. Exact player
preconditions are enforced by the shared test facade.

AVL ordered merges call the teammate's real `join`, including pivot removal.
For overlapping score/key ranges the adapter inserts the donor entries and
destroys the donor; this is O(m log(n+m)), not an O(log n) general merge claim.
The demo has score split but no `split_by_key` primitive; that missing production
API remains the structure owner's integration task. Fenwick uses its own split
and join/rebuild routines unchanged.

When production factories and the handwritten player hash are ready, run this
oracle against those adapters/application too. These tests validate the present
standalone structure code and test facade, not a production app that has not
been implemented on the inspected branches. The team still needs its reviewed
PRs and final integration/tag; this branch does not manufacture those records.

## Viva starting points

Read `sorted_array.hpp` first, then the three cases of select in D4, then
`audit_node`, `audit_fenwick`, `replay` and `shrink`. Be ready to explain why a
sorted vector is deliberately slower but simpler, why tied ranks differ from
positions, why parent size must be recomputed after child size, and why checking
only query results can miss an internal invariant violation.
