#ifndef PLAYER_TABLE_H
#define PLAYER_TABLE_H
#include <vector>

struct Player {
    long long id;
    long long score;
};

struct PlayerTable {
    std::vector<std::vector<Player>> buckets;
    int count = 0;
};

Player* findPlayer(PlayerTable& table, long long id);
void addRecord(PlayerTable& table, Player p);  // ID must be new
void removeRecord(PlayerTable& table, long long id);
bool haveCommonId(PlayerTable& a, PlayerTable& b);
void transferRecords(PlayerTable& destination, PlayerTable& source);

#endif
