# Tools used for the Oracle contribution

Owner: Om Dehariya. Scope: the `om-oracle` branch.

OpenAI Codex generated the sorted-array reference, test-only replay facade,
adapters for the existing standalone structures, differential runner, invariant
checkers, shrinking logic, regression traces, build/test scripts, and the D4
proof and PDF renderer. It inspected the course assignment and the existing
team branches to align ordering, competition ranks and interface semantics.

The tests run the actual AVL and Fenwick sources exported from pinned teammate
commits; Codex did not rewrite those implementations. `include/leaderboard.hpp`
and the basic replay fixture are dependencies copied from Madhav's branch.
The adapters add validation and bridge the standalone APIs as documented in
`oracle/README.md`. Subsequent implementation fixes and recorded checks are in
`oracle/VALIDATION.md` and the commit history.

Tools used: Git for branch inspection and source export; GCC/MinGW for C++17
compilation and tests; PowerShell for repeatable builds and test orchestration;
Python with ReportLab for the D4 PDF; Poppler for visual PDF verification.

No manual changes or review by Om are claimed here. Om should record any later
changes and teammate review after they actually occur, and understand the code
and proof before the viva. This disclosure covers only this contribution;
other members should append their own tool usage.

17 November 2026: OpenAI Codex updated the Oracle for Madhav's current
`Board` interface and replay rules. The revised harness links the teammate
factories and audit helpers, and the runner checks the Architect application
against golden outputs. The original standalone results are marked historical
in `oracle/VALIDATION.md`. No human review of this update is claimed.

17 November 2026: OpenAI Codex rewrote the Oracle and helper scripts for
readability, separated trace generation and backend checks, and added a code
reading guide. Tests compared the new code with the previous version. Human
review is pending.

17 November 2026: OpenAI Codex revised D4 in English against the assignment,
added rotation diagrams and a worked selection example, and regenerated the
two-page PDF with ReportLab. It also corrected stale README instructions and
fixed test-runner count validation. The full Oracle suite was rerun.
