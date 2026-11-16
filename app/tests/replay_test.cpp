#include "replay.hpp"

#include <bits/stdc++.h>

using namespace std;
using namespace leaderboard;

namespace {

bool higher_first(Player left, Player right) {
    if (left.score != right.score) return left.score > right.score;
    return left.id < right.id;
}

class TestBoard final : public Board {
public:
    Position size() const override {
        return players_.size();
    }

    bool contains(Player player) const override {
        for (const auto& current : players_)
            if (current.id == player.id && current.score == player.score)
                return true;
        return false;
    }

    void insert(Player player) override {
        if (contains(player)) throw logic_error("duplicate player");
        auto place = lower_bound(players_.begin(), players_.end(), player,
                                 higher_first);
        players_.insert(place, player);
    }

    void erase(Player player) override {
        for (auto place = players_.begin(); place != players_.end(); ++place) {
            if (place->id == player.id && place->score == player.score) {
                players_.erase(place);
                return;
            }
        }
        throw logic_error("missing player");
    }

    Position count_strictly_higher(Score score)
        const override {
        Position count = 0;
        for (const auto& player : players_)
            if (player.score > score) ++count;
        return count;
    }

    Player select(Position position) const override {
        return players_.at(static_cast<size_t>(position - 1));
    }

    void range(Position first, Position last,
               vector<Player>& result) const override {
        result.clear();
        for (Position position = first; position <= last; ++position)
            result.push_back(select(position));
    }

    pair<unique_ptr<Board>, unique_ptr<Board>>
    split_by_score(Score threshold) override {
        auto high = make_unique<TestBoard>();
        auto low = make_unique<TestBoard>();
        for (const auto& player : players_) {
            if (player.score >= threshold) high->players_.push_back(player);
            else low->players_.push_back(player);
        }
        players_.clear();
        return {move(high), move(low)};
    }

    void merge_from(Board& other) override {
        auto* source = dynamic_cast<TestBoard*>(&other);
        if (!source) throw invalid_argument("wrong backend");
        for (const auto& player : source->players_) insert(player);
        source->players_.clear();
    }

private:
    vector<Player> players_;
};

unique_ptr<Board> new_test_board(const vector<Score>&) {
    return make_unique<TestBoard>();
}

}

void test_example() {
    ifstream trace("app/examples/basic.trace");
    ifstream expected_file("app/examples/basic.expected");
    assert(trace && expected_file);
    ostringstream actual;
    run_replay(trace, actual, new_test_board);
    ostringstream expected;
    expected << expected_file.rdbuf();
    assert(actual.str() == expected.str());
}

void test_error_commands() {
    istringstream errors(
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
        "ADD 3 18446744073709551616 0\n"
        "ADD 3 +4 10\n"
        "ADD 3 4 --1\n"
        "ADD 3 4 1x\n");
    ostringstream error_output;
    run_replay(errors, error_output, new_test_board);
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
           "ERR BAD_COMMAND\n"
           "ERR BAD_COMMAND\n"
           "ERR BAD_COMMAND\n"
           "ERR BAD_COMMAND\n");
}

int main() {
    test_example();
    test_error_commands();
}
