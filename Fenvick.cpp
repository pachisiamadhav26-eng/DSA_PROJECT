#include <algorithm>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <vector>
using namespace std;

struct Player {
    int id;
    long long score;
};

struct Fenwick {
    vector<long long> scores;  // highest score first
    vector<vector<int>> ids;  // sorted player IDs at each score
    vector<int> bit;          // one-based tree of player counts

    Fenwick(vector<long long> possibleScores = {}) {
        sort(possibleScores.rbegin(), possibleScores.rend());
        for (long long score : possibleScores) {
            if (scores.empty() || scores.back() != score) scores.push_back(score);
        }
        ids.resize(scores.size());
        bit.assign(scores.size() + 1, 0);
    }

    int indexOf(long long score) const {
        auto it = lower_bound(scores.begin(), scores.end(), score, greater<long long>());
        if (it == scores.end() || *it != score) throw invalid_argument("unknown score");
        return int(it - scores.begin()) + 1;
    }

    void add(int i, int change) {
        for (; i < int(bit.size()); i += i & -i) bit[i] += change;
    }

    int prefix(int i) const {
        int count = 0;
        for (; i > 0; i -= i & -i) count += bit[i];
        return count;
    }

    void insert(Player p) {
        int i = indexOf(p.score);
        vector<int>& bucket = ids[i - 1];
        auto place = lower_bound(bucket.begin(), bucket.end(), p.id);
        if (place != bucket.end() && *place == p.id) return;
        bucket.insert(place, p.id);
        add(i, 1);
    }

    void erase(Player p) {
        int i = indexOf(p.score);
        vector<int>& bucket = ids[i - 1];
        auto place = lower_bound(bucket.begin(), bucket.end(), p.id);
        if (place == bucket.end() || *place != p.id) return;
        bucket.erase(place);
        add(i, -1);
    }

    int rank(long long score) const {
        // All coordinates before this score have strictly higher scores.
        return 1 + prefix(indexOf(score) - 1);
    }

    Player select(int k) const {
        int n = int(scores.size());
        if (k < 1 || k > prefix(n)) throw out_of_range("invalid position");
        int step = 1;
        while (step <= n / 2) step *= 2;

        // Skip blocks containing fewer than k players. Keep the remaining position.
        int skipped = 0;
        while (step > 0) {
            int next = skipped + step;
            if (next <= n && bit[next] < k) {
                k -= bit[next];
                skipped = next;
            }
            step /= 2;
        }
        // The answer is at BIT coordinate skipped + 1, or vector index skipped.
        return {ids[skipped][k - 1], scores[skipped]};
    }

    void range(int first, int last, vector<Player>& result) const {
        int total = prefix(int(scores.size()));
        for (int k = max(1, first); k <= min(last, total); ++k)
            result.push_back(select(k));
    }

    void rebuild() {
        int n = int(scores.size());
        bit.assign(n + 1, 0);
        for (int i = 1; i <= n; ++i) {
            bit[i] += int(ids[i - 1].size());
            int parent = i + (i & -i);
            if (parent <= n) bit[parent] += bit[i];
        }
    }

    // Keep scores below cutoff here and return the other division.
    Fenwick split(long long cutoff) {
        Fenwick high;
        high.scores = scores;
        high.ids.resize(scores.size());
        for (int i = 0; i < int(scores.size()) && scores[i] >= cutoff; ++i)
            high.ids[i].swap(ids[i]);
        high.rebuild();
        rebuild();
        return high;
    }

    // The leagues have distinct players; their score sets may overlap.
    void join(Fenwick& other) {
        if (this == &other) throw invalid_argument("cannot join a league to itself");
        vector<long long> combinedScores;
        vector<vector<int>> combinedIds;
        int i = 0, j = 0;
        int n = int(scores.size()), m = int(other.scores.size());
        while (i < n || j < m) {
            combinedIds.push_back({});
            vector<int>& bucket = combinedIds.back();
            if (j == m || (i < n && scores[i] > other.scores[j])) {
                combinedScores.push_back(scores[i]);
                bucket.swap(ids[i++]);
            } else if (i == n || other.scores[j] > scores[i]) {
                combinedScores.push_back(other.scores[j]);
                bucket.swap(other.ids[j++]);
            } else {
                combinedScores.push_back(scores[i]);
                bucket.resize(ids[i].size() + other.ids[j].size());
                merge(ids[i].begin(), ids[i].end(), other.ids[j].begin(),
                      other.ids[j].end(), bucket.begin());
                other.ids[j].clear();
                ++i;
                ++j;
            }
        }
        scores.swap(combinedScores);
        ids.swap(combinedIds);
        rebuild();
        other.rebuild();
    }
};

// The replay uses this lookup when a query supplies only a player ID.
unordered_map<int, long long> currentScore;

int main() {
    Fenwick tree({40, 70, 90, 100});
    vector<Player> players = {{1, 100}, {2, 90}, {3, 90}, {4, 70}, {5, 40}};
    for (Player p : players) {
        currentScore[p.id] = p.score;
        tree.insert(p);
    }

    cout << "Rank of player 3: " << tree.rank(currentScore.at(3)) << '\n';
    cout << "Player at position 3: " << tree.select(3).id << '\n';
    vector<Player> page;
    tree.range(2, 4, page);
    cout << "Players at positions 2 to 4:";
    for (Player p : page) cout << ' ' << p.id;
    cout << '\n';

    tree.erase({4, currentScore.at(4)});
    currentScore[4] = 100;
    tree.insert({4, 100});
    tree.erase({5, currentScore.at(5)});
    currentScore.erase(5);

    Fenwick high = tree.split(100);
    cout << "At or above 100: " << high.prefix(int(high.scores.size())) << '\n';
    cout << "Below 100: " << tree.prefix(int(tree.scores.size())) << '\n';
    tree.join(high);
    cout << "Players after join: " << tree.prefix(int(tree.scores.size())) << '\n';
    return 0;
}
