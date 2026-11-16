"""Split benchmark output into experiments and plot five-run means."""
import argparse
import csv
import math
from pathlib import Path
from statistics import mean, stdev

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

GROUPS = {
    "queries": ("rank", "select"),
    "season": ("split", "merge_validated", "join_ordered"),
    "height_gap": ("height_join",),
    "ties": ("tie_insert",),
}
COLORS = {"avl": "#144E75", "fenwick": "#AD4A12"}


def points(rows, name, backend, axis):
    buckets = {}
    for row in rows:
        if row["operation"] == name and row["backend"] == backend:
            buckets.setdefault(int(row[axis]), []).append(float(row["ns_per_operation"]))
    if not buckets:
        return [], [], []
    if any(len(values) != 5 for values in buckets.values()):
        raise ValueError(f"expected five runs per {name}/{backend} point")
    xs = sorted(buckets)
    return xs, [mean(buckets[x]) for x in xs], [stdev(buckets[x]) for x in xs]


def cost(op, backend, x):
    if op == "height_join":
        return 1 + x
    if op == "join_ordered":
        return 1
    if op == "tie_insert":
        return x * math.log2(x) if backend == "avl" else x * (x - 1) / 2
    if op == "merge_validated":
        return x * math.log2(x) if backend == "avl" else x
    if op == "split" and backend == "fenwick":
        return x
    return math.log2(x)


def panel(ax, rows, title, operations, unit, factor=1, xaxis="n"):
    for op in operations:
        for backend in COLORS:
            xs, times, spreads = points(rows, op, backend, xaxis)
            if not xs:
                continue
            values = [value / factor for value in times]
            spreads = [value / factor for value in spreads]
            label = {"avl": "AVL", "fenwick": "Fenwick"}[backend]
            if op == "join_ordered":
                label += " supplied-pivot"
            style = ":" if op == "join_ordered" else "-"
            ax.errorbar(xs, values, yerr=spreads, fmt="o", ls=style, color=COLORS[backend],
                        capsize=3, markersize=4, label=label)
            model = [values[0] * cost(op, backend, x) / cost(op, backend, xs[0]) for x in xs]
            ax.plot(xs, model, "--", color=COLORS[backend], linewidth=1, alpha=.6)
    ax.set_title(title)
    if xaxis == "n":
        ax.set_xscale("log")
        ax.set_xlabel("Players")
    else:
        ax.set_xlabel("Input height difference (levels)")
    ax.set_ylabel(unit)
    ax.grid(True, which="major", alpha=.24)
    ax.legend(fontsize=8)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("results", type=Path)
    parser.add_argument("--collect", type=Path, help="combined CSV from scaling_bench")
    args = parser.parse_args()
    raw, figures = args.results / "raw", args.results / "figures"
    raw.mkdir(parents=True, exist_ok=True)
    figures.mkdir(parents=True, exist_ok=True)
    if args.collect:
        with args.collect.open(newline="") as source:
            records = csv.DictReader(source)
            fields, rows = records.fieldnames, list(records)
        for group, operations in GROUPS.items():
            with (raw / f"{group}.csv").open("w", newline="") as out:
                writer = csv.DictWriter(out, fields)
                writer.writeheader()
                writer.writerows(row for row in rows if row["operation"] in operations)
    for group in GROUPS:
        with (raw / f"{group}.csv").open(newline="") as source:
            rows = list(csv.DictReader(source))
        if group == "queries":
            fig, axes = plt.subplots(1, 2, figsize=(9, 3.4), layout="constrained")
            panel(axes[0], rows, "Rank", ["rank"], "ns per query")
            panel(axes[1], rows, "Select", ["select"], "ns per query")
        elif group == "season":
            fig, axes = plt.subplots(1, 2, figsize=(9, 3.4), layout="constrained")
            panel(axes[0], rows, "Threshold split", ["split"], "us per split", 1000)
            panel(axes[1], rows, "Rejoin", ["merge_validated", "join_ordered"], "us per rejoin", 1000)
            for ax in axes:
                ax.set_yscale("log")
        else:
            fig, ax = plt.subplots(figsize=(7, 3.4), layout="constrained")
            if group == "height_gap":
                panel(ax, rows, "Supplied-pivot AVL join", ["height_join"], "ns per join", xaxis="height_gap")
            else:
                panel(ax, rows, "Descending IDs at one score", ["tie_insert"], "ms per batch", 1000000)
                ax.set_yscale("log")
        fig.suptitle("Points: mean ± sample SD   |   dashed: model anchored at first point", fontsize=10)
        fig.savefig(figures / f"{group}.png", dpi=180)
        plt.close(fig)


if __name__ == "__main__":
    main()
