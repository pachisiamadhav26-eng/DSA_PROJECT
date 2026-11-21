# Benchmark results

**A = AVL tree. B = Fenwick tree. Lower time means faster.**

Each number below is the average of **five runs** on the same machine. These tables use one available score value per player (`U = n`). Rank/select experiments mix one million actual score changes with one million queries of each kind; only the queries are timed here.

## 1. Rank latency

Question: “What rank does this player's score have?”

| Players | A: AVL (nanoseconds/query) | B: Fenwick (nanoseconds/query) |
| ---: | ---: | ---: |
| 1,000 | 77.19 | 95.95 |
| 10,000 | 131.74 | 132.98 |
| 100,000 | 323.71 | 189.14 |
| 1,000,000 | 939.02 | 342.74 |

At one million players, **Fenwick rank was about 2.7 times faster**.

## 2. Select latency

Question: “Which player is at position k?”

| Players | A: AVL (nanoseconds/query) | B: Fenwick (nanoseconds/query) |
| ---: | ---: | ---: |
| 1,000 | 73.93 | 98.56 |
| 10,000 | 131.80 | 147.74 |
| 100,000 | 339.47 | 285.75 |
| 1,000,000 | 1,046.16 | 625.85 |

At one million players, **Fenwick select was about 1.7 times faster**.

## 3. AVL split versus Fenwick rebuilding

Divide the leaderboard into two using a score cutoff targeting approximately half the players in each division.

| Players | A: AVL split (milliseconds) | B: Fenwick split/rebuild (milliseconds) |
| ---: | ---: | ---: |
| 1,000 | 0.00223 | 0.05065 |
| 10,000 | 0.00263 | 0.40891 |
| 100,000 | 0.00430 | 3.97262 |
| 1,000,000 | 0.00576 | 37.21076 |

**AVL split was faster at every tested size.** Fenwick moves whole player buckets and rebuilds its score-count arrays.

## 4. AVL join versus Fenwick rebuilding

Combine the two separated divisions back into one leaderboard.

| Players in total | A: AVL public rejoin (milliseconds) | B: Fenwick merge/rebuild (milliseconds) |
| ---: | ---: | ---: |
| 1,000 | 0.00106 | 0.11121 |
| 10,000 | 0.00165 | 1.55855 |
| 100,000 | 0.00412 | 21.23800 |
| 1,000,000 | 0.00820 | 286.05312 |

**AVL rejoin was faster at every tested size.** Both columns time the complete public operation. AVL includes finding/extracting its pivot. Fenwick includes sorting IDs to check conflicts and rebuilding its counts. These are separated divisions; overlapping regional leagues can follow a different AVL merge path.

## 5. Did the results match our D6 predictions?

| Prediction | What we measured | What we can say |
| --- | --- | --- |
| AVL follows short tree paths rather than checking all players. | With 1,000 times more players, rank/select took about 12–14 times longer. | Growth was much smaller than player growth, but our simple model does not explain the exact timing increase. |
| Fenwick queries depend on score-coordinate count U. | Keeping U=1,000, rank changed from 92.24 to 99.74 ns; select from 94.73 to 126.76 ns as players increased 1,000-fold. | Query time changed modestly when U stayed fixed. |
| AVL split grows slowly; Fenwick split processes U coordinates. | For U=n, split went from 0.00223 to 0.00576 ms for AVL and 0.05065 to 37.21076 ms for Fenwick. | The growth patterns support the comparison. |
| Fenwick split can stay cheap when U stays fixed. | With U=1,000, split went from 0.04624 to 0.04174 ms despite 1,000 times more players. | Moving whole buckets avoids processing each player individually. |
| AVL join work depends on the height gap. | Height gaps 0, 6 and 20 gave 1, 6 and 20 join visits. | Internal work follows the predicted height-gap pattern; timings also vary. |
| Fenwick public merge still processes/sorts players even at fixed U. | At fixed U=1,000, merge increased from 0.10908 to 85.19272 ms. | Keeping the coordinate count fixed does not remove its player-related work. |

These are predictions about growth, not exact predicted nanoseconds. The [full comparison](../../REPORT.md#prediction-versus-measured-result) explains the evidence and gaps, including public AVL rejoin. All endpoint comparisons use 1,000 versus 1,000,000 players.

## What to say about these results

“At one million players, Fenwick answered rank and select queries faster. AVL split and rejoined the leaderboard much faster than Fenwick's rebuilding methods. All timings were repeated five times, and the checks run by our pipeline passed.”

One nanosecond is one billionth of a second. One millisecond is one thousandth of a second. Rank/select values are average time per query in a batch, not complete application response times.

## Supporting results

The [full report](../../REPORT.md) includes the spread across runs, the tie-heavy workload with only 1,000 score values, the separate supplied-pivot AVL join experiment, operation counters, adversarial checks and cache profiling. Hardware recordings are diagnostic samples, not exact cache miss rates.

| Open this | To see |
| --- | --- |
| [Full report](../../REPORT.md) | Methods, detailed comparisons and limitations |
| [Graphs](figures/) | Rank, select, split and join curves |
| [Summary CSV](summary.csv) | Exact averages, standard deviations, minimums and maximums |
| [Raw CSVs](raw/) | Individual measurements behind the averages |
| [Cache evidence](profiles/analysis.json) | Filtered hardware sample summaries |
| [Run settings](metadata.json) | Machine, compiler, seeds and code versions |

