#pragma once

#include "replay.hpp"

#include <fstream>
#include <iterator>
#include <limits>
#include <random>

namespace oracle {

using Trace = std::vector<std::string>;

inline Trace read_trace(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("cannot read " + path);
    }
    Trace lines;
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    return lines;
}

inline void save_trace(const std::string& path, const Trace& lines) {
    std::ofstream file(path);
    if (!file) {
        throw std::runtime_error("cannot write " + path);
    }
    for (const std::string& line : lines) {
        file << line << '\n';
    }
    file.close();
    if (!file) {
        throw std::runtime_error("failed to finish writing " + path);
    }
}

inline Score random_score(std::mt19937_64& random) {
    // Include both score limits, plus a small pool of scores that often tie.
    auto choice = random() % 40;
    if (choice == 0) {
        return std::numeric_limits<Score>::min();
    }
    if (choice == 1) {
        return std::numeric_limits<Score>::max();
    }
    return static_cast<Score>(random() % 33) - 16;
}

inline void add_line(Trace& trace, Session& model, const std::string& line) {
    trace.push_back(line);
    model.execute(parse_command(line));
}

inline Trace generate_trace(std::uint64_t seed, std::size_t line_count, std::size_t max_live) {
    std::mt19937_64 random(seed);
    Trace trace;
    trace.reserve(line_count);
    Session model(std::make_unique<SortedArray>());
    BoardId current_board = 0;
    BoardId next_board = 1;

    while (trace.size() < line_count) {
        std::string board = std::to_string(current_board);
        auto operation = random() % 12;
        PlayerId id = random() % (max_live + 16);
        if (!model.players.empty() && random() % 4 != 0) {
            // Selecting from the map's ID order makes the draw reproducible.
            auto player = model.players.begin();
            std::advance(player, random() % model.players.size());
            id = player->first;
        }

        if (operation == 0 && trace.size() + 2 <= line_count) {
            // Split and rejoin together so the generator has one current board.
            std::string high = std::to_string(next_board++);
            std::string low = std::to_string(next_board++);
            BoardId result = next_board++;
            Score threshold = random_score(random);
            add_line(trace, model, "SPLIT " + board + " " + std::to_string(threshold) + " " + high + " " + low);
            add_line(trace, model, "MERGE " + high + " " + low + " " + std::to_string(result));
            current_board = result;
            continue;
        }

        std::string command;
        if (operation <= 2 && model.players.size() < max_live) {
            // Draw the score and ID in a fixed order.
            Score score = random_score(random);
            PlayerId new_id = random() % (max_live + 16);
            command = "ADD " + board + " " + std::to_string(new_id) + " " + std::to_string(score);
        } else if (operation <= 4) {
            Score score = random_score(random);
            command = "SET " + board + " " + std::to_string(id) + " " + std::to_string(score);
        } else if (operation == 5) {
            command = "REMOVE " + board + " " + std::to_string(id);
        } else if (operation == 6 || operation == 7) {
            command = "RANK " + board + " " + std::to_string(id);
        } else if (operation == 8) {
            Position position = random() % (model.players.size() + 2);
            command = "SELECT " + board + " " + std::to_string(position);
        } else if (operation == 9) {
            Position first = random() % (model.players.size() + 2);
            Position last = random() % (model.players.size() + 2);
            if (first > last) {
                std::swap(first, last);
            }
            command = "RANGE " + board + " " + std::to_string(first) + " " + std::to_string(last);
        } else if (operation == 10) {
            command = "SIZE " + board;
        } else {
            // Exercise the parser with a negative position.
            command = "SELECT " + board + " -1";
        }
        add_line(trace, model, command);
    }
    return trace;
}

}
