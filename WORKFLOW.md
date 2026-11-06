# Project workflow: Rank Queries on a Live Leaderboard

This is the team's working plan for CS 253 / MA 253 Project 2. The course brief remains the authority for assessed requirements. Record the six members' names and fixed roles in `README.md` at the first checkpoint.

## 1. Outcome and non-negotiable requirements

Build a leaderboard replay with one million players and at least one million operations. Implement and compare:

- **A:** an AVL tree keyed by `(score, player_id)`, augmented with subtree sizes, including insertion, deletion, rank, select, rank-range extraction, ordered join, and split.
- **B:** a Fenwick tree over coordinate-compressed scores, including prefix-count rank and binary-lifting select; split and join use rebuilding.
- A custom hash table mapping player ID to current score, a sorted-array oracle, differential tests, invariant checkers, adversarial generators, and a benchmark harness.

A, B, and the oracle must use the same programming language. A library map, set, priority queue, or equivalent must not replace a structure under study; such containers are allowed in tests and data generation. The final submission is the commit tagged `final`.

## 2. Team ownership and pairings

Pairs discuss designs and review work. Each of the six people owns a separate module, derivation, branch, and sequence of commits.

| Pair | Role | Owned files | Proof | Work that starts without another implementation |
| --- | --- | --- | --- | --- |
| AVL and adversary | Structure A owner | `src/structA/` | D1: AVL ordered join, sizes, and height-difference cost | Implement AVL primitives and local rotation tests against the agreed contract. |
| AVL and adversary | Adversary | `adversary/` | D5: split cost and rotation-forcing order | Generate deterministic operation traces and state their expected properties. |
| Fenwick and analysis | Structure B owner | `src/structB/` | D2: binary-lifting select | Implement compression, count updates, prefix sums, and select on small arrays. |
| Fenwick and analysis | Analyst | `bench/`, `results/`, measurement sections of `REPORT.md` | D6: predicted costs before measurement | Build the benchmark driver around a mock adapter; write the cost model first. |
| Interface and correctness | Architect | `INTERFACE.md`, `app/`, shared hash table | D3: competition ranking and agreement of A and B | Publish the contract and build the replay parser/hash table. |
| Interface and correctness | Oracle owner | `oracle/` | D4: AVL size audit and select descent | Build a sorted-array reference and fixed examples from the contract. |

The Architect coordinates `README.md`; the Analyst assembles `REPORT.md` from each owner's findings. Every member supplies their own entry for `TOOLS.md` and a separately authored PDF in `docs/derivations/`.

## 3. One short contract meeting before structure code

Timebox the first meeting to 60-90 minutes. All six members choose the language and settle the following in writing. The Architect commits `INTERFACE.md` before A or B implements the shared interface. Meanwhile, the other owners can work on independent primitives, fixtures, generators, and cost models.

1. **Ranking:** `rank(id) = 1 + count(players with strictly higher score)`. Equal scores share a competition rank such as `1, 2, 2, 4`.
2. **List position:** use a total order for `select(position)` and `range(first,last)`. Proposed rule: score descending, then player ID ascending. Positions are one-based; range endpoints are inclusive. Rank and position are different for ties.
3. **Score revision:** define behavior for an unknown ID, an existing ID, an unchanged score, and removal. The app's custom hash table provides the old score so both structures can remove the old entry before inserting the new one.
4. **Score split:** proposed rule: high division has `score >= threshold`; low division has `score < threshold`. Define ownership of the resulting boards and empty-board behavior.
5. **Two different merges:** `join_ordered(T1, pivot, T2)` requires every key in T1 to precede the pivot and every key in T2 to follow it. A regional league merge may contain overlapping scores and IDs; it needs a separate general merge algorithm and a rule for ID collisions. Do not claim the ordered-join bound for arbitrary league merges.
6. **Fenwick ties:** a score-count Fenwick tree finds a score bucket, not a player ID. Proposed solution for the known replay: compress scores globally and maintain a second, per-score Fenwick index over the player IDs that can occur at that score. This provides a deterministic player within a tied bucket. Document preprocessing, memory, and the policy for a score/ID pair absent from the replay plan.
7. **Trace and results:** define one seeded operation format, identical query output, error values, counters, and adapter signatures for A, B, and oracle.

Write down the chosen rules and examples in `INTERFACE.md`; mark any change with a version and update all implementations and fixtures in the same integration cycle.

## 4. Parallel build sequence

| Target date | Shared checkpoint | Parallel work and acceptance gate |
| --- | --- | --- |
| **6-7 Nov** | Roles, language, interface, and sample trace fixed | Each owner opens a small PR in their own directory; cost predictions exist before timings. |
| **8-9 Nov** | Basic end-to-end replay on small inputs | A and B support updates/rank/select; oracle produces expected results; adversary and benchmark tools read the same trace. |
| **10-12 Nov** | Correctness integration | Add range, split, ordered join, and general league merge; run deterministic edge cases and seeded differential tests; repair invariant failures. |
| **13-14 Nov** | Full-scale and adversarial run | Run at least one million elements and one million operations; audit rotation-heavy cases and height-mismatched joins; record raw logs. |
| **15-16 Nov** | Demo-ready release | Finish figures, PDFs, README, REPORT, TOOLS, fresh-clone run, and presentation rehearsal. |
| **17 Nov** | Course evaluation | Demonstrate from the required release at full scale and present each owner's module and proof. |
| **By 21 Nov** | Submission deadline | Ensure the accepted `final` tag identifies the intended commit and the instructor can access the repository. |

The brief gives an evaluation date of 17 Nov and a submission date of 21 Nov, while requiring the live demo from tag `final` and ignoring commits after that tag. Ask the instructor **by 7 Nov** when `final` must be created. Until clarified, treat **16 Nov** as the internal code and demo freeze.

## 5. Testing workflow

1. **Small fixed cases:** empty board, one player, all equal scores, score changes across ties, negative/large scores, out-of-range select, split at/beyond extremes, and overlapping league scores. Check rank and position separately.
2. **Structure invariants:** A checks BST order, AVL balance, stored heights, and subtree sizes after updates, rotations, split, and join. B checks counts, total size, prefix sums, and the score/ID bucket index. A full-tree size audit follows every operation in bounded-size adversarial sequences.
3. **Differential test:** replay one million seeded operations through A, B, and the sorted-array oracle; compare every observable result and shrink a failure to a minimal operation sequence. Keep the oracle's active population manageable where its array operations are expensive.
4. **Full-scale test:** use at least one million players and one million operations. Compare A and B outputs; use sampled oracle snapshots or independent checks where a full sorted-array replay would be impractical. Record whether a case is an oracle test or a cross-structure comparison.
5. **Adversarial test:** rotation-heavy insert/delete sequences, a full size audit, maximal join height differences, large tie groups, and empty/overlapping split and merge boundaries. Save seeds and minimized failing traces.

No benchmark result counts as a correctness check. Both structures must pass the common test suite before performance claims are made.

## 6. Measurement and report workflow

- Analyst writes predicted operation-count and time models for A and B before seeing measured curves. Include AVL rank/select and ordered split/join versus Fenwick rebuild. Treat output of `r` players as at least `O(r)` work.
- Instrument rotations, comparisons, node visits, Fenwick index steps, touched buckets, rebuild elements, and allocations where practical.
- Measure rank/select latency as `n` grows and ordered split/join against B's rebuild. Report general league merge separately if its key ranges overlap.
- Run each timing at least five times on one stated machine; report mean and spread. Record processor, cache sizes, compiler/version, build flags, input seed, and raw CSV. Make cache claims only with profiler output.
- Generate figures from scripts in `results/`. Explain gaps between prediction and measurements in `REPORT.md`, including tie-bucket work and preprocessing/rebuild costs.

## 7. GitHub collaboration routine

1. Each member accepts the invitation, clones the repository, and sets their own Git name and GitHub-associated commit email **in their clone**. Check `git log -1` and GitHub attribution after the first PR.
2. Work on role branches such as `struct-a/avl-base`, `oracle/differential`, or `bench/cost-model`. Commit small, explainable changes throughout the project period.
3. Open PRs early. At least one PR from **every member** must be reviewed by another member. Pair partners are first reviewers; one cross-pair reviewer checks integration behavior.
4. Preserve the member's commit history when merging. Run the common tests before merging. Avoid simultaneous edits to another owner's directory; propose interface changes in `INTERFACE.md` first.
5. Each PR states what changed, the test command and result, the relevant invariant, and any AI/tool use to add to `TOOLS.md`.
6. Daily 15-minute sync: state merged work, failing test/trace, interface decisions, next PR, and who reviews it. A member waiting on a review continues their own proof, fixtures, documentation, or next independent task.

## 8. Required repository and presentation outputs

- `README.md`: project purpose, named fixed roles, build instructions, and **one command** that runs tests, the application, and every benchmark.
- `INTERFACE.md`: common interface, semantics, and structural invariants; written before structure implementation.
- `REPORT.md`: design choices, figures, predicted-versus-measured costs, gaps, and adversarial results.
- `TOOLS.md`: each AI/other tool used, affected parts, and what the team changed afterward.
- `src/structA/`, `src/structB/`, `app/`, `oracle/`, `adversary/`, `bench/`, `results/`: code, raw CSV, figure scripts, and seeds.
- `docs/derivations/D1.pdf` through `D6.pdf`: separate proofs, each naming its author.
- Presentation: at most 10-12 slides, 15 minutes total, interface/invariants, diagrams of both structures, each person's module and proof, adversarial behavior, and predicted versus measured cost. Rehearse the six speaking segments to fit the team limit.
- Final check: fresh clone of the intended tagged release builds and runs the full-scale demo, tests, and benchmarks; the instructor can access the repository.

## 9. Definition of done for a member

The owned module passes its relevant hidden-test-like cases and the shared differential tests; its invariant checker or evidence is included; its proof PDF is complete; its benchmark/adversarial contribution is reproducible; its tool use is declared; and it has several own commits plus at least one reviewed PR. The member can explain any line in their module and a teammate's main design choice.
