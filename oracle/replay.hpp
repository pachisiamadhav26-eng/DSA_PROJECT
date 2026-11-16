#pragma once

#include "sorted_array.hpp"

#include <map>
#include <sstream>
#include <string>

namespace oracle {

using BoardId = std::uint64_t;

struct Command {
    std::string name;
    BoardId board = 0;
    // The first and second arguments are IDs or positions, depending on the command.
    std::uint64_t first = 0;
    std::uint64_t second = 0;
    Score score = 0;
    bool valid = false;
};

inline std::uint64_t read_unsigned(const std::string& text) {
    if (text.empty() || text.find_first_not_of("0123456789") != std::string::npos) {
        throw std::invalid_argument("expected unsigned decimal");
    }
    // stoull rejects numbers outside the unsigned 64-bit range.
    return std::stoull(text);
}

inline Score read_score(const std::string& text) {
    std::size_t first_digit = 0;
    if (!text.empty() && text[0] == '-') {
        first_digit = 1;
    }
    if (first_digit == text.size() ||
        text.find_first_not_of("0123456789", first_digit) != std::string::npos) {
        throw std::invalid_argument("expected signed decimal");
    }
    return std::stoll(text);
}

inline std::size_t argument_count(const std::string& name) {
    if (name == "SIZE") {
        return 1;
    }
    if (name == "RANK" || name == "REMOVE" || name == "SELECT") {
        return 2;
    }
    if (name == "ADD" || name == "SET" || name == "RANGE" || name == "MERGE") {
        return 3;
    }
    if (name == "SPLIT") {
        return 4;
    }
    return 0;
}

inline Command parse_command(const std::string& line) {
    std::istringstream input(line);
    Command command;
    if (!(input >> command.name)) {
        return command;  // The application skips blank lines.
    }

    std::vector<std::string> arguments;
    std::string word;
    while (input >> word) {
        arguments.push_back(word);
    }
    std::size_t count = argument_count(command.name);
    if (count == 0 || arguments.size() != count) {
        return command;
    }

    try {
        command.board = read_unsigned(arguments[0]);
        if (command.name == "SPLIT") {
            command.score = read_score(arguments[1]);
            command.first = read_unsigned(arguments[2]);
            command.second = read_unsigned(arguments[3]);
        } else if (count >= 2) {
            command.first = read_unsigned(arguments[1]);
            if (command.name == "ADD" || command.name == "SET") {
                command.score = read_score(arguments[2]);
            } else if (count == 3) {
                command.second = read_unsigned(arguments[2]);
            }
        }
        command.valid = true;
    } catch (const std::exception&) {
        // A malformed or overflowing number leaves the command invalid.
    }
    return command;
}

struct PlayerLocation {
    BoardId board;
    Score score;
};

// A simple test model for board ownership. The real app uses its own hash table.
class Session {
public:
    std::map<BoardId, std::unique_ptr<Board>> boards;
    std::map<PlayerId, PlayerLocation> players;

    explicit Session(std::unique_ptr<Board> board) {
        boards[0] = std::move(board);
    }

    std::string execute(const Command& command) {
        if (command.name.empty()) {
            return "";
        }
        if (!command.valid) {
            return "ERR BAD_COMMAND";
        }

        auto board_entry = boards.find(command.board);
        if (board_entry == boards.end()) {
            return "ERR UNKNOWN_BOARD";
        }
        Board& board = *board_entry->second;

        if (command.name == "SIZE") {
            return "SIZE " + std::to_string(board.size());
        }
        if (command.name == "SELECT") {
            if (command.first == 0 || command.first > board.size()) {
                return "ERR BAD_POSITION";
            }
            Player player = board.select(command.first);
            return "PLAYER " + std::to_string(player.id) + " " + std::to_string(player.score);
        }
        if (command.name == "RANGE") {
            return show_range(board, command.first, command.second);
        }
        if (command.name == "SPLIT") {
            return split(board, command);
        }
        if (command.name == "MERGE") {
            return merge(board, command);
        }

        PlayerId id = command.first;
        auto location = players.find(id);
        if (command.name == "ADD") {
            if (location != players.end()) {
                return "ERR DUPLICATE_ID";
            }
            board.insert({id, command.score});
            players[id] = {command.board, command.score};
            return "OK";
        }

        if (location == players.end()) {
            return "ERR UNKNOWN_PLAYER";
        }
        if (location->second.board != command.board) {
            return "ERR WRONG_BOARD";
        }
        Player old_player{id, location->second.score};
        if (command.name == "RANK") {
            Position rank = 1 + board.count_strictly_higher(old_player.score);
            return "RANK " + std::to_string(rank);
        }
        if (command.name == "REMOVE") {
            board.erase(old_player);
            players.erase(location);
        } else if (command.score != old_player.score) {
            // SET replaces the exact old key. An unchanged score needs no work.
            board.erase(old_player);
            board.insert({id, command.score});
            location->second.score = command.score;
        }
        return "OK";
    }

private:
    std::string show_range(Board& board, Position first, Position last) const {
        if (first == 0 || first > last || last > board.size()) {
            return "ERR BAD_RANGE";
        }
        // A prefilled result checks that range replaces old values.
        std::vector<Player> page{{999, 999}};
        board.range(first, last, page);
        std::ostringstream output;
        output << "PLAYERS " << page.size();
        for (Player player : page) {
            output << ' ' << player.id << ' ' << player.score;
        }
        return output.str();
    }

    std::string split(Board& board, const Command& command) {
        BoardId source = command.board;
        BoardId high = command.first;
        BoardId low = command.second;

        if (source == high || source == low || high == low) {
            return "ERR SAME_BOARD";
        }
        if (boards.count(high) || boards.count(low)) {
            return "ERR BOARD_EXISTS";
        }

        auto divisions = board.split_by_score(command.score);
        if (board.size() != 0) {
            throw std::logic_error("split source not empty");
        }
        boards[high] = std::move(divisions.first);
        boards[low] = std::move(divisions.second);
        boards.erase(source);

        // Update the independent player locations using only their scores.
        for (auto& entry : players) {
            PlayerLocation& location = entry.second;
            if (location.board == source) {
                if (location.score >= command.score) {
                    location.board = high;
                } else {
                    location.board = low;
                }
            }
        }
        return "OK";
    }

    std::string merge(Board& board, const Command& command) {
        BoardId left = command.board;
        BoardId right = command.first;
        BoardId result = command.second;

        if (left == right || left == result || right == result) {
            return "ERR SAME_BOARD";
        }
        auto donor = boards.find(right);
        if (donor == boards.end()) {
            return "ERR UNKNOWN_BOARD";
        }
        if (boards.count(result)) {
            return "ERR BOARD_EXISTS";
        }

        // ADD already prevents duplicate IDs across boards.
        board.merge_from(*donor->second);
        if (donor->second->size() != 0) {
            throw std::logic_error("merge source not empty");
        }
        boards[result] = std::move(boards.at(left));
        boards.erase(left);
        boards.erase(right);
        for (auto& entry : players) {
            PlayerLocation& location = entry.second;
            if (location.board == left || location.board == right) {
                location.board = result;
            }
        }
        return "OK";
    }
};

inline std::vector<Score> possible_scores(const std::vector<std::string>& lines) {
    std::vector<Score> scores;
    for (const std::string& line : lines) {
        Command command = parse_command(line);
        if (command.valid && (command.name == "ADD" || command.name == "SET")) {
            scores.push_back(command.score);
        }
    }
    std::sort(scores.begin(), scores.end());
    scores.erase(std::unique(scores.begin(), scores.end()), scores.end());
    return scores;
}

}
