"""Guards for captured-generation replay; fake runtime files never start an engine."""
import copy
import json
from pathlib import Path
import sys
import tempfile
import unittest
from types import SimpleNamespace
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'Tools'))
import ProfileFrozenMaterialPublication as replay


def row(edits):
    return dict(implementation='optimized', materials=1000, parameters=32, frames=1,
                sections=2, views=2, samples=1, edits_percent=edits,
                edited_materials_per_frame=1000 * edits // 100, sample=0,
                schema='mixed_scalar_colour_matrix_texture', gpu_pixels='unmeasured',
                geometry='translated_x4', timer='steady_clock_wall_interval', present_calls=0,
                heap_walks=0, material_only_attribution=0, representative_frame_acceptance=0,
                draws=4000, update_publication_ns=100, capture_prepare_ns=200, submit_ns=600,
                retirement_ns=100, total_ns=1000, checksum=2250 - (1000 * edits // 100) * 1.5,
                record_preparations=0, workspace_allocations=0, adapter_vendor=5140,
                adapter_device=140, dedicated_video_memory=0, feature_level=45056,
                driver_version_available=1, driver_version=1)


def output_rows(rows):
    return '\n'.join(replay.cpu.PREFIX + ' '.join(f'{key}={value}' for key, value in value.items())
                     for value in rows) + '\n'


class FrozenReplayGuards(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.fixture = self.root / 'MaterialCpuProfile.hpp'
        self.fixture.write_text('identical portable fixture')
        self.path = self.root / 'parent/manifest.json'
        self.path.parent.mkdir()
        self.parent = dict(status='COMPLETE', material_only_attribution=False,
                           representative_frame_acceptance=False, hardware_cache_acceptance=False,
                           fixture_sha256=replay.cpu.digest(self.fixture),
                           source_inputs={'Engine/source.cpp': '0' * 64,
                                          'Engine/cpp/Source/Workphone/System/StateContext.cpp': '3' * 64},
                           extra_source_inputs={'Tests/header.hpp': '1' * 64},
                           source_changes={'Engine/cpp/Source/Workphone/System/StateContext.cpp':
                                           {'control': '2' * 64, 'candidate': '3' * 64}},
                           control_receipt_sha256='4' * 64, runner_sha256='5' * 64, runs=[])
        for kind in replay.KINDS:
            folder = self.path.parent / ('runtime_' + kind)
            folder.mkdir()
            for name in (replay.EXECUTABLE, 'Workphone.dll'):
                (folder / name).write_bytes(('captured ' + kind + name).encode())
            self.parent[kind + '_frozen_runtime'] = {str(path): replay.cpu.digest(path) for path in folder.iterdir()}
            name = kind + '_1000x32_1'
            rows = [row(edits) for edits in (0, 1, 100)]
            stdout = self.path.parent / (name + '.stdout.log')
            stderr = self.path.parent / (name + '.stderr.log')
            stdout.write_text(output_rows(rows))
            stderr.write_text('')
            self.parent['runs'].append(dict(kind=kind, name=name, rows=rows,
                command=[str(folder / replay.EXECUTABLE), '--profile-material-cpu', '1000', '32', '1', '1'],
                stdout_sha256=replay.cpu.digest(stdout), stderr_sha256=replay.cpu.digest(stderr)))
        self.parent['comparisons'] = replay.publication.compare(self.parent['runs'])
        self.save()

    def save(self):
        self.path.write_text(json.dumps(self.parent))
        self.checksum = replay.cpu.digest(self.path)

    def validate(self):
        return replay.validate_parent(self.path, self.checksum, self.fixture)

    def args(self, **changes):
        values = dict(parent=self.path, parent_sha256=self.checksum, fixture=self.fixture,
                      output=self.root / 'replay', counts=[1000], widths=[32], frames=1,
                      samples=1, repetitions=1)
        values.update(changes)
        return SimpleNamespace(**values)

    def test_complete_parent_requires_exact_digest_and_portable_fixture(self):
        parent, runtimes, raw = self.validate()
        self.assertEqual(parent, self.parent)
        self.assertEqual(set(runtimes), set(replay.KINDS))
        self.assertEqual(replay.digest_bytes(raw), self.checksum)
        with self.assertRaises(ValueError):
            replay.validate_parent(self.path, '0' * 64, self.fixture)
        self.fixture.write_text('different fixture')
        with self.assertRaises(ValueError):
            self.validate()

    def test_running_rejected_or_empty_comparisons_cannot_be_replayed(self):
        original = copy.deepcopy(self.parent)
        for state in ('RUNNING', 'REJECTED', None):
            self.parent = copy.deepcopy(original)
            self.parent['status'] = state
            self.save()
            with self.subTest(state=state), self.assertRaises(ValueError):
                self.validate()
        self.parent = original
        self.parent['comparisons'] = []
        self.save()
        with self.assertRaises(ValueError):
            self.validate()

    def test_parent_values_comparisons_and_commands_must_match_raw_outputs(self):
        original = copy.deepcopy(self.parent)
        for mutation in ('rows', 'comparison', 'command', 'log_hash', 'missing_pair'):
            self.parent = copy.deepcopy(original)
            if mutation == 'rows':
                self.parent['runs'][0]['rows'][0]['checksum'] += 1
            elif mutation == 'comparison':
                self.parent['comparisons'][0]['inclusive_reduction'] = .9
            elif mutation == 'command':
                self.parent['runs'][0]['command'][3] = '8'
            elif mutation == 'log_hash':
                self.parent['runs'][0]['stdout_sha256'] = '0' * 64
            else:
                self.parent['runs'].pop()
            self.save()
            with self.subTest(mutation=mutation), self.assertRaises(ValueError):
                self.validate()

    def test_runtime_membership_rejects_extra_missing_changed_and_nested_files(self):
        folder = self.path.parent / 'runtime_control'
        payload = folder / 'Workphone.dll'
        original = payload.read_bytes()
        for mutation in ('extra_dll', 'extra_text', 'nested', 'missing', 'changed'):
            if mutation.startswith('extra'):
                extra = folder / ('surprise.dll' if mutation == 'extra_dll' else 'unrecorded.txt')
                extra.write_text('unexpected')
            elif mutation == 'nested':
                extra = folder / 'nested'
                extra.mkdir()
            elif mutation == 'missing':
                payload.unlink()
            else:
                payload.write_bytes(b'changed bytes')
            with self.subTest(mutation=mutation), self.assertRaises(ValueError):
                self.validate()
            if mutation == 'nested':
                extra.rmdir()
            elif mutation.startswith('extra'):
                extra.unlink()
            else:
                payload.write_bytes(original)

    def test_runtime_paths_cannot_leave_the_captured_pair(self):
        source = self.parent['control_frozen_runtime']
        name, checksum = next(iter(source.items()))
        del source[name]
        source[str(self.root / Path(name).name)] = checksum
        self.save()
        with self.assertRaises(ValueError):
            self.validate()

    def test_parent_source_provenance_cannot_be_misrepresented(self):
        self.parent['source_changes']['Engine/cpp/Source/Workphone/System/StateContext.cpp']['candidate'] = '6' * 64
        self.save()
        with self.assertRaises(ValueError):
            self.validate()

    def test_output_cannot_modify_the_parent_runtime_membership(self):
        output = self.path.parent / 'runtime_control/new_output'
        with self.assertRaises(ValueError):
            replay.run_replay(self.args(output=output))
        self.assertFalse(output.exists())

    def test_dimension_limits_and_duplicates_are_rejected(self):
        for changes in (dict(counts=[99]), dict(counts=[10001]), dict(counts=[1000, 1000]),
                        dict(widths=[16]), dict(widths=[8, 8]), dict(widths=[]), dict(frames=0),
                        dict(frames=301), dict(samples=32), dict(repetitions=0)):
            args = self.args(**changes)
            with self.subTest(changes=changes), self.assertRaises(ValueError):
                replay.run_replay(args)
            self.assertFalse(args.output.exists())

    def test_existing_output_is_preserved_before_validation_or_execution(self):
        args = self.args()
        args.output.mkdir()
        sentinel = args.output / 'preserve.txt'
        sentinel.write_text('existing evidence')
        with patch.object(replay.subprocess, 'run') as run, self.assertRaises(ValueError):
            replay.run_replay(args)
        run.assert_not_called()
        self.assertEqual(sentinel.read_text(), 'existing evidence')

    def process(self, command, **kwargs):
        count, width, frames, samples = map(int, command[2:])
        self.assertEqual((count, width, frames, samples), (1000, 32, 1, 1))
        kwargs['stdout'].write(output_rows([row(edits) for edits in (0, 1, 100)]))
        return SimpleNamespace(returncode=0)

    def test_replay_copies_exact_pair_alternates_order_and_records_captured_scope(self):
        args = self.args(repetitions=2)
        with patch.object(replay.subprocess, 'run', side_effect=self.process) as process:
            report = replay.run_replay(args)
        self.assertEqual(report['status'], 'COMPLETE')
        self.assertFalse(report['current_checkout_validated'])
        self.assertFalse(report['material_only_attribution'])
        self.assertFalse(report['m6_complete'])
        self.assertEqual([run['kind'] for run in report['runs']],
                         ['control', 'candidate', 'candidate', 'control'])
        self.assertEqual(process.call_count, 4)
        self.assertEqual(replay.cpu.digest(args.output / 'parent_manifest.json'), self.checksum)
        self.assertEqual(report['parent_source_inputs'], self.parent['source_inputs'])

    def test_each_process_is_guarded_against_parent_and_copied_runtime_drift(self):
        for target in ('parent', 'copy'):
            args = self.args(output=self.root / ('drift_' + target))
            parent_payload = self.path.parent / 'runtime_control/Workphone.dll'
            original = parent_payload.read_bytes()
            def drift(command, **kwargs):
                result = self.process(command, **kwargs)
                payload = parent_payload if target == 'parent' else args.output / 'runtime_control/Workphone.dll'
                payload.write_text('changed while child process ran')
                return result
            with self.subTest(target=target), patch.object(replay.subprocess, 'run', side_effect=drift) as process:
                with self.assertRaises(ValueError):
                    replay.run_replay(args)
                self.assertEqual(process.call_count, 1)
                self.assertEqual(json.loads((args.output / 'manifest.json').read_text())['status'], 'REJECTED')
            parent_payload.write_bytes(original)

    def test_new_output_still_uses_draw_value_and_device_guards(self):
        args = self.args()
        def wrong(command, **kwargs):
            values = [row(edits) for edits in (0, 1, 100)]
            values[0]['draws'] -= 1
            kwargs['stdout'].write(output_rows(values))
            return SimpleNamespace(returncode=0)
        with patch.object(replay.subprocess, 'run', side_effect=wrong), self.assertRaises(ValueError):
            replay.run_replay(args)
        self.assertEqual(json.loads((args.output / 'manifest.json').read_text())['status'], 'REJECTED')


if __name__ == '__main__':
    unittest.main()
