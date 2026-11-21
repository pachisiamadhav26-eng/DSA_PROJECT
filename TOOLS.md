# Tool-use record

The team should add a dated entry for each tool used on submitted work. Entries must state what the tool produced and what a team member changed or verified afterward. This file records the work currently known on the `madhav` branch; other members must add their own entries.

| Date | Tool | Parts affected | What it produced or assisted | Human changes and verification |
| --- | --- | --- | --- | --- |
| 6 Nov 2026 | OpenAI Codex | Initial `README.md` and `.gitignore` | Drafted the initial repository description and ignore rules. | The README was subsequently edited in team commits. The project owner reviewed the repository setup. |
| 10-11 Nov 2026 | OpenAI Codex | `INTERFACE.md`, `include/leaderboard.hpp`, `app/examples/` | Drafted the shared behavior contract, C++ declarations, and sample replay/output. | The project owner chose C++, approved the proposed rank and tie rules, and requested removal of code comments. The header compiled when included by later application tests. |
| 12-13 Nov 2026 | OpenAI Codex | `app/player_table.hpp`, `app/player_table.cpp`, `app/tests/player_table_test.cpp` | Generated the handwritten open-addressing player table and differential test. | The project owner authorized the commit. Growth, deletion, and 200,000 seeded operations were tested against a test-only standard-library map. Further line-by-line human review is pending. |
| 13 Nov 2026 | OpenAI Codex | `app/replay.hpp`, `app/replay.cpp`, `app/leaderboard.cpp`, `app/tests/replay_test.cpp`, `docs/derivations/D3.md` | Drafted the replay application, test backend, and Architect proof. | Human review is pending. The sample and error tests passed with a test backend; the real AVL and Fenwick adapters have not been linked. |
| 13 Nov 2026 | OpenAI Codex, ReportLab, and Poppler | `docs/derivations/D3.pdf` | Generated a temporary Python renderer, rendered the proof, inspected its page image, and removed the renderer from the repository. | The proof content and layout require the author's final review before submission. |
| 13 Nov 2026 | MinGW GCC 6.3 | `app/` tests and entry point | Compiled C++17 code with `-Wall -Wextra -Werror`; test executables were run. | The player-table and replay tests passed. The application entry point compiled as an object; real backend linking remains. |
| 16-17 Nov 2026 | OpenAI Codex | `INTERFACE.md`, `app/leaderboard.cpp`, `app/replay.cpp`, `app/player_table.hpp`, `app/player_table.cpp`, `app/tests/` | Reworded the interface, used clearer command handling and hash-table names, and made the entry point and tests easier to read. | Prepared for Madhav's review before commit. Both application tests pass; temporary million-command replay and million-ID hash-table checks passed. A temporary combined build passed basic, season, and wide-ID traces with both backends; the oracle error fixture still differs, and a full-scale combined run remains. |
| 21 Nov 2026 | OpenAI Codex and PyMuPDF | `docs/derivations/D3.md`, `docs/derivations/D3_250041025.pdf`, `app/web/`, `app/tests/pipeline*.trace` | Aligned D3 with the exact assigned statement and the supplied roll number, rendered the final PDF, and corrected the web demo launcher. | The one-page PDF was visually checked. The player-table and replay tests passed; both structures matched the two new trace fixtures, and the local web page and replay API responded correctly. |

## Om Dehariya - historical Oracle contribution record

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

## Om's representative AI prompts

The following are excerpts from the requests in the Codex conversation. Earlier
entries above remain historical records. Codex generated the changes and ran
the checks; teammate approval is recorded separately on GitHub.

- **17 November 2026 - readability.** "use humanize karke simplest code bana yr jo saare kaam karde jo required hai". Asked Codex to simplify the Oracle while preserving required behavior. Affected `oracle/` code and scripts; comparisons with the previous implementation and the Oracle suites are recorded in `oracle/VALIDATION.md`.
- **21 November 2026 - PR corrections.** "please ek baar tu ye message dekh aur apni om-oracle branch ke issues ko acche se resolve karde". The attached review requested the roll-number PDF, exact D4 statement, sources, file/function references, dated prompts, and final-checkout tests. Codex updated the proof and its renderer, merged current main into this branch, resolved `.gitattributes` and `TOOLS.md` conflicts while retaining both contributions, and changed the build to compile checkout files.
- **21 November 2026 - author metadata.** "bhai mera full name Om Dehariya hai aur roll no 250041029 hai aur branch Maths and Computing hai". Codex used the supplied identity in the D4 source, PDF metadata, footer and submission filename.

The proof text and layout were assisted by OpenAI Codex. Sources actually
consulted were the two course handouts and the local code paths cited in D4.
No external textbook consultation or human approval is claimed. ReportLab
created the PDF; pypdf checked its pages and text; Poppler rendered it for
visual inspection. Current-checkout test results are recorded in VALIDATION.md.
## Analyst - Sumit (250041038)

Sumit supplied the intended workflow, requirements and requested changes. These entries describe assistance with implementation and verification; the original team entries above are retained.

| Date | Tool | Parts affected | What it produced or assisted | Human changes and verification |
| --- | --- | --- | --- | --- |
| 10 Nov 2026 | OpenAI Codex and MinGW g++ | Early 100-player rank practice benchmark | Helped draft the requested benchmark, sample queries and expected-rank explanations. | Sumit selected the practice scope and requested explanations. Small expected-answer checks passed. Commit `cfd8ccf` records the practice work, which was later removed. |
| 16 Nov 2026 | OpenAI Codex and MinGW g++ | Early select/split practice benchmarks | Helped extend the practice code and save timings for the requested operations. | Sumit requested the additional operations. Commit `d2893a1` records this work; its practice measurements are separate from the formal suite. |
| 17 Nov 2026 | OpenAI Codex, Python and MinGW g++ | `bench/runner.cpp`, `bench/support.hpp`, build/run scripts, checks and `docs/derivations/D6.md` | Helped draft and revise the shared-interface harness, operation counters, trace handling and symbolic cost-model explanations under Sumit's directions. | Sumit selected the workflow and requested simplification. The actual component builds and small harness checks passed. The D6 commit `7b78276` precedes the formal run. Final human code/proof review remains required. |
| 17 Nov 2026 | OpenAI Codex, Python and Matplotlib | Formal raw CSVs, summary statistics, figures and `REPORT.md` | Helped check supplied components, execute the full suite, process results and correct trace grouping. Representative prompt: "see bianca has updated her work check if it is compatible with everything if yes then run my full pipeline and get all the results etc" | Sumit requested the experiments and clearer results tables. Automated checks covered 158 raw CSVs, 436 five-run groups and 29 chart pairs. Small Oracle checks and the independent scale-output digest passed; the grouping correction has a regression check. |
| 17 Nov 2026 | Portable Zig 0.13.0 / Clang | Supplied adversarial input generator | Compiled Bianca's unchanged generator after the installed GCC could not handle its C++17 syntax. | Generated small traces passed real-oracle checks. Both measured structures continued to use the same GCC benchmark build. Compiler commands and source hashes were recorded. |
| 17 Nov 2026 | OpenAI Codex, ReportLab and Poppler | D6 PDF and documentation cleanup | Helped render and inspect D6 and remove practice files and redundant notes as requested. | Sumit requested the cleanup. The PDF layout was visually inspected. The earlier three-page version was subsequently shortened following review feedback. |
| 17 Nov 2026 | OpenAI Codex, Windows Performance Recorder and tracerpt | `bench/profile_windows.py`, `bench/analyze_profiles.py` and profiling outputs | Helped set up hardware recordings, draft sample filtering and explain cache evidence. | Sumit approved Administrator recording. All four captures reported zero lost events. Samples were filtered through benchmark thread IDs; exact cache miss rates and a causal explanation of speed differences are not claimed. |
| 21 Nov 2026 | OpenAI Codex, Python, ReportLab, Poppler and Matplotlib | `D6_250041038.pdf`, figure-source captions, final results paths and this tool record | Helped apply PR feedback, condense D6, add identification/code references and publish verified result copies. Representative prompt: "okay you do the changes and ask me whatever info you need" | Sumit supplied his name and roll number, then requested the main-branch tool-record format while preserving team entries. D6 is two pages; exported files match the source run by SHA-256. Original measurements and pre-run model history are retained. No numerical coefficient calibration is claimed. |
