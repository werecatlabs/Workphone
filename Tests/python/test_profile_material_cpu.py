"""Evidence guards for the matched real-adapter CPU runner; no engine startup."""

import copy
import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


TOOL = Path(__file__).resolve().parents[2] / "Tools/ProfileMaterialCpu.py"
SPEC = importlib.util.spec_from_file_location("profile_material_cpu", TOOL)
profile = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(profile)


def rows(kind="optimized"):
    return [dict(implementation=kind, materials=1000, parameters=32, frames=5,
                 sections=2, views=2, samples=2, edits_percent=edits,
                 edited_materials_per_frame=1000 * edits // 100, sample=sample,
                 schema="mixed_scalar_colour_matrix_texture", gpu_pixels="unmeasured", geometry="translated_x4",
                 timer="steady_clock_wall_interval", present_calls=0, heap_walks=0,
                 material_only_attribution=0, representative_frame_acceptance=0,
                 draws=20000, update_publication_ns=100, capture_prepare_ns=200,
                 submit_ns=600, retirement_ns=100, total_ns=1000,
                 checksum=2250 - (1000 * edits // 100) * 1.5, record_preparations=0, workspace_allocations=0,
                 adapter_vendor=0x1414, adapter_device=0x8c, dedicated_video_memory=0,
                 feature_level=0xb000, driver_version_available=0, driver_version=0)
            for edits in (0, 1, 100) for sample in range(2)]


class CpuEvidenceGuards(unittest.TestCase):
    def test_loaded_source_and_unrelated_engine_drift_are_distinct(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            files = ("Tests/cpp/MaterialCpuProfile.hpp", "Tests/cpp/MaterialLoadSequenceTests.cpp",
                     "Tests/cpp/CMakeLists.txt", "CMakeLists.txt", "Engine/cpp/Include/WPGraphics/Material.inl",
                     "Engine/cpp/Source/Workphone/Material.cpp", "Engine/cpp/Source/WPGraphics/ClawMaterial.cpp",
                     "Engine/cpp/Source/WPSQLite/SQLite.cpp", "Engine/cpp/Source/WPImGui/ImGuiText.cpp",
                     "Engine/c/Source/WorkphonePlatformWin32/renderer.c", "Engine/cpp/Source/WPAssimp/AssimpLoader.cpp")
            for name in files:
                path = root / name; path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text("original", encoding="utf-8")
            required, complete = profile.sources(root), profile.all_engine_sources(root)
            unrelated = "Engine/cpp/Source/WPAssimp/AssimpLoader.cpp"
            self.assertNotIn(unrelated, required)
            self.assertIn(unrelated, complete)
            (root / unrelated).write_text("unrelated change", encoding="utf-8")
            self.assertEqual(profile.sources(root), required)
            self.assertNotEqual(profile.all_engine_sources(root), complete)
            for name in files:
                if name != unrelated:
                    self.assertIn(name, required)
            (root / "Engine/cpp/Source/Workphone/Material.cpp").write_text("required change", encoding="utf-8")
            self.assertNotEqual(profile.sources(root), required)

    def test_cli_rejects_invalid_protocol_before_launch(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            base = [sys.executable, str(TOOL), "--baseline-exe", str(root / "missing_old.exe"),
                    "--optimized-exe", str(root / "missing_new.exe"),
                    "--baseline-root", directory, "--optimized-root", directory,
                    "--baseline-build", directory, "--optimized-build", directory,
                    "--output", str(root / "evidence")]
            for invalid in (("--smoke", "--counts", "0"), ("--smoke", "--widths", "16"),
                            ("--smoke", "--frames", "0"), ("--smoke", "--samples", "32"),
                            ("--smoke", "--repetitions", "0"), ("--counts", "1000"),
                            ("--smoke", "--counts", "1000", "1000")):
                result = subprocess.run([*base, *invalid], capture_output=True, text=True)
                with self.subTest(invalid=invalid):
                    self.assertEqual(result.returncode, 2)
                    self.assertIn("error:", result.stderr)
                    self.assertFalse((root / "evidence").exists())
            (root / "evidence").mkdir()
            sentinel = root / "evidence" / "preserve.txt"
            sentinel.write_text("retained evidence", encoding="utf-8")
            result = subprocess.run([*base, "--smoke"], capture_output=True, text=True)
            self.assertEqual(result.returncode, 2)
            self.assertIn("output already exists", result.stderr)
            self.assertEqual(sentinel.read_text(encoding="utf-8"), "retained evidence")

    def parse(self, data, kind="optimized"):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "stdout.log"
            path.write_text("\n".join(profile.PREFIX + " ".join(f"{key}={value}" for key, value in row.items())
                                     for row in data), encoding="utf-8")
            return profile.parse(path, kind, 1000, 32, 5, 2)

    def test_valid_complete_output(self):
        self.assertEqual(len(self.parse(rows())), 6)

    def test_incomplete_or_duplicate_output_rejected(self):
        for data in (rows()[:-1], rows() + [rows()[0]], rows()[:-1] + [rows()[0]]):
            with self.subTest(length=len(data)), self.assertRaises(ValueError):
                self.parse(data)

    def test_wrong_workload_or_implementation_rejected(self):
        for field, value in (("implementation", "baseline957"), ("parameters", 8),
                             ("draws", 19999), ("edited_materials_per_frame", 1000),
                             ("present_calls", 1), ("heap_walks", 1)):
            data = rows(); data[0][field] = value
            with self.subTest(field=field), self.assertRaises(ValueError):
                self.parse(data)

    def test_false_attribution_or_clock_rejected(self):
        for field, value in (("material_only_attribution", 1), ("representative_frame_acceptance", 1),
                             ("timer", "cpu_cycles")):
            data = rows(); data[0][field] = value
            with self.subTest(field=field), self.assertRaises(ValueError):
                self.parse(data)

    def test_nonfinite_or_invalid_timing_rejected(self):
        for value in (float("nan"), float("inf"), -1, "nonsense"):
            data = rows(); data[0]["submit_ns"] = value
            with self.subTest(value=value), self.assertRaises(ValueError):
                self.parse(data)

    def test_stage_total_mismatch_rejected(self):
        data = rows(); data[0]["total_ns"] = 2000
        with self.assertRaises(ValueError):
            self.parse(data)

    def test_invalid_checksum_rejected(self):
        for value in (float("nan"), float("inf"), "nonsense", 1000):
            data = rows(); data[0]["checksum"] = value
            with self.subTest(value=value), self.assertRaises(ValueError):
                self.parse(data)

    def test_stable_rebuild_or_allocation_rejected(self):
        for field in ("record_preparations", "workspace_allocations"):
            data = rows(); data[0][field] = 1
            with self.subTest(field=field), self.assertRaises(ValueError):
                self.parse(data)

    def test_edited_allocations_disclosed_not_disguised(self):
        data = rows(); data[-1]["workspace_allocations"] = 3
        self.assertEqual(self.parse(data)[-1]["workspace_allocations"], 3)

    def test_comparison_checks_output_and_device_parity(self):
        for field in ("checksum", "draws", "adapter_device", "driver_version", "feature_level"):
            old, new = rows("baseline957"), rows()
            new[0][field] += 1
            with self.subTest(field=field), self.assertRaises(ValueError):
                profile.comparisons([{"rows": old}, {"rows": new}])

    def test_missing_comparison_rejected(self):
        with self.assertRaises(ValueError):
            profile.comparisons([{"rows": rows()}])

    def test_reduction_uses_inclusive_stage_total(self):
        old, new = rows("baseline957"), rows()
        for row in new:
            row["total_ns"] = 400
        compared = profile.comparisons([{"rows": old}, {"rows": new}])
        self.assertEqual(len(compared), 3)
        self.assertTrue(all(row["inclusive_adapter_submit_50_percent_observed"] for row in compared))
        self.assertTrue(all(abs(row["inclusive_adapter_submit_median_reduction"] - .6) < 1e-12 for row in compared))
        self.assertTrue(all("baseline_p95_batch_average_ns_per_frame" in row for row in compared))
        self.assertTrue(all("baseline_p95_ns_per_frame" not in row for row in compared))

    def test_compiler_and_native_codegen_mismatch_rejected(self):
        old = {"compiler": {"C": {"version": "19.51"}}, "targets": {
            "WorkphonePlatformWin32": {"RuntimeLibrary": "MultiThreadedDLL", "Optimization": "MaxSpeed"}}}
        profile.matching_builds(old, copy.deepcopy(old))
        for category, field, value in (("compiler", "version", "19.50"), ("targets", "RuntimeLibrary", "MultiThreadedDebugDLL"),
                                        ("targets", "Optimization", "Disabled"),
                                        ("targets", "PropertyGroup/WholeProgramOptimization", "true"),
                                        ("targets", "Link/LinkTimeCodeGeneration", "UseLinkTimeCodeGeneration")):
            new = copy.deepcopy(old)
            new[category]["C" if category == "compiler" else "WorkphonePlatformWin32"][field] = value
            with self.subTest(category=category, field=field), self.assertRaises(ValueError):
                profile.matching_builds(old, new)


if __name__ == "__main__":
    unittest.main()
