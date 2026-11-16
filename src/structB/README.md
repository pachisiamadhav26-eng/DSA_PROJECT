# Structure B — Fenwick leaderboard

`fenwick.cpp` implements `leaderboard::Board`. The replay calls `make_fenwick` with all scores used by `ADD` and `SET`. The board sorts and deduplicates those scores in descending order. Each score has a vector of player IDs in ascending order, and the Fenwick tree stores the size of each bucket.

- `count_strictly_higher` sums the buckets ahead of the queried score. The replay adds one to get competition rank.
- `select` uses binary lifting to find the bucket for a one-based position, then indexes the ID within that bucket. `range` replaces the output with the inclusive page in the same order.
- `split_by_score` puts ties at the threshold in the high board. It moves the buckets and empties the source. Both results retain the full prepared score list so a later score revision can cross the threshold.
- `merge_from` sorts a temporary vector of both boards’ IDs to catch duplicates, including the same ID at different scores. It checks for conflicts before changing either board, combines the coordinates and sorted buckets, rebuilds the counts, and empties the donor on success.

## Costs

Let `M` be the number of supplied scores, `U` the distinct prepared scores, `t` the size of the affected tie bucket and `r` the number of players returned. The bounds below use `log(U + 1)` so they also cover a single coordinate.

| Operation | Cost |
| --- | --- |
| Prepare scores and allocate storage | `O(M log(M + 1) + U)` |
| Rank count and select | `O(log(U + 1))` |
| Contains | `O(log(U + 1) + log(t + 1))` |
| Insert or erase | `O(log(U + 1) + t)` including ID shifts |
| Range | `O(r log(U + 1))` |
| Split | `O(U)` using bucket moves and linear rebuilds |
| General merge | `O(U_left + U_right + N log(N + 1))` for `N` total players, including duplicate validation. Validation uses `O(N)` temporary ID storage. |

Each board uses `O(U + n)` storage for `n` live players, with bucket capacity depending on past updates. `bucket_shifts` counts elements shifted by ordered inserts and erases. Keep the structural audit outside timed runs.

## Focused test

From the repository root, compile the test against the same source the replay uses.

```sh
g++ -std=c++17 -Iinclude src/structB/fenwick.cpp src/structB/tests/fenwick_test.cpp -o fenwick-test
./fenwick-test
```
