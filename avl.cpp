#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <vector>
using namespace std;

struct Player {
    int id;
    long long score;
};

struct Node {
    Player player;
    Node* left = nullptr;
    Node* right = nullptr;
    int height = 1;
    int size = 1;
};

struct AVL {
    int height(Node* t) { return t ? t->height : 0; }
    int size(Node* t)   { return t ? t->size : 0; }

    bool before(Player a, Player b) {
        if (a.score != b.score) return a.score > b.score;
        return a.id < b.id;
    }

    void refresh(Node* t) {
        t->height = 1 + max(height(t->left), height(t->right));
        t->size = 1 + size(t->left) + size(t->right);
    }

    Node* rotateRight(Node* top) {
        Node* pivot = top->left;
        top->left = pivot->right;
        pivot->right = top;
        refresh(top);
        refresh(pivot);
        return pivot;
    }

    Node* rotateLeft(Node* top) {
        Node* pivot = top->right;
        top->right = pivot->left;
        pivot->left = top;
        refresh(top);
        refresh(pivot);
        return pivot;
    }

    Node* balance(Node* t) {
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

    Node* insert(Node* t, Player p) {
        if (!t) return new Node{p};
        if (before(p, t->player)) t->left = insert(t->left, p);
        else if (before(t->player, p)) t->right = insert(t->right, p);
        return balance(t);
    }

    Node* erase(Node* t, Player p) {
        if (!t) return nullptr;
        if (before(p, t->player)) t->left = erase(t->left, p);
        else if (before(t->player, p)) t->right = erase(t->right, p);
        else {
            if (!t->left || !t->right) {
                Node* child = t->left ? t->left : t->right;
                delete t;
                return child;
            }
            Node* next = t->right;
            while (next->left) next = next->left;
            t->player = next->player;
            t->right = erase(t->right, t->player);
        }
        return balance(t);
    }

    int rank(Node* t, long long score) {
        int higher = 0;
        while (t) {
            if (t->player.score > score) {
                higher += size(t->left) + 1;
                t = t->right;
            } else t = t->left;
        }
        return higher + 1;
    }

    Player select(Node* t, int k) {
        if (k < 1 || k > size(t)) throw out_of_range("invalid position");
        while (t) {
            int position = size(t->left) + 1;
            if (k == position) return t->player;
            if (k < position) t = t->left;
            else { k -= position; t = t->right; }
        }
        throw logic_error("incorrect subtree size");
    }

    void range(Node* t, int first, int last, vector<Player>& result) {
        if (!t || first > last) return;
        int position = size(t->left) + 1;
        if (first < position) range(t->left, first, last, result);
        if (first <= position && position <= last) result.push_back(t->player);
        if (position < last) range(t->right, first - position, last - position, result);
    }

    Node* join(Node* left, Player p, Node* right) {
        if (height(left) > height(right) + 1) {
            left->right = join(left->right, p, right);
            return balance(left);
        }
        if (height(right) > height(left) + 1) {
            right->left = join(left, p, right->left);
            return balance(right);
        }
        Node* root = new Node{p, left, right};
        refresh(root);
        return root;
    }

    void split(Node* t, long long cutoff, Node*& high, Node*& low) {
        if (!t) { high = nullptr; low = nullptr; return; }
        if (t->player.score >= cutoff) {
            Node* middle;
            split(t->right, cutoff, middle, low);
            high = join(t->left, t->player, middle);
        } else {
            Node* middle;
            split(t->left, cutoff, high, middle);
            low = join(middle, t->player, t->right);
        }
        delete t;
    }

    void destroy(Node* t) {
        if (!t) return;
        destroy(t->left);
        destroy(t->right);
        delete t;
    }
};

unordered_map<int, long long> currentScore;
int main() {
    AVL tree;
    Node* root = nullptr;
    vector<Player> players = {{1, 100}, {2, 90}, {3, 90}, {4, 70}, {5, 40}};
    for (Player p : players) {
        currentScore[p.id] = p.score;
        root = tree.insert(root, p);
    }

    cout << "Rank of player 3: " << tree.rank(root, currentScore.at(3)) << '\n';
    cout << "Player at position 3: " << tree.select(root, 3).id << '\n';
    vector<Player> page;
    tree.range(root, 2, 4, page);
    cout << "Players at positions 2 to 4:";
    for (Player p : page) cout << ' ' << p.id;
    cout << '\n';

    root = tree.erase(root, {4, currentScore.at(4)});
    currentScore[4] = 100;
    root = tree.insert(root, {4, 100});
    root = tree.erase(root, {5, currentScore.at(5)});
    currentScore.erase(5);

    Node* high;
    Node* low;
    tree.split(root, 100, high, low);
    root = nullptr;
    cout << "At or above 100: " << tree.size(high) << '\n';
    cout << "Below 100: " << tree.size(low) << '\n';

    Player pivot = tree.select(high, tree.size(high));
    high = tree.erase(high, pivot);
    root = tree.join(high, pivot, low);
    cout << "Players after join: " << tree.size(root) << '\n';
    tree.destroy(root);
    return 0;
}
