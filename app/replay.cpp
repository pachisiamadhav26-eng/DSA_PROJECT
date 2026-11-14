#include "replay.hpp"

#include "player_table.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace leaderboard {

namespace {

using BoardId = std::uint64_t;

enum class Operation {
    Invalid, Add, Set, Remove, Rank, Select, Range, Size, Split, Merge
};

struct Command {
    Operation operation = Operation::Invalid;
    BoardId a = 0;
    std::uint64_t b = 0;
    std::uint64_t c = 0;
    Score score = 0;
};

bool parse_unsigned(const std::string& text, std::uint64_t& result) {
    if (text.empty()) return false;
    std::uint64_t value = 0;
    for (char character : text) {
        if (character < '0' || character > '9') return false;
        std::uint64_t digit = static_cast<std::uint64_t>(character - '0');
        if (value > (std::numeric_limits<std::uint64_t>::max() - digit) / 10)
            return false;
        value = value * 10 + digit;
    }
    result = value;
    return true;
}

bool parse_score(const std::string& text, Score& result) {
    if (text.empty()) return false;
    bool negative = text[0] == '-';
    std::size_t start = negative ? 1 : 0;
    if (start == text.size()) return false;
    std::uint64_t limit = static_cast<std::uint64_t>(
        std::numeric_limits<Score>::max());
    if (negative) ++limit;
    std::uint64_t value = 0;
    for (std::size_t i = start; i < text.size(); ++i) {
        char character = text[i];
        if (character < '0' || character > '9') return false;
        std::uint64_t digit = static_cast<std::uint64_t>(character - '0');
        if (value > (limit - digit) / 10) return false;
        value = value * 10 + digit;
    }
    if (negative && value == limit)
        result = std::numeric_limits<Score>::min();
    else
        result = negative ? -static_cast<Score>(value) : static_cast<Score>(value);
    return true;
}

Command parse_command(const std::vector<std::string>& fields) {
    Command command;
    if (fields.empty()) return command;

    if (fields[0] == "ADD" || fields[0] == "SET") {
        if (fields.size() != 4 || !parse_unsigned(fields[1], command.a) ||
            !parse_unsigned(fields[2], command.b) ||
            !parse_score(fields[3], command.score)) return command;
        command.operation = fields[0] == "ADD" ? Operation::Add : Operation::Set;
    } else if (fields[0] == "REMOVE" || fields[0] == "RANK" ||
               fields[0] == "SELECT") {
        if (fields.size() != 3 || !parse_unsigned(fields[1], command.a) ||
            !parse_unsigned(fields[2], command.b)) return command;
        if (fields[0] == "REMOVE") command.operation = Operation::Remove;
        else if (fields[0] == "RANK") command.operation = Operation::Rank;
        else command.operation = Operation::Select;
    } else if (fields[0] == "RANGE") {
        if (fields.size() != 4 || !parse_unsigned(fields[1], command.a) ||
            !parse_unsigned(fields[2], command.b) ||
            !parse_unsigned(fields[3], command.c)) return command;
        command.operation = Operation::Range;
    } else if (fields[0] == "SIZE") {
        if (fields.size() != 2 || !parse_unsigned(fields[1], command.a))
            return command;
        command.operation = Operation::Size;
    } else if (fields[0] == "SPLIT") {
        if (fields.size() != 5 || !parse_unsigned(fields[1], command.a) ||
            !parse_score(fields[2], command.score) ||
            !parse_unsigned(fields[3], command.b) ||
            !parse_unsigned(fields[4], command.c)) return command;
        command.operation = Operation::Split;
    } else if (fields[0] == "MERGE") {
        if (fields.size() != 4 || !parse_unsigned(fields[1], command.a) ||
            !parse_unsigned(fields[2], command.b) ||
            !parse_unsigned(fields[3], command.c)) return command;
        command.operation = Operation::Merge;
    }
    return command;
}

class Boards {
public:
    explicit Boards(std::unique_ptr<Board> initial) {
        entries_.push_back({0, std::move(initial)});
    }

    Board* find(BoardId id) {
        auto entry = lower(id);
        if (entry == entries_.end() || entry->id != id) return nullptr;
        return entry->board.get();
    }

    bool contains(BoardId id) {
        return find(id) != nullptr;
    }

    void insert(BoardId id, std::unique_ptr<Board> board) {
        auto entry = lower(id);
        if (entry != entries_.end() && entry->id == id)
            throw std::logic_error("duplicate board ID");
        entries_.insert(entry, Entry{id, std::move(board)});
    }

    std::unique_ptr<Board> take(BoardId id) {
        auto entry = lower(id);
        if (entry == entries_.end() || entry->id != id)
            throw std::logic_error("missing board ID");
        std::unique_ptr<Board> board = std::move(entry->board);
        entries_.erase(entry);
        return board;
    }

private:
    struct Entry {
        BoardId id;
        std::unique_ptr<Board> board;
    };

    std::vector<Entry> entries_;

    std::vector<Entry>::iterator lower(BoardId id) {
        return std::lower_bound(entries_.begin(), entries_.end(), id,
                                [](const Entry& entry, BoardId key) {
                                    return entry.id < key;
                                });
    }
};

void run_command(const Command& command, Boards& boards, PlayerTable& players,
                 std::ostream& output) {
    if (command.operation == Operation::Invalid) {
        output << "ERR BAD_COMMAND\n";
        return;
    }

    Board* board = boards.find(command.a);
    if (!board) {
        output << "ERR UNKNOWN_BOARD\n";
        return;
    }

    if (command.operation == Operation::Size) {
        output << "SIZE " << board->size() << '\n';
        return;
    }

    if (command.operation == Operation::Split) {
        if (command.a == command.b || command.a == command.c ||
            command.b == command.c) {
            output << "ERR SAME_BOARD\n";
            return;
        }
        if (boards.contains(command.b) || boards.contains(command.c)) {
            output << "ERR BOARD_EXISTS\n";
            return;
        }
        auto divisions = board->split_by_score(command.score);
        boards.take(command.a);
        boards.insert(command.b, std::move(divisions.first));
        boards.insert(command.c, std::move(divisions.second));
        output << "OK\n";
        return;
    }

    if (command.operation == Operation::Merge) {
        if (command.a == command.b || command.a == command.c ||
            command.b == command.c) {
            output << "ERR SAME_BOARD\n";
            return;
        }
        Board* right = boards.find(command.b);
        if (!right) {
            output << "ERR UNKNOWN_BOARD\n";
            return;
        }
        if (boards.contains(command.c)) {
            output << "ERR BOARD_EXISTS\n";
            return;
        }
        board->merge_from(*right);
        std::unique_ptr<Board> merged = boards.take(command.a);
        boards.take(command.b);
        boards.insert(command.c, std::move(merged));
        output << "OK\n";
        return;
    }

    if (command.operation == Operation::Select) {
        if (command.b == 0 || command.b > board->size()) {
            output << "ERR BAD_POSITION\n";
            return;
        }
        Player selected = board->select(command.b);
        output << "PLAYER " << selected.id << ' ' << selected.score << '\n';
        return;
    }

    if (command.operation == Operation::Range) {
        if (command.b == 0 || command.b > command.c ||
            command.c > board->size()) {
            output << "ERR BAD_RANGE\n";
            return;
        }
        std::vector<Player> result;
        board->range(command.b, command.c, result);
        output << "PLAYERS " << result.size();
        for (const Player& player : result)
            output << ' ' << player.id << ' ' << player.score;
        output << '\n';
        return;
    }

    Score old_score = 0;
    bool exists = players.get(command.b, old_score);
    if (command.operation == Operation::Add) {
        if (exists) {
            output << "ERR DUPLICATE_ID\n";
            return;
        }
        board->insert({command.b, command.score});
        if (!players.insert(command.b, command.score))
            throw std::logic_error("player table insertion failed");
        output << "OK\n";
        return;
    }

    if (!exists) {
        output << "ERR UNKNOWN_PLAYER\n";
        return;
    }
    if (!board->contains({command.b, old_score})) {
        output << "ERR WRONG_BOARD\n";
        return;
    }

    if (command.operation == Operation::Rank) {
        output << "RANK " << board->count_strictly_higher(old_score) + 1 << '\n';
    } else if (command.operation == Operation::Remove) {
        board->erase({command.b, old_score});
        if (!players.erase(command.b))
            throw std::logic_error("player table removal failed");
        output << "OK\n";
    } else if (command.operation == Operation::Set) {
        if (old_score != command.score) {
            board->erase({command.b, old_score});
            board->insert({command.b, command.score});
            if (!players.update(command.b, command.score))
                throw std::logic_error("player table update failed");
        }
        output << "OK\n";
    }
}

}

void run_replay(std::istream& input, std::ostream& output,
                const BoardFactory& factory) {
    std::vector<Command> commands;
    std::vector<Score> possible_scores;
    std::size_t additions = 0;
    std::string line;
    while (std::getline(input, line)) {
        std::istringstream stream(line);
        std::vector<std::string> fields;
        std::string field;
        while (stream >> field) fields.push_back(field);
        if (fields.empty()) continue;
        Command command = parse_command(fields);
        commands.push_back(command);
        if (command.operation == Operation::Add) ++additions;
        if (command.operation == Operation::Add ||
            command.operation == Operation::Set)
            possible_scores.push_back(command.score);
    }

    std::unique_ptr<Board> initial = factory(possible_scores);
    if (!initial) throw std::logic_error("board factory returned null");
    Boards boards(std::move(initial));
    PlayerTable players;
    players.reserve(additions);
    for (const Command& command : commands)
        run_command(command, boards, players, output);
}

}
