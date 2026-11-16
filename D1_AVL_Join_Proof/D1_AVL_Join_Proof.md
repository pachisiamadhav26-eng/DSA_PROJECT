# D1 - AVL Join Correctness and Time Complexity

**Author:** Prince Chavda | Structure A (Augmented AVL Tree)

## 1. Statement and assumptions

Let T1 and T2 be valid AVL trees of heights h1 and h2. The height of an empty tree is 0 and the height of a leaf is 1. Their stored heights and subtree sizes are correct. The trees share no nodes, and k is a separate supplied pivot node. Every key in T1 precedes k, and k precedes every key in T2.

Here, "precedes" means higher score first, then smaller player ID when scores tie. The application guarantees globally unique player IDs.

We prove that joinNodes(T1, k, T2) returns an AVL tree containing exactly the nodes of T1, k and T2, with correct ordering, heights and sizes, in O(|h1 - h2| + 1) time. We also prove the height bound needed for the balance argument:

`max(h1, h2) <= height(result) <= max(h1, h2) + 1`.

## 2. Algorithm used in avl.cpp

1. If |h1 - h2| <= 1, make k the root, attach T1 on its left and T2 on its right, and refresh k.
2. If h1 > h2 + 1, recursively join T1.right, k and T2. Attach the result as T1.right, then balance and return T1.
3. Otherwise, recursively join T1, k and T2.left. Attach the result as T2.left, then balance and return T2.

All heights are read from stored metadata. Each balance operation refreshes metadata and performs at most one single rotation or one double rotation.

## 3. Why ordering and contents are preserved

In the first case, the ordering precondition lets us attach T1 and T2 directly under k.

When T1 is taller, every key in T1.right, k and T2 follows the root of T1. The recursive call therefore replaces its right subtree with exactly the required keys, still on the correct side. Its unchanged left subtree also remains correctly placed. The argument for taller T2 is symmetric.

Rotations preserve inorder order and neither add nor remove nodes. Thus the result has the required ordering and contains each input node and the pivot exactly once.

## 4. Local rebalancing fact

Suppose a valid AVL node originally has subtree height H. Replace one child by a valid AVL subtree whose height is unchanged or increases by one. After balance(), the subtree is AVL and its height is H or H + 1.

If no imbalance occurs, this follows directly from the height formula. Otherwise, consider a right-heavy imbalance; the left-heavy case is its mirror image. Let the unchanged left child have height t. The old right child must have height t + 1, and the new right child y has height t + 2. Hence the old parent height was H = t + 2.

Write b = height(y.left) and c = height(y.right). Since y is AVL, max(b, c) = t + 1 and |b - c| <= 1.

- If c >= b, then c = t + 1 and b is t or t + 1. One left rotation makes the old parent have children of heights t and b. Its height becomes t + 1 or t + 2. The new root y is balanced and has height t + 2 or t + 3, which is H or H + 1.
- If b > c, then b = t + 1 and c = t. Let z = y.left. The children of z have heights at most t and at least t - 1, with at least one of height t (empty children have height 0). A right rotation at y followed by a left rotation at the old parent makes both children of the new root z have height t + 1. Every affected node is balanced, and the resulting height is t + 2 = H.

This proves the local fact, including the height bound, rather than merely assuming it.

## 5. AVL balance and result height

We prove balance and the stated result-height bound together by induction on d = |h1 - h2|.

**Base case, d <= 1:** The new root k has two AVL children whose heights differ by at most one. It is therefore balanced, and its height is max(h1, h2) + 1. The height bound holds.

**Inductive step, h1 > h2 + 1:** Let r be the original height of T1.right. Because T1 is AVL, h1 - 2 <= r <= h1 - 1. Also h1 >= h2 + 2, so r >= h2. Therefore the recursive height difference r - h2 is smaller than d.

By induction, the recursive result is AVL and has height r or r + 1. Thus T1.right is replaced by a subtree whose height increases by at most one. Applying the local rebalancing fact restores AVL balance and gives height h1 or h1 + 1. This is exactly the required bound, since max(h1, h2) = h1.

The case h2 > h1 + 1 is symmetric: recurse into T2.left and apply the same fact. Consequently the whole result is AVL, and the height bound holds in every case.

## 6. Correct stored heights and subtree sizes

For every node v, refresh(v) computes:

`height(v) = 1 + max(height(v.left), height(v.right))`

`size(v) = 1 + size(v.left) + size(v.right)`.

In the base case, the children already have correct metadata, so refreshing k gives correct metadata. On returning from recursion, the changed child is correct by induction and the other child is unchanged. Refreshing the parent therefore gives correct values.

During a rotation, the demoted node is refreshed before its new parent. Thus each calculation uses already-correct child metadata. A double rotation applies this process twice. All untouched nodes retain their correct values. In particular:

`size(result) = size(T1 before join) + 1 + size(T2 before join)`.

## 7. Running time and scope

If d <= 1, the work is constant. Otherwise, each recursive call descends one edge of the taller tree. An AVL child has height one or two less than its parent, so the positive height difference decreases by at least one until it is at most one. The induction above also shows that the descended child's height cannot fall below the other tree's height before the base case.

There are therefore O(d + 1) calls. Each does constant work: stored-height checks, pointer changes, metadata updates and at most two rotations. Total time is O(|h1 - h2| + 1), with O(|h1 - h2| + 1) recursive stack space.

This bound assumes a supplied pivot and guaranteed ordering. The public avl_join_ordered wrapper in the current code performs no boundary searches. Searching for boundary keys would add O(h1 + h2); extracting a pivot would add up to O(log n). Merging arbitrary overlapping leagues is a separate operation and does not have this join bound.


## 8. Unequal-height example and connection to the code

Consider T1 of height 4, T2 of height 1, and a separate pivot k. Let T1 have root a, left subtree L of height 3 and right child b of height 2. Let both children B and C of b have height 1. The initial height difference is 3.

```text
T1 (height 4):                 Supplied inputs:
          a                   pivot k
         / \
     L(h=3) b                 T2 = R(h=1)
           / \
       B(h=1) C(h=1)

Recursive join at b:
             k (height 3)
            / \
      b (h=2) R(h=1)
       / \
   B(h=1) C(h=1)

Final result (height 4):
          a
         / \
     L(h=3) k(h=3)
           / \
       b(h=2) R(h=1)
        / \
    B(h=1) C(h=1)
```

The first call descends from a into its right subtree b. At b, the input heights 2 and 1 differ by one, so k becomes their root. That recursive result has height 3, one more than the original b subtree. Back at a, the two children now both have height 3, so no rotation is needed. The result has height 4, within the proved range [4, 5]. Examples requiring rotations are covered by the local rebalancing cases in section 4.

In avl.cpp, joinNodes implements the descent and base attachment; balance chooses rotations; rotateLeft and rotateRight preserve inorder order; refresh updates both height and size. The Work counter increments once per joinNodes call. In this example the join visits two nodes/call levels and performs zero rotations. avl_join_ordered creates the separate pivot node, calls joinNodes, and clears both source roots after success. The supplied-pivot core has the proved height-gap bound; allocating the wrapper result and pivot is separate allocation work.
