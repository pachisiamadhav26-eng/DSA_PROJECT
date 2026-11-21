"""Copy one completed run into the required final results paths, retaining provenance."""

import argparse
import hashlib
import json
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parent.parent


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run", type=Path, help="Completed run directory, e.g. results/full-20261117-final")
    args = parser.parse_args()
    source = args.run.resolve()
    destination = ROOT / "results"
    source.relative_to(ROOT)  # Only publish a run inside this repository.
    if source == destination:
        raise ValueError("Choose a run subdirectory, not results itself")
    if json.loads((source / "metadata.json").read_text(encoding="utf-8"))["status"] != "complete":
        raise ValueError("Only a completed run can supply the submission results")
    files = [(path, destination / "raw" / path.name) for path in sorted((source / "raw").glob("*.csv"))]
    if not files:
        raise ValueError("The selected run has no raw CSV measurements")
    for pattern in ("*.png", "*.svg", "sources.json", "INDEX.md"):
        files += [(path, destination / "figures" / path.name) for path in sorted((source / "figures").glob(pattern))]
    files += [(source / "summary.csv", destination / "summary.csv")]
    if not files or not (source / "figures/sources.json").is_file():
        raise ValueError("Run results/plot.py on the completed run before exporting")
    record = []
    for original, target in files:
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(original, target)
        original_hash = digest(original)
        if digest(target) != original_hash:
            raise RuntimeError(f"Export verification failed: {target}")
        record.append({"path": str(target.relative_to(ROOT)).replace("\\", "/"),
                       "source": str(original.relative_to(ROOT)).replace("\\", "/"), "sha256": original_hash})
    provenance = {"source_run": str(source.relative_to(ROOT)).replace("\\", "/"),
                  "source_metadata_sha256": digest(source / "metadata.json"),
                  "plot_script_sha256": digest(ROOT / "results/plot.py"),
                  "export_script_sha256": digest(Path(__file__)), "files": record}
    (destination / "publication.json").write_text(json.dumps(provenance, indent=2), encoding="utf-8")
    print(f"Exported and SHA-256 verified {len(files)} files to results/raw, results/figures and results/summary.csv")


if __name__ == "__main__":
    main()
