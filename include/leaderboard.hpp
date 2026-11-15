#pragma once

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace leaderboard {

using PlayerId = std::uint64_t;
using Score = std::int64_t;
using Position = std::uint64_t;

struct Player {
    PlayerId id;
    Score score;
};

class Board {
public:
    virtual ~Board() = default;

    virtual Position size() const = 0;
    virtual bool contains(Player player) const = 0;
    virtual void insert(Player player) = 0;
    virtual void erase(Player player) = 0;
    virtual Position count_strictly_higher(Score score) const = 0;
    virtual Player select(Position position) const = 0;
    virtual void range(Position first, Position last,
                       std::vector<Player>& result) const = 0;
    virtual std::pair<std::unique_ptr<Board>, std::unique_ptr<Board>>
    split_by_score(Score threshold) = 0;
    virtual void merge_from(Board& other) = 0;
};

std::unique_ptr<Board> make_avl();
std::unique_ptr<Board> make_fenwick(const std::vector<Score>& possible_scores);

}
