# Analyst work log

Owner: Sumit, Member 6.

## 10-16 November 2026 - Initial development

- Read the project requirements and planned rank, select, split and join measurements.
- Built small 100-player prototypes and checked expected answers. The early work is recorded in commits `cfd8ccf` and `d2893a1`. Those prototypes and saved outputs have now been removed from the branch.

## 17 November 2026 - Shared-interface harness

- Added the harness, dependency-recorded build, five-run suites, raw CSV output, statistics, charts and profiling controller.
- Ordinary workloads are generated inside the harness. Adversary traces use Madhav's replay format and actual application code.
- Earlier development checks covered expected query answers, split/join results, output preservation, statistics and the profiling process handshake. WPR accepted the recording profile XML. Hardware recording and WPA interpretation remain pending.

## 17 November 2026 - Component compatibility

- Verified Architect `a4dc438`, AVL `03f3f57`, Fenwick `0d5ad68` and Oracle `6a25781` together. The integrated development quick suite passed.
- Om's current correctness suite passed against those versions, including a bounded million-command test and a separate million-player/million-query scale check. These are distinct from the Analyst's formal performance workload.
- Checked the supplied Adversary fixtures and saved small traces against the current components. The downloaded package's own runner and validation record need separate updates; verified input files can be supplied to this harness.

## 17 November 2026 - Commit preparation and cleanup

- Simplified the main command and replaced test-only backend copies with checks using actual components.
- Updated D6 for the current merge implementations and rendered its three-page PDF. C++ compilation/linking, Python syntax and PDF layout checks passed. The revised pipeline is deferred at Sumit's request.
- Removed the practice programs/results, personal team plan, six member guides and redundant handoff/validation notes. Kept the build instructions, derivation, report, tool disclosure and this requested activity log.

## 17 November 2026 - Full measurements and hardware recordings

- Verified Bianca's updated generators at `a788f17` with Architect `a4dc438`, AVL `03f3f57`, Fenwick `2d758ef` and Oracle `f3f8c29`. Used the generators and traces without replacing the other owners' modules with Bianca's integration copies.
- Compiled the exact C++17 input generator using a portable Zig/Clang compiler because the installed GCC 6.3 cannot compile its structured bindings. Both measured structures and the oracle still use the same GCC build.
- Checked small adversarial traces against the actual oracle. Ran the formal full suite with 17 traces, up to one million players and one million actual score revisions, five repetitions per experiment, both score policies and operation counters.
- Saved 158 raw CSV files, 436 separately summarized groups and 29 PNG/SVG chart pairs in `results/full-20261117-final/`. All backend checks passed. Independently calculated the million-scale expected output from Bianca's reference function and verified every repetition on both backends.
- Found and fixed summary grouping that combined different traces with equal sizes/operations. Added a regression check and regenerated summaries and charts from the unchanged raw timings. Saved the corrected postprocessor hash in `postchecks.json`.
- With Sumit's approved Administrator prompt, recorded rank/select LLCMisses samples for both backends. Windows tracerpt reported zero lost events for all four recordings. Saved original ETLs, metadata, decoded benchmark-thread samples and analyses. The local WMI schema confirms the payload layout. These single captures are diagnostic samples, not exact miss counts or five-run timings.
- Filled `REPORT.md` with actual measurements and their limitations, updated run instructions and recorded tool use. No commit or push was performed during this run.

## 21 November 2026 - PR submission corrections

- Added author Sumit, roll number 250041038, the exact assigned D6 statement and explicit source-file/function references. Condensed D6 to a two-page PDF named `D6_250041038.pdf`; retained the original pre-run prediction commit in its provenance.
- Added dated representative AI prompts and disclosed this revision in `TOOLS.md`.
- Added exact figure-to-raw-CSV mappings and a hash-verified export to `results/raw/`, `results/figures/` and `results/summary.csv`. Added source captions to the Analyst report. The original formal measurements and profiling evidence remain in the recorded run directory.
- The review-preparation checks are separate from a new performance run. No calibrated numerical cost model or repeated full-scale timing run is claimed.
- Teammate approval, the full team report/PDF and `scripts/run_all.sh` remain team integration tasks. GitHub PR #5 had no submitted reviews when checked on 21 November.
- At Sumit's request, `TOOLS.md` was subsequently shortened to a tools/parts/changes table. It omits dates and prompt examples, so the reviewer's dated-prompt request remains outstanding.

## Remaining work

Human review of code, D6 and results; a teammate-reviewed PR; and verification of the final integrated command from a fresh clone.
