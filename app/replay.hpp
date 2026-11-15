#pragma once

#include "leaderboard.hpp"

#include <functional>
#include <iosfwd>
#include <memory>
#include <vector>

namespace leaderboard {

using BoardFactory = std::function<std::unique_ptr<Board>(const std::vector<Score>&)>;
using BoardAudit = std::vector<Player> (*)(const Board&);

void run_replay(std::istream& input, std::ostream& output,
                const BoardFactory& factory, BoardAudit audit = nullptr);

}
