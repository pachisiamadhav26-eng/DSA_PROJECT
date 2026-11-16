## Rank Queries on a Live Leaderboard

Oracle contribution (Om): see [`oracle/README.md`](oracle/README.md) for the
sorted-array reference, differential tests, invariant checkers, failure shrinker,
and [D4 proof](docs/derivations/D4.pdf). Run `./oracle/run_tests.ps1` in PowerShell to build and execute the
oracle test suite against pinned teammate implementations.

CS 253 / MA 253 Data Structures and Algorithms Laboratory project.

We compare two ways to maintain a leaderboard as players' scores change. The main measurements are **rank and select latency as the player count grows**, and **the time to split and rejoin a league**. Both structures must return the same ranks, players and divisions for the same sequence of operations.

| Structure | Source | Representation |
|---|---|---|
| A | [`avl.cpp` on Prince's branch](https://github.com/pachisiamadhav26-eng/DSA_PROJECT/blob/prince-chavda/avl.cpp) | AVL tree keyed by score and player ID, with height and subtree size in each node |
| B | [`src/structB/fenwick.cpp` on Saksham's branch](https://github.com/pachisiamadhav26-eng/DSA_PROJECT/blob/Saksham/src/structB/fenwick.cpp) | Fenwick tree of counts over compressed scores, with sorted IDs at each score |

Both structures implement [the shared C++17 Board interface](https://github.com/pachisiamadhav26-eng/DSA_PROJECT/blob/madhav/INTERFACE.md). The Architect application uses a handwritten player table to look up each player's current score.

## What we compare

**Rank and position have different meanings when players tie.** Suppose IDs 1, 2, 3 and 4 have scores 100, 90, 90 and 70. Their ranks are 1, 2, 2 and 4. Players 2 and 3 share rank 2, but `select(3)` must return player 3 because ties are listed by increasing ID.

In the table, **n** is the number of active players, **U** is the number of possible distinct scores supplied to the Fenwick tree, **t** is the number of players tied at the affected score, and **q** is the number of positions returned. These are predicted costs for the current code.

| Operation | Structure A — AVL | Structure B — Fenwick |
|---|---|---|
| Rank a player | Count strictly higher scores using subtree sizes. **O(log n)**. | Scores are compressed highest first. Sum the counts before the player's score and add one. **O(log U)**. |
| Select the player at position k | Descend using the left subtree size. **O(log n)**. | Use binary lifting to find the score bucket, then use the remaining position to index its sorted IDs. **O(log U)**. |
| List positions 40–50 | Traverse the relevant part of the tree and return 11 players. **O(log n + q)**. | Call select for each requested position. **O(q log U)**. |
| Insert or delete a player | Change one node, rebalance, and update subtree sizes. **O(log n)**. | Update a count and insert or erase an ID in its sorted vector. **O(log U + t)** in the worst case. |
| Revise a score | Erase the old key and insert the new key. **O(log n)**. | Move the ID between two score buckets and update both counts. **O(log U + t_old + t_new)**. |
| Split at a score threshold | Follow one search path and join the pieces. **O(log n)**. | Move whole ID vectors and rebuild both count arrays. **O(U)**. Scores equal to the threshold enter the high division in both structures. |
| Rejoin the two divisions | With an ordered left tree, separating pivot and ordered right tree, join takes **O(1 + height difference)**. Finding and removing the pivot in the demo adds **O(log n)**. | Merge the score lists and rebuild the count array. Worst case **O(U₁ + U₂ + n₁ + n₂)** when IDs at shared coordinates must be copied. |
| Storage | One node per player, including two pointers, height and subtree size. **O(n)**. | One count per score coordinate and one ID entry per active player. **O(U + n)**. |

## How we will measure and check the comparison

1. **Rank and select latency.** Replay identical score revisions and queries at increasing player counts, up to one million players and one million revisions. Record rank and select times separately. Record U as well as n so the results show what changes when many players share a score.
2. **Split and join time.** Split both boards at the same threshold and verify the same players enter each division. Time rejoining those divisions, including pivot preparation for the AVL. Also time the AVL join itself with an already supplied pivot and vary the difference in tree heights. A single AVL join requires ordered key ranges, so use separated divisions for the direct comparison.
3. **Correctness under updates.** Use a sorted array of `(score, player ID)` as the reference. Compare each player's competition rank, the exact ID at position k, and the full page of IDs. Include equal scores, updates into and out of ties, and insert/delete sequences that force AVL rotations. Recompute AVL subtree sizes during correctness checks and verify Fenwick prefix counts against the array.
4. **Prediction versus observation.** Compare measured times with the table above. For example, at fixed U, increasing a tie group's size can make Fenwick updates slower because IDs shift in the vector. Keep oracle checks and score preprocessing outside query timings.

## Build and run

From the repository root on `om-oracle`, use PowerShell with Git and a C++17 compiler:

```powershell
git fetch origin
./oracle/run_tests.ps1 -Quick
```

Omit `-Quick` to run the full oracle validation suite. The runner builds the
actual replay app with pinned teammate revisions and tests both backends.

## Project requirements

- Support at least one million players and one million operations.
- Implement both structures and a player-ID-to-score hash table from scratch.
- Compare both structures against a sorted-array oracle using differential tests and invariant checks.
- Include adversarial generators, reproducible benchmarks, derivations, and a report.

## Course deliverables

`INTERFACE.md`, `REPORT.md`, `TOOLS.md`, `src/structA/`, `src/structB/`, `app/`, `oracle/`, `adversary/`, `bench/`, `docs/derivations/`, and `results/` will be added during development.
