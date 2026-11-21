"""Build against recorded teammate snapshots or an integrated checkout."""

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build" / "bench"


def run(command, **kwargs):
    return subprocess.run([str(item) for item in command], cwd=ROOT, check=True, **kwargs)


def git(*args):
    return run(["git", *args], capture_output=True).stdout


def export(ref, prefix, destination):
    files = git("ls-tree", "-r", "--name-only", ref, "--", prefix).decode().splitlines()
    for name in files:
        if Path(name).suffix not in {".cpp", ".hpp", ".h", ".md", ".trace", ".expected"}:
            continue
        path = destination / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(git("show", f"{ref}:{name}"))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", default="g++")
    parser.add_argument("--backend", choices=["both", "avl", "fenwick"], default="both")
    parser.add_argument("--local", action="store_true", help="Use the integrated source tree instead of Git branch snapshots")
    parser.add_argument("--architect-ref", default="origin/madhav")
    parser.add_argument("--avl-ref", default="origin/prince-chavda")
    parser.add_argument("--fenwick-ref", default="origin/Saksham")
    parser.add_argument("--oracle-ref", default="origin/om-oracle")
    parser.add_argument("--avl-source", help="Relative path of the shared-interface AVL source")
    parser.add_argument("--fenwick-source", default="src/structB/fenwick.cpp")
    parser.add_argument("--without-oracle", action="store_true", help="Development builds only; retain independent sorted-snapshot checks")
    parser.add_argument("--avl-tools", action="store_true", help="Require the team's ordered join, audit and statistics helpers")
    parser.add_argument("--fenwick-tools", action="store_true", help="Require the team's Fenwick statistics helper")
    args = parser.parse_args()
    compiler = shutil.which(args.compiler)
    if compiler is None:
        raise RuntimeError(f"compiler not found: {args.compiler}")
    BUILD.mkdir(parents=True, exist_ok=True)
    binary = BUILD / ("runner.exe" if sys.platform == "win32" else "runner")
    manifest = BUILD / "build.json"
    # A failed rebuild must not leave an older executable looking current.
    binary.unlink(missing_ok=True)
    manifest.unlink(missing_ok=True)
    dependencies = {}
    source_root = ROOT if args.local else BUILD / "deps"
    if args.local:
        dependencies["integrated_commit"] = git("rev-parse", "HEAD").decode().strip()
        dependencies["integrated_status"] = git("status", "--porcelain").decode()
        if not (source_root / "include/backend_tools.hpp").exists() and (args.avl_tools or args.fenwick_tools):
            raise RuntimeError("integrated tree lacks include/backend_tools.hpp")
    else:
        # Export exact immutable commits without switching branches or editing sources.
        refs = {"architect": args.architect_ref}
        if args.backend != "fenwick":
            refs["avl"] = args.avl_ref
        if args.backend != "avl":
            refs["fenwick"] = args.fenwick_ref
        if not args.without_oracle:
            refs["oracle"] = args.oracle_ref
        commits = {name: git("rev-parse", f"{ref}^{{commit}}").decode().strip()
                   for name, ref in refs.items()}
        # Separate exports by snapshot hash to avoid stale dependency files after rebuilds.
        key = hashlib.sha256(json.dumps(commits, sort_keys=True).encode()).hexdigest()[:16]
        source_root = source_root / key
        source_root.mkdir(parents=True, exist_ok=True)
        export(commits["architect"], "include/", source_root)
        export(commits["architect"], "app/", source_root)
        for name in ("avl", "fenwick"):
            if name not in commits:
                continue
            target = "src/structA/" if name == "avl" else "src/structB/"
            export(commits[name], target, source_root)
            export(commits[name], "avl.cpp" if name == "avl" else "Fenvick.cpp", source_root)
            # Shared header must match the Architect; additional helper headers can be supplied.
            paths = git("ls-tree", "-r", "--name-only", commits[name], "--", "include/").decode().splitlines()
            for path in paths:
                if Path(path).suffix not in {".h", ".hpp"}:
                    continue
                content = git("show", f"{commits[name]}:{path}")
                output = source_root / path
                if path == "include/leaderboard.hpp" and content != output.read_bytes():
                    raise RuntimeError(f"{name} shared header differs from Architect; agree the interface before building")
                if output.exists() and output.read_bytes() != content:
                    raise RuntimeError(f"conflicting dependency header: {path}")
                output.parent.mkdir(parents=True, exist_ok=True)
                output.write_bytes(content)
        if "oracle" in commits:
            # The oracle must compile against the Architect's header, not its historical copy.
            export(commits["oracle"], "oracle/", source_root)
        dependencies = {name: {"ref": refs[name], "commit": commit} for name, commit in commits.items()}

    defines = []
    sources = [ROOT / "bench/runner.cpp", source_root / "app/replay.cpp", source_root / "app/player_table.cpp"]
    if args.backend != "fenwick":
        avl_source = args.avl_source
        if not avl_source:
            avl_source = "src/structA/avl.cpp" if (source_root / "src/structA/avl.cpp").exists() else "avl.cpp"
        sources.append(source_root / avl_source)
        defines.append("-DBENCH_HAS_AVL")
    if args.backend != "avl":
        sources.append(source_root / args.fenwick_source)
        defines.append("-DBENCH_HAS_FENWICK")
    if not args.without_oracle:
        defines.append("-DBENCH_ORACLE")
    if args.avl_tools:
        defines.append("-DBENCH_AVL_TOOLS")
    if args.fenwick_tools:
        defines.append("-DBENCH_FENWICK_TOOLS")
    command = [compiler, "-std=c++17", "-O2", "-g", "-Wall", "-Wextra", "-Wpedantic", *defines,
               "-I", source_root / "include", "-I", source_root / "app", "-I", source_root / "oracle",
               *sources, "-o", binary]
    missing = [str(path) for path in sources if not path.is_file()]
    if missing:
        raise RuntimeError("missing source files: " + ", ".join(missing))
    run(command)
    # Even a successful object compile is insufficient; the command above links all factories.
    run([binary, "--help"])
    files = set(sources)
    files.add(ROOT / "bench/support.hpp")
    for folder in ("include", "app", "oracle", "src/structA", "src/structB"):
        for path in (source_root / folder).rglob("*"):
            if path.suffix in {".h", ".hpp"}:
                files.add(path)
    manifest.write_text(json.dumps({
        "binary": str(binary), "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
        "source_root": str(source_root),
        "compiler": run([compiler, "--version"], capture_output=True, text=True).stdout,
        "command": [str(item) for item in command], "dependencies": dependencies,
        "backend": args.backend, "oracle": not args.without_oracle,
        "avl_tools": args.avl_tools, "fenwick_tools": args.fenwick_tools,
        "source_sha256": {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest() for path in sorted(files)},
    }, indent=2), encoding="utf-8")
    print(f"Build and dependency record saved in {manifest.relative_to(ROOT)}")


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, subprocess.CalledProcessError, OSError) as error:
        print(f"BUILD FAILED: {error}", file=sys.stderr)
        print("Check teammate interface compatibility and factory/helper names. Sources were exported unchanged.", file=sys.stderr)
        sys.exit(1)
