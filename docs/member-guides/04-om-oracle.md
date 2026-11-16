# Om — Oracle and correctness owner — D4

Branch: `om-oracle`. Read [the shared guide](../../details.md). Own the independent sorted-array model, differential harness, invariant checks, reproducible failure collection and shrinking. Your code must establish correctness of the final sources used by the app and benchmarks.

## 1. Starting point and immediate fixes

At `fc2a10c93514b18d1cb87adfbcc0c71205afec1a`, the sorted-array implementation, replay tests, differential runner, shrinking and D4 exist. `SortedArray` lacks the current pure virtual `contains(Player)` override, making it abstract against Madhav's current header. Add an exact-key read-only membership check with the correct const signature.

Your differential runner uses `legacy_adapters.hpp` and direct legacy node fields. Keep historical results labeled with their pinned dependencies. Migrate the acceptance suite to the final factories and coordinated inspection helpers. A legacy adapter that repairs merge or bounds behavior cannot prove the production backend is correct.

## 2. Deliverables and file boundaries

| Path | Required content |
| --- | --- |
| `oracle/sorted_array.hpp` | Simple independently ordered vector model implementing all current Board methods, including contains. |
| `oracle/differential.cpp` | Actual A/B versus oracle, seeded operations, output/state checks, failure capture and reducer. |
| `oracle/replay.hpp` / runner | Independent expected command semantics and/or production replay integration as explicitly labeled. |
| `oracle/tests/` | Hand-calculated fixtures: basic, ties, errors, wide values, multiple-board seasons, regressions. |
| `oracle/build.ps1`, `run_tests.ps1`, README | Verified integrated commands, parameters, exit behavior and dependencies. |
| `oracle/VALIDATION.md` | Runs actually executed, exact hashes, counts, live population and limits. |
| `docs/derivations/D4.md`, `D4.pdf` | Authored rotation-size and select-complexity proof; preserve authorship when normalizing existing D4-om names. |
| Root TOOLS contribution | Tool assistance, copied/reused test infrastructure, human changes/review status. |

## 3. Independent sorted-array reference

Keep entries in descending score, ascending ID order. Use simple binary search/array insertion/deletion. Count higher scores independently of AVL subtree logic or Fenwick lifting. `select(k)` uses index k-1 after validation. Range copies the inclusive slice. Split partitions `score>=threshold` versus lower; merge combines arrays with global ID uniqueness and consumes donor. Audit order and ID uniqueness.

Library containers may be used in oracle/test code under the course's stated test allowance. Do not share implementation logic that could reproduce a backend's bug. Small hand-calculated expected files remain essential because app and oracle can otherwise agree on the same mistaken rule.

## 4. Two correctness layers

**Structure layer:** create the real A/B factories plus SortedArray, call equivalent Board methods, compare exact results and full ordered state. Exercise raw preconditions/helper primitives directly where replay cannot represent the case, such as conflicting IDs in independently constructed boards and supplied-pivot ordered join.

**Application layer:** run the actual `run_replay` and handwritten player table with each backend; compare stdout to independently derived expected outputs/model. Verify wrong-board checks, global uniqueness, source ID invalidation, parser behavior and unchanged state after rejection. A separate oracle Session using a library map does not itself test Madhav's custom table.

Document which layer each test covers. Do not add a standard associative container to the production app just to reuse the oracle's Session.

## 5. Differential generation and checks

1. Use a recorded seed and deterministic generator algorithm. State actual command distribution and max live IDs, including intentionally invalid commands.
2. Gather all potential score coordinates before creating Fenwick boards. Future scores and signed extrema must be represented.
3. Run identical operations on expected/A/B state. Compare success/errors and exact `(id,score)` query values, not just player counts or score buckets.
4. Check full ordered state, sizes, board sets, memberships and player lookup consistency after each command in bounded-live-set runs. Check unchanged state after each rejected command.
5. Invoke independent structure audits after each mutation in rotation/size-focused tests. Recompute sizes rather than trusting stored fields.
6. Save exact failure prefix before attempting shrinking. Record backend, step, command, seed, source hashes, expected/actual result and audit failure.

Cover ADD, SET including no-op, REMOVE, RANK, SELECT, RANGE, SIZE, SPLIT and MERGE. Include ID reuse after removal, globally duplicate IDs, unknown/consumed boards, equal-to-threshold scores, empty outputs, overlapping merge ranges and boundary numeric parsing. For multiply invalid commands, follow the architect's published precedence.

## 6. Independent audits and checker self-tests

### AVL

Recompute each subtree's node count and height from children. Check strict full-key ordering bounds, stored size, stored height and balance in {-1,0,1}. Detect duplicate membership; guard cycles/shared nodes where test inspection permits. Cover all single/double rotations, deletion cascades, key/score split and both unequal-height join directions.

### Fenwick

Check coordinate order/uniqueness, bucket ID order/uniqueness, nonnegative logical counts, total size, every Fenwick block and prefix sums against independently accumulated bucket counts. Check absent/zero-count scores and selection exactly at bucket boundaries.

### Application

Verify exactly one live table entry per player across active boards and that scores match exact board keys. Wrong-board operations cannot alter either board or lookup. Split/merge must not accidentally erase or duplicate global lookup entries.

Test that the checkers themselves detect corrupted size, height, order, balance, count and tie order using test-only fixtures/hooks. Do not add dangerous mutation methods to the public production Board contract. A checker that simply calls the same refresh method as the implementation is insufficiently independent.

## 7. Failure reduction format and guarantees

Preserve the original failing prefix and run metadata. Reduce chunks, then individual commands while retaining the same meaningful failure. Save an independently replayable minimal trace plus expected/actual output and reproduction command.

The current reducer converges to **1-minimal for its failure predicate**: no single command deletion preserves that predicate. It does not prove globally shortest. Do not call it globally minimal. A very generic signature such as `AVL: mismatch for RANK` may preserve a different rank bug; strengthen signatures with the failing invariant/result category where necessary and describe limits.

Deleting setup can invalidate later commands. Either keep necessary dependencies or let the oracle process the new errors while requiring the same target failure. Recollect Fenwick coordinates for candidates. Verify the shrunk result really replays and preserves the defect. Fault injection should demonstrate both detection and reduction, with deliberately injected failures labeled separately from actual backend defects.

Ordinary C++ exceptions can be caught; segmentation faults/timeouts require a subprocess wrapper to capture/reduce safely. State whether that is supported rather than promising crash reduction from an in-process reducer.

## 8. Scale strategy without misleading claims

| Run | Purpose and evidence |
| --- | --- |
| Small deterministic fixtures | Independently known outputs, parser/errors, ties, all rotation/ownership cases. |
| One million seeded operations with bounded live set | Full differential state/audit checking after each command. Record exact max-live and actual peak population. |
| Targeted adversarial mutation runs | Full-tree size audit after every operation as required for the named rotation targets. Record population/run length. |
| One million simultaneous players | Bulk construction and rank/select/season checks with full phase-boundary audits and declared checkpoints. |
| One million actual score revisions plus queries | Final application/performance workload; use feasible checkpoint/state comparisons outside timing and state what was independently checked. |

A sorted-vector update and full scan at every step of a million-player/million-revision run can be quadratic. Do not silently skip validation or claim bounded-live tests establish full-scale update correctness. Combine the evidence above and identify each limit. A checksum is useful for cross-backend regression, but is not independent proof of every answer.

Historical harness defaults can print seed/max-live values unrelated to an external trace's actual generation settings. Record generator metadata and measured counts separately rather than copying that banner into final evidence.

## 9. D4 proof outline

For a single rotation, draw the before/after trees with unchanged subtrees A,B,C. Write exact subtree cardinalities and show demoted-node refresh followed by promoted-node refresh reconstructs correct sizes and preserves total membership. Mirror for the opposite rotation. Explain double rotations as composition and show why both metadata refresh steps matter. Induct over ancestors after insertion/deletion/join.

Then prove select's descent invariant: k is the desired one-based position in the current subtree. Use L=size(left) to justify each branch and the right residual k-(L+1). Each step descends one level; AVL height is O(log n), hence select is O(log n). Correct sizes are the prerequisite, not an assumption omitted from the proof. Include base/empty/invalid input cases and mapping to Prince's functions.

## 10. Handoff and completion

Send minimal defect reports to the owning member, regressions to Madhav, case feedback to Bianca and validated source hashes to Sumit. Proposed reviewer: Madhav.

- [ ] Current header compiles; contains exists; final factories replace legacy-only validation.
- [ ] Both structure and actual application layers have independent expected checks.
- [ ] All invariants/checker corruption tests and shrinking demonstrations pass.
- [ ] Million-operation evidence records seed, count, peak live population and exact sources.
- [ ] Full-scale evidence has explicit validation boundaries.
- [ ] Failure artifacts are preserved and passing fixes include regressions.
- [ ] D4 authored PDF, module README, reviewed PR, tools entry and presentation segment are ready.
