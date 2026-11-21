# Oracle validation record

## PR 6 submission corrections, 21 November 2026

Integrated `origin/main` at `7b161a09144185ee772ae3a566a6d44b109912ed`
into `om-oracle`. Resolved the add/add conflicts in `.gitattributes` and
`TOOLS.md`, retaining the team's and Oracle's tool records. Production files
under `src/`, `app/` and `include/` are unchanged from that main revision.
The default build now compiles those checkout files directly, with C++17,
`-O2 -Wall -Wextra -Wpedantic`. Historical pinned runs below are preserved.

Executed `./oracle/run_tests.ps1 -Quick` with a fresh build, then
`./oracle/run_tests.ps1 -SkipBuild`. Both returned exit code 0.

- All four fixtures (`basic`, `errors`, `seasons`, `wide_ids`) matched the
  independent oracle and both real application modes.
- Quick seeds 253, 20261112 and 42 passed 10,000 commands each. The
  10,000-player/10,000-query scale and split/merge test passed.
- Full seed 253 passed 1,000,000 commands, peak live population 48,
  with complete invariant/state audits after every command (47.9827 seconds).
- Full seeds 20261112 and 42 each passed 100,000 commands, peak 48
  (4.83611 and 4.71249 seconds respectively).
- The separate 1,000,000-player/1,000,000-query scale test passed, including
  split/merge and full structural audits at phase boundaries.
- Oracle boundary, membership, rotation, key-split, join and shrinker checks passed.
- `build/oracle/sources.txt` lists current-checkout input paths and SHA256
  hashes, including AVL `CEB536B73AC3A4855659134D7957A215296F3186DFC24E6E178B5BE828062EBD`
  and Fenwick `7413F4306D42C0D1E8A4A175340D871166264F67BFF98344FCAE0BBDCBE45738`.

D4 was renamed to `docs/derivations/D4_250041029.pdf`. It names Om Dehariya,
roll number 250041029, Maths and Computing; quotes the assigned statement;
and identifies consulted sources and implementation files/functions.
pypdf confirmed two pages and both rendered pages were visually inspected.
Dated representative AI prompts were added to TOOLS.md. A separate clean clone of merge commit `ad37a06` was created with
`git clone --no-local --single-branch --branch om-oracle`.
`./oracle/run_tests.ps1 -Quick` passed from that clone, and both documented
`leaderboard.exe avl` and `leaderboard.exe fenwick` sample commands exactly
matched `app/examples/basic.expected`. No historical dependency export was used.
No teammate review or approval is claimed by these automated checks.

## D4 and branch review, 17 November 2026

Both suites passed with Architect `a4dc438`, AVL `03f3f57`, and Fenwick
`0d5ad68`. The golden fixtures matched the independent oracle and both real
app modes. The full suite passed one million seeded commands, two 100,000-
command seeds, and the separate one-million-player rank/select and split/rejoin
test.

Verified that the runner rejects `--scale 0` and values above the supported
range before narrowing them. In particular, `--max-live 4294967297` and
`--ops 4294967297` each returned exit code 2 on the 32-bit compiler instead
of wrapping around.

The English D4 proof was regenerated from `docs/derivations/D4.md`.
Both PDF pages were rendered and visually inspected. The PDF names the
author and contains the single-rotation, double-rotation, and selection proofs.

## Readability refactor, 17 November 2026

The shared Board header stayed byte-identical. The test executables used
Architect `a4dc4382b11d60ed85697fb9560574d24a8d2366`, AVL
`03f3f57186f96bac6b2ad8f22acbedd7da2de153`, and Fenwick
`0d5ad686ea2349147a4b6eb1ca2b64eb52feae38`. Saksham's newer tip
`2d758efdc7bdd69587981c0c3f44bf26984a7801` has identical Fenwick
source and shared headers.

- `./oracle/run_tests.ps1 -Quick` passed. The four golden fixtures matched
  the oracle and both production app modes. Each of the three seeds passed
  10,000 commands, and the 10,000-player scale test passed.
- `./oracle/run_tests.ps1 -SkipBuild` passed using the freshly built binaries.
  Seed 253 passed 1,000,000 differential commands. Seeds 20261112 and 42
  each passed 100,000, with peak live population 48 in all three runs.
  The separate 1,000,000-player test passed 1,000,000 rank/select queries
  and verified both split partitions and the rejoined board.
- For seed 253, the 10,000-command trace and every oracle output line were
  identical before and after the refactor. An injected rank fault still
  exited with code 1 and saved a three-command 1-minimal trace.
- Before-and-after runs of the PDF helper rendered identical images for
  both D4 pages.

## Fenwick default pin update, 17 November 2026

Both commands built with the default dependency refs. The exact commits were
confirmed in `build/oracle/sources.txt` after each run.

| Dependency | Commit |
| --- | --- |
| Architect | `a4dc4382b11d60ed85697fb9560574d24a8d2366` |
| AVL | `03f3f57186f96bac6b2ad8f22acbedd7da2de153` |
| Fenwick | `0d5ad686ea2349147a4b6eb1ca2b64eb52feae38` |

- `./oracle/run_tests.ps1 -Quick` passed. All four golden fixtures matched
  the oracle and both production app modes. Seeds 253, 20261112, and 42
  each passed 10,000 differential commands, with peak live populations
  of 46, 46, and 48. The separate 10,000-player and 10,000-query scale
  test passed, including split and rejoin.
- `./oracle/run_tests.ps1` passed. The same fixtures passed on both production
  app modes and the oracle. Seed 253 passed 1,000,000 differential commands,
  and seeds 20261112 and 42 each passed 100,000. All three runs peaked at
  48 live players, with invariants and full state checked after every command.
  The separate 1,000,000-player and 1,000,000-query scale test passed,
  including split and rejoin audits at phase boundaries.

## Current shared interface, 17 November 2026

Built with MinGW GCC 6.3.0 and `-std=c++17 -O2 -Wall -Wextra -Wpedantic`.
The Oracle header matches the Architect header. The build linked these sources:

| Dependency | Commit |
| --- | --- |
| Architect | `a4dc4382b11d60ed85697fb9560574d24a8d2366` |
| AVL | `03f3f57186f96bac6b2ad8f22acbedd7da2de153` |
| Fenwick | `c921385a635deb2bd6ff8ca70ec67cf554752fed` |

`./oracle/run_tests.ps1` completed successfully:

- All four hand-calculated fixtures matched Oracle and the production replay
  app in both modes. The production app uses its handwritten player table.
- Both backends passed full-state differential checks for seed 253
  (1,000,000 commands), seed 20261112 (100,000), and seed 42 (100,000).
  Peak population was 48 for each seed.
- Both backends passed the separate 1,000,000-player test with 1,000,000
  rank/select queries, exact partition comparison and rejoin at phase
  boundaries.
- Self-tests passed for exact membership, rotations, full-key split,
  supplied-pivot join, unequal joins and fault reduction.

Also verified that 10,000 generated commands (seed 1711, peak 44) had identical
output in both production app modes and the independent Oracle. Deliberate
fault injection on `basic.trace` returned exit code 1, printed the command
and expected/actual rank, and saved a three-command 1-minimal reproducer.

The bounded runs audit state at every command. The million-player run uses
phase-boundary audits and rank/select queries. Neither measures performance
or establishes correctness for a separate million-revision app trace.

## Historical standalone run

Date: 12 November 2026. Compiler: MinGW.org GCC 6.3.0, C++17, Windows.
Build flags: `-std=c++17 -O2 -Wall -Wextra -Wpedantic`.
Sources tested are the immutable commits listed in `README.md` in this directory.

Completed checks:

- Both C++ executables compiled without warnings.
- `oracle/build.ps1` successfully exported pinned dependencies and rebuilt both
  executables. `oracle/run_tests.ps1 -Quick -SkipBuild` then passed end to end,
  including all three 10,000-command seeds and the 10,000-player scale check.
- Golden output comparison: `basic`, `errors`, `seasons`, `wide_ids` all passed.
- Differential replay: `basic` (12 commands), `errors` (28), `seasons` (26)
  all passed with full state and invariant audits after every command.
- Seed 253, 10,000 commands, maximum 128 live players: passed.
- Seed 253, 1,000,000 commands, maximum 128 live players: passed, full state
  and invariant audits after every command (729.065 seconds including generation).
- Seed 42, 100,000 commands, maximum 128 live players: passed (62.6 seconds).
- Seed 20261112, 100,000 commands, maximum 128 live players: passed (53.2801 seconds).
- 1,000,000 live players, 1,000,000 rank/select queries, split/rejoin and full
  structural audits at phase boundaries: passed.
- Self-tests passed: oracle boundary/error cases, all four AVL rotation patterns,
  deletion, joins with highly unequal heights in both key-order directions,
  corrupt metadata detection, and a 1-minimal shrinker result.
- End-to-end deliberate fault injection returned exit code 1 and wrote a
  two-command minimal reproducer: `ADD 0 8 90`, then `RANK 0 8`.
- The shared header and basic trace match the architect's pinned revision.
- The two-page D4 PDF was rendered and visually checked. PDF files are marked
  binary in `.gitattributes` to preserve their offsets on Windows checkout.

These runs used the earlier test facade and legacy standalone adapters from
commit `fc2a10c`.
No review, final integration, or student-authored modifications are claimed.
