# Project 2: team execution and integration guide

Prepared 17 November 2026. Read this first, then your individual guide. This is a work plan, not a claim that the listed work has already passed tests.

## 1. Sources, authority, and scope

The course screenshots supplied by Sumit define the assignment. The common behavior and C++ contract below come from [madhav/INTERFACE.md](https://github.com/pachisiamadhav26-eng/DSA_PROJECT/blob/293e378620e51ccc5ce21065a6c06a2d6cb4f21f/INTERFACE.md) and its `include/leaderboard.hpp`. The remote branch hashes were checked against GitHub on 17 November 2026 and matched the local remote-tracking snapshots.

Course requirements are mandatory. The madhav interface is the team's integration contract; it must not be independently redesigned by each member. File arrangements, review pairings, evidence schemas, and stages proposed here are coordination recommendations, not extra course rules. Any change to a shared API needs a documented change by Madhav plus updates to all consumers.

No official named allocation sheet was supplied. Names and role assignments below follow repository branches and the existing handoff documentation. Record the confirmed allocation in README.md at the checkpoint. Do not present this guide as proof of an instructor-approved allocation.

## 2. Six owners and their guides

| Member | Branch | Role and main ownership | Derivation | Individual instructions |
| --- | --- | --- | --- | --- |
| Prince Chavda | `prince-chavda` | Augmented AVL, ordered join, key split, structural inspection | D1 | [Prince](docs/member-guides/01-prince-avl.md) |
| Saksham | `Saksham` | Fenwick, compressed coordinates, tie buckets, rebuilding, inspection | D2 | [Saksham](docs/member-guides/02-saksham-fenwick.md) |
| Madhav | `madhav` | Shared interface, application, handwritten player table, integration/build coordination | D3 | [Madhav](docs/member-guides/03-madhav-architect.md) |
| Om | `om-oracle` | Sorted-array oracle, differential harness, independent audits, failure shrinking | D4 | [Om](docs/member-guides/04-om-oracle.md) |
| Bianca | `Bianca_adversary` | Adversarial generators, reproducible cases, observed effects | D5 | [Bianca](docs/member-guides/05-bianca-adversary.md) |
| Sumit | `sumit` | Benchmark harness, counters, profiling, prediction, figures, results/report coordination | D6 | [Sumit](docs/member-guides/06-sumit-analyst.md) |

The architect coordinates assembly, but each owner remains responsible for their own code, tests, documentation, derivation, and explanation during the viva. This plan assumes six members. If a seventh is officially added, assign the course's Scale and Reproducibility role and D7 separately.

## 3. Current snapshot: what exists and what still needs integration

| Branch | Inspected commit | Observed content and immediate action |
| --- | --- | --- |
| `madhav` | `293e378620e51ccc5ce21065a6c06a2d6cb4f21f` | Contract, shared header, replay, custom player table, tests, D3 source/PDF exist. Backend linking and combined validation remain team work. |
| `prince-chavda` | `441e2339c52e1b7fb27b01ffa170e852a4b703e6` | `avl.cpp` has tree operations and D1 exists under `D1_AVL_Join_Proof/`. Types are used unqualified outside the shared namespace; exported factory is `make_avl_board()` rather than required `leaderboard::make_avl()`. Fix integration first. |
| `Saksham` | `82fd786205a84472e4fe956105192f8eec87c9a3` | `src/structB/fenwick.cpp`, tests, `include/backend_tools.hpp`, D2 exist. Production merge uses `std::unordered_set` for ID checks; reconcile with the course's library-container restriction. |
| `om-oracle` | `fc2a10c93514b18d1cb87adfbcc0c71205afec1a` | Sorted-array oracle, reducer, tests, D4 exist. `SortedArray` lacks the current pure virtual `contains(Player)` override. Differential runner uses legacy adapters; migrate validation to final backends. |
| `Bianca_adversary` | `6d7e85ac0f791a98f82ec15eacc2faf27795dc18` | Generators, runner, cases and D5 draft exist. Recorded checks use pinned legacy adapters; repeat relevant checks against the integrated production build. |
| `sumit` | `cfd8ccf74372d81a820cd605376363a31a7bdefa` | Working checkout also has uncommitted benchmark infrastructure, benchmark-specific `bench/run_all.py`, handoff/report material, D6 draft, scripts and practice results. These are local work, not evidence of a released integrated system. |

These are source observations, not a fresh execution of teammate test suites. Historical validation documents apply only to the commits and adapters they actually tested.

## 4. Shared semantics: every member must use these

### 4.1 Types and order

Use C++17. `PlayerId` and `Position` are `std::uint64_t`; `Score` is `std::int64_t`. `Player` has fields in this order: `id`, then `score`. Support ID zero, full-width IDs, negative scores, and signed score extrema. Never narrow through `int`, and never compare using subtraction or negating the score.

```cpp
bool before(Player a, Player b) {
    return a.score != b.score ? a.score > b.score : a.id < b.id;
}
```

The first player has the greatest score. For equal scores, smaller IDs come first. Position is one-based and unique. Competition rank is `1 + number of players with a strictly higher score`; equal scores share rank. `RANGE` refers to positions, not a band of competition ranks.

For players `(5,100), (3,90), (8,90), (2,70)`, ordered IDs are `5,3,8,2`, competition ranks are `1,2,2,4`, and `select(3)` returns `{8,90}`. A score threshold of 90 puts IDs `5,3,8` in high and ID `2` in low.

### 4.2 Exact production C++ boundary

This reproduces the inspected shared header. Include the canonical file; do not create six private copies of these declarations.

```cpp
namespace leaderboard {
using PlayerId = std::uint64_t;
using Score = std::int64_t;
using Position = std::uint64_t;
struct Player { PlayerId id; Score score; };

class Board {
public:
    virtual ~Board() = default;
    virtual Position size() const = 0;
    virtual bool contains(Player player) const = 0;
    virtual void insert(Player player) = 0;
    virtual void erase(Player player) = 0;
    virtual Position count_strictly_higher(Score score) const = 0;
    virtual Player select(Position position) const = 0;
    virtual void range(Position first, Position last,
                       std::vector<Player>& result) const = 0;
    virtual std::pair<std::unique_ptr<Board>, std::unique_ptr<Board>>
        split_by_score(Score threshold) = 0;
    virtual void merge_from(Board& other) = 0;
};
std::unique_ptr<Board> make_avl();
std::unique_ptr<Board> make_fenwick(const std::vector<Score>& possible_scores);
}
```

The header supplies `<cstdint>`, `<memory>`, `<utility>`, and `<vector>`. Production backend translation units export factories, not `main()`. Each executable has one entry point. Tests and legacy demos must not accidentally enter the application link command.

| Boundary rule | Required behavior |
| --- | --- |
| `contains` | Exact `(id,score)` membership in this board; no mutation. |
| `erase` | Receives the old exact key from the application; no separate per-backend authoritative player map. |
| `count_strictly_higher` | Returns a count, not count plus one. Rank adds one in the application. |
| `range` | Replace/clear the output vector, return inclusive positions in order, no appended stale values. |
| `split_by_score(t)` | Return `{high,low}` where high has `score >= t`; source object becomes empty. |
| `merge_from(other)` | Same backend type; receiver holds union and donor becomes empty on success. Self merge is invalid. |
| Ownership | Use unique ownership; emptied sources remain destructible. No shared owning AVL nodes or double frees. |
| Application boards | Successful split invalidates the source board ID; successful merge invalidates both input IDs and creates a fresh result ID. |
| IDs | Unique across all active boards. A board-specific command checks exact membership, not just global lookup. |
| Fenwick universe | Replay scans ADD/SET scores before running. Sort/deduplicate them descending. Never silently assign an unknown score to a nearby coordinate. |

Normal user errors must leave state unchanged. Contract/precondition exceptions and resource failures need a documented policy; do not turn an internal bug into `OK` or an invented user-error code. Ordered AVL join is distinct from arbitrary league merge: overlapping score ranges cannot use the ordered-join bound.

### 4.3 Replay and output format

UTF-8 text; one command per nonblank line; ASCII command tokens; decimal numeric fields; no comments or metadata lines in a trace. Board 0 initially exists and is empty. Current replay ignores blank lines. Unsigned fields reject signs, fractions and overflow; scores use signed decimal with optional `-` in the inspected parser. Do not assume `+5` is accepted.

| Input | Output on success |
| --- | --- |
| `ADD board id score` | `OK` |
| `SET board id new_score` | `OK`, including unchanged score |
| `REMOVE board id` | `OK` |
| `RANK board id` | `RANK value` |
| `SELECT board position` | `PLAYER id score` |
| `RANGE board first last` | `PLAYERS count id score id score ...` |
| `SIZE board` | `SIZE value` |
| `SPLIT board threshold high_board low_board` | `OK` |
| `MERGE left_board right_board result_board` | `OK` |

Each command produces exactly one output line. Diagnostics and generator settings belong on stderr or in sidecars, never replay stdout. New output board IDs must be unused and distinct from sources and each other. Use monotonically fresh board IDs in valid generators to avoid ambiguity about reuse of consumed IDs.

Valid error lines are `ERR UNKNOWN_BOARD`, `ERR BOARD_EXISTS`, `ERR DUPLICATE_ID`, `ERR UNKNOWN_PLAYER`, `ERR WRONG_BOARD`, `ERR BAD_POSITION`, `ERR BAD_RANGE`, `ERR SAME_BOARD`, `ERR ID_CONFLICT`, and `ERR BAD_COMMAND`. For traces violating multiple conditions, use the architect's documented precedence; do not let each test author invent one. `ID_CONFLICT` is listed by the contract, but ordinary replay cannot create duplicate active IDs if ADD is correct; test conflicting raw boards separately and agree exception-to-output mapping.

Use `app/examples/basic.trace` and `.expected` from madhav as the canonical twelve-line smoke test. Do not rewrite their expected answers to fit a backend.

## 5. Final repository arrangement and build boundary

```text
README.md                     project, named roles, prerequisites, one full-run command
INTERFACE.md                  canonical semantics/invariants, maintained by Madhav
REPORT.md                     predictions, design, real results, limitations
TOOLS.md                      consolidated tool/AI use and subsequent changes
details.md                    this team plan
include/leaderboard.hpp        shared production contract
include/backend_tools.hpp     coordinated audit/statistics/AVL primitive helpers
src/structA/                   Prince's library implementation and focused tests
src/structB/                   Saksham's library implementation and focused tests
app/                          Madhav's replay, custom player table, examples, tests
oracle/                       Om's independent reference, differential tests, reducer
adversary/                    Bianca's generators, fixtures, validation instructions
bench/                        Sumit's workloads, timing/counter/profile drivers
docs/member-guides/            these six role guides
docs/derivations/              D1 through D6 as separate authored PDFs and sources
results/                      raw CSV, metadata, summaries, figures, plotting scripts
```

Existing files can be moved into these assignment paths through reviewed changes. Avoid duplicated implementations in root demos and library directories. The shared helper header currently exists on Saksham, not madhav; coordinate its adoption instead of treating it as already canonical. Raw AVL node types stay private. For audit, ordered join, key split and counters, expose a separately agreed helper API without changing `Board` casually.

`std::vector`, sorting, and pointer utilities are used by the shared design. They must not replace the AVL, Fenwick or handwritten hash. The course explicitly limits library containers as structure substitutes and says they are allowed only in tests/data generation; review auxiliary associative containers in production conservatively. In particular, move production `unordered_set` conflict validation to a handwritten helper or a documented sorted-vector approach; keep test-only sets confined to test code.

Madhav should deliver a root build/run orchestrator, with a documented invocation such as `python run_all.py` (proposed, currently not supplied by this guide). It must build, run required tests, demonstrate the application, and run every required benchmark. Quick smoke mode can be separate but must not be advertised as completing full-scale submission checks. Exact compiler/source commands must be documented and verified after integration.

Proposed production application link after the agreed source moves:

```powershell
New-Item -ItemType Directory -Force build | Out-Null
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -I include -I app app/leaderboard.cpp app/replay.cpp app/player_table.cpp src/structA/avl.cpp src/structB/fenwick.cpp -o build/leaderboard.exe
.\build\leaderboard.exe avl app/examples/basic.trace
.\build\leaderboard.exe fenwick app/examples/basic.trace
```

This is a target command, not a claim that today's sumit checkout already contains those paths or builds. Both outputs must exactly match `basic.expected`.

## 6. Work order and handoffs

| Stage | Work and required exit evidence |
| --- | --- |
| 1: freeze contract | Madhav records header revision, error precedence and helper decisions. Everyone checks types, tie ordering and ownership. Preserve honest history: do not backdate the requirement that interface precede implementation. |
| 2: compatible modules | Prince fixes namespace/factory and publishes library path. Saksham uses canonical header and resolves production container issue. Om supplies `contains` and migrates legacy dependencies. Link one app with both real factories. |
| 3: deterministic correctness | Canonical example, ties, boundaries, invalid operations, split/merge ownership, hash checks and focused structure tests pass. Audits independently recompute metadata. |
| 4: difficult and randomized correctness | Bianca's cases run through final app and Om's harness. Om completes million seeded operations with declared live-set size and shrinking checks. Full size audits follow every mutation in targeted rotation cases. |
| 5: prediction freeze | Each owner gives Sumit actual implementation costs. Sumit commits D6 models before collecting formal measurements. |
| 6: scale and performance | At least one million simultaneous players and one million score revisions with queries; five or more repetitions per timing; all seeds and machine details recorded. Correctness checks outside timing. |
| 7: submission | Consolidate docs/PDFs/results, member PR reviews, 10–12-slide maximum presentation, clean-clone reproduction, then tag the validated commit `final`. |

Dependencies are bidirectional: Om/Bianca report minimal failing traces to structure owners; structure owners return fixed commits and regression checks; Sumit rebuilds against those exact commits before formal results. The architect maintains a table of open blockers instead of marking an entire role complete because its demo runs.

## 7. Common deliverable formats

### 7.1 Module README

Each module README must contain: named owner and branch; purpose and public entry points; dependencies and exact header revision; source/test file map; verified build/run commands; worked example; invariants; time/space costs with variables defined; edge cases; validation actually executed; known limitations; handoff to consumers. Separate measured facts from expected behavior.

### 7.2 Derivation source and PDF

Keep each D1–D6 separately in `docs/derivations/`. Canonical recommendation: `D1.md` + `D1.pdf` through `D6.md` + `D6.pdf`; preserve attribution when moving existing names. The PDF must name its author. Use this order:

1. Title, derivation number, author, role, date, source commit.
2. Exact statement and assumptions; definitions and notation matching code.
3. Algorithm/pseudocode and invariant or recurrence.
4. Correctness proof with base case, induction/loop step and termination.
5. Complexity derivation with charged operations and boundary cases.
6. Small worked example/diagram and mapping to implementation functions.
7. References and tool-use disclosure/reference to TOOLS.md.

A passing test is evidence of implementation behavior, not a substitute for a proof. A bound for ordered join cannot be relabeled as a bound for arbitrary merge.

### 7.3 Test evidence and failure handoff

For every reported validation, record date, owner, exact source commits, compiler/flags, full command, seed, requested and actual command counts, peak live players, exit code, result and log paths. Generated workloads record generator version/hash and trace hash. A failure handoff contains the original trace, reduced trace, expected and actual output, first differing step, invariant failure, and exact reproduction command. Preserve the failure before fixing it; add it to regression tests.

### 7.4 Benchmark evidence

The local benchmark CSV currently uses this schema; preserve it unless a coordinated versioned change is necessary:

```csv
backend,experiment,operation,players,coordinates,seed,run,calls,elapsed_ns,ns_per_call,checksum,validation,left_height,right_height
```

`players` is live population for the measured operation; `coordinates` is actual compressed universe size; `calls` is the number represented by the elapsed time. `elapsed_ns` is a batch duration and `ns_per_call = elapsed_ns/calls`. `left_height/right_height` apply to ordered-join experiments; existing runner uses -1 where inapplicable. `validation` identifies actual checking, not an unearned PASS.

Store machine CPU/cache/compiler/flags, source commits and hashes, experiment settings, timing inclusions, trace hashes, run status and limitations in a companion metadata JSON. Keep raw rows for each repetition; derive means and a declared spread (for example sample standard deviation plus min/max) in summaries. Record counter reset boundaries and whether instrumentation affects timing. Cache claims require actual profiler output, not elapsed-time inference.

### 7.5 Pull request handoff

Use a PR body with these fields:

```text
Owner / role / derivation:
Problem and resulting behavior:
Files and exported API:
Canonical interface revision:
Dependencies and exact commits:
Build and test commands actually run:
Results, seed/count/live-size, evidence paths:
Cost changes and known limitations:
Tool-assisted work and human changes:
Reviewer and requested integration action:
```

Proposed review pairing: Prince reviews Saksham, Saksham reviews Prince; Madhav reviews Om, Om reviews Madhav; Bianca reviews Sumit, Sumit reviews Bianca. Each member needs at least one PR reviewed by another member. Use meaningful incremental commits; one or two late bulk uploads lose the course's individual GitHub credit. Do not overwrite peers' TOOLS.md entries during merges.

## 8. Acceptance matrix

| Requirement | Primary owner | Evidence / acceptance |
| --- | --- | --- |
| Exact adapter compatibility | Madhav + A/B/Om | One header; app and harness link actual implementations without patched source copies. |
| Correct ranks/tie positions/pages | All, checked by Om | Example plus tie-heavy and randomized output equality. |
| AVL size/height/order/balance | Prince + Om | Independent audits after rotations, deletions, split and join; all rotation shapes. |
| Fenwick counts/select/tie IDs | Saksham + Om | Count/bucket audit, empty coordinates, zero-count gaps, full-width values. |
| Handwritten lookup | Madhav + Om | Collision, deletion, resize, wrong-board and split/merge consistency checks. |
| Adversarial targets | Bianca + Prince/Om | Generator, predicted effect, measured counters, per-operation size audit, unequal-height joins. |
| Differential and reduction | Om | One million seeded operations with declared population; same-failure 1-minimal reducer evidence. |
| Full application workload | Madhav + Sumit | One million players and one million actual score revisions plus queries, split/merge demonstration. |
| Timing and predictions | Sumit + owners | Predated model, raw >=5 repetitions, spread, seeds, machine, figures; separate join/general merge labels. |
| Individual deliverables | Each member | Named code, reviewed PR, authored derivation PDF, module slide and explanation. |
| Reproducibility | Madhav coordinates all | Fresh clone of final-tagged commit; one documented command runs required components and experiments. |

Do not confuse one million operations on a 128-player live set with one million simultaneous players. Both correctness stress and scale behavior need evidence. Full audits after every command at million-player scale can be quadratic; use bounded-live-set differential stress, full-scale phase/checkpoint checks, and the mandatory per-operation audits on declared targeted adversarial tests. Report exactly what each run establishes.

## 9. Report, presentation and final release

Each owner supplies a short report section with design, invariants, operation costs, tests, limitations, and links to derivation/evidence. Sumit assembles figures and measurement analysis; Madhav coordinates consistency and final build instructions. Never manufacture timing data or hide unsupported cases.

The report must explain coordinate preprocessing and tie-vector costs, AVL join boundary checking/pivot preparation, general merge cost, oracle validation scope, adversarial effects and prediction gaps. Include real profiler records for cache claims.

The course specifies 15 minutes total, at most 10–12 slides, diagrams for both structures, interface/invariants, one module-and-derivation slide per member, adversarial inputs/effects, and predicted versus measured costs. It also says 3–4 minutes per member, which exceeds 15 minutes for six people. Seek instructor clarification; do not silently turn this into a different duration requirement. Prepare a 15-minute team rehearsal and keep deeper derivation material available.

Before tagging: every listed requirement has linked evidence; open blockers are resolved; PDF authors are correct; the instructor has repository read access; tool use is consolidated; reviewed PRs are recorded; a fresh clone reproduces the release at full scale. Only the validated release commit is tagged `final`; later commits do not count for the examiner's hidden tests. No release tag, push, merge or branch change is performed by creating this guide.
