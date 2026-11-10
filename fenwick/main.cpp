#include "fenwick_leaderboard.hpp"
#include <iostream>

using namespace leaderboard;

int main() {
    FenwickLeaderboard board({40, 70, 90, 100});
    for (Player p : std::vector<Player>{{1,100}, {2,90}, {3,90}, {4,70}, {5,40}})
        board.insert(p.id, p.score);

    std::cout << "Initial leaderboard (position, ID, score, rank)\n";
    for (Position k = 1; k <= board.size(); ++k) {
        const Player p = board.select(k);
        std::cout << k << "  " << p.id << "  " << p.score << "  " << board.rank(p.id) << '\n';
    }

    board.update(4, 100);
    board.erase(5);
    std::cout << "After ID 4 moves to 100 and ID 5 leaves, positions 2..4 contain IDs";
    for (Player p : board.range(2, 4)) std::cout << ' ' << p.id;
    std::cout << '\n';

    auto high = board.split_off(100);
    std::cout << "Split at 100: " << board.size() << " below, " << high.size() << " at or above\n";
    board.merge_from(high);
    board.audit();
    std::cout << "Merged size: " << board.size() << ", donor size: " << high.size() << '\n';
}
