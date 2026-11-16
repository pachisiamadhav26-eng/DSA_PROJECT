#pragma once

#include "leaderboard.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace leaderboard {

class PlayerTable {
public:
    PlayerTable();

    bool insert(PlayerId id, Score score);
    bool get(PlayerId id, Score& score) const;
    bool update(PlayerId id, Score score);
    bool erase(PlayerId id);
    void reserve(std::size_t expected_players);
    std::size_t size() const;

private:
    enum class State { Empty, Occupied, Deleted };

    struct Slot {
        PlayerId id = 0;
        Score score = 0;
        State state = State::Empty;
    };

    std::vector<Slot> slots_;
    std::size_t size_ = 0;
    std::size_t deleted_count_ = 0;

    static std::uint64_t hash_id(PlayerId id);
    std::size_t find_slot(PlayerId id) const;
    void resize_table(std::size_t capacity);
};

}
