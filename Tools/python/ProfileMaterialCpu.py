"""Run the same real-adapter CPU fixture against original and current DLLs.

The measurement includes actual DX11 submission with translated geometry intended
to clip. Rasterized pixels are unmeasured. It does not isolate
material-only CPU, GPU execution, Present, or representative whole-frame p95.
Each engine is frozen to a separate runtime before sequential, rotating runs.
"""

import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import re
import shutil
import statistics
import subprocess
import xml.etree.ElementTree as ET


PREFIX = "material_cpu "
BASELINE_REF = "9572201fa8b6100b4d53a0ffc2a239133aa100ea"
REQUIRED_ENGINE_DIRECTORIES = ("Engine/cpp/Include", "Engine/cpp/Source/Workphone",
                               "Engine/cpp/Source/WPGraphics", "Engine/cpp/Source/WPSQLite",
                               "Engine/cpp/Source/WPImGui", "Engine/cpp/Project/Workphone",
                               "Engine/cpp/Project/WPGraphics", "Engine/cpp/Project/WPSQLite",
                               "Engine/cpp/Project/WPImGui", "Engine/c")


def digest(path):
    value = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def command_output(command, cwd):
    return subprocess.check_output(command, cwd=cwd, text=True, encoding="utf-8").strip()


def source_file(path):
    return path.is_file() and (path.suffix.lower() in {".cpp", ".hpp", ".c", ".h", ".cmake", ".inl", ".inc", ".ipp", ".hxx", ".cxx"}
                               or path.name == "CMakeLists.txt")


def all_engine_sources(root):
    return {str(path.relative_to(root)).replace("\\", "/"): digest(path)
            for path in sorted((root / "Engine").rglob("*")) if source_file(path)}


def sources(root):
    paths = [root / "Tests/cpp/MaterialCpuProfile.hpp", root / "Tests/cpp/MaterialLoadSequenceTests.cpp",
             root / "Tests/cpp/CMakeLists.txt", root / "CMakeLists.txt"]
    for directory in (*REQUIRED_ENGINE_DIRECTORIES, "CMake"):
        paths.extend(path for path in (root / directory).rglob("*") if source_file(path))
    return {str(path.relative_to(root)).replace("\\", "/"): digest(path) for path in sorted(set(paths))}


def build_provenance(root, build):
    result = {"configuration": "RelWithDebInfo", "cache_sha256": digest(build / "CMakeCache.txt"),
              "compiler": {}, "targets": {}}
    for language in ("C", "CXX"):
        paths = list((build / "CMakeFiles").glob(f"*/CMake{language}Compiler.cmake"))
        if len(paths) != 1:
            raise ValueError(f"ambiguous or absent compiler provenance in {build}: {language}")
        data = paths[0].read_text(encoding="utf-8")
        fields = {}
        for field in (f"CMAKE_{language}_COMPILER", f"CMAKE_{language}_COMPILER_VERSION", f"CMAKE_{language}_COMPILER_ID"):
            match = re.search(r'set\(' + field + r' "([^"]+)"\)', data)
            if not match:
                raise ValueError(f"missing compiler metadata {field}")
            fields[field] = match.group(1)
        compiler = Path(fields[f"CMAKE_{language}_COMPILER"])
        fields["executable_sha256"] = digest(compiler)
        result["compiler"][language] = fields
    xml_ns = {"ms": "http://schemas.microsoft.com/developer/msbuild/2003"}
    for name in ("WorkphoneCore", "WorkphoneGraphics", "WorkphonePlatformWin32", "Workphone", "WPGraphics", "WPMaterialLoadSequenceTests"):
        paths = list(build.rglob(name + ".vcxproj"))
        if len(paths) != 1:
            raise ValueError(f"ambiguous or absent target provenance: {name}")
        tree = ET.parse(paths[0])
        fields = {}
        def normalized(value):
            return (value or "").replace(str(root), "<SOURCE_ROOT>").replace(str(root).replace("\\", "/"), "<SOURCE_ROOT>")
        for group in tree.findall("ms:PropertyGroup", xml_ns):
            condition = group.get("Condition", "")
            if condition and "RelWithDebInfo|x64" not in condition:
                continue
            for child in group:
                tag = child.tag.rsplit("}", 1)[-1]
                if tag in ("WholeProgramOptimization", "PlatformToolset", "UseDebugLibraries", "LinkIncremental", "WindowsTargetPlatformVersion"):
                    fields["PropertyGroup/" + tag] = normalized(child.text)
        for group in tree.findall("ms:ItemDefinitionGroup", xml_ns):
            condition = group.get("Condition", "")
            if "RelWithDebInfo|x64" not in condition:
                continue
            compile_group = group.find("ms:ClCompile", xml_ns)
            if compile_group is None:
                continue
            for child in compile_group:
                tag = child.tag.rsplit("}", 1)[-1]
                fields[tag] = normalized(child.text)
            link_group = group.find("ms:Link", xml_ns)
            if link_group is not None:
                for child in link_group:
                    tag = child.tag.rsplit("}", 1)[-1]
                    fields["Link/" + tag] = normalized(child.text)
        if not fields:
            raise ValueError(f"missing RelWithDebInfo compiler flags: {name}")
        result["targets"][name] = fields
    return result


def matching_builds(old, new):
    if old["compiler"] != new["compiler"]:
        raise ValueError("baseline/current compilers differ")
    # Source definitions may evolve with the implementation. Compare the actual
    # optimization/CRT/code-generation choices; retain all flags for review.
    stable = ("Optimization", "RuntimeLibrary", "InlineFunctionExpansion", "FavorSizeOrSpeed",
              "WholeProgramOptimization", "EnableEnhancedInstructionSet", "FloatingPointModel",
              "ExceptionHandling", "RuntimeTypeInfo", "DebugInformationFormat", "LanguageStandard", "AdditionalOptions",
              "PropertyGroup/WholeProgramOptimization", "PropertyGroup/PlatformToolset", "PropertyGroup/UseDebugLibraries",
              "PropertyGroup/LinkIncremental", "PropertyGroup/WindowsTargetPlatformVersion",
              "Link/LinkTimeCodeGeneration", "Link/EnableCOMDATFolding", "Link/OptimizeReferences",
              "Link/TargetMachine", "Link/AdditionalOptions")
    for target in old["targets"]:
        for field in stable:
            if old["targets"][target].get(field) != new["targets"][target].get(field):
                raise ValueError(f"baseline/current code-generation flag differs: {target}/{field}")


def modules(executable):
    paths = [executable, *executable.parent.glob("*.dll")]
    paths.extend(path for name in ("available.cfg", "config.cfg")
                 if (path := executable.parent / name).is_file())
    return {str(path): digest(path) for path in sorted(paths)}


def freeze(executable, directory):
    before = modules(executable)
    directory.mkdir()
    for path in before:
        shutil.copy2(path, directory / Path(path).name)
    if modules(executable) != before:
        raise ValueError("source runtime changed while freezing")
    frozen_exe = directory / executable.name
    frozen = modules(frozen_exe)
    if {Path(path).name: sha for path, sha in frozen.items()} != \
            {Path(path).name: sha for path, sha in before.items()}:
        raise ValueError("frozen runtime differs from source")
    return frozen_exe, before, frozen


def numeric(value):
    try:
        return int(value)
    except ValueError:
        try:
            result = float(value)
            if not math.isfinite(result):
                raise ValueError("CPU profile emitted a nonfinite number")
            return result
        except ValueError:
            return value


def parse(path, implementation, count, width, frames, samples):
    rows = []
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        if line.startswith(PREFIX):
            rows.append(dict((key, numeric(value)) for key, value in
                             (field.split("=", 1) for field in line.split()[1:])))
    expected = {(edits, sample) for edits in (0, 1, 100) for sample in range(samples)}
    if len(rows) != len(expected) or {(r["edits_percent"], r["sample"]) for r in rows} != expected:
        raise ValueError(f"incomplete or duplicate CPU profile: {path}")
    for row in rows:
        for field, value in {"implementation": implementation, "materials": count, "parameters": width,
                             "frames": frames, "sections": 2, "views": 2, "samples": samples,
                             "edited_materials_per_frame": count * row["edits_percent"] // 100,
                             "schema": "mixed_scalar_colour_matrix_texture", "gpu_pixels": "unmeasured", "geometry": "translated_x4",
                             "timer": "steady_clock_wall_interval",
                             "present_calls": 0, "heap_walks": 0, "material_only_attribution": 0,
                             "representative_frame_acceptance": 0, "draws": count * 4 * frames}.items():
            if row.get(field) != value:
                raise ValueError(f"unexpected {field} in {path}: {row.get(field)!r}")
        timings = [row[field] for field in ("update_publication_ns", "capture_prepare_ns",
                                           "submit_ns", "retirement_ns", "total_ns")]
        if any(type(value) not in (int, float) or value < 0 or not math.isfinite(value) for value in timings):
            raise ValueError(f"invalid CPU times: {path}")
        if timings[-1] <= 0 or not math.isclose(sum(timings[:4]), timings[-1], rel_tol=2e-5):
            raise ValueError(f"CPU stage totals do not add up: {path}")
        if type(row.get("checksum")) not in (int, float) or not math.isfinite(row["checksum"]):
            raise ValueError(f"invalid checksum: {path}")
        edited = count * row["edits_percent"] // 100
        expected_checksum = count * 2.25 - (edited * 1.5 if frames % 2 else 0)
        if row["checksum"] != expected_checksum:
            raise ValueError(f"CPU fixture did not produce its intended authored values: {path}")
        if implementation == "optimized" and row["edits_percent"] == 0 and \
                (row["record_preparations"] != 0 or row["workspace_allocations"] != 0):
            raise ValueError(f"unchanged current material records rebuilt or allocated: {path}")
    return rows


def p95(values):
    ordered = sorted(values)
    return ordered[math.ceil(len(ordered) * 0.95) - 1]


def comparisons(runs):
    groups = {}
    for run in runs:
        for row in run["rows"]:
            key = row["materials"], row["parameters"], row["edits_percent"]
            groups.setdefault(key, {"baseline957": [], "optimized": []})[row["implementation"]].append(row)
    result = []
    for (count, width, edits), pair in sorted(groups.items()):
        old, new = pair["baseline957"], pair["optimized"]
        if len(old) != len(new) or not old:
            raise ValueError("CPU comparison is missing a paired implementation")
        for before, after in zip(old, new):
            for field in ("checksum", "draws", "adapter_vendor", "adapter_device", "dedicated_video_memory",
                          "feature_level", "driver_version_available", "driver_version"):
                if before[field] != after[field]:
                    raise ValueError(f"matched CPU workload/device differs: {count}x{width}/{edits}, {field}")
        baseline = [row["total_ns"] / row["frames"] for row in old]
        current = [row["total_ns"] / row["frames"] for row in new]
        old_median, new_median = statistics.median(baseline), statistics.median(current)
        result.append({"materials": count, "parameters": width, "edits_percent": edits,
                       "samples_per_implementation": len(baseline),
                       "baseline_median_ns_per_frame": old_median, "optimized_median_ns_per_frame": new_median,
                       "baseline_p95_batch_average_ns_per_frame": p95(baseline),
                       "optimized_p95_batch_average_ns_per_frame": p95(current),
                       "inclusive_adapter_submit_median_reduction": 1 - new_median / old_median,
                       "inclusive_adapter_submit_50_percent_observed": new_median <= old_median * 0.5,
                       "baseline_stages_median_ns_per_frame": {field: statistics.median(row[field] / row["frames"] for row in old)
                           for field in ("update_publication_ns", "capture_prepare_ns", "submit_ns", "retirement_ns")},
                       "optimized_stages_median_ns_per_frame": {field: statistics.median(row[field] / row["frames"] for row in new)
                           for field in ("update_publication_ns", "capture_prepare_ns", "submit_ns", "retirement_ns")}})
    return result


def resume_completed_runs(parent_path, expected_digest, report, output, counts, widths, frames, samples, repetitions):
    """Import completed processes only after restoring the exact captured sources.

    The abandoned live-source attempt remains abandoned. An immutable source
    snapshot, identical frozen runtime pair and revalidated raw rows are required
    before any completed measurement can contribute to the new protocol.
    """
    raw = parent_path.read_bytes()
    if not re.fullmatch(r'[0-9a-f]{64}', expected_digest or '') or hashlib.sha256(raw).hexdigest() != expected_digest:
        raise ValueError('resume parent manifest differs from its explicit SHA256')
    parent = json.loads(raw)
    if parent.get('status') != 'ABORTED_SOURCE_DRIFT':
        raise ValueError('resume requires an explicitly source-drift-aborted capture')
    for field in ('baseline_ref', 'optimized_ref', 'fixture_sha256', 'source_inputs_sha256',
                  'all_engine_source_inputs_sha256'):
        if parent.get(field) != report.get(field):
            raise ValueError('resume source snapshot differs from captured input: ' + field)
    if parent.get('baseline_ref') != BASELINE_REF:
        raise ValueError('resume baseline is not the original revision')
    for kind in ('baseline', 'optimized'):
        matching_builds(parent['build_provenance'][kind], report['build_provenance'][kind])
        expected = parent[kind + '_runtime_frozen_sha256']
        actual = report[kind + '_runtime_source_sha256']
        if {Path(name).name: value for name, value in expected.items()} != \
                {Path(name).name: value for name, value in actual.items()}:
            raise ValueError('resume frozen runtime differs: ' + kind)
        original_exe = parent_path.parent / ('runtime_' + kind) / 'WPMaterialLoadSequenceTests.exe'
        if modules(original_exe) != expected:
            raise ValueError('resume parent runtime drift: ' + kind)
    prepared = []
    labels = set()
    for run in parent['runs']:
        name = run['name']
        match = re.fullmatch(r'(baseline|optimized)_(\d+)x(8|32|128)_run([1-9]\d*)', name)
        if not match or name in labels:
            raise ValueError('invalid or duplicate completed resume process')
        kind, count, width, repetition = match.groups()
        count, width, repetition = int(count), int(width), int(repetition)
        command = run['command']
        if count not in counts or width not in widths or not 1 <= repetition <= repetitions or \
                run['repetition'] != repetition or command != [
                    str(parent_path.parent / ('runtime_' + kind) / 'WPMaterialLoadSequenceTests.exe'),
                    '--profile-material-cpu', str(count), str(width), str(frames), str(samples)]:
            raise ValueError('completed resume workload differs from requested dimensions')
        stdout = parent_path.parent / (name + '.stdout.log')
        stderr = parent_path.parent / (name + '.stderr.log')
        if Path(run['stdout']).resolve() != stdout.resolve() or Path(run['stderr']).resolve() != stderr.resolve():
            raise ValueError('resume raw output is outside its captured process')
        rows = parse(stdout, 'baseline957' if kind == 'baseline' else 'optimized', count, width, frames, samples)
        if rows != run['rows']:
            raise ValueError('completed resume rows differ from guarded raw output')
        prepared.append((run, stdout, stderr, digest(stdout), digest(stderr)))
        labels.add(name)
    if not prepared or digest(parent_path) != expected_digest:
        raise ValueError('resume parent is empty or changed during validation')
    for run, stdout, stderr, stdout_hash, stderr_hash in prepared:
        copied_out, copied_err = output / stdout.name, output / stderr.name
        shutil.copy2(stdout, copied_out); shutil.copy2(stderr, copied_err)
        if digest(copied_out) != stdout_hash or digest(copied_err) != stderr_hash:
            raise ValueError('resume output changed while copying')
        imported = dict(run)
        imported.update(stdout=str(copied_out), stderr=str(copied_err),
                        stdout_sha256=stdout_hash, stderr_sha256=stderr_hash,
                        execution_origin='validated_completed_parent_process')
        report['runs'].append(imported)
    (output / 'resume_parent_manifest.json').write_bytes(raw)
    report['resume'] = {'manifest': str(parent_path), 'sha256': expected_digest,
                        'imported_processes': len(prepared), 'exact_source_snapshot_restored': True,
                        'original_status': parent['status']}
    return labels


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline-exe", type=Path, required=True)
    parser.add_argument("--optimized-exe", type=Path, required=True)
    parser.add_argument("--baseline-root", type=Path, required=True)
    parser.add_argument("--optimized-root", type=Path, required=True)
    parser.add_argument("--baseline-build", type=Path, required=True)
    parser.add_argument("--optimized-build", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--counts", type=int, nargs="+", default=[1000, 10000])
    parser.add_argument("--widths", type=int, nargs="+", default=[8, 32, 128])
    parser.add_argument("--frames", type=int, default=5)
    parser.add_argument("--samples", type=int, default=7)
    parser.add_argument("--repetitions", type=int, default=3)
    parser.add_argument("--timeout-seconds", type=int, default=1200)
    parser.add_argument("--smoke", action="store_true")
    parser.add_argument('--resume-from', type=Path)
    parser.add_argument('--resume-sha256')
    args = parser.parse_args()
    if bool(args.resume_from) != bool(args.resume_sha256):
        parser.error('resume requires both the parent manifest and its explicit SHA256')
    if args.resume_from:
        args.resume_from = args.resume_from.resolve()
    for field in ("baseline_exe", "optimized_exe", "baseline_root", "optimized_root", "baseline_build", "optimized_build", "output"):
        setattr(args, field, getattr(args, field).resolve())
    if args.output.exists():
        parser.error("output already exists; preserve prior evidence and select a fresh directory")
    if len(set(args.counts)) != len(args.counts) or len(set(args.widths)) != len(args.widths) or \
            any(count < 1 or count > 10000 for count in args.counts) or \
            any(width not in (8, 32, 128) for width in args.widths) or \
            not 1 <= args.frames <= 300 or not 1 <= args.samples <= 31 or not 1 <= args.repetitions <= 31 or \
            args.timeout_seconds <= 0:
        parser.error("invalid or duplicate CPU dimensions")
    if not args.smoke and (set(args.counts) != {1000, 10000} or set(args.widths) != {8, 32, 128} or
                           args.samples < 7 or args.repetitions < 3):
        parser.error("full CPU protocol requires both counts, all widths, >=7 samples and >=3 process repetitions; use --smoke for a diagnostic")
    if command_output(["git", "rev-parse", "HEAD"], args.baseline_root) != BASELINE_REF:
        parser.error("baseline checkout is not the original pre-optimization revision")
    if command_output(["git", "diff", "--name-only", "HEAD", "--", "Engine"], args.baseline_root):
        parser.error("baseline production engine sources are modified")
    old_fixture = args.baseline_root / "Tests/cpp/MaterialCpuProfile.hpp"
    new_fixture = args.optimized_root / "Tests/cpp/MaterialCpuProfile.hpp"
    if digest(old_fixture) != digest(new_fixture):
        parser.error("baseline/current portable fixtures differ")
    args.output.mkdir(parents=True)
    report = {"status": "RUNNING", "scope": "inclusive update/publication, capture/preparation, DX11 submission with translated geometry and record retirement",
              "timer": "steady_clock owner wall interval; includes CPU/driver waits",
              "platform": platform.platform(), "processor": platform.processor(), "smoke": args.smoke,
              "background_workloads_controlled": False,
              "runner_sha256": digest(Path(__file__).resolve()),
              "baseline_ref": BASELINE_REF, "optimized_ref": command_output(["git", "rev-parse", "HEAD"], args.optimized_root),
              "fixture_sha256": digest(new_fixture), "runs": [],
              "material_only_cpu_gate_complete": False, "representative_frame_p95_gate_complete": False,
              "hardware_cache_gate_complete": False, "m6_performance_acceptance": False,
              "limitations": ["Actual DX11 submission and software/driver work are included; material-only CPU is not isolated",
                              "Geometry is translated by x=4 to intend clipping; rasterized pixels are not measured and Present is excluded",
                              "These are not representative frame or GPU timings",
                              "Reported p95 is across normalized multi-frame batch intervals, not individual production frames",
                              "Mixed parameter widths are real retained schemas; only Value0 and standard diffuse/roughness are edited",
                              "No hardware cache misses, instruction counts, or GPU residency are measured"]}
    manifest = args.output / "manifest.json"
    def save():
        manifest.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    try:
        builds = {"baseline": build_provenance(args.baseline_root, args.baseline_build),
                  "optimized": build_provenance(args.optimized_root, args.optimized_build)}
        matching_builds(builds["baseline"], builds["optimized"])
        report["build_provenance"] = builds
        source_inputs = {"baseline": sources(args.baseline_root), "optimized": sources(args.optimized_root)}
        all_engine_inputs = {"baseline": all_engine_sources(args.baseline_root),
                             "optimized": all_engine_sources(args.optimized_root)}
        report["source_inputs_sha256"] = source_inputs
        report["required_source_scope"] = [*REQUIRED_ENGINE_DIRECTORIES, "CMake", "CMakeLists.txt",
                                           "Tests/cpp/MaterialCpuProfile.hpp", "Tests/cpp/MaterialLoadSequenceTests.cpp",
                                           "Tests/cpp/CMakeLists.txt"]
        report["all_engine_source_inputs_sha256"] = all_engine_inputs
        executables = {}
        frozen_modules = {}
        for name, executable in (("baseline", args.baseline_exe), ("optimized", args.optimized_exe)):
            frozen_exe, original, frozen = freeze(executable, args.output / f"runtime_{name}")
            executables[name] = frozen_exe; frozen_modules[name] = frozen
            report[f"{name}_runtime_source_sha256"] = original
            report[f"{name}_runtime_frozen_sha256"] = frozen
        completed = resume_completed_runs(args.resume_from, args.resume_sha256, report, args.output,
            args.counts, args.widths, args.frames, args.samples, args.repetitions) if args.resume_from else set()
        save()
        for repetition in range(args.repetitions):
            for count in args.counts:
                for width in args.widths:
                    order = ("baseline", "optimized") if repetition % 2 == 0 else ("optimized", "baseline")
                    for kind in order:
                        executable = executables[kind]
                        if modules(executable) != frozen_modules[kind]:
                            raise ValueError("frozen runtime drift before CPU process")
                        label = f"{kind}_{count}x{width}_run{repetition+1}"
                        if label in completed:
                            print(f'resumed {label}', flush=True)
                            continue
                        stdout, stderr = args.output / f"{label}.stdout.log", args.output / f"{label}.stderr.log"
                        command = [str(executable), "--profile-material-cpu", str(count), str(width), str(args.frames), str(args.samples)]
                        environment = dict(os.environ)
                        environment["PATH"] = str(executable.parent) + os.pathsep + environment.get("PATH", "")
                        with stdout.open("w", encoding="utf-8") as out, stderr.open("w", encoding="utf-8") as err:
                            process = subprocess.run(command, cwd=executable.parent, env=environment,
                                                     stdout=out, stderr=err, timeout=args.timeout_seconds, check=False)
                        if process.returncode:
                            raise ValueError(f"{label} exited {process.returncode}; inspect {stderr}")
                        if modules(executable) != frozen_modules[kind]:
                            raise ValueError("frozen runtime drift during CPU process")
                        rows = parse(stdout, "baseline957" if kind == "baseline" else "optimized", count, width, args.frames, args.samples)
                        report["runs"].append({"name": label, "command": command, "stdout": str(stdout), "stderr": str(stderr),
                                               "rows": rows, "repetition": repetition+1})
                        save()
                        print(f"accepted {label}", flush=True)
        report["comparisons"] = comparisons(report["runs"])
        report["source_changed_during_run"] = any(sources(root) != source_inputs[kind] for kind, root in
                                                  (("baseline", args.baseline_root), ("optimized", args.optimized_root)))
        report["required_source_changed_during_run"] = report["source_changed_during_run"]
        unrelated_changes = {}
        for kind, root in (("baseline", args.baseline_root), ("optimized", args.optimized_root)):
            after = all_engine_sources(root)
            before = all_engine_inputs[kind]
            unrelated_changes[kind] = {path: {"before": before.get(path), "after": after.get(path)}
                for path in sorted(before.keys() | after.keys())
                if path not in source_inputs[kind] and before.get(path) != after.get(path)}
        report["unrelated_engine_source_changes"] = unrelated_changes
        report["all_engine_source_changed_during_run"] = report["source_changed_during_run"] or any(unrelated_changes.values())
        report["frozen_runtime_changed_during_run"] = any(modules(executables[kind]) != frozen_modules[kind] for kind in executables)
        if report["source_changed_during_run"] or report["frozen_runtime_changed_during_run"]:
            raise ValueError("profile inputs changed during CPU protocol")
        if digest(Path(__file__).resolve()) != report['runner_sha256']:
            raise ValueError('CPU runner changed during protocol')
        report["status"] = "COMPLETE"
        report["full_inclusive_cpu_protocol_completed"] = not args.smoke
        save()
        print(json.dumps({"status": report["status"], "runs": len(report["runs"]), "manifest": str(manifest)}, indent=2))
    except Exception as error:
        report["status"] = "REJECTED"; report["error"] = str(error)
        save()
        raise


if __name__ == "__main__":
    main()
