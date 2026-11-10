#include "leaderboard.h"
#include <algorithm>

// These three functions are the whole Fenwick algorithm. All indices here start at 1.
void add(FenwickTree& tree, int i, int delta) {
    while (i < int(tree.bit.size())) {
        tree.bit[i] += delta;
        i += i & -i;
    }
}

int prefixSum(const FenwickTree& tree, int i) {
    int sum = 0;
    while (i > 0) {
        sum += tree.bit[i];
        i -= i & -i;
    }
    return sum;
}

// For a valid k, return the first index whose prefix sum reaches k.
int findKth(const FenwickTree& tree, int k) {
    int n = int(tree.bit.size()) - 1;
    int step = 1;
    while (step <= n / 2) step *= 2;
    int i = 0;
    while (step > 0) {
        int next = i + step;
        if (next <= n && tree.bit[next] < k) {
            k -= tree.bit[next];  // skip this entire block of players
            i = next;
        }
        step /= 2;
    }
    return i + 1;
}

void clearBoard(Board& b) { b = Board{}; }

void initBoard(Board& b, const std::vector<long long>& possibleScores) {
    clearBoard(b);
    std::vector<long long> sorted = possibleScores;
    std::sort(sorted.begin(), sorted.end());
    for (long long score : sorted) {
        if (b.scores.empty() || b.scores.back() != score) b.scores.push_back(score);
    }
    b.ids.resize(b.scores.size());
    b.tree.bit.assign(b.scores.size() + 1, 0);
}

// Score and ID vectors are zero-based. Add 1 when calling the Fenwick functions.
int scoreIndex(const Board& b, long long score) {
    int i = int(std::lower_bound(b.scores.begin(), b.scores.end(), score) - b.scores.begin());
    if (i == int(b.scores.size()) || b.scores[i] != score) return -1;
    return i;
}

void putInBucket(Board& b, int i, long long id) {
    std::vector<long long>& ids = b.ids[i];
    ids.insert(std::lower_bound(ids.begin(), ids.end(), id), id);
    add(b.tree, i + 1, 1);
}

void takeFromBucket(Board& b, int i, long long id) {
    std::vector<long long>& ids = b.ids[i];
    ids.erase(std::lower_bound(ids.begin(), ids.end(), id));
    add(b.tree, i + 1, -1);
}

bool insertPlayer(Board& b, long long id, long long score) {
    int i = scoreIndex(b, score);
    if (i < 0 || findPlayer(b.players, id)) return false;
    putInBucket(b, i, id);
    addRecord(b.players, {id, score});
    return true;
}

bool erasePlayer(Board& b, long long id) {
    Player* p = findPlayer(b.players, id);
    if (p == nullptr) return false;
    takeFromBucket(b, scoreIndex(b, p->score), id);
    removeRecord(b.players, id);
    return true;
}

bool updateScore(Board& b, long long id, long long newScore) {
    Player* p = findPlayer(b.players, id);
    int dest = scoreIndex(b, newScore);
    if (p == nullptr || dest < 0) return false;
    if (p->score == newScore) return true;
    putInBucket(b, dest, id);
    takeFromBucket(b, scoreIndex(b, p->score), id);
    p->score = newScore;
    return true;
}

int rankOf(Board& b, long long id) {
    Player* p = findPlayer(b.players, id);
    if (p == nullptr) return 0;
    int i = scoreIndex(b, p->score);
    return 1 + b.players.count - prefixSum(b.tree, i + 1);
}

bool selectPlayer(const Board& b, int position, Player& answer) {
    int total = b.players.count;
    if (position < 1 || position > total) return false;
    int i = findKth(b.tree, total - position + 1) - 1;
    int higher = total - prefixSum(b.tree, i + 1);
    int offset = position - higher - 1;
    answer = {b.ids[i][offset], b.scores[i]};
    return true;
}

std::vector<Player> listPositions(const Board& b, int first, int last) {
    std::vector<Player> result;
    if (first < 1 || last > b.players.count || first > last) return result;
    for (int k = first; k <= last; ++k) {
        Player p;
        selectPlayer(b, k, p);
        result.push_back(p);
    }
    return result;
}

// Build all blocks in one pass over score counts.
void rebuildTree(Board& b) {
    int n = int(b.scores.size());
    b.tree.bit.assign(n + 1, 0);
    for (int i = 1; i <= n; ++i) {
        b.tree.bit[i] += int(b.ids[i - 1].size());
        int parent = i + (i & -i);
        if (parent <= n) b.tree.bit[parent] += b.tree.bit[i];
    }
}

bool splitBoard(Board& b, long long threshold, Board& high) {
    if (&b == &high || high.players.count != 0) return false;
    high.scores = b.scores;
    high.ids.assign(b.scores.size(), {});
    for (int i = 0; i < int(b.scores.size()); ++i) {
        if (b.scores[i] < threshold) continue;
        high.ids[i].swap(b.ids[i]);
        for (long long id : high.ids[i]) {
            addRecord(high.players, {id, b.scores[i]});
            removeRecord(b.players, id);
        }
    }
    rebuildTree(b);
    rebuildTree(high);
    return true;
}

bool mergeBoards(Board& b, Board& other) {
    if (&b == &other || haveCommonId(b.players, other.players)) return false;
    std::vector<long long> scores;
    std::vector<std::vector<long long>> ids;
    int i = 0, j = 0;
    while (i < int(b.scores.size()) || j < int(other.scores.size())) {
        if (j == int(other.scores.size()) ||
            (i < int(b.scores.size()) && b.scores[i] < other.scores[j])) {
            scores.push_back(b.scores[i]);
            ids.push_back(b.ids[i]);
            ++i;
        } else if (i == int(b.scores.size()) || other.scores[j] < b.scores[i]) {
            scores.push_back(other.scores[j]);
            ids.push_back(other.ids[j]);
            ++j;
        } else {
            scores.push_back(b.scores[i]);
            // Both lists are sorted by ID. Merge them into one tie group.
            std::vector<long long> group(b.ids[i].size() + other.ids[j].size());
            std::merge(b.ids[i].begin(), b.ids[i].end(), other.ids[j].begin(),
                       other.ids[j].end(), group.begin());
            ids.push_back(group);
            ++i;
            ++j;
        }
    }
    b.scores.swap(scores);
    b.ids.swap(ids);
    transferRecords(b.players, other.players);
    other.ids.assign(other.scores.size(), {});
    rebuildTree(b);
    rebuildTree(other);
    return true;
}
