#include "backend_tools.hpp"

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <unordered_set>

namespace leaderboard {
namespace {

bool before(Player left, Player right) {
    if (left.score != right.score) return left.score > right.score;
    return left.id < right.id;
}

struct Node;
using Tree = std::unique_ptr<Node>;

struct Node {
    explicit Node(Player value) : player(value) {}
    Player player;
    Tree left;
    Tree right;
    int height = 1;
    Position size = 1;
};

int height(const Tree& tree) { return tree ? tree->height : 0; }
Position count(const Tree& tree) { return tree ? tree->size : 0; }

void refresh(Node& node) {
    node.height = 1 + std::max(height(node.left), height(node.right));
    node.size = 1 + count(node.left) + count(node.right);
}

void rotate_left(Tree& root, std::uint64_t& rotations) {
    auto pivot = std::move(root->right);
    root->right = std::move(pivot->left);
    refresh(*root);
    pivot->left = std::move(root);
    refresh(*pivot);
    root = std::move(pivot);
    ++rotations;
}

void rotate_right(Tree& root, std::uint64_t& rotations) {
    auto pivot = std::move(root->left);
    root->left = std::move(pivot->right);
    refresh(*root);
    pivot->right = std::move(root);
    refresh(*pivot);
    root = std::move(pivot);
    ++rotations;
}

void balance(Tree& root, std::uint64_t& rotations) {
    refresh(*root);
    int difference = height(root->left) - height(root->right);
    if (difference > 1) {
        if (height(root->left->right) > height(root->left->left))
            rotate_left(root->left, rotations);
        rotate_right(root, rotations);
    } else if (difference < -1) {
        if (height(root->right->left) > height(root->right->right))
            rotate_right(root->right, rotations);
        rotate_left(root, rotations);
    }
}

void insert_key(Tree& root, Player player, std::uint64_t& rotations) {
    if (!root) {
        root = std::make_unique<Node>(player);
        return;
    }
    if (before(player, root->player)) insert_key(root->left, player, rotations);
    else if (before(root->player, player)) insert_key(root->right, player, rotations);
    else throw std::invalid_argument("duplicate AVL key");
    balance(root, rotations);
}

Tree take_last(Tree& root, std::uint64_t& rotations) {
    if (!root->right) {
        auto last = std::move(root);
        root = std::move(last->left);
        return last;
    }
    auto last = take_last(root->right, rotations);
    balance(root, rotations);
    return last;
}

void erase_key(Tree& root, Player player, std::uint64_t& rotations) {
    if (!root) throw std::invalid_argument("missing AVL key");
    if (before(player, root->player)) erase_key(root->left, player, rotations);
    else if (before(root->player, player)) erase_key(root->right, player, rotations);
    else if (!root->left || !root->right) {
        auto removed = std::move(root);
        root = removed->left ? std::move(removed->left) : std::move(removed->right);
        return;
    } else {
        root->player = take_last(root->left, rotations)->player;
    }
    balance(root, rotations);
}

Tree join(Tree left, Tree pivot, Tree right, std::uint64_t& rotations,
          std::uint64_t& visits) {
    ++visits;
    if (height(left) > height(right) + 1) {
        left->right = join(std::move(left->right), std::move(pivot), std::move(right),
                           rotations, visits);
        balance(left, rotations);
        return left;
    }
    if (height(right) > height(left) + 1) {
        right->left = join(std::move(left), std::move(pivot), std::move(right->left),
                           rotations, visits);
        balance(right, rotations);
        return right;
    }
    pivot->left = std::move(left);
    pivot->right = std::move(right);
    refresh(*pivot);
    return pivot;
}

std::pair<Tree, Tree> split(Tree root, Player key, std::uint64_t& rotations,
                            std::uint64_t& visits) {
    if (!root) return {};
    auto left = std::move(root->left);
    auto right = std::move(root->right);
    if (before(root->player, key)) {
        auto parts = split(std::move(right), key, rotations, visits);
        parts.first = join(std::move(left), std::move(root), std::move(parts.first),
                           rotations, visits);
        return parts;
    }
    auto parts = split(std::move(left), key, rotations, visits);
    parts.second = join(std::move(parts.second), std::move(root), std::move(right),
                        rotations, visits);
    return parts;
}

void page(const Node* node, Position first, Position last, std::vector<Player>& result) {
    if (!node) return;
    Position pivot = count(node->left) + 1;
    if (first < pivot) page(node->left.get(), first, std::min(last, pivot - 1), result);
    if (first <= pivot && pivot <= last) result.push_back(node->player);
    if (last > pivot)
        page(node->right.get(), first > pivot ? first - pivot : 1, last - pivot, result);
}

Tree build_balanced(const std::vector<Player>& players, std::size_t first, std::size_t end) {
    if (first == end) return {};
    std::size_t middle = first + (end - first) / 2;
    auto root = std::make_unique<Node>(players[middle]);
    root->left = build_balanced(players, first, middle);
    root->right = build_balanced(players, middle + 1, end);
    refresh(*root);
    return root;
}

class AvlBoard final : public Board {
public:
    Tree root;
    std::uint64_t rotations = 0;
    std::uint64_t join_visits = 0;

    Position size() const override { return count(root); }

    bool contains(Player p) const override {
        const Node* node = root.get();
        while (node) {
            if (before(p, node->player)) node = node->left.get();
            else if (before(node->player, p)) node = node->right.get();
            else return true;
        }
        return false;
    }

    void insert(Player p) override { insert_key(root, p, rotations); }
    void erase(Player p) override { erase_key(root, p, rotations); }

    Position count_strictly_higher(Score score) const override {
        Position higher = 0;
        for (const Node* node = root.get(); node;) {
            if (node->player.score > score) {
                higher += count(node->left) + 1;
                node = node->right.get();
            } else {
                node = node->left.get();
            }
        }
        return higher;
    }

    Player select(Position k) const override {
        if (k == 0 || k > size()) throw std::out_of_range("AVL position");
        const Node* node = root.get();
        while (node) {
            Position pivot = count(node->left) + 1;
            if (k == pivot) return node->player;
            if (k < pivot) node = node->left.get();
            else {
                k -= pivot;
                node = node->right.get();
            }
        }
        throw std::logic_error("AVL subtree size");
    }

    void range(Position first, Position last, std::vector<Player>& result) const override {
        if (first == 0 || first > last || last > size()) throw std::out_of_range("AVL range");
        result.clear();
        page(root.get(), first, last, result);
    }

    std::pair<std::unique_ptr<Board>, std::unique_ptr<Board>>
    split_by_score(Score threshold) override {
        // The last ID at this score follows every other tied player. Shift the
        // cutoff to the next lower score, whose first ID is zero.
        if (threshold == std::numeric_limits<Score>::min()) {
            auto high = std::make_unique<AvlBoard>();
            auto low = std::make_unique<AvlBoard>();
            high->root = std::move(root);
            join_visits = 0;
            return {std::move(high), std::move(low)};
        }
        return avl_split_by_key(*this, {0, threshold - 1});
    }

    void merge_from(Board& other) override {
        auto* donor = dynamic_cast<AvlBoard*>(&other);
        if (!donor || donor == this) throw std::invalid_argument("AVL merge backend or self");
        join_visits = 0;
        if (!donor->root) return;
        if (!root) {
            root = std::move(donor->root);
            return;
        }
        if (before(select(size()), donor->select(1))) {
            auto pivot = take_last(root, rotations);
            root = join(std::move(root), std::move(pivot), std::move(donor->root),
                        rotations, join_visits);
        } else if (before(donor->select(donor->size()), select(1))) {
            auto pivot = take_last(donor->root, rotations);
            root = join(std::move(donor->root), std::move(pivot), std::move(root),
                        rotations, join_visits);
        } else {
            std::vector<Player> left, right, combined;
            range(1, size(), left);
            donor->range(1, donor->size(), right);
            combined.reserve(left.size() + right.size());
            std::merge(left.begin(), left.end(), right.begin(), right.end(),
                       std::back_inserter(combined), before);
            root = build_balanced(combined, 0, combined.size());
            donor->root.reset();
        }
    }
};

std::pair<int, Position> audit_node(const Node* node, const Player* lower, const Player* upper,
                                   std::unordered_set<PlayerId>& ids, std::vector<Player>& out) {
    if (!node) return {0, 0};
    const Player& player = node->player;
    if ((lower && !before(*lower, player)) || (upper && !before(player, *upper)) ||
        !ids.insert(player.id).second) throw std::logic_error("AVL key order or ID uniqueness");
    auto left = audit_node(node->left.get(), lower, &player, ids, out);
    out.push_back(player);
    auto right = audit_node(node->right.get(), &player, upper, ids, out);
    int expected_height = 1 + std::max(left.first, right.first);
    Position expected_size = 1 + left.second + right.second;
    if (node->size != expected_size) throw std::logic_error("AVL subtree size");
    if (node->height != expected_height) throw std::logic_error("AVL stored height");
    if (std::abs(left.first - right.first) > 1) throw std::logic_error("AVL balance");
    return {expected_height, expected_size};
}

}

std::unique_ptr<Board> make_avl() { return std::make_unique<AvlBoard>(); }

std::vector<Player> audit_avl(const Board& board) {
    const auto& avl = dynamic_cast<const AvlBoard&>(board);
    std::vector<Player> result;
    std::unordered_set<PlayerId> ids;
    audit_node(avl.root.get(), nullptr, nullptr, ids, result);
    return result;
}

AvlStats avl_stats(const Board& board) {
    const auto& avl = dynamic_cast<const AvlBoard&>(board);
    return {height(avl.root), avl.rotations, avl.join_visits};
}

std::pair<std::unique_ptr<Board>, std::unique_ptr<Board>>
avl_split_by_key(Board& source, Player key) {
    auto& avl = dynamic_cast<AvlBoard&>(source);
    auto first = std::make_unique<AvlBoard>();
    auto second = std::make_unique<AvlBoard>();
    avl.join_visits = 0;
    auto parts = split(std::move(avl.root), key, avl.rotations, avl.join_visits);
    first->root = std::move(parts.first);
    second->root = std::move(parts.second);
    return {std::move(first), std::move(second)};
}

std::unique_ptr<Board> avl_join_ordered(Board& left, Player pivot, Board& right) {
    auto& a = dynamic_cast<AvlBoard&>(left);
    auto& b = dynamic_cast<AvlBoard&>(right);
    if (&a == &b) throw std::invalid_argument("AVL ordered join with self");
    auto result = std::make_unique<AvlBoard>();
    auto separator = std::make_unique<Node>(pivot);
    result->root = join(std::move(a.root), std::move(separator), std::move(b.root),
                        result->rotations, result->join_visits);
    return result;
}

}
