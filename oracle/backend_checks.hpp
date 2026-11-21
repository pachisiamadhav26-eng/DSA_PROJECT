#pragma once

#include "backend_tools.hpp"
#include "sorted_array.hpp"

#include <iostream>
#include <limits>

namespace oracle {

using leaderboard::audit_avl;
using leaderboard::audit_fenwick;
using leaderboard::avl_join_ordered;
using leaderboard::avl_split_by_key;
using leaderboard::avl_stats;
using leaderboard::make_avl;
using leaderboard::make_fenwick;

// An audit reads one board, checks its internal rules and returns its players.
using Audit = std::vector<Player> (*)(const Board&);

inline void require(bool condition, const char* message) {
    if (!condition) {
        throw std::logic_error(message);
    }
}

inline void check_players(const std::vector<Player>& actual, const std::vector<Player>& expected,
                          std::size_t start, std::size_t end) {
    // Compare against the half-open slice [start, end) of the oracle's order.
    require(actual.size() == end - start, "player count differs from oracle");
    for (std::size_t index = 0; index < actual.size(); ++index) {
        require(same_player(actual[index], expected[start + index]), "contents differ from oracle");
    }
}

inline void test_invalid_oracle_calls(SortedArray& board) {
    SortedArray donor({{8, 7}});
    const std::vector<std::string> cases = {
        "duplicate ID", "missing key", "position zero", "position too large",
        "reversed range", "merge with duplicate ID"
    };

    for (const std::string& name : cases) {
        bool rejected = false;
        try {
            if (name == "duplicate ID") {
                board.insert({8, 100});
            } else if (name == "missing key") {
                board.erase({8, 91});
            } else if (name == "position zero") {
                board.select(0);
            } else if (name == "position too large") {
                board.select(5);
            } else if (name == "reversed range") {
                std::vector<Player> page;
                board.range(2, 1, page);
            } else {
                board.merge_from(donor);
            }
        } catch (const std::logic_error&) {
            rejected = true;
        }
        if (!rejected) {
            throw std::logic_error("oracle accepted invalid call: " + name);
        }
    }
    require(board.size() == 4 && donor.size() == 1, "failed merge changed a board");
}

inline void test_exact_membership(Board& board, PlayerId id) {
    board.insert({id, 90});
    require(board.contains({id, 90}), "contains rejected the exact key");
    require(!board.contains({id, 91}), "contains accepted the wrong score");
    require(!board.contains({19, 90}), "contains accepted the wrong ID");
}

inline void test_oracle_boundaries() {
    PlayerId max_id = std::numeric_limits<PlayerId>::max();
    SortedArray board({
        {max_id, std::numeric_limits<Score>::min()},
        {0, std::numeric_limits<Score>::max()},
        {8, 90}, {3, 90}
    });

    require(board.select(1).id == 0, "highest score must be first");
    require(board.select(3).id == 8, "equal scores must use ID order");
    require(board.count_strictly_higher(90) == 1, "ties must share the same rank");
    require(board.contains({8, 90}) && !board.contains({8, 91}), "oracle exact membership");
    test_invalid_oracle_calls(board);

    auto divisions = board.split_by_score(90);
    require(board.size() == 0 && divisions.first->size() == 3, "oracle split");
    divisions.first->merge_from(*divisions.second);
    require(divisions.first->size() == 4 && divisions.second->size() == 0, "oracle merge");

    auto avl = make_avl();
    auto fenwick = make_fenwick({90, 91});
    test_exact_membership(*avl, max_id);
    test_exact_membership(*fenwick, max_id);
}

inline void test_avl_rotations() {
    // These insertion orders cause the four AVL rotation patterns.
    const std::vector<std::vector<Score>> orders = {
        {30, 20, 10}, {10, 20, 30}, {30, 10, 20}, {10, 30, 20}
    };
    for (const auto& order : orders) {
        auto board = make_avl();
        for (Score score : order) {
            board->insert({static_cast<PlayerId>(score), score});
            audit_avl(*board);
        }
        require(avl_stats(*board).rotations > 0, "expected an AVL rotation");
        for (Score score : order) {
            board->erase({static_cast<PlayerId>(score), score});
            audit_avl(*board);
        }
    }
}

inline void test_avl_joins() {
    for (bool large_scores_first : {true, false}) {
        auto large = make_avl();
        auto small = make_avl();
        Score small_score = 5000;
        if (large_scores_first) {
            small_score = 0;
        }
        for (PlayerId id = 0; id < 4096; ++id) {
            large->insert({id, static_cast<Score>(id) + 2});
        }
        small->insert({5000, small_score});
        large->merge_from(*small);
        require(audit_avl(*large).size() == 4097 && small->size() == 0, "unequal join");
    }

    PlayerId max_id = std::numeric_limits<PlayerId>::max();
    auto tied = make_avl();
    tied->insert({1, 90});
    tied->insert({2, 90});
    tied->insert({max_id, 90});
    auto parts = avl_split_by_key(*tied, {2, 90});
    check_players(audit_avl(*parts.first), {{1, 90}}, 0, 1);
    check_players(audit_avl(*parts.second), {{2, 90}, {max_id, 90}}, 0, 2);
    require(tied->size() == 0, "key split left players on its source");

    parts.second->erase({2, 90});
    auto joined = avl_join_ordered(*parts.first, {2, 90}, *parts.second);
    require(parts.first->size() == 0 && parts.second->size() == 0, "join did not empty both inputs");
    check_players(audit_avl(*joined), {{1, 90}, {2, 90}, {max_id, 90}}, 0, 3);
}

inline void check_scale_split(Board& board, Audit audit, const std::vector<Player>& expected,
                              Score threshold, std::size_t high_count) {
    auto divisions = board.split_by_score(threshold);
    require(board.size() == 0, "scale split source");
    check_players(audit(*divisions.first), expected, 0, high_count);
    check_players(audit(*divisions.second), expected, high_count, expected.size());

    divisions.first->merge_from(*divisions.second);
    require(divisions.second->size() == 0, "scale merge source");
    check_players(audit(*divisions.first), expected, 0, expected.size());
}

inline void test_scale(std::size_t player_count) {
    require(player_count >= 1 && player_count <= 1000000, "scale must be 1..1000000");

    std::vector<Player> players;
    std::vector<Score> scores;
    players.reserve(player_count);
    scores.reserve(player_count);
    for (PlayerId id = 0; id < player_count; ++id) {
        Score score = static_cast<Score>(id / 4);  // Four IDs per score.
        players.push_back({id, score});
        scores.push_back(score);
    }

    SortedArray reference(players);
    auto avl = make_avl();
    auto fenwick = make_fenwick(scores);
    for (Player player : players) {
        avl->insert(player);
        fenwick->insert(player);
    }

    const auto& expected = reference.entries();
    check_players(audit_avl(*avl), expected, 0, expected.size());
    check_players(audit_fenwick(*fenwick), expected, 0, expected.size());

    // Alternate rank and select. Compare each answer to the independent model.
    for (std::size_t index = 0; index < player_count; ++index) {
        if (index % 2 == 0) {
            Score score = players[index].score;
            Position higher = reference.count_strictly_higher(score);
            require(avl->count_strictly_higher(score) == higher, "scale AVL rank");
            require(fenwick->count_strictly_higher(score) == higher, "scale Fenwick rank");
        } else {
            Position position = index + 1;
            Player player = reference.select(position);
            require(same_player(avl->select(position), player), "scale AVL select");
            require(same_player(fenwick->select(position), player), "scale Fenwick select");
        }
    }

    Score threshold = static_cast<Score>(player_count / 8);
    std::size_t high_count = 0;
    while (high_count < expected.size() && expected[high_count].score >= threshold) {
        ++high_count;
    }
    check_scale_split(*avl, audit_avl, expected, threshold, high_count);
    check_scale_split(*fenwick, audit_fenwick, expected, threshold, high_count);
    std::cout << "PASS scale players=" << player_count << " queries=" << player_count
              << " split/merge (full audits at phase boundaries)\n";
}

}
