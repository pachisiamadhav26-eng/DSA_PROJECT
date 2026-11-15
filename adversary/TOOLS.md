# Tool-use disclosure

15 November 2026: OpenAI Codex inspected every tracked file at the tips of seven
GitHub branches and the local empty untracked adversary file, generated this
adversary C++ input generator, Python runner, golden fixtures, explanation and
D5 draft, and ran compilation/differential/scale checks. Git was used only to
read remote refs, clone a separate mirror, inspect objects and export snapshots.
PowerShell, Python and GCC were used for local authoring, validation and ZIP
packaging. No commits or pushes were made. Human line-by-line review and official
D5 requirement verification remain pending; no student-authored modifications
or teammate review are claimed.

Om's oracle, adapters, checker and reducer, Prince's AVL and Saksham's Fenwick
remain their existing contributions. They are pinned dependencies, not newly
authored adversary algorithms. The offline kit includes unmodified copies solely
for local testing; the contribution ZIP excludes them.
