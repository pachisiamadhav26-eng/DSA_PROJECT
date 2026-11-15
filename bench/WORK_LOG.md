# Member 6 work log

This log tracks my work on the leaderboard benchmarks. Each entry records what I worked on, how I checked it, and where the team can find the output.

## 1. Read the project brief and planned the benchmarks

- Read the project brief, repository README, and shared interface. Identified four main measurements for the two structures: rank, select, score-threshold split, and rejoin of the resulting divisions.
- Kept ordered AVL join with a supplied pivot separate from rejoining two divisions. The interface also treats merging regional leagues with overlapping scores as a separate operation.
- Chose rank as the first benchmark so I could get one small comparison working before adding more operations.

## 2. Built the first rank benchmark

- Added [rank_bench.cpp](rank_bench.cpp) and documented how to build and run it in the [benchmark README](README.md).
- Used the real rank functions from the AVL and Fenwick branches. Both structures start with the same 100 players, grouped into 10 distinct scores.
- Wrote five rank queries with expected answers worked out from the board. Two players share a score so the checks cover competition ranking with ties. The runner checks both answers before timing and times only the rank calls.
- Used Codex to draft the runner and benchmark documentation and to check that the runner builds and executes against both structure files.

## 3. Ran the checks and saved the first result

- Both implementations returned `1, 11, 11, 51, 91` for the five queries. Each structure was timed over five runs, with the mean and fastest and slowest run averages reported.
- Saved the terminal output in [results/rank-100-smoke.txt](../results/rank-100-smoke.txt). The code, instructions, and result are in commit `cfd8ccf` on `sumit`.
- This result is a check that the benchmark works on the small board. The full comparison will use several board sizes and a larger workload.

## 4. Added select and split benchmarks — 16 November 2026

- Added `select_split_bench.cpp` using the same 100 players and 10 scores as the rank benchmark. Codex helped draft the code and documentation.
- Added five select queries that check the returned player ID and score, including two positions within the same tied-score group.
- Added a score-60 split test. The expected divisions are IDs 1–50 and 51–100. The runner checks every player in both divisions after each split.
- Wrote the initial select and split cost expectations in the benchmark README before running the new timings. Each operation has five timing runs. Split uses fresh input boards prepared outside the timer.
- The documented run command saves output and errors in `results/select-split-100-smoke.txt`.
- Compiled with MinGW using C++17, optimization, and warning flags. All five select answers matched in both structures, and all split result checks passed. Saved the run output in the result file above.

## Next entry

Continue the D6 cost model and record the next benchmark change with the date, how it was checked, and the commit or result file.
