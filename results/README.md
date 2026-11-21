# Analyst results

The final submission paths contain verified copies of the completed 17 November run:

| Path | Contents |
| --- | --- |
| [raw/](raw/) | 158 raw measurement CSVs |
| [figures/](figures/) | 29 PNG/SVG figure pairs |
| [figures/INDEX.md](figures/INDEX.md) | Exact input files and selectors for each figure |
| [summary.csv](summary.csv) | Five-run means, sample standard deviations and min/max |
| [publication.json](publication.json) | Source run and SHA-256 of every published file |

Read [the Analyst report](../REPORT.md) for the methods, prediction comparisons and limitations, or [the simple latency tables](full-20261117-final/README.md) for a quick explanation. The timestamped [source run](full-20261117-final/) retains machine/compiler metadata, inputs, logs and hardware recordings. Its historical paths and hashes describe the original execution.

From the repository root:

```powershell
# Regenerate summaries, graphs and exact source captions from the saved measurements.
python results/plot.py results/full-20261117-final

# Publish and hash-check the copies under the final required paths.
python results/export_submission.py results/full-20261117-final
```

These commands process saved measurements; they do not rerun the benchmark. Use `bench/run_all.py` for new experiments. The team integration PR must supply the full team report/PDF and team-wide run command.
