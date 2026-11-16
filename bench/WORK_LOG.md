# Member 6 work log

This log tracks my work on the leaderboard benchmarks. Each entry records what I worked on, how I checked it, and where the team can find the output.

## 1. Read the project brief and planned the benchmarks

- Read the project brief, repository README, and shared interface. Identified four main measurements for the two structures: rank, select, score-threshold split, and rejoin of the resulting divisions.
- Kept ordered AVL join with a supplied pivot separate from rejoining two divisions. The interface also treats merging regional leagues with overlapping scores as a separate operation.
- Chose rank as the first benchmark so I could get one small comparison working before adding more operations.

## 2. Built the first rank benchmark

- Added [rank_bench.cpp](practice/rank_bench.cpp) and documented how to build and run it in the [practice README](practice/README.md).
- Used the real rank functions from the AVL and Fenwick branches. Both structures start with the same 100 players, grouped into 10 distinct scores.
- Wrote five rank queries with expected answers worked out from the board. Two players share a score so the checks cover competition ranking with ties. The runner checks both answers before timing and times only the rank calls.
- Used Codex to draft the runner and benchmark documentation and to check that the runner builds and executes against both structure files.

## 3. Ran the checks and saved the first result

- Both implementations returned `1, 11, 11, 51, 91` for the five queries. Each structure was timed over five runs, with the mean and fastest and slowest run averages reported.
- Saved the terminal output, now in [results/practice/rank-100-smoke.txt](../results/practice/rank-100-smoke.txt). The original code, instructions, and result are in commit `cfd8ccf` on `sumit`.
- This result is a check that the benchmark works on the small board. The full comparison will use several board sizes and a larger workload.

## 4. Added select and split benchmarks — 16 November 2026

- Added `select_split_bench.cpp` using the same 100 players and 10 scores as the rank benchmark. Codex helped draft the code and documentation.
- Added five select queries that check the returned player ID and score, including two positions within the same tied-score group.
- Added a score-60 split test. The expected divisions are IDs 1–50 and 51–100. The runner checks every player in both divisions after each split.
- Wrote the initial select and split cost expectations in the benchmark README before running the new timings. Each operation has five timing runs. Split uses fresh input boards prepared outside the timer.
- The documented run command saved output and errors, now located in `results/practice/select-split-100-smoke.txt`.
- Compiled with MinGW using C++17, optimization, and warning flags. All five select answers matched in both structures, and all split result checks passed. Saved the run output in the result file above.

## 5. Prepared the formal benchmark foundation — 16 November 2026

- Reviewed all fetched branches against the shared interface. Temporary compilation checks found that the latest AVL has namespace/factory mismatches and the oracle needs `contains`. The Fenwick translation unit compiled. Teammate owners are handling compatibility.
- Moved the older trials and their outputs into `bench/practice/` and `results/practice/`, preserving the original work. Pinned their documented build commands to the old demo commits.
- Added the standalone replay generator `workload.cpp` and validity/repeatability checks. Its workload has exactly one real score revision followed by one rank and one select query per group. Codex drafted this foundation and the documentation.
- Added the D6 prediction draft in `docs/derivations/D6.md` before collecting formal measurements. The next implementation stage is the shared-interface timing runner once compatible dependencies are available.
- Compiled the generator with MinGW C++17 and warning flags. The replay-order, ID/position validity, real-revision, exact-count, reproducibility and invalid-option checks all passed. Generated `build/bench/check.trace` with 100 players, 20 revisions, 20 rank queries and 20 select queries, plus its settings file. These generated files stay local under ignored `build/`.

## 6. Implemented the shared-interface experiment codebase — 17 November 2026

- Added the C++ harness and scripts for dependency-pinned builds, small/full suites, per-run CSVs, backend comparisons, counters, trace inputs, summaries and Matplotlib figures. The harness reuses the Architect's replay engine and unchanged shared Board contract.
- Added a Windows WPR controller that brackets query work with a process handshake. WPR accepted its profile XML. Hardware sampling, administrative collection and WPA analysis have not been validated.
- Expanded runner verification to 22 scenarios, including an intentionally wrong rank, known replay outputs, duplicate-output protection, empty splits, join/counter helper plumbing, optional oracle plumbing, and the profile handshake. Test-only vector backends and the oracle fixture are explicitly labelled and never linked into the production build.
- Ran the actual Fenwick quick development suite with five repetitions. Query answers, split/rejoin outputs, counter collection and result generation passed. The development outputs are under ignored `build/bench/`; these are not the formal final comparison.
- Prepared the D6 PDF and report structure. D6 records earlier practice timings and distinguishes current predictions from measured results. Codex helped produce the code and documents; Sumit's review remains necessary.
- Full experiments remain dependent on the owners' compatible AVL/oracle implementations and helper definitions. No formal large-scale results or cache conclusions are claimed.
- Verified a 10,000-player real Fenwick query run and ran Bianca's actual tie trace through the Architect replay with Fenwick for five repetitions. Its complete output digest matched her expected output. Statistical and Matplotlib-output checks passed. Detailed preparation evidence is recorded in `bench/VALIDATION.md`.

## 7. Verified the updated component integration — 17 November 2026

- Fetched and reviewed Prince `03f3f57`, Saksham `0d5ad68`, and Om `6a25781` against Architect `a4dc438`. The previously reported interface blockers are fixed.
- Built the harness with the actual updated oracle and both backends/helper sets enabled. The integrated quick comparison passed.
- Exported Om's code unchanged to an ignored review folder, built against the current backend refs, and passed his quick and full correctness suites. Full checks include the bounded million-command seed, two 100,000-command seeds, and a separate million-player/million-query scale test.
- Om's default Fenwick dependency pin still names `c921385`; explicitly selected current `0d5ad68` for the checks. Our benchmark build already resolves the requested current refs and records their SHAs.
- D6 requires a dated update for the latest public merge implementations before collecting formal timing results. Hardware profiling remains pending.

## 8. Simplified and prepared the Analyst contribution — 17 November 2026

- Reduced the main entry point to build, small real-component checks and the selected suite. Replaced the old fake backend/oracle fixtures with checks against the real implementations.
- Removed the redundant standalone input generator. Ordinary inputs are generated inside the harness; supplied Adversary traces use the shared replay format. Historical practice programs and outputs remain under `practice/`.
- Rewrote D6 with the rank/select derivations, split height charges, ordered-join bound, current public merge costs, symbolic cost models and experiment settings. Updated the PDF and report outline.
- Prepared the requested folder layout, instructions and dependency record. This preparation uses compile/syntax and document checks. The benchmark pipeline, formal measurements and hardware profiling are deferred at Sumit's request.
