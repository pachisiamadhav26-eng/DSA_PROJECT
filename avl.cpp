#include "leaderboard.hpp"
#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

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

Node* rotateRight(Node* t) {
    Node* p = t->left;
    t->left = p->right;
    p->right = t;
    refresh(t);
    refresh(p);
    return p;
}

Node* rotateLeft(Node* t) {
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

void splitScore(Node* t, Score cut, Node*& high, Node*& low) {
    if (!t) {
        high = low = nullptr;
        return;
    }

    if (t->player.score >= cut) {
        Node* middle = nullptr;
        splitScore(t->right, cut, middle, low);
        Node* left = t->left;
        t->left = t->right = nullptr;
        high = joinNodes(left, t, middle);
    }
    else {
        Node* middle = nullptr;
        splitScore(t->left, cut, high, middle);
        Node* right = t->right;
        t->left = t->right = nullptr;
        low = joinNodes(middle, t, right);
    }
}

void splitKey(Node* t, Player key, Node*& first, Node*& second) {
    if (!t) {
        first = second = nullptr;
        return;
    }

    if (before(t->player, key)) {
        Node* middle = nullptr;
        splitKey(t->right, key, middle, second);
        Node* left = t->left;
        t->left = t->right = nullptr;
        first = joinNodes(left, t, middle);
    }
    else {
        Node* middle = nullptr;
        splitKey(t->left, key, first, middle);
        Node* right = t->right;
        t->left = t->right = nullptr;
        second = joinNodes(middle, t, right);
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
           int& h, Position& s) {
    if (!t) {
        h = 0;
        s = 0;
        return true;
    }

    if (low && !before(*low, t->player)) return false;
    if (high && !before(t->player, *high)) return false;

    int lh = 0, rh = 0;
    Position ls = 0, rs = 0;

    if (!audit(t->left, low, &t->player, lh, ls)) return false;
    if (!audit(t->right, &t->player, high, rh, rs)) return false;

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
        root = eraseNode(root, p);
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

        std::vector<Player> temp;
        temp.reserve(static_cast<std::size_t>(last - first + 1));
        rangeNodes(root, first, last, temp);
        out.swap(temp);
    }

    std::pair<std::unique_ptr<Board>, std::unique_ptr<Board>>
    split_by_score(Score cut) override {
        auto high = std::make_unique<AvlBoard>();
        auto low = std::make_unique<AvlBoard>();
        splitScore(root, cut, high->root, low->root);
        root = nullptr;
        return {std::move(high), std::move(low)};
    }

    std::pair<std::unique_ptr<AvlBoard>, std::unique_ptr<AvlBoard>>
    split_by_key(Player key) {
        auto first = std::make_unique<AvlBoard>();
        auto second = std::make_unique<AvlBoard>();
        splitKey(root, key, first->root, second->root);
        root = nullptr;
        return {std::move(first), std::move(second)};
    }

    void join_ordered(AvlBoard& left, Player pivot, AvlBoard& right) {
        if (this == &left || this == &right ||
            &left == &right || root)
            throw std::invalid_argument("Invalid join boards");

        if (left.root && !before(lastNode(left.root)->player, pivot))
            throw std::invalid_argument("Left keys not before pivot");

        if (right.root && !before(pivot, firstNode(right.root)->player))
            throw std::invalid_argument("Right keys not after pivot");

        Node* middle = new Node{pivot};
        root = joinNodes(left.root, middle, right.root);
        left.root = right.root = nullptr;
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

        if (before(lastNode(root)->player, firstNode(b->root)->player)) {
            Node* pivot = nullptr;
            root = takeLast(root, pivot);
            root = joinNodes(root, pivot, b->root);
            b->root = nullptr;
            return;
        }

        if (before(lastNode(b->root)->player, firstNode(root)->player)) {
            Node* pivot = nullptr;
            b->root = takeLast(b->root, pivot);
            root = joinNodes(b->root, pivot, root);
            b->root = nullptr;
            return;
        }

        std::vector<Node*> a, c, all;
        inorder(root, a);
        inorder(b->root, c);

        std::vector<PlayerId> ids;
        ids.reserve(a.size() + c.size());

        for (Node* p : a) ids.push_back(p->player.id);
        for (Node* p : c) ids.push_back(p->player.id);

        std::sort(ids.begin(), ids.end());
        for (std::size_t i = 1; i < ids.size(); ++i)
            if (ids[i] == ids[i - 1])
                throw std::invalid_argument("Duplicate player ID");

        all.reserve(a.size() + c.size());
        std::merge(a.begin(), a.end(), c.begin(), c.end(),
                   std::back_inserter(all),
                   [](Node* x, Node* y) {
                       return before(x->player, y->player);
                   });

        root = build(all, 0, all.size());
        b->root = nullptr;
    }

    bool validate() const {
        int h = 0;
        Position s = 0;
        return audit(root, nullptr, nullptr, h, s);
    }
};

std::unique_ptr<Board> make_avl_board() {
    return std::make_unique<AvlBoard>();
}
