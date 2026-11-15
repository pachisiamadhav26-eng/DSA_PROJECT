// From the repo root: g++ -std=c++17 -Iinclude Fenvick.cpp src/structB/fenwick.cpp -o fenwick-demo
#include "leaderboard.hpp"
#include <iostream>

int main() {
    using leaderboard::Player;
    auto board = leaderboard::make_fenwick({70, 90, 100, 110});
    for (Player p : std::vector<Player>{{5, 100}, {3, 90}, {8, 90}, {2, 70}})
        board->insert(p);

    std::cout << "Rank of ID 8: " << 1 + board->count_strictly_higher(90) << '\n';
    std::cout << "ID at position 3: " << board->select(3).id << '\n';
    std::vector<Player> page;
    board->range(2, 3, page);
    std::cout << "Positions 2 to 3:";
    for (Player p : page) std::cout << ' ' << p.id;
    std::cout << '\n';

    auto [high, low] = board->split_by_score(90);
    std::cout << "Split sizes (high, low, source): " << high->size() << ' '
              << low->size() << ' ' << board->size() << '\n';
    high->merge_from(*low);
    high->erase({8, 90});
    high->insert({8, 110});
    high->range(1, high->size(), page);
    std::cout << "After revising ID 8 to 110:";
    for (Player p : page) std::cout << ' ' << p.id;
    std::cout << '\n';
}
