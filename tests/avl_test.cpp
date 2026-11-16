#define AVL_ADVERSARY_TEST
#include "../src/structA/avl.cpp"
#include "replay.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
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

// Tallest AVL that fits the node count, using the generator's own recurrence.
int tallest(std::size_t nodes) {
    std::vector<std::size_t> counts{0, 1};
    while (1 + counts.back() + counts[counts.size()-2] <= nodes)
        counts.push_back(1 + counts.back() + counts[counts.size()-2]);
    return static_cast<int>(counts.size()) - 1;
}

}

int main(int argc, char** argv) {
    using namespace leaderboard;
    if (argc == 3) {
        std::ifstream trace(argv[2]);
        assert(trace.is_open());
        const std::string name = argv[1];
        assert(name == "cascade" || name == "unequal" || name == "churn" ||
               name == "seasons" || name == "errors" || name == "mixed");
        std::stringstream replies;
        run_replay(trace, replies, [](const auto&) { return make_avl(); });
        std::map<std::string,int> errors;
        for (std::string line; std::getline(replies, line); )
            if (line.rfind("ERR ", 0) == 0) ++errors[line];
        assert(errors == expected_errors(name));
        if (name != "cascade" && name != "unequal") return 0;

        // The board holding the most players is the tree the generator built.
        std::map<std::uint64_t,int> added;
        trace.clear();
        trace.seekg(0);
        for (std::string line; std::getline(trace, line); ) {
            std::istringstream parts(line);
            std::string op;
            std::uint64_t board = 0;
            parts >> op;
            if (op == "ADD" && (parts >> board)) ++added[board];
        }
        std::size_t nodes = 0;
        for (const auto& entry : added)
            nodes = std::max(nodes, static_cast<std::size_t>(entry.second));
        if (name == "cascade") {
            assert(adversary_probe.erase_height == tallest(nodes));
            assert(adversary_probe.rotations_before_erase == 0);
            assert(adversary_probe.erase_rotations == 2*((adversary_probe.erase_height-1)/2));
        } else {
            assert(adversary_probe.join_left_height == 0);
            assert(adversary_probe.join_right_height == tallest(nodes));
        }
        return 0;
    }
    assert(argc == 1);

    AvlBoard balanced_child;
    for (Score score : {40, 50, 30, 60, 45})
        balanced_child.insert({static_cast<PlayerId>(score), score});
    assert(height(balanced_child.root) == 3 && adversary_probe.rotations == 0);
    balanced_child.erase({30, 30});
    assert(height(balanced_child.root) == 3 && adversary_probe.rotations == 1);
    assert(audit_avl(balanced_child).size() == 4);

    for (PlayerId cutoff : {7, 8}) {
        AvlBoard tied;
        tied.insert({3, 100});
        tied.insert({8, 90});
        tied.insert({1, 90});
        tied.insert({2, 70});
        const auto [first, second] = avl_split_by_key(tied, {cutoff, 90});
        const auto before_cut = audit_avl(*first);
        const auto at_or_after_cut = audit_avl(*second);
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
    auto saved = a.root->left;
    a.root->left = a.root;
    rejects([&] { audit_avl(a); }); a.root->left = saved;
    auto pieces = avl_split_by_key(a, {31, 31});
    assert(a.size() == 0 && pieces.first->size() == 31 && pieces.second->size() == 32);
    pieces.second->erase({31, 31});
    auto joined = avl_join_ordered(*pieces.first, {31, 31}, *pieces.second);
    assert(pieces.first->size() == 0 && pieces.second->size() == 0);
    assert(audit_avl(*joined).size() == 63 && joined->select(32).id == 31);
    rejects([&] { joined->select(0); });
    rejects([&] { joined->erase({999, 999}); });
    rejects([&] { joined->insert({31, 31}); });
    auto reject_merge = [](Score first, Score second, bool overlap) {
        auto a = make_avl(), b = make_avl();
        a->insert({1, first});
        b->insert({1, second});
        if (overlap) a->insert({2, second - 1});
        bool rejected = false;
        try { a->merge_from(*b); }
        catch (const std::invalid_argument& e) {
            rejected = std::string(e.what()) == "ID_CONFLICT";
        }
        assert(rejected);
        assert(a->size() == (overlap ? 2 : 1) && a->contains({1, first}));
        if (overlap) assert(a->contains({2, second - 1}));
        assert(b->size() == 1 && b->contains({1, second}));
        assert(audit_avl(*a).size() == a->size() && audit_avl(*b).size() == b->size());
    };
    reject_merge(200, 100, false);
    reject_merge(100, 200, false);
    reject_merge(200, 100, true);

    auto high = make_avl(), low = make_avl();
    high->insert({1, 100});
    low->insert({2, 99});
    high->merge_from(*low);
    assert(low->size() == 0 && audit_avl(*high).size() == 2);
    AvlBoard left, right;
    left.insert({0, 10}); right.insert({std::numeric_limits<PlayerId>::max(), -10});
    rejects([&] { avl_join_ordered(left, {5, 20}, left); });
    auto result = avl_join_ordered(left, {5, 0}, right);
    assert(audit_avl(*result).size() == 3 && left.size() == 0 && right.size() == 0);
    std::vector<Player> page{{123, 456}};
    result->range(1, 2, page);
    assert(page.size() == 2 && page[0].id == 0 && page[1].id == 5);
}
