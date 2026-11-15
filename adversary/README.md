# Adversarial replays

Module owner: Bianca. The generator emits deterministic C++17 command traces for
the production replay app. See [D5](../docs/derivations/D5.pdf) for the AVL split
cost and the rotation-heavy insertion proof.

```powershell
python tools/project.py adversary
python tools/project.py --full adversary
```

The first command runs targeted cases plus a 10,000-player, 10,000-revision
exerciser. The second uses 1,000,000 players and 1,000,000 revisions. Both
commands compile and use the same AVL, Fenwick and oracle as the app.

| Case | What it stresses |
| --- | --- |
| rotations, ascending, descending | All four local rotations, monotone insertions, stale height or size |
| ties | Shared ranks, ID order, vector insertions at the front |
| churn | Deletion, reinsertion, no-op revision and stale lookup |
| extremes | Full score range, split boundaries |
| seasons, unequal | Overlapping leagues, empty sides and unequal AVL heights |
| errors | Invalid commands, wrong board and rejection atomicity |
| mixed | Seeded interleavings of updates and observations |
| scale | Unique live scores with interleaved rank/select/range and season split/rejoin |

Run the production generator directly after `python tools/project.py build`:

```powershell
./build/project/generator.exe --case mixed --players 64 --ops 10000 --seed 253
```

Targeted traces are generated twice for reproducibility, checked against the
independent sorted-vector oracle, and passed through per-command backend audits.
On the scale trace the set of live scores after revision t is [t+1,t+n], so
every expected answer can be computed independently without shifting a million
vector entries per update. Both app modes audit the trees and the handwritten
player table at season boundaries and at replay end. The log and trace are in
`build/adversary/`. The committed files under `validation/` document earlier
standalone adapter runs.
