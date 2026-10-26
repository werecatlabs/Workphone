"""Replay an accepted publication comparison's exact captured runtime pair.

This is a separate captured-generation protocol, not current-checkout validation.
It never relaxes the live-source guards in ProfileMaterialPublication.py.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess

import ProfileMaterialCpu as cpu
import ProfileMaterialPublication as publication


KINDS = ('control', 'candidate')
EXECUTABLE = 'WPMaterialLoadSequenceTests.exe'


def digest_bytes(value):
    return hashlib.sha256(value).hexdigest()


def valid_digest(value):
    return isinstance(value, str) and re.fullmatch('[0-9a-f]{64}', value) is not None


def dimensions(counts, widths, frames, samples, repetitions):
    if (not counts or not widths or len(set(counts)) != len(counts) or
            len(set(widths)) != len(widths) or any(type(n) is not int or not 100 <= n <= 10000 for n in counts) or
            any(type(n) is not int or n not in (8, 32, 128) for n in widths) or
            type(frames) is not int or not 1 <= frames <= 300 or
            type(samples) is not int or not 1 <= samples <= 31 or
            type(repetitions) is not int or not 1 <= repetitions <= 31):
        raise ValueError('invalid or duplicate frozen replay dimensions')


def runtime_receipt(parent_path, parent, kind):
    directory = parent_path.parent / ('runtime_' + kind)
    receipt = parent.get(kind + '_frozen_runtime')
    if not isinstance(receipt, dict) or not receipt:
        raise ValueError('parent has no frozen ' + kind + ' runtime')
    expected = {}
    for value, checksum in receipt.items():
        path = Path(value)
        if (not path.is_absolute() or path.parent.resolve() != directory.resolve() or
                path.resolve() != (directory / path.name).resolve() or not valid_digest(checksum) or
                path.name in expected or
                (path.name != EXECUTABLE and path.suffix.lower() != '.dll' and
                 path.name not in ('available.cfg', 'config.cfg'))):
            raise ValueError('invalid parent runtime member: ' + str(value))
        expected[path.name] = checksum
    if EXECUTABLE not in expected or not any(name.lower().endswith('.dll') for name in expected):
        raise ValueError('parent runtime lacks its executable or DLLs')
    verify_runtime(directory, expected)
    return directory, expected


def verify_runtime(directory, expected):
    if not directory.is_dir() or any(path.is_dir() or path.is_symlink() for path in directory.iterdir()):
        raise ValueError('runtime is missing or contains unexpected directories/links')
    actual = {path.name: cpu.digest(path) for path in directory.iterdir()}
    if actual != expected:
        raise ValueError('frozen runtime contents or membership changed: ' + str(directory))


def validate_parent(path, expected_sha256, fixture):
    raw = path.read_bytes()
    if not valid_digest(expected_sha256) or digest_bytes(raw) != expected_sha256:
        raise ValueError('parent manifest SHA256 differs from the explicit expected digest')
    parent = json.loads(raw)
    if not isinstance(parent, dict) or parent.get('status') != 'COMPLETE' or not parent.get('comparisons'):
        raise ValueError('only a COMPLETE accepted publication comparison can be replayed')
    if (parent.get('material_only_attribution') is not False or
            parent.get('representative_frame_acceptance') is not False or
            parent.get('hardware_cache_acceptance') is not False):
        raise ValueError('parent attribution boundaries differ')
    if not valid_digest(parent.get('fixture_sha256')) or cpu.digest(fixture) != parent['fixture_sha256']:
        raise ValueError('current portable fixture differs from the accepted captured fixture')
    for field in ('source_inputs', 'extra_source_inputs', 'source_changes'):
        if not isinstance(parent.get(field), dict) or not parent[field]:
            raise ValueError('parent source provenance is incomplete: ' + field)
    for field in ('source_inputs', 'extra_source_inputs'):
        if any(not isinstance(name, str) or not valid_digest(value) for name, value in parent[field].items()):
            raise ValueError('parent source digest provenance is invalid: ' + field)
    if set(parent['source_changes']) - publication.ALLOWED_CHANGES:
        raise ValueError('parent production differences exceed the accepted publication scope')
    for name, change in parent['source_changes'].items():
        if (not isinstance(change, dict) or not valid_digest(change.get('control')) or
                not valid_digest(change.get('candidate')) or
                change['candidate'] != parent['source_inputs'].get(name)):
            raise ValueError('parent production differences do not match its source inputs')
    for field in ('control_receipt_sha256', 'runner_sha256'):
        if not valid_digest(parent.get(field)):
            raise ValueError('parent digest provenance is incomplete: ' + field)
    runtimes = {kind: runtime_receipt(path, parent, kind) for kind in KINDS}
    runs = parent.get('runs')
    if not isinstance(runs, list) or not runs:
        raise ValueError('parent workload has no runs')
    labels = set()
    signatures = {kind: set() for kind in KINDS}
    settings = set()
    for run in runs:
        if not isinstance(run, dict):
            raise ValueError('invalid parent workload run')
        kind = run.get('kind')
        name = run.get('name')
        command = run.get('command')
        if (kind not in KINDS or not isinstance(name, str) or
                not re.fullmatch(r'(control|candidate)_\d+x(8|32|128)_\d+', name) or
                name in labels or not name.startswith(kind + '_') or
                not isinstance(command, list) or len(command) != 6 or
                command[1] != '--profile-material-cpu' or
                Path(command[0]).resolve() != (runtimes[kind][0] / EXECUTABLE).resolve()):
            raise ValueError('parent command/name is not the captured portable CPU workload')
        labels.add(name)
        count, width, frames, samples = map(int, command[2:])
        dimensions([count], [width], frames, samples, 1)
        if not name.startswith(f'{kind}_{count}x{width}_') or int(name.rsplit('_', 1)[1]) < 1:
            raise ValueError('parent run name and workload dimensions differ')
        repetition = int(name.rsplit('_', 1)[1])
        signatures[kind].add((count, width, frames, samples, repetition))
        settings.add((width, frames, samples))
        stdout = path.parent / (name + '.stdout.log')
        stderr = path.parent / (name + '.stderr.log')
        for log, field in ((stdout, 'stdout_sha256'), (stderr, 'stderr_sha256')):
            if not valid_digest(run.get(field)) or cpu.digest(log) != run[field]:
                raise ValueError('parent output provenance changed: ' + str(log))
        parsed = cpu.parse(stdout, 'optimized', count, width, frames, samples)
        if parsed != run.get('rows'):
            raise ValueError('parent stored rows differ from its guarded raw output')
    if signatures['control'] != signatures['candidate'] or len(settings) != 1:
        raise ValueError('parent is missing an identical paired process workload')
    if publication.compare(runs) != parent['comparisons']:
        raise ValueError('parent comparisons differ from the guarded workload')
    if digest_bytes(path.read_bytes()) != expected_sha256:
        raise ValueError('parent changed while validating its inputs')
    return parent, runtimes, raw


def verify_inputs(parent_path, parent_sha256, fixture, parent, runtimes, frozen):
    if cpu.digest(parent_path) != parent_sha256 or cpu.digest(fixture) != parent['fixture_sha256']:
        raise ValueError('parent manifest or portable fixture changed during replay')
    for kind in KINDS:
        verify_runtime(*runtimes[kind])
        if kind in frozen:
            verify_runtime(*frozen[kind])


def run_replay(args):
    dimensions(args.counts, args.widths, args.frames, args.samples, args.repetitions)
    if args.output.exists():
        raise ValueError('output already exists; preserve previous evidence')
    parent, runtimes, raw = validate_parent(args.parent, args.parent_sha256, args.fixture)
    if any(args.output.is_relative_to(directory) for directory, _ in runtimes.values()):
        raise ValueError('replay output cannot be inside the parent frozen runtime')
    args.output.mkdir(parents=True)
    frozen = {}
    report = {
        'status': 'RUNNING', 'protocol': 'frozen_publication_replay_v1',
        'scope': 'captured generation only; inclusive render-owner wall intervals with real DX11 submission',
        'current_checkout_validated': False, 'background_workloads_controlled': False,
        'material_only_attribution': False, 'hardware_cache_acceptance': False,
        'representative_frame_acceptance': False, 'm6_complete': False,
        'parent_manifest': str(args.parent), 'parent_manifest_sha256': args.parent_sha256,
        'parent_source_inputs': parent['source_inputs'],
        'parent_extra_source_inputs': parent['extra_source_inputs'],
        'parent_source_changes': parent['source_changes'],
        'parent_control_receipt_sha256': parent['control_receipt_sha256'],
        'parent_runner_sha256': parent['runner_sha256'],
        'fixture_sha256': parent['fixture_sha256'],
        'runner_sha256': cpu.digest(Path(__file__)),
        'dependency_sha256': {str(Path(tool.__file__).resolve()): cpu.digest(Path(tool.__file__))
                              for tool in (cpu, publication)},
        'workload': {'counts': args.counts, 'widths': args.widths, 'frames': args.frames,
                     'samples': args.samples, 'repetitions': args.repetitions}, 'runs': []}
    manifest = args.output / 'manifest.json'

    def save():
        manifest.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')

    try:
        (args.output / 'parent_manifest.json').write_bytes(raw)
        for kind in KINDS:
            verify_inputs(args.parent, args.parent_sha256, args.fixture, parent, runtimes, frozen)
            directory = args.output / ('runtime_' + kind)
            directory.mkdir()
            for name in runtimes[kind][1]:
                shutil.copy2(runtimes[kind][0] / name, directory / name)
            frozen[kind] = (directory, runtimes[kind][1])
            report[kind + '_frozen_runtime'] = {str(directory / name): value for name, value in frozen[kind][1].items()}
            verify_inputs(args.parent, args.parent_sha256, args.fixture, parent, runtimes, frozen)
        save()
        pair = 0
        for repetition in range(args.repetitions):
            for count in args.counts:
                for width in args.widths:
                    order = KINDS if pair % 2 == 0 else tuple(reversed(KINDS))
                    pair += 1
                    for kind in order:
                        verify_inputs(args.parent, args.parent_sha256, args.fixture, parent, runtimes, frozen)
                        executable = frozen[kind][0] / EXECUTABLE
                        label = f'{kind}_{count}x{width}_{repetition + 1}'
                        stdout = args.output / (label + '.stdout.log')
                        stderr = args.output / (label + '.stderr.log')
                        command = [str(executable), '--profile-material-cpu', str(count), str(width),
                                   str(args.frames), str(args.samples)]
                        environment = dict(os.environ)
                        environment['PATH'] = str(executable.parent) + os.pathsep + environment.get('PATH', '')
                        with stdout.open('w', encoding='utf-8') as out, stderr.open('w', encoding='utf-8') as err:
                            result = subprocess.run(command, cwd=executable.parent, env=environment,
                                                    stdout=out, stderr=err, timeout=1200)
                        verify_inputs(args.parent, args.parent_sha256, args.fixture, parent, runtimes, frozen)
                        if result.returncode:
                            raise ValueError(label + ' failed: ' + str(result.returncode))
                        rows = cpu.parse(stdout, 'optimized', count, width, args.frames, args.samples)
                        report['runs'].append({'kind': kind, 'name': label, 'command': command, 'rows': rows,
                                               'stdout_sha256': cpu.digest(stdout), 'stderr_sha256': cpu.digest(stderr)})
                        save()
                        print('accepted ' + label, flush=True)
        report['comparisons'] = publication.compare(report['runs'])
        verify_inputs(args.parent, args.parent_sha256, args.fixture, parent, runtimes, frozen)
        if any(cpu.digest(Path(name)) != value for name, value in report['dependency_sha256'].items()) or \
                cpu.digest(Path(__file__)) != report['runner_sha256']:
            raise ValueError('replay runner dependencies changed')
        report['status'] = 'COMPLETE'
        save()
        return report
    except Exception as error:
        report['status'] = 'REJECTED'
        report['error'] = str(error)
        save()
        raise


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--parent', type=Path, required=True)
    parser.add_argument('--parent-sha256', required=True)
    parser.add_argument('--fixture', type=Path,
                        default=Path(__file__).resolve().parents[1] / 'Tests/cpp/MaterialCpuProfile.hpp')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--counts', type=int, nargs='+', default=[1000, 10000])
    parser.add_argument('--widths', type=int, nargs='+', default=[32])
    parser.add_argument('--frames', type=int, default=1)
    parser.add_argument('--samples', type=int, default=3)
    parser.add_argument('--repetitions', type=int, default=1)
    args = parser.parse_args()
    args.parent = args.parent.resolve()
    args.fixture = args.fixture.resolve()
    args.output = args.output.resolve()
    try:
        run_replay(args)
    except (ValueError, OSError, KeyError, TypeError) as error:
        parser.exit(2, 'error: ' + str(error) + '\n')


if __name__ == '__main__':
    main()
