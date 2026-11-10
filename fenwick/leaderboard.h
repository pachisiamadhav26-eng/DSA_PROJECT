#pragma once
#include "../player_table.h"

struct FenwickTree {
    std::vector<int> bit;  // index 0 is unused
};

struct Board {
    FenwickTree tree;
    std::vector<long long> scores;             // sorted from lowest to highest
    std::vector<std::vector<long long>> ids;  // sorted player IDs at each score
    PlayerTable players;
};

void add(FenwickTree& tree, int i, int delta);
int prefixSum(const FenwickTree& tree, int i);
int findKth(const FenwickTree& tree, int k);

// Start with an empty board. Fenwick needs all possible scores before insertion.
void initBoard(Board& board, const std::vector<long long>& scores);
bool insertPlayer(Board& board, long long id, long long score);
bool erasePlayer(Board& board, long long id);
bool updateScore(Board& board, long long id, long long newScore);
int rankOf(Board& board, long long id);  // zero means unknown ID
bool selectPlayer(const Board& board, int position, Player& answer);
std::vector<Player> listPositions(const Board& board, int first, int last);
bool splitBoard(Board& board, long long threshold, Board& high); // high must be empty
bool mergeBoards(Board& board, Board& other); // empties other on success
void clearBoard(Board& board);
