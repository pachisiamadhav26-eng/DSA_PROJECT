# Rank queries on a live leaderboard

CS 253 / MA 253 data structures project

## Design

The application replays commands against either an AVL tree or a Fenwick tree.
A handwritten open-addressing hash table tracks each player's current score.
Players appear in descending score order, with ascending IDs breaking ties.
Competition rank is one plus the number of players with strictly higher scores.
Tied players share a rank and occupy distinct positions.

The AVL stores subtree sizes for rank and select. A threshold split moves
existing nodes into two trees and assigns scores at the threshold to the high
division. Ordered merges reuse the AVL join. Overlapping merges merge sorted
node lists and rebuild a balanced tree.

The Fenwick tree stores counts over scores collected from the trace before
replay. Sorted ID vectors handle ties. Binary lifting locates the score bucket
for a given position. The player is then selected by their offset in that bucket.

## Complexity

Let n be the number of players on a board, U the number of score coordinates
in the replay, t the size of an affected tie bucket, and k the number of players
returned by a range query.

| Operation | AVL | Fenwick |
| --- | --- | --- |
| Insert or remove | O(log n) | O(log U + t) |
| Rank or select | O(log n) | O(log U) |
| Range query | O(log n + k) | O(k log U) |
| Threshold split | O(log n) | O(U) |
| Storage per populated board | O(n) | O(U + n) |

A supplied-pivot AVL join takes O(1 + |h₁ − h₂|). Pivot extraction adds search
work for a merge. An overlapping AVL merge takes O(N log N) in the worst case
for N total players because it sorts IDs to detect conflicts. A same-domain
Fenwick merge checks IDs, merges overlapping buckets and rebuilds counts in
O(U + N) expected time. Multiple populated Fenwick boards share the coordinate
values, with each board allocating its own buckets and counts.

## Correctness and scale

The independent sorted-array oracle checks both backends and their replay
outputs. Fixtures cover ties, invalid commands, season changes, and full-width
IDs and scores. Invariant tests check tree balance and subtree sizes as well
as Fenwick blocks and bucket ordering.

On macOS arm64 with Apple Clang 21, all six CTest tests passed in Release and
under address and undefined-behavior sanitizers. The long suite also passed
three million-command mixed traces with at most 64 player IDs per trace.
A separate test checked one million simultaneous players, one million rank
and select queries, and split and rejoin.

Each backend also replayed one million player additions and one million score
revisions. All 2,002,018 output lines per backend matched independent expected
values, including rank and select during updates and checks before and after
the split and merge. See [validation results](results/validation-long.txt).

## Benchmark

The fixed-board benchmark checks five rank queries on 100 players with ten
distinct scores. On the test machine, AVL averaged 3.90 ns per rank call and
Fenwick averaged 2.55 ns. Each reported mean uses five batches, and the two
checksums agree. See [benchmark output](results/rank-100.txt) and the
[build environment](results/environment.txt).

The measured scope is rank lookup on this fixed input. Rank and select scaling,
revision costs, and split and join timing require separate measurements.
