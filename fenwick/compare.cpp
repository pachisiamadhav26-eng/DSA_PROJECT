#include "leaderboard.h"
#include <chrono>
#include <iostream>
#include <random>
#include <sstream>

long long nowNs() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

int main(int argc, char** argv) {
    int args[] = {10000, 10000, 1024};
    if (argc > 4) return 1;
    for (int i = 1; i < argc; ++i) {
        std::istringstream input(argv[i]);
        char extra;
        if (!(input >> args[i - 1]) || args[i - 1] <= 0 || (input >> extra)) {
            std::cerr << "Use positive integers for players, revisions and possible scores.\n";
            return 1;
        }
    }
    int players = args[0], revisions = args[1], possibleScores = args[2];
    std::vector<long long> scores;
    for (int i = 0; i < possibleScores; ++i) scores.push_back(i);
    Board board;
    initBoard(board, scores);
    std::mt19937 random(253);
    for (int i = 0; i < players; ++i) insertPlayer(board, i, random() % possibleScores);
    long long updateTime = 0, rankTime = 0, selectTime = 0;
    unsigned long long checksum = 0;
    for (int i = 0; i < revisions; ++i) {
        int id = random() % players;
        int score = random() % possibleScores;
        int queryId = random() % players;
        int position = 1 + random() % players;
        long long start = nowNs();
        updateScore(board, id, score);
        updateTime += nowNs() - start;
        start = nowNs();
        int rank = rankOf(board, queryId);
        rankTime += nowNs() - start;
        Player p;
        start = nowNs();
        if (!selectPlayer(board, position, p)) return 1;
        selectTime += nowNs() - start;
        checksum = checksum * 31 + rank;
        checksum = checksum * 31 + p.id;
        checksum = checksum * 31 + p.score;
    }
    Board high;
    long long start = nowNs();
    splitBoard(board, possibleScores / 2, high);
    long long splitTime = nowNs() - start;
    checksum = checksum * 31 + high.players.count;
    start = nowNs();
    mergeBoards(board, high);
    long long mergeTime = nowNs() - start;
    std::cout << "players=" << players << " revisions=" << revisions
              << " coordinates=" << possibleScores << '\n'
              << "Mean update (ns): " << double(updateTime) / revisions << '\n'
              << "Mean rank (ns): " << double(rankTime) / revisions << '\n'
              << "Mean select (ns): " << double(selectTime) / revisions << '\n'
              << "Split (ms): " << splitTime / 1000000.0 << '\n'
              << "Merge (ms): " << mergeTime / 1000000.0 << '\n'
              << "Checksum: " << checksum << '\n';
    clearBoard(board);
    clearBoard(high);
}
