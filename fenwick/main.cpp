#include "leaderboard.h"
#include <iostream>

int main() {
    Board board;
    initBoard(board, {40, 70, 90, 100});
    insertPlayer(board, 1, 100);
    insertPlayer(board, 2, 90);
    insertPlayer(board, 3, 90);
    insertPlayer(board, 4, 70);
    insertPlayer(board, 5, 40);

    std::cout << "ID  Score  Rank\n";
    for (Player p : listPositions(board, 1, 5))
        std::cout << p.id << "   " << p.score << "     " << rankOf(board, p.id) << '\n';

    updateScore(board, 4, 100);
    erasePlayer(board, 5);
    Player p;
    if (selectPlayer(board, 2, p))
        std::cout << "After the update, position 2 is player " << p.id << '\n';

    Board high;
    splitBoard(board, 100, high);
    std::cout << "Below 100: " << board.players.count << '\n';
    std::cout << "At or above 100: " << high.players.count << '\n';
    mergeBoards(board, high);
    std::cout << "Merged players: " << board.players.count << '\n';
    clearBoard(board);
    clearBoard(high);
}
