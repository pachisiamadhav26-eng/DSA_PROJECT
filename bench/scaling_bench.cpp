#include "backend_tools.hpp"

#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace leaderboard;
using Clock = std::chrono::steady_clock;
constexpr std::uint64_t seed_base = 253;
constexpr int repeats = 5;
constexpr std::size_t queries = 100000;

void check(bool ok) {
    if (!ok) throw std::runtime_error("unexpected benchmark result");
}

double elapsed(Clock::time_point start) {
    return std::chrono::duration<double, std::nano>(Clock::now() - start).count();
}

void row(const char* op, const char* backend, std::size_t n, std::size_t u, int gap,
         int run, std::size_t operations, double ns, std::uint64_t evidence) {
    std::cout << op << ',' << backend << ',' << n << ',' << u << ',' << gap << ','
              << run << ',' << seed_base + run << ',' << operations << ','
              << ns / operations << ',' << evidence << '\n';
}

void check_ends(const Board& board, std::size_t first_id, std::size_t last_id) {
    check(board.size() == last_id - first_id + 1);
    const auto first = board.select(1), last = board.select(board.size());
    check(first.id == last_id && first.score == static_cast<Score>(last_id));
    check(last.id == first_id && last.score == static_cast<Score>(first_id));
}

void scaling(std::size_t n, int run) {
    std::vector<Score> scores(n);
    std::iota(scores.begin(), scores.end(), Score{1});
    std::mt19937_64 random(seed_base + run);
    std::vector<Position> positions(queries);
    for (Position& position : positions) position = 1 + random() % n;
    const auto expected = std::accumulate(positions.begin(), positions.end(), Position{0});
    const auto median = n / 2;
    for (bool fenwick : {run % 2 == 0, run % 2 != 0}) {
        const char* backend = fenwick ? "fenwick" : "avl";
        auto board = fenwick ? make_fenwick(scores) : make_avl();
        for (PlayerId id = 1; id <= n; id++) board->insert({id, scores[id - 1]});

        for (bool select : {false, true}) {
            std::uint64_t checksum = 0;
            const auto start = Clock::now();
            if (select) {
                for (auto k : positions) checksum += board->select(k).id;
            } else {
                for (auto k : positions) checksum += 1 + board->count_strictly_higher(scores[n - k]);
            }
            const auto ns = elapsed(start);
            check(checksum == (select ? queries * (n + 1) - expected : expected));
            row(select ? "select" : "rank", backend, n, n, -1, run, queries, ns, checksum);
        }

        auto start = Clock::now();
        auto parts = board->split_by_score(static_cast<Score>(median + 1));
        const auto split_ns = elapsed(start);
        check(board->size() == 0);
        check_ends(*parts.first, median + 1, n);
        check_ends(*parts.second, 1, median);
        row("split", backend, n, n, -1, run, 1, split_ns, parts.first->size());

        start = Clock::now();
        parts.first->merge_from(*parts.second);
        const auto merge_ns = elapsed(start);
        check(parts.second->size() == 0);
        check_ends(*parts.first, 1, n);
        row("merge_validated", backend, n, n, -1, run, 1, merge_ns, parts.first->size());

        if (!fenwick) {
            parts = parts.first->split_by_score(static_cast<Score>(median + 1));
            const Player pivot{median, static_cast<Score>(median)};
            parts.second->erase(pivot);
            const int gap = std::abs(avl_height(*parts.first) - avl_height(*parts.second));
            start = Clock::now();
            auto joined = avl_join_ordered(*parts.first, pivot, *parts.second);
            const auto join_ns = elapsed(start);
            check(parts.first->size() == 0 && parts.second->size() == 0);
            check_ends(*joined, 1, n);
            row("join_ordered", "avl", n, n, gap, run, 1, join_ns, joined->size());
        }
    }
}

void unequal_join(int height, int run) {
    auto taller = make_avl(), shorter = make_avl();
    const PlayerId n = (PlayerId{1} << height) - 1;
    for (PlayerId stride = (n + 1) / 2; stride > 0; stride /= 2)
        for (PlayerId id = stride; id <= n; id += 2 * stride)
            taller->insert({id, static_cast<Score>(id)});
    shorter->insert({n + 2, static_cast<Score>(n + 2)});
    const Player pivot{n + 1, static_cast<Score>(n + 1)};
    const int gap = avl_height(*taller) - avl_height(*shorter);
    check(gap == height - 1);

    auto start = Clock::now();
    auto joined = avl_join_ordered(*shorter, pivot, *taller);
    const auto ns = elapsed(start);
    check(shorter->size() == 0 && taller->size() == 0);
    check_ends(*joined, 1, n + 2);
    check(joined->select(2).id == pivot.id);
    row("height_join", "avl", n + 2, n + 2, gap, run, 1, ns, joined->size());
}

void ties(std::size_t n, int run) {
    for (bool fenwick : {run % 2 == 0, run % 2 != 0}) {
        auto board = fenwick ? make_fenwick({0}) : make_avl();
        auto start = Clock::now();
        for (PlayerId id = n; id > 0; id--) board->insert({id, 0});
        const auto ns = elapsed(start);
        check(board->size() == n && board->select(1).id == 1 && board->select(n).id == n);
        const auto shifts = fenwick ? fenwick_stats(*board).bucket_shifts : 0;
        if (fenwick) check(shifts == n * (n - 1) / 2);
        row("tie_insert", fenwick ? "fenwick" : "avl", n, 1, -1, run, 1, ns, shifts);
    }
}
}

int main(int argc, char** argv) {
    const bool quick = argc == 2 && std::string(argv[1]) == "--quick";
    if (argc > 2 || (argc == 2 && !quick)) {
        std::cerr << "usage: scaling_bench [--quick]\n";
        return 2;
    }
    try {
        std::cout << "operation,backend,n,u,height_gap,run,seed,operations,ns_per_operation,check_value\n"
                  << std::fixed << std::setprecision(3);
        for (int run = 0; run < (quick ? 1 : repeats); run++) {
            for (std::size_t n : {1000, 10000, 100000, 1000000}) {
                scaling(n, run);
                if (quick) break;
            }
            for (int height : {4, 8, 12, 16, 19}) {
                unequal_join(height, run);
                if (quick) break;
            }
            for (std::size_t n : {10000, 50000, 100000, 200000}) {
                ties(n, run);
                if (quick) break;
            }
        }
        std::cout.flush();
        if (!std::cout) throw std::runtime_error("cannot write benchmark CSV");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
