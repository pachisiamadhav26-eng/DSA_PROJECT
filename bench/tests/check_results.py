"""Verify summaries and backend comparisons using synthetic measurements."""

import csv
import importlib.util
from pathlib import Path
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "bench"))
from run import verify_pair

spec = importlib.util.spec_from_file_location("benchmark_plot", ROOT / "results/plot.py")
plot = importlib.util.module_from_spec(spec)
spec.loader.exec_module(plot)


def main():
    with tempfile.TemporaryDirectory(dir=ROOT / "build/bench") as temporary:
        directory = Path(temporary)
        (directory / "raw").mkdir()
        fields = ["backend", "experiment", "operation", "players", "coordinates", "seed",
                  "run", "calls", "checksum", "ns_per_call"]
        paths = []
        for backend in ["avl", "fenwick"]:
            path = directory / "raw" / f"{backend}.csv"
            paths.append(path)
            with path.open("w", newline="") as target:
                writer = csv.DictWriter(target, fieldnames=fields)
                writer.writeheader()
                for run in range(1, 6):
                    writer.writerow(dict(zip(fields, [backend, "synthetic", "rank", 100, 10,
                                                     253, run, 1000, 42, run])))
        verify_pair(paths)
        records = plot.summarize(directory)
        assert len(records) == 2
        assert all(row["mean"] == 3 and row["min"] == 1 and row["max"] == 5 and row["runs"] == 5 for row in records)
        assert all(abs(row["stddev"] - 2.5**0.5) < 1e-12 for row in records)
        plot.chart(directory / "synthetic", records, "Synthetic validation only")
        assert (directory / "synthetic.png").read_bytes().startswith(b"\x89PNG")
        assert "<svg" in (directory / "synthetic.svg").read_text(encoding="utf-8")
        paths[1].write_text(paths[1].read_text().replace(",42,", ",43,"))
        try:
            verify_pair(paths)
        except RuntimeError:
            pass
        else:
            raise AssertionError("incorrect checksums were accepted")
        for trace, multiplier in [(0, 1), (1, 10)]:
            with (directory / "raw" / f"trace-{trace}-avl.csv").open("w", newline="") as target:
                writer = csv.DictWriter(target, fieldnames=fields)
                writer.writeheader()
                for run in range(1, 6):
                    writer.writerow(dict(zip(fields, ["avl", "trace_per_call_overhead_included", "rank",
                                                     100, 10, 253, run, 1000, 42, run * multiplier])))
        trace_records = [row for row in plot.summarize(directory) if row["experiment"].startswith("trace")]
        assert len(trace_records) == 2 and all(row["runs"] == 5 for row in trace_records)
        assert sorted(row["mean"] for row in trace_records) == [3, 30]
    print("PASS: five-run means/spread, image generation, and backend mismatch detection")


if __name__ == "__main__":
    main()
