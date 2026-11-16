# Integration requirements for Analyst experiments

Based on branches fetched on 17 November 2026. Builds record the resolved commits so later owner fixes can be used.

| Owner | Required handoff |
| --- | --- |
| Architect | Stable Board, factories, replay sources and player table. Verified `a4dc438`. |
| AVL | Verified `03f3f57`: correct namespace/factory, shared Board, ordered join, audits and statistics. Passed integrated quick suite and targeted mixed-operation/height-gap checks. |
| Fenwick | Verified `0d5ad68`: shared Board, audits and statistics. Passed focused tests and the integrated quick suite. Public merge now sorts IDs, which must be reflected in D6. |
| Oracle | Verified `6a25781`: `contains` and latest shared header match; production-backend differential runner is available. Integrated benchmark quick suite and Om's quick correctness suite passed against the latest Architect/AVL/Fenwick commits. His build supports overriding its older default Fenwick pin with `-FenwickRef origin/Saksham`. |
| Adversary | Replay files/generators for rotations and height gaps, with generation commands/settings/seeds. Existing generators are reusable; their validations refer to the sources actually tested. |

The existing auxiliary contract `include/backend_tools.hpp` supplements the shared Board. Full suite needs these owner implementations:

```cpp
std::vector<Player> audit_avl(const Board&);
AvlStats avl_stats(const Board&); // height, rotations, last_join_visits
std::unique_ptr<Board> avl_join_ordered(Board& left, Player pivot, Board& right);
std::vector<Player> audit_fenwick(const Board&);
FenwickStats fenwick_stats(const Board&); // coordinates, bucket_shifts
```

Ordered join consumes both sources and returns the result, with ordering supplied as a precondition. State whether the implementation performs boundary searches. The runner measures the public helper and records heights, output checks and visit totals.

Teammate code is exported unchanged. Use `--avl-source` / `--fenwick-source` for a renamed source; add translation units to `build.py` if an implementation spans multiple source files. Local builds after integration use `--local`. No teammate interfaces are modified by these scripts.

The initial AVL/oracle interface blockers are resolved in these versions. D6 was revised on 17 November for the linear AVL overlap merge under unique IDs and the Fenwick public merge's ID-sorting cost. Review it again if the code versions change before formal experiments.
