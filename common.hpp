#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

namespace leaderboard {

using Id = std::int64_t;
using Score = std::int64_t;
using Position = std::size_t;

struct Player {
    Id id;
    Score score;
};

inline bool operator==(Player a, Player b) {
    return a.id == b.id && a.score == b.score;
}

// Leaderboard traversal order. Comparing directly also handles INT64_MIN.
inline bool before(Player a, Player b) {
    return a.score != b.score ? a.score > b.score : a.id < b.id;
}

inline void check_range(Position first, Position last, Position size) {
    if (first == 0 || first > last || last > size)
        throw std::out_of_range("positions must satisfy 1 <= first <= last <= size");
}

// Handwritten separate-chaining hash table, shared by both implementations.
class PlayerTable {
    std::vector<std::vector<Player>> chains_;
    Position count_ = 0;

    static std::uint64_t hash_id(Id id) {
        std::uint64_t x = static_cast<std::uint64_t>(id);
        x += 0x9e3779b97f4a7c15ULL;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        return x ^ (x >> 31);
    }

    void grow() {
        const Position capacity = chains_.empty() ? 8 : 2 * chains_.size();
        std::vector<std::vector<Player>> next(capacity);
        each([&](Player p) { next[hash_id(p.id) % capacity].push_back(p); });
        chains_.swap(next);
    }

public:
    Position size() const { return count_; }

    template<class Visit>
    void each(Visit visit) const {
        for (const auto& chain : chains_)
            for (Player p : chain) visit(p);
    }

    const Player* find(Id id) const {
        if (chains_.empty()) return nullptr;
        const auto& chain = chains_[hash_id(id) % chains_.size()];
        for (const auto& p : chain)
            if (p.id == id) return &p;
        return nullptr;
    }

    Score score_of(Id id) const {
        if (const auto* p = find(id)) return p->score;
        throw std::out_of_range("unknown player ID");
    }

    void insert(Player p) {
        if (find(p.id)) throw std::invalid_argument("duplicate player ID");
        if (count_ >= chains_.size()) grow();
        chains_[hash_id(p.id) % chains_.size()].push_back(p);
        ++count_;
    }

    void set_score(Id id, Score score) {
        if (!chains_.empty()) {
            for (auto& p : chains_[hash_id(id) % chains_.size()]) {
                if (p.id == id) { p.score = score; return; }
            }
        }
        throw std::out_of_range("unknown player ID");
    }

    bool erase(Id id) {
        if (chains_.empty()) return false;
        auto& chain = chains_[hash_id(id) % chains_.size()];
        for (auto& p : chain) {
            if (p.id == id) {
                p = chain.back();
                chain.pop_back();
                --count_;
                return true;
            }
        }
        return false;
    }

    // Duplicate IDs are rejected before modifying either league.
    void append_disjoint(const PlayerTable& other) {
        other.each([&](Player p) {
            if (find(p.id)) throw std::invalid_argument("leagues share a player ID");
        });
        try {
            other.each([&](Player p) { insert(p); });
        } catch (...) {
            other.each([&](Player p) { erase(p.id); });
            throw;
        }
    }
};

} // namespace leaderboard
