"""Summarize raw repetitions and export Matplotlib error-bar figures."""

import argparse
import csv
from collections import defaultdict
from pathlib import Path
import statistics
import sys

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "build/bench/python"))


def summarize(directory):
    groups = defaultdict(list)
    for path in sorted((directory / "raw").glob("*.csv")):
        with path.open(newline="", encoding="utf-8") as source:
            for row in csv.DictReader(source):
                # Different trace files can share sizes and operations.
                if row["experiment"].startswith("trace"):
                    trace_name = path.stem.rsplit("-", 1)[0]
                    row["experiment"] += "_" + trace_name
                key = tuple(row[k] for k in ("backend", "experiment", "operation", "players", "coordinates", "seed"))
                groups[key].append(row)
    records = []
    for key, rows in sorted(groups.items()):
        counter = key[2].endswith("_count")
        values = [int(row["checksum"]) if counter else float(row["ns_per_call"]) for row in rows]
        records.append(dict(zip(
            ["backend", "experiment", "operation", "players", "coordinates", "seed", "runs", "mean", "stddev", "min", "max", "unit"],
            [*key, len(values), statistics.mean(values), statistics.stdev(values) if len(values) > 1 else 0,
             min(values), max(values), "count" if counter else "ns/call"])))
    if not records:
        raise ValueError("no raw CSV rows found")
    with (directory / "summary.csv").open("w", newline="", encoding="utf-8") as target:
        writer = csv.DictWriter(target, fieldnames=list(records[0]))
        writer.writeheader()
        writer.writerows(records)
    return records


def chart(path, rows, title):
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    figure, axis = plt.subplots(figsize=(8.4, 4.8), layout="constrained")
    for backend, color, marker in [("avl", "#2563eb", "o"), ("fenwick", "#c2410c", "s")]:
        series = sorted((row for row in rows if row["backend"] == backend), key=lambda row: int(row["players"]))
        if not series:
            continue
        means = [float(row["mean"]) for row in series]
        spread = [[mean - float(row["min"]) for mean, row in zip(means, series)],
                  [float(row["max"]) - mean for mean, row in zip(means, series)]]
        axis.errorbar([int(row["players"]) for row in series], means, yerr=spread,
                      color=color, marker=marker, capsize=4, label=backend)
    axis.set_xscale("log")
    axis.set_ylim(bottom=0)
    axis.set_xlabel("Active players (log scale)")
    axis.set_ylabel("Mean nanoseconds per call")
    axis.set_title(title.replace("_", " "), fontsize=10)
    axis.grid(alpha=0.25)
    axis.legend(title="Mean with min-max spread")
    figure.savefig(path.with_suffix(".svg"))
    figure.savefig(path.with_suffix(".png"), dpi=180)
    plt.close(figure)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    args = parser.parse_args()
    records = summarize(args.directory)
    charts = args.directory / "figures"
    charts.mkdir(exist_ok=True)
    groups = defaultdict(list)
    for row in records:
        if row["unit"] == "ns/call" and not row["experiment"].startswith("trace"):
            groups[(row["experiment"], row["operation"], row["seed"])].append(row)
    for index, (key, rows) in enumerate(sorted(groups.items())):
        chart(charts / f"{index + 1:02d}-{key[1]}", rows, f"{key[0]} / {key[1]} / seed {key[2]}")
    print(f"Saved summary.csv and {len(groups)} SVG/PNG charts in {args.directory}")


if __name__ == "__main__":
    main()
