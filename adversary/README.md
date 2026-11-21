# Adversarial inputs

Adversarial generator through the root CMake project. The offline Python
runner has been used to build and exercise the project. It checks the real application,
the independent oracle, backend invariants, golden fixtures, generator repeatability,
invalid generator arguments, deliberate failure reduction, and scale workloads.

## Files

- [generator.cpp](generator.cpp) writes replay commands to standard output.
  `make_tall_avl` builds the trees for the deletion cascade and unequal merge.
- [run.py](run.py) builds the project, runs the traces and fixtures, checks
  million-scale output against an independent calculation, and writes
  `validation.json` with command results and hashes of the test inputs.
- [ties.trace](tests/ties.trace) checks shared rank, ordering by ID within a
  tie, and movement after a score change. [ties.expected](tests/ties.expected)
  records the expected reply to each command.
- [wide.trace](tests/wide.trace) checks full-width player IDs and score
  extremes. [wide.expected](tests/wide.expected) records the expected replies.
- [nested_split.trace](tests/nested_split.trace) keeps three boards live,
  reuses a removed player ID, and checks a rejected merge before a successful
  one. [nested_split.expected](tests/nested_split.expected) records the
  expected replies, including errors.
- The [D5 source](../docs/derivations/D5.md) and
  [submission PDF](../docs/derivations/D5_250041008.pdf) prove the AVL split
  bound and explain why the descending workload forces frequent rotations.

## Running the tests

```sh
python3 adversary/run.py
python3 adversary/run.py --ops 1000000 --scale 1000000
./build/bin/generator --case cascade --players 88 > build/cascade.trace
./build/bin/generator --case unequal --players 64 > build/unequal.trace
./build/bin/generator --case ties --players 1000 > build/ties.trace
./build/bin/generator --case mixed --players 64 --ops 1000000 --seed 253 > build/mixed.trace
```

The `ctest` integration test runs the full-scale form above. Allow about one to
two minutes in a Release build. The `avl_test` replay checks the exact error
counts and codes for `churn`, `seasons`, and `errors`, and checks that
`cascade`, `unequal`, and `mixed` produce zero error replies.

Cases: `rotations`, `cascade`, `ascending`, `descending`, `ties`, `churn`, `extremes`,
`seasons`, `unequal`, `errors`, `mixed`, `scale`.

- Rotations/monotone scores exercise rebalancing and subtree metadata.
- Cascade constructs a minimum-node AVL of the largest height within the player
  budget, then deletes along its shorter path. The suite checks that the
  88-player trace builds a height-9 tree without rotating, and that its
  deletion performs eight primitive rotations.
- Ties insert decreasing IDs into one score bucket, forcing vector shifts.
- Churn removes/reinserts IDs, performs no-op revisions and drains the board.
- Extremes exercise int64 limits. Static wide fixtures exercise uint64 limits.
- Seasons exercises consumed sources, empty splits, and reversed merges.
  With at least four players it also merges overlapping AVL key ranges.
- Unequal merges a singleton with the tallest AVL that fits the remaining
  player budget. At budget 64 the boards have heights 1 and 8. The test
  verifies that extracting the singleton as pivot gives ordered join inputs
  of heights 0 and 8.
- Errors exercise syntax, bounds, duplicate IDs and wrong-board requests.
- Mixed uses portable mt19937_64 modulo decisions and a bounded live population.
  It splits at score 0, revises players across both divisions, and merges them back.
  Split and merge commands use fresh board IDs and alternate at intervals of
  `max(10, ops / 200)` commands. Each million-command trace completes 100
  split/merge cycles.
- Scale streams n ADDs and r SETs. Every 1,000 revisions it queries the rank of
  the most recently updated player, the rank of player 1, and the player in
  the middle position. It then inspects the board, including positions
  40 through 50 when they exist, splits it, checks both divisions, and rejoins
  them. Generation uses O(1) auxiliary memory. The application and Python
  validation retain the input. The same case is also replayed through the
  independent oracle at 1,000 players, because the per-command oracle audit
  cannot afford a million.

`--players` is 2..1,000,000; `--ops` is 0..10,000,000. Ops is an exact command
count for mixed, a SET count for scale, and irrelevant to the fixed-pattern cases.
Only mixed uses the seed. The suite uses seeds 253, 42 and 20261115.
`--scale` in the runner controls both the structural scale check and the real
application's equal-size player/revision workload. The runner computes the
expected ranks, positions and pages for that workload independently.

