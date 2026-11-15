# Leaderboard benchmarks

This is the first benchmark for the leaderboard. It checks whether the AVL and Fenwick versions give the right rank on a small board, then times the same rank calls on both. The board is fixed so anyone on the team can reproduce the answers.

`select_split_bench.cpp` adds select and split benchmarks using the same board. The original rank runner can still be run on its own.

## Test board

There are 100 players and 10 distinct scores. The program generates the list from these groups:

| Player IDs | Score |
| --- | ---: |
| 1–10 | 100 |
| 11–20 | 90 |
| 21–30 | 80 |
| 31–40 | 70 |
| 41–50 | 60 |
| 51–60 | 50 |
| 61–70 | 40 |
| 71–80 | 30 |
| 81–90 | 20 |
| 91–100 | 10 |

## Queries and expected answers

Rank is one plus the number of players with a strictly higher score. IDs 12 and 20 both score 90, so they both have rank 11.

| Player ID | Score | Players with a higher score | Expected rank |
| ---: | ---: | ---: | ---: |
| 1 | 100 | 0 | 1 |
| 12 | 90 | 10 | 11 |
| 20 | 90 | 10 | 11 |
| 56 | 50 | 50 | 51 |
| 100 | 10 | 90 | 91 |

Each test case stores the player ID, the player's score, and the expected rank. For example, `{12, 90, 11}` means that player 12 has a score of 90 and should get rank 11. The expected rank is the benchmark's ground truth, worked out from the test board. Ten players scored higher than 90, so the expected rank is 11.

| Structure | Call for player 12 | What the function receives |
| --- | --- | --- |
| AVL | `avl.rank(root, 90)` | The root of the AVL tree and the score `90`. |
| Fenwick | `fenwick.rank(90)` | The score `90`. The `fenwick` object already holds the board's counts. |

The player ID labels the test result. Both functions calculate the rank independently, and the runner compares their answers with the expected rank afterwards. The full app would look up the score from the player ID first. This benchmark already has the score.

## What the timer covers

The program builds both boards and checks all five answers before timing. Each timing run repeats the five queries 2,000,000 times, for 10,000,000 rank calls per structure. Repeating the queries gives the clock enough work to measure without needing a bigger board. The program reports the average time per call over five runs, along with the fastest and slowest run averages. The queries use scores already looked up, so player-ID lookup, board setup, answer checks and printing are outside the timer.

I expect AVL rank to depend on the number of players, since it follows a path through the player tree. I expect Fenwick rank to depend on the number of distinct scores, since it finds the score's index and totals the counts ahead of it. This one small board is a starting point for checking the runner. Later runs at several sizes will show how the costs change as the board grows.

## Run it from the repo root

The structure code currently lives on two branches. These PowerShell commands copy it into the ignored `build/` folder. The runner calls the real rank functions and keeps the original demo entry points out of the way.

```powershell
# Create a local folder for the source copies and executable.
New-Item -ItemType Directory -Force build | Out-Null

# Copy the AVL source from the local snapshot of its remote branch.
git show origin/prince-chavda:avl.cpp | Set-Content -Encoding ascii build/avl.cpp

# Copy the Fenwick source from the local snapshot of its remote branch.
git show origin/Saksham:Fenvick.cpp | Set-Content -Encoding ascii build/Fenvick.cpp

# Compile the rank runner as C++17 and find both source copies in build/.
g++ -std=c++17 -O2 -Ibuild bench/rank_bench.cpp -o build/rank_bench.exe

# Check the five expected ranks and run the small timing trial.
.\build\rank_bench.exe
```

## Select and split

Select takes a one-based position and returns a player. Equal scores are listed by increasing player ID. We check both the returned ID and score against these answers before timing select.

| Position | Expected player ID | Expected score |
| ---: | ---: | ---: |
| 1 | 1 | 100 |
| 12 | 12 | 90 |
| 20 | 20 | 90 |
| 56 | 56 | 50 |
| 100 | 100 | 10 |

The calls are `avl.select(root, position)` and `fenwick.select(position)`. Each timing run repeats the five queries 2,000,000 times. I expect AVL select to follow a path through the player tree, while Fenwick select searches the score counts and then indexes the sorted IDs in that score group.

Split uses a score cutoff of 60. The high division must contain IDs 1–50, including the players scoring exactly 60. The low division must contain IDs 51–100. We check the size, ID order, and score of every player in every split result.

I expect AVL split to follow a search path and join pieces of the tree, with cost growing logarithmically with player count. Fenwick moves the relevant score buckets and rebuilds the count arrays, with cost growing with the number of score coordinates. Both create two divisions with the same players.

Split changes the input board, so each timing run uses 10,000 separate copies of the original board prepared before the timer starts. The timed batch includes the split calls and storing their output boards. Checks and cleanup happen afterwards. Both benchmarks report the mean and minimum and maximum run averages over five runs. A single board size is an initial timing check; measuring growth needs additional sizes.

After preparing the source copies with the commands above, run these commands from the repo root.

```powershell
# Compile the select and split runner.
g++ -std=c++17 -O2 -Ibuild bench/select_split_bench.cpp -o build/select_split_bench.exe

# Create the folder for saved results.
New-Item -ItemType Directory -Force results | Out-Null

# Run the checks and benchmarks and save terminal output and errors.
.\build\select_split_bench.exe 2>&1 | Tee-Object -FilePath results/select-split-100-smoke.txt
```
