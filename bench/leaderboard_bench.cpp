#include "backend_tools.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {

using namespace leaderboard;
constexpr std::uint64_t seed = 253;
constexpr int trials = 5;

template<class Operation>
double elapsed(Operation operation) {
#ifdef _WIN32
    static const double ns_per_tick = [] {
        LARGE_INTEGER frequency;
        if (!QueryPerformanceFrequency(&frequency))
            throw std::runtime_error("high-resolution performance timer unavailable");
        return 1.0e9 / frequency.QuadPart;
    }();
    LARGE_INTEGER start, end;
    QueryPerformanceCounter(&start);
    operation();
    QueryPerformanceCounter(&end);
    return ns_per_tick * (end.QuadPart - start.QuadPart);
#else
    using Clock = std::chrono::steady_clock;
    auto start = Clock::now();
    operation();
    return std::chrono::duration<double, std::nano>(Clock::now() - start).count();
#endif
}

struct Metrics {
    std::uint64_t rotations = 0;
    std::uint64_t shifts = 0;
    std::uint64_t join_visits = 0;
    int height_gap = -1;
};

struct Case {
    std::size_t players;
    std::size_t ties;
    std::vector<Score> coordinates;
    std::vector<Position> positions;
    std::vector<Score> thresholds;
    std::uint64_t rank_checksum = 0;
    std::uint64_t select_checksum = 0;

    Case(std::size_t n, std::size_t t, std::size_t queries) : players(n), ties(t) {
        coordinates.resize((n + t - 1) / t);
        std::iota(coordinates.begin(), coordinates.end(), Score{0});
        std::size_t top_size = 1 + (n - 1) % t;
        std::mt19937_64 random(seed);
        for (std::size_t i = 0; i < queries; ++i) {
            Position position = 1 + random() % n;
            std::size_t index = position - 1;
            std::size_t score = (n - 1) / t - (index + t - top_size) / t;
            std::size_t preceding = n - std::min(n, (score + 1) * t);
            PlayerId id = score * t + (index - preceding) + 1;
            positions.push_back(position);
            thresholds.push_back(static_cast<Score>(score));
            rank_checksum += preceding + 1;
            select_checksum += id + score;
        }
    }

    Player entry(PlayerId id) const { return {id, static_cast<Score>((id - 1) / ties)}; }
};

void row(const Case& workload, const char* backend, const char* metric, int trial,
         std::size_t operations, double ns, std::uint64_t checksum, Metrics counters = {}) {
    std::cout << backend << ',' << metric << ',' << workload.players << ','
              << workload.coordinates.size() << ',' << workload.ties << ',' << seed << ','
              << trial << ',' << operations << ',' << ns << ',' << checksum << ','
              << counters.rotations << ',' << counters.shifts << ',' << counters.join_visits << ','
              << counters.height_gap << '\n';
}

std::unique_ptr<Board> new_board(bool avl, const Case& data) {
    return avl ? make_avl() : make_fenwick(data.coordinates);
}

void query_times(const Case& data, const Board& board, const char* backend) {
    for (int i = 1; i <= trials; ++i) {
        std::uint64_t rank = 0, select = 0;
        double rank_time = elapsed([&] {
            for (Score score : data.thresholds) rank += board.count_strictly_higher(score) + 1;
        });
        double select_time = elapsed([&] {
            for (auto k : data.positions) {
                auto p = board.select(k);
                select += p.id + static_cast<std::uint64_t>(p.score);
            }
        });
        if (rank != data.rank_checksum || select != data.select_checksum)
            throw std::logic_error("benchmark checksum differs from model");
        row(data, backend, "rank", i, data.thresholds.size(), rank_time, rank);
        row(data, backend, "select", i, data.positions.size(), select_time, select);
    }
}

void run_case(const Case& data, bool avl, bool query_only) {
    const char* backend = avl ? "AVL" : "Fenwick";
    int build_trials = query_only ? 1 : trials;
    std::unique_ptr<Board> board;
    for (int i = 1; i <= build_trials; ++i) {
        board = new_board(avl, data);
        double ns = elapsed([&] {
            for (PlayerId id = data.players; id; --id) board->insert(data.entry(id));
        });
        if (!query_only) {
            Metrics counters;
            if (avl) counters.rotations = avl_stats(*board).rotations;
            else counters.shifts = fenwick_stats(*board).bucket_shifts;
            row(data, backend, "descending_build", i, data.players, ns, board->size(), counters);
        }
    }
    query_times(data, *board, backend);
    if (query_only) return;

    Score cutoff = static_cast<Score>(data.coordinates.size() / 2);
    Position high_size = data.players - static_cast<Position>(cutoff) * data.ties;
    for (int i = 1; i <= trials; ++i) {
        std::pair<std::unique_ptr<Board>, std::unique_ptr<Board>> parts;
        double split_ns = elapsed([&] { parts = board->split_by_score(cutoff); });
        if (board->size() || parts.first->size() != high_size ||
            parts.second->size() != data.players - high_size)
            throw std::logic_error("benchmark split count");
        Metrics split_counters;
        if (avl) split_counters.join_visits = avl_stats(*board).last_join_visits;
        row(data, backend, "split", i, 1, split_ns, high_size, split_counters);
        double merge_ns = elapsed([&] { parts.first->merge_from(*parts.second); });
        if (parts.first->size() != data.players || parts.second->size())
            throw std::logic_error("benchmark separated merge count");
        Metrics merge_counters;
        if (avl) {
            auto stats = avl_stats(*parts.first);
            merge_counters.rotations = stats.rotations;
            merge_counters.join_visits = stats.last_join_visits;
        }
        row(data, backend, "separated_rejoin", i, 1, merge_ns, data.players, merge_counters);
        board = std::move(parts.first);
    }
    board.reset();

    for (int i = 1; i <= trials; ++i) {
        auto left = new_board(avl, data);
        auto right = new_board(avl, data);
        for (PlayerId id = data.players; id; --id)
            (id % 2 ? left : right)->insert(data.entry(id));
        double ns = elapsed([&] { left->merge_from(*right); });
        if (left->size() != data.players || right->size())
            throw std::logic_error("benchmark overlap merge count");
        row(data, backend, "overlap_merge", i, 1, ns, data.players);
    }
}

void supplied_pivots() {
    constexpr std::size_t right_count = 8191;
    for (std::size_t left_count : {8191, 511, 31, 0}) {
        Case descriptor(left_count + right_count + 1, 1, 0);
        for (int trial = 1; trial <= trials; ++trial) {
            auto left = make_avl();
            auto right = make_avl();
            for (std::size_t i = 1; i <= left_count; ++i)
                left->insert({i, static_cast<Score>(i)});
            for (std::size_t i = 1; i <= right_count; ++i)
                right->insert({left_count + i, -static_cast<Score>(i)});
            int gap = avl_stats(*right).height - avl_stats(*left).height;
            std::unique_ptr<Board> result;
            double ns = elapsed([&] {
                result = avl_join_ordered(*left, {descriptor.players, 0}, *right);
            });
            auto stats = avl_stats(*result);
            if (result->size() != descriptor.players || left->size() || right->size())
                throw std::logic_error("benchmark ordered join count");
            row(descriptor, "AVL", "supplied_pivot_join", trial, 1, ns,
                result->size(), {stats.rotations, 0, stats.last_join_visits, gap});
        }
    }
}

}

int main(int argc, char** argv) {
    try {
        std::cout << std::fixed << std::setprecision(2)
                  << "backend,metric,n,u,tie_size,seed,trial,operations,total_ns,checksum,"
                     "rotations,bucket_shifts,join_visits,height_gap\n";
        if (argc == 3 && std::string(argv[1]) == "--profile") {
            std::string backend = argv[2];
            if (backend != "avl" && backend != "fenwick")
                throw std::invalid_argument("profile backend must be avl or fenwick");
            run_case(Case(1000000, 16, 2000000), backend == "avl", true);
            return 0;
        }
        bool quick = argc == 2 && std::string(argv[1]) == "--quick";
        if (argc != 1 && !quick) throw std::invalid_argument("expected --quick or --profile BACKEND");
        std::vector<std::size_t> sizes = quick ? std::vector<std::size_t>{1000, 10000} :
                                               std::vector<std::size_t>{1000, 10000, 100000, 1000000};
        for (auto n : sizes) {
            for (std::size_t ties : {1, 16}) {
                Case workload(n, ties, quick ? 10000 : 50000);
                run_case(workload, true, false);
                run_case(workload, false, false);
            }
        }
        supplied_pivots();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
