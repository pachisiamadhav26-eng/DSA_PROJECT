"""Filter tracerpt CSV exports to benchmark threads and summarize PMU samples.

First decode each hardware.etl with tracerpt -of CSV, saving its event summary.
The Windows payload schema used here is saved beside the formal recordings.
"""

import argparse
from collections import Counter
import csv
import hashlib
import json
from pathlib import Path
import re


def analyze(directory, decoded):
    metadata = json.loads((directory / "profile.json").read_text())
    pid = metadata["benchmark_pid"]
    threads = set()
    # PMU event header PID/TID can be 0xffffffff. Resolve payload ThreadId
    # through thread lifecycle/rundown events, whose headers identify the PID.
    with decoded.open(newline="", encoding="mbcs") as source:
        for row in csv.reader(source, skipinitialspace=True):
            if len(row) >= 22 and row[0].strip() == "Thread":
                if int(row[9], 16) == pid:
                    threads.add(int(row[10], 16))
    if not threads:
        raise ValueError(f"No thread mapping for benchmark PID {pid}")
    samples = []
    all_pmu = 0
    with decoded.open(newline="", encoding="mbcs") as source:
        for row in csv.reader(source, skipinitialspace=True):
            if len(row) < 23 or row[1].strip() != "PmcCounterProf":
                continue
            all_pmu += 1
            if row[3].strip() != "2":
                raise ValueError("Unsupported PMU payload version")
            tid, event = int(row[20]), int(row[21])
            if tid in threads and event == 29:  # LLCMisses in saved pmcsources
                samples.append([row[16].strip(), tid, row[11].strip(), row[19].strip(), event])
    with (directory / "benchmark-samples.csv").open("w", newline="") as target:
        writer = csv.writer(target)
        writer.writerow(["timestamp_100ns", "thread_id", "processor", "instruction_pointer", "source_id"])
        writer.writerows(samples)
    summary = (directory / "event-summary.txt").read_text()
    lost = re.search(r"Total Events\s+Lost\s+(\d+)", summary)
    calls = re.search(r"calls=(\d+)", (directory / "benchmark.txt").read_text())
    reported = re.search(r"\|\s*(\d+)\s+PerfInfo\s+0\s+PmcCounterProf", summary)
    if not lost or not calls:
        raise ValueError("Missing lost-event status or query count")
    result = dict(backend=metadata["command"][2], operation=metadata["command"][metadata["command"].index("--operation") + 1],
                  benchmark_pid=pid, benchmark_threads=sorted(threads), calls=int(calls[1]),
                  event="LLCMisses", source_id=29, sampling_interval=metadata["sampling_interval"],
                  benchmark_samples=len(samples), system_samples=all_pmu, lost_events=int(lost[1]),
                  tracerpt_reported_pmu_events=int(reported[1]) if reported else None,
                  etl_sha256=hashlib.sha256((directory / "hardware.etl").read_bytes()).hexdigest(),
                  samples_per_million_queries=len(samples) * 1000000 / int(calls[1]),
                  samples_by_thread=dict(Counter(sample[1] for sample in samples)),
                  interpretation="One diagnostic capture; samples are not exact misses or a miss rate. Includes loop/checksum and OS activity on benchmark threads; no function attribution. Decoded sample counts may differ from tracerpt's aggregate event summary.")
    (directory / "analysis.json").write_text(json.dumps(result, indent=2))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("results", type=Path)
    parser.add_argument("decoded", type=Path, help="Directory containing BACKEND-OPERATION-events.csv exports")
    args = parser.parse_args()
    results = [analyze(directory, args.decoded / f"{directory.name}-events.csv")
               for directory in sorted((args.results / "profiles").iterdir()) if directory.is_dir()]
    if not results:
        raise ValueError("No profiles found")
    (args.results / "profiles" / "analysis.json").write_text(json.dumps(results, indent=2))
    print(json.dumps(results, indent=2))


if __name__ == "__main__":
    main()
