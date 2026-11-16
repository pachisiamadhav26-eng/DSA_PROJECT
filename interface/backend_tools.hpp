#pragma once

#include "leaderboard.hpp"
#include <vector>
using namespace std;

struct AvlStats {
    int height = 0;
    unsigned long long rotations = 0;
    unsigned long long last_join_visits = 0;
};

struct FenwickStats {
    long long coordinates = 0;
    unsigned long long bucket_shifts = 0;
};

// Check the structure and return its players in leaderboard order.
vector<Player> audit_avl(const AVL& board);
vector<Player> audit_fenwick(const Fenwick& board);

// Return measurements recorded by each structure.
AvlStats avl_stats(const AVL& board);
FenwickStats fenwick_stats(const Fenwick& board);

// Empty source into two empty output boards.
// before contains keys preceding key; after contains keys equal to or following key.
void avl_split_by_key(AVL& source, Player key, AVL& before, AVL& after);

// Combine ordered inputs into an empty result board, then empty both inputs.
// All left keys must precede pivot, and pivot must precede all right keys.
void avl_join_ordered(AVL& left, Player pivot, AVL& right, AVL& result);
