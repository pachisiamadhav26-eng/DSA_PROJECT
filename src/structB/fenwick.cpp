#include "backend_tools.hpp"

#include <algorithm>
#include <functional>
#include <iterator>
#include <stdexcept>
#include <unordered_set>

namespace leaderboard {
namespace {

using Coordinates = std::shared_ptr<const std::vector<Score>>;
using Bucket = std::vector<PlayerId>;

std::size_t low_bit(std::size_t i) { return i & (~i + 1); }

Coordinates compress(const std::vector<Score>& scores) {
    auto result = std::make_shared<std::vector<Score>>(scores);
    std::sort(result->begin(), result->end(), std::greater<Score>());
    result->erase(std::unique(result->begin(), result->end()), result->end());
    return result;
}

Bucket join_bucket(const Bucket& first, const Bucket& second) {
    Bucket result;
    result.reserve(first.size() + second.size());
    std::merge(first.begin(), first.end(), second.begin(), second.end(),
               std::back_inserter(result));
    return result;
}

struct FenwickBoard final : Board {
    explicit FenwickBoard(Coordinates scores) : scores_(std::move(scores)) {}

    Position size() const override { return total_; }

    bool contains(Player p) const override {
        if (total_ == 0) return false;
        auto i = lower(p.score);
        return i != scores_->size() && (*scores_)[i] == p.score &&
               std::binary_search(buckets_[i].begin(), buckets_[i].end(), p.id);
    }

    void insert(Player p) override {
        auto i = exact(p.score);
        prepare();
        auto& bucket = buckets_[i];
        auto place = std::lower_bound(bucket.begin(), bucket.end(), p.id);
        if (place != bucket.end() && *place == p.id)
            throw std::invalid_argument("duplicate Fenwick key");
        auto shifts = bucket.end() - place;
        bucket.insert(place, p.id);
        shifts_ += static_cast<std::uint64_t>(shifts);
        for (++i; i < bit_.size(); i += low_bit(i)) ++bit_[i];
        ++total_;
    }

    void erase(Player p) override {
        auto i = exact(p.score);
        if (total_ == 0) throw std::invalid_argument("missing Fenwick key");
        auto& bucket = buckets_[i];
        auto place = std::lower_bound(bucket.begin(), bucket.end(), p.id);
        if (place == bucket.end() || *place != p.id)
            throw std::invalid_argument("missing Fenwick key");
        shifts_ += static_cast<std::uint64_t>(bucket.end() - place - 1);
        bucket.erase(place);
        for (++i; i < bit_.size(); i += low_bit(i)) --bit_[i];
        --total_;
    }

    Position count_strictly_higher(Score score) const override {
        return total_ ? prefix(lower(score)) : 0;
    }

    Player select(Position k) const override {
        if (k == 0 || k > total_) throw std::out_of_range("Fenwick position");
        std::size_t n = scores_->size();
        std::size_t step = 1;
        while (step <= n / 2) step *= 2;
        std::size_t skipped = 0;
        for (; step; step /= 2) {
            if (step > n - skipped) continue;
            auto next = skipped + step;
            if (bit_[next] < k) {
                k -= bit_[next];
                skipped = next;
            }
        }
        // skipped counts whole score buckets; k is the position within the next.
        return {buckets_[skipped][static_cast<std::size_t>(k - 1)], (*scores_)[skipped]};
    }

    void range(Position first, Position last, std::vector<Player>& out) const override {
        if (first == 0 || first > last || last > total_)
            throw std::out_of_range("Fenwick range");
        out.reserve(static_cast<std::size_t>(last - first + 1));
        out.clear();
        for (Position k = first; ; ++k) {
            out.push_back(select(k));
            if (k == last) break;
        }
    }

    std::pair<std::unique_ptr<Board>, std::unique_ptr<Board>>
    split_by_score(Score threshold) override {
        auto high = std::make_unique<FenwickBoard>(scores_);
        auto low = std::make_unique<FenwickBoard>(scores_);
        if (total_ == 0) return {std::move(high), std::move(low)};
        high->prepare();
        low->buckets_.swap(buckets_);
        low->bit_.swap(bit_);
        total_ = 0;
        for (std::size_t i = 0; i < scores_->size() && (*scores_)[i] >= threshold; ++i)
            high->buckets_[i].swap(low->buckets_[i]);
        high->rebuild();
        low->rebuild();
        return {std::move(high), std::move(low)};
    }

    void merge_from(Board& other) override {
        auto* donor = dynamic_cast<FenwickBoard*>(&other);
        if (!donor || donor == this) throw std::invalid_argument("Fenwick merge backend or self");
        // Validate IDs before transferring buckets, including conflicts at different scores.
        std::unordered_set<PlayerId> existing;
        for (const auto& bucket : buckets_)
            existing.insert(bucket.begin(), bucket.end());
        for (const auto& bucket : donor->buckets_)
            for (PlayerId id : bucket)
                if (existing.count(id)) throw std::invalid_argument("ID_CONFLICT");
        if (scores_ == donor->scores_ || *scores_ == *donor->scores_) {
            merge_same_coordinates(*donor);
        } else {
            auto scores = std::make_shared<std::vector<Score>>();
            std::set_union(scores_->begin(), scores_->end(),
                           donor->scores_->begin(), donor->scores_->end(),
                           std::back_inserter(*scores), std::greater<Score>());
            FenwickBoard result(scores);
            if (total_ || donor->total_) {
                result.prepare();
                std::size_t i = 0, j = 0;
                for (std::size_t k = 0; k < scores->size(); ++k) {
                    Score score = (*scores)[k];
                    const Bucket* a = bucket_at(score, i);
                    const Bucket* b = donor->bucket_at(score, j);
                    if (a && b) result.buckets_[k] = join_bucket(*a, *b);
                    else if (a) result.buckets_[k] = *a;
                    else if (b) result.buckets_[k] = *b;
                }
                result.rebuild();
            }
            scores_ = std::move(result.scores_);
            buckets_.swap(result.buckets_);
            bit_.swap(result.bit_);
            total_ = result.total_;
            donor->clear_counts();
        }
    }

    FenwickStats stats() const { return {scores_->size(), shifts_}; }

    std::vector<Player> audit() const {
        if (!std::is_sorted(scores_->begin(), scores_->end(), std::greater<Score>()) ||
            std::adjacent_find(scores_->begin(), scores_->end()) != scores_->end())
            throw std::logic_error("Fenwick coordinate ordering");
        if (buckets_.empty() && bit_.empty() && total_ == 0) return {};
        if (scores_->size() != buckets_.size() || bit_.size() != buckets_.size() + 1 ||
            bit_[0] != 0) throw std::logic_error("Fenwick array dimensions");
        std::vector<Player> result;
        std::vector<Position> expected(bit_.size(), 0);
        std::unordered_set<PlayerId> ids;
        result.reserve(static_cast<std::size_t>(total_));
        for (std::size_t i = 0; i < buckets_.size(); ++i) {
            const auto& bucket = buckets_[i];
            for (std::size_t j = 0; j < bucket.size(); ++j) {
                if ((j && bucket[j - 1] >= bucket[j]) || !ids.insert(bucket[j]).second)
                    throw std::logic_error("Fenwick ID order or uniqueness");
                result.push_back({bucket[j], (*scores_)[i]});
            }
            expected[i + 1] = expected[i] + bucket.size();
            std::size_t k = i + 1;
            if (bit_[k] != expected[k] - expected[k - low_bit(k)])
                throw std::logic_error("Fenwick block count");
        }
        if (expected.back() != total_ || prefix(scores_->size()) != total_)
            throw std::logic_error("Fenwick total count");
        return result;
    }

private:
    Coordinates scores_;
    std::vector<Bucket> buckets_;
    std::vector<Position> bit_;
    Position total_ = 0;
    std::uint64_t shifts_ = 0;

    std::size_t lower(Score score) const {
        return static_cast<std::size_t>(std::lower_bound(scores_->begin(), scores_->end(),
            score, std::greater<Score>()) - scores_->begin());
    }

    std::size_t exact(Score score) const {
        auto i = lower(score);
        if (i == scores_->size() || (*scores_)[i] != score)
            throw std::invalid_argument("score absent from Fenwick coordinates");
        return i;
    }

    Position prefix(std::size_t i) const {
        Position sum = 0;
        for (; i; i -= low_bit(i)) sum += bit_[i];
        return sum;
    }

    void prepare() {
        if (!bit_.empty()) return;
        std::vector<Bucket> buckets(scores_->size());
        std::vector<Position> bit(scores_->size() + 1);
        buckets.swap(buckets_);
        bit.swap(bit_);
    }

    void rebuild() {
        std::fill(bit_.begin(), bit_.end(), 0);
        total_ = 0;
        for (std::size_t i = 1; i < bit_.size(); ++i) {
            Position count = buckets_[i - 1].size();
            total_ += count;
            bit_[i] += count;
            auto parent = i + low_bit(i);
            if (parent < bit_.size()) bit_[parent] += bit_[i];
        }
    }

    void clear_counts() {
        std::vector<Bucket>().swap(buckets_);
        std::vector<Position>().swap(bit_);
        total_ = 0;
    }

    const Bucket* bucket_at(Score score, std::size_t& index) const {
        if (index == scores_->size() || (*scores_)[index] != score) return nullptr;
        auto i = index++;
        return total_ && !buckets_[i].empty() ? &buckets_[i] : nullptr;
    }

    void merge_same_coordinates(FenwickBoard& donor) {
        if (!donor.total_) return;
        if (!total_) {
            clear_counts();
            buckets_.swap(donor.buckets_);
            bit_.swap(donor.bit_);
            total_ = donor.total_;
            donor.total_ = 0;
            return;
        }
        std::vector<std::pair<std::size_t, Bucket>> overlap;
        for (std::size_t i = 0; i < buckets_.size(); ++i)
            if (!buckets_[i].empty() && !donor.buckets_[i].empty())
                overlap.emplace_back(i, join_bucket(buckets_[i], donor.buckets_[i]));
        for (std::size_t i = 0; i < buckets_.size(); ++i)
            if (buckets_[i].empty()) buckets_[i].swap(donor.buckets_[i]);
        for (auto& entry : overlap) buckets_[entry.first].swap(entry.second);
        rebuild();
        donor.clear_counts();
    }
};

}

std::unique_ptr<Board> make_fenwick(const std::vector<Score>& scores) {
    return std::make_unique<FenwickBoard>(compress(scores));
}

FenwickStats fenwick_stats(const Board& board) {
    return dynamic_cast<const FenwickBoard&>(board).stats();
}

std::vector<Player> audit_fenwick(const Board& board) {
    return dynamic_cast<const FenwickBoard&>(board).audit();
}

}
