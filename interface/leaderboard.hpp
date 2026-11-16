#pragma once

// Scores can be negative. Player IDs are nonnegative.
struct Player {
    unsigned long long id;
    long long score;
};

// Each structure's implementation defines its data and member functions.
struct AVL;
struct Fenwick;
