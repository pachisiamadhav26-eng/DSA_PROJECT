#include "replay.hpp"

#include "player_table.hpp"

#include <charconv>
#include <cstdint>
#include <map>
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

template<class Integer>
bool parse_number(std::istream& input, Integer& value) {
    std::string token;
    if (!(input >> token)) return false;
    const char* end = token.data() + token.size();
    auto result = std::from_chars(token.data(), end, value);
    return result.ec == std::errc{} && result.ptr == end;
}

Command parse_command(const std::string& line) {
    static const std::map<std::string, Operation> operations{
        {"ADD", Operation::Add}, {"SET", Operation::Set}, {"REMOVE", Operation::Remove},
        {"RANK", Operation::Rank}, {"SELECT", Operation::Select}, {"RANGE", Operation::Range},
        {"SIZE", Operation::Size}, {"SPLIT", Operation::Split}, {"MERGE", Operation::Merge},
    };
    std::istringstream input(line);
    std::string name;
    input >> name;
    auto op = operations.find(name);
    Command command;
    if (op == operations.end() || !parse_number(input, command.a)) return {};

    if (op->second == Operation::Split && !parse_number(input, command.score)) return {};
    if (op->second != Operation::Size && !parse_number(input, command.b)) return {};
    if ((op->second == Operation::Range || op->second == Operation::Split ||
         op->second == Operation::Merge) && !parse_number(input, command.c)) return {};
    if ((op->second == Operation::Add || op->second == Operation::Set) &&
        !parse_number(input, command.score)) return {};
    if (input >> name) return {};

    command.operation = op->second;
    return command;
}

// A null entry marks a consumed ID so it cannot be reused.
using Boards = std::map<BoardId, std::unique_ptr<Board>>;

void run_command(const Command& command, Boards& boards, PlayerTable& players,
                 std::ostream& output) {
    if (command.operation == Operation::Invalid) {
        output << "ERR BAD_COMMAND\n";
        return;
    }

    auto source = boards.find(command.a);
    if (source == boards.end() || !source->second) {
        output << "ERR UNKNOWN_BOARD\n";
        return;
    }

    Board* board = source->second.get();
    if (command.operation == Operation::Size) {
        output << "SIZE " << board->size() << '\n';
        return;
    }

    if (command.operation == Operation::Split || command.operation == Operation::Merge) {
        const bool merging = command.operation == Operation::Merge;
        auto right = boards.find(command.b);
        if (merging && (right == boards.end() || !right->second)) {
            output << "ERR UNKNOWN_BOARD\n";
            return;
        }
        if (command.a == command.b || command.a == command.c || command.b == command.c) {
            output << "ERR SAME_BOARD\n";
            return;
        }
        if (boards.count(command.c) || (!merging && right != boards.end())) {
            output << "ERR BOARD_EXISTS\n";
            return;
        }
        if (merging) {
            board->merge_from(*right->second);
            boards.emplace(command.c, std::move(source->second));
            right->second.reset();
        } else {
            auto [high, low] = board->split_by_score(command.score);
            boards.emplace(command.b, std::move(high));
            boards.emplace(command.c, std::move(low));
            source->second.reset();
        }
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
        Command command = parse_command(line);
        commands.push_back(command);
        if (command.operation == Operation::Add) ++additions;
        if (command.operation == Operation::Add ||
            command.operation == Operation::Set)
            possible_scores.push_back(command.score);
    }

    if (input.bad() || !input.eof()) throw std::runtime_error("cannot read trace");

    std::unique_ptr<Board> initial = factory(possible_scores);
    if (!initial) throw std::logic_error("board factory returned null");
    Boards boards;
    boards.emplace(0, std::move(initial));
    PlayerTable players;
    players.reserve(additions);
    for (const Command& command : commands)
        run_command(command, boards, players, output);
    output.flush();
    if (!output) throw std::runtime_error("cannot write replay output");
}

}
