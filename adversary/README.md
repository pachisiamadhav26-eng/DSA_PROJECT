# Adversary contribution

Prepared for the adversary role on `Bianca_adversary`, 15 November 2026.
The role/branch association is inferred from the local checkout and branch name;
the repository does not contain a complete official allocation or assignment sheet.

An adversary here means a test-input designer: construct legal and deliberately
invalid leaderboard commands that reveal correctness mistakes and expensive cases.
It does not mean cybersecurity attacks.

## Run the complete package

Prerequisites: Python 3 and a C++17 compiler. From the extracted offline kit root:

```powershell
python adversary/run.py
# If g++ is not on PATH:
python adversary/run.py --compiler C:/msys64/ucrt64/bin/g++.exe
# Longer correctness run (three million-command seeds, then scale check):
python adversary/run.py --ops 1000000 --scale 1000000
```

The offline kit contains pinned teammate sources in `reference/`. The contribution
ZIP deliberately contains only your new files. After copying it into a clone,
`run.py` reads teammate dependencies with `git show` at the documented commits.
If those commits are missing, run `git fetch origin`, then rerun. The script itself
does not fetch, commit, push, check out, merge, or modify tracked teammate files.
All generated files go to `build/adversary/`, already ignored by the repository.
The script exits nonzero on a build or validation failure.

## Generate a workload without running correctness tests

```powershell
New-Item -ItemType Directory -Force build/adversary | Out-Null
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic adversary/generator.cpp -o build/adversary/generator.exe
./build/adversary/generator.exe --case ties --players 1000 | Set-Content -Encoding ascii build/adversary/ties.trace
./build/adversary/generator.exe --case mixed --players 64 --ops 1000000 --seed 253 | Set-Content -Encoding ascii build/adversary/mixed.trace
./build/adversary/generator.exe --case scale --players 1000000 --ops 1000000 | Set-Content -Encoding ascii build/adversary/scale.trace
```

`--players` accepts 2 through 1,000,000; `--ops` accepts 0 through 10,000,000.
`--ops` is an exact command count for `mixed`. For `scale` it counts SET revisions
after ADD setup, and the generator appends four observation commands. Other cases
have a fixed pattern whose length depends on players where applicable.
`--seed` affects only `mixed`. Invalid options/numbers return exit code 2.

## Cases and their purposes

| Case | Construction | Bug or cost targeted |
| --- | --- | --- |
| rotations | Four three-key shapes, followed by deletion and empty split/rejoin | LL/RR/LR/RL rotations, stale heights and subtree sizes |
| ascending | Insert increasing scores | One-sided insertion pressure and metadata maintenance |
| descending | Insert decreasing scores | Mirror insertion pressure |
| ties | Same score, decreasing IDs; move a player out of and into the tie | Rank versus position, ID order, worst-case front insertion in Fenwick vectors |
| churn | Remove/re-add IDs, unchanged SET, then drain the board | Two-child deletion, exact-key removal, stale lookup entries, empty board |
| extremes | INT64_MIN/MAX scores, ID zero and INT_MAX | Overflow-prone comparisons, threshold boundaries |
| seasons | Equal cutoff, overlap introduced after split, empty sides, reversed merge | Equality belongs in high, ownership, arbitrary merge, consumed boards |
| unequal | One-player high division versus n-1 low players | Ordered join with unequal tree heights |
| errors | Duplicate IDs, wrong board, absent players, invalid positions/ranges, malformed integers | Error handling and unchanged state after rejection |
| mixed | Seeded updates/removals/ranks/selects with at most n active IDs | Long interleavings, ties, no-op SET, reproducibility |
| scale | Stream n ADDs then ops SETs, cycling IDs through new scores | Million-player/million-revision workload generation |

Monotone insertion does not unbalance a correct AVL: it forces the implementation
to rebalance. A tie-heavy input does not make Fenwick prefix sums linear: shifting
IDs in its sorted vector is the expensive part. This distinction matters in a viva.

## What each new file does

| File | Responsibility |
| --- | --- |
| `generator.cpp` | Standalone C++17 workload generator; no teammate code required |
| `run.py` | Read pinned dependencies, compile, generate twice to check determinism, run oracle comparison and structural audits, verify known outputs, exercise fault reduction, run scale test |
| `tests/ties.trace` | Small manually checkable replay covering tie order, rank, revision and split/merge |
| `tests/ties.expected` | Independently calculated output for that replay |
| `tests/wide.trace` | Full uint64 IDs and int64 scores; oracle-only because legacy demos use int IDs |
| `tests/wide.expected` | Independently calculated wide-ID results |
| `validation/` | Actual run evidence; JSON command records, dependency hashes and summaries |
| `TOOLS.md` | Disclosure of generated work and human review still needed |
| `../D5.md` | Explanations/derivations supporting these adversarial workloads |
| `../ADVERSARY_HANDOFF.md` | How this role fits the team and how to submit it |

## Generator walkthrough

1. `number()` rejects signed/nondecimal option values; `main()` checks limits.
2. `Writer::line()` prints a whitespace-separated command. `add()` targets board
   zero; `page()` emits size, endpoint selections and up to twelve entries.
3. `generate()` chooses one adversarial family. Each SPLIT/MERGE uses fresh board
   IDs. Queries on consumed boards are intentional rejection checks.
4. `mixed` uses `mt19937_64`, explicit modulo choices, a live-slot bitmap and
   current scores. Its memory is O(players), runtime/output O(ops+players).
   It does not use container iteration order or implementation-dependent distributions.
5. `scale` stores no player population: its counters produce commands directly,
   taking O(players+ops) time and O(1) generator auxiliary memory. The output file
   and the replay consumer still need their own storage.
6. Output goes only to stdout; diagnostics go to stderr. Flush/write failures
   return nonzero. Redirect each run to a separate file to preserve evidence.

## Validation boundaries

The checker and reducer are Om's existing work; this contribution reuses them.
They validate both actual standalone teammate structures through Om's adapters,
not the architect's production application or its handwritten hash table.
The adapters add bounds checks and implement overlap merging; passing them does
not establish that the raw demos expose the full final interface.

Default targeted tests use 64 players; three random seeds use a bounded 64-ID
population. The imported runner prints `max_live=128` for trace inputs because
that is its CLI default; it does not measure the population in an external trace.
The separate `--scale` check constructs the requested population and queries it.
For external traces the imported checker also prints its default `seed=253`;
the generator command and trace filename identify the actual generation seed.
The generator's million-revision trace was generated/count-checked; it was not
run through a sorted-vector oracle at one million simultaneous players. Such a
replay with full audits per command would be prohibitively expensive.

`oracle-failure.trace` and `oracle-failure.min.trace` under build are intentional
fault-injection artifacts on a passing suite. Fault injection changes an AVL rank
output in the test harness; it does not change teammate sources. The existing
reducer returns a 1-minimal reproducer, not a globally shortest one, and cannot
recover from arbitrary process crashes.

See `review_docs/PROJECT_ANALYSIS.md` in the offline kit for interface discrepancies
that the team must resolve before using one final replay application.
