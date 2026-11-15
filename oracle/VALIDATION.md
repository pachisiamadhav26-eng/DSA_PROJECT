# Oracle validation record

This file records the initial standalone snapshots. Current production evidence
and the commands to reproduce it are in [REPORT.md](../REPORT.md) and
[README.md](README.md).

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

These are correctness checks, not measurements of production latency. The
test-only facade and legacy adapter limitations are described in `README.md`.
No review, final integration, or student-authored modifications are claimed.
