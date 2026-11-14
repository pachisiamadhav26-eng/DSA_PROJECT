#include "replay.hpp"

#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: leaderboard <avl|fenwick> <trace-file>\n";
        return 2;
    }

    std::string backend = argv[1];
    leaderboard::BoardFactory factory;
    if (backend == "avl") {
        factory = [](const std::vector<leaderboard::Score>&) {
            return leaderboard::make_avl();
        };
    } else if (backend == "fenwick") {
        factory = [](const std::vector<leaderboard::Score>& scores) {
            return leaderboard::make_fenwick(scores);
        };
    } else {
        std::cerr << "backend must be avl or fenwick\n";
        return 2;
    }

    std::ifstream input(argv[2]);
    if (!input) {
        std::cerr << "cannot open trace file\n";
        return 2;
    }

    try {
        leaderboard::run_replay(input, std::cout, factory);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
