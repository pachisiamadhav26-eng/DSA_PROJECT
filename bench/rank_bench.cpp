#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <numeric>
#include <stdexcept>
#include <unordered_map>
#include <vector>

// The branch sources are standalone demos. Keep their names separate and
// rename their entry points so this file can call both structures directly.
#define main demo_main
namespace avl_demo {
#include "avl.cpp"
}
namespace fenwick_demo {
#include "Fenvick.cpp"
}
#undef main

struct RankQuery {
    int playerId;
    long long score;
    int expected;
};

constexpr std::array<RankQuery, 5> queries{{
    {1, 100, 1}, {12, 90, 11}, {20, 90, 11}, {56, 50, 51}, {100, 10, 91}
}};

template <typename RankFunction>
void timeRank(const char* name, RankFunction rank) {
    using Clock = std::chrono::steady_clock;
    constexpr int batches = 2'000'000;
    std::array<double, 5> nsPerCall{};
    long long checksum = 0;

    for (double& timing : nsPerCall) {
        const auto start = Clock::now();
        for (int batch = 0; batch < batches; ++batch) {
            for (const RankQuery& query : queries) {
                checksum += rank(query.score);
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
    std::vector<long long> scores;
    for (int id = 1; id <= 100; ++id) {
        scores.push_back(100 - 10 * ((id - 1) / 10));
    }

    avl_demo::AVL avl;
    avl_demo::Node* root = nullptr;
    fenwick_demo::Fenwick fenwick(scores);
    for (int id = 1; id <= 100; ++id) {
        using AvlId = decltype(avl_demo::Player{}.id);
        root = avl.insert(root, {static_cast<AvlId>(id), scores[id - 1]});
        fenwick.insert({id, scores[id - 1]});
    }

    std::cout << "Players: " << scores.size() << ", distinct scores: " << fenwick.scores.size()
              << "\nPlayer\tScore\tExpected\tAVL\tFenwick\n";
    for (const RankQuery& query : queries) {
        const int avlRank = avl.rank(root, query.score);
        const int fenwickRank = fenwick.rank(query.score);
        std::cout << query.playerId << '\t' << query.score << '\t' << query.expected
                  << "\t\t" << avlRank << '\t' << fenwickRank << '\n';
        if (avlRank != query.expected || fenwickRank != query.expected) {
            std::cerr << "Rank check failed for player " << query.playerId << '\n';
            avl.destroy(root);
            return 1;
        }
    }

    timeRank("AVL", [&](long long score) { return avl.rank(root, score); });
    timeRank("Fenwick", [&](long long score) { return fenwick.rank(score); });
    avl.destroy(root);
}
