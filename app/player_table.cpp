#include "player_table.hpp"

#include <limits>
#include <stdexcept>
#include <utility>

namespace leaderboard {

namespace {

constexpr std::size_t minimum_capacity = 16;
constexpr std::size_t missing_index = std::numeric_limits<std::size_t>::max();

}

PlayerTable::PlayerTable() : slots_(minimum_capacity) {}

std::uint64_t PlayerTable::mix(PlayerId id) {
    std::uint64_t value = id + 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}

std::size_t PlayerTable::find_index(PlayerId id) const {
    const std::size_t mask = slots_.size() - 1;
    std::size_t index = static_cast<std::size_t>(mix(id)) & mask;
    for (std::size_t checked = 0; checked < slots_.size(); checked++) {
        const Slot& slot = slots_[index];
        if (slot.state == State::Empty) return missing_index;
        if (slot.state == State::Occupied && slot.id == id) return index;
        index = (index + 1) & mask;
    }
    return missing_index;
}

bool PlayerTable::insert(PlayerId id, Score score) {
    if (find_index(id) != missing_index) return false;
    const std::size_t capacity = slots_.size();
    if ((size_ + deleted_ + 1) * 10 >= capacity * 7) {
        const bool grow = (size_ + 1) * 10 >= capacity * 7;
        rehash(grow ? capacity * 2 : capacity);
    }

    const std::size_t mask = slots_.size() - 1;
    std::size_t index = static_cast<std::size_t>(mix(id)) & mask;
    std::size_t first_deleted = missing_index;
    for (;;) {
        Slot& slot = slots_[index];
        if (slot.state == State::Deleted && first_deleted == missing_index)
            first_deleted = index;
        if (slot.state == State::Empty) {
            std::size_t target = first_deleted == missing_index ? index : first_deleted;
            Slot& destination = slots_[target];
            if (destination.state == State::Deleted) --deleted_;
            destination = {id, score, State::Occupied};
            ++size_;
            return true;
        }
        index = (index + 1) & mask;
    }
}

bool PlayerTable::get(PlayerId id, Score& score) const {
    const std::size_t index = find_index(id);
    if (index == missing_index) return false;
    score = slots_[index].score;
    return true;
}

bool PlayerTable::update(PlayerId id, Score score) {
    const std::size_t index = find_index(id);
    if (index == missing_index) return false;
    slots_[index].score = score;
    return true;
}

bool PlayerTable::erase(PlayerId id) {
    const std::size_t index = find_index(id);
    if (index == missing_index) return false;
    slots_[index].state = State::Deleted;
    --size_;
    ++deleted_;

    if (size_ == 0) rehash(minimum_capacity);
    else if (slots_.size() > minimum_capacity && size_ * 8 < slots_.size())
        rehash(slots_.size() / 2);
    else if (deleted_ > size_) rehash(slots_.size());
    return true;
}

void PlayerTable::reserve(std::size_t expected_players) {
    std::size_t capacity = slots_.size();
    while (expected_players >= capacity * 7 / 10) {
        if (capacity > std::numeric_limits<std::size_t>::max() / 2)
            throw std::length_error("player table capacity overflow");
        capacity *= 2;
    }
    if (capacity != slots_.size()) rehash(capacity);
}

std::size_t PlayerTable::size() const {
    return size_;
}

void PlayerTable::rehash(std::size_t capacity) {
    std::vector<Slot> replacement(capacity);
    const std::size_t mask = capacity - 1;
    for (const Slot& slot : slots_) {
        if (slot.state != State::Occupied) continue;
        std::size_t index = static_cast<std::size_t>(mix(slot.id)) & mask;
        while (replacement[index].state == State::Occupied)
            index = (index + 1) & mask;
        replacement[index] = slot;
    }
    slots_.swap(replacement);
    deleted_ = 0;
}

}
