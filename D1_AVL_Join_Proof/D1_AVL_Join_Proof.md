# D1 - AVL Join Correctness and Time Complexity

**Module owner:** Structure A (Augmented AVL Tree)

Let $T_1$ and $T_2$ be valid AVL trees with heights $h_1$ and $h_2$, and let $k$ be a supplied pivot node. Every key in $T_1$ must precede $k$, and $k$ must precede every key in $T_2$. In our leaderboard, higher scores come first; tied scores are ordered by increasing player ID. We prove that `joinNodes(T1, k, T2)` produces a correctly ordered, balanced AVL tree with valid subtree sizes in $O(|h_1-h_2|+1)$ time.

## Why BST ordering is preserved

If $|h_1-h_2|\leq 1$, the algorithm makes $k$ the root, attaching $T_1$ on the left and $T_2$ on the right. The given ordering condition makes this a valid search tree.

If $h_1>h_2+1$, the algorithm recursively joins the **right subtree** of $T_1$ with $k$ and $T_2$, then reattaches the result. Every key in this replacement remains after the root of $T_1$ in leaderboard order. The case where $T_2$ is taller is symmetric, using its **left subtree**. Since AVL rotations preserve inorder order, the final tree is correctly ordered and contains exactly the original nodes plus the pivot.

## Why AVL balance is preserved

We use induction on the height difference.

**Base case:** For $|h_1-h_2|\leq1$, the new root has balance factor $h_1-h_2\in\{-1,0,1\}$. Both attached trees are already AVL-balanced.

**Inductive step:** When one tree is taller, Join descends its relevant spine. By induction, the recursive result is balanced and its height is at most **one greater than the subtree it replaces**. Thus an affected ancestor's balance factor can move outside the permitted range only to $+2$ or $-2$. The `balance()` function performs the needed single or double rotation and restores AVL balance. The same argument applies on the other spine. Therefore all nodes in the joined tree remain AVL-balanced.

## Why subtree sizes remain correct

Each node stores

$$
size(v)=1+size(v.left)+size(v.right).
$$

In the base case, `refresh(k)` sets $size(k)=size(T_1)+1+size(T_2)$. On the recursive return path, `refresh()` recomputes the affected nodes from their updated children. For a rotation, the lower node is refreshed before its new parent, preserving the formula. Double rotations are compositions of single rotations. Hence

$$
\boxed{size(result)=size(T_1)+1+size(T_2)}.
$$

## Why running time is $O(|h_1-h_2|+1)$

Let $d=|h_1-h_2|$. If $d\leq1$, Join attaches the pivot and updates its metadata in $O(1)$ time.

Otherwise, each recursive call moves one level down the taller tree's right or left spine. The current height drops by at least one, until its height is within one of the smaller tree's height. Each visited level performs $O(1)$ work: a comparison, a pointer update, `refresh()`, and at most a constant number of rotations. Therefore the number of visited levels is $O(d+1)$ and

$$
\boxed{T_{join}=O(|h_1-h_2|+1)}.
$$

## Scope of this bound

This bound applies to the **core ordered Join with a supplied pivot and already guaranteed ordering**. A wrapper that searches for boundary keys to validate the precondition incurs additional $O(h_1+h_2)$ time. Extracting a pivot from one input tree also incurs up to $O(\log n)$ time. Merging two arbitrary leagues whose scores overlap is a different operation and does **not** satisfy the height-difference Join bound.
