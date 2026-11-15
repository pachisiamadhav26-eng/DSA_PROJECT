# Leaderboard benchmarks

Module owner: Sumit Dobal. The production runner compares competition rank,
selection, score split, separated rejoin, overlapping merge, insertion and
ordered AVL join with a supplied pivot.

```powershell
python tools/project.py bench
python tools/project.py --full bench
```

Full cases use 1,000, 10,000, 100,000 and 1,000,000 players, with unique scores
and with up to 16 players per score. The query sequence uses seed 253.
Preparation and correctness checks sit outside timed sections. Output goes
to `build/benchmark/`, including all five trials, mean/min/max/sample standard
deviation and SVG figures. The checksums are computed independently from the
known workload. `rotations`, `bucket_shifts` and `join_visits` count operations
on the production implementations.

An optional process-level Linux hardware profile is available with
`python bench/run.py --cache`. This requires an installed `perf` and access to
hardware events. It runs one backend per process, repeating the capture five
times. The counts cover process setup and the batch of rank/select queries.
See [REPORT.md](../REPORT.md) for the capture status of the measured machine.

`rank_bench.cpp` and `results/rank-100-smoke.txt` are the original standalone
100-player baseline. The top-level project command runs the production runner.
