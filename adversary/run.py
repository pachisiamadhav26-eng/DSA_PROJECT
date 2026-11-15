"""Run deterministic workloads against the production app and the oracle."""

import argparse
import hashlib
from itertools import zip_longest
import json
import os
from pathlib import Path
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build" / "project"
WORK = ROOT / "build" / "adversary"
SUFFIX = ".exe" if os.name == "nt" else ""


def executable(name):
    return BUILD / (name + SUFFIX)


def run(*command, **kwargs):
    return subprocess.run([str(value) for value in command], cwd=ROOT,
                          check=True, text=True, **kwargs)


def generated(case, players, ops, seed):
    return (executable("generator"), "--case", case, "--players", players,
            "--ops", ops, "--seed", seed)


def small_cases(operations):
    for trace in sorted((ROOT / "adversary" / "tests").glob("*.trace")):
        expected = trace.with_suffix(".expected").read_text(encoding="utf-8")
        compare_small(trace, expected)
    cases = ("rotations", "ascending", "descending", "ties", "churn", "extremes",
             "seasons", "unequal", "errors", "mixed")
    for name in cases:
        for seed in (253, 42, 20261115) if name == "mixed" else (253,):
            command = generated(name, 64, operations, seed)
            text = run(*command, capture_output=True).stdout
            if text != run(*command, capture_output=True).stdout:
                raise RuntimeError(f"Non-reproducible workload {name}/{seed}")
            if name == "mixed" and len(text.splitlines()) != operations:
                raise RuntimeError("Wrong mixed command count")
            trace = WORK / f"{name}-{seed}.trace"
            trace.write_text(text, encoding="utf-8")
            compare_small(trace)


def compare_small(trace, expected=None):
    reference = run(executable("oracle"), trace, capture_output=True).stdout
    if expected is not None and reference != expected:
        raise RuntimeError(f"Oracle differs from golden output for {trace.name}")
    for backend in ("avl", "fenwick"):
        actual = run(executable("leaderboard"), backend, trace, "--audit",
                     capture_output=True).stdout
        if actual != reference:
            raise RuntimeError(f"{backend} differs from oracle for {trace.name}")
    run(executable("differential"), "--trace", trace)
    print(f"PASS {trace.name} through the production app and oracle", flush=True)


def scale_expected(n, revisions):
    # At revision t the scores form [t+1,t+n], and the ID of score s
    # is 1+(s-1)%n. This gives independent expected results in O(1) space.
    def player(score):
        return f"PLAYER {(score - 1) % n + 1} {score}"

    def page(high, length):
        pairs = " ".join(f"{(s - 1) % n + 1} {s}" for s in range(high, high - length, -1))
        return f"PLAYERS {length} {pairs}"

    for _ in range(n):
        yield "OK"
    stride = max(1, n // 32)
    for t in range(1, revisions + 1):
        yield "OK"
        if t % stride == 0 or t == revisions:
            yield "RANK 1"
            yield player(t + 1)
            yield page(t + n, min(n, 3))
    high = n + revisions
    yield "OK"
    yield f"SIZE {n // 2}"
    yield player(high)
    yield f"SIZE {n - n // 2}"
    yield player(high - n // 2)
    yield "OK"
    yield f"SIZE {n}"
    yield player(high)
    yield player(revisions + 1)
    yield page(high, min(n, 12))


def trace_digest(trace):
    sha = hashlib.sha256()
    counts = {}
    with trace.open("rb") as content:
        for line in content:
            sha.update(line)
            name = line.split(maxsplit=1)[0].decode("ascii")
            counts[name] = counts.get(name, 0) + 1
    return sha.hexdigest(), counts


def scale_case(players, revisions):
    trace = WORK / "scale.trace"
    with trace.open("wb") as output:
        run(*generated("scale", players, revisions, 253), stdout=output)
    sha, counts = trace_digest(trace)
    if counts.get("ADD") != players or counts.get("SET", 0) != revisions:
        raise RuntimeError("Wrong number of setup players or revisions")
    summary = {"players": players, "revisions": revisions, "sha256": sha,
               "command_counts": counts, "backends": {}}
    for backend in ("avl", "fenwick"):
        output = WORK / f"scale-{backend}.out"
        start = time.perf_counter()
        with output.open("wb") as stream:
            run(executable("leaderboard"), backend, trace, "--audit", stdout=stream)
        elapsed = time.perf_counter() - start
        with output.open(encoding="ascii") as lines:
            actual = (line.rstrip("\r\n") for line in lines)
            count = 0
            for count, (got, want) in enumerate(zip_longest(actual, scale_expected(players, revisions)), 1):
                if got != want:
                    raise RuntimeError(f"{backend} line {count}: actual {got!r}, expected {want!r}")
        if count != sum(counts.values()):
            raise RuntimeError(f"{backend} output line count does not match trace")
        summary["backends"][backend] = {"output_lines": count, "audit_wall_seconds": elapsed}
        print(f"PASS {backend} scale {players} players, {revisions} revisions, "
              f"{count} outputs ({elapsed:.2f} s including audits and I/O)", flush=True)
    (WORK / "scale-summary.json").write_text(json.dumps(summary, indent=2) + "\n",
                                             encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", default=os.environ.get("CXX", "g++"))
    parser.add_argument("--ops", type=int, default=10000, help="bounded mixed commands per seed")
    parser.add_argument("--players", "--scale", type=int, default=10000)
    parser.add_argument("--revisions", type=int, default=10000)
    parser.add_argument("--no-build", action="store_true")
    parser.add_argument("--scale-only", action="store_true")
    args = parser.parse_args()
    if not 2 <= args.players <= 1000000 or not (0 <= args.ops <= 10000000 and
                                               0 <= args.revisions <= 10000000):
        parser.error("players must be 2..1000000; operations and revisions 0..10000000")
    if not args.no_build:
        run(sys.executable, "tools/project.py", "--compiler", args.compiler, "build")
    WORK.mkdir(parents=True, exist_ok=True)
    if not args.scale_only:
        small_cases(args.ops)
    scale_case(args.players, args.revisions)


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(error, file=sys.stderr)
        sys.exit(1)
