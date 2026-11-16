#include "player_table.hpp"
#include <bits/stdc++.h>

using namespace std;
using namespace leaderboard;

void test_growth_and_deletion() {
    PlayerTable growing;
    for (uint64_t id = 0; id < 10000; ++id)
        assert(growing.insert(id * 65537, static_cast<Score>(id)));
    for (uint64_t id = 0; id < 10000; id += 2)
        assert(growing.erase(id * 65537));
    for (uint64_t id = 0; id < 10000; ++id) {
        Score actual_score = 0;
        bool found = growing.get(id * 65537, actual_score);
        assert(found == (id % 2 != 0));
        if (found) assert(actual_score == static_cast<Score>(id));
    }
    for (uint64_t id = 0; id < 10000; id += 2)
        assert(growing.insert(id * 65537, -static_cast<Score>(id)));
    assert(growing.size() == 10000);
}

void test_random_operations() {
    PlayerTable table;
    unordered_map<PlayerId, Score> expected;
    table.reserve(100000);

    mt19937_64 random(0x253253ULL);
    for (size_t step = 0; step < 200000; ++step) {
        PlayerId id = random() % 50000;
        Score score = static_cast<Score>(random() % 200001) - 100000;
        unsigned operation = static_cast<unsigned>(random() % 4);

        if (operation == 0) {
            bool actual = table.insert(id, score);
            bool wanted = expected.emplace(id, score).second;
            assert(actual == wanted);
        } else if (operation == 1) {
            bool actual = table.update(id, score);
            auto found = expected.find(id);
            bool wanted = found != expected.end();
            if (wanted) found->second = score;
            assert(actual == wanted);
        } else if (operation == 2) {
            bool actual = table.erase(id);
            bool wanted = expected.erase(id) != 0;
            assert(actual == wanted);
        } else {
            Score actual_score = 0;
            bool actual = table.get(id, actual_score);
            auto found = expected.find(id);
            bool wanted = found != expected.end();
            assert(actual == wanted);
            if (wanted) assert(actual_score == found->second);
        }

        assert(table.size() == expected.size());
    }

    table.reserve(200000);
    for (const auto& entry : expected) {
        Score actual_score = 0;
        assert(table.get(entry.first, actual_score));
        assert(actual_score == entry.second);
    }

    assert(table.insert(0, -1) == (expected.emplace(0, -1).second));
    Score zero_score = 0;
    assert(table.get(0, zero_score));
    assert(zero_score == expected.at(0));
    assert(table.size() == expected.size());
}

int main() {
    test_growth_and_deletion();
    test_random_operations();
}
