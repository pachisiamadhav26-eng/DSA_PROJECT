#pragma once
#include "../include/leaderboard.hpp"
#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <unordered_set>

namespace oracle {
using namespace leaderboard;
inline bool before(Player a, Player b) {
    return a.score != b.score ? a.score > b.score : a.id < b.id;
}
inline bool equal(Player a, Player b) { return a.id == b.id && a.score == b.score; }

// Deliberately simple reference: a sorted vector, not an augmented tree.
// Library containers are permitted in the course's oracle/test code.
class SortedArray final : public Board {
    std::vector<Player> players_;
public:
    SortedArray() = default;
    explicit SortedArray(std::vector<Player> players) : players_(std::move(players)) {
        std::sort(players_.begin(), players_.end(), before);
        audit();
    }
    const std::vector<Player>& entries() const { return players_; }
    Position size() const override { return players_.size(); }
    bool contains(Player p) const override {
        auto it = std::lower_bound(players_.begin(), players_.end(), p, before);
        return it != players_.end() && equal(*it, p);
    }
    void insert(Player p) override {
        for (auto q : players_)
            if (p.id == q.id) throw std::invalid_argument("duplicate ID");
        players_.insert(std::lower_bound(players_.begin(), players_.end(), p, before), p);
    }
    void erase(Player p) override {
        auto it = std::lower_bound(players_.begin(), players_.end(), p, before);
        if (it == players_.end() || !equal(*it, p))
            throw std::invalid_argument("missing exact player");
        players_.erase(it);
    }
    Position count_strictly_higher(Score s) const override {
        return std::lower_bound(players_.begin(), players_.end(), s,
            [](Player p, Score score) { return p.score > score; }) - players_.begin();
    }
    Player select(Position k) const override {
        if (k == 0 || k > size()) throw std::out_of_range("position");
        return players_[static_cast<std::size_t>(k - 1)];
    }
    void range(Position first, Position last, std::vector<Player>& out) const override {
        if (first == 0 || first > last || last > size()) throw std::out_of_range("range");
        out.assign(players_.begin() + static_cast<std::size_t>(first - 1),
                   players_.begin() + static_cast<std::size_t>(last));
    }
    std::pair<std::unique_ptr<Board>, std::unique_ptr<Board>>
    split_by_score(Score threshold) override {
        auto cut = std::find_if(players_.begin(), players_.end(),
                               [threshold](Player p) { return p.score < threshold; });
        auto high = std::make_unique<SortedArray>();
        auto low = std::make_unique<SortedArray>();
        high->players_.assign(players_.begin(), cut);
        low->players_.assign(cut, players_.end());
        players_.clear();
        return {std::move(high), std::move(low)};
    }
    void merge_from(Board& source) override {
        auto other = dynamic_cast<SortedArray*>(&source);
        if (!other || other == this) throw std::invalid_argument("merge backend/self");
        std::unordered_set<PlayerId> ids;
        for (auto p : players_) ids.insert(p.id);
        for (auto p : other->players_)
            if (!ids.insert(p.id).second) throw std::invalid_argument("duplicate ID");
        std::vector<Player> joined;
        joined.reserve(players_.size() + other->players_.size());
        std::merge(players_.begin(), players_.end(), other->players_.begin(),
                   other->players_.end(), std::back_inserter(joined), before);
        players_.swap(joined);
        other->players_.clear();
    }
    void audit() const {
        std::unordered_set<PlayerId> ids;
        for (std::size_t i = 0; i < players_.size(); i++) {
            if (!ids.insert(players_[i].id).second ||
                (i && !before(players_[i-1], players_[i])))
                throw std::logic_error("oracle order/unique ID");
        }
    }
};
}
