# Oracle and differential testing

Module owner: Om Dehariya.

`SortedArray` uses a vector ordered by decreasing score, then increasing ID.
It supports the full signed 64-bit score and unsigned 64-bit ID domains.
A binary search finds the first score no longer strictly higher. Add one to
that index for competition rank. Selection reads the one-based position.
Insertion and deletion take O(n), and merging two sorted ranges takes
expected O(n+m) including duplicate-ID validation.

`differential.cpp` runs the same trace through the sorted array and both
production `Board` factories. For each command it compares results, board
membership, score lookups and full ordered contents. The AVL audit recomputes
height and subtree size from child pointers, checks global key bounds and
balance, and verifies unique IDs. Fenwick's audit reconstructs bucket prefix
counts and checks every stored Fenwick block, coordinates and ID order.

Run from the repository root:

```powershell
python tools/project.py test
python tools/project.py --full test
./build/project/oracle.exe oracle/tests/seasons.trace
./build/project/differential.exe --seed 253 --ops 1000000 --max-live 48
```

The CLI test also runs the production replay app with both backends and compares
each fixed fixture to hand-written expected output. The differential runner
uses its independent replay and state model to check the backend after every
command. The scale check compares one million rank/select queries on a
million-player board and audits partitions at phase boundaries.

To replay an existing trace, pass `--trace PATH`. To save a generated trace,
pass `--dump PATH`. A mismatch writes `oracle-failure.trace` (the failing
prefix) and `oracle-failure.min.trace` (same failure signature, 1-minimal under
single-command deletion). A crash outside normal exception handling requires
external debugging. The runner self-tests the shrinker by injecting a bad rank.

D4 covers size preservation through rotations and why select takes logarithmic
time. See `docs/derivations/D4-om.pdf`.
