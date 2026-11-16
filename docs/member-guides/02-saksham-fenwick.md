# Saksham — Structure B owner — D2

Branch: `Saksham`. Read [the shared guide](../../details.md) first. Own a handwritten Fenwick count tree with coordinate compression and exact tied-player selection, not just a count-only demonstration.

## 1. Scope and current starting point

At `82fd786205a84472e4fe956105192f8eec87c9a3`, `src/structB/fenwick.cpp` already implements the current Board methods and `make_fenwick`. It has focused tests, a helper header and D2 source/PDF. Treat these as existing work to validate/integrate, not instructions to rewrite everything.

Review production `std::unordered_set` used for merge conflict checking. The course restricts library container use; move this validation to a handwritten helper or a documented sorted-vector method and update costs. Test-only audit sets should live in test/debug tooling rather than silently shipping as a production dependency. Coordinate helper-header adoption with Madhav and Prince.

## 2. Deliverables

| Path | Content |
| --- | --- |
| `src/structB/fenwick.cpp` | Library Board implementation, exact factory and agreed B helpers; no `main()`. |
| `src/structB/tests/fenwick_test.cpp` and further focused tests | Compression, counts, select, ties, rebuild, transfer and invalid preconditions. |
| `src/structB/README.md` | Owner, layout, coordinate policy, costs including tie maintenance, verified commands and limitations. |
| `include/backend_tools.hpp` | Coordinated declarations for `audit_fenwick` and `fenwick_stats`; shared with other owners. |
| `docs/derivations/D2.md`, `D2.pdf` | Named binary-lifting correctness and cost proof. |
| Root `TOOLS.md` contribution | Actual assisted work and subsequent human edits/review. |

## 3. Representation and invariants

Use sorted unique descending score coordinates. The factory receives all ADD/SET scores collected by the application, possibly with duplicates and arbitrary input order. Distinguish configured generator universe from actual coordinate count U. A coordinate may have zero live players but remain valid for a future update.

For each coordinate, maintain a sorted ascending vector of player IDs. For Fenwick's one-based index i, `bit[i]` equals the sum of bucket counts from `i-lowbit(i)+1` through i. Total count equals the sum of bucket lengths. Every active ID occurs once in the board; IDs are globally unique under the application contract. Counts must support at least a million players without narrowing.

An empty initial coordinate universe is valid for an empty replay. No update loop may start at Fenwick index zero. Document lazy allocation: a constructor can be cheap while first insertion pays O(U) initialization; include that in setup/update cost descriptions.

## 4. Operation requirements

### 4.1 contains, insertion and exact deletion

Locate a score by binary search in descending coordinates, then locate ID in its sorted bucket. `contains` returns false for an absent score, wrong score or missing ID without changing state. For insert/erase, locate the exact coordinate; reject unsupported scores before destructive changes. Insert an ID at its sorted place and increment Fenwick ancestors. Erase the exact ID and decrement them without unsigned underflow.

Application SET erases the old key then inserts the new one. The normal replay precollects all new scores, so these insertions must be representable. If another caller supplies an unprepared score, define rejection explicitly. Do not silently map it to a neighbor; do not change the shared interface without agreement.

Bucket shifts matter. Inserting near the beginning of a tie of size t costs O(t); vector growth can also relocate elements. Distinguish measured shift counts from all allocation/copy costs. No-op SET is handled without changing counts.

### 4.2 Rank count

Let j be the number of coordinates strictly greater than score s. Return prefix(j); the application adds one. This works for arbitrary queried s, including scores absent from the coordinate list. Equal-score players must never count as higher, and negative/extreme scores must use direct comparisons.

### 4.3 Selection and range

For valid k, binary lifting finds the smallest one-based index i with prefix(i) >= k. The player offset is `k-prefix(i-1)`, one-based inside the bucket. Return that sorted bucket's ID and score. Test zero-count buckets between occupied buckets: lifting must skip them correctly. Do not linearly scan all coordinates or tied IDs for selection.

Reject k=0 and k>total. Replace output vector contents for inclusive range. Current repeated-select range takes O(r log U), including r returned players; document this rather than claiming AVL's O(log n+r).

### 4.4 Split, merge and rebuilding

Score split returns `{high,low}`, equality in high, and empties the source. Move whole bucket vectors where possible, then rebuild counts. Both results must retain access to the full prepared universe if future SET commands can cross the division threshold. Sharing an immutable coordinate vector is safe; sharing mutable bucket ownership is not.

Use a linear Fenwick rebuild: initialize counts and add each cell to its parent, rather than n repeated player insertions. Document whether O(U) storage is allocated for each nonempty board, including many-board workloads.

Merge must support same coordinates, different coordinate universes, overlapping scores and ties with disjoint IDs. Unite descending coordinates, merge shared-score ID lists in increasing order, rebuild counts, and empty donor only after validation. Validate self/wrong-backend/conflicts under the agreed raw API policy before destructive transfer. If duplicate detection sorts N IDs, its O(N log N) cost must be included; a library hash's expected O(N) is not a valid model after replacing it with sorting.

## 5. Costs to send Sumit

| Operation | Explain this cost, including conditions |
| --- | --- |
| Score compression | O(M log M) sorting for M supplied scores, then deduplication; U actual coordinates. |
| First storage preparation | O(U) initialization when lazily allocated. |
| contains | O(log U + log(t+1)) with binary-searched ID bucket. |
| rank count / select | O(log U), interpreting empty/single-coordinate cases separately. |
| Insert/delete | O(log U+t) for sorted vector movement, plus allocation effects; first initialization separate. |
| Score revision | O(log U+t_old+t_new), unless unchanged score no-op. |
| Range | O(r log U) for current repeated-select design. |
| Split | O(U) when moving whole buckets and rebuilding, not copying all tied IDs. |
| General merge | Coordinate/bucket work plus ID movement and actual duplicate validation; state exact bound for chosen implementation. |
| Space | O(U+n) per populated board representation; mention bucket/vector overhead and capacity. |

If your code changes these costs, tell Sumit before formal timing. Rebuild speed is part of the structure's limit/comparison; do not disguise a rebuild as logarithmic join.

## 6. Audit and test plan

Expose agreed audit/statistics helpers without making mutable internals public. Independently reconstruct prefix counts from buckets, verify every Fenwick block, coordinate uniqueness/order, sorted tie IDs, membership uniqueness and total size. Define `bucket_shifts` as element movements due to ordered insertion/erasure; state that it does not automatically count reallocations or merge copies.

| Test family | Required cases |
| --- | --- |
| Coordinates | Empty, duplicate/unsorted input scores, negative/extreme values, single coordinate, non-power-of-two U. |
| Prefix and rank | No live players, all tied, absent query scores, first/last scores and gaps. |
| Select | First/last positions, every position on a small board, zero-count runs, bucket boundary positions and invalid k. |
| Ties | Decreasing-ID insertion, front/middle/back removal, ID 0/UINT64_MAX, moving into/out of huge ties. |
| Rebuild | Empty/all-high/all-low/equality split, repeated split/merge, future SET crossing threshold. |
| Merge | Same/different universes, both/one empty, overlapping ties, self/wrong-type, same ID at same/different scores with unchanged-state rejection. |
| API | Exact factory/namespace, replace-not-append range, exact contains, source/donor emptiness and lifetime. |
| Integration | Canonical replay, final oracle differential runs, Bianca's tie-heavy cases, million-scale phase checks. |

Run audit after every mutation in focused correctness tests. Keep audit work outside measured latency. Record compiler/flags/commit/seed and actual outcomes. The current focused test file is a useful start but does not alone establish all cases above.

## 7. D2 proof structure

Define nonnegative bucket counts, prefix sums, valid `1<=k<=n`, one-based indexing, and U. State the target: smallest i satisfying prefix(i)>=k. Present exact lifting pseudocode, including the largest power-of-two start and the strict `< k` test.

Prove the search maintains a skipped prefix strictly below the original target and a correct residual k; explain why the Fenwick candidate block can be added at each aligned step. Show halving the step refines the maximum prefix still below the target. At termination, next index is minimal; zero-count gaps do not invalidate the argument. Prove that bucket offset yields the correct increasing-ID player, then show O(log U) steps. Explain invalid/empty inputs separately.

Include a non-power-of-two example with empty buckets, not only a dense four-coordinate example. Tie-vector update costs are separate from lifting's bound.

## 8. Handoff and completion

Give Madhav canonical factory/source path, coordinate policy and transfer semantics; give Om audits and error/precondition behavior; give Bianca cases that stress tie shifts; give Sumit U, shift counters and truthful costs. Proposed reviewer: Prince.

- [ ] Current canonical header and application link without copied/patched sources.
- [ ] All operations return exact players, not just correct score buckets.
- [ ] Production associative-container issue resolved and costs updated.
- [ ] Rebuild/merge/ownership and boundary tests pass with recorded evidence.
- [ ] Final oracle and adversarial checks use this exact source commit.
- [ ] D2 authored PDF, module README, reviewed PR, tools entry and presentation segment are complete.
