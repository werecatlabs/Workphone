"""Cook a representative forest and validate its source-free Windows package."""

import argparse
from contextlib import closing
import hashlib
import json
from pathlib import Path
import shutil
import sqlite3
import subprocess
import tempfile
import uuid


def hash_file(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def inventory(directory):
    return {path.relative_to(directory).as_posix(): hash_file(path)
            for path in sorted(directory.rglob('*')) if path.is_file()}


def identity(path):
    return str(uuid.uuid5(uuid.NAMESPACE_URL, 'workphone:installed-foliage:v1/' + path))


def component(name, **properties):
    return dict(handle_name='component', componentType='workphone::scene::' + name,
                uuid=identity(name), Enabled='true', children=[], **properties)


def validate(options):
    host = options.host.resolve(strict=True)
    tool = options.tool.resolve(strict=True)
    editor = options.editor.resolve(strict=True) if options.editor else None
    output = options.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    captures = output / 'captures'
    captures.mkdir()
    installed = output / 'package'
    programs = {host, tool, *host.parent.glob('*.dll'), *tool.parent.glob('*.dll')}
    if editor:
        programs.update({editor, *editor.parent.glob('*.dll')})
    binaries = {str(path): hash_file(path) for path in sorted(programs)}

    def run(executable, args, label, cwd=None):
        with (output / (label + '.log')).open('w', encoding='utf-8') as log:
            result = subprocess.run([str(executable), *map(str, args)], cwd=cwd or executable.parent,
                                    stdout=log, stderr=subprocess.STDOUT, timeout=180)
        log_path = output / (label + '.log')
        if result.returncode:
            raise RuntimeError(f'{label} exited with {result.returncode}. See {log_path}')
        return log_path.read_text(encoding='utf-8', errors='replace').splitlines()

    with tempfile.TemporaryDirectory(prefix='wp-foliage-install-') as temporary:
        root = Path(temporary).resolve()
        source = root / 'source'
        cooked = root / 'cache'
        catalog = root / 'catalog.db'
        run(host, ['--installed-foliage-export', source], 'export')
        forest_uuid = identity('forest.folworld')
        scene_uuid = identity('forest.fbscene')
        terrain_actor = dict(handle_name='actors', uuid=identity('forest actor'), name='InstalledForest',
                             children=[component('TerrainSystem', terrainAssetUuid=identity('ground.terrain'),
                                                 foliageWorldAssetUuid=forest_uuid)])
        camera_actor = dict(handle_name='actors', uuid=identity('camera actor'), name='ForestCamera',
                             children=[dict(handle_name='localTransform', Position='0 5 235',
                                            Scale='1 1 1', Orientation='1 0 0 0', children=[]),
                                       component('Camera', isActive='true', fov='60',
                                                 nearClip='0.25', farClip='480')])
        (source / 'forest.fbscene').write_text(json.dumps(dict(children=[terrain_actor, camera_actor])),
                                              encoding='utf-8')
        extensions = {'fbscene', 'terrain', 'folworld', 'folpage', 'folspec', 'texres'}
        entries = [(identity(path.relative_to(source).as_posix()), path.relative_to(source).as_posix(), path.suffix[1:])
                   for path in sorted(source.rglob('*')) if path.is_file() and path.suffix[1:] in extensions]
        with closing(sqlite3.connect(catalog)) as database, database:
            database.execute('CREATE TABLE resources(id INTEGER PRIMARY KEY,uuid TEXT,path TEXT,type TEXT)')
            database.executemany('INSERT INTO resources(uuid,path,type) VALUES(?,?,?)', entries)
        args = [source, catalog, cooked, installed, 'pc', '--root', scene_uuid,
                '--build-profile', 'foliage-acceptance', '--toolchain', 'installed-cooker']
        for extension in sorted(extensions):
            args += ['--map', extension + '=' + extension]
        run(tool, args, 'cook')

        # The checked package lives outside this disposable source and cache root.
        for directory in (source, cooked):
            if not directory.resolve().is_relative_to(root):
                raise RuntimeError(f'Refusing to remove a path outside the fixture root: {directory}')
            shutil.rmtree(directory)
        catalog.unlink()
        if any(path.exists() for path in (source, cooked, catalog)):
            raise RuntimeError('The native consumers require the source, catalog and cache to be absent')

        before = inventory(installed)
        try:
            for path in before:
                (installed / path).chmod(0o444)
            run(tool, ['--verify', installed, 'pc', *[entry[0] for entry in entries]], 'verify')
            report = None
            if editor:
                workspace = root / 'editor workspace'
                workspace.mkdir()
                lines = run(editor, ['--interactive', '--dx11', '--package-root', editor.parent.parent,
                                    '--workspace', workspace, '--runtime-scene-package', installed,
                                    '--runtime-scene-uuid', scene_uuid, '--runtime-scene-target', 'pc',
                                    '--runtime-scene-validate'], 'scene-bootstrap', workspace)
                reports = [json.loads(line.split('=', 1)[1]) for line in lines
                           if line.startswith('RuntimeSceneBootstrap=')]
                if len(reports) != 1 or reports[0]['status'] != 'loaded' or reports[0]['components'] != 2:
                    raise RuntimeError(f'Installed scene must bind its native terrain and camera: {reports}')
                report = reports[0]
            lines = run(host, ['--installed-foliage-consume', installed, forest_uuid, captures], 'flight')
            frames = [line for line in lines if line.startswith('FOLIAGE_FRAME,')]
            if (len(frames) != 65 or 'FOLIAGE_INSTALLED,64,11264' not in lines or
                    not any(line.startswith('PASS: installed representative foliage') for line in lines)):
                raise RuntimeError('Installed forest flight evidence is incomplete')
            if before != inventory(installed):
                raise RuntimeError('The installed package bytes or membership changed during validation')
            if binaries != {str(path): hash_file(path) for path in sorted(programs)}:
                raise RuntimeError('A consumer binary changed during validation')
        finally:
            for path in before:
                (installed / path).chmod(0o666)

    evidence = dict(schema='workphone.installed-foliage.v1', target='pc', sceneUuid=scene_uuid,
                    foliageUuid=forest_uuid, cells=64, trees=2048, grassClumps=9216, frames=len(frames),
                    sourceRemoved=True, packageStable=True, rendererRecreated=True, sceneBootstrap=report,
                    adapter=[line for line in lines if line.startswith('DX11 ')], releaseCertified=False,
                    binaries=binaries, package=before, captures=inventory(captures))
    (output / 'evidence.json').write_text(json.dumps(evidence, indent=2) + '\n', encoding='utf-8')
    print(f'PASS: source-free installed forest, {len(frames)} frames, evidence at {output}')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('host', type=Path, help='WPGraphicsCookedResourceTests.exe')
    parser.add_argument('tool', type=Path, help='WorkphoneResourcePackage.exe')
    parser.add_argument('output', type=Path, help='fresh evidence directory')
    parser.add_argument('--editor', type=Path, help='optional installed Editor.exe under the product bin directory')
    validate(parser.parse_args())
