# Plain-struct leaderboard interface

This document describes the shared interface change for the live-leaderboard project. Structure A uses an AVL tree and Structure B uses a Fenwick tree. Both must produce the same observable results for the replay application, oracle, tests and benchmarks.

The simplified design uses ordinary structs, concrete objects and reference parameters. The common headers are **leaderboard.hpp** and **backend_tools.hpp**.

## Architect handoff: publish this folder

Commit this entire folder at the repository root with the name **interface**.

~~~text
DSA_PROJECT/
    interface/
        README.md
        leaderboard.hpp
        backend_tools.hpp
~~~

This package contains the shared declarations and the migration guide. Each contributor implements and exposes their concrete struct as specified below.

### Prompt for a contributor's coding assistant

Commit this folder first, then have each contributor obtain that commit on their working branch. Give their coding assistant the following task, filling in the role, module paths and branch.

~~~text
Read interface/README.md, interface/leaderboard.hpp and
interface/backend_tools.hpp completely before editing.

My role is [role]. My modules are [paths], on branch [branch].
Migrate my modules and their direct tests to the plain-struct contract.
Use the exact public signatures, built-in numeric types, ordering, ranking
and source-consumption rules described in those files.

Use the existing implementation where possible. Keep changes scoped to my
modules and list any cross-module changes the architect must coordinate.
Preserve the replay format and existing golden outputs.
Update my assigned derivation and cost assumptions where the code changes.

Run the relevant checks. Report the changed files, exact commands and
results, and any remaining integration dependencies. A module test passing
does not establish that the entire migrated project is working.
Prepare the changes locally for review and leave pushing to me.
~~~

The README supplies the shared member signatures as well as the two headers' declarations. Contributors must check both sources of the contract. The architect verifies the combined build and full acceptance suite after merging the coordinated module changes.

### Initial publication on main

1. Add the three files under interface/ so all contributors can read the same proposed contract.
2. The existing main build can continue using its current include/ headers while contributors prepare the coordinated migration.
3. Each contributor follows the signatures and ownership rules in this README on their branch.
4. Integrate the matching backend, application, oracle, test and benchmark changes together.
5. Update the build configuration to select the new headers consistently, run all acceptance checks, and then retire the old contract.

Publishing interface/ is a documentation-and-contract handoff. It does not by itself migrate the existing application or establish the full project's correctness.

### Header selection during migration

For a build using these headers, add the interface directory to the compiler search path. For example, a fully migrated program can use:

~~~powershell
g++ -std=c++17 -Iinterface your_program.cpp -o your_program.exe
~~~

Use the same contract in every source file linked into one executable. Quoted includes search the including file's own directory first, so remove or update references that would accidentally pick the old include/leaderboard.hpp. Merely adding another compiler flag does not resolve mixed interface versions.

Place concrete backend definitions in headers visible to the application and tests, such as src/structA/avl.hpp and src/structB/fenwick.hpp. This folder's forward declarations introduce their names; their owners still supply the definitions.

## Migration status

The matching standalone Fenwick example compiles with these headers and passed 3,000 sorted-list checks.

The existing repository builds with its original interface. Its quick test suite passed, including the four golden replay files, three seeded 10,000-command runs and the 10,000-player scale check. Compiling the existing application against the simplified headers fails because it still refers to Board and BoardFactory. The existing backends also require adaptation.

**The complete repository must be migrated before the main build uses these headers.** The interface folder can first be published on main as the migration contract. The tasks below assign the changes to module owners.

## Review against the professor's project requirements

**The plain-struct interface can support the requirements in the supplied handout.** The handout specifies operations, invariants, algorithms, proofs and measurements. These can be implemented through ordinary struct members and reference parameters.

The two headers establish common data types and helper declarations. Complete backend member definitions, the application, the six derivations and the required experiments are still needed to establish implementation compliance.

| Handout requirement | Mapping to the simplified design | Evidence needed after migration |
|---|---|---|
| One million players with continuously revised scores | long long counts and scores; unsigned long long IDs; erase followed by insert for a revision | Run with one million active players and one million score revisions, interleaved with queries and season operations. |
| Structure A is an AVL tree keyed by score and player ID | struct AVL with the agreed total order | Audit key order, heights, balance factors and every subtree size. |
| AVL insertion and deletion | insert(Player), erase(Player) | Demonstrate logarithmic tree updates, including rotations and deletion cases. |
| Rank queries | Application rank(id) uses the handwritten lookup, contains(Player), and one plus count_strictly_higher(score) | Compare the rank of tied and untied players against the sorted array. |
| Selection by position | select(position) in both structs | AVL descends by subtree sizes; Fenwick uses binary lifting to find the first prefix reaching the position. |
| Players in positions 40 to 50 | range(40, 50, result) | Return exactly 11 players, in the agreed ID order within ties, when the board has at least 50 players. |
| Ordered AVL join(T1, k, T2) | avl_join_ordered(left, pivot, right, result) | Preserve key order, AVL balance and subtree sizes in O(1 + absolute height difference). |
| AVL split(T, k) | avl_split_by_key(source, key, before, after) | Implement as ordered joins along a search path and prove the total O(log n) cost. |
| Score-threshold season split | split_by_score(threshold, high, low) | High has scores >= threshold; low has scores < threshold. Their union equals the source, which becomes empty. |
| Structure B stores counts over compressed scores | struct Fenwick, score coordinates, count array and sorted ID buckets | Verify all bucket counts, prefix counts, exact selected IDs and total size. |
| Fenwick split and join use rebuilding | split_by_score and merge_from | Rebuild counts from the resulting buckets and include all coordinate, bucket and ordering work in costs. |
| Merge two regional leagues | merge_from(other) on both structures | Handle overlapping score ranges and ties with distinct IDs. Measure general merging separately from ordered AVL join. |
| Player ID to current score is a hash table written from scratch | Application-owned handwritten player table | Preserve insert, lookup, update and delete correctness through churn and resizing. |
| Oracle is a sorted array | Independent C++ sorted-vector reference | Compare complete player results, ranks and divisions with both real backends. |
| Adversarial updates and unequal-height joins | Existing generators and concrete audit/statistics functions | Audit the entire AVL after every operation in rotation-focused adversarial tests; test joins with very different heights. |
| Measure rank/select as n grows, and AVL season operations against Fenwick rebuilding | Existing benchmark cases updated to concrete structs | Write cost predictions first, run both backends on identical inputs, and explain observed gaps. |
| D1 through D6 | Assigned derivations below | Verify each argument against the migrated implementation and regenerate submitted documents as needed. |

The old contract's abstract Board and factory functions were implementation choices. Removing them changes caller syntax while preserving the professor's operation requirements.

The exact ID tie-break, consuming source boards, output-reference signatures and replay syntax below are team conventions. They make the professor's required operations observable and consistent across the four implementations.

### Complexity obligations

Use n for active players, U for prepared score coordinates, t for a tie-group size and r for the number of returned players.

| Operation | Required or expected Structure A cost | Expected cost of the supplied simple Structure B |
|---|---|---|
| Insert / delete | O(log n), with metadata updated through rotations | O(log U + t), including shifts in a sorted ID vector |
| Score revision | Two O(log n) tree operations | O(log U + t_old + t_new) |
| Rank | O(log n) after ID lookup | O(log U) after ID lookup |
| Select one exact player | O(log n) | O(log U), including constant-time ID-vector indexing |
| Return r positions | O(log n + r) | O(r log U) using repeated select |
| Split by key / score | O(log n) structural work using joins | O(U) for coordinate copies, bucket transfers and rebuilding |
| Ordered join with a supplied pivot | O(1 + absolute height difference) | League merging rebuilds its count index |
| General league merge | State and justify the actual union algorithm's cost | Include ID validation, map operations, tie-bucket sorting, copying and rebuilding |
| Storage | O(n) for nodes | O(U + n) per board retaining its coordinates, including arrays and stored IDs |

The handout describes logarithmic rank-range extraction. Returning r explicit players requires at least O(r) output work. Document AVL extraction as O(log n + r). For the fixed page 40 to 50, r is 11.

The supplied simple Fenwick merge uses an ordered map and set, then sorts the combined ID buckets. A conservative bound is O((n1+n2) log(n1+n2+1) + (U1+U2) log(U1+U2+1)). Its count-array rebuild alone is linear in the resulting coordinate count. Measure and describe the complete merge.

For ordered AVL join, the caller establishes the key-range and unused-pivot preconditions. A full input traversal inside the timed ordered primitive would add linear work. Finding a separating pivot before rejoining two divisions has its own cost and belongs in the end-to-end rejoin measurement.

Preserve the logarithmic structural AVL split by transferring tree ownership through references. Re-inserting every output player or deep-copying the output trees would change that cost. The ID-to-score table requires no per-player score updates during a split or merge.

### Handwritten player table and allowed STL use

The std::map in the standalone main() is a demonstration lookup. The final replay application must use the handwritten player-ID-to-score hash table required by the professor.

Vectors used for score counts and tied IDs, and temporary maps/sets used to rebuild Fenwick divisions or check IDs, support the implementation. Rank and score-bucket selection must actually use the implemented Fenwick count array. Structure A must actually use an augmented AVL tree.

## Shared data and types

The common player header defines the player and declares the two concrete structure names.

~~~cpp
struct Player {
    unsigned long long id;
    long long score;
};

struct AVL;
struct Fenwick;
~~~

| Value | Type |
|---|---|
| Player ID | unsigned long long |
| Score | long long |
| Position and active player count | long long |
| Tree height | int |
| Rotation and bucket-shift counters | unsigned long long |

Scores may be negative. IDs are unique across all active boards. Positions start at one.

Compare scores and IDs directly instead of subtracting them. Test the minimum and maximum long long scores and the maximum unsigned long long ID. A score split at the minimum score puts every active player in high; avoid overflowing when computing threshold boundaries.

The two forward declarations introduce type names. Each backend owner must expose the complete struct definition in a header available to its callers. Suggested locations are src/structA/avl.hpp and src/structB/fenwick.hpp. Keep the demonstration main() in a separate source file for the integrated application build.

## Member functions required in both structures

Use AVL for the parameter types in Structure A and Fenwick for the parameter types in Structure B. These declarations show Structure B.

~~~cpp
#include "leaderboard.hpp"
#include <vector>
using namespace std;

struct Fenwick {
    // Fields and implementation helpers.

    Fenwick(const vector<long long>& possibleScores = {});
    long long size() const;
    bool contains(Player player) const;
    void insert(Player player);
    void erase(Player player);
    long long count_strictly_higher(long long score) const;
    Player select(long long position) const;
    void range(long long first, long long last, vector<Player>& result) const;
    void split_by_score(long long threshold, Fenwick& high, Fenwick& low);
    void merge_from(Fenwick& other);
};
~~~

These are the shared member signatures. Backend-specific helper functions remain inside their structs.

| Function | Required behavior |
|---|---|
| size() | Return the active player count. An empty board returns zero. |
| contains(player) | Check the exact score and ID together on this board. |
| insert(player) | Add a player with a globally unused active ID. |
| erase(player) | Remove the exact existing score and ID entry. |
| count_strictly_higher(score) | Count players whose scores exceed the argument. This must also work when the argument has no prepared Fenwick coordinate. |
| select(position) | Return the player at the one-based position. Require 1 <= position <= size(). |
| range(first, last, result) | Replace result with the inclusive ordered page. Require 1 <= first <= last <= size(). |
| split_by_score(threshold, high, low) | Empty the source into the supplied outputs. Scores >= threshold enter high; remaining scores enter low. |
| merge_from(other) | Combine both boards in the receiver and empty other. Equal scores across the inputs are allowed. Shared player IDs are invalid. |

Both split outputs must be empty, mutually distinct and different from the source. A merge requires two different boards of the same concrete type. Invalid operations must preserve the input boards.

The application enforces global ID uniqueness and supplies the old score with an ID for deletion. A score revision erases the old entry and inserts the new entry. An unchanged score is a successful no-op.

Backend insertion assumes the application has established global ID uniqueness. Membership is checked with both the ID and its current score. Validate a revised score's coordinate support before removing the old entry; preprocessing all ADD and SET scores makes supported replay revisions possible.

## Ordering and rank rules

Compare fields directly. Higher scores appear first, with smaller IDs first within a tied score.

~~~text
Player IDs      5    3    8    2
Scores        100   90   90   70
Positions       1    2    3    4
Ranks           1    2    2    4
~~~

Competition rank is one plus count_strictly_higher(playerScore). IDs 3 and 8 both have rank 2, while select(3) returns ID 8.

For AVL proofs, interpret a key preceding another key using this same descending-score, ascending-ID comparator. Keep that order consistent in ordered-join preconditions, key splits, the oracle and the application.

## Object ownership and basic usage

Construct concrete boards directly and pass them by reference.

~~~cpp
Fenwick board({70, 90, 100, 110});
board.insert({5, 100});
board.insert({3, 90});
board.insert({8, 90});
board.insert({2, 70});

vector<Player> result;
board.range(2, 3, result);

Fenwick high, low;
board.split_by_score(90, high, low);
// board is empty, high has three players, low has one.

high.merge_from(low);
// high has all four players and low is empty.
~~~

Construct an AVL board with `AVL board;`. The Fenwick constructor receives every score that may appear in the replay, including future revisions. It must reject insertion at an unprepared score before changing any count.

The application retires the source board ID after a split. After a merge it retires both input board IDs and assigns a new ID to the result. Retired IDs remain unavailable for reuse. Player scores and the global player lookup stay the same during split and merge.

## Inspection and ordered AVL primitives

The backend-tools header declares the concrete inspection functions and measurement records.

~~~cpp
vector<Player> audit_avl(const AVL& board);
vector<Player> audit_fenwick(const Fenwick& board);
AvlStats avl_stats(const AVL& board);
FenwickStats fenwick_stats(const Fenwick& board);
~~~

Audits verify internal invariants and return players in leaderboard order. AvlStats records height, rotations and last ordered-join visits. FenwickStats records coordinate count and ID-bucket shifts.

An AVL audit must recompute heights and subtree sizes from the children, verify balance factors in {-1, 0, 1}, verify total key order and reject repeated player IDs. A Fenwick audit must verify strictly ordered coordinates, sorted unique IDs, nonnegative counts, prefix counts and the total size.

An audit validates the complete board even when the experiment records only a root size or one query result. Keep audits outside timed query and update measurements.

The required ordered AVL primitives also use caller-owned outputs.

~~~cpp
void avl_split_by_key(AVL& source, Player key, AVL& before, AVL& after);
void avl_join_ordered(AVL& left, Player pivot, AVL& right, AVL& result);
~~~

For key split, before receives all keys preceding key in leaderboard order. After receives keys equal to or following it. Both outputs must be empty and distinct from the source and each other. The source becomes empty.

For ordered join, all left keys must precede pivot, and pivot must precede all right keys. Both inputs and the initially empty result must be distinct. The pivot ID must be unused. The inputs become empty. General league merging can contain overlapping score ranges and is measured separately from this ordered primitive.

## Changes assigned to each contributor

| Owner and role | Modules | Required migration |
|---|---|---|
| Prince — AVL | src/structA, AVL tests, D1 | Expose struct AVL, implement the member signatures, use reference outputs for splits and joins, and update inspection functions to accept const AVL&. Preserve ordering, heights, sizes, rotations and node ownership. |
| Saksham — Fenwick | src/structB, Fenwick tests, D2 | Expose struct Fenwick, retain compressed score counts and sorted ID buckets, return exact players from selection, use reference split outputs and merge through Fenwick&. |
| Madhav — Architect | app, shared interface, application tests, D3 | Update backend selection, board storage and replay calls to concrete structs. Update handwritten-table parameter types. Preserve parsing, global uniqueness, retired board IDs and error precedence. |
| Om — Oracle | oracle, differential tests, D4 | Make the sorted-array reference independent of Board, update split/merge calls and compare both concrete backends against the reference after every command. Preserve shrinking and boundary checks. |
| Bianca — Adversary | adversary, D5 | Rebuild the real application and both backends. Re-run deterministic traces, golden outputs, rotations, ties, churn, season operations and scale checks. Update direct backend calls if used. |
| Sumit — Analyst | bench, results, report, D6 | Update construction, split outputs, merges and inspection calls. Re-run timings on both backends with identical data and compiler settings, including ID-bucket work and rebuilding costs. |

### D1–D6 proof assignments from the handout

| Derivation | Owner | Required argument |
|---|---|---|
| D1 | Prince, Structure A | Ordered AVL join produces a valid AVL with correct sizes in O(1 + absolute height difference). Explain the taller-spine descent, balancing and metadata refresh. |
| D2 | Saksham, Structure B | Binary lifting returns the smallest coordinate whose prefix count is at least k in O(log U). State the alignment and remaining-count invariants and show how the remaining position selects the exact ID inside a tie. |
| D3 | Madhav, Architect | Competition rank is one plus the number of players with strictly higher scores, and both structures compute exactly that value. Explain why tied IDs change positions while sharing rank. |
| D4 | Om, Oracle | Subtree sizes stay correct after every single and double AVL rotation. Prove selection by subtree-size descent is correct and takes O(log n). |
| D5 | Bianca, Adversary | Key split, implemented through joins, costs O(log n) in total because the height changes telescope. Give an insertion order forcing rotations on a constant fraction of insertions. |
| D6 | Sumit, Analyst | Write the rank, select, split, ordered-join and rebuilding cost models before measuring. Include the simple implementation's ID-bucket and map/set work, then compare measured results with the predictions. |

Update API names and examples in each derivation to match the new signatures. The mathematical algorithm and complexity claims must match the actual code used for the new measurements.

### Structure A contributor checklist

1. Expose the full AVL struct definition and its node representation to callers.
2. Key the tree by descending score and ascending player ID.
3. Maintain both stored height and subtree size after insertion, deletion and every rotation. Refresh the demoted node before the promoted node.
4. Implement rank counting, size-based selection and output-sensitive range traversal.
5. Implement key split and ordered join with the exact output-reference signatures. Preserve their preconditions, output ownership and stated bounds.
6. Implement score split without dividing a tied-score group. Handle extreme score thresholds.
7. Implement general league merge for overlapping score ranges and report its actual union cost.
8. Update audits, counters, direct tests and D1. Coordinate rotation and selection evidence with the D4 owner and split evidence with the D5 owner.

### Structure B contributor checklist

1. Expose the full Fenwick struct definition with ordinary vectors and member functions.
2. Prepare unique descending coordinates from all possible replay scores.
3. Keep exact player membership in sorted ID buckets and maintain nonnegative Fenwick counts.
4. Count strictly higher scores with a prefix before the queried score. Support query values between prepared coordinates.
5. Select by binary lifting directly in O(log U), followed by the correct offset into the tied-ID vector.
6. Validate positions and ranges. Replace the range output vector's contents on a successful call.
7. Implement consuming score split and donor-emptying general merge through the supplied references. Rebuild all required counts and reject ID conflicts before transfer.
8. Update membership, ties, empty-board, coordinate and numeric-boundary tests; update D2 and the measured bucket-shift counter.

### Architect contributor checklist

1. Make complete concrete backend declarations visible to the application. Update construction and backend selection.
2. Store active boards safely. Moving an AVL board must transfer ownership of its nodes exactly once.
3. Keep the handwritten ID-to-score table and its global uniqueness checks.
4. Gather all ADD and SET scores before creating the initial Fenwick board, then execute commands in their original order.
5. Implement ID-based RANK as lookup, exact board-membership check and one plus count_strictly_higher.
6. Preserve score-revision, deletion, one-based position and inclusive-range semantics.
7. Preserve source consumption, permanent retirement of board IDs and duplicate destination checks during SPLIT and MERGE.
8. Update application tests and D3 while preserving the established replay syntax and outputs.

### Oracle contributor checklist

1. Adapt the C++ sorted-array reference to the new concrete calls and output references.
2. Sort by the exact shared comparator and implement competition rank independently.
3. Compare exact selected players and complete range contents, including tied IDs.
4. Compare split outputs with independent filtering and merge outputs with the sorted union of input players.
5. Verify invalid requests preserve state and outputs agree with the specified replay error rules.
6. Preserve deterministic seeds, failure traces and shrinking.
7. Test single and double rotations with a full recomputation of every subtree size. Complete D4's size and selection arguments.

### Adversary contributor checklist

1. Generate reproducible insert/delete sequences exercising the single and double AVL rotation cases.
2. Include a proven rotation-heavy insertion order. For example, distinct scores in decreasing order insert keys in increasing leaderboard order.
3. Run a full AVL audit after every operation in these rotation-focused sequences.
4. Join ordered inputs with very different heights, including an empty side when the ordered-join contract allows it.
5. Exercise all-tied scores, sparse coordinates, extreme scores, churn, empty split outputs and overlapping league scores.
6. Run the million-player, million-revision replay with interleaved queries and season operations.
7. State the audit frequency for each workload. Large phase-boundary checks complement the per-operation audits used in bounded adversarial tests.
8. Complete D5's telescoping split proof and constant-fraction rotation argument.

### Analyst contributor checklist

1. Write D6's cost models before collecting migrated-implementation timings.
2. Use the same prepared players, updates and queries for both structures.
3. Measure rank and select latency as n grows. Record U and tie-group size so their effects are visible.
4. Measure AVL key/score split against Fenwick division rebuilding.
5. Measure ordered AVL join with a supplied pivot separately from preparing that pivot and from arbitrary league merge.
6. Vary the ordered-join height difference and record join visits, rotations and bucket shifts.
7. Keep parsing, coordinate preprocessing, player lookup, console output and full audits outside backend-only timings. Label end-to-end timings separately.
8. Record the compiler, flags, machine, seeds, trial counts, timing units and workload sizes. Report variation across trials.
9. Re-run measurements after migration and explain gaps from the predictions. Attribute cache effects only when supported by profiling evidence.

### Application integration

Update every call site using the old abstract board or factories.

| Earlier call style | New call style |
|---|---|
| make_avl() | Construct `AVL board;` |
| make_fenwick(scores) | Construct `Fenwick board(scores);` |
| board->insert(player) | Call board.insert(player) when storing an object. |
| auto parts = board->split_by_score(threshold) | Create high and low, then call board.split_by_score(threshold, high, low). |
| parts.first->merge_from(*parts.second) | Call high.merge_from(low). |
| Inspection through const Board& | Pass const AVL& or const Fenwick& to the matching function. |

The application chooses a backend before replay and executes the same commands with that concrete structure. Complete struct declarations must be visible wherever objects are constructed. AVL boards own nodes; moving, storing or removing a board must preserve single ownership and cleanup.

The handwritten player table continues to map an ID to its current score. Check contains({id, oldScore}) before a board-specific mutation. Keep the table size equal to the sum of active board sizes.

Parse player and board IDs as unsigned decimal integers. Validate replay positions against board size before converting to signed long long, so very large input values cannot wrap into a valid position.

### Replay output compatibility

Keep ADD, SET, REMOVE, RANK, SELECT, RANGE, SIZE, SPLIT and MERGE. Preserve one output line per input line and the existing golden output files.

Successful mutations print OK. Query formats remain RANK value, PLAYER id score, PLAYERS count ... and SIZE value. Preserve the existing error codes and validation order, including source-board checks, repeated-board checks and destination-ID checks.

Board 0 starts empty. New destination board IDs must be distinct and permanently unused.

| Input line | Result |
|---|---|
| ADD board id score | Insert the player on the chosen board. |
| SET board id score | Revise that player's score on the chosen board. |
| REMOVE board id | Delete the player from that board. |
| RANK board id | Print the competition rank. |
| SELECT board position | Print the ID and score at that one-based position. |
| RANGE board first last | Print the complete inclusive page in leaderboard order. |
| SIZE board | Print its player count. |
| SPLIT board threshold high_board low_board | Consume the source and create the two divisions with new IDs. |
| MERGE left_board right_board result_board | Consume the two sources and create their union with a new ID. |

| Output | Meaning |
|---|---|
| OK | Successful mutation, including an unchanged score revision. |
| RANK value | Competition-rank query result. |
| PLAYER id score | One selected player. |
| PLAYERS count id score ... | Range-query result with every returned pair in order. |
| SIZE value | Board-size query result. |
| ERR BAD_COMMAND | Invalid command, token count, numeric syntax or numeric range. A blank input line is also invalid. |
| ERR UNKNOWN_BOARD | A required source board is absent or retired. |
| ERR BOARD_EXISTS | A destination ID has already been used. |
| ERR DUPLICATE_ID | ADD requests an ID active anywhere in the application. |
| ERR UNKNOWN_PLAYER | The ID is absent from the global player table. |
| ERR WRONG_BOARD | The player exists, but its exact key is absent from the requested board. |
| ERR BAD_POSITION | A syntactically valid position is outside the chosen board. |
| ERR BAD_RANGE | A syntactically valid range violates the range bounds. |
| ERR SAME_BOARD | A split or merge repeats any of its board arguments. |
| ERR ID_CONFLICT | A merge receives inputs with a shared player ID. Global uniqueness makes this unreachable during a valid replay. |

Validate syntax first, then required source boards. For split/merge, check repeated arguments before checking whether destination IDs were used. For other player operations, check global existence before exact board membership. Keep errors state-preserving.

The parser accepts nonnegative decimal IDs and positions, and signed decimal scores. A negative SELECT argument is a syntax error, while position zero is a valid integer token that fails the position bounds. Test overflow cases before converting numeric arguments to their backend types.

### Cost accounting

Fenwick count updates and selection remain logarithmic in the coordinate count. Sorted ID-vector insertion and deletion also include entries shifted inside a tie group. The simple map-based merge includes map/set operations, sorting combined buckets and rebuilding the count array.

The plain-struct example copies coordinate lists into split outputs. Include that setup and storage in measurements. Re-run benchmarks and update the report for the migrated implementation; previously recorded timings describe the earlier code.

## Validation scope

The matching standalone Fenwick example was tested separately with these declarations. The three-file interface package contains the common contract and this guide. Contributors supply the implementations and adapt their modules before running the whole-project checks below.

## Acceptance checks before merging

After updating the callers and build inputs, run these commands from a fresh checkout with a C++17 compiler available.

~~~powershell
python tools/project.py build
python tools/project.py --skip-build demo
python tools/project.py --skip-build test
python tools/project.py --skip-build adversary
python tools/project.py --skip-build bench
~~~

Then run the project-sized evaluation.

~~~powershell
python tools/project.py --full all
~~~

The build must include both concrete backends. Verify the basic, errors, seasons and wide-ID golden traces against the application and oracle. Preserve seeded differential checks, structural audits and scale checks.

The migration is ready to merge when the application, AVL, Fenwick, oracle, adversary and benchmarks all build and run together with these headers, and their observed outputs agree.

## Final handoff and acceptance record

Each contributor should report their commit, updated module files, matching derivation, exact validation commands and results. Record whether measurements include preprocessing, application bookkeeping, output or audits.

The shared acceptance record should contain:

- A fresh build of all application, oracle, test and benchmark executables using the new headers.
- Matching golden output for both real backends and the sorted-array oracle.
- Exact agreement on competition ranks, selected IDs, positions 40–50, threshold divisions and league unions.
- Full-tree audits after every operation in the rotation-focused adversarial runs.
- Boundary checks for empty boards, ties, invalid operations, signed scores and wide IDs.
- One million active players and one million score revisions with queries and season operations.
- Completed D1–D6 derivations matching the migrated code.
- New comparison results for rank/select growth and AVL split/join against Fenwick rebuilding, with an explanation of prediction gaps.

The interface design supports these obligations. Completion of the implementation, proofs and evaluation must be demonstrated by the corresponding owners before submission.
