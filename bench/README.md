# Analyst benchmarks

Owner: Sumit, Member 6. This code measures the real AVL and Fenwick implementations through Madhav's `Board` interface. Om's oracle checks small runs. The completed formal suite is in `results/full-20261117-final/`; see `REPORT.md` for measured results.

## Files

| File | Purpose |
| --- | --- |
| `runner.cpp` | Benchmark harness for rank, select, split, rejoin, ordered join and counters |
| `support.hpp` | Timer, common checks, options and CSV writer |
| `build.py` | Compile unchanged teammate sources and record their exact versions |
| `run.py` | Run five repetitions, compare backend results and save metadata |
| `run_all.py` | One entry point for build, checks, experiments and charts |
| `profile_windows.py` | Record cache-event samples around a prepared query phase |
| `analyze_profiles.py` | Filter Windows trace exports to benchmark threads and summarize samples |
| `tests/` | Small real-backend checks and summary-statistics checks |
| `WORK_LOG.md` | Activity history |
| `../results/plot.py` | Generate the summary CSV and PNG/SVG charts |
| `../docs/derivations/D6.md`, `D6.pdf` | Editable and submitted cost predictions |

## Inputs

The runner generates ordinary initial players and updates from a recorded seed. Both structures receive the same workload. There is one ID per player, and scores lie in 0..U-1. The supplied universe can include currently unused scores.

The Adversary supplies difficult replay inputs such as rotation-heavy updates and unequal-height joins. Pass saved traces with `--trace`. The runner reuses the Architect's real replay engine, parser and handwritten player table. Om supplies correctness checks and reference answers.

## Build and run later

Use PowerShell from the repository root. Python 3, Git and a C++17 compiler are required. Python dependencies stay in the ignored build folder.

```powershell
# Download the latest teammate branch snapshots.
git fetch origin

# Install chart and PDF libraries locally for this project.
python -m pip install --target build/bench/python -r bench/requirements.txt

# Build both backends and oracle, check the harness, then run the small suite.
python bench/run_all.py --suite quick

# Run the full experiment suite after reviewing D6 and the quick results.
python bench/run_all.py --suite full
```

The default build exports the current local snapshots of `origin/madhav`, `origin/prince-chavda`, `origin/Saksham` and `origin/om-oracle`. It records their resolved commits and source/binary hashes. It never fetches, switches branches or changes teammate sources. Build failures remove stale executable records.

Both structures implement `Board` and their factories in `include/leaderboard.hpp`. The harness also uses the team's `include/backend_tools.hpp` for audits, AVL height/rotation/join statistics, ordered join and Fenwick bucket-shift counts. The oracle supplies `oracle/sorted_array.hpp`. These versions compiled together during preparation.

After the team integrates its modules, add `--local` to build this checkout. Use `--compiler PATH` to select another compiler. Advanced source/ref overrides are available through `python bench/build.py --help`.

```powershell
# Add a supplied small adversarial trace and compare its outputs with the oracle.
python bench/run_all.py --suite quick --trace path/to/rotations.trace --check-trace
```

Repeat `--trace` for additional inputs. Use whole-trace oracle checking for small cases. Large traces can be replayed without `--check-trace`; preserve the generator command and actual seed alongside the trace hash in your experiment notes.

## What is measured

Quick runs use 100 and 1,000 players with 1,000 actual revisions. Full runs use 1,000, 10,000, 100,000 and 1,000,000 players, with one million actual revisions per query experiment. All timings have five repetitions.

Ordinary query workloads repeat blocks of up to 256 revisions, then that many rank and select queries. This is a documented experimental choice. Two score policies compare U=n with U=min(n,1000); the quick tie-heavy cases use U=10. Each backend/run starts fresh with the same seed.

Rank and select timers include the query loops and checksums. Setup, random generation, updates, player-score lookup, printing and checks happen outside these timers. The result is batch mean time per core query, not individual-query tail latency. No separate cold-cache experiment is claimed.

Split uses several score cutoffs; ties go to the high side. Public rejoin uses `merge_from`, including all validation/pivot/rebuild work it performs. A separate AVL experiment supplies the pivot in advance and records actual input heights and join visits. Destructive inputs are prepared before timing. Batch memory is bounded; million-player trees use one copy per repetition.

Small query runs check all answers and snapshots, including the real oracle. Large runs check final snapshots and sampled final queries and compare complete backend checksums. Available structural audits run outside timing. These checks complement Om's differential suite.

Supplied trace measurements preserve command order and time individual backend calls. They include per-call clock overhead and are labelled separately from batched query measurements. The output digest includes expected application errors.

Counter experiments record construction/revision rotations, bucket shifts and ordered-join visits. Counter rows end in `_count`; their totals are in the CSV checksum column, their timing is zero, and summary unit is `count`. Generic Board calls cannot expose query-node visits. Any always-enabled owner counters also affect timed builds.

## Saved outputs

Each suite creates a new `results/quick-TIMESTAMP/` or `results/full-TIMESTAMP/` folder containing:

```text
metadata.json     settings, seeds, machine, code versions, completion status
run.log           commands and their outputs/errors
raw/*.csv         every measured repetition
summary.csv       mean, sample standard deviation, minimum, maximum, run count
figures/          PNG and SVG charts with min/max error bars
```

Existing output folders are rejected. Only metadata marked `complete` represents a finished suite; a failure can leave partial CSVs. Review the machine/cache information and supply verified details through `--machine-json FILE` if needed. Keep formal measurements on the stated machine.

```powershell
# Rebuild summaries and charts from saved raw CSVs.
python results/plot.py results/your-run-folder

# Recreate the D6 PDF after editing its Markdown source.
python docs/derivations/render_d6.py
```

## Cache profiling

Use Administrator PowerShell and Windows Performance Analyzer (WPA). The controller samples the exposed `LLCMisses` event with interval 65,536. It starts collection after setup and stops before teardown using ready/go/done/release signals.

```powershell
# List hardware profiling events exposed by Windows.
wpr -pmcsources

# Capture an AVL rank phase after the normal benchmark build.
python bench/profile_windows.py --backend avl --operation rank --players 1000000 --scores 1000000

# Capture the matching Fenwick phase.
python bench/profile_windows.py --backend fenwick --operation rank --players 1000000 --scores 1000000
```

Save the ETL, WPRP, metadata, logs and analysis. Filter to the benchmark PID/query phase and check lost events and adequate samples. Samples are not exact miss totals. Four formal diagnostic recordings are saved under `results/full-20261117-final/profiles/`, with filtered CSVs and JSON analyses. Each reported zero lost events. Collection failure is recorded, and the script does not cancel another WPR session. See [Microsoft's PMU guide](https://learn.microsoft.com/en-us/windows-hardware/test/wpt/recording-pmu-events).

WPA is optional for the saved sample summary. Windows `tracerpt` exports can be analyzed as follows, using `avl-rank`, `avl-select`, `fenwick-rank` and `fenwick-select` as NAME:

```powershell
# Decode one recording and save its lost-event summary; replace NAME for each capture.
tracerpt results/full-20261117-final/profiles/NAME/hardware.etl -o build/bench/NAME-events.csv -of CSV -summary results/full-20261117-final/profiles/NAME/event-summary.txt -y

# Filter all four decoded exports to the recorded benchmark threads.
python bench/analyze_profiles.py results/full-20261117-final build/bench
```

The analyzer uses the installed Windows PMU payload schema saved in `profiles/event-schema.json`. It maps thread lifecycle events to the recorded PID because PMU event header IDs can be unset. It counts LLCMisses source 29, as verified in each recording's saved event list. Counts refer to decoded samples; tracerpt's aggregate count can differ slightly. No exact miss rate or function attribution is claimed. Each diagnostic capture is separate from the five-run timing experiments.

## Submission

D6 predictions, full measurements, figures and hardware sample summaries are saved. Human review, a teammate-reviewed PR and verification of the final integrated command from a fresh clone remain. The Architect should include the benchmark invocation in the team's final all-in-one command. Preparation history is recorded in `WORK_LOG.md`.
