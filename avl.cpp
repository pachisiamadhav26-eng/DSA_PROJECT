
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <stdexcept>
#include <vector>
using namespace std;

using PlayerId = uint64_t;
using Score = int64_t;
using Position = uint64_t;

struct Player {
    PlayerId id;
    Score score;
};

struct Node {
    Player player;
    Node* left = nullptr;
    Node* right = nullptr;
    int height = 1;
    Position size = 1;
};

struct AVL {
    int height(Node* t) const {
        return t ? t->height : 0;
    }

    Position size(Node* t) const {
        return t ? t->size : 0;
    }

    bool before(Player a, Player b) const {
        if (a.score != b.score) return a.score > b.score;
        return a.id < b.id;
    }

    void refresh(Node* t) const {
        if (!t) return;
        t->height = 1 + max(height(t->left), height(t->right));
        t->size = 1 + size(t->left) + size(t->right);
    }

    Node* rotateRight(Node* top) const {
        Node* pivot = top->left;
        top->left = pivot->right;
        pivot->right = top;
        refresh(top);
        refresh(pivot);
        return pivot;
    }

    Node* rotateLeft(Node* top) const {
        Node* pivot = top->right;
        top->right = pivot->left;
        pivot->left = top;
        refresh(top);
        refresh(pivot);
        return pivot;
    }

    Node* balance(Node* t) const {
        if (!t) return nullptr;

        refresh(t);
        int diff = height(t->left) - height(t->right);

        if (diff > 1) {
            if (height(t->left->right) > height(t->left->left))
                t->left = rotateLeft(t->left);
            return rotateRight(t);
        }

        if (diff < -1) {
            if (height(t->right->left) > height(t->right->right))
                t->right = rotateRight(t->right);
            return rotateLeft(t);
        }

        return t;
    }

    bool contains(Node* t, Player p) const {
        while (t) {
            if (before(p, t->player))
                t = t->left;
            else if (before(t->player, p))
                t = t->right;
            else
                return true;
        }
        return false;
    }

    Node* insert(Node* t, Player p) const {
        if (!t) return new Node{p};

        if (before(p, t->player))
            t->left = insert(t->left, p);
        else if (before(t->player, p))
            t->right = insert(t->right, p);
        else
            throw invalid_argument("Duplicate player key");

        return balance(t);
    }

    Node* erase(Node* t, Player p) const {
        if (!t)
            throw out_of_range("Player not found");

        if (before(p, t->player))
            t->left = erase(t->left, p);
        else if (before(t->player, p))
            t->right = erase(t->right, p);
        else {
            if (!t->left || !t->right) {
                Node* child = t->left ? t->left : t->right;
                delete t;
                return child;
            }

            Node* next = t->right;
            while (next->left)
                next = next->left;

            t->player = next->player;
            t->right = erase(t->right, next->player);
        }

        return balance(t);
    }

    Position count_strictly_higher(Node* t, Score score) const {
        Position count = 0;

        while (t) {
            if (t->player.score > score) {
                count += size(t->left) + 1;
                t = t->right;
            }
            else {
                t = t->left;
            }
        }
        return count;
    }

    Position rank(Node* t, Score score) const {
        return count_strictly_higher(t, score) + 1;
    }

    Player select(Node* t, Position k) const {
        if (k == 0 || k > size(t))
            throw out_of_range("Invalid position");

        while (t) {
            Position leftSize = size(t->left);

            if (k == leftSize + 1)
                return t->player;

            if (k <= leftSize)
                t = t->left;
            else {
                k -= leftSize + 1;
                t = t->right;
            }
        }

        throw logic_error("Incorrect subtree size");
    }

    void rangeRec(Node* t, Position first, Position last,
                  vector<Player>& result) const {
        if (!t || first > last) return;

        Position pos = size(t->left) + 1;

        if (first < pos)
            rangeRec(t->left, first, last, result);

        if (first <= pos && pos <= last)
            result.push_back(t->player);

        if (pos < last) {
            Position newFirst = first > pos ? first - pos : 1;
            rangeRec(t->right, newFirst, last - pos, result);
        }
    }

    void range(Node* t, Position first, Position last,
               vector<Player>& result) const {
        if (first == 0 || first > last || last > size(t))
            throw out_of_range("Invalid range");

        result.clear();
        rangeRec(t, first, last, result);
    }

    // All left keys must precede pivot; pivot must precede right keys.
    Node* join(Node* left, Node* pivot, Node* right) const {
        if (height(left) > height(right) + 1) {
            left->right = join(left->right, pivot, right);
            return balance(left);
        }

        if (height(right) > height(left) + 1) {
            right->left = join(left, pivot, right->left);
            return balance(right);
        }

        pivot->left = left;
        pivot->right = right;
        refresh(pivot);
        return pivot;
    }

    Node* join_ordered(Node* left, Player pivot, Node* right) const {
        return join(left, new Node{pivot}, right);
    }

    void splitScoreRec(Node* t, Score cutoff,
                       Node*& high, Node*& low) const {
        if (!t) {
            high = low = nullptr;
            return;
        }

        Node* left = t->left;
        Node* right = t->right;
        t->left = t->right = nullptr;

        if (t->player.score >= cutoff) {
            Node* middle = nullptr;

            splitScoreRec(right, cutoff, middle, low);
            high = join(left, t, middle);
        }
        else {
            Node* middle = nullptr;

            splitScoreRec(left, cutoff, high, middle);
            low = join(middle, t, right);
        }
    }

    // Consumes root; all three root variables must be distinct.
    void split_by_score(Node*& root, Score cutoff,
                        Node*& high, Node*& low) const {
        splitScoreRec(root, cutoff, high, low);
        root = nullptr;
    }

    void splitKeyRec(Node* t, Player key,
                     Node*& first, Node*& second) const {
        if (!t) {
            first = second = nullptr;
            return;
        }

        Node* left = t->left;
        Node* right = t->right;
        t->left = t->right = nullptr;

        if (before(t->player, key)) {
            Node* middle = nullptr;

            splitKeyRec(right, key, middle, second);
            first = join(left, t, middle);
        }
        else {
            Node* middle = nullptr;

            splitKeyRec(left, key, first, middle);
            second = join(middle, t, right);
        }
    }

    void split_by_key(Node*& root, Player key,
                      Node*& first, Node*& second) const {
        splitKeyRec(root, key, first, second);
        root = nullptr;
    }

    void inorder(Node* t, vector<Node*>& nodes) const {
        if (!t) return;

        inorder(t->left, nodes);
        nodes.push_back(t);
        inorder(t->right, nodes);
    }

    Node* build(vector<Node*>& nodes, size_t l, size_t r) const {
        if (l == r) return nullptr;

        size_t m = l + (r - l) / 2;
        Node* root = nodes[m];

        root->left = build(nodes, l, m);
        root->right = build(nodes, m + 1, r);

        refresh(root);
        return root;
    }

    // Merges leagues even when their score ranges overlap.
    void merge_leagues(Node*& a, Node*& b) const {
        if (&a == &b)
            throw invalid_argument("Cannot merge a board with itself");

        if (!a) {
            a = b;
            b = nullptr;
            return;
        }

        if (!b) return;

        vector<Node*> x, y, all;

        inorder(a, x);
        inorder(b, y);

        vector<PlayerId> ids;
        ids.reserve(x.size() + y.size());

        for (Node* p : x)
            ids.push_back(p->player.id);

        for (Node* p : y)
            ids.push_back(p->player.id);

        sort(ids.begin(), ids.end());

        for (size_t i = 1; i < ids.size(); i++) {
            if (ids[i] == ids[i - 1])
                throw invalid_argument("Duplicate player ID across leagues");
        }

        all.reserve(x.size() + y.size());

        merge(x.begin(), x.end(),
              y.begin(), y.end(),
              back_inserter(all),
              [this](Node* u, Node* v) {
                  return before(u->player, v->player);
              });

        a = build(all, 0, all.size());
        b = nullptr;
    }

    bool audit(Node* t, const Player* lower,
               const Player* upper, int& h, Position& s) const {
        if (!t) {
            h = 0;
            s = 0;
            return true;
        }

        if (lower && !before(*lower, t->player))
            return false;

        if (upper && !before(t->player, *upper))
            return false;

        int lh = 0, rh = 0;
        Position ls = 0, rs = 0;

        if (!audit(t->left, lower, &t->player, lh, ls))
            return false;

        if (!audit(t->right, &t->player, upper, rh, rs))
            return false;

        h = 1 + max(lh, rh);
        s = 1 + ls + rs;

        return abs(lh - rh) <= 1 &&
               t->height == h &&
               t->size == s;
    }

    bool validate(Node* t) const {
        int h = 0;
        Position s = 0;
        return audit(t, nullptr, nullptr, h, s);
    }

    void destroy(Node* t) const {
        if (!t) return;

        destroy(t->left);
        destroy(t->right);
        delete t;
    }
};
