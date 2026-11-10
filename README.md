# Rank Queries on a Live Leaderboard

CS 253 / MA 253 Data Structures and Algorithms Laboratory project.

Structure 1 uses an AVL tree. Structure 2 uses a Fenwick tree and is on the `Saksham` branch. Both implementations use structs for data and ordinary functions for operations. The examples use signed 64-bit player IDs and scores and support the project's million-player scale with integer counts.

## Run the demo

From the repository root on `Saksham`, use these PowerShell commands:

```powershell
cd fenwick
g++ -std=c++17 main.cpp fenwick.cpp ../player_table.cpp -o demo.exe
.\demo.exe
```

For Structure 1, use `cd avl` and compile `main.cpp avl.cpp ../player_table.cpp`. On Linux or macOS, run the executable as `./demo.exe`. Each branch has its own `leaderboard.h` with the structs and function declarations. Start with `main.cpp` to see a five-player example. The Fenwick algorithm is in the first three functions of `fenwick.cpp`.

## How the two structures work

**Fenwick.** Each distinct possible score has a numbered position, starting at 1, in ascending score order. The tree stores counts. `add` changes a count, `prefixSum` counts players up to a score, and `findKth` uses binary lifting to find the first position whose prefix count reaches k. Give `initBoard` all scores that can appear in the replay. An update to an unknown score returns `false`.

If player 7 moves from 90 to 100, look up their old score, subtract one player at the position for 90, and add one at the position for 100. Each score also has a sorted list of player IDs so an exact position query can return a player inside a tie. Split and merge rebuild the count arrays.

**AVL.** Each node stores a player, left and right pointers, a height and a subtree size. Rotations update both numbers. Rank counts players with a higher score. Select compares the requested position with the size of the left subtree. Split uses AVL joins. Merge joins disjoint ordered ranges and inserts donor nodes for interleaved ranges.

**Player lookup.** The course specifies a handwritten ID-to-score hash table. It lives in `player_table.cpp`, outside both trees. For example, with eight buckets, IDs 7 and 15 both use bucket 7. Searching that short list finds the right ID. The table uses `id % bucketCount`, handles negative IDs, and doubles its bucket count as it fills.

## Operations

Higher scores come first. Tied players share a rank and are listed by increasing ID. For scores 100, 90, 90 and 70, ranks are 1, 2, 2 and 4.

| Function | Behavior |
|---|---|
| `insertPlayer(board, id, score)` | Add a new player. Duplicate IDs return false. |
| `erasePlayer(board, id)`, `updateScore(board, id, score)` | Remove a player or change their score. Missing IDs return false. |
| `rankOf(board, id)` | One plus the number of strictly higher scores. Returns 0 for a missing ID. |
| `selectPlayer(board, k, answer)` | Write the player at one-based position k into `answer`. Invalid positions return false. |
| `listPositions(board, first, last)` | Return an inclusive page. An invalid request gives an empty vector. |
| `splitBoard(board, threshold, high)` | Move players scoring at least the threshold into an empty board called `high`. |
| `mergeBoards(board, other)` | Move players into `board` and empty `other`. A shared ID or self-merge returns false. |

Create a board with `Board board;` and call `initBoard` before inserting players. Pass boards by reference. The AVL board owns its nodes, so copying an active AVL board by assignment would copy its pointers. Call `clearBoard` on each board at the end.

## Comparisons

Let n be the active player count, U the possible score count, t a tie-group size, q a page length and m the number of players moved. Player-table costs assume well-distributed IDs and amortized resizing. A long chain of colliding IDs increases lookup cost.

| Operation | AVL | Fenwick |
|---|---|---|
| Insert or erase | O(log n) | O(log U + t) because a sorted vector can shift IDs |
| Update score | O(log n) | O(log U + t_old + t_new) |
| Rank and exact-player select | O(log n) | O(log U) |
| List q positions | O(log n + q) | O(q log U) |
| Split | O(log n) tree work plus expected O(m) for ID records | Expected O(U + m) |
| Merge | Ordered ranges take O(log(n + m)) tree work plus expected amortized O(m) for IDs. Interleaved ranges take O(m log(n + m)) tree work. | Rebuild the combined buckets and counts in expected O(U1 + U2 + n1 + n2). |

## Run the checks and benchmark

From `fenwick/` on `Saksham`:

```powershell
g++ -std=c++17 -O2 check.cpp fenwick.cpp ../player_table.cpp -o check.exe
.\check.exe
g++ -std=c++17 -O2 compare.cpp fenwick.cpp ../player_table.cpp -o compare.exe
.\compare.exe 10000 10000 1024
```

For AVL, run the same commands from `avl/` with `avl.cpp` in place of `fenwick.cpp`. The checker compares the board with a sorted list after 3,000 inserts, deletions and score changes. It checks split, merge, ties and invalid requests. The AVL checker also verifies heights, balance and subtree sizes. The Fenwick checker verifies every prefix count.

The benchmark arguments are players, revisions and possible scores. Each revision is followed by a rank query and an exact-player selection. Use the same arguments and compiler settings for both programs, then compare the checksums, average query times and total split/merge times. Timing excludes data generation and includes clock overhead. The timed merge reunites the two divisions from a split. A project-sized example is `compare.exe 1000000 1000000 100000`.

## Example measurement

In a single GCC 13.1 run with optimization enabled (`-O2`), one million players, one million revisions and 100,000 possible scores, both programs produced checksum `77401577035553977`.

| Implementation | Mean rank | Mean select | Split | Merge after split |
|---|---:|---:|---:|---:|
| AVL | 2,201 ns | 1,926 ns | 363 ms | 83 ms |
| Fenwick | 704 ns | 619 ns | 376 ms | 121 ms |

These timings include player lookup and membership updates. They describe one run on this machine.

## Project requirements

- Support at least one million players and one million operations.
- Implement both structures and a player-ID-to-score hash table from scratch.
- Compare both structures against a sorted-array oracle using differential tests and invariant checks.
- Include adversarial generators, reproducible benchmarks, derivations, and a report.

## Course deliverables

`INTERFACE.md`, `REPORT.md`, `TOOLS.md`, `src/structA/`, `src/structB/`, `app/`, `oracle/`, `adversary/`, `bench/`, `docs/derivations/`, and `results/` will be added during development.
