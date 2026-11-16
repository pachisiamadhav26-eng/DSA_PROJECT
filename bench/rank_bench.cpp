#include <algorithm>
#include <array>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <vector>

#include "backend_tools.hpp"

struct RankQuery {
    int playerId;
    long long score;
    int expected;
};

constexpr std::array<RankQuery, 5> queries{{
    {1, 100, 1}, {12, 90, 11}, {20, 90, 11}, {56, 50, 51}, {100, 10, 91}
}};

void timeRank(const char* name, const leaderboard::Board& board) {
    using Clock = std::chrono::steady_clock;
    constexpr int batches = 2'000'000;
    std::array<double, 5> nsPerCall{};
    long long checksum = 0;

    for (double& timing : nsPerCall) {
        const auto start = Clock::now();
        for (int batch = 0; batch < batches; batch++) {
            for (const RankQuery& query : queries) {
                checksum += 1 + board.count_strictly_higher(query.score);
            }
        }
        const auto stop = Clock::now();
        timing = std::chrono::duration<double, std::nano>(stop - start).count()
               / (batches * queries.size());
    }

    const auto spread = std::minmax_element(nsPerCall.begin(), nsPerCall.end());
    const double mean = std::accumulate(nsPerCall.begin(), nsPerCall.end(), 0.0)
                      / nsPerCall.size();
    std::cout << std::fixed << std::setprecision(2)
              << name << ": " << mean << " ns/rank (min " << *spread.first
              << ", max " << *spread.second << ", checksum " << checksum << ")\n";
}

int main() {
    std::vector<leaderboard::Score> scores;
    for (int id = 1; id <= 100; id++) {
        scores.push_back(100 - 10 * ((id - 1) / 10));
    }

    auto avl = leaderboard::make_avl();
    auto fenwick = leaderboard::make_fenwick(scores);
    for (int id = 1; id <= 100; id++) {
        avl->insert({static_cast<leaderboard::PlayerId>(id), scores[id - 1]});
        fenwick->insert({static_cast<leaderboard::PlayerId>(id), scores[id - 1]});
    }

    std::cout << "Players: " << scores.size() << ", distinct scores: " << leaderboard::fenwick_stats(*fenwick).coordinates
              << "\nPlayer\tScore\tExpected\tAVL\tFenwick\n";
    for (const RankQuery& query : queries) {
        const int avlRank = 1 + avl->count_strictly_higher(query.score);
        const int fenwickRank = 1 + fenwick->count_strictly_higher(query.score);
        std::cout << query.playerId << '\t' << query.score << '\t' << query.expected
                  << "\t\t" << avlRank << '\t' << fenwickRank << '\n';
        if (avlRank != query.expected || fenwickRank != query.expected) {
            std::cerr << "Rank check failed for player " << query.playerId << '\n';
            return 1;
        }
    }

    timeRank("AVL", *avl);
    timeRank("Fenwick", *fenwick);
}
