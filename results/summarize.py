"""Turn benchmark CSV trials into summary CSV and standalone vector figures."""

import argparse
import csv
from collections import defaultdict
from math import ceil, floor, log10
from pathlib import Path
import statistics

PALETTE = {"AVL": "#1466a3", "Fenwick": "#b64c2e"}


def summarize(raw):
    groups = defaultdict(list)
    with raw.open(encoding="utf-8", newline="") as stream:
        for row in csv.DictReader(stream):
            key = tuple(row[k] for k in ("backend", "metric", "n", "u", "tie_size", "seed", "height_gap"))
            groups[key].append(row)
    rows = []
    for key, group in groups.items():
        if sorted(int(row["trial"]) for row in group) != list(range(1, 6)):
            raise ValueError(f"Expected five trials for {key}")
        times = [float(r["total_ns"]) / int(r["operations"]) for r in group]
        if any(x <= 0 for x in times):
            raise ValueError(f"Timer resolution too low for {key}")
        row = dict(zip(("backend", "metric", "n", "u", "tie_size", "seed", "height_gap"), key))
        row.update(mean_ns=round(statistics.mean(times), 2), min_ns=round(min(times), 2),
                   max_ns=round(max(times), 2), stddev_ns=round(statistics.stdev(times), 2))
        for name in ("rotations", "bucket_shifts", "join_visits"):
            row[f"mean_{name}"] = round(statistics.mean(int(r[name]) for r in group), 2)
        rows.append(row)
    with raw.with_name("benchmark_summary.csv").open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    return rows


def plot(path, rows, metric, title, x_label):
    selected = [row for row in rows if row["metric"] == metric]
    pivot = metric == "supplied_pivot_join"
    coordinate = "height_gap" if pivot else "n"
    xs = sorted({int(row[coordinate]) for row in selected})
    lower = floor(log10(min(row["min_ns"] for row in selected)))
    upper = ceil(log10(max(row["max_ns"] for row in selected)))
    start, end = (xs[0], xs[-1]) if pivot else (log10(xs[0]), log10(xs[-1]))

    def x(value):
        numeric = value if pivot else log10(value)
        return 84 + (numeric - start) * 612 / max(1, end - start)

    def y(value):
        return 356 - (log10(value) - lower) * 266 / max(1, upper - lower)

    svg = ['<svg xmlns="http://www.w3.org/2000/svg" width="760" height="470" viewBox="0 0 760 470">',
           '<rect width="760" height="470" fill="white"/>',
           '<g font-family="Arial, sans-serif" fill="#19283a">',
           f'<text x="84" y="35" font-size="23" font-weight="bold">{title}</text>',
           '<text x="84" y="61" font-size="13">Five trials per point · bars show min to max · time in ns (log)</text>']
    for power in range(lower, upper + 1):
        line = y(10**power)
        svg.append(f'<path d="M84 {line:.2f}H696" stroke="#e0e6ed"/>')
        svg.append(f'<text x="74" y="{line + 4:.2f}" font-size="11" text-anchor="end">{10**power:g}</text>')
    for value in xs:
        point = x(value)
        svg.append(f'<path d="M{point:.2f} 90V356" stroke="#e0e6ed"/>')
        svg.append(f'<text x="{point:.2f}" y="376" font-size="11" text-anchor="middle">{value:,}</text>')
    svg.append(f'<text x="390" y="402" font-size="13" text-anchor="middle">{x_label}</text>')

    series = defaultdict(list)
    for row in selected:
        series[(row["backend"], int(row["tie_size"]))].append(row)
    for i, ((backend, ties), points) in enumerate(sorted(series.items())):
        color = PALETTE[backend]
        dash = ' stroke-dasharray="6 4"' if ties != 1 else ""
        points.sort(key=lambda row: int(row[coordinate]))
        coords = [(x(int(row[coordinate])), y(row["mean_ns"])) for row in points]
        polyline = " ".join(f"{a:.2f},{b:.2f}" for a, b in coords)
        svg.append(f'<polyline points="{polyline}" fill="none" stroke="{color}" stroke-width="2"{dash}/>')
        for (a, b), row in zip(coords, points):
            lo, hi = y(row["min_ns"]), y(row["max_ns"])
            svg.append(f'<path d="M{a:.2f} {lo:.2f}V{hi:.2f}" stroke="{color}"/>')
            svg.append(f'<circle cx="{a:.2f}" cy="{b:.2f}" r="3" fill="{color}"/>')
        label = backend if pivot else f"{backend}, ties {ties}"
        offset = 84 + 153 * i
        svg.append(f'<path d="M{offset} 438h20" stroke="{color}" stroke-width="2"{dash}/>')
        svg.append(f'<text x="{offset + 27}" y="442" font-size="12">{label}</text>')
    svg.append("</g></svg>")
    path.write_text("\n".join(svg) + "\n", encoding="utf-8")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("raw", type=Path)
    raw = parser.parse_args().raw
    result = summarize(raw)
    charts = {
        "rank": "Competition rank latency",
        "select": "Selection latency",
        "split": "Split by score",
        "separated_rejoin": "Rejoin separated divisions",
        "overlap_merge": "Merge overlapping leagues",
        "supplied_pivot_join": "AVL join with supplied pivot",
        "descending_build": "Insertion with descending IDs",
    }
    for metric, title in charts.items():
        plot(raw.with_name(metric + ".svg"), result, metric, title,
             "Difference in AVL input heights" if metric == "supplied_pivot_join" else "Live players (log)")
