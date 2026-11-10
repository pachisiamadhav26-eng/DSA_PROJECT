#include "leaderboard.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <random>

void check(bool correct, const char* message) {
    if (!correct) {
        std::cerr << "Failed: " << message << '\n';
        std::exit(1);
    }
}

bool oracleOrder(Player a, Player b) {
    if (a.score == b.score) return a.id < b.id;
    return a.score > b.score;
}

void checkStructure(const Board& board) {
    int total = 0;
    for (int i = 0; i < int(board.ids.size()); ++i) {
        total += int(board.ids[i].size());
        check(prefixSum(board.tree, i + 1) == total, "Fenwick prefix");
    }
    check(total == board.players.count, "Fenwick count");
}

void verify(Board& board, std::vector<Player> list) {
    checkStructure(board);
    std::sort(list.begin(), list.end(), oracleOrder);
    check(board.players.count == int(list.size()), "player count");
    std::vector<Player> page = listPositions(board, 1, int(list.size()));
    check(page.size() == list.size(), "page length");
    int rank = 0;
    for (int i = 0; i < int(list.size()); ++i) {
        if (i == 0 || list[i].score != list[i - 1].score) rank = i + 1;
        check(rankOf(board, list[i].id) == rank, "competition rank");
        Player p;
        check(selectPlayer(board, i + 1, p), "valid position");
        check(p.id == list[i].id && p.score == list[i].score, "selected player");
        check(page[i].id == p.id && page[i].score == p.score, "page order");
    }
    Player unused;
    check(!selectPlayer(board, 0, unused), "position zero");
    check(!selectPlayer(board, int(list.size()) + 1, unused), "past last position");
}

int main() {
    std::vector<long long> scores = {-20, -1, 0, 10, 25, 100, 500};
    Board board;
    initBoard(board, scores);
    std::vector<Player> list;
    std::mt19937 random(253);
    for (int step = 0; step < 3000; ++step) {
        long long id = int(random() % 80) - 40;
        long long score = scores[random() % scores.size()];
        int i = 0;
        while (i < int(list.size()) && list[i].id != id) ++i;
        if (i == int(list.size())) {
            check(insertPlayer(board, id, score), "insert");
            list.push_back({id, score});
        } else if (random() % 3 == 0) {
            check(erasePlayer(board, id), "erase");
            list.erase(list.begin() + i);
        } else {
            check(updateScore(board, id, score), "update");
            list[i].score = score;
        }
        verify(board, list);
        if (step % 30 == 0) {
            long long threshold = scores[random() % scores.size()];
            std::vector<Player> lowList, highList;
            for (Player p : list) {
                if (p.score < threshold) lowList.push_back(p);
                else highList.push_back(p);
            }
            Board high;
            check(splitBoard(board, threshold, high), "split");
            verify(board, lowList);
            verify(high, highList);
            check(mergeBoards(board, high), "merge split divisions");
            verify(board, list);
            verify(high, {});
            clearBoard(high);
        }
    }
    clearBoard(board);
    Board other;
    initBoard(board, {5, 20, 100});
    initBoard(other, {5, 50});
    insertPlayer(board, 1, 100);
    insertPlayer(board, 3, 5);
    insertPlayer(other, 1, 50);
    check(!mergeBoards(board, other), "duplicate ID in two leagues");
    verify(board, {{1,100}, {3,5}});
    verify(other, {{1,50}});
    erasePlayer(other, 1);
    insertPlayer(other, 2, 50);
    insertPlayer(other, 4, 5);
    check(mergeBoards(board, other), "interleaved leagues");
    verify(board, {{1,100}, {2,50}, {3,5}, {4,5}});
    check(!insertPlayer(board, 1, 5), "duplicate insert");
    check(!erasePlayer(board, 99) && !updateScore(board, 99, 5), "missing ID");
    clearBoard(board);
    clearBoard(other);
    std::cout << "Passed the sorted-list checks, including 3000 updates/inserts/deletes.\n";
}
