#include "backend_tools.hpp"
#include <cassert>
#include <limits>

int main() {
    using namespace leaderboard;
    auto board = make_avl();
    const auto max_id = std::numeric_limits<PlayerId>::max();
    for (auto id : {max_id, PlayerId{7}, PlayerId{0}, PlayerId{8}})
        board->insert({id, 10});
    auto parts = avl_split_by_key(*board, {8, 10});
    assert(board->size() == 0);
    assert(audit_avl(*parts.first).size() == 2);
    assert(parts.first->select(2).id == 7);
    assert(audit_avl(*parts.second).size() == 2);
    assert(parts.second->select(1).id == 8);
    parts.second->erase({8, 10});
    auto rejoined = avl_join_ordered(*parts.first, {8, 10}, *parts.second);
    assert(parts.first->size() == 0 && parts.second->size() == 0);
    assert(audit_avl(*rejoined).size() == 4 && rejoined->select(4).id == max_id);
    auto other = make_avl();
    other->insert({4, 11});
    other->insert({5, 9});
    rejoined->merge_from(*other);
    assert(audit_avl(*rejoined).size() == 6 && other->size() == 0);
    assert(rejoined->select(1).id == 4 && rejoined->select(6).id == 5);
    rejoined->erase({7, 10});
    assert(audit_avl(*rejoined).size() == 5 && !rejoined->contains({7, 10}));
}
