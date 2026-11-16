# Prince Chavda — Structure A owner — D1

Branch: `prince-chavda`. Start with [the shared guide](../../details.md), especially sections 3–7. This is the implementation/handoff plan; checkboxes are pending acceptance criteria, not claims of completed work.

## 1. Your responsibility

Deliver a handwritten augmented AVL library implementing the canonical `leaderboard::Board` interface. Own insertion, exact-key deletion, lookup, competition-rank counting, positional selection, page extraction, score split, key split, ordered join, arbitrary league merge, destruction, metadata and inspection. Your derivation is D1: validity, sizes and `O(|h1-h2|+1)` cost of ordered AVL join.

You do not own the authoritative ID-to-score hash, replay parser, sorted-array oracle, adversarial generator or benchmark runner. Provide operations/hooks those modules can call without copying your `.cpp` or accessing private nodes.

## 2. First fix the inspected integration blockers

At `441e2339c52e1b7fb27b01ffa170e852a4b703e6`, `avl.cpp` includes the shared header but uses `Player`, `Position` and `Board` unqualified in global scope. Put the implementation in `namespace leaderboard` (private helpers may use an anonymous namespace inside it), or qualify all shared types correctly. Export exactly:

```cpp
namespace leaderboard {
std::unique_ptr<Board> make_avl();
}
```

The existing `make_avl_board()` name does not satisfy the application's factory declaration. Move the library to the agreed `src/structA/avl.cpp` path. Keep any demo `main()` in a separate file. Compile and link against Madhav's actual header before expanding experiments.

## 3. Required files and delivery format

| File | Required content |
| --- | --- |
| `src/structA/avl.cpp` | Production implementation, factory, coordinated helper definitions; no executable entry point. |
| `src/structA/README.md` | Owner, API, node layout, comparator, algorithms, invariants, costs, verified commands, tests and limitations. |
| `src/structA/tests/` | Focused insertion/deletion, rotation, rank/select, split/join, merge and lifetime regression tests. |
| `include/backend_tools.hpp` | Only agreed declarations shared with architect/B/Om/Sumit; avoid independent edits that conflict with Saksham's header. |
| `docs/derivations/D1.md`, `D1.pdf` | Authored proof, moved from `D1_AVL_Join_Proof/` with attribution retained. |
| `TOOLS.md` contribution | Generated portions, tools, your subsequent changes and actual review status. |

## 4. Implement and verify in this order

### 4.1 Nodes, comparison and metadata

Use a node containing `Player`, two children, height and subtree size. State `height(nullptr)=0`, leaf height 1, `size(nullptr)=0`. Refresh with `height=1+max(child heights)` and `size=1+left size+right size`. Compare scores descending, then IDs ascending; field comparisons avoid signed overflow.

After a rotation, refresh the demoted node before the promoted node. For double rotations, both single rotations must maintain both fields. Count primitive rotations consistently: a double rotation is two primitive rotations; separately report double-rotation events if desired. No stale size may survive a rebalance return.

### 4.2 Insertion, deletion and contains

Insert the exact key along BST order and rebalance on return. Application uniqueness is by ID globally; exact-key membership is by both fields. Document which duplicate/precondition checks your raw API performs. Delete using the old score supplied by the app; cover leaf, one-child, two-child and root replacement. If replacing a node with a successor, copy the full `Player`, then erase the exact successor key and refresh every ancestor.

`contains({id,score})` is a read-only `O(log n)` search. It must return false for a real ID with the wrong score. Failed raw operations must not corrupt the tree. Never give the application a replacement authoritative player map.

### 4.3 Rank count, select and range

`count_strictly_higher(s)` excludes all equal-score players, regardless of ID. At a node whose score is greater than `s`, its left subtree and itself count; continue right for additional higher scores. Otherwise continue left. The application adds one to obtain competition rank.

For `select(k)`, let `p=size(left)+1`: recurse left when `k<p`, return node at equality, recurse right with `k-p` otherwise. Reject 0 and `k>n`. For range, visit only relevant branches and output in order. Target `O(log n+r)` for r returned players; repeated select would instead be `O(r log n)` and must not be labeled output-optimal. Replace the caller's previous vector contents.

### 4.4 Ordered join and key split

`join_ordered(T1,k,T2)` assumes every key in T1 precedes k and k precedes every key in T2 under the leaderboard comparator. If heights differ by at most one, attach both children to the pivot. Otherwise descend the taller tree's appropriate spine, join recursively, then refresh/rebalance on return. Reuse/transfer nodes safely and leave consumed source owners empty.

`split_by_key(T,k)` returns keys preceding k and keys equal to/following k. Implement using a search path and joins. Score split is a separate boundary: every score equal to threshold goes high, including ID zero. Do not synthesize a cutoff by adding/subtracting 1 at integer extrema. Source becomes empty after split.

The current public join performs extreme-key searches to validate boundaries; those searches add work beyond the join core. For Sumit's height-gap experiment, agree a preconditioned helper with no boundary searches, as proposed in Saksham's `backend_tools.hpp`. If you retain checking in a measured wrapper, count it and report the wrapper cost separately. Never claim a checked equal-height wrapper is constant-time if it still walks down both trees.

### 4.5 Arbitrary league merge

`merge_from` accepts overlapping score ranges, including equal scores with different IDs. Use an ordered-join fast path only when the complete key ranges are separated. For general overlap, use a correct documented union/rebuild strategy; label its true cost separately.

Current overlap merge sorts all IDs for duplicate detection before merging sorted node arrays. With `N=n1+n2`, that validation is `O(N log N)`, even if the subsequent traversal/rebuild is `O(N)`. Include it or redesign using the handwritten helper. Duplicate IDs at different scores require rejection under a validating raw merge; check separated fast paths too, or explicitly agree the global-ID precondition with Madhav. Check backend type/self-aliasing before transfer. On success donor is empty; on rejected input both boards retain contents.

## 5. Inspection and counters to hand off

Coordinate the separate helper header with Madhav, Saksham, Om and Sumit. Existing proposals include `audit_avl`, `avl_stats`, `avl_split_by_key`, `avl_join_ordered`. Provide stable declarations and definitions together; agree whether audit throws and returns ordered entries. For expected tooling signatures inspect Saksham's `include/backend_tools.hpp` rather than inventing another ABI.

Audit must independently recurse and recompute height and size, validate lower/upper ordering bounds and balance, and return/check ordered membership. A self-reported `validate()` is useful but Om must also be able to detect deliberately corrupted metadata. Cycles/shared ownership must not cause endless recursion in checker tests; test-only inspection may be needed.

Define lifetime counters versus last-operation counters. Report actual height, primitive rotations and join visits, including when counters reset. A nonzero rotation count is evidence that Bianca's sequence exercised rotations; merely naming a trace `rotations` is not.

## 6. Minimum validation matrix

| Family | Required cases and expected checks |
| --- | --- |
| Basic metadata | Empty, one node, increasing/decreasing insertion; full order/height/size audit. |
| Rotations | All LL/RR/LR/RL shapes under descending-score comparison; audit after each insertion/deletion, not just the end. |
| Deletion | Leaf, one/two children, root, repeated drain to empty, deletion rebalance with child balance zero. |
| Ties | All scores equal, IDs out of order, selection within tie, competition rank 1, exact deletion. |
| Boundaries | ID 0 and UINT64_MAX, INT64_MIN/MAX scores, invalid select/range. |
| Split | Empty, all high, all low, equality, absent key cutoff, extreme thresholds; union/disjointness and source empty. |
| Join | Empty sides, equal heights, both directions of maximum attainable height gap, supplied pivot excluded from inputs. |
| Merge | Separated and overlapping ranges, ties across inputs, empty donor/receiver, self/wrong-type rejection, conflict policy. |
| Ownership | Split/merge outputs survive source destruction; no double deletion, leak or dangling pointer under available memory tooling. |
| Integration | Actual shared header, exact factory, canonical 12-line replay, Om's final harness and Bianca's cases. |

Save source commit, command and outcome for each suite. A demo on five nodes is insufficient scale evidence. Keep full audit timings out of performance results.

## 7. D1 proof structure

State total-order and distinctness assumptions, source ownership, height convention and supplied-pivot model. Prove BST order and node-set preservation first. Prove balance, height and exact subtree sizes by induction on the join descent, including the rebalance cases and refresh order. Establish the bound on result height needed by rebalancing.

Charge one recursive step to the decrease in taller-side height until height difference is at most one; prove `O(|h1-h2|+1)` node visits and constant metadata/rotation work per visited level. Separate boundary validation, pivot extraction/allocation and general merge costs. Include a concrete unequal-height diagram and map the proof to your function names.

## 8. Handoff and completion

Give Madhav the factory/source path and build instructions; give Om independent-inspection access and failure semantics; give Bianca rotation counters and valid join setup; give Sumit true costs and helper definitions before D6 is frozen. Submit an incremental PR using the shared handoff template; proposed reviewer is Saksham.

- [ ] Canonical header compiles and exact factory links without patching your source.
- [ ] Every Board method and required AVL primitive is implemented and tested.
- [ ] Size/height audits pass after every targeted adversarial mutation.
- [ ] Ordered join and general merge have separate truthful costs.
- [ ] Final oracle/adversarial runs identify your exact tested commit.
- [ ] D1 PDF names you and is consistent with the implementation.
- [ ] Module README, tool disclosure, reviewed PR and module/derivation presentation segment are ready.
