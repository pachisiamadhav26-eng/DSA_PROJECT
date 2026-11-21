#define AVL_ADVERSARY_TEST
#include "../src/structA/avl.cpp"
#include "replay.hpp"
#include <cassert>
#include <fstream>
#include <limits>
#include <map>
#include <sstream>
#include <string>

namespace {
    // Each adversary case must produce exactly these errors, so a case cannot drift.
    std::map<std::string,int> expected_errors(const std::string& name) {
        if (name == "churn")   return {{"ERR BAD_POSITION", 1}};
        if (name == "seasons") return {{"ERR UNKNOWN_BOARD", 3}};
        if (name == "errors")  return {{"ERR BAD_COMMAND", 7}, {"ERR BAD_POSITION", 3},
                                    {"ERR BAD_RANGE", 2}, {"ERR DUPLICATE_ID", 2},
                                    {"ERR SAME_BOARD", 1}, {"ERR UNKNOWN_BOARD", 1},
                                    {"ERR UNKNOWN_PLAYER", 2}, {"ERR WRONG_BOARD", 1}};
        return {};
    }
}

int main(int argc, char** argv) {
    using namespace leaderboard;
    if (argc == 3) {
        std::ifstream trace(argv[2]);
        assert(trace.is_open());
        const std::string name = argv[1];
        std::stringstream replies;
        run_replay(trace, replies, [](const auto&) { return make_avl(); });
        std::map<std::string,int> errors;
        for (std::string line; std::getline(replies, line); )
            if (line.rfind("ERR ", 0) == 0) ++errors[line];
        assert(errors == expected_errors(name));
        return 0;
    }
    assert(argc == 1);

    AvlBoard balanced_child;
    for (Score score : {40, 50, 30, 60, 45})
        balanced_child.insert({static_cast<PlayerId>(score), score});
    assert(height(balanced_child.root) == 3 && avl_stats(balanced_child).rotations == 0);
    balanced_child.erase({30, 30});
    assert(height(balanced_child.root) == 3 && avl_stats(balanced_child).rotations == 1);
    assert(audit_avl(balanced_child).size() == 4);

    for (PlayerId cutoff : {7, 8}) {
        AvlBoard tied;
        tied.insert({3, 100});
        tied.insert({8, 90});
        tied.insert({1, 90});
        tied.insert({2, 70});
        auto parts = avl_split_by_key(tied, {cutoff, 90});
        const auto before_cut = audit_avl(*parts.first);
        const auto at_or_after_cut = audit_avl(*parts.second);
        assert(tied.size() == 0);
        assert(before_cut.size() == 2 && before_cut[0].id == 3 && before_cut[1].id == 1);
        assert(at_or_after_cut.size() == 2 && at_or_after_cut[0].id == 8 && at_or_after_cut[1].id == 2);
    }

    auto rejects = [](auto operation) {
        bool failed = false;
        try { operation(); } catch (const std::exception&) { failed = true; }
        assert(failed);
    };
    AvlBoard a;
    for (int i = 0; i < 63; i++) a.insert({static_cast<PlayerId>(i), i});
    assert(audit_avl(a).size() == 63);
    a.root->size++;
    rejects([&] { audit_avl(a); }); a.root->size--;
    a.root->height++;
    rejects([&] { audit_avl(a); }); a.root->height--;
    auto old = a.root->left->player;
    a.root->left->player = a.root->player;
    rejects([&] { audit_avl(a); }); a.root->left->player = old;
    auto pieces = avl_split_by_key(a, {31, 31});
    assert(a.size() == 0 && pieces.first->size() == 31 && pieces.second->size() == 32);
    pieces.second->erase({31, 31});
    auto joined = avl_join_ordered(*pieces.first, {31, 31}, *pieces.second);
    assert(pieces.first->size() == 0 && pieces.second->size() == 0);
    assert(audit_avl(*joined).size() == 63 && joined->select(32).id == 31);
    rejects([&] { joined->select(0); });
    rejects([&] { joined->erase({999, 999}); });
    rejects([&] { joined->insert({31, 31}); });
    assert(audit_avl(*joined).size() == 63 && joined->select(32).id == 31);

    AvlBoard two_children;
    for (PlayerId id : {50, 80, 20, 90, 60, 30, 10, 65})
        two_children.insert({id, static_cast<Score>(id)});
    assert(two_children.root->player.id == 50);
    two_children.erase({50, 50});
    const auto remaining = audit_avl(two_children);
    assert(remaining.size() == 7 && remaining[2].id == 65 && remaining[3].id == 60);

    auto high = make_avl(), low = make_avl();
    high->insert({1, 100});
    low->insert({2, 99});
    high->merge_from(*low);
    assert(low->size() == 0 && audit_avl(*high).size() == 2);
    AvlBoard left, right;
    left.insert({0, 10}); right.insert({std::numeric_limits<PlayerId>::max(), -10});
    rejects([&] { avl_join_ordered(left, {5, 20}, left); });
    auto result = avl_join_ordered(left, {5, 0}, right);
    assert(avl_stats(*result).last_join_visits == 1);
    assert(audit_avl(*result).size() == 3 && left.size() == 0 && right.size() == 0);
    std::vector<Player> page{{123, 456}};
    result->range(1, 2, page);
    assert(page.size() == 2 && page[0].id == 0 && page[1].id == 5);

    const auto max_score = std::numeric_limits<Score>::max();
    AvlBoard odds, evens;
    odds.insert({1, max_score});
    odds.insert({3, max_score});
    evens.insert({0, max_score});
    evens.insert({2, max_score});
    evens.insert({4, std::numeric_limits<Score>::min()});
    odds.merge_from(evens);
    auto parts = odds.split_by_score(max_score);
    assert(odds.size() == 0 && evens.size() == 0);
    const auto ties = audit_avl(*parts.first);
    assert(ties.size() == 4 && parts.second->size() == 1 && parts.second->select(1).id == 4);
    for (PlayerId id = 0; id < ties.size(); id++) assert(ties[id].id == id);
}
