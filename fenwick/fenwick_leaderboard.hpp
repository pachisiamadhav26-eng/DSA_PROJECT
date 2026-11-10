#pragma once

#include "../common.hpp"
#include <iterator>

namespace leaderboard {

// Coordinates are 1-based here. The leaderboard's score and ID vectors are 0-based.
class FenwickCounts {
    std::vector<Position> bit_;

    static Position lowbit(Position i) { return i & (Position{0} - i); }

public:
    explicit FenwickCounts(Position coordinates) : bit_(coordinates + 1, 0) {}

    void change(Position i, bool increment) {
        if (i == 0 || i >= bit_.size()) throw std::out_of_range("invalid coordinate");
        for (; i < bit_.size(); i += lowbit(i)) {
            if (increment) ++bit_[i];
            else --bit_[i]; // caller has removed one existing player
        }
    }

    Position prefix(Position i) const {
        Position total = 0;
        for (; i > 0; i -= lowbit(i)) total += bit_[i];
        return total;
    }

    // Precondition: 1 <= target <= total active players.
    Position kth_smallest(Position target) const {
        Position idx = 0, skipped = 0, step = 1;
        const Position coordinates = bit_.size() - 1;
        while (step <= coordinates / 2) step *= 2;
        for (; step > 0; step /= 2) {
            const Position next = idx + step;
            if (next <= coordinates && skipped + bit_[next] < target) {
                idx = next;
                skipped += bit_[next];
            }
        }
        return idx + 1;
    }

    void rebuild(const std::vector<std::vector<Id>>& buckets) {
        for (Position i = 1; i < bit_.size(); ++i) bit_[i] = buckets[i - 1].size();
        for (Position i = 1; i < bit_.size(); ++i) {
            const Position parent = i + lowbit(i);
            if (parent < bit_.size()) bit_[parent] += bit_[i];
        }
    }
};

class FenwickLeaderboard {
    std::vector<Score> scores_;
    std::vector<std::vector<Id>> buckets_;
    FenwickCounts counts_;
    PlayerTable players_;

    struct SortedUniverse {};
    FenwickLeaderboard(std::vector<Score> scores, SortedUniverse)
        : scores_(std::move(scores)), buckets_(scores_.size()), counts_(scores_.size()) {}

    static std::vector<Score> compress(std::vector<Score> values) {
        std::sort(values.begin(), values.end());
        values.erase(std::unique(values.begin(), values.end()), values.end());
        return values;
    }

    Position coordinate(Score score) const {
        const auto it = std::lower_bound(scores_.begin(), scores_.end(), score);
        if (it == scores_.end() || *it != score)
            throw std::invalid_argument("score absent from the compressed universe");
        return static_cast<Position>(it - scores_.begin());
    }

    void insert_in_bucket(Position i, Id id) {
        auto& ids = buckets_[i];
        ids.insert(std::lower_bound(ids.begin(), ids.end(), id), id);
    }

    void erase_from_bucket(Position i, Id id) {
        auto& ids = buckets_[i];
        ids.erase(std::lower_bound(ids.begin(), ids.end(), id));
    }

public:
    explicit FenwickLeaderboard(std::vector<Score> possible_scores)
        : FenwickLeaderboard(compress(std::move(possible_scores)), SortedUniverse{}) {}

    Position size() const { return players_.size(); }

    void insert(Id id, Score score) {
        const Position i = coordinate(score);
        players_.insert({id, score});
        try { insert_in_bucket(i, id); }
        catch (...) { players_.erase(id); throw; }
        counts_.change(i + 1, true);
    }

    void erase(Id id) {
        const Position i = coordinate(players_.score_of(id));
        erase_from_bucket(i, id);
        counts_.change(i + 1, false);
        players_.erase(id);
    }

    void update(Id id, Score score) {
        const Score old = players_.score_of(id);
        if (old == score) return;
        const Position src = coordinate(old);
        const Position dest = coordinate(score);
        insert_in_bucket(dest, id);
        erase_from_bucket(src, id);
        counts_.change(src + 1, false);
        counts_.change(dest + 1, true);
        players_.set_score(id, score);
    }

    Position rank(Id id) const {
        const Position i = coordinate(players_.score_of(id));
        return size() - counts_.prefix(i + 1) + 1;
    }

    Player select(Position k) const {
        check_range(k, k, size());
        const Position i = counts_.kth_smallest(size() - k + 1) - 1;
        const Position higher = size() - counts_.prefix(i + 1);
        const Position offset = k - higher - 1;
        return {buckets_[i][offset], scores_[i]};
    }

    std::vector<Player> range(Position first, Position last) const {
        check_range(first, last, size());
        std::vector<Player> result;
        result.reserve(last - first + 1);
        for (Position k = first; k <= last; ++k) result.push_back(select(k));
        return result;
    }

    FenwickLeaderboard split_off(Score threshold) {
        FenwickLeaderboard high(scores_, SortedUniverse{});
        const auto cut = std::lower_bound(scores_.begin(), scores_.end(), threshold);
        const Position start = static_cast<Position>(cut - scores_.begin());
        for (Position i = start; i < scores_.size(); ++i)
            for (Id id : buckets_[i]) high.players_.insert({id, scores_[i]});
        for (Position i = start; i < scores_.size(); ++i)
            high.buckets_[i].swap(buckets_[i]);
        high.players_.each([&](Player p) { players_.erase(p.id); });
        counts_.rebuild(buckets_);
        high.counts_.rebuild(high.buckets_);
        return high;
    }

    void merge_from(FenwickLeaderboard& other) {
        if (this == &other) throw std::invalid_argument("cannot merge league with itself");
        std::vector<Score> universe;
        std::set_union(scores_.begin(), scores_.end(), other.scores_.begin(),
                       other.scores_.end(), std::back_inserter(universe));
        FenwickLeaderboard merged(std::move(universe), SortedUniverse{});
        Position a = 0, b = 0;
        for (Position i = 0; i < merged.scores_.size(); ++i) {
            const Score s = merged.scores_[i];
            const bool has_a = a < scores_.size() && scores_[a] == s;
            const bool has_b = b < other.scores_.size() && other.scores_[b] == s;
            if (has_a && has_b) {
                const auto& left = buckets_[a++];
                const auto& right = other.buckets_[b++];
                auto& dest = merged.buckets_[i];
                dest.reserve(left.size() + right.size());
                std::merge(left.begin(), left.end(), right.begin(), right.end(),
                           std::back_inserter(dest));
            } else if (has_a) merged.buckets_[i] = buckets_[a++];
            else merged.buckets_[i] = other.buckets_[b++];
        }
        merged.counts_.rebuild(merged.buckets_);
        players_.append_disjoint(other.players_);
        scores_.swap(merged.scores_);
        buckets_.swap(merged.buckets_);
        counts_ = std::move(merged.counts_);
        for (auto& ids : other.buckets_) std::vector<Id>{}.swap(ids);
        other.counts_.rebuild(other.buckets_);
        other.players_ = PlayerTable{};
    }

    void audit() const {
        Position total = 0;
        for (Position i = 0; i < scores_.size(); ++i) {
            const auto& ids = buckets_[i];
            for (Position j = 0; j < ids.size(); ++j) {
                if ((j && ids[j - 1] >= ids[j]) || players_.score_of(ids[j]) != scores_[i])
                    throw std::logic_error("Fenwick bucket invariant failed");
            }
            total += ids.size();
            if (counts_.prefix(i + 1) != total)
                throw std::logic_error("Fenwick prefix mismatch");
        }
        if (total != size()) throw std::logic_error("Fenwick player count mismatch");
    }
};

} // namespace leaderboard
