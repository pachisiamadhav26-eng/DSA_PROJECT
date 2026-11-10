# Rank Queries on a Live Leaderboard

CS 253 / MA 253 Data Structures and Algorithms Laboratory project.

The project compares an order-statistic AVL tree and a Fenwick tree for a leaderboard with score revisions, rank and position queries, and season split/merge operations.

Both C++17 implementations maintain the same leaderboard. A player has a signed 64-bit ID and score. Higher scores appear first. Players with equal scores share their competition rank and appear in ascending ID order. For example, scores `100, 90, 90, 70` produce ranks `1, 2, 2, 4`. The third position contains the second player scoring 90.

## Repository layout

The [Saksham branch](https://github.com/pachisiamadhav26-eng/DSA_PROJECT/tree/Saksham) contains Structure B in `fenwick/` and its player lookup table in `common.hpp`. This README on `main` documents both structures and their baseline comparison. The AVL build instructions apply to the separate Structure A source package.

## Build and run

For Structure B, check out `Saksham` and run `cd fenwick` from the repository root. For the Structure A package, run `cd avl`. The following commands work from either source directory with a C++17 compiler and its standard library. Each source program builds independently.

**Windows PowerShell with MinGW GCC**

```powershell
g++ -std=c++17 -O2 -Wall -Wextra -pedantic main.cpp -o demo.exe
g++ -std=c++17 -O2 -Wall -Wextra -pedantic check.cpp -o check.exe
g++ -std=c++17 -O2 -Wall -Wextra -pedantic compare.cpp -o compare.exe
.\demo.exe
.\check.exe
.\compare.exe 10000 10000 1024
```

**Linux or macOS with GCC or Clang**

```sh
c++ -std=c++17 -O2 -Wall -Wextra -pedantic main.cpp -o demo
c++ -std=c++17 -O2 -Wall -Wextra -pedantic check.cpp -o check
c++ -std=c++17 -O2 -Wall -Wextra -pedantic compare.cpp -o compare
./demo
./check
./compare 10000 10000 1024
```

The demo gives IDs 2 and 3 rank 2. After ID 4 moves to score 100 and ID 5 leaves, positions 2 through 4 contain IDs `4, 2, 3`. Splitting at 100 produces two leagues of size two.

## Interface

From the repository root, include `avl/avl_leaderboard.hpp` on Structure 1 or `fenwick/fenwick_leaderboard.hpp` on Structure 2. Both classes are in the `leaderboard` namespace.

```cpp
// Structure A package
leaderboard::AvlLeaderboard a;
```

```cpp
// Saksham branch
leaderboard::FenwickLeaderboard b({40, 70, 90, 100});
```

| Call on either board | Behavior |
|---|---|
| `insert(id, score)` | Add a unique player |
| `erase(id)` | Remove an existing player |
| `update(id, score)` | Revise an existing player's score. An identical score is a no-op |
| `size()` | Return the active player count |
| `rank(id)` | Return one plus the number of players with a strictly higher score |
| `select(k)` | Return the `Player{id, score}` at one-based position k |
| `range(first, last)` | Return players at every position in the inclusive range. Use `range(40, 50)` for the project example |
| `auto high = board.split_off(T)` | Move scores ≥ T to the returned league. The original league keeps scores < T |
| `board.merge_from(other)` | Move all players into the receiving league. The donor becomes empty and can be used again |
| `audit()` | Check internal invariants and throw on an inconsistency |

Duplicate insertions, self-merges, and merges containing a shared ID throw `std::invalid_argument`. Missing player IDs and invalid positions throw `std::out_of_range`. A page requires `1 <= first <= last <= size()`. A failed duplicate-ID merge preserves both leagues.

## Structure A · augmented AVL tree

`avl_leaderboard.hpp` stores one node per player, ordered by descending score and then ascending ID. Every node stores its height and subtree size. Rotations refresh the former root before refreshing the new root. A score change removes the old key and inserts the new key.

Rank counts strictly higher scores by skipping whole subtrees. Select compares the requested position with the size of the left subtree. Range traversal skips branches outside the requested page.

The internal `join(left, pivot, right)` requires every left key to precede the pivot and every right key to follow it. It walks the taller tree's spine, attaches the pivot, and rebalances on the way back. Threshold split follows one search path and reconstructs its two outputs with joins.

For leagues with separated key ranges, merge detaches a boundary player to use as the pivot. For interleaved leagues, it inserts the donor's nodes into the receiving AVL. Each league also owns its ID table, so the public split and merge operations include membership updates.

## Structure B · Fenwick tree over score counts

`fenwick_leaderboard.hpp` sorts and deduplicates the possible scores. For a recorded replay, supply all initial scores and all scores that later updates can introduce. Within a league, an insert or update to a score outside this universe throws `std::invalid_argument`. Merging two leagues takes the sorted union of their universes.

Compressed coordinates run from 1 to U in ascending score order. If `i` is the coordinate of a player's score and `n` is the player count, `prefix(i)` counts all players at or below that score. Therefore `rank = 1 + n - prefix(i)`.

For position `k` from the top, binary lifting finds the smallest coordinate whose prefix count reaches `n - k + 1`. The descent tries one power of two at a time and skips a block only when its cumulative count is strictly below the target. This handles duplicate scores and coordinates whose current count is zero.

Each coordinate also has a sorted vector of player IDs. After selecting a coordinate, the zero-based ID offset is `k - (n - prefix(i)) - 1`. Reading that ID costs constant time. Insertion and deletion can shift the remainder of the vector. This baseline deliberately keeps the bucket code small.

A split transfers complete score buckets and rebuilds both Fenwick arrays. A merge combines sorted score universes, merges the sorted ID vectors at equal scores, and builds a new count array. The linear Fenwick build initializes each block with its bucket size and adds it once to its parent.

## Shared player lookup

`common.hpp` implements the ID-to-score hash table from scratch with separate chaining, a 64-bit hash mixer, and capacity doubling. Both structures use it to locate a player's old score. The verification program uses a separate set of oracle records and sorts the active players independently.

## Comparisons and predicted costs

Let **n** be the active players, **U** the number of compressed coordinates, **t** the size of an affected tie group, **q** the number of returned players, and **m** the number of players moved from one league to another. Hash-table bounds assume well-distributed IDs and amortized resizing. Logs in the table describe growth for nonempty inputs.

| Operation | A · AVL | B · Fenwick with sorted ID buckets |
|---|---|---|
| Insert or erase | O(log n) | O(log U + t) |
| Revise a score | O(log n) | O(log U + t_old + t_new) |
| Competition rank | O(log n) | O(log U) |
| Select an exact player | O(log n) | O(log U) |
| List q consecutive positions | O(log n + q) | O(q log U) using repeated select |
| Split at a score | O(log n) tree work, plus expected O(m) to transfer ID records | O(U + m) expected, including both rebuilds |
| Merge separated key ranges | O(log(n + m)) tree work, plus expected amortized O(m) for IDs | O(U_A + U_B + n + m) expected |
| Merge interleaved leagues | O(m log(n + m)) tree work, plus ID-table work | Same rebuild as for separated leagues |
| Storage | One node and one ID record per player | U coordinates and count blocks, plus bucket entries and ID records for n players |

For AVL join with a supplied pivot, the tree-only cost is O(1 + |h_left − h_right|). Public ordered merge also has to locate and detach its pivot. The table assumes hash capacity is proportional to the current or recent player count. After extensive deletion, iterating a table also scans retained hash capacity. Both implementations can retain allocated storage after players leave.

## Replay and measurements

`compare [players] [revisions] [score_coordinates]` defaults to `10000 10000 1024`. It pre-generates initial scores and a fixed-seed stream. Each revision is followed by one rank query and one exact-player select. Run the benchmark for each implementation with the same arguments and C++ toolchain to replay identical events. Compare the final checksum columns first, then compare operation times. Each executable reports its own row. After the replay it reads positions 40–50 (clamped for small leagues), splits at the midpoint of the score universe, and merges the two divisions.

The output includes total insertion time, mean update/rank/select time, one range latency, full split and ordered-merge times, and a result checksum. Query timing includes clock overhead. Event generation, Fenwick coordinate preparation, board construction, and audits are outside the timers. Merge timing covers the separated ranges produced by the split. Correctness checks separately exercise interleaved regional leagues.

The original combined driver ran AVL followed by Fenwick on Windows with GCC 13.1 and `-O2`. That run used **1,000,000 players, 1,000,000 revisions, and 100,000 possible scores**. The table reports timings for the public leaderboard operations.

| Structure | Load (ms) | Mean update (ns) | Mean rank (ns) | Mean select (ns) | Split (ms) | Ordered merge (ms) |
|---|---:|---:|---:|---:|---:|---:|
| A · AVL | 1,993.79 | 4,832.53 | 1,808.40 | 1,609.79 | 296.99 | 62.89 |
| B · Fenwick | 811.10 | 1,140.32 | 605.80 | 502.47 | 220.04 | 136.75 |

Both runs produced checksum `6285843591463296880`. Fenwick had lower rank and select latency in this run. AVL merged the split divisions faster. Its full split still moved individual ID records, which helps explain why the logarithmic tree split alone cannot predict the measured time. These are single-run observations with AVL executed first. To study growth, repeat runs while increasing n at fixed U, then vary U at fixed n to change tie-group sizes.

## Correctness checks

`check.cpp` compares size, every player's rank, every selected position, and page contents with a sorted-array oracle. It performs 3,000 deterministic randomized insertions, deletions, and score revisions, checking the implementation on the current branch after each change. It also splits and rejoins the league during that replay.

Additional cases cover the four AVL rotation patterns, sorted insert/delete sequences, different join heights, all-tied inputs, empty leagues, sparse coordinates, signed score extremes, overlapping league scores, different Fenwick universes, duplicate-ID merge rejection, and invalid requests. AVL audits recompute heights and subtree sizes recursively and check ordering, balance, and player records. Fenwick audits check ID ordering within each bucket, player membership, every prefix count, and the total.

## Project requirements

- Support at least one million players and one million operations.
- Implement both structures and a player-ID-to-score hash table from scratch.
- Compare both structures against a sorted-array oracle using differential tests and invariant checks.
- Include adversarial generators, reproducible benchmarks, derivations, and a report.

## Course deliverables

`INTERFACE.md`, `REPORT.md`, `TOOLS.md`, `src/structA/`, `src/structB/`, `app/`, `oracle/`, `adversary/`, `bench/`, `docs/derivations/`, and `results/` will be added during development.
