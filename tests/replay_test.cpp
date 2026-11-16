#include "replay.hpp"
#include "../oracle/sorted_array.hpp"

#include <cassert>
#include <fstream>
#include <sstream>

int main() {
    auto factory = [](const std::vector<leaderboard::Score>&) {
        return std::make_unique<oracle::SortedArray>();
    };
    std::ifstream trace("app/examples/basic.trace");
    std::ifstream expected_file("app/examples/basic.expected");
    assert(trace && expected_file);
    std::ostringstream actual;
    leaderboard::run_replay(trace, actual, factory);
    std::ostringstream expected;
    expected << expected_file.rdbuf();
    assert(actual.str() == expected.str());

    std::istringstream errors(
        "ADD 0 1 10\n"
        "ADD 0 1 20\n"
        "RANK 0 1\n"
        "SPLIT 0 10 1 2\n"
        "SIZE 0\n"
        "ADD 2 1 10\n"
        "RANK 2 1\n"
        "RANK 1 1\n"
        "MERGE 1 2 3\n"
        "MERGE 3 3 4\n"
        "SET 3 1 10\n"
        "SELECT 3 0\n"
        "RANGE 3 2 1\n"
        "REMOVE 3 1\n"
        "RANK 3 1\n"
        "ADD 3 2 -9223372036854775808\n"
        "SELECT 3 1\n"
        "ADD 3 -1 5\n"
        "ADD 3 4 9223372036854775808\n"
        "ADD 3 18446744073709551615 9223372036854775807\n"
        "RANK 3 18446744073709551615\n"
        "ADD 3 18446744073709551616 0\n");
    std::ostringstream error_output;
    leaderboard::run_replay(errors, error_output, factory);
    assert(error_output.str() ==
           "OK\n"
           "ERR DUPLICATE_ID\n"
           "RANK 1\n"
           "OK\n"
           "ERR UNKNOWN_BOARD\n"
           "ERR DUPLICATE_ID\n"
           "ERR WRONG_BOARD\n"
           "RANK 1\n"
           "OK\n"
           "ERR SAME_BOARD\n"
           "OK\n"
           "ERR BAD_POSITION\n"
           "ERR BAD_RANGE\n"
           "OK\n"
           "ERR UNKNOWN_PLAYER\n"
           "OK\n"
           "PLAYER 2 -9223372036854775808\n"
           "ERR BAD_COMMAND\n"
           "ERR BAD_COMMAND\n"
           "OK\n"
           "RANK 1\n"
           "ERR BAD_COMMAND\n");

    std::istringstream seasons(
        "SPLIT 0 0 1 2\n"
        "MERGE 1 99 1\n"
        "MERGE 1 2 1\n"
        "MERGE 1 2 3\n"
        "SPLIT 3 0 0 4\n"
        "\n"
        "SIZE 3\n");
    std::ostringstream season_output;
    leaderboard::run_replay(seasons, season_output, factory);
    assert(season_output.str() == "OK\nERR UNKNOWN_BOARD\nERR SAME_BOARD\n"
                                  "OK\nERR BOARD_EXISTS\nERR BAD_COMMAND\nSIZE 0\n");
    std::istringstream bad_input("SIZE 0\n");
    bad_input.setstate(std::ios::badbit);
    std::ostringstream bad_input_output;
    bool read_failed = false;
    try { leaderboard::run_replay(bad_input, bad_input_output, factory); }
    catch (const std::runtime_error&) { read_failed = true; }
    assert(read_failed && bad_input_output.str().empty());

    std::istringstream valid_input("SIZE 0");
    std::ostream bad_output(nullptr);
    bool write_failed = false;
    try { leaderboard::run_replay(valid_input, bad_output, factory); }
    catch (const std::runtime_error&) { write_failed = true; }
    assert(write_failed);

}
