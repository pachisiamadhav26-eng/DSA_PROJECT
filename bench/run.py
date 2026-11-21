"""Run checked experiments, preserve provenance, and summarize results."""

import argparse
import csv
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import platform
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "build/bench/python"))

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build/bench"


def machine_info():
    info = {"platform": platform.platform(), "processor": platform.processor(),
            "python": platform.python_version()}
    if sys.platform == "win32":
        command = ("$ErrorActionPreference='Stop'; @{processors=@(Get-CimInstance Win32_Processor | "
                   "Select-Object Name,NumberOfCores,NumberOfLogicalProcessors,L2CacheSize,L3CacheSize); "
                   "caches=@(Get-CimInstance Win32_CacheMemory | Select-Object Level,InstalledSize,"
                   "MaxCacheSize,DeviceID)} | ConvertTo-Json -Depth 5 -Compress")
        result = subprocess.run(["powershell", "-NoProfile", "-Command", command], capture_output=True, text=True)
        try:
            info["windows"] = json.loads(result.stdout) if result.returncode == 0 else {"error": result.stderr}
        except ValueError:
            info["windows"] = {"error": result.stdout + result.stderr}
    elif Path("/proc/cpuinfo").exists():
        info["cpuinfo"] = Path("/proc/cpuinfo").read_text()
        info["cache"] = {str(path): path.read_text().strip()
                         for path in Path("/sys/devices/system/cpu/cpu0/cache").glob("index*/size")}
    return info


def load_build():
    manifest = json.loads((BUILD / "build.json").read_text(encoding="utf-8"))
    binary = Path(manifest["binary"])
    if hashlib.sha256(binary.read_bytes()).hexdigest() != manifest["binary_sha256"]:
        raise RuntimeError("executable differs from build record; rebuild")
    for name, digest in manifest["source_sha256"].items():
        if hashlib.sha256((ROOT / name).read_bytes()).hexdigest() != digest:
            raise RuntimeError(f"source changed since build: {name}; rebuild")
    return manifest, binary


def verify_pair(paths):
    def rows(path):
        with path.open(newline="", encoding="utf-8") as source:
            return list(csv.DictReader(source))
    a, b = map(rows, paths)
    if len(a) != len(b):
        raise RuntimeError("backend result row counts differ")
    for left, right in zip(a, b):
        keys = ("experiment", "operation", "players", "coordinates", "seed", "run", "calls", "checksum")
        if any(left[key] != right[key] for key in keys):
            raise RuntimeError(f"backend comparison failed: {left} versus {right}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--suite", choices=["quick", "full"], default="quick")
    parser.add_argument("--backend", choices=["both", "avl", "fenwick"], default="both")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--seed", type=int, default=253)
    parser.add_argument("--batch", type=int, default=256)
    parser.add_argument("--trace", type=Path, action="append", default=[])
    parser.add_argument("--check-trace", action="store_true", help="Replay supplied small traces through Om's oracle as well")
    parser.add_argument("--machine-json", type=Path, help="Add manually verified CPU/cache specifications")
    args = parser.parse_args()
    try:
        import matplotlib
    except ImportError as error:
        raise RuntimeError("install chart dependencies first: python -m pip install --target build/bench/python -r bench/requirements.txt") from error
    if not 0 <= args.seed < 2**64 or not 1 <= args.batch <= 1000000:
        raise ValueError("invalid seed or batch")
    manifest, binary = load_build()
    backends = ["avl", "fenwick"] if args.backend == "both" else [args.backend]
    if manifest["backend"] != "both" and backends != [manifest["backend"]]:
        raise RuntimeError("requested backend is absent from the build")
    if args.check_trace and not manifest["oracle"]:
        raise RuntimeError("trace oracle checking requires an oracle-enabled build")
    if args.suite == "full" and (args.backend != "both" or not manifest["oracle"] or
                                not manifest["avl_tools"] or not manifest["fenwick_tools"]):
        raise RuntimeError("full suite requires both backends, compatible oracle, --avl-tools and --fenwick-tools; rebuild after those are ready")
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    output = args.output or ROOT / "results" / f"{args.suite}-{stamp}"
    output = output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    (output / "raw").mkdir()
    machine = machine_info()
    if args.machine_json:
        machine["user_verified"] = json.loads(args.machine_json.read_text(encoding="utf-8"))
    metadata = {"started_utc": stamp, "status": "running", "suite": args.suite,
                "arguments": vars(args).copy(), "build": manifest, "machine": machine,
                "traces": [{"path": str(path.resolve()), "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}
                           for path in args.trace], "commands": [],
                "limitations": ["Large query runs check final snapshots and cross-backend checksums; these do not replace Om's differential/invariant suite.",
                                "Trace rows include per-call clock overhead. Batched query rows include loop and checksum work.",
                                "Cache profiling is a separate run. No cache conclusions follow from timing alone."]}
    metadata["matplotlib_version"] = matplotlib.__version__
    metadata["experiment_scripts_sha256"] = {
        str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
        for path in [ROOT / "bench/build.py", ROOT / "bench/run.py", ROOT / "bench/run_all.py", ROOT / "results/plot.py"]
    }
    metadata["arguments"] = {key: str(value) if isinstance(value, Path) else [str(p) for p in value]
                             if key == "trace" else value for key, value in vars(args).items()}
    record = output / "metadata.json"

    def save():
        record.write_text(json.dumps(metadata, indent=2), encoding="utf-8")

    def execute(backend, experiment, suffix, extra):
        target = output / "raw" / f"{suffix}-{backend}.csv"
        command = [str(binary), "--backend", backend, "--experiment", experiment,
                   "--seed", str(args.seed), "--runs", "5", "--output", str(target), *map(str, extra)]
        metadata["commands"].append(command)
        save()
        with (output / "run.log").open("a", encoding="utf-8") as log:
            log.write("\n" + json.dumps(command) + "\n")
            log.flush()
            result = subprocess.run(command, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT)
        if result.returncode:
            raise RuntimeError(f"experiment failed; inspect {output / 'run.log'}")
        print(f"Passed {suffix} / {backend}", flush=True)
        return target

    save()
    try:
        sizes = [100, 1000] if args.suite == "quick" else [1000, 10000, 100000, 1000000]
        for policy in ("players", "ties"):
            for n in sizes:
                tie_limit = 10 if args.suite == "quick" else 1000
                coordinates = max(2, n if policy == "players" else min(n, tie_limit))
                updates = 1000 if args.suite == "quick" else 1000000
                extra = ["--players", n, "--scores", coordinates, "--updates", updates,
                         "--batch", args.batch, "--score-policy", policy]
                paths = [execute(b, "queries", f"{policy}-{n}-queries", extra) for b in backends]
                if len(paths) == 2:
                    verify_pair(paths)
                percentages = [0, 50, 100] if args.suite == "quick" else [0, 1, 50, 99, 100]
                for percent in percentages:
                    paths = [execute(b, "seasons", f"{policy}-{n}-seasons-{percent}", extra + ["--high-percent", percent]) for b in backends]
                    if len(paths) == 2:
                        verify_pair(paths)
        if "avl" in backends and manifest["avl_tools"]:
            for n in sizes:
                for percent in [0, 1, 50, 99, 100]:
                    execute("avl", "ordered-join", f"ordered-{n}-{percent}", ["--players", n, "--high-percent", percent])
        for backend in backends:
            if manifest[f"{backend}_tools"]:
                for n in sizes:
                    execute(backend, "counters", f"counters-{n}", ["--players", n, "--scores", min(n, 1000)])
        for index, path in enumerate(args.trace):
            paths = [execute(b, "trace", f"trace-{index}", ["--trace", path.resolve(), "--check-trace", int(args.check_trace)]) for b in backends]
            if len(paths) == 2:
                verify_pair(paths)
        subprocess.run([sys.executable, str(ROOT / "results/plot.py"), str(output)], check=True)
        metadata["status"] = "complete"
        metadata["finished_utc"] = datetime.now(timezone.utc).isoformat()
        save()
        print(f"Saved results in {output}")
    except BaseException as error:
        metadata["status"] = "failed"
        metadata["error"] = str(error)
        save()
        raise


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, ValueError, OSError, subprocess.CalledProcessError) as error:
        print(f"EXPERIMENT FAILED: {error}", file=sys.stderr)
        sys.exit(1)
