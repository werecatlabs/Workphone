"""Profile the existing racing sample in serial, fresh processes.

Memory and CPU observations run separately. The sample's normal scene, resource
managers, physics and rendering validation remain active. These measurements do
not constitute an original-versus-optimized comparison or hardware-cache proof.
"""

import argparse
from datetime import datetime, timezone
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import shutil
import statistics
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
QUALITY = {"low": 0, "medium": 1, "high": 2, "cinematic": 3}
PHASES = {
    "process_bootstrap_before_type_manager", "runtime_before_scene_generation",
    "scene_generated", "scene_warmed_before_measurement",
    "measurement_end_before_readback", "application_unloaded", "type_manager_destroyed",
}
ASSET_SUFFIXES = {
    ".png", ".jpg", ".jpeg", ".tga", ".dds", ".hdr", ".mesh", ".fbscene",
    ".texres", ".matres", ".folspec", ".folworld", ".lua", ".cfg", ".hlsl",
}


def digest(path):
    value = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def save(path, value):
    path.write_text(json.dumps(value, indent=2, allow_nan=False) + "\n", encoding="utf-8")


def load(path):
    return json.loads(path.read_text(encoding="utf-8-sig"))


def git(*arguments):
    return subprocess.check_output(["git", "-C", str(ROOT), *arguments], text=True).strip()


def identities(paths):
    return {str(path.resolve()): digest(path) for path in sorted(set(paths)) if path.is_file()}


def source_identities():
    directories = [
        "Samples/cpp/VehicleAdvanced", "Engine/cpp/Include/WPGraphics",
        "Engine/cpp/Source/WPGraphics", "Engine/cpp/Include/WPProcedural",
        "Engine/cpp/Source/WPProcedural", "Engine/cpp/Include/Workphone/Graphics",
        "Engine/cpp/Source/Workphone/Graphics", "Engine/cpp/Include/Workphone/Scene",
        "Engine/cpp/Source/Workphone/Scene", "Engine/cpp/Include/Workphone/System",
        "Engine/cpp/Source/Workphone/System", "Engine/c/Include/WorkphoneGraphics",
        "Engine/cpp/Include/Workphone/Mesh", "Engine/cpp/Source/Workphone/Mesh",
        "Engine/c/Source/WorkphoneGraphics", "Engine/c/Include/WorkphonePlatformWin32",
        "Engine/c/Source/WorkphonePlatformWin32",
    ]
    suffixes = {".cpp", ".c", ".h", ".hpp", ".cmake", ".txt"}
    paths = [path for directory in directories for path in (ROOT / directory).rglob("*")
             if path.is_file() and path.suffix.lower() in suffixes]
    paths += [Path(__file__), ROOT / "Tests/cpp/MaterialHeapProfile.hpp",
              ROOT / "Tools/CollectMaterialPmu.ps1", ROOT / "Tools/MaterialScenePmu.wprp"]
    return identities(paths)


def snapshot_runtime(executable, plugins, output, copy_symbols):
    """Own copied modules so unrelated workspace rebuilds cannot replace a DLL."""
    runtime = output / "runtime"
    runtime.mkdir()
    paths = [executable, *executable.parent.glob("*.dll")]
    if copy_symbols:
        paths += [path for name in (executable.stem, "Workphone", "WPGraphics", "WPProcedural",
                                    "WPPhysics", "WPVehiclePhysics")
                  if (path := executable.parent / (name + ".pdb")).is_file()]
    original = identities(paths)
    for path in paths:
        shutil.copy2(path, runtime / path.name)
        if digest(runtime / path.name) != original[str(path.resolve())]:
            raise ValueError(f"module changed while copying: {path}")
    if identities(paths) != original:
        raise ValueError("runtime binaries changed during snapshot")
    text = plugins.read_text(encoding="utf-8-sig")
    lines = text.splitlines()
    folders = [index for index, line in enumerate(lines) if line.strip().startswith("PluginFolder=")]
    if len(folders) != 1:
        raise ValueError("sample plugin configuration must have one explicit PluginFolder")
    lines[folders[0]] = "PluginFolder=" + str(runtime.resolve())
    copied_plugins = runtime / "wp_plugins_profile.cfg"
    copied_plugins.write_text("\n".join(lines) + "\n", encoding="utf-8")
    modules = frozen_identities(runtime)
    return runtime / executable.name, copied_plugins, original, modules


def frozen_identities(runtime):
    return identities(path for path in runtime.iterdir()
                      if path.suffix.lower() in (".exe", ".dll", ".pdb")
                      or path.name == "wp_plugins_profile.cfg")


def finite(value, label, positive=False):
    if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value):
        raise ValueError(f"nonfinite {label}")
    if value < 0 or (positive and value == 0):
        raise ValueError(f"invalid negative/zero {label}")
    return value


def validate(profile, review, mode, view, pid, args, seconds):
    if profile.get("schema") != "workphone.production-scene-profile.v1":
        raise ValueError("unexpected scene profile schema")
    if profile.get("mode") != mode or profile.get("processId") != pid or profile.get("exitCode") != 0:
        raise ValueError("profile process/mode/completion identity mismatch")
    if profile.get("runCompleted") is not True or profile.get("profileInvariantsPassed") is not True:
        raise ValueError("scene profile did not complete its measurement invariants")
    if (profile.get("renderValidationPassed") is not True or profile.get("nativeFrameValidationPassed") is not True
            or review.get("validationPassed") is not True):
        raise ValueError("existing rendering validation did not pass")
    if (profile.get("hardwareCacheAcceptance") is not False or profile.get("cpuTimingAcceptance") is not False
            or profile.get("cpuTimingIncludesMemoryWalks") is not (mode == "memory")
            or profile.get("gpuResourceBytesMeasured") is not False
            or profile.get("materialOnlyAttribution") is not False):
        raise ValueError("profile contradicts the measurement/acceptance scope")
    if (profile.get("seed") != args.seed or profile.get("quality") != QUALITY[args.quality]
            or profile.get("view") != view or profile.get("width") != args.width
            or profile.get("height") != args.height or profile.get("activeWorkerThreads") != 0
            or profile.get("requestedSeconds") != seconds):
        raise ValueError("profile workload differs from requested scene")
    if profile.get("renderValidation") != review:
        raise ValueError("embedded rendering evidence differs from the separate review")
    if (review.get("seed") != args.seed or review.get("quality") != QUALITY[args.quality]
            or review.get("camera") != view or review.get("requestedWidth") != args.width
            or review.get("requestedHeight") != args.height):
        raise ValueError("render workload differs from requested scene")
    if (review.get("nativeUpdates", 0) <= 0 or review.get("nativeUpdates") != review.get("nativeRenders")
            or review.get("nativeCompletedPasses", 0) <= 0):
        raise ValueError("scene did not complete matching native update/render work")
    target = review.get("device", {}).get("captureTarget", {})
    if target.get("width") != args.width or target.get("height") != args.height:
        raise ValueError("actual native target dimensions differ from the requested workload")
    measured = finite(profile.get("measurementSeconds"), "measurement seconds", positive=True)
    if measured < seconds - 0.1:
        raise ValueError("measurement interval is shorter than requested")
    finite(profile.get("processKernelSeconds"), "kernel CPU seconds")
    finite(profile.get("processUserSeconds"), "user CPU seconds")
    if profile.get("renderOwnerUpdatesMeasured", 0) < 30 or profile.get("intervalsRetained", 0) < 29:
        raise ValueError("insufficient actual rendered updates")
    if profile.get("intervalsTruncated") is not False or profile.get("droppedMemorySamples") != 0:
        raise ValueError("bounded observation buffers overflowed")
    samples = profile.get("memorySamples", [])
    if missing := PHASES - {sample.get("phase") for sample in samples}:
        raise ValueError(f"missing scene lifecycle checkpoints: {sorted(missing)}")
    if mode == "cpu":
        if profile.get("heapWalksDuringMeasurement") != 0 or any(item.get("heapCaptured") is not False for item in samples):
            raise ValueError("CPU-only run performed an intrusive heap walk")
    else:
        if not all(item.get("heapCaptured") is True for item in samples):
            raise ValueError("memory run is missing an actual heap checkpoint")
        for item in samples:
            for name in ("heapBusyBytes", "privateCommitBytes", "processLifetimePeakPrivateCommitBytes"):
                if type(item.get(name)) is not int or item[name] < 0:
                    raise ValueError(f"invalid allocation counter: {name}")
        if not args.smoke and sum(item["phase"] == "periodic_warmed_scene" for item in samples) < 3:
            raise ValueError("memory run has insufficient periodic residency samples")
    workload = profile.get("workload", {})
    for name in ("uniqueMaterials", "uniqueTextures", "uniqueMeshes", "vertices", "indices", "meshSections"):
        finite(workload.get(name), "workload " + name, positive=True)
    if review.get("sceneMeshes", 0) <= 0 or review.get("sceneTextures", 0) <= 0:
        raise ValueError("scene lacks its actual resource graph")
    native = review.get("dx11", {})
    for name in ("frames", "draws", "triangles"):
        finite(native.get(name), "native " + name, positive=True)
    telemetry = profile.get("telemetry", {})
    if (telemetry.get("dx11Available") is not True or telemetry.get("failedPasses") != 0
            or telemetry.get("dx11PresentFailuresLifetime") != 0
            or telemetry.get("dx11PresentOccludedLifetime") != 0
            or telemetry.get("nativeUpdateSerial") != telemetry.get("nativeRenderSerial")):
        raise ValueError("typed telemetry reports unavailable or incomplete native work")
    for name in ("frames", "lastSerial", "nativeUpdateSerial", "completedPassesLastFrame",
                 "dx11FramesLifetime", "dx11DrawsLifetime", "dx11PresentAttemptsLifetime"):
        finite(telemetry.get(name), "telemetry " + name, positive=True)
    return measured


def slope(samples, value, frequency):
    if len(samples) < 2:
        return None
    xs = [(sample["qpc"] - samples[0]["qpc"]) / frequency for sample in samples]
    ys = [sample[value] for sample in samples]
    xmean, ymean = statistics.mean(xs), statistics.mean(ys)
    denominator = sum((x - xmean) ** 2 for x in xs)
    return (sum((x - xmean) * (y - ymean) for x, y in zip(xs, ys)) / denominator
            if denominator else None)


def summarize(profile, review):
    elapsed = profile["measurementSeconds"]
    cpu_seconds = profile["processUserSeconds"] + profile["processKernelSeconds"]
    result = {"wall_seconds": elapsed, "process_cpu_seconds_all_threads": cpu_seconds,
              "average_process_cpu_cores": cpu_seconds / elapsed,
              "render_owner_updates": profile["renderOwnerUpdatesMeasured"],
              "render_owner_interval_mean_ms": profile.get("renderOwnerUpdateMeanMs"),
              "render_owner_interval_p95_ms": profile.get("renderOwnerUpdateP95Last8192Ms"),
              "native_statistics": review["dx11"], "device": review.get("device"),
              "workload": profile["workload"], "telemetry": profile.get("telemetry"),
              "memory_timings_eligible": False,
              "cpu_timings_eligible": profile["mode"] == "cpu"}
    if profile["mode"] == "memory":
        phases = {item["phase"]: item for item in profile["memorySamples"]}
        steady = [item for item in profile["memorySamples"] if item["phase"] in
                  ("scene_warmed_before_measurement", "periodic_warmed_scene", "measurement_end_before_readback")]
        result["memory"] = {
            "steady_samples": len(steady),
            "heap_busy_min_bytes": min(item["heapBusyBytes"] for item in steady),
            "heap_busy_max_bytes": max(item["heapBusyBytes"] for item in steady),
            "private_commit_min_bytes": min(item["privateCommitBytes"] for item in steady),
            "private_commit_max_bytes": max(item["privateCommitBytes"] for item in steady),
            "checkpoint_heap_peak_bytes": max(item["heapBusyBytes"] for item in profile["memorySamples"]),
            "process_lifetime_peak_private_commit_bytes": max(item["processLifetimePeakPrivateCommitBytes"] for item in profile["memorySamples"]),
            "private_commit_slope_bytes_per_second": slope(steady, "privateCommitBytes", profile["qpcFrequency"]),
            "heap_busy_slope_bytes_per_second": slope(steady, "heapBusyBytes", profile["qpcFrequency"]),
            "bootstrap": phases["process_bootstrap_before_type_manager"],
            "generated": phases["scene_generated"],
            "warmed": phases["scene_warmed_before_measurement"],
            "measurement_end": phases["measurement_end_before_readback"],
            "application_unloaded": phases["application_unloaded"],
            "type_manager_destroyed": phases["type_manager_destroyed"],
            "slope_is_leak_certification": False,
        }
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, default=ROOT / "bin/windows/v145/x64/MD/RelWithDebInfo/SampleVehicleAdvanced.exe")
    parser.add_argument("--plugins", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--pmu-preflight", type=Path, required=True)
    parser.add_argument("--views", nargs="+", choices=("car", "track", "follow", "corner"), default=["car", "track", "follow"])
    parser.add_argument("--quality", choices=QUALITY, default="high")
    parser.add_argument("--resolution", default="1280x720")
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--cpu-seconds", type=float, default=30)
    parser.add_argument("--memory-seconds", type=float, default=60)
    parser.add_argument("--cpu-repetitions", type=int, default=3)
    parser.add_argument("--memory-repetitions", type=int, default=1)
    parser.add_argument("--timeout-seconds", type=float, default=600)
    parser.add_argument("--no-copy-symbols", action="store_true")
    parser.add_argument("--smoke", action="store_true")
    args = parser.parse_args()
    try:
        args.width, args.height = map(int, args.resolution.split("x"))
    except ValueError:
        parser.error("resolution must be WIDTHxHEIGHT")
    if (min(args.width, args.height, args.cpu_repetitions, args.memory_repetitions) < 1
            or not all(math.isfinite(value) and value > 0 for value in
                       (args.cpu_seconds, args.memory_seconds, args.timeout_seconds))
            or not 0 <= args.seed <= 0xffffffff):
        parser.error("positive dimensions, durations and repetition counts are required")
    if not args.smoke and (args.cpu_seconds < 30 or args.memory_seconds < 30 or args.cpu_repetitions < 3):
        parser.error("baseline protocol needs >=30s intervals and >=3 CPU repetitions; use --smoke for harness checks")
    if not args.smoke and len(set(args.views)) < 2:
        parser.error("baseline protocol needs at least two scene views; use --smoke for harness checks")
    executable = args.exe.resolve()
    plugins = (args.plugins or executable.parent / "wp_plugins_samples.cfg").resolve()
    for path in (executable, plugins, args.pmu_preflight):
        if not path.is_file():
            parser.error(f"missing input: {path}")
    for name in ("Workphone.dll", "WPGraphics.dll", "WPPhysics.dll", "WPProcedural.dll"):
        if not (executable.parent / name).is_file():
            parser.error(f"missing local runtime module: {name}")
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    preflight = load(args.pmu_preflight)
    report = {"schema": "workphone.production-scene-profile-set.v1", "status": "RUNNING",
              "started_utc": datetime.now(timezone.utc).isoformat(), "platform": platform.platform(),
              "git_head_before": git("rev-parse", "HEAD"), "git_status_before": git("status", "--short"),
              "pmu_preflight": str(args.pmu_preflight.resolve()), "pmu_preflight_sha256": digest(args.pmu_preflight),
              "hardware_cache_validation": "not_collected" if preflight.get("hardware_counters_available") else "unavailable",
              "hardware_cache_blockers": preflight.get("blockers", []),
              "production_performance_acceptance": False, "original_vs_optimized_comparison": False,
              "smoke": args.smoke, "runner_concurrent_sample_processes": 1,
              "background_workloads_controlled": False, "runs": [],
              "limitations": ["Existing configured procedural racing sample; not a shipped asset-scene certification",
                              "Separate intrusive memory and heap-walk-free CPU processes",
                              "CPU intervals include guest scheduling and Present; software GPU work contributes process CPU",
                              "Background OS and unrelated workspace activity is uncontrolled; copied modules preserve runtime identity",
                              "No original scene baseline, equal-coverage comparison, cache-miss claim or leak-soak certification",
                              "Media inventory is not a runtime dependency trace; GPU allocations are not added to CPU totals"]}
    manifest = output / "manifest.json"
    save(manifest, report)
    try:
        subprocess.run(["git", "-C", str(ROOT), "diff", "--binary", "HEAD", "--output=" + str(output / "source.patch")], check=True)
        report["source_hashes_before"] = source_identities()
        # A tracked patch cannot recover newly added profiling sources.
        tracked = set(git("ls-files", "-z").split("\0"))
        report["archived_untracked_sources"] = {}
        for name, expected_hash in report["source_hashes_before"].items():
            path = Path(name)
            relative = path.relative_to(ROOT)
            if relative.as_posix() not in tracked:
                archived = output / "source_untracked" / relative
                archived.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(path, archived)
                if digest(archived) != expected_hash:
                    raise ValueError(f"untracked source changed while archiving: {path}")
                report["archived_untracked_sources"][relative.as_posix()] = expected_hash
        assets = [path for path in (ROOT / "bin/Media").rglob("*")
                  if path.is_file() and path.suffix.lower() in ASSET_SUFFIXES and ".workphone" not in path.parts]
        report["asset_hashes"] = identities(assets)
        copied_exe, copied_plugins, original, modules = snapshot_runtime(executable, plugins, output, not args.no_copy_symbols)
        report["original_module_hashes"] = original
        report["frozen_module_hashes"] = modules
        report["working_directory"] = str(executable.parent)
        save(manifest, report)
        tasks = [("memory", repetition, view, args.memory_seconds)
                 for repetition in range(1, args.memory_repetitions + 1) for view in dict.fromkeys(args.views)]
        tasks += [("cpu", repetition, view, args.cpu_seconds)
                  for repetition in range(1, args.cpu_repetitions + 1)
                  for view in (list(dict.fromkeys(args.views)) if repetition % 2 else list(reversed(list(dict.fromkeys(args.views)))))]
        workloads = {}
        for mode, repetition, view, seconds in tasks:
            name = f"{mode}_{view}_sample{repetition}"
            run_directory = output / name
            run_directory.mkdir()
            if frozen_identities(copied_exe.parent) != modules:
                raise ValueError("frozen runtime files changed")
            profile_path, review_path = run_directory / "profile.json", run_directory / "render.json"
            command = [str(copied_exe), "--plugins", str(copied_plugins), "--seed", str(args.seed),
                       "--quality", args.quality, "--view", view, "--resolution", args.resolution,
                       "--no-hud", "--no-audio", "--benchmark", str(seconds),
                       "--capture", str(run_directory / "scene.bmp"), "--evidence", str(review_path),
                       "--scene-profile", str(profile_path), "--scene-profile-mode", mode]
            environment = dict(os.environ)
            environment["PATH"] = str(copied_exe.parent) + os.pathsep + environment.get("PATH", "")
            print(f"Starting {name}: {seconds:g}s after rendered warmup", flush=True)
            with (run_directory / "stdout.log").open("w", encoding="utf-8") as out, (run_directory / "stderr.log").open("w", encoding="utf-8") as err:
                child = subprocess.Popen(command, cwd=executable.parent, env=environment, stdout=out, stderr=err)
                try:
                    status = child.wait(timeout=args.timeout_seconds)
                except subprocess.TimeoutExpired:
                    child.kill()
                    child.wait()
                    raise RuntimeError(f"owned sample process timed out: {name}")
            if status:
                raise RuntimeError(f"{name} exited {status}; inspect its preserved logs/profile")
            if frozen_identities(copied_exe.parent) != modules:
                raise ValueError("frozen runtime files changed while measuring")
            profile, review = load(profile_path), load(review_path)
            validate(profile, review, mode, view, child.pid, args, seconds)
            if not (run_directory / "scene.bmp").is_file():
                raise ValueError("native frame capture missing")
            workload = profile["workload"]
            if view in workloads and workloads[view] != workload:
                raise ValueError(f"actual geometry/resource workload changed between {view} repetitions")
            workloads[view] = workload
            run = {"name": name, "mode": mode, "view": view, "repetition": repetition,
                   "process_id": child.pid, "command": command, "exit_code": status,
                   "profile": str(profile_path), "profile_sha256": digest(profile_path),
                   "review": str(review_path), "review_sha256": digest(review_path),
                   "capture_sha256": digest(run_directory / "scene.bmp"),
                   "summary": summarize(profile, review)}
            report["runs"].append(run)
            save(manifest, report)
            print(f"Completed {name}", flush=True)
        report["source_hashes_after"] = source_identities()
        report["source_changed_during_run"] = report["source_hashes_before"] != report["source_hashes_after"]
        report["asset_hashes_match_after"] = identities(assets) == report["asset_hashes"]
        if not report["asset_hashes_match_after"]:
            raise ValueError("inventoried scene inputs changed during the run set")
        report["cpu_view_summary"] = []
        for view in dict.fromkeys(args.views):
            selected = [run["summary"] for run in report["runs"] if run["mode"] == "cpu" and run["view"] == view]
            report["cpu_view_summary"].append({"view": view, "repetitions": len(selected),
                "median_interval_p95_ms": statistics.median(item["render_owner_interval_p95_ms"] for item in selected),
                "interval_p95_range_ms": [min(item["render_owner_interval_p95_ms"] for item in selected), max(item["render_owner_interval_p95_ms"] for item in selected)],
                "median_process_cpu_cores": statistics.median(item["average_process_cpu_cores"] for item in selected)})
        report["status"] = "COMPLETE"
        report["baseline_protocol_completed"] = not args.smoke
        print("Scene profile set completed; hardware/performance acceptance remains separate.", flush=True)
        return 0
    except Exception as error:
        report["status"] = "FAILED"
        report["error"] = str(error)
        print(str(error), file=sys.stderr, flush=True)
        return 1
    finally:
        report["completed_utc"] = datetime.now(timezone.utc).isoformat()
        save(manifest, report)


if __name__ == "__main__":
    raise SystemExit(main())
