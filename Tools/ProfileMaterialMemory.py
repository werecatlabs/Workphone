"""Compare fresh-process material allocation profiles from two engine revisions.

Run the same test-only probe against real baseline and optimized DLLs. No frozen
sizeof reference or process working-set estimate is used for the allocation gate.
"""

import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import hashlib
import json
import os
from pathlib import Path
import platform
import statistics
import subprocess
import sys


PREFIX = "full_material_memory "
PRIMARY = "loaded_materials_queue_drained"
METRICS = {
    "crt_busy_including_reported_overhead": "heap_busy_delta_from_before_schema_bytes",
    "process_private_commit": "private_commit_delta_from_before_schema_bytes",
    "whole_fixture_crt_busy_including_shared_startup": "heap_busy_delta_from_bootstrap_bytes",
    "whole_fixture_private_commit_including_shared_startup": "private_commit_delta_from_bootstrap_bytes",
}
COMMON_PHASES = {"process_bootstrap", "fixture_ready_before_inputs", "before_schema",
                 "shared_schema_and_authored_source", "loaded_materials_no_views", PRIMARY,
                 "fixture_mesh_inputs_added", "inputs_and_local_schema_destroyed_registry_retained",
                 "fixture_inputs_released", "fixture_destroyed_registry_retained"}
BASELINE_PHASES = {"one_percent_edited_no_views", "all_edited_no_views", "graphs_unloaded"}
OPTIMIZED_PHASES = {"zero_percent_two_views", "one_percent_pending_two_views",
                    "one_percent_published_old_view_retained", "all_pending_two_old_views",
                    "all_published_two_old_views", "all_updated_three_views", "oldest_view_released",
                    "intermediate_view_released", "all_views_cleared_graphs_alive",
                    "graphs_unloaded_cache_capacity_retained", "views_and_record_cache_destroyed"}


def digest(path):
    value = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def scalar(value):
    try:
        return int(value)
    except ValueError:
        try:
            return float(value)
        except ValueError:
            return value


def phases(path):
    result = {}
    with path.open(encoding="utf-8", errors="replace") as stream:
        for line in stream:
            if not line.startswith(PREFIX):
                continue
            row = dict((key, scalar(value)) for key, value in
                       (field.split("=", 1) for field in line.split()[1:]))
            phase = row["phase"]
            if phase in result:
                raise ValueError(f"duplicate profile phase {phase}: {path}")
            result[phase] = row
    if PRIMARY not in result or "fixture_destroyed_registry_retained" not in result:
        raise ValueError(f"incomplete material profile: {path}")
    return result


def provenance(executable):
    modules = [executable]
    modules.extend(executable.parent.glob("*.dll"))
    return {str(path): digest(path) for path in sorted(modules) if path.is_file()}


def run_probe(executable, output, name, count, width, kind, frozen_modules, timeout):
    if provenance(executable) != frozen_modules:
        raise ValueError(f"profile binaries changed before {name}")
    stdout = output / f"{name}.stdout.log"
    stderr = output / f"{name}.stderr.log"
    command = [str(executable), "--profile-full-memory", str(count), str(width)]
    environment = dict(os.environ)
    environment["PATH"] = str(executable.parent) + os.pathsep + environment.get("PATH", "")
    with stdout.open("w", encoding="utf-8") as out, stderr.open("w", encoding="utf-8") as err:
        status = subprocess.run(command, cwd=executable.parent, env=environment,
                                stdout=out, stderr=err, check=False, timeout=timeout)
    if status.returncode:
        raise RuntimeError(f"{name} exited {status.returncode}; inspect {stderr}")
    if provenance(executable) != frozen_modules:
        raise ValueError(f"profile binaries changed during {name}")
    rows = phases(stdout)
    required = COMMON_PHASES | (BASELINE_PHASES if kind == "baseline" else OPTIMIZED_PHASES)
    if missing := required - rows.keys():
        raise ValueError(f"missing lifecycle phases {sorted(missing)}: {stdout}")
    expected = "baseline957" if kind == "baseline" else "optimized"
    for row in rows.values():
        if (row["implementation"] != expected or row["materials"] != count or row["parameters"] != width
                or row["schema"] != "mixed_scalar_colour_matrix_texture" or row["names"] != "ValueN"
                or row["source_prototype_included"] != 1):
            raise ValueError(f"inconsistent implementation/workload metadata: {stdout}")
        for field, value in row.items():
            if ("bytes" in field or field.endswith("_count") or field == "heap_busy_blocks"):
                if type(value) is not int:
                    raise ValueError(f"allocation counters must be finite integers: {stdout}, {field}")
    primary = rows[PRIMARY]
    if primary["materials"] != count or primary["parameters"] != width:
        raise ValueError(f"profile dimensions differ from requested workload: {stdout}")
    if primary["live_materials"] != count or primary["checksum"] != count * 1.25:
        raise ValueError(f"loaded material topology/value checksum mismatch: {stdout}")
    if (primary["graphics_queue_count"] != 0 or primary["materialized_texture_facades"] !=
            (count * 15 if kind == "baseline" else 0) or
            primary["distinct_value_payloads_including_prototype"] != (count + 1 if kind == "baseline" else 1)):
        raise ValueError(f"material topology/sharing does not match the acceptance workload: {stdout}")
    for absolute, emitted in (("heap_busy_bytes", "heap_busy_delta_from_before_schema_bytes"),
                              ("private_commit_bytes", "private_commit_delta_from_before_schema_bytes")):
        if primary[absolute] - rows["before_schema"][absolute] != primary[emitted]:
            raise ValueError(f"inconsistent measured baseline subtraction: {stdout}")
    sparse_phase = "one_percent_edited_no_views" if kind == "baseline" else "one_percent_published_old_view_retained"
    final_phase = "all_edited_no_views" if kind == "baseline" else "all_updated_three_views"
    if (rows[sparse_phase]["checksum"] != count * 1.25 - 0.75 * max(1, count // 100)
            or rows[final_phase]["checksum"] != count * 1.25):
        raise ValueError(f"edited material values differ from the acceptance workload: {stdout}")
    if kind == "optimized":
        surface_bytes = primary["current_snapshot_bytes"] // count
        if (surface_bytes <= 0 or rows["all_pending_two_old_views"]["pending_surface_bytes"] != count * surface_bytes
                or rows[final_phase]["retired_snapshot_bytes"] != (count + max(1, count // 100)) * surface_bytes
                or rows[final_phase]["resident_prepared_records"] != count * 2 + max(1, count // 100)
                or rows["graphs_unloaded_cache_capacity_retained"]["resident_prepared_records"] != 0):
            raise ValueError(f"pending/retired allocation lifecycle mismatch: {stdout}")
    bootstrap = rows["process_bootstrap"]
    primary["heap_busy_delta_from_bootstrap_bytes"] = primary["heap_busy_bytes"] - bootstrap["heap_busy_bytes"]
    primary["private_commit_delta_from_bootstrap_bytes"] = primary["private_commit_bytes"] - bootstrap["private_commit_bytes"]
    return {"name": name, "command": command, "stdout": str(stdout), "stderr": str(stderr),
            "phases": rows}


def compare(runs, count, widths, target):
    comparisons = []
    for width in widths:
        selected = {kind: [run for run in runs if run["kind"] == kind and run["width"] == width]
                    for kind in ("baseline", "optimized")}
        baseline_primary = selected["baseline"][0]["phases"][PRIMARY]
        optimized_primary = selected["optimized"][0]["phases"][PRIMARY]
        if (baseline_primary["schema"], baseline_primary["names"]) != (
                optimized_primary["schema"], optimized_primary["names"]):
            raise ValueError("before/after schema and parameter names differ")
        if baseline_primary["engine_memory_pool_enabled"] != optimized_primary["engine_memory_pool_enabled"]:
            raise ValueError("before/after engine allocator configuration differs")
        for label, field in METRICS.items():
            values = {kind: [run["phases"][PRIMARY][field] for run in selected[kind]]
                      for kind in selected}
            if min(values["baseline"]) <= 0 or min(values["optimized"]) <= 0:
                raise ValueError("allocation-gate increments must be strictly positive")
            old, new = statistics.median(values["baseline"]), statistics.median(values["optimized"])
            reduction = 100.0 * (1.0 - new / old)
            conservative = 100.0 * (1.0 - max(values["optimized"]) / min(values["baseline"]))
            comparisons.append({"materials": count, "parameters": width, "metric": label,
                                "baseline_samples_bytes": values["baseline"],
                                "optimized_samples_bytes": values["optimized"],
                                "baseline_median_bytes": old, "optimized_median_bytes": new,
                                "median_reduction_percent": reduction,
                                "worst_observed_reduction_percent": conservative,
                                "target_reduction_percent": target,
                                "passed": conservative >= target})
    return comparisons


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline-exe", type=Path, required=True)
    parser.add_argument("--optimized-exe", type=Path, required=True)
    parser.add_argument("--baseline-ref", required=True)
    parser.add_argument("--optimized-ref", required=True,
                        help="Include a working-diff identity when optimized sources are uncommitted")
    parser.add_argument("--output", type=Path, required=True,
                        help="New evidence directory; existing results are never overwritten")
    parser.add_argument("--count", type=int, default=10000)
    parser.add_argument("--parameters", type=int, nargs="+", choices=(8, 32, 128), default=[8, 32, 128])
    parser.add_argument("--repetitions", type=int, default=3)
    parser.add_argument("--jobs", type=int, default=1,
                        help="Independent profiling processes; process-private metrics remain separate, working sets can vary")
    parser.add_argument("--timeout-seconds", type=int, default=600)
    parser.add_argument("--target-percent", type=float, default=50.0)
    args = parser.parse_args()
    if args.count < 1 or args.repetitions < 1 or args.jobs < 1 or args.timeout_seconds < 1 or not 0 < args.target_percent < 100:
        parser.error("count/repetitions/jobs must be positive and target percent must be between 0 and 100")
    before, after = args.baseline_exe.resolve(), args.optimized_exe.resolve()
    if not before.is_file() or not after.is_file() or before.parent == after.parent:
        parser.error("baseline and optimized executables must exist in separate binary directories")
    for executable in (before, after):
        for library in ("Workphone.dll", "WPGraphics.dll", "WPSQLite.dll"):
            if not (executable.parent / library).is_file():
                parser.error(f"profile must have isolated local engine libraries: {executable.parent / library}")
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    report = {"platform": platform.platform(), "python": sys.version,
              "baseline_ref": args.baseline_ref, "optimized_ref": args.optimized_ref,
              "baseline_modules_sha256": provenance(before), "optimized_modules_sha256": provenance(after),
              "concurrent_process_limit": args.jobs,
              "plan_acceptance_workload": args.count == 10000 and set(args.parameters) == {8, 32, 128}
                  and args.repetitions >= 3 and args.target_percent >= 50,
              "primary_phase": PRIMARY,
              "scope": "fresh-process graph growth from before shared schema; all CRT busy blocks and all process private commitment",
              "limitations": ["CRT busy bytes exclude free heap-manager regions; process private commitment covers those separately",
                              "heap peaks are checkpoint samples; process peaks cover the process lifetime",
                              "current immutable views have no fabricated baseline equivalent",
                              "graphics fixture allocates no GPU resources; shared startup and mesh-input phases reported separately"],
              "runs": []}
    (output / "profiles.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    try:
        scheduled = []
        for repetition in range(args.repetitions):
            for width in dict.fromkeys(args.parameters):
                order = ("baseline", "optimized") if repetition % 2 == 0 else ("optimized", "baseline")
                for kind in order:
                    name = f"{kind}_{args.count}x{width}_sample{repetition + 1}"
                    scheduled.append((name, kind, width, repetition + 1))
        with ThreadPoolExecutor(max_workers=args.jobs) as workers:
            futures = {}
            for name, kind, width, repetition in scheduled:
                future = workers.submit(run_probe, before if kind == "baseline" else after,
                                        output, name, args.count, width, kind,
                                        report[f"{kind}_modules_sha256"], args.timeout_seconds)
                futures[future] = (name, kind, width, repetition)
            for future in as_completed(futures):
                name, kind, width, repetition = futures[future]
                try:
                    run = future.result()
                except Exception:
                    for pending in futures:
                        pending.cancel()
                    raise
                print(f"Completed {name}", flush=True)
                run.update(kind=kind, width=width, repetition=repetition)
                report["runs"].append(run)
                (output / "profiles.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
        report["comparisons"] = compare(report["runs"], args.count, dict.fromkeys(args.parameters), args.target_percent)
        report["target_passed"] = all(row["passed"] for row in report["comparisons"])
        report["plan_memory_gate_passed"] = report["plan_acceptance_workload"] and report["target_passed"]
        (output / "profiles.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
        for row in report["comparisons"]:
            print(f"{row['parameters']} slots / {row['metric']}: "
                  f"{row['median_reduction_percent']:.2f}% median reduction, "
                  f"{row['worst_observed_reduction_percent']:.2f}% worst observed / "
                  f"{'PASS' if row['passed'] else 'TARGET NOT MET'}", flush=True)
        return 0 if report["target_passed"] else 2
    except Exception as error:
        report["error"] = str(error)
        (output / "profiles.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
        raise


if __name__ == "__main__":
    sys.exit(main())
