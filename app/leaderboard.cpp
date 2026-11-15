#include "replay.hpp"
#include "backend_tools.hpp"

#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);
    bool check = argc == 4 && std::string(argv[3]) == "--audit";
    if (argc != 3 && !check) {
        std::cerr << "usage: leaderboard <avl|fenwick> <trace-file> [--audit]\n";
        return 2;
    }

    std::string backend = argv[1];
    leaderboard::BoardFactory factory;
    leaderboard::BoardAudit audit = nullptr;
    if (backend == "avl") {
        factory = [](const std::vector<leaderboard::Score>&) {
            return leaderboard::make_avl();
        };
        if (check) audit = leaderboard::audit_avl;
    } else if (backend == "fenwick") {
        factory = [](const std::vector<leaderboard::Score>& scores) {
            return leaderboard::make_fenwick(scores);
        };
        if (check) audit = leaderboard::audit_fenwick;
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
        leaderboard::run_replay(input, std::cout, factory, audit);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
