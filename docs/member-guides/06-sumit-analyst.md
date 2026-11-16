# Sumit — Analyst and benchmark owner — D6

Branch: `sumit`. Read [the shared guide](../../details.md). Own the reproducible benchmark harness, operation counters integration, cache profiling, raw measurements, figures and predicted-versus-measured analysis. D6 must be written before formal measurements.

## 1. Current state and immediate dependencies

The working checkout contains local benchmark changes: `bench/build.py`, `run.py`, `runner.cpp`, `support.hpp`, `workload.cpp`, `profile_windows.py`, tests, practice material, `results/plot.py` and `docs/derivations/D6.md`. They are not all committed in the inspected remote sumit snapshot. Preserve existing work and review it incrementally.

Formal combined runs depend on Prince's canonical namespace/factory, Om's contains implementation and final adapter migration, and agreed helper definitions. Current `bench/build.py` checks header identity and links factories, which is useful, but successful test-reference builds are not measurements of AVL/Fenwick. Never publish practice/reference numbers as final structure results.

## 2. Deliverables

| Path | Required content |
| --- | --- |
| `bench/workload.cpp` | Deterministic ordinary revision/query replay generator with explicit seed/U/counts. |
| `bench/runner.cpp`, `support.hpp` | Actual backend calls, timing boundaries, consumed outputs, validation and raw rows. |
| `bench/build.py`, `run.py`, `run_all.py` | Reproducible builds, source identities, suite execution, failures and metadata. Existing run_all is benchmark-specific, not yet the whole-project acceptance workflow. |
| `bench/tests/` | Meaningful checks for workload counts/validity, runner provenance and result schema; test-reference clearly labeled. |
| `bench/profile_windows.py` or documented alternative | Separate real profiler acquisition with available tool/event limitations. |
| `bench/README.md`, work log | Verified commands, definitions, suite scope, pending/complete status and limits. |
| `results/` | Raw per-run CSV, metadata, summaries, profiler evidence, figures and plotting script. |
| `docs/derivations/D6.md`, `D6.pdf` | Authored premeasurement models and later comparison clearly separated. |
| `REPORT.md` measurement sections | Prediction, measured results, spread, explanation and evidence links; coordinate final report assembly. |

## 3. D6 model before measurement

Define n active players, U actual score coordinates, t bucket occupancy, r output count, h1/h2 input heights, and N combined merge population. State the measured operation boundary before choosing a model.

| Experiment | Model terms to write before timing |
| --- | --- |
| AVL rank | Hash lookup if included, comparator/size-read work along O(log n) path; distinguish rank count kernel from application rank. |
| AVL select | O(log n) size-guided descent; no lookup needed. |
| Fenwick rank | Coordinate binary search plus O(log U) prefix traversal; application hash cost if included. |
| Fenwick select | O(log U) lifting plus O(1) indexed bucket access; do not charge O(t) to indexed selection. |
| Revision | A erase+insert O(log n); B O(log U+t_old+t_new), first allocation separate and unchanged SET no-op separately. |
| AVL split | O(log n) telescoping joins, excluding external board registry/hash/validation costs unless deliberately measuring end to end. |
| AVL supplied-pivot join | O(|h1-h2|+1) core; separate boundary searches, pivot extraction/allocation and wrapper behavior. |
| Fenwick split | O(U) bucket/index preparation and rebuild under current move-based design. |
| General merge | Actual coordinate/bucket/node work plus duplicate validation. Sorting IDs may add O(N log N). Not the ordered-join bound. |

Use models such as fixed overhead plus coefficients times log2(n), log2(U), height gap or rebuild work, but do not imply complexity alone predicts exact nanoseconds. Commit the prediction/model version before collecting formal results. Record any later fitted coefficients as measured analysis, not prior prediction.

Include allocation, vector movement, memory layout/cache hypotheses, branch behavior and timer overhead as possible effects. Cache hypotheses remain hypotheses until profiling supports them. D6 must include cost derivations, not only a table of asymptotic labels.

## 4. Required experiment plan

### 4.1 Rank and select as n grows

Use a declared size series, for example 10^3, 10^4, 10^5 and 10^6; these intermediate sizes are proposed, while the million scale is required. Use identical initial populations, revisions and query streams for both backends. Record actual U and tie distribution, not just a configured maximum score.

Separate rank and select batches. The ordinary generator currently emits one genuine SET followed by one RANK and one SELECT per update. At full scale this means 10^6 setup ADDs plus 10^6 SETs, 10^6 RANKs and 10^6 SELECTs: four million commands, not one million total. Record each category. Avoid naming a SET a revision if it leaves the score unchanged.

Run both a fixed/small-U tie-heavy configuration and a larger-U configuration where feasible, since n and U are different variables. Declare how queries/players are sampled. Do not compare different traces or state histories between A and B.

### 4.2 Split versus rebuild

Prepare equivalent boards outside timing. Use central, skewed and empty-side thresholds, with ties at the cutoff. Measure backend split separately from application parsing/output. Verify high/low membership, ordering, counts, consumed source and equality rule afterwards. Restore/rebuild a fresh original board outside timing before each repeat.

### 4.3 Ordered join and full league merge

Measure supplied-pivot AVL join over a range of actual height gaps in both orientations. The pivot must be absent from the inputs and separate all keys. Use a helper that omits boundary searches for the core experiment, or explicitly include/label their cost. Record heights, visits and rotations.

For the practical split/rejoin comparison, include AVL pivot extraction/preparation if the public operation requires it and compare to B's actual rebuilding implementation on the same players. Keep an additional overlapping-range league-merge experiment separately labeled. The general merge does not inherit the supplied-pivot bound. Time conflict validation if the measured public method actually executes it.

### 4.4 Adversarial/counter experiments

Use Bianca's reproducible cases. Record rotations, join visits, Fenwick bucket shifts and any agreed path-step counters, with reset semantics. Keep counters/profiling in separately labeled runs if they perturb timing. Analyze whether input patterns explain time changes; do not equate a rotation-heavy sequence with an unbalanced resulting AVL.

### 4.5 Cache profiling

Use an actually available profiler and record tool version, executable hash, command, events, permissions and sample/counter limitations. Preserve raw output. Compare matching workloads and setup boundaries on one documented machine. If hardware events are unavailable, state that limitation and omit unsupported cache conclusions; elapsed-time differences alone do not establish cache misses.

## 5. Timing protocol

1. Build optimized real backends with recorded compiler/version/flags and exact source hashes. Reject stale executable or changed source manifests.
2. Validate representative small workloads with Om's oracle and actual production app before measuring. Use final snapshots/checkpoints and cross-backend checksums at full scale, reporting that scope honestly.
3. Prepare coordinates, traces, structures and input arrays outside kernel timings. Document warm-up, allocation and reset policy.
4. Use a sufficiently fine monotonic clock; the local Windows helper uses QueryPerformanceCounter. Time batches large enough to exceed clock resolution, and preserve `calls`.
5. Consume results through a checksum or observable sink. State whether the checksum/loop overhead is included; do not subtract guessed overhead.
6. Repeat each configuration at least five times with fresh initial state. Record every run, order and seed. Distinguish repeated timings of the same seeded trace from a separate multi-seed workload study.
7. Alternate/randomize backend run order where practical to reduce thermal/order bias; document the actual order. Keep machine/power/background-load policy consistent.
8. Validate outside the timed section and fail the run on mismatched results. Never emit a passing final dataset from a failed or partially completed suite.

Oracle work, full audits, parsing, stdout printing and coordinate preprocessing are excluded from kernel latency. End-to-end application replay can be measured separately with those costs included and clearly named.

## 6. Raw result and metadata format

Preserve the existing local CSV contract unless coordinated:

```csv
backend,experiment,operation,players,coordinates,seed,run,calls,elapsed_ns,ns_per_call,checksum,validation,left_height,right_height
```

One row represents one run/batch/operation/configuration, not an average hiding repetitions. Units are nanoseconds. `calls>0`; elapsed time is nonnegative; means must come from appropriate comparable rows. Height fields use -1 when inapplicable under the current runner. Record U even when the AVL does not use coordinates, so paired workloads remain identifiable.

Companion metadata must contain source/dependency commits, dirty-tree/source hashes where relevant, compiler command/version, binary hash, CPU/cache/RAM/OS, experiment options, seeds, actual counts, trace hashes, timestamp, validation method, timing boundaries, profiler settings and run status. Sidecars already produced by local scripts should be retained, not replaced with an incompatible invented schema.

Recommended result grouping: `results/<suite-and-run-id>/raw/`, metadata JSON, summary CSV, figures and profiler subfolder. Keep each invocation distinct; never overwrite raw evidence. Generated executables and giant regenerable traces belong under ignored build directories unless deliberately archived; preserve seeds/hashes/commands to reproduce them.

## 7. Commands and readiness boundaries

The current local scripts support these invocations; they require the corresponding files and compatible teammate commits and are not proof that a full run has passed:

```powershell
python bench/build.py --help
python bench/build.py --local --avl-tools --fenwick-tools
python bench/run.py --suite quick --backend both
python bench/run.py --suite full --backend both
```

`--local` means an integrated source tree; branch-snapshot builds use the script's documented refs instead. The full runner requires both backends, oracle and A/B tools. Do not work around readiness checks with `--without-oracle` or a test-reference backend and call the result final. Development smoke runs may omit parts if clearly labeled.

After all required checks pass, give Madhav these verified commands for the root full-run orchestrator. Inspect `results/plot.py --help` and profiler documentation for their actual supported CLI before writing final README examples; do not invent flags.

## 8. Analysis, figures and report format

Summarize each configuration with mean and a named spread, such as sample standard deviation and min/max, from at least five runs. Label axes/units, n, U, workload family, repetitions and validation scope. Preserve raw rows underlying every point and generate figures from scripts, not hand-edited numbers.

Required figures/analysis cover rank latency versus n, select latency versus n, split/rejoin comparison and height-gap join behavior. Add tie-shift/update analysis and profiler-based cache discussion where evidence supports them. Overlay/compare a prior model only with units and calibration method explained; distinguish fitted observations from a priori predictions.

For each gap between model and measurement, discuss concrete implementation work: small-n fixed overhead, AVL boundary searches, B initialization, ID shifts, duplicate validation, allocator behavior and supported profiler evidence. Do not retrofit the predicted model silently or invent a complexity improvement based on a short flat plot.

## 9. Handoff and completion

Ask Prince/Saksham for exact costs and stable stats; Om for validated commits and check scope; Bianca for deterministic difficult traces; Madhav for production integration and final orchestrator. Give every figure/data path and reproduction command to the team. Proposed reviewer: Bianca.

- [ ] D6 prediction source committed before formal timings; authored PDF prepared afterwards with history preserved.
- [ ] Real backends, exact canonical interface and oracle/hooks build together.
- [ ] Million players and million genuine revisions plus queries executed with declared validation.
- [ ] At least five raw repetitions per reported timing, spread/seeds/machine/provenance preserved.
- [ ] Ordered join, practical rejoin and arbitrary merge are separately named and costed.
- [ ] Required counters and cache profiling supplied, or unsupported cache claims explicitly omitted.
- [ ] Figures regenerate from raw CSV; report explains prediction gaps and limitations.
- [ ] Full workflow joins Madhav's clean-clone command; reviewed PR, tool disclosure and module/derivation slide are ready.
