# Your contribution and submission

The project studies live leaderboard operations using an augmented AVL tree
and a Fenwick count tree. Your inferred role is adversarial workload design on
`Bianca_adversary`. The branch currently contains README.md, .gitignore and an
empty D5.md. The local untracked avl_adversary.cpp is also empty; it was left
untouched. No existing adversary implementation could be recovered from those files.

Your new work consists of the adversary/ directory and this handoff plus D5.md.
The broader review documents and reference sources are reading/testing material
in separate ZIPs; they are not teammate code you should commit as your own.
Detailed file responsibilities, commands and caveats are in adversary/README.md.

## Team interfaces

- Madhav: common Board contract, application replay, handwritten player table.
- Prince: AVL tree, rotations, subtree sizes, split and ordered join.
- Saksham: Fenwick counts, compressed scores, sorted tie buckets, split/rebuild.
- Om: sorted-array oracle, differential replay, invariants and failure reducer.
- Sumit: rank benchmark and later performance experiments.
- You/adversary: difficult reproducible traces, predicted costs and D5 draft.

These ownership labels are inferred from branch/file content; they are not a
substitute for an official team-allocation sheet.

## Commit and push yourself (PowerShell)

Use a fresh clone so existing uncommitted work stays separate. Extract only
`adversary_contribution.zip` into the clone root. It has no outer directory.
Replace the ZIP path below with the location of your downloaded file.

```powershell
git clone https://github.com/pachisiamadhav26-eng/DSA_PROJECT.git DSA_PROJECT_adversary
Set-Location DSA_PROJECT_adversary
git switch Bianca_adversary
Expand-Archive -LiteralPath "$env:USERPROFILE/Downloads/adversary_contribution.zip" -DestinationPath . -Force
python adversary/run.py
git status --short
git diff -- D5.md
git add -- adversary D5.md ADVERSARY_HANDOFF.md
git diff --cached --stat
git diff --cached
git commit -m "Add reproducible adversarial leaderboard workloads and D5 derivation"
git push origin Bianca_adversary
```

If g++ is not on PATH, pass `--compiler C:/msys64/ucrt64/bin/g++.exe` or your
actual C++17 compiler path. Stop if tests fail. `git diff --cached` lets you review
new files as well as the D5 replacement. The final two commands create and push
your commit only when YOU run them. None were executed during preparation.
Do not use `git add .`: the explicit paths keep review/reference material out.

If you already have a working clone, first inspect `git status`, then copy these
three paths into the intended branch and use the same test/stage/review commands.
Do not force-push or reset local work. This package does not merge teammates'
branches or implement their missing production adapters.
