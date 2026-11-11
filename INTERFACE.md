# Leaderboard interface and invariants

This document is the common behavior contract for the AVL implementation (A), Fenwick implementation (B), application, and sorted-array oracle. All four must produce the same observable results. The current standalone examples may use different function names; their application adapters must follow this contract. Changes to the contract must be applied to all four parts together.

## 1. Language and data model

The project uses C++17, matching the current repository build instructions. The shared declarations are in `include/leaderboard.hpp`; the logical types are:

```cpp
using PlayerId = std::uint64_t;
using Score = std::int64_t;
using Position = std::uint64_t;

struct Player {
    PlayerId id;
    Score score;
};
```

Player IDs are unique among all active boards that may later be merged. Scores may be negative. Positions are one-based. An empty board has size zero.

## 2. Ordering, rank, and position

Player `a` comes before player `b` when `a.score > b.score`, or when their scores are equal and `a.id < b.id`. The implementation must compare fields directly rather than subtracting scores or IDs, which could overflow.

The **competition rank** of an active player is `1 + count of active players with a strictly higher score`. Tied scores share a rank. The **position** is the unique index in the total order above; tied players have different positions. `select(k)` returns the player in position `k`.

The application's `rank(id)` looks up the current score and computes `1 + count_strictly_higher(score)`. For an AVL tree, the higher count comes from subtree sizes. For a Fenwick tree, it comes from score counts before the player's score in descending score order.

## 3. Observable operations

The application exposes the following logical operations. The backend signatures are in `include/leaderboard.hpp`, and the replay-file syntax is in section 8.

| Operation | Successful result | Invalid input |
| --- | --- | --- |
| `size(board)` | Number of active players. | Unknown board is an error. |
| `add(board, id, score)` | Add one new player. | An ID already active on any board is an error. |
| `remove(board, id)` | Remove that player. | An absent ID or wrong board is an error. |
| `revise(board, id, new_score)` | Replace the old score with the new score; an unchanged score is a no-op. | An absent ID or wrong board is an error. |
| `rank(board, id)` | One-based competition rank. | An absent ID or wrong board is an error. |
| `select(board, k)` | Player at one-based position `k`. | `k < 1` or `k > size(board)` is an error. |
| `range(board, first, last)` | Players in positions `first` through `last`, inclusive and in list order. | `first < 1`, `first > last`, or `last > size(board)` is an error. |
| `split_by_score(board, threshold)` | A high board with `score >= threshold` and a low board with `score < threshold`. | Unknown board is an error. |
| `merge_leagues(left, right)` | One board containing both inputs, ordered by the same rule. Equal scores may occur in both inputs. | Unknown board, identical input boards, or duplicate player IDs are errors. |

Successful split consumes its source board. Successful merge consumes its two source boards. If an operation reports an error, it must not silently change a board. An empty result from a valid split is allowed.

## 4. Structure-facing operations

The application owns the handwritten hash table from `PlayerId` to current `Score` and board identity. It enforces ID uniqueness and supplies the exact `(score, id)` key to a structure for deletion. Each structure must support these logical primitives:

| Primitive | Contract |
| --- | --- |
| `insert(Player)` | Insert a player whose ID is not already active. |
| `erase(Player)` | Remove the exact existing `(score, id)` entry. |
| `count_strictly_higher(Score)` | Count players whose score is greater than the argument. |
| `select(Position)` | Return the player at that position, including the agreed ID order within a tie. |
| `range(Position, Position)` | Return the inclusive ordered page of players. |
| `split_by_score(Score)` | Produce the high and low divisions without losing or duplicating players. |
| `merge_leagues(board, board)` | Combine arbitrary score ranges with globally distinct IDs. |

AVL additionally exposes `split_by_key(T, k)` and `join_ordered(T1, k, T2)` as the ordered tree primitives required by the project. `split_by_key` places keys preceding `k` on one side and keys equal to or following `k` on the other. `join_ordered` requires every key in `T1` to precede `k` and `k` to precede every key in `T2`. A general league merge may contain overlapping score ranges and cannot claim the ordered-join bound. Fenwick split and join rebuild their indexes.

The Fenwick tree stores counts over compressed scores. Its score counts alone identify a score bucket, not a particular player tied at that score. The current Structure B design keeps IDs sorted within each score bucket; selection must use the bucket offset and return the same player as AVL and the oracle. The replay can collect score coordinates before execution; behavior for a score not in the prepared coordinates must be defined by B's adapter rather than silently producing a wrong result.

## 5. Structural and cross-implementation invariants

- **Application/hash:** every active ID has exactly one hash entry containing its current board and score; every hash entry has exactly one matching structure entry. The hash and structure sizes agree after each successful operation.
- **AVL:** keys follow the total order; each stored height equals `1 + max(child heights)`; each balance factor is in `{-1, 0, 1}`; each stored subtree size equals `1 + left size + right size`. These facts must remain true after rotations, deletion, split, and join.
- **Fenwick:** every count is nonnegative and equals the number of live players at its score; prefix sums and total size agree with those counts; compressed coordinates have consistent score order; IDs inside a tied-score bucket are sorted and occur exactly once.
- **Split/merge:** the outputs of a split are disjoint and their union equals the input. A successful merge contains exactly the union of both inputs. All outputs remain ordered and satisfy their structure invariants.
- **Oracle:** its array uses the same total order and unique-ID rule. It is a correctness reference, not a performance substitute for A or B.

## 6. Worked example shared by all implementations

Start with `(id=5, score=100)`, `(id=3, score=90)`, `(id=8, score=90)`, and `(id=2, score=70)`.

| Query | Expected result |
| --- | --- |
| Full list | IDs `5, 3, 8, 2` |
| Ranks of IDs `5, 3, 8, 2` | `1, 2, 2, 4` |
| `select(3)` | `(id=8, score=90)` |
| `range(2, 3)` | IDs `3, 8` |
| `split_by_score(90)` | High IDs `5, 3, 8`; low ID `2` |

After revising ID `8` to score `110`, the list is `8, 5, 3, 2` and the ranks are `1, 2, 3, 4` in that list order.

## 7. Cost expectations and scope

AVL targets logarithmic insertion, deletion, rank, select, and split; ordered join with supplied pivot targets `O(|h1 - h2| + 1)`. Returning `r` players requires at least `O(r)` output work, so AVL range extraction is stated as `O(log n + r)`. Fenwick rank and score-bucket selection target `O(log U)` for `U` compressed scores; ID-bucket maintenance and full rebuild costs must be included in measurements. General league merge is measured separately from ordered AVL join.

Both structures and the oracle use C++. A library container must not replace the AVL, Fenwick, or required handwritten player hash table in the final application.

## 8. C++ adapter and replay format

The `Board` interface in `include/leaderboard.hpp` is implemented by one adapter for each structure. The application selects one backend for a replay. Both factories return a new empty board. The Fenwick factory receives every score that can appear in the replay; the application gathers scores from `ADD` and `SET` commands before running it.

The application checks normal user errors before calling a backend. It supplies the exact `Player` value to `erase`, including the old score from its handwritten hash table. `select` uses one-based positions. `range` clears its output vector and fills it in leaderboard order. `split_by_score` returns `{high, low}` and leaves its source empty. `merge_from` accepts only another board of the same backend type and leaves that board empty on success. Backend methods may reject a violated precondition; they must not silently return a wrong answer.

The replay is UTF-8 text with one whitespace-separated command per line. Board `0` exists and is empty at the start. Board IDs and player IDs are nonnegative decimal integers. Scores are signed decimal integers. New board IDs supplied to `SPLIT` or `MERGE` must be unused and distinct from each other and from the source IDs. A successful split or merge invalidates its source board IDs.

| Command | Effect |
| --- | --- |
| `ADD board id score` | Add a new player. |
| `SET board id new_score` | Revise an existing player's score. |
| `REMOVE board id` | Remove a player. |
| `RANK board id` | Query competition rank. |
| `SELECT board position` | Query one player by position. |
| `RANGE board first last` | Query an inclusive position range. |
| `SIZE board` | Query the active player count. |
| `SPLIT board threshold high_board low_board` | Create high and low divisions. |
| `MERGE left_board right_board result_board` | Combine two leagues with distinct IDs. |

Every input command produces exactly one output line. Successful mutations print `OK`. Queries print `RANK value`, `PLAYER id score`, `PLAYERS count id score ...`, or `SIZE value`. Invalid input prints `ERR code` and leaves the state unchanged. Error codes are `UNKNOWN_BOARD`, `BOARD_EXISTS`, `DUPLICATE_ID`, `UNKNOWN_PLAYER`, `WRONG_BOARD`, `BAD_POSITION`, `BAD_RANGE`, `SAME_BOARD`, and `ID_CONFLICT`. Parsing errors print `ERR BAD_COMMAND`.

`app/examples/basic.trace` is a valid replay. Its complete expected output is `app/examples/basic.expected`. Both backends and the oracle must produce those same twelve output lines.
