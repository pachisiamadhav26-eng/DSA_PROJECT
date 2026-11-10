#include "fenwick_leaderboard.hpp"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <string>

using namespace leaderboard;
using Clock = std::chrono::steady_clock;

struct Revision {
    Id id;
    Score score;
    Id query_id;
    Position position;
};

template<class Fn>
double measure_ns(Fn fn) {
    const auto start = Clock::now();
    fn();
    return std::chrono::duration<double, std::nano>(Clock::now() - start).count();
}

template<class Board>
void replay(const char* name, Board& board, const std::vector<Score>& initial,
                     const std::vector<Revision>& events, Score threshold) {
    double update_ns = 0, rank_ns = 0, select_ns = 0;
    std::uint64_t checksum = 0;
    auto mix = [&](std::uint64_t x) { checksum = checksum * 1000003ULL + x; };
    const double load_ns = measure_ns([&] {
        for (Position i = 0; i < initial.size(); ++i)
            board.insert(static_cast<Id>(i), initial[i]);
    });
    for (const auto& e : events) {
        update_ns += measure_ns([&] { board.update(e.id, e.score); });
        Position rank = 0;
        rank_ns += measure_ns([&] { rank = board.rank(e.query_id); });
        mix(rank);
        Player selected{};
        select_ns += measure_ns([&] { selected = board.select(e.position); });
        mix(static_cast<std::uint64_t>(selected.id));
        mix(static_cast<std::uint64_t>(selected.score));
    }
    const Position last = std::min<Position>(50, board.size());
    const Position first = std::min<Position>(40, last);
    std::vector<Player> page;
    const double range_ns = measure_ns([&] { page = board.range(first, last); });
    for (Player p : page) mix(static_cast<std::uint64_t>(p.id));
    const auto start = Clock::now();
    auto high = board.split_off(threshold);
    const double split_ms = std::chrono::duration<double, std::milli>(Clock::now() - start).count();
    mix(high.size());
    const double merge_ns = measure_ns([&] { board.merge_from(high); });
    board.audit();
    mix(board.size());
    const double samples = static_cast<double>(events.size());
    std::cout << name << ',' << load_ns / 1e6 << ',' << update_ns / samples << ','
              << rank_ns / samples << ',' << select_ns / samples << ',' << range_ns << ','
              << split_ms << ',' << merge_ns / 1e6 << ',' << checksum << '\n';
}

Position positive(const char* value) {
    const std::string s(value);
    if (s.empty() || s.find_first_not_of("0123456789") != std::string::npos)
        throw std::invalid_argument("arguments must be positive integers");
    const auto n = std::stoll(s);
    if (!n || static_cast<unsigned long long>(n) > std::numeric_limits<Position>::max())
        throw std::out_of_range("unsupported argument size");
    return static_cast<Position>(n);
}

int main(int argc, char** argv) {
    try {
        if (argc > 4) throw std::invalid_argument("usage: compare [players [revisions [scores]]]");
        const Position n = argc > 1 ? positive(argv[1]) : 10000;
        const Position revisions = argc > 2 ? positive(argv[2]) : 10000;
        const Position u = argc > 3 ? positive(argv[3]) : 1024;
        std::mt19937_64 rng(2026);
        std::uniform_int_distribution<Score> score(0, static_cast<Score>(u - 1));
        std::uniform_int_distribution<Position> player(0, n - 1);
        std::vector<Score> initial(n), universe(u);
        std::iota(universe.begin(), universe.end(), Score{0});
        for (auto& s : initial) s = score(rng);
        std::vector<Revision> events;
        events.reserve(revisions);
        for (Position i = 0; i < revisions; ++i)
            events.push_back({static_cast<Id>(player(rng)), score(rng),
                              static_cast<Id>(player(rng)), 1 + player(rng)});
        std::cout << "players=" << n << " revisions=" << revisions << " U=" << u << " seed=2026\n"
                  << "structure,insert_all_ms,update_mean_ns,rank_mean_ns,select_mean_ns,"
                     "one_range_ns,split_ms,ordered_merge_ms,checksum\n" << std::fixed
                  << std::setprecision(2);
        const Score threshold = static_cast<Score>(u / 2);
        FenwickLeaderboard board(std::move(universe));
        replay("Fenwick", board, initial, events, threshold);
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
