# Harness validation record

Historical checks completed on 17 November 2026 are recorded below. The latest simplification is compile/syntax checked only; the pipeline will be rerun later as requested. These records do not claim completion of the final experiments.

| Check | Result |
| --- | --- |
| Compile runner against Architect `293e378` and actual Fenwick `82fd786` | Passed with MinGW C++17 flags and warnings enabled. External oracle disabled for this development build because its latest inspected source lacks `contains`. |
| Runner plumbing tests | 22 scenarios passed with an explicitly labelled test-only sorted-vector backend and test oracle fixture. Includes known replay output, deliberate wrong rank, split extremes, ordered-join/helpers, counters, output preservation and profile handshake. |
| Result processing | Synthetic five-run means, sample standard deviation, min/max, PNG/SVG output and mismatched-backend detection passed. |
| Real Fenwick quick pipeline | Passed 18 experiment configurations with five repetitions each, including queries, split/rejoin and counters. Saved development metadata/raw CSVs/summary/16 SVG+PNG charts in ignored `build/bench/final-fenwick-validation/`. |
| Large-population code path | Real Fenwick with 10,000 players, 10,000 coordinates and 2,000 revisions passed final snapshot and sampled-query checks. One development repetition; not a formal timing claim. |
| Real Adversary trace | Bianca's `adversary/tests/ties.trace` ran through Architect replay and actual Fenwick for five repetitions. The complete output digest matched her supplied `ties.expected`. |
| Profile configuration | Windows WPR accepted the XML and lists its `Leaderboard` profile. The ready/go/done/release process handshake passed. Actual hardware-event collection and WPA analysis remain unverified. |
| D6 | Authored Markdown rendered to a two-page PDF. Both page images were inspected for clipping, overlap and legibility. Human proof review is pending. |

The earlier test-only backend and oracle fixture were never linked by `build.py`. They were removed during simplification after real components became compatible. Current `tests/check_runner.py` uses the actual backends and Om's oracle. The historical checks above describe the earlier implementation; later updates follow below.

## Latest component integration — 17 November 2026

- Architect `a4dc438`, AVL `03f3f57`, Fenwick `0d5ad68`, and Oracle `6a25781` compile and link together in the Analyst harness with both helper sets and the real oracle enabled.
- The integrated quick suite passed. Development outputs are in ignored `build/bench/om-integrated-check/`.
- Om's own build and quick suite passed in an isolated export, explicitly overriding all three dependencies to the current branches. This covers four golden fixtures in both production app modes, three 10,000-command differential seeds, structural checks, shrinker checks and the 10,000-player scale check.
- Om's full suite also passed against those exact current commits: seed 253 with 1,000,000 commands and peak 48 players, two further 100,000-command seeds, and the separate 1,000,000-player/1,000,000-query split/merge scale check. The bounded runs audit after every command; the large-population test audits at phase boundaries. These are correctness checks rather than the Analyst's million-revision performance workload.
- The earlier interface blockers are resolved. The formal million-revision timing workload and hardware cache analysis are separate pending experiments.

To repeat code checks after a successful dependency export:

```powershell
# Rebuild test-only fixtures and exercise the runner's behavior.
python bench/tests/check_runner.py

# Check summary statistics, plots and mismatch detection with synthetic data.
python bench/tests/check_results.py
```
