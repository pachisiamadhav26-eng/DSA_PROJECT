#include "support.hpp"
#include "replay.hpp"
#include <functional>
#include <iostream>
#include <sstream>
#ifndef _WIN32
#include <thread>
#endif
#ifdef BENCH_ORACLE
#include "sorted_array.hpp"
#endif
#if defined(BENCH_AVL_TOOLS) || defined(BENCH_FENWICK_TOOLS)
#include "backend_tools.hpp"
#endif

namespace bench {
std::unique_ptr<Board> make_board(const std::string& backend, const std::vector<Score>& scores) {
#ifdef BENCH_HAS_AVL
    if (backend == "avl") return make_avl();
#endif
#ifdef BENCH_HAS_FENWICK
    if (backend == "fenwick") return make_fenwick(scores);
#endif
    (void)scores;
    throw std::invalid_argument("backend not built: " + backend);
}

struct Population {
    std::vector<Player> players;
    std::vector<Score> coordinates;
    std::mt19937_64 random;
    Population(std::uint64_t n, std::uint64_t universe, std::uint64_t seed) : random(seed) {
        for (std::uint64_t i = 0; i < n; ++i)
            players.push_back({i + 1, static_cast<Score>(random() % universe)});
        // Every configured coordinate is supplied, including scores used only by updates.
        for (std::uint64_t i = 0; i < universe; ++i) coordinates.push_back(static_cast<Score>(i));
    }
    std::unique_ptr<Board> board(const std::string& backend) const {
        auto result = make_board(backend, coordinates);
        for (Player p : players) result->insert(p);
        return result;
    }
};

void audit_structure(const std::string& backend, const Board& board) {
    (void)backend; (void)board;
#ifdef BENCH_AVL_TOOLS
    if (backend == "avl") (void)audit_avl(board);
#endif
#ifdef BENCH_FENWICK_TOOLS
    if (backend == "fenwick") (void)audit_fenwick(board);
#endif
}

// Use Om's implementation for small differential checks once its header is compatible.
void validate_queries(const Board& board, const std::vector<Player>& current,
                      const std::vector<Score>& rank_scores,
                      const std::vector<Position>& positions) {
    const auto ordered = sorted(current);
    check_board(board, ordered);
#ifdef BENCH_ORACLE
    oracle::SortedArray reference(current);
#endif
    for (Score score : rank_scores) {
        const auto expected = expected_rank(ordered, score);
        require(1 + board.count_strictly_higher(score) == expected, "rank mismatch");
#ifdef BENCH_ORACLE
        require(1 + reference.count_strictly_higher(score) == expected, "oracle rank mismatch");
#endif
    }
    for (Position position : positions) {
        const auto expected = ordered.at(static_cast<std::size_t>(position - 1));
        require(equal(board.select(position), expected), "select mismatch");
#ifdef BENCH_ORACLE
        require(equal(reference.select(position), expected), "oracle select mismatch");
#endif
    }
}

void queries(const Options& options, const std::string& backend, std::uint64_t run, Csv& csv) {
    const auto n = options.number("--players", 1000);
    const auto universe = options.number("--scores", 1000);
    const auto updates = options.number("--updates", 1000);
    const auto batch = options.number("--batch", 256);
    const auto seed = options.number("--seed", 253);
    Population input(n, universe, seed);
    auto board = input.board(backend);
    Row rank, select;
    rank.backend = select.backend = backend;
    rank.experiment = select.experiment = "updates_with_batched_queries_" + options.get("--score-policy", "custom");
    rank.n = select.n = n;
    rank.coordinates = select.coordinates = universe;
    rank.seed = select.seed = seed;
    rank.run = select.run = run;
    rank.operation = "rank";
    select.operation = "select";
    rank.validation = select.validation = n <= 2000 ? "all_queries_and_snapshots" : "final_snapshot";
    std::vector<Score> rank_scores;
    std::vector<Position> positions;
    for (std::uint64_t done = 0; done < updates;) {
        const auto count = std::min(batch, updates - done);
        // Updates and query preparation are outside the query timers.
        for (std::uint64_t i = 0; i < count; ++i) {
            const auto slot = input.random() % n;
            Player& player = input.players[slot];
            const Score score = static_cast<Score>((player.score + 1 + input.random() % (universe - 1)) % universe);
            board->erase(player);
            player.score = score;
            board->insert(player);
        }
        rank_scores.clear();
        positions.clear();
        for (std::uint64_t i = 0; i < count; ++i) {
            rank_scores.push_back(input.players[input.random() % n].score);
            positions.push_back(1 + input.random() % n);
        }
        if (n <= 2000) validate_queries(*board, input.players, rank_scores, positions);
        double start = now_ns();
        for (Score score : rank_scores) consume(rank.checksum, 1 + board->count_strictly_higher(score));
        rank.elapsed += now_ns() - start;
        start = now_ns();
        for (Position position : positions) {
            const auto player = board->select(position);
            consume(select.checksum, player.id);
            consume(select.checksum, static_cast<std::uint64_t>(player.score));
        }
        select.elapsed += now_ns() - start;
        rank.calls += count;
        select.calls += count;
        done += count;
    }
    validate_queries(*board, input.players, rank_scores, positions);
    audit_structure(backend, *board);
    csv.write(rank);
    csv.write(select);
}

void seasons(const Options& options, const std::string& backend, std::uint64_t run, Csv& csv) {
    const auto n = options.number("--players", 1000);
    const auto universe = options.number("--scores", 1000);
    const auto seed = options.number("--seed", 253);
    const auto percent = options.number("--high-percent", 50);
    Population input(n, universe, seed);
    const auto expected = sorted(input.players);
    const Score cutoff = percent == 0 ? static_cast<Score>(universe) :
        expected[static_cast<std::size_t>(std::max<std::uint64_t>(1, n * percent / 100) - 1)].score;
    auto boundary = std::find_if(expected.begin(), expected.end(),
        [cutoff](Player p) { return p.score < cutoff; });
    const std::vector<Player> expected_high(expected.begin(), boundary), expected_low(boundary, expected.end());
    // Bounded batches keep the number of simultaneously allocated players near 100000.
    const auto copies = std::max<std::uint64_t>(1, std::min<std::uint64_t>(1000, 100000 / n));
    std::vector<std::unique_ptr<Board>> boards;
    for (std::uint64_t i = 0; i < copies; ++i) boards.push_back(input.board(backend));
    using Pieces = std::pair<std::unique_ptr<Board>, std::unique_ptr<Board>>;
    std::vector<Pieces> divisions(copies);
    Row row;
    row.backend = backend;
    row.experiment = "season_high_percent_" + std::to_string(percent) + "_" + options.get("--score-policy", "custom");
    row.operation = "split";
    row.n = n; row.coordinates = universe; row.seed = seed; row.run = run; row.calls = copies;
    row.validation = "all_output_players";
    double start = now_ns();
    for (std::size_t i = 0; i < copies; ++i) divisions[i] = boards[i]->split_by_score(cutoff);
    row.elapsed = now_ns() - start;
    for (std::size_t i = 0; i < copies; ++i) {
        require(boards[i]->size() == 0, "split source not empty");
        check_board(*divisions[i].first, expected_high);
        check_board(*divisions[i].second, expected_low);
        audit_structure(backend, *divisions[i].first);
        audit_structure(backend, *divisions[i].second);
    }
    csv.write(row);
    row.operation = "rejoin_merge_from";
    start = now_ns();
    for (auto& parts : divisions) parts.first->merge_from(*parts.second);
    row.elapsed = now_ns() - start;
    for (auto& parts : divisions) {
        require(parts.second->size() == 0, "merge donor not empty");
        check_board(*parts.first, expected);
        audit_structure(backend, *parts.first);
    }
    csv.write(row);
}

void ordered_join(const Options& options, std::uint64_t run, Csv& csv) {
#ifdef BENCH_AVL_TOOLS
    const auto n = options.number("--players", 1000);
    const auto percent = options.number("--high-percent", 50);
    const auto first_count = (n - 1) * percent / 100;
    std::vector<Player> expected;
    for (std::uint64_t i = 0; i < n; ++i) expected.push_back({i + 1, static_cast<Score>(n - i)});
    const Player pivot = expected[first_count];
    const auto copies = std::max<std::uint64_t>(1, std::min<std::uint64_t>(1000, 100000 / n));
    std::vector<std::unique_ptr<Board>> left, right, results(copies);
    for (std::uint64_t copy = 0; copy < copies; ++copy) {
        left.push_back(make_avl());
        right.push_back(make_avl());
        for (std::uint64_t i = 0; i < first_count; ++i) left.back()->insert(expected[i]);
        for (std::uint64_t i = first_count + 1; i < n; ++i) right.back()->insert(expected[i]);
    }
    Row row;
    row.backend = "avl"; row.experiment = "ordered_join_percent_" + std::to_string(percent);
    row.operation = "ordered_join_supplied_pivot"; row.n = n; row.coordinates = n;
    row.seed = options.number("--seed", 253); row.run = run; row.calls = copies;
    row.left_height = avl_stats(*left.front()).height; row.right_height = avl_stats(*right.front()).height;
    row.validation = "all_output_players";
    const double start = now_ns();
    for (std::size_t i = 0; i < copies; ++i) results[i] = avl_join_ordered(*left[i], pivot, *right[i]);
    row.elapsed = now_ns() - start;
    std::uint64_t visits = 0;
    for (std::size_t i = 0; i < copies; ++i) {
        require(left[i]->size() == 0 && right[i]->size() == 0, "ordered join sources not empty");
        check_board(*results[i], expected);
        audit_structure("avl", *results[i]);
        visits += avl_stats(*results[i]).last_join_visits;
    }
    csv.write(row);
    row.operation = "join_visits_count"; row.elapsed = 0; row.checksum = visits;
    csv.write(row);
#else
    (void)options; (void)run; (void)csv;
    throw std::runtime_error("ordered join requires the team's avl_join_ordered and avl_stats helpers; rebuild with --avl-tools");
#endif
}

// Wrapper times only the backend calls. Trace parsing, application hash lookup,
// formatting, and output happen outside these intervals. Per-call timer overhead
// remains included, so trace rows are labelled separately from batched queries.
struct TraceTotals {
    std::map<std::string, Row> rows;
    std::uint64_t max_live = 0, live = 0;
};
class MeasuredBoard final : public Board {
    std::unique_ptr<Board> board;
    std::shared_ptr<TraceTotals> totals;
    template <typename Function> auto measure(const char* name, Function action) const -> decltype(action()) {
        Row& row = totals->rows[name];
        const double start = now_ns();
        auto result = action();
        row.elapsed += now_ns() - start;
        ++row.calls;
        return result;
    }
public:
    MeasuredBoard(std::unique_ptr<Board> source, std::shared_ptr<TraceTotals> stats)
        : board(std::move(source)), totals(std::move(stats)) {}
    Position size() const override { return board->size(); }
    bool contains(Player p) const override { return board->contains(p); }
    void insert(Player p) override {
        measure("insert", [&] { board->insert(p); return 0; });
        ++totals->live;
        totals->max_live = std::max(totals->max_live, totals->live);
    }
    void erase(Player p) override {
        measure("erase", [&] { board->erase(p); return 0; });
        --totals->live;
    }
    Position count_strictly_higher(Score s) const override {
        return measure("rank", [&] { return board->count_strictly_higher(s); });
    }
    Player select(Position k) const override { return measure("select", [&] { return board->select(k); }); }
    void range(Position a, Position b, std::vector<Player>& out) const override {
        measure("range", [&] { board->range(a, b, out); return 0; });
    }
    std::pair<std::unique_ptr<Board>, std::unique_ptr<Board>> split_by_score(Score s) override {
        auto result = measure("split", [&] { return board->split_by_score(s); });
        return {std::make_unique<MeasuredBoard>(std::move(result.first), totals),
                std::make_unique<MeasuredBoard>(std::move(result.second), totals)};
    }
    void merge_from(Board& other) override {
        auto& donor = dynamic_cast<MeasuredBoard&>(other);
        measure("merge_from", [&] { board->merge_from(*donor.board); return 0; });
        totals->max_live = std::max(totals->max_live, board->size());
    }
};

class DigestBuffer : public std::streambuf {
public:
    std::uint64_t checksum = 0, lines = 0;
protected:
    int_type overflow(int_type c) override {
        if (!traits_type::eq_int_type(c, traits_type::eof())) {
            const auto ch = traits_type::to_char_type(c);
            consume(checksum, static_cast<unsigned char>(ch));
            if (ch == '\n') ++lines;
        }
        return traits_type::not_eof(c);
    }
};

void trace(const Options& options, const std::string& backend, std::uint64_t run, Csv& csv) {
    const auto path = options.get("--trace", "");
    std::ifstream input(path);
    require(input.good(), "cannot open trace: " + path);
    auto totals = std::make_shared<TraceTotals>();
    std::uint64_t coordinates = 0;
    DigestBuffer digest;
    std::ostream output(&digest);
    run_replay(input, output, [&](const std::vector<Score>& scores) -> std::unique_ptr<Board> {
        if (coordinates == 0) {
            auto unique = scores;
            std::sort(unique.begin(), unique.end());
            unique.erase(std::unique(unique.begin(), unique.end()), unique.end());
            coordinates = unique.size();
        }
        return std::make_unique<MeasuredBoard>(make_board(backend, scores), totals);
    });
    std::string validation = "output_digest_only";
#ifdef BENCH_ORACLE
    if (options.number("--check-trace", 0)) {
        std::ifstream reference_input(path);
        DigestBuffer reference_digest;
        std::ostream reference_output(&reference_digest);
        run_replay(reference_input, reference_output, [](const std::vector<Score>&) -> std::unique_ptr<Board> {
            return std::make_unique<oracle::SortedArray>();
        });
        require(reference_digest.lines == digest.lines && reference_digest.checksum == digest.checksum,
                "trace output differs from oracle");
        validation = "oracle_output_digest";
    }
#else
    require(options.number("--check-trace", 0) == 0, "trace checks require an oracle-enabled build");
#endif
    for (auto& entry : totals->rows) {
        Row& row = entry.second;
        row.backend = backend; row.experiment = "trace_per_call_overhead_included";
        row.operation = entry.first; row.n = totals->max_live; row.coordinates = coordinates;
        row.run = run; row.checksum = digest.checksum; row.validation = validation;
        row.seed = options.number("--seed", 253);
        csv.write(row);
    }
}

void counters(const Options& options, const std::string& backend, std::uint64_t run, Csv& csv) {
    (void)csv;
    Population input(options.number("--players", 1000), options.number("--scores", 1000), options.number("--seed", 253));
    auto board = input.board(backend);
    Row row;
    row.backend = backend; row.experiment = "construction_counters"; row.operation = "unavailable";
    row.n = input.players.size(); row.coordinates = input.coordinates.size();
    row.seed = options.number("--seed", 253); row.run = run; row.calls = 1;
    row.validation = "snapshot";
    check_board(*board, sorted(input.players));
#ifdef BENCH_AVL_TOOLS
    if (backend == "avl") {
        const auto stats = avl_stats(*board);
        row.operation = "rotations_count"; row.checksum = stats.rotations;
        row.left_height = stats.height;
        csv.write(row);
    }
#endif
#ifdef BENCH_FENWICK_TOOLS
    if (backend == "fenwick") {
        const auto stats = fenwick_stats(*board);
        row.operation = "bucket_shifts_count"; row.checksum = stats.bucket_shifts;
        csv.write(row);
    }
#endif
    require(row.operation != "unavailable", "backend does not expose requested counter hooks");
    const auto updates = options.number("--updates", 1000);
    const auto universe = input.coordinates.size();
    const auto initial = row.checksum;
    (void)initial;
    for (std::uint64_t i = 0; i < updates; ++i) {
        Player& player = input.players[input.random() % input.players.size()];
        const Score score = static_cast<Score>((player.score + 1 + input.random() % (universe - 1)) % universe);
        board->erase(player);
        player.score = score;
        board->insert(player);
    }
    check_board(*board, sorted(input.players));
#ifdef BENCH_AVL_TOOLS
    if (backend == "avl") {
        row.checksum = avl_stats(*board).rotations - initial;
        row.operation = "revision_rotations_count";
    }
#endif
#ifdef BENCH_FENWICK_TOOLS
    if (backend == "fenwick") {
        row.checksum = fenwick_stats(*board).bucket_shifts - initial;
        row.operation = "revision_bucket_shifts_count";
    }
#endif
    row.experiment = "revision_counters_" + std::to_string(updates) + "_updates";
    csv.write(row);
}

void wait_file(const std::string& path) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
    while (!std::ifstream(path).good()) {
        require(std::chrono::steady_clock::now() < deadline, "profile handshake timed out: " + path);
#ifdef _WIN32
        Sleep(10);
#else
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
#endif
    }
}
void signal_file(const std::string& path) {
    std::ofstream file(path); file << "ready\n";
    require(file.good(), "cannot write profile signal: " + path);
}

void profile(const Options& options, const std::string& backend) {
    Population input(options.number("--players", 100000), options.number("--scores", 100000), options.number("--seed", 253));
    auto board = input.board(backend);
    const auto operation = options.get("--operation", "rank");
    require(operation == "rank" || operation == "select", "profile operation must be rank or select");
    std::vector<Player> questions;
    std::vector<Score> rank_scores;
    std::vector<Position> positions;
    for (int i = 0; i < 65536; ++i) {
        questions.push_back(input.players[input.random() % input.players.size()]);
        rank_scores.push_back(questions.back().score);
        positions.push_back(1 + input.random() % input.players.size());
    }
    validate_queries(*board, input.players, {rank_scores.front()}, {positions.front()});
    const std::string control = options.get("--control-dir", "");
    require(!control.empty(), "profile requires --control-dir");
    signal_file(control + "/ready");
    wait_file(control + "/go");
    std::uint64_t checksum = 0, calls = 0;
    const auto stop = std::chrono::steady_clock::now() + std::chrono::seconds(options.number("--seconds", 5));
    do {
        for (std::size_t i = 0; i < questions.size(); ++i) {
            if (operation == "rank") consume(checksum, 1 + board->count_strictly_higher(rank_scores[i]));
            else {
                const auto player = board->select(positions[i]);
                consume(checksum, player.id); consume(checksum, static_cast<std::uint64_t>(player.score));
            }
        }
        calls += questions.size();
    } while (std::chrono::steady_clock::now() < stop);
    std::cout << "profile_backend=" << backend << " operation=" << operation
              << " calls=" << calls << " checksum=" << checksum << std::endl;
    signal_file(control + "/done");
    wait_file(control + "/release");
}
}

int main(int argc, char** argv) {
    try {
        bench::Options options(argc, argv);
        const std::vector<std::string> allowed{"--backend", "--experiment", "--players", "--scores", "--updates",
            "--batch", "--seed", "--runs", "--output", "--high-percent", "--trace", "--check-trace",
            "--operation", "--control-dir", "--seconds", "--score-policy", "--help"};
        for (const auto& option : options.values)
            bench::require(std::find(allowed.begin(), allowed.end(), option.first) != allowed.end(), "unknown option: " + option.first);
        if (options.values.count("--help")) {
            std::cout << "runner --backend avl|fenwick --experiment queries|seasons|ordered-join|trace|counters|profile\n"
                      << " --players N --scores U --updates M --batch B --seed S --runs R --output FILE\n"
                      << " --high-percent P --trace FILE --check-trace 0|1\n";
            return 0;
        }
        const auto n = options.number("--players", 1000), universe = options.number("--scores", 1000);
        bench::require(n >= 1 && n <= 1000000 && universe >= 2 && universe <= 1000000,
                       "players must be 1..1000000 and scores 2..1000000");
        const auto runs = options.number("--runs", 5);
        bench::require(runs >= 1 && runs <= 100 && options.number("--updates", 1000) >= 1 &&
                       options.number("--updates", 1000) <= 10000000 && options.number("--batch", 256) >= 1 &&
                       options.number("--batch", 256) <= 1000000 && options.number("--high-percent", 50) <= 100 &&
                       options.number("--check-trace", 0) <= 1 && options.number("--seconds", 5) >= 1 &&
                       options.number("--seconds", 5) <= 3600, "invalid run/update/batch/percentage/check/seconds option");
        const std::string backend = options.get("--backend", "avl");
        bench::require(backend == "avl" || backend == "fenwick", "backend must be avl or fenwick");
        const auto experiment = options.get("--experiment", "queries");
        if (experiment == "profile") { bench::profile(options, backend); return 0; }
        bench::Csv csv(options.get("--output", "raw.csv"));
        for (std::uint64_t run = 1; run <= runs; ++run) {
            if (experiment == "queries") bench::queries(options, backend, run, csv);
            else if (experiment == "seasons") bench::seasons(options, backend, run, csv);
            else if (experiment == "ordered-join") {
                bench::require(backend == "avl", "ordered join is AVL-only");
                bench::ordered_join(options, run, csv);
            } else if (experiment == "trace") bench::trace(options, backend, run, csv);
            else if (experiment == "counters") bench::counters(options, backend, run, csv);
            else throw std::invalid_argument("unknown experiment: " + experiment);
            std::cout << backend << ' ' << experiment << " run " << run << '/' << runs << " passed\n";
        }
    } catch (const std::exception& error) {
        std::cerr << "BENCHMARK FAILED: " << error.what() << '\n';
        return 1;
    }
}
