# Adversarial inputs

Bianca's contribution contains the generator, reproducible test runner, three
trace fixtures and D5. The runner builds the generator and the merged AVL and
Fenwick modules, checks fixtures against the application and independent oracle,
and exercises generated inputs and scale workloads.

## One-command build and test

Use Python 3.8+, Git and a compiler with full C++17 support. GCC 7 or newer is
required for structured bindings and fold expressions. The validated Windows
compiler is MSYS2 UCRT64 GCC 14.2.0. MinGW GCC 6.3 cannot build this generator.

From a full clone on Bianca_adversary, run:

```sh
python adversary/run.py --compiler g++
```

On this Windows installation, choose the compatible compiler explicitly:

```powershell
python adversary/run.py --compiler C:/msys64/ucrt64/bin/g++.exe
```

For three million-command mixed traces and one million players/revisions:

```powershell
python adversary/run.py --compiler C:/msys64/ucrt64/bin/g++.exe --ops 1000000 --scale 1000000
```

A C++17 feature probe runs before compilation. The default test sizes are 10,000
mixed commands per seed and 10,000 players/revisions. The runner writes source
hashes and command results to build/adversary/validation.json. No Python packages
are required. Each invocation rebuilds its executables.

The merged main branch currently has no final root build command. This Python
command builds the adversary validation dependency graph directly. It reads the
current include/ and src/ files, preserving Prince's and Saksham's merged code.
Application and oracle dependencies are exported into ignored build/adversary/deps
from immutable team commits using git show:

- Architect application, a4dc4382b11d60ed85697fb9560574d24a8d2366.
- Om's oracle/checker, f3f8c29cb357a29a46ad5a654f7d5fac390aceea.

The executable builds combine these pinned test dependencies with the current
merged structures and shared headers. Teammate modules are sourced from Git
history during validation. A full GitHub clone includes those ancestor objects.
If an object is missing, run git fetch origin. For a shallow clone, first run
git fetch --unshallow. The runner itself makes no network requests. The team can
adopt a shared root build after the final application and oracle are merged.

## Files and workloads

- generator.cpp emits leaderboard replay commands on standard output.
- run.py performs the build, checks all submitted fixtures, compares generated
  traces with the oracle and both structures, and checks scale replies against
  an independent calculation.
- tests/ties.trace and ties.expected cover tied ranks, ID order and score changes.
- tests/wide.trace and wide.expected cover uint64 IDs and int64 score limits.
- tests/nested_split.trace and nested_split.expected cover three live boards,
  removed-ID reuse, rejected merge and subsequent successful merge.
- The D5 source and submission PDF are in docs/derivations at the repository root.
  They prove telescoping split cost and a constant fraction of rotating insertions.

Supported cases are rotations, cascade, ascending, descending, ties, churn,
extremes, seasons, unequal, errors, mixed and scale. Cascade constructs a
minimum-node AVL within the player budget and deletes along its short-child
path. Unequal merges that shape with a singleton. Ties inserts decreasing IDs
into one score bucket. Seasons exercises cutoff equality, overlapping ranges,
consumed board IDs and empty divisions. Mixed uses mt19937_64 modulo decisions
with seeds 253, 42 and 20261115 and fresh IDs for periodic split/merge cycles.

The runner checks repeatability, exact mixed command counts, full ordered state
and backend invariants through Om's checker. Fixture outputs must agree with
both application backends and the oracle. At scale, the checker audits phase
boundaries and the application executes one revision per player. The runner
calculates expected ranks, positions, pages and split/rejoin replies separately.

After building, generate an individual trace in PowerShell:

```powershell
./build/adversary/generator.exe --case cascade --players 88 | Set-Content -Encoding ascii build/adversary/cascade.trace
./build/adversary/differential.exe --trace build/adversary/cascade.trace
```

--players accepts 2 through 1,000,000 and --ops accepts 0 through 10,000,000.
For mixed, ops is the exact command count. For scale, ops counts score revisions
in addition to setup and queries. Only mixed uses --seed. Full audits accompany
bounded differential replays. Million-player tests audit at phase boundaries.
