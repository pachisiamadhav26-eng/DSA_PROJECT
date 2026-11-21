"""Record hardware-event samples around a prepared query phase using Windows WPR."""

import argparse
import ctypes
from datetime import datetime, timezone
import json
from pathlib import Path
import subprocess
import sys
import time

from run import BUILD, ROOT, load_build, machine_info


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--backend", required=True, choices=["avl", "fenwick"])
    parser.add_argument("--operation", choices=["rank", "select"], default="rank")
    parser.add_argument("--players", type=int, default=100000)
    parser.add_argument("--scores", type=int, default=100000)
    parser.add_argument("--seed", type=int, default=253)
    parser.add_argument("--seconds", type=int, default=10)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if sys.platform != "win32" or not ctypes.windll.shell32.IsUserAnAdmin():
        raise RuntimeError("run this script from an Administrator PowerShell on Windows to collect hardware events")
    manifest, binary = load_build()
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    output = (args.output or ROOT / "results" / f"profile-{args.backend}-{args.operation}-{stamp}").resolve()
    output.mkdir(parents=True, exist_ok=False)
    control = BUILD / f"profile-control-{stamp}"
    control.mkdir()
    profile = output / "hardware.wprp"
    profile.write_text('''<?xml version="1.0" encoding="utf-8"?>
<WindowsPerformanceRecorder Version="1.0" Author="Leaderboard Analyst">
 <Profiles>
  <SystemCollector Id="Collector" Name="Leaderboard hardware samples">
   <BufferSize Value="1024"/><Buffers Value="128"/>
  </SystemCollector>
  <SystemProvider Id="Provider">
   <Keywords><Keyword Value="ProcessThread"/><Keyword Value="Loader"/><Keyword Value="PmcProfile"/></Keywords>
  </SystemProvider>
  <HardwareCounter Id="Counters" Strict="true">
   <SampledCounters><SampledCounter Value="LLCMisses" Interval="65536"/></SampledCounters>
  </HardwareCounter>
  <Profile Id="Leaderboard.Verbose.File" Name="Leaderboard" Description="Query-phase LLC miss samples" LoggingMode="File" DetailLevel="Verbose">
   <Collectors><SystemCollectorId Value="Collector"><SystemProviderId Value="Provider"/><HardwareCounterId Value="Counters"/></SystemCollectorId></Collectors>
  </Profile>
 </Profiles>
</WindowsPerformanceRecorder>
''', encoding="utf-8")
    sources = subprocess.run(["wpr", "-pmcsources"], capture_output=True, text=True, check=True)
    (output / "available-events.txt").write_text(sources.stdout + sources.stderr, encoding="utf-8")
    if "LLCMisses" not in sources.stdout:
        raise RuntimeError("LLCMisses is not exposed on this machine; select a supported event and record its meaning")
    command = [str(binary), "--backend", args.backend, "--experiment", "profile", "--players", str(args.players),
               "--scores", str(args.scores), "--seed", str(args.seed), "--seconds", str(args.seconds),
               "--operation", args.operation, "--control-dir", str(control)]
    metadata = {"status": "running", "build": manifest, "command": command,
                "machine": machine_info(),
                "event": "LLCMisses", "sampling_interval": 65536,
                "interpretation": "Hardware-event samples, not an exact miss count. Analyze benchmark PID in WPA; record lost events and timer-loop overhead."}

    def save():
        (output / "profile.json").write_text(json.dumps(metadata, indent=2), encoding="utf-8")

    def wait(name, limit):
        deadline = time.monotonic() + limit
        while not (control / name).exists():
            if process.poll() is not None:
                raise RuntimeError(f"benchmark exited before {name}; inspect benchmark.txt")
            if time.monotonic() > deadline:
                raise RuntimeError(f"timed out waiting for {name}")
            time.sleep(0.05)

    save()
    started = False
    with (output / "benchmark.txt").open("w", encoding="utf-8") as log:
        process = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT, cwd=ROOT)
        metadata["benchmark_pid"] = process.pid
        save()
        try:
            wait("ready", 600)
            result = subprocess.run(["wpr", "-start", f"{profile}!Leaderboard", "-filemode"], capture_output=True, text=True)
            (output / "wpr-start.txt").write_text(result.stdout + result.stderr, encoding="utf-8")
            if result.returncode:
                raise RuntimeError("WPR could not start the hardware profile; inspect wpr-start.txt. No collection was verified.")
            started = True
            (control / "go").touch()
            wait("done", args.seconds + 60)
            result = subprocess.run(["wpr", "-stop", str(output / "hardware.etl")], capture_output=True, text=True)
            (output / "wpr-stop.txt").write_text(result.stdout + result.stderr, encoding="utf-8")
            if result.returncode:
                raise RuntimeError("WPR failed to save the trace; inspect wpr-stop.txt")
            started = False
            (control / "release").touch()
            if process.wait(timeout=30):
                raise RuntimeError("profile benchmark failed")
            metadata["status"] = "captured_requires_WPA_review"
            save()
        except BaseException as error:
            metadata["status"] = "failed"
            metadata["error"] = str(error)
            save()
            raise
        finally:
            if started:
                # Only stop a session successfully started by this invocation.
                subprocess.run(["wpr", "-stop", str(output / "incomplete.etl")], capture_output=True)
            (control / "release").touch()
            (control / "go").touch()
            if process.poll() is None:
                process.terminate()
                process.wait(timeout=30)
    print(f"Saved {output / 'hardware.etl'}. Open in WPA, filter to PID {metadata['benchmark_pid']}, and export the PMU analysis.")


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, OSError, subprocess.CalledProcessError) as error:
        print(f"PROFILE FAILED: {error}", file=sys.stderr)
        sys.exit(1)
