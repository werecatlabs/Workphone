"""Run the product Editor against a checked, source-free mixed scene package."""
import json
import pathlib
import shutil
import sqlite3
import struct
import subprocess
import sys
import tempfile
import uuid
from contextlib import closing

editor, tool = (pathlib.Path(value).resolve() for value in sys.argv[1:3])
product = editor.parent.parent

def run(executable, args, cwd, expected=0):
    result = subprocess.run([str(executable), *map(str, args)], cwd=cwd,
                            capture_output=True, text=True, timeout=90)
    assert result.returncode == expected, (result.returncode, result.stdout, result.stderr)
    return result

with tempfile.TemporaryDirectory(prefix="wp-mixed-scene-runtime-") as temporary:
    root = pathlib.Path(temporary)
    source, cooked, package = (root / name for name in ("source", "cooked", "runtime package"))
    source.mkdir()
    vertices = [(-.2, -.2), (.2, -.2), (0, .2)]
    geometry = struct.pack('<4s7I6f', b'WPM1', 1, 3, 1, 0, 3, 3, 1, -.2, -.2, .5, .2, .2, .5)
    geometry += b''.join(struct.pack('<8f', x, y, .5, 0, 0, -1, .5, .5) for x, y in vertices)
    (source/'character.mesh').write_bytes(geometry + struct.pack('<6I', 0, 1, 2, 0, 3, 0))
    (source/'character.mesh.materials').write_text('["data://surface.matres"]')
    (source/'character.skinres').write_text(json.dumps(dict(version=1, mesh='data://character.mesh',
        skeleton='data://rig.skelres', vertices=[dict(joints=[0,0,0,0], weights=[1,0,0,0]) for _ in vertices])))
    # Native typed material uploads require DX11. Exercise the Software geometry/
    # skin/component lifecycle with a separate material-free mesh in the same mount.
    (source/'plain.mesh').write_bytes((source/'character.mesh').read_bytes())
    plain_skin = json.loads((source/'character.skinres').read_text())
    plain_skin['mesh'] = 'data://plain.mesh'
    (source/'plain.skinres').write_text(json.dumps(plain_skin))
    (source/'rig.skelres').write_text('<skeleton><bones><bone id="0" name="Root"/></bones>'
        '<animations><animation name="Move" length="1"><tracks><track bone="Root" handle="0"><keyframes>'
        '<keyframe time="0"><translate x="0" y="0" z="0"/></keyframe>'
        '<keyframe time="1"><translate x="2" y="0" z="0"/></keyframe>'
        '</keyframes></track></tracks></animation></animations></skeleton>')
    image = bytearray(18); image[2] = 2; image[12] = image[14] = 2; image[16] = 32; image[17] = 0x28
    (source/'leaf.tga').write_bytes(image + bytes([0,255,0,255])*4)
    (source/'leaf.texres').write_text(json.dumps(dict(version=1, source='data://leaf.tga', mipFilter='colour', atlasColumns=1, alphaCutoff=.5)))
    (source/'surface.matres').write_text(json.dumps(dict(version=1, baseColour=[1,1,1,1], metalness=0, roughness=1, baseColourTexture='data://leaf.texres')))
    (source/'Helper.lua').write_text('return {amount=42}')
    (source/'Actor.lua.deps').write_text('data://Helper.lua\n')
    (source/'Actor.lua').write_text("local helper=require('Helper'); class 'MixedRuntimeActor'; function MixedRuntimeActor:__init(owner) self.owner=owner end; function MixedRuntimeActor:update() mixedRuntimeTicks=(mixedRuntimeTicks or 0)+helper.amount end")
    (source/'mixed.fbscene').write_text('{}')
    (source/'plain.fbscene').write_text('{}')
    extensions = {'mesh','skinres','skelres','texres','matres','lua','fbscene'}
    entries = [(str(uuid.uuid4()), p.name, p.suffix[1:]) for p in source.iterdir() if p.suffix[1:] in extensions]
    bindings = {name: identity for identity, name, _ in entries}
    def component(name, **fields):
        return dict(handle_name='component', componentType='workphone::scene::'+name, uuid=str(uuid.uuid4()), children=[], **fields)
    actor = dict(handle_name='actors', uuid=str(uuid.uuid4()), name='MixedRuntimeActor', children=[
        component('Mesh', **{'Mesh Resource':bindings['character.skinres'], 'Skeleton':bindings['rig.skelres']}),
        component('MeshRenderer'), component('Animator', autoPlay='false', animationTime='0.25'),
        component('Script', scriptAssetUuid=bindings['Actor.lua'], className='MixedRuntimeActor', updateInEditMode='true')])
    (source/'mixed.fbscene').write_text(json.dumps(dict(children=[actor])))
    plain_actor = json.loads(json.dumps(actor))
    plain_actor['uuid'] = str(uuid.uuid4())
    for value in plain_actor['children']:
        value['uuid'] = str(uuid.uuid4())
        if value['componentType'] == 'workphone::scene::Mesh':
            value['Mesh Resource'] = bindings['plain.skinres']
    (source/'plain.fbscene').write_text(json.dumps(dict(children=[plain_actor])))
    catalog = root/'catalog.db'
    with closing(sqlite3.connect(catalog)) as database, database:
        database.execute('CREATE TABLE resources(id INTEGER PRIMARY KEY,uuid TEXT,path TEXT,type TEXT)')
        database.executemany('INSERT INTO resources(uuid,path,type) VALUES(?,?,?)', entries)
    args = [source, catalog, cooked, package, 'pc', '--root', bindings['mixed.fbscene'], '--root', bindings['plain.fbscene'],
            '--build-profile', 'mixed-runtime', '--toolchain', 'installed-cooker']
    for extension in sorted(extensions): args += ['--map', extension+'='+extension]
    run(tool, args, tool.parent)
    shutil.rmtree(source); shutil.rmtree(cooked); catalog.unlink()
    before = {p.relative_to(package):p.read_bytes() for p in package.rglob('*') if p.is_file()}
    try:
        for path in before: (package/path).chmod(0o444)
        workspace = root/'Editor workspace with spaces'; workspace.mkdir()
        args = ['--interactive','--package-root',product,'--workspace',workspace,
                '--runtime-scene-package',package,'--runtime-scene-uuid',bindings['mixed.fbscene'],
                '--runtime-scene-target','pc','--runtime-scene-validate']
        launches = []
        for renderer, scene in (('--dx11', 'mixed.fbscene'), ('--software', 'plain.fbscene')):
            selected = [*args, renderer]
            selected[selected.index('--runtime-scene-uuid')+1] = bindings[scene]
            loaded = run(editor, selected, workspace)
            assert 'Runtime scene package loaded' in loaded.stdout, (loaded.stdout,loaded.stderr)
            report = json.loads(next(line.split('=', 1)[1] for line in loaded.stdout.splitlines()
                                     if line.startswith('RuntimeSceneBootstrap=')))
            assert {key: report[key] for key in ('status', 'actors', 'components', 'scripts', 'meshes')} == {
                'status': 'loaded', 'actors': 1, 'components': 4, 'scripts': 1, 'meshes': 1}, report
            launches.append((renderer, loaded))
        unavailable = run(editor, [*args, '--software'], workspace, expected=1)
        assert 'active DX11 device' in unavailable.stdout, (unavailable.stdout, unavailable.stderr)
        report = json.loads(next(line.split('=', 1)[1] for line in unavailable.stdout.splitlines()
                                 if line.startswith('RuntimeSceneBootstrap=')))
        assert report['status'] == 'failed', report
        args[args.index('--runtime-scene-uuid')+1] = str(uuid.uuid4())
        rejected = subprocess.run([str(editor), *map(str,args)], cwd=workspace,
                                  capture_output=True, text=True, timeout=90)
        assert rejected.returncode == 1, (rejected.returncode,rejected.stdout,rejected.stderr)
        report = json.loads(next(line.split('=', 1)[1] for line in rejected.stdout.splitlines()
                                 if line.startswith('RuntimeSceneBootstrap=')))
        assert report['status'] == 'failed' and 'UUID' in report['diagnostic'], report
        assert before == {p.relative_to(package):p.read_bytes() for p in package.rglob('*') if p.is_file()}
        print('PASS: product Editor textured DX11 and material-free Software async mixed scenes, explicit Software material rejection, missing UUID rejection, source/catalog/index absent and read-only package preserved')
        for renderer, loaded in launches:
            print(renderer, loaded.stdout)
    finally:
        for path in before: (package/path).chmod(0o666)
