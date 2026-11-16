#include "backend_checks.hpp"
#include "trace.hpp"

#include <chrono>
#include <iostream>

namespace {

using namespace oracle;

struct RunResult {
    std::size_t failing_line = 0;  // Zero means the run passed.
    std::size_t peak_live = 0;
    std::string failure;
};

void check_lookup(const Session& session, BoardId board_id, Board& board,
                  const std::vector<Player>& players) {
    require(players.size() == board.size(), "board size differs from its audit");
    for (Player player : players) {
        auto entry = session.players.find(player.id);
        require(entry != session.players.end(), "player missing from lookup");
        require(entry->second.board == board_id && entry->second.score == player.score,
                "player lookup/board state mismatch");
        require(board.contains(player), "contains rejected an existing key");
    }
}

void audit_reference(const Session& reference) {
    std::size_t total = 0;
    for (const auto& entry : reference.boards) {
        // Every board in this session comes from the sorted-array oracle.
        auto& board = static_cast<SortedArray&>(*entry.second);
        board.audit();
        check_lookup(reference, entry.first, board, board.entries());
        total += board.entries().size();
    }
    require(total == reference.players.size(), "oracle lookup total");
}

void check_state(const Session& actual, const Session& reference, Audit audit_board) {
    require(actual.boards.size() == reference.boards.size(), "board count differs");
    require(actual.players.size() == reference.players.size(), "player lookup count differs");

    std::size_t total = 0;
    for (const auto& entry : reference.boards) {
        BoardId id = entry.first;
        auto found = actual.boards.find(id);
        require(found != actual.boards.end(), "board ID differs from oracle");

        Board& board = *found->second;
        auto players = audit_board(board);
        const auto& expected = static_cast<const SortedArray&>(*entry.second).entries();
        check_players(players, expected, 0, expected.size());
        check_lookup(actual, id, board, players);
        total += players.size();
    }
    // Every audited player must have a matching lookup and there must be no extras.
    require(total == actual.players.size(), "player lookup total");
}

std::string check_backend(Session& actual, const Session& reference,
                          const Command& command, const std::string& expected_output,
                          const std::string& name, Audit audit_board, bool inject_fault) {
    try {
        std::string output = actual.execute(command);
        // Deliberately change one valid AVL rank to check the failure reporter.
        if (inject_fault && command.name == "RANK" && expected_output.find("RANK ") == 0) {
            output = "RANK injected";
        }
        if (output != expected_output) {
            return name + ": " + command.name + " expected " + expected_output + ", actual " + output;
        }
        check_state(actual, reference, audit_board);
    } catch (const std::exception& error) {
        return name + ": " + command.name + ": " + error.what();
    }
    return "";
}

RunResult replay(const Trace& trace, bool inject_fault) {
    // Fenwick needs the possible scores, including future revisions, up front.
    Session reference(std::make_unique<SortedArray>());
    Session avl(make_avl());
    Session fenwick(make_fenwick(possible_scores(trace)));
    RunResult result;

    for (std::size_t index = 0; index < trace.size(); ++index) {
        Command command = parse_command(trace[index]);
        std::string expected_output;
        try {
            expected_output = reference.execute(command);
            audit_reference(reference);
            result.peak_live = std::max(result.peak_live, reference.players.size());
        } catch (const std::exception& error) {
            result.failing_line = index + 1;
            result.failure = std::string("oracle: ") + error.what();
            return result;
        }

        result.failure = check_backend(avl, reference, command, expected_output,
                                       "AVL", audit_avl, inject_fault);
        if (result.failure.empty()) {
            result.failure = check_backend(fenwick, reference, command, expected_output,
                                           "Fenwick", audit_fenwick, false);
        }
        if (!result.failure.empty()) {
            result.failing_line = index + 1;
            return result;
        }
    }
    return result;
}

Trace without_lines(const Trace& trace, std::size_t start, std::size_t end) {
    Trace shorter = trace;
    shorter.erase(shorter.begin() + start, shorter.begin() + end);
    return shorter;
}

Trace shrink(Trace trace, const std::string& failure, bool inject_fault) {
    // Try dropping chunks first so a long failing trace shrinks quickly.
    std::size_t chunk_count = 2;
    while (trace.size() >= 2) {
        std::size_t width = (trace.size() + chunk_count - 1) / chunk_count;
        bool removed_chunk = false;
        for (std::size_t start = 0; start < trace.size(); start += width) {
            std::size_t end = std::min(start + width, trace.size());
            Trace candidate = without_lines(trace, start, end);
            if (replay(candidate, inject_fault).failure == failure) {
                trace.swap(candidate);
                if (chunk_count > 2) {
                    --chunk_count;
                }
                removed_chunk = true;
                break;
            }
        }
        if (!removed_chunk) {
            if (chunk_count >= trace.size()) {
                break;
            }
            chunk_count = std::min(trace.size(), chunk_count * 2);
        }
    }

    // Restart after every deletion. When no single line can be removed
    // while keeping the failure message, the trace is 1-minimal.
    std::size_t index = 0;
    while (index < trace.size()) {
        Trace candidate = without_lines(trace, index, index + 1);
        if (replay(candidate, inject_fault).failure == failure) {
            trace.swap(candidate);
            index = 0;
        } else {
            ++index;
        }
    }
    return trace;
}

void test_shrinker() {
    Trace trace = {"SIZE 0", "ADD 0 7 10", "ADD 0 8 5", "RANK 0 7", "REMOVE 0 8"};
    RunResult result = replay(trace, true);
    require(result.failing_line != 0, "fault injection not detected");

    Trace reduced = shrink(trace, result.failure, true);
    require(reduced.size() == 2, "shrinker result");
    for (std::size_t index = 0; index < reduced.size(); ++index) {
        Trace candidate = without_lines(reduced, index, index + 1);
        require(replay(candidate, true).failure != result.failure, "shrinker not 1-minimal");
    }
}

void run_self_tests() {
    test_oracle_boundaries();
    test_avl_rotations();
    test_avl_joins();
    test_shrinker();
    std::cout << "PASS oracle boundaries, exact membership, rotations, key split, joins, shrinker\n";
}

struct Options {
    std::uint64_t seed = 253;
    std::size_t operations = 1000000;
    std::size_t max_live = 128;
    std::size_t scale_players = 0;
    std::string input_file;
    std::string dump_file;
    bool self_test_only = false;
    bool inject_fault = false;
};

Options read_options(int argc, char** argv) {
    Options options;
    for (int index = 1; index < argc; ++index) {
        std::string flag = argv[index];
        if (flag == "--self-test") {
            options.self_test_only = true;
            continue;
        }
        if (flag == "--inject-fault") {
            options.inject_fault = true;
            continue;
        }
        if (index + 1 == argc) {
            throw std::invalid_argument("missing argument for " + flag);
        }
        ++index;
        std::string value = argv[index];
        if (flag == "--seed") {
            options.seed = read_unsigned(value);
        } else if (flag == "--ops") {
            options.operations = read_unsigned(value);
        } else if (flag == "--max-live") {
            options.max_live = read_unsigned(value);
        } else if (flag == "--trace") {
            options.input_file = value;
        } else if (flag == "--dump") {
            options.dump_file = value;
        } else if (flag == "--scale") {
            options.scale_players = read_unsigned(value);
        } else {
            throw std::invalid_argument("unknown option: " + flag);
        }
    }
    if (options.max_live == 0 || options.max_live > 1000000) {
        throw std::invalid_argument("max-live must be 1..1000000");
    }
    return options;
}

int run(const Options& options) {
    run_self_tests();
    if (options.self_test_only) {
        return 0;
    }
    if (options.scale_players != 0) {
        test_scale(options.scale_players);
        return 0;
    }

    auto start = std::chrono::steady_clock::now();
    Trace trace;
    std::string description;
    if (options.input_file.empty()) {
        trace = generate_trace(options.seed, options.operations, options.max_live);
        description = "seed=" + std::to_string(options.seed);
    } else {
        trace = read_trace(options.input_file);
        description = "trace=" + options.input_file;
    }
    if (!options.dump_file.empty()) {
        save_trace(options.dump_file, trace);
    }

    RunResult result = replay(trace, options.inject_fault);
    if (result.failing_line != 0) {
        trace.resize(result.failing_line);
        save_trace("oracle-failure.trace", trace);
        std::cerr << "FAIL " << description << " line=" << result.failing_line
                  << ' ' << result.failure << "\nCommand: " << trace.back() << '\n';

        Trace reduced = shrink(trace, result.failure, options.inject_fault);
        save_trace("oracle-failure.min.trace", reduced);
        std::cerr << "Saved full failing prefix and 1-minimal trace ("
                  << reduced.size() << " commands)\n";
        return 1;
    }

    auto elapsed = std::chrono::steady_clock::now() - start;
    double seconds = std::chrono::duration<double>(elapsed).count();
    std::cout << "PASS " << description << " lines=" << trace.size()
              << " peak_live=" << result.peak_live
              << " full invariant/state audits after every command seconds=" << seconds << '\n';
    return 0;
}

}

int main(int argc, char** argv) {
    try {
        return run(read_options(argc, argv));
    } catch (const std::exception& error) {
        std::cerr << "ERROR " << error.what() << '\n';
        return 2;
    }
}
