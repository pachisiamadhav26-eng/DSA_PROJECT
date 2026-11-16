# Rank benchmark

Build the project from the root README, then run `./build/bin/rank_bench`.

The benchmark uses 100 players in ten score groups. IDs 1–10 score 100,
IDs 11–20 score 90, and the groups continue down to score 10. It checks
that IDs 1, 12, 20, 56, and 100 have ranks 1, 11, 11, 51, and 91.

Each of five timed batches makes 10,000,000 rank calls per backend.
The program reports the mean, minimum, and maximum of the batch averages.
It includes virtual dispatch and excludes setup, player lookup, correctness
checks, and printing. The checksum records the sum of the returned ranks.

Use the Release build with link-time optimization disabled to reproduce the
measurement. See [recorded output](../results/rank-100.txt) and
[build environment](../results/environment.txt).
