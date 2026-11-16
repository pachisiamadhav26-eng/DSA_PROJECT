#include "../src/structB/fenwick.cpp"
#include <cassert>
#include <limits>
#include <stdexcept>

namespace leaderboard {
namespace {
struct FenwickAuditTest {
    static void check_empty_split(FenwickBoard& board) {
        auto [high, low] = board.split_by_score(1);
        const auto& empty = dynamic_cast<const FenwickBoard&>(*low);
        assert(high->size() == 2 && empty.size() == 0);
        assert(empty.buckets_.capacity() == 0 && empty.bit_.capacity() == 0);
        low->insert({3, 1});
        assert(low->select(1).id == 3);
    }

    static void run(FenwickBoard& f) {
        auto rejected = [&] {
            bool failed = false;
            try { f.audit(); } catch (const std::logic_error&) { failed = true; }
            assert(failed);
        };
        f.bit_[1]++; rejected(); f.bit_[1]--;
        std::swap(f.buckets_[1][0], f.buckets_[1][1]); rejected();
        std::swap(f.buckets_[1][0], f.buckets_[1][1]);
        auto valid = f.scores_;
        f.scores_ = std::make_shared<const std::vector<Score>>(std::vector<Score>{1, 3, 2});
        rejected(); f.scores_ = valid;
        f.total_++; rejected(); f.total_--;
    }
};
}
}

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

    // A merge conflict must preserve both boards, including IDs at different scores.
    for (const std::vector<Score>& scores : {std::vector<Score>{100, 80, -5},
                                            std::vector<Score>{80, 60}}) {
        auto left = make_fenwick({100, 80, -5});
        auto right = make_fenwick(scores);
        left->insert({42, 80});
        right->insert({42, scores.front()});
        bool rejected = false;
        try { left->merge_from(*right); }
        catch (const std::invalid_argument&) { rejected = true; }
        assert(rejected);
        assert(left->size() == 1 && right->size() == 1);
        assert(audit_fenwick(*left).front().id == 42);
        assert(audit_fenwick(*right).front().score == scores.front());
    }
    FenwickBoard internal(compress({1, 2, 3}));
    internal.insert({1, 2}); internal.insert({2, 2});
    assert(internal.audit().size() == 2);
    FenwickAuditTest::run(internal);
    auto rejects = [](auto operation) {
        bool failed = false;
        try { operation(); } catch (const std::exception&) { failed = true; }
        assert(failed);
    };
    rejects([&] { internal.insert({3, 99}); });
    assert(internal.size() == 2);
    rejects([&] { internal.select(0); });
    rejects([&] { internal.erase({1, 3}); });
    FenwickAuditTest::check_empty_split(internal);

}
