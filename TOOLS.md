# Tool-use record

The team should add a dated entry for each tool used on submitted work. Entries must state what the tool produced and what a team member changed or verified afterward. This file records the work currently known on the `madhav` branch; other members must add their own entries.

| Date | Tool | Parts affected | What it produced or assisted | Human changes and verification |
| --- | --- | --- | --- | --- |
| 6 Nov 2026 | OpenAI Codex | Initial `README.md` and `.gitignore` | Drafted the initial repository description and ignore rules. | The README was subsequently edited in team commits. The project owner reviewed the repository setup. |
| 10-11 Nov 2026 | OpenAI Codex | `INTERFACE.md`, `include/leaderboard.hpp`, `app/examples/` | Drafted the shared behavior contract, C++ declarations, and sample replay/output. | The project owner chose C++, approved the proposed rank and tie rules, and requested removal of code comments. The header compiled when included by later application tests. |
| 12-13 Nov 2026 | OpenAI Codex | `app/player_table.hpp`, `app/player_table.cpp`, `app/tests/player_table_test.cpp` | Generated the handwritten open-addressing player table and differential test. | The project owner authorized the commit. Growth, deletion, and 200,000 seeded operations were tested against a test-only standard-library map. Further line-by-line human review is pending. |
| 13 Nov 2026 | OpenAI Codex | `app/replay.hpp`, `app/replay.cpp`, `app/leaderboard.cpp`, `app/tests/replay_test.cpp`, `docs/derivations/D3.md` | Drafted the replay application, test backend, and Architect proof. | Human review is pending. The sample and error tests passed with a test backend; the real AVL and Fenwick adapters have not been linked. |
| 13 Nov 2026 | OpenAI Codex, ReportLab, and Poppler | `docs/derivations/render_d3.py`, `docs/derivations/D3.pdf` | Generated the PDF renderer, rendered the proof, and inspected its page image. | The proof content and layout require the author's final review before submission. |
| 13 Nov 2026 | MinGW GCC 6.3 | `app/` tests and entry point | Compiled C++17 code with `-Wall -Wextra -Werror`; test executables were run. | The player-table and replay tests passed. The application entry point compiled as an object; real backend linking remains. |

## Oracle contribution

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
## Production integration

OpenAI Codex completed the shared replay rules, integrated the existing AVL
and Fenwick algorithms with the application, migrated oracle and adversary
tests to the real backends, and built the full sweep benchmark. It drafted
D1, D2, D5 and D6 and updated D4's test explanation. The named module owners
and original commits identify team contributions. The integration commits
use the actual configured Git identity. Human review and viva preparation
should be recorded by each member after they occur.

MinGW GCC and Python ran production validation and generated trial logs and
figures. ReportLab rendered derivations, and Poppler rendered the pages for
visual inspection. Git preserved the branch histories; earlier adversary
tool-use records are in [adversary/TOOLS.md](adversary/TOOLS.md).
