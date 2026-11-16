# Leaderboard interface and invariants

This is the agreement between the AVL tree, Fenwick tree, replay application, and sorted-array oracle. They may use different code internally, but the same command must give the same answer in all of them. That lets us run one trace against each implementation and compare the answers. Standalone demos may use different function names; the code connected to the application must follow this interface. If we change a rule here, we must update all four parts.

The main rules are simple: higher scores come first; for equal scores, smaller IDs come first; equal scores share a competition rank. A board means one league or division. The application keeps each player's current score in its own hash table and asks the chosen board to do rank, position, split, and merge operations.

## 1. Language and data model

The project uses C++17. These are the shared types in `include/leaderboard.hpp`:

```cpp
using PlayerId = std::uint64_t;
using Score = std::int64_t;
using Position = std::uint64_t;

struct Player {
    PlayerId id;
    Score score;
};
```

The header puts these names inside `namespace leaderboard`. This is only a name group: `leaderboard::Player` means the `Player` type from this project. It is not another data structure.

The ID and position types are unsigned because neither is negative. `Score` is signed because scores may be negative. An ID can belong to only one active board at a time. Positions start at 1, while the size of an empty board is 0. The application rejects position 0 even though an unsigned type can hold it.

## 2. Ordering, rank, and position

To display players, put higher scores first. If two players have the same score, put the smaller ID first. The ID rule makes `select` and `range` predictable even when scores tie. In code, compare the fields directly. Subtracting two scores or IDs to compare them could overflow.

**Rank and position are different.** Competition rank is `1 + number of players with a strictly higher score`. Players with equal scores share a rank. Position is a player's place in the displayed list, so even tied players have different positions. `select(k)` returns the player at position `k`.

For example, scores `100, 90, 90, 70` have ranks `1, 2, 2, 4`. The two players on `90` both have rank 2, but one appears before the other in the list.

For `RANK board id`, the application finds the player's current score and asks that board for `count_strictly_higher(score)`. It adds 1 to get the rank. Rank is within the requested board, so a split can change a player's rank. AVL uses subtree sizes to find the higher count. Fenwick stores score buckets from highest to lowest, so the buckets before this score contain the strictly higher players.

## 3. What a user can ask

These are the text commands a user can request. The functions the two structures implement are listed in section 4.

| Command | What it does | Invalid input |
| --- | --- | --- |
| `SIZE board` | Count active players. | Unknown board. |
| `ADD board id score` | Add a player. | ID already active on any board. |
| `REMOVE board id` | Remove a player. | ID absent or on another board. |
| `SET board id new_score` | Change a score; same score is a no-op. | ID absent or on another board. |
| `RANK board id` | Get one-based competition rank. | ID absent or on another board. |
| `SELECT board position` | Get the player at that one-based position. | Position outside `1..size`. |
| `RANGE board first last` | List positions `first` through `last`, inclusive. | `first < 1`, `first > last`, or `last > size`. |
| `SPLIT board threshold high_board low_board` | Scores `>= threshold` go high; the rest go low. | Unknown source or invalid new board IDs. |
| `MERGE left_board right_board result_board` | Combine both leagues, including overlapping scores. | Unknown or identical sources, invalid result ID, or duplicate player IDs. |

Every command that names a board needs that board to exist. The new board IDs in `SPLIT` and `MERGE` must not already exist.

After a split, the old board ID no longer exists; the two new board IDs do. After a merge, the two old board IDs no longer exist; the result board ID does. A valid split may produce an empty board. An error must leave the state unchanged.

## 4. What each board must implement

The application has a handwritten hash table that maps each player ID to its current score. It uses the old score when removing or revising a player. The table also stops the same ID from being added twice.

This division of work is deliberate. The hash table finds a player by ID; AVL or Fenwick keeps the scores ordered for rank and position queries.

A player may move to another board after a split or merge. `contains(Player)` checks which board has that player. The hash table does not need to change during those board operations because the player's score and ID stay the same.

Both structures implement these functions from `Board`:

| Board function | Required behavior |
| --- | --- |
| `size()` | Return the number of players on this board. |
| `contains(Player)` | Report whether the exact player key is on this board. |
| `insert(Player)` | Insert a player whose ID is not already active. |
| `erase(Player)` | Remove the exact existing `(score, id)` entry. |
| `count_strictly_higher(Score)` | Count players whose score is greater than the argument. |
| `select(Position)` | Return the player at that position, including the agreed ID order within a tie. |
| `range(Position, Position)` | Return the inclusive ordered page of players. |
| `split_by_score(Score)` | Produce the high and low divisions without losing or duplicating players. |
| `merge_from(Board&)` | Add all players from another board, even when the score ranges overlap; IDs must be distinct. |

The AVL implementation also needs two tree operations from the project brief. They are AVL-only, not methods of the common `Board` interface:

- `split_by_key(T, k)` puts keys before `k` on one side, and `k` and later keys on the other.
- `join_ordered(T1, k, T2)` combines two trees and a pivot only when every key in `T1` comes before `k`, and `k` comes before every key in `T2`.

A league merge may have overlapping scores, so it cannot automatically use the fast ordered-join bound. Fenwick handles split and merge by rebuilding its index.

Scores can be large or negative, so Fenwick maps each distinct possible score to a small array index. This is coordinate compression.

A score bucket is the group of players with one score. Fenwick counts can locate a bucket, but cannot choose an individual player when several have that score. Structure B therefore keeps IDs in order within each tied-score bucket. Its `select` result must match AVL and the oracle.

The replay gathers possible scores before it starts. If a score was not prepared, the Fenwick board must handle that case explicitly rather than give a wrong answer.

## 5. Rules that must always stay true

These are invariants: each part must preserve them after every successful change.

- **Application and hash table:** each active ID has one entry with its current score, and each entry matches one player on one active board. The hash-table size equals the total size of all active boards. Before changing a particular board, the application checks that the player is on it.
- **AVL:** players follow the display order. A node's stored height is `1 + max(left height, right height)`. Its balance factor, `left height - right height`, is `-1`, `0`, or `1`. Its stored size is `1 + left size + right size`. Rotations, deletion, split, and join must keep these values correct.
- **Fenwick:** a score bucket counts exactly the active players with that score and cannot have a negative count. Fenwick prefix sums and total size must match those counts. Compressed coordinates keep their score order, and IDs within a tied-score bucket stay sorted without duplicates.
- **Split and merge:** the two split boards have no player in common, and together they contain exactly the old board's players. A merge contains exactly the players of both input boards. Every result keeps the required order and structure invariants.
- **Oracle:** its sorted array uses the same order and unique-ID rule. We use it to check answers, not as the fast structure under study.

## 6. Worked example shared by all implementations

Start with `(id=5, score=100)`, `(id=3, score=90)`, `(id=8, score=90)`, and `(id=2, score=70)`.

| Query | Expected result |
| --- | --- |
| Full list | IDs `5, 3, 8, 2` |
| Ranks of IDs `5, 3, 8, 2` | `1, 2, 2, 4` |
| `select(3)` | `(id=8, score=90)` |
| `range(2, 3)` | IDs `3, 8` |
| `split_by_score(90)` | High IDs `5, 3, 8`; low ID `2` |

If that split creates boards `1` and `2`, board `0` disappears. Player 2 now has rank 1 on the low board, rather than rank 4 before the split. `RANK 0 5` gives `ERR UNKNOWN_BOARD`; `RANK 2 5` gives `ERR WRONG_BOARD` because player 5 is on board `1`.

If we merge boards `1` and `2` back into board `3`, then revise ID `8` to score `110`, the list is `8, 5, 3, 2`. Their ranks in that order are `1, 2, 3, 4`.

## 7. Cost expectations and scope

Let `n` be the number of players, `U` the number of distinct compressed scores, and `r` the number of players returned by a range query. Big-O describes how the work grows as these numbers grow, not an exact running time in milliseconds.

| Work | Expected cost | Why |
| --- | --- | --- |
| AVL add, remove, rank, or select | `O(log n)` | The tree stays balanced. |
| AVL split | `O(log n)` | The required tree algorithm uses joins while preserving heights and sizes. |
| AVL ordered join with pivot `k` | `O(height difference + 1)` | Cost depends on the difference between the input heights. |
| AVL range | `O(log n + r)` | Find the start, then output `r` players. |
| Fenwick rank or score-bucket select | `O(log U)` | Prefix sums and binary lifting visit logarithmically many coordinates. |

If the two AVL heights are `h1` and `h2`, the precise ordered-join bound is `O(|h1 - h2| + 1)`. These are targets for structure operations, not a promise about every whole command.

Fenwick also maintains sorted IDs within a score bucket, and split and merge rebuild its index. Include those costs in measurements. A general league merge can mix score ranges, so measure it separately from an ordered AVL join.

Both structures and the oracle use C++. A library container must not replace the AVL, Fenwick, or required handwritten player hash table in the final application.

## 8. How the application replays commands

The application chooses either the AVL or Fenwick implementation of `Board`; this chosen implementation is the backend. A factory is a function that creates a new empty board. Fenwick needs to know the possible scores first, so the replay collects scores from `ADD` and `SET` before it creates the Fenwick board and runs the commands.

The application checks ordinary user errors before calling a board. When it calls `erase`, it gives the board both the ID and the old score from the hash table. A player missing from the hash table is unknown; a player in the hash table but missing from the requested board is on the wrong board.

The backend methods agree on these details:

- `select` receives a one-based position.
- `range` clears its output vector and fills it in display order.
- `split_by_score` returns `{high, low}` and leaves the source board empty.
- `merge_from` accepts a board of the same backend type and leaves that source empty on success.
- A backend may reject a broken precondition, but it must not silently return a wrong answer.

The trace is a UTF-8 text file with one whitespace-separated command per line. Board `0` starts empty. Board IDs and player IDs are nonnegative decimal integers; scores are signed decimal integers. New board IDs in `SPLIT` or `MERGE` must be unused and different from the source board IDs. The two new IDs in a split must also differ from each other. After a successful split or merge, the source board IDs are no longer valid.

Every command produces one output line:

| Case | Output |
| --- | --- |
| Successful add, set, remove, split, or merge | `OK` |
| Rank query | `RANK value` |
| Select query | `PLAYER id score` |
| Range query | `PLAYERS count id score ...` |
| Size query | `SIZE value` |
| Invalid command or operation | `ERR code` |

An error leaves the state unchanged. These are the error codes:

- Bad command or number: `BAD_COMMAND`.
- Board IDs: `UNKNOWN_BOARD`, `BOARD_EXISTS`, `SAME_BOARD` (two IDs that must differ are equal).
- Player IDs: `DUPLICATE_ID`, `UNKNOWN_PLAYER`, `WRONG_BOARD`, `ID_CONFLICT` (reserved for duplicate IDs across boards during merge; global uniqueness prevents this in a valid replay).
- Query positions: `BAD_POSITION`, `BAD_RANGE`.

`app/examples/basic.trace` is a valid replay. Its complete expected output is `app/examples/basic.expected`. Both backends and the oracle must produce those same twelve output lines.
