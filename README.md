# Rank Queries on a Live Leaderboard

CS 253 / MA 253 Data Structures and Algorithms Laboratory project.

| Structure | Source | Representation |
|---|---|---|
| A | `avl/main.cpp` in the Structure A package | AVL tree keyed by score and player ID, with height and subtree size in each node |
| B | [`fenwick/main.cpp` on Saksham](https://github.com/pachisiamadhav26-eng/DSA_PROJECT/blob/Saksham/fenwick/main.cpp) | Fenwick tree of counts over compressed scores, with sorted IDs at each score |

Each source is a standalone C++17 program. Data and operations are defined in structs above `main()`. Both examples use an `unordered_map` to look up a player's current score by ID. The project-wide requirement for a handwritten player table is separate from these two structure implementations.

## Build and run

From the repository root on `Saksham`:

```powershell
g++ -std=c++17 fenwick/main.cpp -o fenwick.exe
.\fenwick.exe
```

For Structure A, compile `avl/main.cpp` the same way. Each `main()` inserts five players, queries rank and position, lists a page, revises a score, deletes a player, splits at score 100, and joins the divisions.

## Operations

Positions start at 1. Higher scores appear first. Equal scores share their competition rank and players with that score appear in increasing ID order. A revision erases the old (score, ID) key and inserts the new one. The caller supplies a unique ID for each active player.

| Operation | A: AVL | B: Fenwick |
|---|---|---|
| Insert / erase | Rebalance the search path and refresh heights and sizes. O(log n). | Update one score count in O(log U). The sorted ID bucket may shift t entries. |
| Rank | Count strictly higher scores using subtree sizes. O(log n). | One plus the prefix count before the player's score. O(log U). |
| Select position k | Descend using left-subtree sizes. O(log n). | Binary lifting finds the score and the offset into its tie group. O(log U). |
| Extract q positions | Visit the requested portion of the tree. O(log n + q). | Select each requested position. O(q log U). |
| Split by score | Split along one path and join the pieces. O(log n). | Move whole score buckets and rebuild both count arrays. |
| Join | `join(left, pivot, right)` requires every left key to precede the pivot and every right key to follow it. O(1 + absolute height difference). | Merge the two score lists and their ID buckets, then rebuild. The leagues may share scores and must have distinct player IDs. |

Here n is the number of players, U is the number of possible scores, and t is a tie-group size. Supply all possible replay scores to the Fenwick constructor. Scores equal to the split threshold go into the high division.

## Project requirements

- Support at least one million players and one million operations.
- Implement both structures and a player-ID-to-score hash table from scratch.
- Compare both structures against a sorted-array oracle using differential tests and invariant checks.
- Include adversarial generators, reproducible benchmarks, derivations, and a report.

## Course deliverables

`INTERFACE.md`, `REPORT.md`, `TOOLS.md`, `src/structA/`, `src/structB/`, `app/`, `oracle/`, `adversary/`, `bench/`, `docs/derivations/`, and `results/` will be added during development.
