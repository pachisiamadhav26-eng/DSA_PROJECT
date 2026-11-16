# Bianca — Adversarial input owner — D5

Branch: `Bianca_adversary`. Read [the shared guide](../../details.md). Your job is to construct difficult, reproducible operation sequences, explain the effect they should provoke, and show what the actual final implementations do.

## 1. Scope and starting point

At `6d7e85ac0f791a98f82ec15eacc2faf27795dc18`, `adversary/generator.cpp`, `run.py`, fixtures, validation files and a root D5 draft exist. Existing cases cover rotations, ascending/descending insertion, ties, churn, extremes, seasons, unequal joins, errors, mixed and scale.

The existing runner explicitly uses pinned legacy sources/adapters. Preserve historical evidence as historical, then rerun required cases against the production application and Om's final checker. Generating a million-revision trace is different from executing and validating it.

You own generator correctness and the explanation of the stress target. Prince owns AVL fixes; Saksham owns Fenwick fixes; Om owns the differential checker/reducer; Sumit owns formal timing. Reuse their work with attribution instead of independently changing their implementations.

## 2. Deliverables

| Path | Required content |
| --- | --- |
| `adversary/generator.cpp` | Standalone reproducible C++17 trace generator, stdout commands only. |
| `adversary/run.py` | Compile/generate/repeat/validate workflow, exact dependencies and nonzero failure exits. |
| `adversary/tests/*.trace`, `*.expected` | Small independently checkable fixtures, including ties and full numeric widths. |
| `adversary/README.md` | Every case, parameters, command-count semantics, targeted bug/cost and verified commands. |
| `adversary/validation/` | Real command records, exact hashes, seeds, counts, results and stated limits. |
| `docs/derivations/D5.md`, `D5.pdf` | Authored split proof and rotation-forcing construction; normalize root D5 location during integration. |
| TOOLS contribution | Generator/document assistance and human changes; consolidate nested disclosure into root without losing attribution. |

## 3. Generator boundary and output rules

Generate the exact replay syntax from the architect's interface. Board 0 starts empty. Use fresh output board IDs and globally unique active player IDs in valid cases. Track consumed board IDs so later invalid references are deliberate and labeled. Include invalid cases in separate named families with known expected errors.

Accept explicit seed, case, player count and operation/revision count. Document allowed ranges and the exact meaning of every count. Current mixed `--ops` counts commands, whereas scale `--ops` counts SET revisions after ADD setup and before appended observation commands; preserve/document this distinction or change all consumers together.

Write only commands to stdout; settings/diagnostics go to stderr or JSON. Do not insert `# comments` into traces. Save settings including generator commit/hash, RNG algorithm, seed, case, parameters, actual output command count, successful-revision count and trace SHA-256. A seed alone does not reproduce a trace if generator code changes.

For deterministic output, avoid unordered-container iteration and unspecified random-distribution behavior. The current mt19937_64 plus explicit modulo choices is reproducible but has modulo bias; do not describe it as perfectly uniform. Generate twice with identical settings and compare file hashes. Check output write errors and invalid CLI options.

## 4. Required case families and observed evidence

| Case | Construction | Check / observed effect |
| --- | --- | --- |
| Four rotation shapes | Minimal LL/RR/LR/RL-triggering insertions under descending-score comparator | Count primitive rotations and recompute full-tree sizes/heights/order after each mutation. |
| Sustained rotation insertion | Increasing scores and mirrored decreasing scores at growing n | Show constant-fraction insertion rotations, actual rotation counts, valid final AVL. |
| Delete/reinsert stress | Delete roots/two-child nodes, alternate extremes and reinsert; repeat without growing live population indefinitely | Deletion rebalance metadata, stale lookup, actual rotation events; audits after every target mutation. |
| Extreme-height ordered join | A large valid AVL versus one-node/empty tree, in both orientations, with valid separating pivot | Record actual heights and join visits, full output union/order/size, emptied sources. |
| Tie stress | Equal scores, descending IDs, moves into/out of tie and bucket-front insert/delete | All rank 1 in a single tie; increasing-ID select; B shift counts rather than a false claim of slow prefix sums. |
| Threshold boundaries | Threshold equals live score, lies between scores, is signed min/max, produces empty side | Equality belongs high; no overflow; disjoint union and source invalidation. |
| General league merge | Split, revise across threshold to create overlap, then merge; equal scores in both inputs | Arbitrary union correct; do not apply ordered-join complexity to overlap. |
| Churn and emptiness | Remove/re-add IDs, unchanged SET, drain board, split empty, rejoin | Hash/tombstone consistency, no stale membership, valid empty outputs. |
| Numeric/error boundary | ID 0/UINT64_MAX, signed extrema, overflow tokens, bad argument counts, wrong board, bad positions/ranges | Exact error codes/precedence and no state changes. |
| Seeded mixed | Bounded-live random interleavings with declared invalid fraction | Om compares output/full state and shrinks failures. |
| Full scale | At least 10^6 ADDs and 10^6 genuine SET revisions, observations and season operations | Count actual players/revisions/queries; distinguish generation from execution and independent checking. |

Three-element examples demonstrate specific rotations but not sustained worst-case behavior. A case named unequal is insufficient unless the actual height gap is recorded. Correct AVLs stay balanced under monotone insertion; the intended effect is rotations and metadata stress.

## 5. Required per-case evidence format

For each case, publish a compact record with these fields:

```text
Case and owner:
Targeted requirement/bug/cost:
Construction and why it triggers the target:
Generator commit, seed, exact command and parameters:
Trace path/hash, total commands, actual peak live population:
Expected small-case output / independent oracle:
Backend/application/checker commits and build flags:
Audit frequency and counters measured:
Observed result, exit code and evidence paths:
Failure reproducer or limitations:
```

For required rotation targets, full-tree size audit runs after every operation in the declared targeted test, not merely at the end. These scans are correctness work and must not be included in Sumit's tree-operation latency. If full per-operation audits at a huge population are impractical, keep the targeted audited suite at a declared feasible size and run separate scale checks with explicitly different audit frequency; never describe checkpoint-only validation as per-operation auditing.

## 6. D5: logarithmic split proof

State the exact `split_by_key` or score-split algorithm implemented by Prince, its order/threshold boundary and the AVL join lemma from D1. A split follows one root-to-leaf path and joins retained sibling subtrees while unwinding.

Do not argue that O(log n) joins each cost O(log n) and then assert O(log n). Define the sequence of intermediate heights and charge each join's spine descent to height increases/decreases that telescope along the split path. Show the summed height-gap terms are O(h), and constant work per split level is also O(h). With AVL h=O(log n), total is O(log n). Include the required intermediate-height facts and both split output accumulations; reference a valid theorem if using one rather than skipping its assumptions.

Prove partition/order, exact node-set preservation, balance and sizes using join's guarantee. Show equality goes to the agreed side and source ownership transfers safely. Include empty/all-one-side and tied-score examples. Diagram the path and retained subtrees so the telescoping charge is inspectable.

## 7. D5: rotation-forcing construction

Give an explicit deterministic family, such as repeated rightmost insertion in total-key order (equivalently choose the appropriate monotone score direction for this descending comparator). Prove rotations occur on a constant fraction of insertions; a graph or intuition alone is insufficient.

One useful counting approach for the standard AVL insertion algorithm is to track the rightmost path after monotone insertions: a no-rotation insertion lengthens it by one, while a single rotation on that path cancels that growth. Relate final path length to O(log n) and derive a linear lower bound on rotation-causing insertions, checking that the argument matches Prince's actual balancing algorithm. Write the argument fully and verify small n; do not copy a bound for an unrelated insertion policy.

For deletion sequences, define the objective/counter and compare legal candidate sequences if claiming they force many rotations. The phrase “as many as possible” in the assignment calls for a deliberate construction and evidence; do not claim global optimality without a proof. Report actual single/double rotations and total operations.

## 8. Run/handoff workflow

1. Build the generator and reject invalid settings correctly.
2. Generate small fixtures twice; verify deterministic hashes and manual expected answers.
3. Run targeted traces through Om's final checker and actual application with both backends.
4. Collect per-operation AVL audits and rotation/join counters through agreed hooks; record actual effects.
5. On failure, send Om the full prefix for shrinking and send the responsible owner the reduced reproduction. Keep seed/hash/source identity intact.
6. Supply Sumit valid, deterministic benchmark inputs and expected membership outside timing. State setup/revision/query counts separately.
7. Repeat relevant integration checks after owners fix defects; preserve old evidence as superseded rather than editing it into a new PASS.

Current generator commands are documented in `adversary/README.md`; use its actual CLI, including `--case`, `--players`, `--ops` and `--seed`. Existing `run.py` results establish only the dependencies it records until migrated to final sources.

## 9. Completion checklist

- [ ] Every course adversarial target has an explicit construction and observed evidence.
- [ ] Required full-tree size audit runs after every targeted operation.
- [ ] Both join orientations use recorded actual heights and valid ordered keys.
- [ ] Traces exactly match canonical replay; expected outputs are independently checked.
- [ ] Seeds, versions, hashes and count meanings reproduce every case.
- [ ] Final production checks are distinguished from legacy-adapter checks and generation-only scale evidence.
- [ ] D5 authored source/PDF includes the telescoping split proof and rotation-fraction proof.
- [ ] README, tools disclosure, reviewed PR and adversarial module/derivation slide are ready.

Proposed reviewer: Sumit. Om validates correctness; Prince confirms the claimed rotation/join effects; Saksham confirms tie-vector stress interpretation.
