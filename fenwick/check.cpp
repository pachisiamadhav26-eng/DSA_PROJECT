#include "fenwick_leaderboard.hpp"
#include <iostream>
#include <limits>
#include <map>
#include <random>

using namespace leaderboard;
using Records = std::map<Id, Score>;

void require(bool condition, const char* reason) {
    if (!condition) throw std::logic_error(reason);
}

template<class Error, class Fn>
void expect_error(Fn fn) {
    try { fn(); }
    catch (const Error&) { return; }
    throw std::logic_error("expected operation to reject its input");
}

// The oracle materializes the leaderboard as a sorted array; it does no tree arithmetic.
template<class Board>
void verify(const Board& board, const Records& records) {
    board.audit();
    std::vector<Player> sorted;
    for (const auto& [id, score] : records) sorted.push_back({id, score});
    std::sort(sorted.begin(), sorted.end(), [](Player a, Player b) {
        if (a.score == b.score) return a.id < b.id;
        return a.score > b.score;
    });
    require(board.size() == sorted.size(), "size mismatch");
    Position expected_rank = 0;
    for (Position i = 0; i < sorted.size(); ++i) {
        if (!i || sorted[i].score != sorted[i - 1].score) expected_rank = i + 1;
        require(board.rank(sorted[i].id) == expected_rank, "competition rank mismatch");
        require(board.select(i + 1) == sorted[i], "select mismatch");
    }
    if (!sorted.empty()) {
        require(board.range(1, sorted.size()) == sorted, "full range mismatch");
        Position first = sorted.size() >= 50 ? 40 : (sorted.size() + 1) / 2;
        Position last = std::min(sorted.size(), first + 10);
        std::vector<Player> page(sorted.begin() + first - 1, sorted.begin() + last);
        require(board.range(first, last) == page, "page mismatch");
    }
}

template<class Board>
void check_split(Board& board, const Records& records, Score threshold) {
    Records low_records, high_records;
    for (const auto& [id, s] : records)
        (s < threshold ? low_records : high_records).emplace(id, s);
    auto high = board.split_off(threshold);
    verify(board, low_records);
    verify(high, high_records);
    board.merge_from(high);
    verify(board, records);
    verify(high, {});
}

template<class Board>
void check_merge(Board& a, Board& b) {
    a.insert(10, 100); a.insert(11, 80); a.insert(14, 60);
    b.insert(12, 90); b.insert(13, 80); b.insert(10, 50);
    expect_error<std::invalid_argument>([&] { a.merge_from(b); });
    verify(a, {{10,100}, {11,80}, {14,60}});
    verify(b, {{12,90}, {13,80}, {10,50}});
    b.erase(10);
    a.merge_from(b);
    verify(a, {{10,100}, {12,90}, {11,80}, {13,80}, {14,60}});
    verify(b, {});
    expect_error<std::invalid_argument>([&] { a.merge_from(a); });
    b.insert(12, 90); // emptied donor remains usable
    verify(b, {{12,90}});
}

template<class Board>
void check_errors(Board& board) {
    expect_error<std::out_of_range>([&] { board.select(1); });
    expect_error<std::out_of_range>([&] { board.erase(1); });
    expect_error<std::out_of_range>([&] { board.update(1, 0); });
    expect_error<std::out_of_range>([&] { board.rank(1); });
    board.insert(-1, 0);
    expect_error<std::invalid_argument>([&] { board.insert(-1, 0); });
    expect_error<std::out_of_range>([&] { board.select(0); });
    expect_error<std::out_of_range>([&] { board.range(2, 1); });
    expect_error<std::out_of_range>([&] { board.range(1, 2); });
    verify(board, {{-1,0}});
    board.erase(-1);
    verify(board, {});
}

void random_replay() {
    std::vector<Score> universe;
    for (Score s = -10; s <= 10; ++s) universe.push_back(s * 2);
    FenwickLeaderboard board(universe);
    Records records;
    std::mt19937 rng(253);
    for (int step = 0; step < 3000; ++step) {
        Id id = static_cast<Id>(rng() % 90) - 45;
        Score s = universe[rng() % universe.size()];
        if (records.count(id) == 0) {
            board.insert(id, s); records[id] = s;
        } else if (rng() % 3 == 0) {
            board.erase(id); records.erase(id);
        } else {
            board.update(id, s); records[id] = s;
        }
        verify(board, records);
        if (step % 31 == 0) {
            const Score threshold = static_cast<Score>(rng() % 65) - 32;
            check_split(board, records, threshold);
        }
    }
}

int main() {
    try {
        FenwickLeaderboard b({100, 60, 80, 80}), other_b({50, 80, 90});
        check_merge(b, other_b);
        FenwickLeaderboard empty_b({0});
        check_errors(empty_b);
        empty_b.insert(5, 0);
        expect_error<std::invalid_argument>([&] { empty_b.update(5, 99); });
        expect_error<std::invalid_argument>([&] { empty_b.insert(6, 99); });
        verify(empty_b, {{5,0}});
        const Score minimum = std::numeric_limits<Score>::min();
        const Score maximum = std::numeric_limits<Score>::max();
        FenwickLeaderboard extremes({minimum, maximum});
        for (Score s : {minimum, maximum}) extremes.insert(s, s);
        check_split(extremes, {{minimum,minimum},{maximum,maximum}}, maximum);
        FenwickLeaderboard no_coordinates({});
        check_split(no_coordinates, {}, 0);
        random_replay();
        std::cout << "Passed: Fenwick matches the sorted-array oracle and passes all invariant checks.\n";
    } catch (const std::exception& e) {
        std::cerr << "Check failed: " << e.what() << '\n';
        return 1;
    }
}
