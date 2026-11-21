#include "replay.hpp"
#include <bits/stdc++.h>

using namespace std;
using namespace leaderboard;

unique_ptr<Board> new_avl_board(const vector<Score>&) {
    return make_avl();
}

unique_ptr<Board> new_fenwick_board(const vector<Score>& scores) {
    return make_fenwick(scores);
}

int main(int argc, char** argv) {
    if (argc != 3) {
        cerr << "usage: leaderboard <avl|fenwick> <trace-file>\n";
        return 2;
    }

    string backend = argv[1];
    BoardFactory factory;
    if (backend == "avl") {
        factory = new_avl_board;
    } else if (backend == "fenwick") {
        factory = new_fenwick_board;
    } else {
        cerr << "backend must be avl or fenwick\n";
        return 2;
    }

    ifstream input(argv[2]);
    if (!input) {
        cerr << "cannot open trace file\n";
        return 2;
    }

    try {
        run_replay(input, cout, factory);
    } catch (const exception& error) {
        cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
