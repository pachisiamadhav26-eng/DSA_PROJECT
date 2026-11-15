# Rank Queries on a Live Leaderboard

CS 253 / MA 253 Data Structures and Algorithms Laboratory · Project 2

The application replays score revisions, competition ranks, position queries,
score splits and league merges against either an augmented AVL tree or a
Fenwick tree. The same handwritten player table and command parser drive both
backends. A sorted vector supplies the independent reference for correctness.

## Run from a fresh clone

Install Python 3.8+ and a C++17 compiler such as `g++`, then open a terminal
in the repository root. The build, sample replay, tests, adversarial workloads
and all benchmark cases run with one command:

```powershell
python tools/project.py all
```

For the full evaluation run, including **one million active players, one million
score revisions and one million individually audited seeded commands**:

```powershell
python tools/project.py --full all
```

To build and try a replay interactively in the terminal:

```powershell
python tools/project.py build
./build/project/leaderboard.exe avl app/examples/basic.trace --audit
./build/project/leaderboard.exe fenwick app/examples/basic.trace --audit
```

Both produce the 12 lines in [basic.expected](app/examples/basic.expected).
On Linux/macOS, use `python3` and omit `.exe`. Use
`python tools/project.py --compiler PATH build` for a compiler off PATH.
After building, use `--skip-build` with `demo`, `test`, `adversary` or `bench`
to reuse the binaries. Generated traces, output logs and fresh benchmark CSVs
are stored under `build/`.

## Rules and ownership

Scores descend and IDs ascend within a tie. Competition ranks for scores
100, 90, 90, 70 are 1, 2, 2, 4. Select uses a unique one-based position.
Scores equal to a split threshold stay in high. Successful splits and merges
consume their source boards.

| Role | Owner | Module | Derivation |
| --- | --- | --- | --- |
| AVL | Prince Chavda | [src/structA](src/structA/) | [D1](docs/derivations/D1.pdf) |
| Fenwick | Saksham Bhatia | [src/structB](src/structB/) | [D2](docs/derivations/D2.pdf) |
| Architect | Madhav Pachisia | [app](app/) | [D3](docs/derivations/D3.pdf) |
| Oracle | Om Dehariya | [oracle](oracle/) | [D4](docs/derivations/D4-om.pdf) |
| Adversary | Bianca | [adversary](adversary/) | [D5](docs/derivations/D5.pdf) |
| Analyst | Sumit Dobal | [bench](bench/) | [D6](docs/derivations/D6.pdf) |

The full command format and invariants are in [INTERFACE.md](INTERFACE.md).
[REPORT.md](REPORT.md) explains the cost model, test evidence and measured
results. [TOOLS.md](TOOLS.md) records tool assistance.
