#pragma once

#include "leaderboard.hpp"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <map>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace bench {
using namespace leaderboard;

// QPC avoids the coarse steady_clock implementation in older Windows MinGW.
inline double now_ns() {
#ifdef _WIN32
    static const double frequency = [] {
        LARGE_INTEGER f;
        if (!QueryPerformanceFrequency(&f)) throw std::runtime_error("QPC unavailable");
        return static_cast<double>(f.QuadPart);
    }();
    LARGE_INTEGER value;
    if (!QueryPerformanceCounter(&value)) throw std::runtime_error("QPC failed");
    return static_cast<double>(value.QuadPart) * (1e9 / frequency);
#else
    return std::chrono::duration<double, std::nano>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
#endif
}

inline std::uint64_t integer(const std::string& text) {
    if (text.empty() || text.find_first_not_of("0123456789") != std::string::npos)
        throw std::invalid_argument("expected unsigned decimal: " + text);
    return std::stoull(text);
}

struct Options {
    std::map<std::string, std::string> values;
    Options(int argc, char** argv) {
        for (int i = 1; i < argc; ++i) {
            const std::string name = argv[i];
            if (name == "--help") { values[name] = "1"; continue; }
            if (name.compare(0, 2, "--") || i + 1 == argc || values.count(name))
                throw std::invalid_argument("invalid/duplicate option: " + name);
            values[name] = argv[++i];
        }
    }
    std::string get(const std::string& key, const std::string& fallback) const {
        auto it = values.find(key);
        return it == values.end() ? fallback : it->second;
    }
    std::uint64_t number(const std::string& key, std::uint64_t fallback) const {
        return integer(get(key, std::to_string(fallback)));
    }
};

inline bool before(Player a, Player b) {
    return a.score != b.score ? a.score > b.score : a.id < b.id;
}
inline bool equal(Player a, Player b) { return a.id == b.id && a.score == b.score; }
inline void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
inline std::vector<Player> sorted(std::vector<Player> players) {
    std::sort(players.begin(), players.end(), before);
    return players;
}
inline void check_board(const Board& board, const std::vector<Player>& expected) {
    require(board.size() == expected.size(), "board size mismatch");
    std::vector<Player> actual;
    if (!expected.empty()) board.range(1, board.size(), actual);
    require(actual.size() == expected.size() &&
            std::equal(actual.begin(), actual.end(), expected.begin(), equal),
            "board membership/order mismatch");
}
inline Position expected_rank(const std::vector<Player>& ordered, Score score) {
    return 1 + static_cast<Position>(std::lower_bound(ordered.begin(), ordered.end(), score,
        [](Player p, Score s) { return p.score > s; }) - ordered.begin());
}
inline void consume(std::uint64_t& checksum, std::uint64_t value) {
    checksum = (checksum ^ value) * 1099511628211ULL;
}

struct Row {
    std::string backend, experiment, operation;
    std::uint64_t n = 0, coordinates = 0, seed = 0, run = 0, calls = 0, checksum = 0;
    double elapsed = 0;
    std::string validation;
    int left_height = -1, right_height = -1;
};

class Csv {
    std::ofstream output;
public:
    explicit Csv(const std::string& path) {
        require(!std::ifstream(path).good(), "output already exists: " + path);
        output.open(path);
        require(output.good(), "cannot create output: " + path);
        output << "backend,experiment,operation,players,coordinates,seed,run,calls,elapsed_ns,ns_per_call,checksum,validation,left_height,right_height\n";
    }
    void write(const Row& row) {
        require(row.calls != 0, "cannot record an empty measurement");
        require(row.experiment.find_first_of(",\r\n\"") == std::string::npos,
                "experiment labels cannot contain CSV delimiters");
        output << std::setprecision(17);
#ifdef BENCH_TEST_BACKEND
        output << "test-reference-";
#endif
        output << row.backend << ',' << row.experiment << ','
               << row.operation << ',' << row.n << ',' << row.coordinates << ','
               << row.seed << ',' << row.run << ',' << row.calls << ',' << row.elapsed
               << ',' << row.elapsed / row.calls << ',' << row.checksum << ','
               << row.validation << ',' << row.left_height << ',' << row.right_height << '\n';
        output.flush();
        require(output.good(), "failed to save CSV");
    }
};
}
