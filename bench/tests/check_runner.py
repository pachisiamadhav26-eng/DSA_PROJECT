"""Check the runner against the real backends and Oracle using small inputs."""

import csv
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "bench"))
from run import load_build, verify_pair


def main():
    manifest, binary = load_build()
    if not (manifest["oracle"] and manifest["avl_tools"] and manifest["fenwick_tools"]):
        raise RuntimeError("build with the oracle and both helper sets before running checks")
    with tempfile.TemporaryDirectory(dir=ROOT / "build/bench") as temporary:
        directory = Path(temporary)

        def check(backend, name, options):
            output = directory / f"{name}-{backend}.csv"
            command = [str(binary), "--backend", backend, "--players", "32", "--scores", "10",
                       "--updates", "100", "--runs", "1", "--output", str(output), *options]
            result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
            if result.returncode:
                raise RuntimeError(f"{name}/{backend} failed:\n{result.stdout}{result.stderr}")
            with output.open(newline="", encoding="utf-8") as source:
                rows = list(csv.DictReader(source))
            assert rows and all(row["backend"] == backend for row in rows)
            return output

        for name, options in [
            ("queries", ["--experiment", "queries"]),
            ("empty-high", ["--experiment", "seasons", "--high-percent", "0"]),
            ("middle", ["--experiment", "seasons", "--high-percent", "50"]),
            ("empty-low", ["--experiment", "seasons", "--high-percent", "100"]),
        ]:
            verify_pair([check(backend, name, options) for backend in ("avl", "fenwick")])
        for percent in (0, 1, 50, 99, 100):
            check("avl", f"join-{percent}", ["--experiment", "ordered-join", "--high-percent", str(percent)])
        for backend in ("avl", "fenwick"):
            check(backend, "counters", ["--experiment", "counters"])

        fixture = Path(manifest["source_root"]) / "oracle/tests/basic.trace"
        paths = [check(backend, "trace", ["--experiment", "trace", "--trace", str(fixture),
                                         "--check-trace", "1"]) for backend in ("avl", "fenwick")]
        verify_pair(paths)
        # A later invocation must preserve existing results.
        result = subprocess.run([str(binary), "--output", str(paths[0])], capture_output=True)
        assert result.returncode != 0
    print("PASS: real-backend query, split, join, counter, oracle trace, and output-preservation checks")


if __name__ == "__main__":
    main()
