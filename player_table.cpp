#include "player_table.h"

// This table belongs to the application. The two trees use scores for ordering.
int bucketNumber(long long id, int bucketCount) {
    int index = int(id % bucketCount);
    if (index < 0) index += bucketCount;
    return index;
}

Player* findPlayer(PlayerTable& table, long long id) {
    if (table.buckets.empty()) return nullptr;
    int i = bucketNumber(id, int(table.buckets.size()));
    for (Player& player : table.buckets[i]) {
        if (player.id == id) return &player;
    }
    return nullptr;
}

void addRecord(PlayerTable& table, Player p) {
    if (table.buckets.empty()) table.buckets.resize(8);
    if (table.count == int(table.buckets.size())) {
        int newSize = 2 * table.count;
        std::vector<std::vector<Player>> larger(newSize);
        for (const std::vector<Player>& bucket : table.buckets) {
            for (Player old : bucket)
                larger[bucketNumber(old.id, newSize)].push_back(old);
        }
        table.buckets.swap(larger);
    }
    int i = bucketNumber(p.id, int(table.buckets.size()));
    table.buckets[i].push_back(p);
    ++table.count;
}

void removeRecord(PlayerTable& table, long long id) {
    if (table.buckets.empty()) return;
    int i = bucketNumber(id, int(table.buckets.size()));
    std::vector<Player>& bucket = table.buckets[i];
    for (int j = 0; j < int(bucket.size()); ++j) {
        if (bucket[j].id == id) {
            bucket[j] = bucket.back();
            bucket.pop_back();
            --table.count;
            return;
        }
    }
}

bool haveCommonId(PlayerTable& a, PlayerTable& b) {
    for (const std::vector<Player>& bucket : b.buckets)
        for (Player p : bucket)
            if (findPlayer(a, p.id) != nullptr) return true;
    return false;
}

void transferRecords(PlayerTable& destination, PlayerTable& source) {
    for (const std::vector<Player>& bucket : source.buckets)
        for (Player p : bucket) addRecord(destination, p);
    source = PlayerTable{};
}
