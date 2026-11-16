# Project 2: Rank queries on a live leaderboard

CS 253 / MA 253 Data Structures and Algorithms Lab

## The problem

Maintain a game leaderboard for one million players whose scores change. The application must answer a player's rank, return the players in a range of positions, split a board at a score threshold, and merge two leagues. The assignment compares an augmented AVL tree with a Fenwick tree on the same replay, including one million score revisions interleaved with queries and season operations.

Players sort by highest score first. Within a tie, the smaller ID comes first. Scores `100, 90, 90, 70` therefore have **ranks** `1, 2, 2, 4` and **positions** `1, 2, 3, 4`. Rank is one plus the number of players with strictly higher scores. Position identifies a specific player in the ordered list.

## How the parts fit together and who owned them

- **Madhav Pachisia — Architect, D3 (`app/`, `include/`).** Defined the shared board interface and replay rules. The application reads each command, checks the board and player IDs, and calls the chosen backend. Its handwritten hash table maps player ID to current score, so a revision can remove the old entry before inserting the new one.
- **Prince — AVL owner, D1 (`src/structA/`).** Each tree node stores its height and subtree size. Rotations repair balance after changes. Subtree sizes let rank and select skip whole subtrees. Split follows a search path and joins the pieces back into two balanced trees.
- **Saksham Bhatia — Fenwick owner, D2 (`src/structB/`).** The replay collects possible scores up front and assigns each distinct score an index. A Fenwick tree stores counts at those scores. Rank reads a prefix count. Select uses binary lifting to find the score bucket, then indexes its sorted player IDs. Season operations rebuild the counts.
- **Om Dehariya — Oracle owner, D4 (`oracle/`).** A sorted array serves as the reference answer. The differential checker runs the same commands through the reference and both backends, checks their internal invariants, and can reduce a failing trace.
- **Bianca — Adversary, D5 (`adversary/`).** Generates difficult cases such as ties, a deletion that triggers several AVL rotations, and a merge of trees with very different heights. The runner checks generated traces and the million-scale replay.
- **Sumit — Analyst, D6 (`bench/`).** Owned the rank benchmark and the experiment design. The current scaling harness also measures select, splits, and rejoins. It reports the supplied-pivot AVL join separately from a public merge that validates player IDs.

The assignment brief defines the roles and derivations. The ownership above follows the project README and contribution notes. D1–D6 identify the assigned derivations.

## How to run it

Use a C++17 compiler, CMake 3.16+, and Python 3.8+ for tests. From the workspace containing `DSA_PROJECT-final`:

```sh
cd DSA_PROJECT-final
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 4
./build/bin/leaderboard avl app/examples/basic.trace
./build/bin/leaderboard fenwick app/examples/basic.trace
ctest --test-dir build -C Release --output-on-failure
```

Both backends print the same 12 lines. In the example, `RANK 0 8` gives `RANK 2` while `SELECT 0 3` gives `PLAYER 8 90`. Splitting at 90 puts three players in the high division. After the merge and a revision to 110, player 8 has rank 1. The full expected output is in [basic.expected](DSA_PROJECT-final/app/examples/basic.expected).

**Full-scale verification and benchmarks**

```sh
python3 adversary/run.py --ops 1000000 --scale 1000000
./build/bin/scaling_bench --quick
```

Run `scaling_bench` without `--quick` for all benchmark sizes. To reproduce the build, tests, demo, CSVs, and figures in one command, install the plotting dependency and run:

```sh
python3 -m pip install -r bench/requirements.txt
sh scripts/run_all.sh
```

See the [course problem statement](CS253_MA253_Lab_Projects.docx.pdf), pages 6–7, and the [command interface](DSA_PROJECT-final/INTERFACE.md).
