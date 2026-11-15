#include "backend_tools.hpp"
#include <cassert>
#include <limits>

int main() {
    using namespace leaderboard;
    const auto max_id = std::numeric_limits<PlayerId>::max();
    auto board = make_fenwick({100, 80, -5, -100, 80});
    board->insert({max_id, 80});
    board->insert({0, 80});
    board->insert({5, -5});
    assert(board->select(1).id == 0 && board->select(2).id == max_id);
    assert(board->count_strictly_higher(70) == 2);
    auto parts = board->split_by_score(80);
    assert(board->size() == 0 && audit_fenwick(*parts.first).size() == 2);
    parts.second->erase({5, -5});
    parts.second->insert({5, 100});
    parts.second->merge_from(*parts.first);
    assert(parts.first->size() == 0 && audit_fenwick(*parts.second).size() == 3);
    auto donor = make_fenwick({101, 12, 80});
    donor->insert({7, 80});
    parts.second->merge_from(*donor);
    assert(donor->size() == 0 && audit_fenwick(*parts.second).size() == 4);
    assert(parts.second->select(1).id == 5 && parts.second->select(3).id == 7);
    assert(!parts.second->contains({7, 12}) && parts.second->contains({7, 80}));
}
