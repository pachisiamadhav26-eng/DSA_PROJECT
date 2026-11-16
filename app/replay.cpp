#include "replay.hpp"

#include "player_table.hpp"

#include <bits/stdc++.h>

using namespace std;

namespace leaderboard {

namespace {

using BoardId = uint64_t;

struct Command {
    string action;
    BoardId board = 0;
    uint64_t first = 0;
    uint64_t second = 0;
    Score score = 0;
};

bool parse_unsigned(const string& text, uint64_t& result) {
    if (text.empty() || text.find_first_not_of("0123456789") != string::npos)
        return false;
    try {
        result = stoull(text);
        return true;
    } catch (const exception&) {
        return false;
    }
}

bool parse_score(const string& text, Score& result) {
    if (text.empty()) return false;
    size_t first_digit = text[0] == '-' ? 1 : 0;
    if (first_digit == text.size() ||
        text.find_first_not_of("0123456789", first_digit) != string::npos)
        return false;
    try {
        result = stoll(text);
        return true;
    } catch (const exception&) {
        return false;
    }
}

Command parse_command(const vector<string>& fields) {
    Command command;
    if (fields.empty()) return command;

    if (fields[0] == "ADD" || fields[0] == "SET") {
        if (fields.size() != 4 || !parse_unsigned(fields[1], command.board) ||
            !parse_unsigned(fields[2], command.first) ||
            !parse_score(fields[3], command.score)) return command;
        command.action = fields[0];
    } else if (fields[0] == "REMOVE" || fields[0] == "RANK" ||
               fields[0] == "SELECT") {
        if (fields.size() != 3 || !parse_unsigned(fields[1], command.board) ||
            !parse_unsigned(fields[2], command.first)) return command;
        command.action = fields[0];
    } else if (fields[0] == "RANGE") {
        if (fields.size() != 4 || !parse_unsigned(fields[1], command.board) ||
            !parse_unsigned(fields[2], command.first) ||
            !parse_unsigned(fields[3], command.second)) return command;
        command.action = fields[0];
    } else if (fields[0] == "SIZE") {
        if (fields.size() != 2 || !parse_unsigned(fields[1], command.board))
            return command;
        command.action = fields[0];
    } else if (fields[0] == "SPLIT") {
        if (fields.size() != 5 || !parse_unsigned(fields[1], command.board) ||
            !parse_score(fields[2], command.score) ||
            !parse_unsigned(fields[3], command.first) ||
            !parse_unsigned(fields[4], command.second)) return command;
        command.action = fields[0];
    } else if (fields[0] == "MERGE") {
        if (fields.size() != 4 || !parse_unsigned(fields[1], command.board) ||
            !parse_unsigned(fields[2], command.first) ||
            !parse_unsigned(fields[3], command.second)) return command;
        command.action = fields[0];
    }
    return command;
}

class Boards {
public:
    explicit Boards(unique_ptr<Board> initial) {
        entries_.push_back({0, move(initial)});
    }

    Board* find(BoardId id) {
        auto entry = find_place(id);
        if (entry == entries_.end() || entry->id != id) return nullptr;
        return entry->board.get();
    }

    bool has_board(BoardId id) {
        return find(id) != nullptr;
    }

    void insert(BoardId id, unique_ptr<Board> board) {
        auto entry = find_place(id);
        if (entry != entries_.end() && entry->id == id)
            throw logic_error("duplicate board ID");
        entries_.insert(entry, Entry{id, move(board)});
    }

    unique_ptr<Board> remove_and_get(BoardId id) {
        auto entry = find_place(id);
        if (entry == entries_.end() || entry->id != id)
            throw logic_error("missing board ID");
        unique_ptr<Board> board = move(entry->board);
        entries_.erase(entry);
        return board;
    }

private:
    struct Entry {
        BoardId id;
        unique_ptr<Board> board;
    };

    vector<Entry> entries_;

    vector<Entry>::iterator find_place(BoardId id) {
        size_t left = 0;
        size_t right = entries_.size();
        while (left < right) {
            size_t middle = left + (right - left) / 2;
            if (entries_[middle].id < id) left = middle + 1;
            else right = middle;
        }
        return entries_.begin() + left;
    }
};

void split_board(const Command& command, Board* board, Boards& boards,
                 ostream& output) {
    const BoardId high_id = command.first;
    const BoardId low_id = command.second;
    if (command.board == high_id || command.board == low_id || high_id == low_id) {
        output << "ERR SAME_BOARD\n";
        return;
    }
    if (boards.has_board(high_id) || boards.has_board(low_id)) {
        output << "ERR BOARD_EXISTS\n";
        return;
    }
    auto divisions = board->split_by_score(command.score);
    boards.remove_and_get(command.board);
    boards.insert(high_id, move(divisions.first));
    boards.insert(low_id, move(divisions.second));
    output << "OK\n";
}

void merge_boards(const Command& command, Board* board, Boards& boards,
                  ostream& output) {
    const BoardId right_id = command.first;
    const BoardId result_id = command.second;
    if (command.board == right_id || command.board == result_id ||
        right_id == result_id) {
        output << "ERR SAME_BOARD\n";
        return;
    }
    Board* right = boards.find(right_id);
    if (!right) {
        output << "ERR UNKNOWN_BOARD\n";
        return;
    }
    if (boards.has_board(result_id)) {
        output << "ERR BOARD_EXISTS\n";
        return;
    }
    board->merge_from(*right);
    unique_ptr<Board> merged = boards.remove_and_get(command.board);
    boards.remove_and_get(right_id);
    boards.insert(result_id, move(merged));
    output << "OK\n";
}

void show_player(const Command& command, Board* board, ostream& output) {
    const Position position = command.first;
    if (position == 0 || position > board->size()) {
        output << "ERR BAD_POSITION\n";
        return;
    }
    Player selected = board->select(position);
    output << "PLAYER " << selected.id << ' ' << selected.score << '\n';
}

void show_range(const Command& command, Board* board, ostream& output) {
    const Position first = command.first;
    const Position last = command.second;
    if (first == 0 || first > last || last > board->size()) {
        output << "ERR BAD_RANGE\n";
        return;
    }
    vector<Player> result;
    board->range(first, last, result);
    output << "PLAYERS " << result.size();
    for (const Player& player : result)
        output << ' ' << player.id << ' ' << player.score;
    output << '\n';
}

void run_command(const Command& command, Boards& boards, PlayerTable& players,
                 ostream& output) {
    if (command.action.empty()) {
        output << "ERR BAD_COMMAND\n";
        return;
    }

    Board* board = boards.find(command.board);
    if (!board) {
        output << "ERR UNKNOWN_BOARD\n";
        return;
    }

    if (command.action == "SIZE") {
        output << "SIZE " << board->size() << '\n';
        return;
    }

    if (command.action == "SPLIT") {
        split_board(command, board, boards, output);
        return;
    }

    if (command.action == "MERGE") {
        merge_boards(command, board, boards, output);
        return;
    }

    if (command.action == "SELECT") {
        show_player(command, board, output);
        return;
    }

    if (command.action == "RANGE") {
        show_range(command, board, output);
        return;
    }

    Score old_score = 0;
    const PlayerId id = command.first;
    bool exists = players.get(id, old_score);
    if (command.action == "ADD") {
        if (exists) {
            output << "ERR DUPLICATE_ID\n";
            return;
        }
        board->insert({id, command.score});
        if (!players.insert(id, command.score))
            throw logic_error("player table insertion failed");
        output << "OK\n";
        return;
    }

    if (!exists) {
        output << "ERR UNKNOWN_PLAYER\n";
        return;
    }
    if (!board->contains({id, old_score})) {
        output << "ERR WRONG_BOARD\n";
        return;
    }

    if (command.action == "RANK") {
        output << "RANK " << board->count_strictly_higher(old_score) + 1 << '\n';
    } else if (command.action == "REMOVE") {
        board->erase({id, old_score});
        if (!players.erase(id))
            throw logic_error("player table removal failed");
        output << "OK\n";
    } else if (command.action == "SET") {
        if (old_score != command.score) {
            board->erase({id, old_score});
            board->insert({id, command.score});
            if (!players.update(id, command.score))
                throw logic_error("player table update failed");
        }
        output << "OK\n";
    }
}

}

void run_replay(istream& input, ostream& output,
                const BoardFactory& factory) {
    vector<Command> commands;
    vector<Score> possible_scores;
    size_t additions = 0;
    string line;
    while (getline(input, line)) {
        istringstream stream(line);
        vector<string> fields;
        string field;
        while (stream >> field) fields.push_back(field);
        if (fields.empty()) continue;
        Command command = parse_command(fields);
        commands.push_back(command);
        if (command.action == "ADD") ++additions;
        if (command.action == "ADD" || command.action == "SET")
            possible_scores.push_back(command.score);
    }

    unique_ptr<Board> initial = factory(possible_scores);
    if (!initial) throw logic_error("board factory returned null");
    Boards boards(move(initial));
    PlayerTable players;
    players.reserve(additions);
    for (const Command& command : commands)
        run_command(command, boards, players, output);
}

}
