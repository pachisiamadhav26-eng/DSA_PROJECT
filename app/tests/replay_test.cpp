#include "replay.hpp"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

bool before(leaderboard::Player left, leaderboard::Player right) {
    if (left.score != right.score) return left.score > right.score;
    return left.id < right.id;
}

class TestBoard final : public leaderboard::Board {
public:
    leaderboard::Position size() const override {
        return players_.size();
    }

    bool contains(leaderboard::Player player) const override {
        for (const auto& current : players_)
            if (current.id == player.id && current.score == player.score)
                return true;
        return false;
    }

    void insert(leaderboard::Player player) override {
        if (contains(player)) throw std::logic_error("duplicate player");
        auto place = std::lower_bound(players_.begin(), players_.end(), player,
                                      before);
        players_.insert(place, player);
    }

    void erase(leaderboard::Player player) override {
        for (auto place = players_.begin(); place != players_.end(); ++place) {
            if (place->id == player.id && place->score == player.score) {
                players_.erase(place);
                return;
            }
        }
        throw std::logic_error("missing player");
    }

    leaderboard::Position count_strictly_higher(leaderboard::Score score)
        const override {
        leaderboard::Position count = 0;
        for (const auto& player : players_)
            if (player.score > score) ++count;
        return count;
    }

    leaderboard::Player select(leaderboard::Position position) const override {
        return players_.at(static_cast<std::size_t>(position - 1));
    }

    void range(leaderboard::Position first, leaderboard::Position last,
               std::vector<leaderboard::Player>& result) const override {
        result.clear();
        for (leaderboard::Position position = first; position <= last; ++position)
            result.push_back(select(position));
    }

    std::pair<std::unique_ptr<leaderboard::Board>,
              std::unique_ptr<leaderboard::Board>>
    split_by_score(leaderboard::Score threshold) override {
        auto high = std::unique_ptr<TestBoard>(new TestBoard());
        auto low = std::unique_ptr<TestBoard>(new TestBoard());
        for (const auto& player : players_) {
            if (player.score >= threshold) high->players_.push_back(player);
            else low->players_.push_back(player);
        }
        players_.clear();
        return {std::move(high), std::move(low)};
    }

    void merge_from(leaderboard::Board& other) override {
        auto* source = dynamic_cast<TestBoard*>(&other);
        if (!source) throw std::invalid_argument("wrong backend");
        for (const auto& player : source->players_) insert(player);
        source->players_.clear();
    }

private:
    std::vector<leaderboard::Player> players_;
};

leaderboard::BoardFactory factory() {
    return [](const std::vector<leaderboard::Score>&) {
        return std::unique_ptr<leaderboard::Board>(new TestBoard());
    };
}

}

int main() {
    std::ifstream trace("app/examples/basic.trace");
    std::ifstream expected_file("app/examples/basic.expected");
    assert(trace && expected_file);
    std::ostringstream actual;
    leaderboard::run_replay(trace, actual, factory());
    std::ostringstream expected;
    expected << expected_file.rdbuf();
    assert(actual.str() == expected.str());

    std::istringstream errors(
        "ADD 0 1 10\n"
        "ADD 0 1 20\n"
        "RANK 0 1\n"
        "SPLIT 0 10 1 2\n"
        "SIZE 0\n"
        "ADD 2 1 10\n"
        "RANK 2 1\n"
        "RANK 1 1\n"
        "MERGE 1 2 3\n"
        "MERGE 3 3 4\n"
        "SET 3 1 10\n"
        "SELECT 3 0\n"
        "RANGE 3 2 1\n"
        "REMOVE 3 1\n"
        "RANK 3 1\n"
        "ADD 3 2 -9223372036854775808\n"
        "SELECT 3 1\n"
        "ADD 3 -1 5\n"
        "ADD 3 4 9223372036854775808\n"
        "ADD 3 18446744073709551615 9223372036854775807\n"
        "RANK 3 18446744073709551615\n"
        "ADD 3 18446744073709551616 0\n");
    std::ostringstream error_output;
    leaderboard::run_replay(errors, error_output, factory());
    assert(error_output.str() ==
           "OK\n"
           "ERR DUPLICATE_ID\n"
           "RANK 1\n"
           "OK\n"
           "ERR UNKNOWN_BOARD\n"
           "ERR DUPLICATE_ID\n"
           "ERR WRONG_BOARD\n"
           "RANK 1\n"
           "OK\n"
           "ERR SAME_BOARD\n"
           "OK\n"
           "ERR BAD_POSITION\n"
           "ERR BAD_RANGE\n"
           "OK\n"
           "ERR UNKNOWN_PLAYER\n"
           "OK\n"
           "PLAYER 2 -9223372036854775808\n"
           "ERR BAD_COMMAND\n"
           "ERR BAD_COMMAND\n"
           "OK\n"
           "RANK 1\n"
           "ERR BAD_COMMAND\n");

    std::istringstream seasons(
        "SPLIT 0 0 1 2\n"
        "MERGE 1 99 1\n"
        "MERGE 1 2 1\n"
        "MERGE 1 2 3\n"
        "SPLIT 3 0 0 4\n"
        "\n"
        "SIZE 3\n");
    std::ostringstream season_output;
    leaderboard::run_replay(seasons, season_output, factory());
    assert(season_output.str() == "OK\nERR UNKNOWN_BOARD\nERR SAME_BOARD\n"
                                  "OK\nERR BOARD_EXISTS\nERR BAD_COMMAND\nSIZE 0\n");
}
