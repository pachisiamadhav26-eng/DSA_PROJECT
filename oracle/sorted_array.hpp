#pragma once

#include "../include/leaderboard.hpp"

#include <algorithm>
#include <stdexcept>
#include <unordered_set>

namespace oracle {

using leaderboard::Board;
using leaderboard::Player;
using leaderboard::PlayerId;
using leaderboard::Position;
using leaderboard::Score;

inline bool comes_before(Player first, Player second) {
    if (first.score != second.score) {
        return first.score > second.score;
    }
    return first.id < second.id;
}

inline bool same_player(Player first, Player second) {
    return first.id == second.id && first.score == second.score;
}

// Keep one vector in the same order that users see on the leaderboard.
class SortedArray final : public Board {
public:
    SortedArray() = default;

    explicit SortedArray(std::vector<Player> players) : players_(std::move(players)) {
        std::sort(players_.begin(), players_.end(), comes_before);
        audit();
    }

    const std::vector<Player>& entries() const {
        return players_;
    }

    Position size() const override {
        return players_.size();
    }

    bool contains(Player player) const override {
        auto position = find_position(player);
        return position != players_.end() && same_player(*position, player);
    }

    void insert(Player player) override {
        // An ID must be unique even if the old score is different.
        for (Player existing : players_) {
            if (existing.id == player.id) {
                throw std::invalid_argument("duplicate ID");
            }
        }
        players_.insert(find_position(player), player);
    }

    void erase(Player player) override {
        auto position = find_position(player);
        if (position == players_.end() || !same_player(*position, player)) {
            throw std::invalid_argument("missing exact player");
        }
        players_.erase(position);
    }

    Position count_strictly_higher(Score score) const override {
        // Find the first index whose score is at most the requested score.
        std::size_t left = 0;
        std::size_t right = players_.size();
        while (left < right) {
            std::size_t middle = left + (right - left) / 2;
            if (players_[middle].score > score) {
                left = middle + 1;
            } else {
                right = middle;
            }
        }
        return left;
    }

    Player select(Position position) const override {
        if (position == 0 || position > size()) {
            throw std::out_of_range("position");
        }
        return players_[position - 1];  // Query positions start at 1.
    }

    void range(Position first, Position last, std::vector<Player>& output) const override {
        if (first == 0 || first > last || last > size()) {
            throw std::out_of_range("range");
        }
        // assign replaces the old output. Its end iterator is exclusive.
        output.assign(players_.begin() + (first - 1), players_.begin() + last);
    }

    std::pair<std::unique_ptr<Board>, std::unique_ptr<Board>>
    split_by_score(Score threshold) override {
        auto high = std::make_unique<SortedArray>();
        auto low = std::make_unique<SortedArray>();
        for (Player player : players_) {
            if (player.score >= threshold) {
                high->players_.push_back(player);
            } else {
                low->players_.push_back(player);
            }
        }
        players_.clear();
        return {std::move(high), std::move(low)};
    }

    void merge_from(Board& source) override {
        // Board is the shared interface. Only another SortedArray is valid here.
        auto* donor = dynamic_cast<SortedArray*>(&source);
        if (donor == nullptr || donor == this) {
            throw std::invalid_argument("merge backend/self");
        }

        // Check all IDs before changing either board.
        std::unordered_set<PlayerId> seen_ids;
        for (Player player : players_) {
            seen_ids.insert(player.id);
        }
        for (Player player : donor->players_) {
            if (seen_ids.count(player.id) != 0) {
                throw std::invalid_argument("duplicate ID");
            }
        }

        // Take the next player from whichever array comes first.
        std::vector<Player> merged;
        merged.reserve(players_.size() + donor->players_.size());
        std::size_t left = 0;
        std::size_t right = 0;
        while (left < players_.size() && right < donor->players_.size()) {
            if (comes_before(players_[left], donor->players_[right])) {
                merged.push_back(players_[left]);
                ++left;
            } else {
                merged.push_back(donor->players_[right]);
                ++right;
            }
        }
        while (left < players_.size()) {
            merged.push_back(players_[left]);
            ++left;
        }
        while (right < donor->players_.size()) {
            merged.push_back(donor->players_[right]);
            ++right;
        }
        players_.swap(merged);
        donor->players_.clear();
    }

    void audit() const {
        std::unordered_set<PlayerId> seen_ids;
        for (std::size_t index = 0; index < players_.size(); ++index) {
            Player player = players_[index];
            if (seen_ids.count(player.id) != 0) {
                throw std::logic_error("duplicate ID in oracle");
            }
            seen_ids.insert(player.id);
            if (index > 0 && !comes_before(players_[index - 1], player)) {
                throw std::logic_error("incorrect oracle order");
            }
        }
    }

private:
    std::vector<Player> players_;

    // lower_bound finds the first entry that does not come before the key.
    std::vector<Player>::const_iterator find_position(Player player) const {
        return std::lower_bound(players_.begin(), players_.end(), player, comes_before);
    }
};

}
