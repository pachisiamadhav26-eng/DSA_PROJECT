#pragma once
#include "sorted_array.hpp"
// Preload every header used by the standalone demos before namespacing them.
#include <functional>
#include <iostream>
#include <unordered_map>
#include <limits>
#include <unordered_set>
#define main oracle_unused_demo_main
namespace avl_demo {
#include "avl.cpp"
}
namespace fenwick_demo {
#include "Fenvick.cpp"
}
#undef main

namespace oracle {
inline void require(bool ok, const char* message) {
    if (!ok) throw std::logic_error(message);
}
inline int legacy_id(PlayerId id) {
    if (id > static_cast<PlayerId>(std::numeric_limits<int>::max()))
        throw std::out_of_range("legacy demo supports IDs only through INT_MAX");
    return static_cast<int>(id);
}

// Recompute metadata from topology, never from stored child sizes/heights.
inline std::pair<int,int> audit_node(const avl_demo::Node* n,
        const Player* lower, const Player* upper,
        std::unordered_set<const avl_demo::Node*>& visited,
        std::unordered_set<PlayerId>& ids, std::vector<Player>& contents) {
    if (!n) return {0,0};
    require(visited.insert(n).second, "AVL cycle/shared node");
    Player p{static_cast<PlayerId>(n->player.id), n->player.score};
    require(n->player.id >= 0 && ids.insert(p.id).second, "AVL duplicate/negative ID");
    require((!lower || before(*lower,p)) && (!upper || before(p,*upper)), "AVL BST order");
    auto l = audit_node(n->left,lower,&p,visited,ids,contents);
    contents.push_back(p);
    auto r = audit_node(n->right,&p,upper,visited,ids,contents);
    require(n->size == 1+l.second+r.second, "AVL subtree size");
    require(n->height == 1+std::max(l.first,r.first), "AVL stored height");
    require(std::abs(l.first-r.first) <= 1, "AVL balance");
    return {1+std::max(l.first,r.first),1+l.second+r.second};
}
inline std::vector<Player> audit_avl(const avl_demo::Node* root) {
    std::unordered_set<const avl_demo::Node*> visited;
    std::unordered_set<PlayerId> ids;
    std::vector<Player> result;
    audit_node(root,nullptr,nullptr,visited,ids,result);
    return result;
}
inline std::vector<Player> audit_fenwick(const fenwick_demo::Fenwick& f) {
    require(f.scores.size()==f.ids.size() && f.bit.size()==f.ids.size()+1, "Fenwick dimensions");
    require(f.bit[0]==0, "Fenwick sentinel");
    std::vector<long long> prefix(1,0);
    std::vector<Player> result;
    std::unordered_set<PlayerId> seen;
    for (std::size_t i=0;i<f.scores.size();++i) {
        require(!i || f.scores[i-1]>f.scores[i], "Fenwick score order");
        for (std::size_t j=0;j<f.ids[i].size();++j) {
            int id=f.ids[i][j];
            require(id>=0 && (!j || f.ids[i][j-1]<id) && seen.insert(id).second,
                    "Fenwick ID order/uniqueness");
            result.push_back({static_cast<PlayerId>(id),f.scores[i]});
        }
        prefix.push_back(prefix.back()+f.ids[i].size());
        std::size_t k=i+1;
        require(f.bit[k]>=0 && f.bit[k]==prefix[k]-prefix[k-(k&(~k+1))], "Fenwick block count");
    }
    // The block equations imply every prefix; explicitly compare actual prefixes too.
    for (std::size_t k=0;k<prefix.size();++k)
        require(f.prefix(static_cast<int>(k))==prefix[k], "Fenwick prefix sum");
    return result;
}

class AvlBoard final : public Board {
public:
    mutable avl_demo::AVL tree;
    avl_demo::Node* root=nullptr;
    ~AvlBoard() override { tree.destroy(root); }
    AvlBoard() = default;
    AvlBoard(const AvlBoard&) = delete;
    AvlBoard& operator=(const AvlBoard&) = delete;
    Position size() const override { return tree.size(root); }
    void insert(Player p) override { root=tree.insert(root,{legacy_id(p.id),p.score}); }
    void erase(Player p) override { root=tree.erase(root,{legacy_id(p.id),p.score}); }
    Position count_strictly_higher(Score s) const override { return tree.rank(root,s)-1; }
    Player select(Position k) const override {
        if (!k || k>size()) throw std::out_of_range("position");
        auto p=tree.select(root,static_cast<int>(k));
        return {static_cast<PlayerId>(p.id),p.score};
    }
    void range(Position first,Position last,std::vector<Player>& out) const override {
        if (!first || first>last || last>size()) throw std::out_of_range("range");
        std::vector<avl_demo::Player> page;
        tree.range(root,static_cast<int>(first),static_cast<int>(last),page);
        out.clear();
        for (auto p:page) out.push_back({static_cast<PlayerId>(p.id),p.score});
    }
    std::pair<std::unique_ptr<Board>,std::unique_ptr<Board>> split_by_score(Score s) override {
        auto high=std::make_unique<AvlBoard>(); auto low=std::make_unique<AvlBoard>();
        tree.split(root,s,high->root,low->root); root=nullptr;
        return {std::move(high),std::move(low)};
    }
    void merge_from(Board& source) override {
        auto other=dynamic_cast<AvlBoard*>(&source);
        if (!other || other==this) throw std::invalid_argument("merge backend/self");
        if (!other->root) return;
        if (!root) { root=other->root; other->root=nullptr; return; }
        // Exercise the real ordered join when ranges permit. Arbitrary overlap
        // has no standalone primitive: the adapter uses ordinary AVL insertions.
        if (before(select(size()),other->select(1))) {
            auto pivot=tree.select(root,tree.size(root));
            root=tree.erase(root,pivot);
            root=tree.join(root,pivot,other->root); other->root=nullptr;
        } else if (before(other->select(other->size()),select(1))) {
            auto pivot=tree.select(other->root,tree.size(other->root));
            other->root=tree.erase(other->root,pivot);
            root=tree.join(other->root,pivot,root); other->root=nullptr;
        } else {
            auto entries=audit_avl(other->root);
            for (auto p:entries) insert(p);
            tree.destroy(other->root); other->root=nullptr;
        }
    }
};
class FenwickBoard final : public Board {
public:
    fenwick_demo::Fenwick tree;
    explicit FenwickBoard(const std::vector<Score>& scores)
        : tree(std::vector<long long>(scores.begin(),scores.end())) {}
    Position size() const override { return tree.prefix(static_cast<int>(tree.scores.size())); }
    void insert(Player p) override { tree.insert({legacy_id(p.id),p.score}); }
    void erase(Player p) override { tree.erase({legacy_id(p.id),p.score}); }
    Position count_strictly_higher(Score s) const override {
        auto i=std::lower_bound(tree.scores.begin(),tree.scores.end(),s,std::greater<long long>());
        return tree.prefix(static_cast<int>(i-tree.scores.begin()));
    }
    Player select(Position k) const override {
        if (!k || k>size()) throw std::out_of_range("position");
        auto p=tree.select(static_cast<int>(k));
        return {static_cast<PlayerId>(p.id),p.score};
    }
    void range(Position first,Position last,std::vector<Player>& out) const override {
        if (!first || first>last || last>size()) throw std::out_of_range("range");
        std::vector<fenwick_demo::Player> page;
        tree.range(static_cast<int>(first),static_cast<int>(last),page); out.clear();
        for (auto p:page) out.push_back({static_cast<PlayerId>(p.id),p.score});
    }
    std::pair<std::unique_ptr<Board>,std::unique_ptr<Board>> split_by_score(Score s) override {
        auto high=std::make_unique<FenwickBoard>(std::vector<Score>{});
        auto low=std::make_unique<FenwickBoard>(std::vector<Score>{});
        high->tree=tree.split(s); low->tree=std::move(tree);
        tree=fenwick_demo::Fenwick();
        return {std::move(high),std::move(low)};
    }
    void merge_from(Board& source) override {
        auto other=dynamic_cast<FenwickBoard*>(&source);
        if (!other || other==this) throw std::invalid_argument("merge backend/self");
        tree.join(other->tree);
    }
};
inline std::vector<Player> inspect(const Board& b) {
    if (auto a=dynamic_cast<const AvlBoard*>(&b)) return audit_avl(a->root);
    if (auto f=dynamic_cast<const FenwickBoard*>(&b)) return audit_fenwick(f->tree);
    auto& o=dynamic_cast<const SortedArray&>(b); o.audit(); return o.entries();
}
}
