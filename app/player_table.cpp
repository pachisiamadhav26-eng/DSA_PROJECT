#include "player_table.hpp"

#include <limits>
#include <stdexcept>
#include <utility>

using namespace std;

namespace leaderboard {

constexpr size_t minimum_capacity = 16;

PlayerTable::PlayerTable() : slots_(minimum_capacity) {}

uint64_t PlayerTable::hash_id(PlayerId id) {
    uint64_t value = id + 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}

size_t PlayerTable::find_slot(PlayerId id) const {
    const size_t mask = slots_.size() - 1;
    size_t index = static_cast<size_t>(hash_id(id)) & mask;
    for (size_t checked = 0; checked < slots_.size(); ++checked) {
        const Slot& slot = slots_[index];
        if (slot.state == State::Empty) return slots_.size();
        if (slot.state == State::Occupied && slot.id == id) return index;
        index = (index + 1) & mask;
    }
    return slots_.size();
}

bool PlayerTable::insert(PlayerId id, Score score) {
    if (find_slot(id) != slots_.size()) return false;
    const size_t capacity = slots_.size();
    const size_t used_slots = size_ + deleted_count_;
    if ((used_slots + 1) * 10 >= capacity * 7) {
        const bool need_more_room = (size_ + 1) * 10 >= capacity * 7;
        resize_table(need_more_room ? capacity * 2 : capacity);
    }

    const size_t mask = slots_.size() - 1;
    size_t index = static_cast<size_t>(hash_id(id)) & mask;
    const size_t no_slot = slots_.size();
    size_t first_deleted = no_slot;
    while (slots_[index].state != State::Empty) {
        if (slots_[index].state == State::Deleted &&
            first_deleted == no_slot)
            first_deleted = index;
        index = (index + 1) & mask;
    }

    const size_t target = first_deleted == no_slot ? index : first_deleted;
    Slot& destination = slots_[target];
    if (destination.state == State::Deleted) --deleted_count_;
    destination.id = id;
    destination.score = score;
    destination.state = State::Occupied;
    ++size_;
    return true;
}

bool PlayerTable::get(PlayerId id, Score& score) const {
    const size_t index = find_slot(id);
    if (index == slots_.size()) return false;
    score = slots_[index].score;
    return true;
}

bool PlayerTable::update(PlayerId id, Score score) {
    const size_t index = find_slot(id);
    if (index == slots_.size()) return false;
    slots_[index].score = score;
    return true;
}

bool PlayerTable::erase(PlayerId id) {
    const size_t index = find_slot(id);
    if (index == slots_.size()) return false;
    slots_[index].state = State::Deleted;
    --size_;
    ++deleted_count_;

    if (size_ == 0) {
        resize_table(minimum_capacity);
    } else if (slots_.size() > minimum_capacity && size_ * 8 < slots_.size()) {
        resize_table(slots_.size() / 2);
    } else if (deleted_count_ > size_) {
        resize_table(slots_.size());
    }
    return true;
}

void PlayerTable::reserve(size_t expected_players) {
    size_t capacity = slots_.size();
    while (expected_players >= capacity * 7 / 10) {
        if (capacity > numeric_limits<size_t>::max() / 2)
            throw length_error("player table capacity overflow");
        capacity *= 2;
    }
    if (capacity != slots_.size()) resize_table(capacity);
}

size_t PlayerTable::size() const {
    return size_;
}

void PlayerTable::resize_table(size_t capacity) {
    if (capacity < minimum_capacity) capacity = minimum_capacity;
    vector<Slot> previous = move(slots_);
    slots_.assign(capacity, Slot{});
    size_ = 0;
    deleted_count_ = 0;
    const size_t mask = slots_.size() - 1;

    for (const Slot& slot : previous) {
        if (slot.state != State::Occupied) continue;
        size_t index = static_cast<size_t>(hash_id(slot.id)) & mask;
        while (slots_[index].state == State::Occupied)
            index = (index + 1) & mask;
        slots_[index] = slot;
        ++size_;
    }
}

}
