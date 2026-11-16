# Oracle validation record

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
