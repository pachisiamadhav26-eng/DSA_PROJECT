#include "backend_tools.hpp"
#include <algorithm>
#include <functional>
#include <iterator>
#include <stdexcept>
#include <unordered_set>
using namespace std;
typedef long long ll;

namespace leaderboard {
namespace {

struct FenwickBoard : Board {
    vector<ll> scores;
    vector<vector<PlayerId>> ids;
    vector<ll> bit;
    Position total=0;
    unsigned long long shifts=0;

    FenwickBoard(const vector<Score>& possibleScores={}){
        scores.assign(possibleScores.begin(), possibleScores.end());
        sort(scores.rbegin(), scores.rend());
        scores.erase(unique(scores.begin(), scores.end()), scores.end());
        ids.resize(scores.size());
        bit.resize(scores.size()+1);
    }

    // Number of prepared scores that are strictly higher.
    ll scoreIndex(ll score) const{
        return lower_bound(scores.begin(), scores.end(), score, greater<ll>())-scores.begin();
    }

    ll prefix(ll i) const{
        ll sum=0;
        for (; i>0; i-=i&-i) sum+=bit[i];
        return sum;
    }

    // Buckets start at 0. The Fenwick array starts at 1.
    void add(ll i, ll delta){
        for (i++; i<(ll)bit.size(); i+=i&-i) bit[i]+=delta;
    }

    void rebuild(){
        fill(bit.begin(), bit.end(), 0);
        total=0;
        for (ll i=1; i<(ll)bit.size(); i++){
            ll count=ids[i-1].size();
            bit[i]+=count;
            total+=count;
            ll parent=i+(i&-i);
            if (parent<(ll)bit.size()) bit[parent]+=bit[i];
        }
    }

    Position size() const{
        return total;
    }

    bool contains(Player p) const{
        ll i=scoreIndex(p.score);
        return i<(ll)scores.size() && scores[i]==p.score &&
               binary_search(ids[i].begin(), ids[i].end(), p.id);
    }

    void insert(Player p){
        ll i=scoreIndex(p.score);
        if (i==(ll)scores.size() || scores[i]!=p.score)
            throw invalid_argument("Score was not prepared");
        auto& bucket=ids[i];
        auto pos=lower_bound(bucket.begin(), bucket.end(), p.id);
        if (pos!=bucket.end() && *pos==p.id)
            throw invalid_argument("Duplicate player key");
        ll moved=bucket.end()-pos;
        bucket.insert(pos,p.id);
        shifts+=moved;
        add(i,1);
        total++;
    }

    void erase(Player p){
        ll i=scoreIndex(p.score);
        if (i==(ll)scores.size() || scores[i]!=p.score)
            throw invalid_argument("Player key not found");
        auto& bucket=ids[i];
        auto pos=lower_bound(bucket.begin(), bucket.end(), p.id);
        if (pos==bucket.end() || *pos!=p.id)
            throw invalid_argument("Player key not found");
        shifts+=bucket.end()-pos-1;
        bucket.erase(pos);
        add(i,-1);
        total--;
    }

    Position count_strictly_higher(Score score) const{
        return prefix(scoreIndex(score));
    }

    Player select(Position k) const{
        if (k==0 || k>total) throw out_of_range("Invalid position");
        ll n=scores.size();
        ll d=1;
        while (d<=n/2) d*=2;

        ll x=0;
        // Skip a block only if the answer lies after it.
        for (; d>0; d/=2){
            ll next=x+d;
            if (next<=n && bit[next]<(ll)k){
                k-=bit[next];
                x=next;
            }
        }
        return {ids[x][k-1], scores[x]};
    }

    void range(Position first, Position last, vector<Player>& result) const{
        if (first==0 || first>last || last>total) throw out_of_range("Invalid range");
        result.reserve(last-first+1);
        result.clear();
        for (Position k=first; k<=last; k++) result.push_back(select(k));
    }

    pair<unique_ptr<Board>, unique_ptr<Board>> split_by_score(Score threshold){
        auto high=make_unique<FenwickBoard>();
        auto low=make_unique<FenwickBoard>();
        high->scores=low->scores=scores;
        high->ids.resize(scores.size());
        low->ids.resize(scores.size());
        high->bit.resize(bit.size());
        low->bit.resize(bit.size());
        for (ll i=0; i<(ll)scores.size(); i++){
            if (scores[i]>=threshold) high->ids[i].swap(ids[i]);
            else low->ids[i].swap(ids[i]);
        }
        high->rebuild();
        low->rebuild();
        rebuild();
        return {move(high),move(low)};
    }

    void merge_from(Board& other){
        auto* donor=dynamic_cast<FenwickBoard*>(&other);
        if (donor==nullptr || donor==this) throw invalid_argument("Invalid merge");

        unordered_set<PlayerId> seen;
        seen.reserve(total+donor->total);
        for (const auto& bucket : ids) seen.insert(bucket.begin(),bucket.end());
        for (const auto& bucket : donor->ids){
            for (PlayerId id : bucket){
                if (seen.count(id)) throw invalid_argument("ID_CONFLICT");
            }
        }

        FenwickBoard joined;
        set_union(scores.begin(), scores.end(), donor->scores.begin(), donor->scores.end(),
                  back_inserter(joined.scores), greater<ll>());
        joined.ids.resize(joined.scores.size());
        joined.bit.resize(joined.scores.size()+1);
        ll i=0, j=0;
        for (ll k=0; k<(ll)joined.scores.size(); k++){
            ll score=joined.scores[k];
            bool left=i<(ll)scores.size() && scores[i]==score;
            bool right=j<(ll)donor->scores.size() && donor->scores[j]==score;
            if (left && right){
                auto& bucket=joined.ids[k];
                bucket.reserve(ids[i].size()+donor->ids[j].size());
                merge(ids[i].begin(), ids[i].end(), donor->ids[j].begin(), donor->ids[j].end(),
                      back_inserter(bucket));
                i++;
                j++;
            }
            else if (left) joined.ids[k]=ids[i++];
            else joined.ids[k]=donor->ids[j++];
        }

        joined.rebuild();
        scores.swap(joined.scores);
        ids.swap(joined.ids);
        bit.swap(joined.bit);
        total=joined.total;
        for (auto& bucket : donor->ids) bucket.clear();
        donor->rebuild();
    }

    vector<Player> audit() const{
        if (ids.size()!=scores.size() || bit.size()!=scores.size()+1 || bit[0]!=0)
            throw logic_error("Wrong Fenwick array sizes");
        vector<Player> players;
        vector<ll> sums(bit.size(),0);
        unordered_set<PlayerId> seen;
        players.reserve(total);
        seen.reserve(total);
        for (ll i=0; i<(ll)scores.size(); i++){
            if (i>0 && scores[i-1]<=scores[i]) throw logic_error("Score order");
            for (ll j=0; j<(ll)ids[i].size(); j++){
                PlayerId id=ids[i][j];
                if ((j>0 && ids[i][j-1]>=id) || !seen.insert(id).second)
                    throw logic_error("Player ID order or duplicate");
                players.push_back({id,scores[i]});
            }
            ll k=i+1;
            sums[k]=sums[k-1]+ids[i].size();
            if (bit[k]!=sums[k]-sums[k-(k&-k)]) throw logic_error("Fenwick count");
        }
        if (sums.back()!=(ll)total) throw logic_error("Wrong board size");
        return players;
    }
};

}

unique_ptr<Board> make_fenwick(const vector<Score>& scores){
    return make_unique<FenwickBoard>(scores);
}

FenwickStats fenwick_stats(const Board& board){
    const auto& tree=dynamic_cast<const FenwickBoard&>(board);
    return {tree.scores.size(),tree.shifts};
}

vector<Player> audit_fenwick(const Board& board){
    return dynamic_cast<const FenwickBoard&>(board).audit();
}

}
