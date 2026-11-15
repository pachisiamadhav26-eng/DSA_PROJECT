# Live leaderboard design and results

The application replays the same commands with an augmented AVL tree or a
Fenwick tree and a handwritten, open-addressed player table. An independent
sorted vector checks results and state. Ties use decreasing score then
increasing player ID. Competition rank counts strictly higher scores, while
select looks up an exact one-based position.

## Design and predictions

| Operation | AVL | Fenwick |
| --- | --- | --- |
| Rank, select | O(log n) using subtree sizes | O(log U) using compressed score counts, with an ID offset for selection |
| Return q positions | O(log n + q) | O(q log U) |
| Score update | O(log n) tree work | O(log U + t_old + t_new) in the worst case with sorted bucket vectors |
| Split by score | O(log n) with ordered joins and telescoping heights | O(U) to move buckets and rebuild counts for both divisions |
| Rejoin separated ranges | O(log n) including pivot search/removal | O(U) with shared coordinates and disjoint occupied buckets |
| Overlapping leagues | O(nL + nR) traversal, sorted merge and balanced rebuild reusing AVL nodes | O(U + M) for common coordinates, with M the entries in buckets occupied on both sides |
| Ordered join with supplied pivot | O(1 + absolute input height difference) | Not applicable |

For different Fenwick coordinate sets, take an ordered union and rebuild in
O(UL + UR + nL + nR). AVL keeps one node and two links per player. Fenwick
uses O(U + n) space for score coordinates, counts and the ordered IDs. The
handwritten player table enforces globally unique active IDs with linear
probing, tombstones and rehashing. A score lookup plus backend exact-key
membership check identifies a player on the requested board. This avoids
changing the table's records during season operations.

## Correctness and adversarial evidence

- Fixed app fixtures cover tie ranks, inclusive range, wide IDs, extreme
  scores, retired boards, overlapping merges and command error precedence.
- The oracle ran seed 253 for one million commands with up to 48 live
  players, comparing all outputs and auditing both structures after every
  command. A separate million-player rank/select check uses phase audits.
- Production tests exercised rotations, churn, unequal-height joins, overlapping
  seasons and malformed commands. Targeted traces were generated twice and
  compared for reproducibility.
- Both production backends matched all 2,000,106 outputs from a replay with
  one million players, one million revisions, interleaved queries and split/rejoin.
  A closed-form reference checked every output, and structural and player-table
  audits ran at season boundaries and the end. See
  [scale_summary.json](results/scale_summary.json).
- In the million-player unique-score monotone build, AVL recorded 999,980
  rotations. With 62,500 buckets of 16 players added in descending ID order,
  Fenwick recorded 7,500,000 existing-entry shifts, exactly 62,500 × 16 × 15 / 2.

## Timing method

Windows 11 build 26200, Intel Core i5-13420H (8 cores, 12 logical processors).
The OS reports L1 data caches of 4 × 32 KiB and 4 × 48 KiB, L1 instruction caches
of 4 × 32 KiB and 4 × 64 KiB, L2 of 4 × 1,280 KiB and 1 × 2,048 KiB, and
shared L3 of 12,288 KiB. MinGW.org GCC 6.3.0 (`mingw32` target), C++17, `-O2`.
Windows timings use QueryPerformanceCounter. Each configuration has five
trials. The full runs use 50,000 rank and 50,000 select calls per trial with
seed 253. Checksums, setup, auditing and I/O are outside rank/select timers.
Fenwick coordinate compression is outside insertion and query timings.
The app's scale replay duration includes parsing, I/O and audits and is logged
separately from backend timings.

## Measured comparisons

The table shows first-order comparisons at one million players. The raw trials
and summary include min, max and standard deviation for all configurations.

| Workload or measure | AVL mean | Fenwick mean |
| --- | ---: | ---: |
| Rank, 1,000,000 distinct scores | 2,317 ns | 1,142 ns |
| Select, 1,000,000 distinct scores | 1,897 ns | 2,324 ns |
| Rank, 62,500 scores with 16 players each | 2,666 ns | 308 ns |
| Select, 62,500 scores with 16 players each | 2,513 ns | 438 ns |
| Score split, unique scores | 4.88 µs | 27.97 ms |
| Separated rejoin, unique scores | 1.22 µs | 21.77 ms |
| Overlapping merge, unique scores | 136.28 ms | 30.10 ms |

The smaller Fenwick domain cut query times in this workload. With one million
distinct scores, Fenwick select trials ranged from 868 to 7,244 ns per call,
and the five-trial mean exceeded the AVL mean. The split and
rejoin trends are consistent with touching only part of the AVL tree and
rebuilding Fenwick count arrays over U coordinates. Overlap changes the AVL
algorithm to traversal and rebuild, so its cost rises substantially. With
unique Fenwick scores the two interleaved leagues occupy different buckets,
avoiding tied-ID merges even though their score ranges overlap.

The supplied-pivot AVL join visited 1, 4, 8 and 13 nodes at corresponding
height gaps 0, 4, 8 and 13. These counters align with the height-gap model.
Single-operation timings have considerable spread and can overlap across height
gaps. The join figure and min/max accompany the means. Wall-clock slopes and
relative constants are specific to this machine and workload.

Hardware cache counters were listed by Windows Performance Recorder. This
session had no administrator token for capture, and Linux `perf` was
unavailable on the measured host. The report makes no attribution to cache-hit
or miss behavior. The runner includes an optional process-level capture for a
Linux host with `perf` access. Details are in [bench/README.md](bench/README.md).

## Results

- [Trials CSV](results/benchmark_trials.csv) and [summary CSV](results/benchmark_summary.csv)
- [Clean-clone validation](results/validation.txt)
- [Rank](results/rank.svg) and [selection](results/select.svg)
- [Score split](results/split.svg), [separated rejoin](results/separated_rejoin.svg) and [overlap merge](results/overlap_merge.svg)
- [Supplied pivot join](results/supplied_pivot_join.svg) and [insertion](results/descending_build.svg)
