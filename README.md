# Analyst contribution - Sumit

CS 253 / MA 253 Project 2: Rank queries on a live leaderboard.

Author: Sumit, roll number 250041038. This branch contains the Analyst contribution. The integrated team project is maintained on `main`.

| Path | Contents |
| --- | --- |
| [bench/](bench/README.md) | Benchmark harness, build/run scripts, counters, profiling and checks |
| [results/](results/README.md) | Raw timings, summary tables, graphs and experiment evidence |
| [docs/derivations/D6.md](docs/derivations/D6.md) | Cost-model derivation source |
| [docs/derivations/D6_250041038.pdf](docs/derivations/D6_250041038.pdf) | Two-page D6 submission |
| [REPORT.md](REPORT.md) | Analyst measurements, prediction comparisons and limitations |
| [TOOLS.md](TOOLS.md) | Tools used for the Analyst contribution |

The harness measures the actual teammate implementations through the shared interface. Its default build exports the recorded remote-branch sources into the ignored `build/` directory; teammate implementation files do not need to be tracked on this branch. See [bench/README.md](bench/README.md) for dependencies and options.

```powershell
# Refresh the remote-tracking component branches used by the build.
git fetch origin

# Install plotting and PDF dependencies in the ignored build directory.
python -m pip install --target build/bench/python -r bench/requirements.txt

# Compile the actual components, run checks and execute the quick suite.
python bench/run_all.py --suite quick

# Run the full Analyst experiment suite on the selected component versions.
python bench/run_all.py --suite full
```

Saved formal measurements identify the original component versions and machine. A new run after fetching may use newer versions and must be reported separately. Use `--local` only in an integrated checkout with the required teammate source files.

PR #5 already merged the Analyst contribution into `main`. The subsequent Analyst-only cleanup on this branch is for its standalone file layout; do not merge those teammate-file deletions back into `main`. History remains intact, and a local backup ref preserves the branch before cleanup.
