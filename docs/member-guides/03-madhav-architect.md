# Madhav — Architect and application owner — D3

Branch: `madhav`. Read [the shared guide](../../details.md). Own the common behavior, header, application, handwritten player table and integration coordination. Your derivation proves competition-rank correctness for both structures.

## 1. Starting point and scope

At `293e378620e51ccc5ce21065a6c06a2d6cb4f21f`, your branch supplies `INTERFACE.md`, `include/leaderboard.hpp`, replay/parser/application sources, custom table, tests, the canonical example and D3. Preserve this contract as the reference while integrating other branches. An interface document is not proof that all implementations conform yet.

You coordinate the root build/reproduction command and named team allocation. You do not implement teammates' structures or claim their proofs. Resolve shared decisions visibly so Om's tests, Bianca's generators and Sumit's timings use the same behavior.

## 2. Deliverable map

| Path | Your delivery |
| --- | --- |
| `INTERFACE.md` | Canonical operations, invariants, costs/scope, replay format, preconditions and clarified errors. |
| `include/leaderboard.hpp` | Single production Board declaration and exact factories. |
| `include/backend_tools.hpp` | Coordinated optional audit/stats/key-split/ordered-join declarations with A/B owners. |
| `app/player_table.hpp`, `.cpp` | Handwritten ID-to-current-score table; no standard associative-container substitute. |
| `app/replay.hpp`, `.cpp` | Shared parser, score precollection, board registry and command execution. |
| `app/leaderboard.cpp` | CLI selecting actual AVL/Fenwick factory and trace path. |
| `app/examples/basic.trace`, `.expected` | Canonical 12-command example; preserve exact expected output. |
| `app/tests/` | Hash, replay, errors and board lifecycle tests. |
| Root build/run orchestrator + README | One verified full-run command and separate optional quick mode. Name/implementation agreed during integration. |
| `docs/derivations/D3.md`, `D3.pdf` | Authored competition-rank proof. |
| Root README/TOOLS/report coordination | Roles, instructor access, consolidated disclosures, consistent instructions and release evidence. |

## 3. Freeze these decisions before teammates finish adapters

1. Confirm full-width numeric types, descending scores/ascending IDs, competition rank versus position, inclusive ranges, equality in high division and consuming split/merge.
2. Publish exact header revision and library source/factory names. Require every backend to include that header rather than importing an old copy.
3. Adopt/version the helper API from Saksham only after Prince, Om and Sumit agree its behavior and counter reset rules. It is not currently part of your inspected branch.
4. Document error precedence when a command violates several rules. Match parser-level rejection, board checks, ID checks and range checks between app and oracle.
5. Specify raw backend preconditions versus checks: global uniqueness, merge conflicts, wrong types, unknown Fenwick coordinates and exceptions. Distinguish user errors from internal failures.
6. Clarify board ID reuse. Contract says new IDs must be unused; current registry only tracks active IDs. Either enforce never-used IDs or explicitly document active-ID reuse and update all consumers together. Valid generators should use fresh IDs meanwhile.
7. Preserve historical truth: the course requires interface/invariants before structures. Record actual dates; never backdate a document or rewrite attribution to imply compliance.

## 4. Handwritten player table

The table maps full-width PlayerId to current Score globally across active boards. Pick/document open addressing or chaining. If open addressing, explicitly represent empty, occupied and tombstone states; ID zero cannot double as an empty sentinel. If chaining, own nodes and handle collision chains/resizing correctly.

Required behavior: get absent/present, insert unique ID, update existing score, erase existing ID, reserve/rehash, size and destruction. Rehash preserves all entries including negative/extreme scores and wide IDs. Define maximum load and growth; explain expected versus worst-case costs honestly. Standard `unordered_map` is not the implementation.

Test deliberately colliding keys using knowledge of your hash/capacity; deletion in the middle of a probe chain; lookup past tombstones; repeated erase/reinsert; growth thresholds; update without size changes; empty table; ID 0/UINT64_MAX; INT64_MIN/MAX scores. Hash count must equal the sum of active-board sizes. Om should cross-check actual membership, not just count.

## 5. Command execution and state consistency

| Command | Required sequence / invariant |
| --- | --- |
| ADD | Validate board, check global ID absence, insert exact Player and hash entry; report OK only when consistent. |
| SET | Global lookup, verify exact membership on requested board, no-op if unchanged; erase old key, insert new key, update hash. Precollect new score for B. |
| REMOVE | Global lookup, exact board membership, erase key and hash entry; no stale lookup afterwards. |
| RANK | Lookup score, confirm board membership, return backend higher count + 1. |
| SELECT/RANGE | Validate one-based positions first; output exact ordered `(id,score)` values and inclusive count. |
| SPLIT | Validate distinct fresh output IDs; split backend; install high/low and invalidate source ID. Hash entries/scores remain unchanged. |
| MERGE | Validate both boards and fresh result ID; merge actual backend objects; invalidate sources and install result. Hash entries/scores remain unchanged. |

Any normal rejected command leaves all boards and hash unchanged. Validate foreseeable input errors before mutations. Define exception behavior if allocation/backend failure interrupts a multi-step update: either rollback where supported or fail the run explicitly without pretending a recoverable `ERR` has preserved state. Do not promise strong exception safety unless implemented/tested.

The contract lists `ID_CONFLICT`, but correct replay ADD prevents active duplicates. Raw conflicting boards still need an agreed policy; current `merge_from` exceptions are not automatically translated into a replay `ERR ID_CONFLICT`. Document reachability/mapping instead of fabricating an integration test that cannot create the state.

## 6. Parser and CLI

Support only the exact shared commands and token counts. Enforce uint64 bounds on IDs/positions and int64 bounds on scores; reject negative unsigned fields, numeric trailing junk, overflow, missing/extra arguments and unknown command names. Current implementation ignores whitespace-only lines. Preserve that behavior or change contract/tests together.

Preparse the replay and collect scores from syntactically valid ADD/SET commands before calling the Fenwick factory. The factory handles sorting/deduplication. Document replay memory proportional to command count and collected coordinates; preprocessing time is separate from query latency.

Keep protocol stdout clean: exactly one result per nonblank command, no banners/debug output. CLI usage, open-file errors and fatal exceptions go to stderr with nonzero exit status. At the inspected revision the CLI is `leaderboard <avl|fenwick> <trace-file>`; do not document an oracle CLI mode unless you add it deliberately.

## 7. Integration procedure

1. Review Prince's namespace/factory fix and Om's missing contains override. Ensure all actual sources use your header.
2. Agree final `src/structA/avl.cpp` and `src/structB/fenwick.cpp` paths, separate executable entry points and helper definitions. Link the full app against both factories.
3. Run the canonical example for both backends and compare all twelve lines to `.expected`.
4. Extend replay tests to ties, invalid commands, multiple boards, consumed IDs, wrong-board requests, empty divisions, overlap merge and wide values. Run with both actual backends; an in-test fake is supplementary.
5. Integrate Om's final differential/audit suite and Bianca's generators without replacing their work with app-local duplicates. Resolve interface failures before timing.
6. Add a root orchestrator invoking compilation, all tests, application example and every required benchmark; propagate nonzero exit codes. Record full/quick distinctions, prerequisites, outputs and expected duration from actual runs.
7. Work with Sumit on the million-player/million-revision replay and with Om on feasible correctness phases. Ensure season splits/merges are demonstrated, not just rank queries.
8. Coordinate results/report/derivation placement and final clean-clone verification. Do not merge legacy demos that accidentally define another main or retain forbidden production lookup maps.

## 8. D3 proof requirements

Define total order and competition ranking; let H(s) be the number of players whose score is strictly greater than s. Prove rank(p)=1+H(p.score), independent of ID within a tie.

For AVL, prove why left-subtree size plus node can be counted when node.score > s, and why node/right may be excluded otherwise under the total order. State correct subtree sizes as a prerequisite; D4 supplies rotation preservation. Prove the accumulated result is exactly H(s).

For Fenwick, define descending coordinates, the boundary before all coordinates equal to s, and why the prefix covers exactly greater-score players. Establish that bucket counts equal live multiplicities, including ties and gaps. Conclude both implementations return the same rank. Demonstrate ranks 1,2,2,4 and contrast select positions only to explain the required distinct behavior.

State complexity assumptions separately: expected hash lookup plus O(log n) AVL or O(log U) Fenwick, not unconditional worst-case O(1) hashing. Map statements to actual app and backend functions.

## 9. Final handoff checklist

- [ ] Team allocation and canonical contract revision are recorded; all open decisions above resolved.
- [ ] Hash is handwritten and tested, including collisions/deletion/rehash.
- [ ] App links real A/B factories and both canonical replays match exactly.
- [ ] Error, lifecycle and membership tests exercise final application sources.
- [ ] One documented full-run command covers tests, app and all benchmarks from a fresh clone.
- [ ] README, report, TOOLS and separately authored D1–D6 PDFs are assembled without losing attribution.
- [ ] Instructor read access, one reviewed PR per member and meaningful history are verified.
- [ ] D3 PDF, module/derivation slide, shared interface/invariant explanation and full-scale demo are ready.
- [ ] Only the validated release commit is tagged final after team review; no pending fixes are hidden after the tag.

Proposed reviewer for your PR: Om. Ask each owner to review their own integrated source and evidence before declaring their module complete.
