#include "include/backend_tools.hpp"
#include <algorithm>
#include <cstdlib>
#include <stdexcept>

namespace leaderboard {
namespace { // Keep helper names private to this file.
// IDs are globally unique by the application contract. Higher scores come first.
bool before(Player a, Player b) {
    if (a.score != b.score) return a.score > b.score;
    return a.id < b.id;
}

// Node* stores the address of a node; nullptr means there is no node.
// n->left means the left child of the node pointed to by n.
struct Node {
    Player player;
    Node* left = nullptr;
    Node* right = nullptr;
    int height = 1;
    Position size = 1;
};

int height(const Node* n) { return n ? n->height : 0; }
Position nodeSize(const Node* n) { return n ? n->size : 0; }
// Recalculate metadata after changing children.
void refresh(Node* n) {
    n->height = 1 + std::max(height(n->left), height(n->right));
    n->size = 1 + nodeSize(n->left) + nodeSize(n->right);
}

// These counters measure rotations and recursive join calls.
struct Work {
    std::uint64_t rotations = 0;
    std::uint64_t visits = 0;
};
Node* rotateLeft(Node* n, Work& w) {
    Node* p = n->right;
    n->right = p->left;
    p->left = n;
    refresh(n);
    refresh(p);
    ++w.rotations;
    return p;
}
Node* rotateRight(Node* n, Work& w) {
    Node* p = n->left;
    n->left = p->right;
    p->right = n;
    refresh(n);
    refresh(p);
    ++w.rotations;
    return p;
}

// Repair LL, LR, RR or RL imbalance.
Node* balance(Node* n, Work& w) {
    if (!n) return nullptr;
    refresh(n);
    int d = height(n->left) - height(n->right);
    if (d > 1) {
        if (height(n->left->right) > height(n->left->left))
            n->left = rotateLeft(n->left, w);
        return rotateRight(n, w);
    }
    if (d < -1) {
        if (height(n->right->left) > height(n->right->right))
            n->right = rotateRight(n->right, w);
        return rotateLeft(n, w);
    }
    return n;
}

Node* insertNode(Node* n, Node* p, Work& w) {
    if (!n) return p;
    if (before(p->player, n->player))
        n->left = insertNode(n->left, p, w);
    else
        n->right = insertNode(n->right, p, w);
    return balance(n, w);
}

// Ordinary BST deletion, then rebalance while returning up the tree.
Node* eraseNode(Node* n, Player p, Work& w) {
    if (!n) return nullptr;
    if (before(p, n->player))
        n->left = eraseNode(n->left, p, w);
    else if (before(n->player, p))
        n->right = eraseNode(n->right, p, w);
    else {
        if (!n->left || !n->right) {
            Node* child = n->left ? n->left : n->right;
            delete n;
            return child;
        }
        Node* next = n->right;
        while (next->left) next = next->left;
        n->player = next->player;
        n->right = eraseNode(n->right, next->player, w);
    }
    return balance(n, w);
}

// Precondition: every key in a precedes pivot, which precedes every key in b.
Node* joinNodes(Node* a, Node* pivot, Node* b, Work& w) {
    ++w.visits;
    if (height(a) > height(b) + 1) {
        a->right = joinNodes(a->right, pivot, b, w);
        return balance(a, w);
    }
    if (height(b) > height(a) + 1) {
        b->left = joinNodes(a, pivot, b->left, w);
        return balance(b, w);
    }
    pivot->left = a;
    pivot->right = b;
    refresh(pivot);
    return pivot;
}

// Split at a score: equal scores go into the high tree.
// Node*& lets this function change the caller's output pointer.
void splitScore(Node* n, Score cut, Node*& high, Node*& low, Work& w) {
    if (!n) {
        high = low = nullptr;
        return;
    }
    Node* middle = nullptr;
    if (n->player.score >= cut) {
        splitScore(n->right, cut, middle, low, w);
        high = joinNodes(n->left, n, middle, w);
    } else {
        splitScore(n->left, cut, high, middle, w);
        low = joinNodes(middle, n, n->right, w);
    }
}

// Split at a full (score, id) key. An equal key goes into the second tree.
void splitKey(Node* n, Player key, Node*& firstTree, Node*& secondTree, Work& w) {
    if (!n) {
        firstTree = secondTree = nullptr;
        return;
    }
    Node* middle = nullptr;
    if (before(n->player, key)) {
        splitKey(n->right, key, middle, secondTree, w);
        firstTree = joinNodes(n->left, n, middle, w);
    } else {
        splitKey(n->left, key, firstTree, middle, w);
        secondTree = joinNodes(middle, n, n->right, w);
    }
}
Node* first(Node* n) {
    while (n->left) n = n->left;
    return n;
}
Node* last(Node* n) {
    while (n->right) n = n->right;
    return n;
}
Node* takeLast(Node* n, Node*& pivot, Work& w) {
    if (!n->right) {
        pivot = n;
        Node* child = n->left;
        n->left = n->right = nullptr;
        refresh(n);
        return child;
    }
    n->right = takeLast(n->right, pivot, w);
    return balance(n, w);
}

void rangeNodes(const Node* n, Position lo, Position hi, std::vector<Player>& out) {
    if (!n || lo > hi) return;
    Position pos = nodeSize(n->left) + 1;
    if (lo < pos) rangeNodes(n->left, lo, hi, out);
    if (lo <= pos && pos <= hi) out.push_back(n->player);
    if (pos < hi) {
        Position rightStart = 1;
        if (lo > pos) rightStart = lo - pos;
        rangeNodes(n->right, rightStart, hi - pos, out);
    }
}

void collect(Node* n, std::vector<Node*>& out) {
    if (!n) return;
    collect(n->left, out);
    out.push_back(n);
    collect(n->right, out);
}

// The middle element becomes the root of each balanced subtree.
Node* build(std::vector<Node*>& nodes, std::size_t lo, std::size_t hi) {
    if (lo == hi) return nullptr;
    std::size_t mid = lo + (hi - lo) / 2;
    Node* n = nodes[mid];
    n->left = build(nodes, lo, mid);
    n->right = build(nodes, mid + 1, hi);
    refresh(n);
    return n;
}

void destroy(Node* n) {
    if (!n) return;
    destroy(n->left);
    destroy(n->right);
    delete n;
}

// Board methods are the functions the application calls.
// override means this method implements a method declared in Board.
// unique_ptr owns a board; std::move transfers that ownership.
class AvlBoard final : public Board {
public:
    Node* root = nullptr;
    // Counters belong to this board instance; transferred nodes carry no history.
    Work work;
    AvlBoard() = default;
    // Copying raw tree pointers would cause the same nodes to be deleted twice.
    AvlBoard(const AvlBoard&) = delete;
    AvlBoard& operator=(const AvlBoard&) = delete;
    ~AvlBoard() override { destroy(root); }
    Position size() const override { return nodeSize(root); }
    bool contains(Player p) const override {
        const Node* n = root;
        while (n) {
            if (before(p, n->player)) n = n->left;
            else if (before(n->player, p)) n = n->right;
            else return true;
        }
        return false;
    }
    void insert(Player p) override {
        if (contains(p)) throw std::invalid_argument("duplicate player key");
        root = insertNode(root, new Node{p}, work);
    }
    void erase(Player p) override {
        if (!contains(p)) throw std::out_of_range("missing player key");
        root = eraseNode(root, p, work);
    }
    // Competition rank is this count + 1. Equal scores are not counted.
    Position count_strictly_higher(Score score) const override {
        Position count = 0;
        const Node* n = root;
        while (n) {
            if (n->player.score > score) {
                count += nodeSize(n->left) + 1;
                n = n->right;
            } else n = n->left;
        }
        return count;
    }
    // Left subtree size tells us the current node's position.
    Player select(Position k) const override {
        if (!k || k > size()) throw std::out_of_range("invalid position");
        const Node* n = root;
        while (n) {
            Position pos = nodeSize(n->left) + 1;
            if (k == pos) return n->player;
            if (k < pos) n = n->left;
            else {
                k -= pos;
                n = n->right;
            }
        }
        throw std::logic_error("corrupt subtree size");
    }
    void range(Position lo, Position hi, std::vector<Player>& out) const override {
        if (!lo || lo > hi || hi > size()) throw std::out_of_range("invalid range");
        std::vector<Player> result;
        result.reserve(static_cast<std::size_t>(hi - lo + 1));
        rangeNodes(root, lo, hi, result);
        out.swap(result);
    }
    std::pair<std::unique_ptr<Board>, std::unique_ptr<Board>>
    split_by_score(Score cut) override {
        auto high = std::make_unique<AvlBoard>();
        auto low = std::make_unique<AvlBoard>();
        work.visits = 0;
        splitScore(root, cut, high->root, low->root, work);
        root = nullptr;
        return {std::move(high), std::move(low)};
    }
    void merge_from(Board& other) override {
        auto* b = dynamic_cast<AvlBoard*>(&other);
        if (!b || b == this) throw std::invalid_argument("invalid merge boards");
        work.visits = 0;
        if (!b->root) return;
        if (!root) {
            root = b->root;
            b->root = nullptr;
            return;
        }
        Node* pivot = nullptr;
        if (before(last(root)->player, first(b->root)->player)) {
            root = takeLast(root, pivot, work);
            root = joinNodes(root, pivot, b->root, work);
        } else if (before(last(b->root)->player, first(root)->player)) {
            b->root = takeLast(b->root, pivot, work);
            root = joinNodes(b->root, pivot, root, work);
        } else {
            // O(n+m): application guarantees globally distinct IDs.
            // Finish all allocations before modifying either tree.
            std::vector<Node*> a, c, all;
            a.reserve(static_cast<std::size_t>(size()));
            c.reserve(static_cast<std::size_t>(b->size()));
            collect(root, a);
            collect(b->root, c);
            all.reserve(a.size() + c.size());
            // Merge two sorted lists, just like the merge step of merge sort.
            std::size_t i = 0, j = 0;
            while (i < a.size() && j < c.size()) {
                if (before(a[i]->player, c[j]->player)) all.push_back(a[i++]);
                else all.push_back(c[j++]);
            }
            while (i < a.size()) all.push_back(a[i++]);
            while (j < c.size()) all.push_back(c[j++]);
            root = build(all, 0, all.size());
        }
        b->root = nullptr;
    }
};
AvlBoard& asAvl(Board& b) {
    // Check that this Board really is an AVL board.
    auto* a = dynamic_cast<AvlBoard*>(&b);
    if (!a) throw std::invalid_argument("expected AVL board");
    return *a;
}
const AvlBoard& asAvl(const Board& b) {
    auto* a = dynamic_cast<const AvlBoard*>(&b);
    if (!a) throw std::invalid_argument("expected AVL board");
    return *a;
}

// Recompute height and size from the children, like the original audit.
void checkTree(const Node* n, int& h, Position& count, std::vector<Player>& out) {
    if (!n) {
        h = 0;
        count = 0;
        return;
    }
    int leftHeight = 0, rightHeight = 0;
    Position leftSize = 0, rightSize = 0;
    checkTree(n->left, leftHeight, leftSize, out);
    out.push_back(n->player);
    checkTree(n->right, rightHeight, rightSize, out);
    h = 1 + std::max(leftHeight, rightHeight);
    count = 1 + leftSize + rightSize;
    if (std::abs(leftHeight - rightHeight) > 1 || n->height != h || n->size != count)
        throw std::logic_error("invalid AVL height/balance/size");
}
} // namespace

// Public wrappers below match the team's shared header.
std::unique_ptr<Board> make_avl() { return std::make_unique<AvlBoard>(); }
std::vector<Player> audit_avl(const Board& board) {
    int h = 0;
    Position count = 0;
    std::vector<Player> players;
    checkTree(asAvl(board).root, h, count, players);
    std::vector<PlayerId> ids;
    for (std::size_t i = 0; i < players.size(); ++i) {
        if (i > 0 && !before(players[i - 1], players[i]))
            throw std::logic_error("invalid BST order");
        ids.push_back(players[i].id);
    }
    std::sort(ids.begin(), ids.end());
    for (std::size_t i = 1; i < ids.size(); ++i)
        if (ids[i - 1] == ids[i]) throw std::logic_error("duplicate ID");
    return players;
}
AvlStats avl_stats(const Board& board) {
    const auto& a = asAvl(board);
    return {height(a.root), a.work.rotations, a.work.visits};
}
std::pair<std::unique_ptr<Board>, std::unique_ptr<Board>>
avl_split_by_key(Board& source, Player key) {
    AvlBoard& tree = asAvl(source);
    auto firstTree = std::make_unique<AvlBoard>();
    auto secondTree = std::make_unique<AvlBoard>();
    tree.work.visits = 0;
    splitKey(tree.root, key, firstTree->root, secondTree->root, tree.work);
    tree.root = nullptr;
    return {std::move(firstTree), std::move(secondTree)};
}
std::unique_ptr<Board> avl_join_ordered(Board& left, Player pivot, Board& right) {
    auto& a = asAvl(left);
    auto& b = asAvl(right);
    if (&a == &b) throw std::invalid_argument("join requires distinct boards");
    auto result = std::make_unique<AvlBoard>();
    Node* middle = new Node{pivot};
    // Ordering and unique IDs are caller preconditions: no boundary scans.
    result->root = joinNodes(a.root, middle, b.root, result->work);
    a.root = b.root = nullptr;
    return result;
}
} // namespace leaderboard
