#include "backend_tools.hpp"
#include <unordered_set>
#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace leaderboard {
namespace {

struct Node {
    Player player;
    Node* left = nullptr;
    Node* right = nullptr;
    int height = 1;
    Position size = 1;
};

int height(Node* t) { return t ? t->height : 0; }
Position nodeSize(Node* t) { return t ? t->size : 0; }

bool before(Player a, Player b) {
    return a.score != b.score ? a.score > b.score : a.id < b.id;
}

void refresh(Node* t) {
    if (!t) return;
    t->height = 1 + std::max(height(t->left), height(t->right));
    t->size = 1 + nodeSize(t->left) + nodeSize(t->right);
}

#ifdef AVL_ADVERSARY_TEST
struct {
    int rotations = 0;
    int erase_height = 0;
    int rotations_before_erase = 0;
    int erase_rotations = 0;
    int join_left_height = -1;
    int join_right_height = -1;
} adversary_probe;
#endif

Node* rotateRight(Node* t) {
#ifdef AVL_ADVERSARY_TEST
    adversary_probe.rotations++;
#endif
    Node* p = t->left;
    t->left = p->right;
    p->right = t;
    refresh(t);
    refresh(p);
    return p;
}

Node* rotateLeft(Node* t) {
#ifdef AVL_ADVERSARY_TEST
    adversary_probe.rotations++;
#endif
    Node* p = t->right;
    t->right = p->left;
    p->left = t;
    refresh(t);
    refresh(p);
    return p;
}

Node* balance(Node* t) {
    if (!t) return nullptr;
    refresh(t);
    int d = height(t->left) - height(t->right);

    if (d > 1) {
        if (height(t->left->right) > height(t->left->left))
            t->left = rotateLeft(t->left);
        return rotateRight(t);
    }

    if (d < -1) {
        if (height(t->right->left) > height(t->right->right))
            t->right = rotateRight(t->right);
        return rotateLeft(t);
    }
    return t;
}

Node* insertNode(Node* t, Node* p) {
    if (!t) return p;
    if (before(p->player, t->player))
        t->left = insertNode(t->left, p);
    else
        t->right = insertNode(t->right, p);
    return balance(t);
}

Node* eraseNode(Node* t, Player p) {
    if (before(p, t->player))
        t->left = eraseNode(t->left, p);
    else if (before(t->player, p))
        t->right = eraseNode(t->right, p);
    else {
        if (!t->left || !t->right) {
            Node* child = t->left ? t->left : t->right;
            delete t;
            return child;
        }

        Node* next = t->right;
        while (next->left) next = next->left;
        t->player = next->player;
        t->right = eraseNode(t->right, next->player);
    }
    return balance(t);
}

Node* joinNodes(Node* a, Node* pivot, Node* b) {
#ifdef AVL_ADVERSARY_TEST
    if (adversary_probe.join_left_height < 0) {
        adversary_probe.join_left_height = height(a);
        adversary_probe.join_right_height = height(b);
    }
#endif
    if (height(a) > height(b) + 1) {
        a->right = joinNodes(a->right, pivot, b);
        return balance(a);
    }

    if (height(b) > height(a) + 1) {
        b->left = joinNodes(a, pivot, b->left);
        return balance(b);
    }

    pivot->left = a;
    pivot->right = b;
    refresh(pivot);
    return pivot;
}

template<class BeforeCut>
void splitNodes(Node* t, BeforeCut before_cut, Node*& first, Node*& second) {
    if (!t) {
        first = second = nullptr;
        return;
    }

    if (before_cut(t->player)) {
        Node* middle = nullptr;
        splitNodes(t->right, before_cut, middle, second);
        first = joinNodes(t->left, t, middle);
    }
    else {
        Node* middle = nullptr;
        splitNodes(t->left, before_cut, first, middle);
        second = joinNodes(middle, t, t->right);
    }
}

Node* firstNode(Node* t) {
    while (t && t->left) t = t->left;
    return t;
}

Node* lastNode(Node* t) {
    while (t && t->right) t = t->right;
    return t;
}

Node* takeLast(Node* t, Node*& last) {
    if (!t->right) {
        last = t;
        Node* left = t->left;
        t->left = t->right = nullptr;
        refresh(t);
        return left;
    }

    t->right = takeLast(t->right, last);
    return balance(t);
}

void rangeNodes(Node* t, Position first, Position last,
                std::vector<Player>& out) {
    if (!t || first > last) return;

    Position pos = nodeSize(t->left) + 1;

    if (first < pos)
        rangeNodes(t->left, first, last, out);

    if (first <= pos && pos <= last)
        out.push_back(t->player);

    if (pos < last)
        rangeNodes(t->right, first > pos ? first - pos : 1,
                   last - pos, out);
}

void inorder(Node* t, std::vector<Node*>& nodes) {
    if (!t) return;
    inorder(t->left, nodes);
    nodes.push_back(t);
    inorder(t->right, nodes);
}

Node* build(std::vector<Node*>& nodes, std::size_t l, std::size_t r) {
    if (l == r) return nullptr;

    std::size_t m = l + (r - l) / 2;
    Node* t = nodes[m];
    t->left = build(nodes, l, m);
    t->right = build(nodes, m + 1, r);
    refresh(t);
    return t;
}

void destroy(Node* t) {
    if (!t) return;
    destroy(t->left);
    destroy(t->right);
    delete t;
}

bool audit(Node* t, const Player* low, const Player* high,
           int& h, Position& s, std::unordered_set<Node*>& visited,
           std::unordered_set<PlayerId>& ids, std::vector<Player>& contents) {
    if (!t) {
        h = 0;
        s = 0;
        return true;
    }

    if (!visited.insert(t).second || !ids.insert(t->player.id).second) return false;
    if (low && !before(*low, t->player)) return false;
    if (high && !before(t->player, *high)) return false;

    int lh = 0, rh = 0;
    Position ls = 0, rs = 0;

    if (!audit(t->left, low, &t->player, lh, ls, visited, ids, contents)) return false;
    contents.push_back(t->player);
    if (!audit(t->right, &t->player, high, rh, rs, visited, ids, contents)) return false;

    h = 1 + std::max(lh, rh);
    s = 1 + ls + rs;

    return std::abs(lh - rh) <= 1 &&
           t->height == h && t->size == s;
}

struct AvlBoard final : Board {
    Node* root = nullptr;

    ~AvlBoard() override { destroy(root); }
    AvlBoard() = default;
    AvlBoard(const AvlBoard&) = delete;
    AvlBoard& operator=(const AvlBoard&) = delete;

    Position size() const noexcept override {
        return nodeSize(root);
    }

    bool contains(Player p) const noexcept override {
        Node* t = root;
        while (t) {
            if (before(p, t->player)) t = t->left;
            else if (before(t->player, p)) t = t->right;
            else return true;
        }
        return false;
    }

    void insert(Player p) override {
        if (contains(p))
            throw std::invalid_argument("Duplicate player key");
        root = insertNode(root, new Node{p});
    }

    void erase(Player p) override {
        if (!contains(p))
            throw std::out_of_range("Player not found");
#ifdef AVL_ADVERSARY_TEST
        adversary_probe.erase_height = height(root);
        adversary_probe.rotations_before_erase = adversary_probe.rotations;
#endif
        root = eraseNode(root, p);
#ifdef AVL_ADVERSARY_TEST
        adversary_probe.erase_rotations = adversary_probe.rotations - adversary_probe.rotations_before_erase;
#endif
    }

    Position count_strictly_higher(Score score) const noexcept override {
        Position count = 0;
        Node* t = root;

        while (t) {
            if (t->player.score > score) {
                count += nodeSize(t->left) + 1;
                t = t->right;
            }
            else t = t->left;
        }
        return count;
    }

    Player select(Position k) const override {
        if (!k || k > size())
            throw std::out_of_range("Invalid position");

        Node* t = root;
        while (t) {
            Position pos = nodeSize(t->left) + 1;
            if (k == pos) return t->player;
            if (k < pos) t = t->left;
            else {
                k -= pos;
                t = t->right;
            }
        }
        throw std::logic_error("Incorrect subtree size");
    }

    void range(Position first, Position last,
               std::vector<Player>& out) const override {
        if (!first || first > last || last > size())
            throw std::out_of_range("Invalid range");

        out.reserve(static_cast<std::size_t>(last - first + 1));
        out.clear();
        rangeNodes(root, first, last, out);
    }

    std::pair<std::unique_ptr<Board>, std::unique_ptr<Board>>
    split_by_score(Score cut) override {
        return split([cut](Player p) { return p.score >= cut; });
    }

    std::pair<std::unique_ptr<Board>, std::unique_ptr<Board>>
    split_by_key(Player key) {
        return split([key](Player p) { return before(p, key); });
    }

    template<class BeforeCut>
    std::pair<std::unique_ptr<Board>, std::unique_ptr<Board>> split(BeforeCut before_cut) {
        auto first = std::make_unique<AvlBoard>();
        auto second = std::make_unique<AvlBoard>();
        splitNodes(root, before_cut, first->root, second->root);
        root = nullptr;
        return {std::move(first), std::move(second)};
    }

    void merge_from(Board& other) override {
        auto* b = dynamic_cast<AvlBoard*>(&other);
        if (!b || b == this)
            throw std::invalid_argument("Invalid merge board");

        if (!b->root) return;
        if (!root) {
            root = b->root;
            b->root = nullptr;
            return;
        }

        std::vector<Node*> a, c;
        inorder(root, a);
        inorder(b->root, c);

        std::vector<PlayerId> ids;
        ids.reserve(a.size() + c.size());

        for (Node* p : a) ids.push_back(p->player.id);
        for (Node* p : c) ids.push_back(p->player.id);

        std::sort(ids.begin(), ids.end());
        if (std::adjacent_find(ids.begin(), ids.end()) != ids.end())
            throw std::invalid_argument("ID_CONFLICT");

        const bool root_first = before(lastNode(root)->player, firstNode(b->root)->player);
        if (root_first || before(lastNode(b->root)->player, firstNode(root)->player)) {
            Node* left = root_first ? root : b->root;
            Node* right = root_first ? b->root : root;
            Node* pivot = nullptr;
            left = takeLast(left, pivot);
            root = joinNodes(left, pivot, right);
            b->root = nullptr;
            return;
        }

        std::vector<Node*> all;
        all.reserve(a.size() + c.size());
        std::merge(a.begin(), a.end(), c.begin(), c.end(),
                   std::back_inserter(all),
                   [](Node* x, Node* y) {
                       return before(x->player, y->player);
                   });

        root = build(all, 0, all.size());
        b->root = nullptr;
    }

};

}

std::unique_ptr<Board> make_avl() {
    return std::make_unique<AvlBoard>();
}

int avl_height(const Board& board) {
    return height(dynamic_cast<const AvlBoard&>(board).root);
}

std::vector<Player> audit_avl(const Board& board) {
    const auto& tree = dynamic_cast<const AvlBoard&>(board);
    int height = 0;
    Position size = 0;
    std::unordered_set<Node*> visited;
    std::unordered_set<PlayerId> ids;
    std::vector<Player> contents;
    if (!audit(tree.root, nullptr, nullptr, height, size, visited, ids, contents))
        throw std::logic_error("AVL ordering, metadata, balance or ownership");
    return contents;
}

std::pair<std::unique_ptr<Board>, std::unique_ptr<Board>>
avl_split_by_key(Board& source, Player key) {
    return dynamic_cast<AvlBoard&>(source).split_by_key(key);
}

std::unique_ptr<Board> avl_join_ordered(Board& left, Player pivot, Board& right) {
    auto& a = dynamic_cast<AvlBoard&>(left);
    auto& b = dynamic_cast<AvlBoard&>(right);
    if (&a == &b) throw std::invalid_argument("join sources must differ");
    auto result = std::make_unique<AvlBoard>();
    Node* middle = new Node{pivot};
    result->root = joinNodes(a.root, middle, b.root);
    a.root = b.root = nullptr;
    return result;
}

}
