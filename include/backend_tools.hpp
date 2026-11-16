#pragma once

#include "leaderboard.hpp"

namespace leaderboard {

// Structural inspection and the two AVL primitives used by tests and benchmarks.
struct FenwickStats {
    std::size_t coordinates;
    std::uint64_t bucket_shifts;
};

int avl_height(const Board& board);
std::vector<Player> audit_avl(const Board& board);
std::vector<Player> audit_fenwick(const Board& board);
FenwickStats fenwick_stats(const Board& board);

// Returns the keys preceding key, then the keys equal to or following key.
std::pair<std::unique_ptr<Board>, std::unique_ptr<Board>>
avl_split_by_key(Board& source, Player key);

// Caller guarantees left < pivot < right and globally distinct IDs.
// Both sources are emptied. This primitive performs no boundary searches.
std::unique_ptr<Board> avl_join_ordered(Board& left, Player pivot, Board& right);

}
