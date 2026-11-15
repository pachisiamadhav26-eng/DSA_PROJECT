# Validation performed 15 November 2026

Environment: Windows, Python 3.12, MSYS2 UCRT64 GCC 14.2.0.
Flags: `-std=c++17 -O2 -Wall -Wextra -Wpedantic`.
The generator also compiled with MinGW GCC 6.3.0 and produced exactly the same
10,000-command seed-253 trace as GCC 14.2.0.

Completed against pinned source versions in the JSON records:

- Hand-calculated 14-command ties/update/split/merge fixture: oracle output matched
  expected output; actual AVL/Fenwick differential checks passed.
- Nine generated targeted families (60, 87, 87, 87, 449, 22, 99, 97 and 31 commands)
  passed with complete state and invariant checks after every command.
- Mixed seeds 253, 42 and 20261115: each passed at 10,000 commands and then
  1,000,000 commands, with at most 64 active players. Long-run replay/audit times
  printed by the existing runner were about 73.0, 59.9 and 104.9 seconds.
  These are correctness-run durations including auditing, not latency benchmarks.
- Each generated output was repeated and compared for exact reproducibility.
- Full uint64-ID/int64-score oracle fixture matched its known answers.
- Invalid generator arguments (zero players, unknown case, negative seed) returned 2.
- Injected rank fault returned 1 and emitted a reduced reproducer.
- Existing one-million-player test passed one million alternating rank/select
  queries plus split/rejoin, auditing complete structures at phase boundaries.
- Streaming million-scale workload: exactly 1,000,000 ADD commands, 1,000,000
  SET revisions and four observations, 2,000,004 lines, 42,666,740 bytes. The
  file's SHA-256 is in additional_checks.json. This checks generation only.
- Smaller version of that workload, 2,000 players and 2,000 revisions, passed
  differential replay and full audits (4,004 commands).
- Existing app player-table and replay tests passed unchanged, with assertions
  enabled. Sumit's fixed benchmark returned all five expected ranks in both trees.

The raw dependency hashes and executed command results are recorded in
million_command_runs.json. Generator stdout is represented by its normalized
text SHA-256 and line count to avoid embedding multi-megabyte traces in JSON.
The million-scale file hash instead covers the exact Windows output bytes (CRLF).
additional_checks.json records portability, workload counts and smaller replay.
quick_final_run.json records a final rerun of the runner after evidence-output
compaction; it uses 1,000 mixed commands per seed and a 10,000-player scale check.

Known limitations: legacy int IDs, adapters rather than production factories,
bounded population in long differential runs, no execution claim for the
million-player/million-SET file, no fresh performance study, and unresolved
application/oracle semantics described in the project review.
