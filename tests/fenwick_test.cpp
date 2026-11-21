#include "backend_tools.hpp"

#include <cassert>
#include <vector>

using namespace leaderboard;

int main() {
    auto board = make_fenwick({100, 90, 70, -5, 110});
    board->insert({5, 100});
    board->insert({3, 90});
    board->insert({8, 90});
    board->insert({2, 70});

    auto players = audit_fenwick(*board);
    assert(players.size() == 4);
    assert(players[0].id == 5 && players[1].id == 3);
    assert(players[2].id == 8 && players[3].id == 2);
    assert(board->count_strictly_higher(90) == 1);
    assert(board->select(3).id == 8);

    board->erase({8, 90});
    board->insert({8, 110});
    players = audit_fenwick(*board);
    assert(players[0].id == 8);
    assert(board->count_strictly_higher(70) == 3);

    auto parts = board->split_by_score(90);
    assert(board->size() == 0);
    assert(audit_fenwick(*parts.first).size() == 3);
    assert(audit_fenwick(*parts.second).size() == 1);
    parts.first->merge_from(*parts.second);
    assert(parts.second->size() == 0);
    assert(audit_fenwick(*parts.first).size() == 4);

    parts.first->erase({2, 70});
    parts.first->insert({2, -5});
    players = audit_fenwick(*parts.first);
    assert(players.back().id == 2 && players.back().score == -5);
}
