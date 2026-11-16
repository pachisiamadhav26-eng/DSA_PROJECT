# D4 Subtree sizes through rotations and logarithmic select

Author: Om Dehariya (Oracle owner)
Project 2: Rank queries on a live leaderboard
CS 253 / MA 253

## Definitions and assumptions

Keys are ordered by decreasing score, then increasing player ID. Define
size(null) = 0 and size(v) = 1 + size(v.left) + size(v.right). A leaf has height
1 and the empty tree has height 0. Positions are one-based. Select returns the
player at a position; it does not return a competition rank. Tied scores share
a rank but have different positions.

Assume an AVL subtree is valid before a local insertion, deletion or join step.
Its unchanged child subtrees already have correct sizes. After changing child
pointers, refresh the affected nodes from children towards the new subtree root.
We prove that this preserves the size invariant.

## Single rotation

For a left rotation, the old root x has left subtree A and right child y;
y has subtrees B and C. The result has root y, left child x with subtrees A
and B, and right subtree C. Write a, b and c for the sizes of A, B and C.

    Before: x(A, y(B, C))       After: y(x(A, B), C)
    new size(x) = 1 + a + b
    new size(y) = 1 + new size(x) + c = 2 + a + b + c

Only x and y change children. All other node sizes remain valid. Refreshing x
before y therefore computes both sizes correctly. The old subtree also had
2 + a + b + c nodes, so its parent sees the same total. A right rotation is
the mirror image: in y(x(A, B), C) -> x(A, y(B, C)), refresh y before x,
giving size(y) = 1 + b + c and size(x) = 2 + a + b + c.

## Double rotation

A left-right rotation starts at x whose left child is y and whose right
subtree is D; y has left subtree A and right child z with subtrees B and C.

    Before: x(y(A, z(B, C)), D)
    After:  z(y(A, B), x(C, D))
    new size(y) = 1 + a + b
    new size(x) = 1 + c + d
    new size(z) = 1 + new size(y) + new size(x) = 3 + a + b + c + d

Refresh y and x before z. Equivalently, perform a left rotation at y followed
by a right rotation at x; the single-rotation proof applies at each step.
The right-left case is symmetric: x(A, y(z(B, C), D)) becomes
z(x(A, B), y(C, D)), with sizes 1+a+b, 1+c+d, and 3+a+b+c+d.
Thus every single or double rotation preserves both the represented keys and
the size invariant, provided metadata is refreshed in dependency order.

## Induction over updates

An empty tree satisfies the invariant, and a newly allocated leaf has size 1.
On the return path from insertion or deletion, each node's children have
already been repaired. Recomputing 1 + left size + right size gives its correct
size; any subsequent rotation is correct by the preceding lemmas. The argument
applies at every level, so all ancestors are correct. A join implementation
must likewise refresh every node whose children change, including a newly
created pivot. The rotation proof alone does not establish join's time bound.

## Correctness and cost of select

For a nonempty current subtree with root v, let L = size(v.left). Its ordered
sequence consists of L left-subtree keys, then v, then the right-subtree keys.
Maintain this loop invariant: the requested player is the k-th key in the
current subtree, with 1 <= k <= size(current).

1. If k = L + 1, return v.
2. If k <= L, descend left and retain k.
3. Otherwise descend right and replace k by k - L - 1.

The sequence decomposition proves each case, including the new valid bounds
on k. Each descent reduces height, so the algorithm terminates at the desired
key. Reject k = 0 or k > size(root) before descending. Correct subtree sizes
are essential: a stale size can select the wrong branch even in a balanced BST.

Each step takes O(1) time and descends one edge, hence total O(h). For AVL height
h, the minimum node count satisfies N(0)=0, N(1)=1 and
N(h)=1+N(h-1)+N(h-2). In particular N(h)+1 >= 2*(N(h-2)+1), so N(h) grows at
least exponentially in h/2, and h = O(log(n+1)). Thus select is O(log n) on a
nonempty AVL tree and uses O(1) auxiliary space in the iterative implementation.
Without AVL balance the bound is only O(h), potentially O(n).

## Connection to the oracle tests

The sorted array supplies the expected player at index k-1. It compares fields
directly, avoiding overflow from score subtraction. The AVL checker traverses
the whole topology, recomputes sizes and heights from children, verifies global
BST bounds, balance, and unique IDs, and rejects cycles/shared child pointers.
It does not trust stored sizes while computing expected sizes.

The bounded differential replay audits every command, including rotations,
deletions, splits and joins. Separate tests corrupt size/height/order fields to
verify that the checker rejects them. The million-player test audits at phase
boundaries, since a full audit after every insertion would require quadratic
work. An audit costs O(n) expected time and O(n) extra space with visited-ID
and pointer sets; it is correctness work, excluded from performance benchmarks.

## Viva explanation in Hinglish

Size ka matlab us node ke poore subtree mein kitne players hain. Rotation mein
players change nahi hote, sirf links change hote hain. Isliye pehle neeche wale
node ka size update karte hain, phir naye root ka. Select(k) mein left subtree
mein L players hain: k=L+1 toh root, k<=L toh left, warna right mein k-L-1.
AVL ki height logarithmic hai, isliye selection bhi logarithmic hai. Oracle
simple sorted vector hai, isliye uska result optimized tree se independently
compare kar sakte hain.
