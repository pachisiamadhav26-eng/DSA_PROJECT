# Independent oracle and differential checks

The differential runner links both the AVL and Fenwick libraries. The sorted
array implements the same `Board` interface. Its independent parser follows the
application's documented error precedence.

Build using the root README, then run:

```sh
./build/bin/oracle oracle/tests/wide_ids.trace
./build/bin/differential --seed 253 --ops 10000 --max-live 128
./build/bin/differential --trace oracle/tests/seasons.trace
./build/bin/differential --scale 1000000
```

The bounded replay compares exact outputs, complete board contents, ownership,
lookup scores, and structural invariants after every command. It then sends the
same complete trace through both real application backends and compares every
output line, covering the handwritten player table as well as the test facade.
Application state is observed through its outputs; it is not introspected after
every command. The sorted reference and application use independent parsers.

AVL inspection recomputes sizes and heights, checks ordering and balance, and
rejects duplicate IDs, cycles and shared nodes. Fenwick inspection checks score
order, bucket ID order/uniqueness, every tree block and the total. The stored block sums determine the prefix sums. Backend unit tests deliberately corrupt metadata to test the checkers.

A mismatch writes `oracle-failure.trace` and `oracle-failure.min.trace` in the
working directory. Reduction preserves the failure signature and reaches
single-deletion minimality, not a globally shortest trace. `--inject-fault`
intentionally corrupts a rank output and must return status 1; normal success is
0 and setup/CLI errors are 2. The reducer cannot contain a process crash.

Full-width IDs are now checked on both real backends and the application.
The million-player test constructs a sorted reference in bulk and audits at
phase boundaries, avoiding quadratic full-state checks after every insertion.
The Python runner separately verifies one million revisions on one million
players, with interleaved queries and a final split and merge.
