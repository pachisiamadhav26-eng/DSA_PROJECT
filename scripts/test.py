"""Check the integrated application, structures, oracle, and submitted fixtures."""

import argparse
import os
from pathlib import Path
import subprocess


ROOT = Path(__file__).resolve().parent.parent
SUFFIX = ".exe" if os.name == "nt" else ""


def run(command):
    result = subprocess.run([str(x) for x in command], cwd=ROOT, capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(f"{' '.join(map(str, command))} failed:\n{result.stdout}{result.stderr}")
    return result.stdout.splitlines()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--quick", action="store_true")
    parser.add_argument("--bin-dir", type=Path, default=ROOT / "build/bin")
    args = parser.parse_args()
    binary = lambda name: args.bin_dir / (name + SUFFIX)

    for name in ("player_table_test", "replay_test", "avl_test", "fenwick_test"):
        run([binary(name)])
        print("PASS", name, flush=True)

    run([binary("differential"), "--self-test"])
    print("PASS differential self-test", flush=True)

    fixtures = [*sorted((ROOT / "oracle/tests").glob("*.trace")),
                *sorted((ROOT / "adversary/tests").glob("*.trace"))]
    for trace in fixtures:
        expected = trace.with_suffix(".expected").read_text(encoding="utf-8").splitlines()
        for backend in ("avl", "fenwick"):
            if run([binary("leaderboard"), backend, trace]) != expected:
                raise RuntimeError(f"{backend} differs from {trace}")
        if run([binary("oracle"), trace]) != expected:
            raise RuntimeError(f"oracle differs from {trace}")
        run([binary("differential"), "--trace", trace])
        print("PASS fixture", trace.relative_to(ROOT), flush=True)

    count = 10000 if args.quick else 1000000
    run([binary("differential"), "--seed", "253", "--ops", str(count), "--max-live", "48"])
    print("PASS seeded differential", count, "operations", flush=True)
    run([binary("differential"), "--scale", str(count)])
    print("PASS scale", count, "players", flush=True)

    generator = binary("generator")
    if generator.exists():
        for case in ("rotations", "cascade", "ties", "seasons", "unequal", "mixed"):
            command = [generator, "--case", case, "--players", "88" if case == "cascade" else "64",
                       "--ops", "10000" if args.quick else "100000", "--seed", "253"]
            trace = ROOT / "build" / ("generated-" + case + ".trace")
            trace.write_text("\n".join(run(command)) + "\n", encoding="utf-8")
            run([binary("differential"), "--trace", trace])
            print("PASS generated", case, flush=True)
    print("ALL TESTS PASSED", flush=True)


if __name__ == "__main__":
    main()
