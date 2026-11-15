#include <algorithm>
#include <array>
#include <chrono>
#include <functional>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#define main demo_main
namespace avl_demo {
#include "avl.cpp"
}
namespace fenwick_demo {
#include "Fenvick.cpp"
}
#undef main

using Clock = std::chrono::steady_clock;

struct SelectQuery {
    int position;
    int expectedId;
    long long expectedScore;
};

const std::array<SelectQuery, 5> queries{{
    {1, 1, 100}, {12, 12, 90}, {20, 20, 90}, {56, 56, 50}, {100, 100, 10}
}};

void printTimings(const char* name, const std::array<double, 5>& timings) {
    const auto spread = std::minmax_element(timings.begin(), timings.end());
    const double mean = std::accumulate(timings.begin(), timings.end(), 0.0)
                      / timings.size();
    std::cout << std::fixed << std::setprecision(2) << name << ": " << mean
              << " ns/call (min " << *spread.first << ", max " << *spread.second << ")\n";
}

template <typename SelectFunction>
void benchmarkSelect(const char* name, SelectFunction select) {
    constexpr int batches = 2'000'000;
    std::array<double, 5> timings{};
    long long checksum = 0;
    for (double& timing : timings) {
        const auto start = Clock::now();
        for (int batch = 0; batch < batches; ++batch) {
            for (const SelectQuery& query : queries) {
                const auto player = select(query.position);
                checksum += player.id + player.score;
            }
        }
        timing = std::chrono::duration<double, std::nano>(Clock::now() - start).count()
               / (batches * queries.size());
    }
    printTimings(name, timings);
    std::cout << "Checksum: " << checksum << '\n';
}

// Check every player in a division, including order and score, outside timing.
template <typename SelectFunction>
bool checkDivision(int actualSize, int firstId, int lastId, SelectFunction select) {
    if (actualSize != lastId - firstId + 1) return false;
    for (int id = firstId; id <= lastId; ++id) {
        const auto player = select(id - firstId + 1);
        if (player.id != id || player.score != 100 - 10 * ((id - 1) / 10)) return false;
    }
    return true;
}

int main() {
    std::vector<long long> scores;
    for (int id = 1; id <= 100; ++id) scores.push_back(100 - 10 * ((id - 1) / 10));

    avl_demo::AVL avl;
    avl_demo::Node* root = nullptr;
    fenwick_demo::Fenwick fenwick(scores);
    for (int id = 1; id <= 100; ++id) {
        root = avl.insert(root, {id, scores[id - 1]});
        fenwick.insert({id, scores[id - 1]});
    }
    std::cout << "Players: 100, distinct scores: 10\n"
              << "Position\tExpected ID\tExpected score\tAVL ID\tFenwick ID\n";
    for (const SelectQuery& query : queries) {
        const auto a = avl.select(root, query.position);
        const auto b = fenwick.select(query.position);
        std::cout << query.position << '\t' << query.expectedId << "\t\t"
                  << query.expectedScore << "\t\t" << a.id << '\t' << b.id << '\n';
        if (a.id != query.expectedId || b.id != query.expectedId ||
            a.score != query.expectedScore || b.score != query.expectedScore) {
            std::cerr << "Select check failed\n";
            avl.destroy(root);
            return 1;
        }
    }
    benchmarkSelect("AVL select", [&](int position) { return avl.select(root, position); });
    benchmarkSelect("Fenwick select", [&](int position) { return fenwick.select(position); });
    avl.destroy(root);

    // Split changes its input. Prepare independent boards before each timed batch.
    constexpr int copies = 10'000;
    constexpr long long cutoff = 60;
    std::array<double, 5> avlTimings{}, fenwickTimings{};
    bool passed = true;
    for (int run = 0; run < 5; ++run) {
        std::vector<avl_demo::Node*> roots(copies, nullptr), high(copies), low(copies);
        for (auto& board : roots) {
            for (int id = 1; id <= 100; ++id) board = avl.insert(board, {id, scores[id - 1]});
        }
        const auto avlStart = Clock::now();
        for (int i = 0; i < copies; ++i) avl.split(roots[i], cutoff, high[i], low[i]);
        avlTimings[run] = std::chrono::duration<double, std::nano>(Clock::now() - avlStart).count()
                        / copies;
        for (int i = 0; i < copies; ++i) {
            const bool highValid = checkDivision(avl.size(high[i]), 1, 50,
                [&](int k) { return avl.select(high[i], k); });
            const bool lowValid = checkDivision(avl.size(low[i]), 51, 100,
                [&](int k) { return avl.select(low[i], k); });
            passed = passed && highValid && lowValid;
            avl.destroy(high[i]);
            avl.destroy(low[i]);
        }

        std::vector<fenwick_demo::Fenwick> boards;
        boards.reserve(copies);
        for (int i = 0; i < copies; ++i) boards.push_back(fenwick);
        std::vector<fenwick_demo::Fenwick> highBoards(copies);
        const auto fenwickStart = Clock::now();
        for (int i = 0; i < copies; ++i) highBoards[i] = boards[i].split(cutoff);
        fenwickTimings[run] = std::chrono::duration<double, std::nano>(Clock::now() - fenwickStart).count()
                            / copies;
        for (int i = 0; i < copies; ++i) {
            const bool highValid = checkDivision(highBoards[i].prefix(int(highBoards[i].scores.size())),
                1, 50, [&](int k) { return highBoards[i].select(k); });
            const bool lowValid = checkDivision(boards[i].prefix(int(boards[i].scores.size())),
                51, 100, [&](int k) { return boards[i].select(k); });
            passed = passed && highValid && lowValid;
        }
    }
    if (!passed) {
        std::cerr << "Split check failed\n";
        return 1;
    }
    std::cout << "Split cutoff 60: high IDs 1-50, low IDs 51-100. All split checks passed.\n";
    printTimings("AVL split", avlTimings);
    printTimings("Fenwick split", fenwickTimings);
}
