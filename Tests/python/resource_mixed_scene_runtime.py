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
    (source/'coarse.mesh').write_bytes((source/'character.mesh').read_bytes())
    high = struct.pack('<4s7I6f', b'WPM1', 1, 3, 1, 0, 3072, 3072, 1, -.2, -.2, .5, .2, .2, .5)
    high += geometry[56:] * 1024
    high += struct.pack('<3075I', *range(3072), 0, 3072, 0)
    (source/'high.mesh').write_bytes(high)
    plain_skin = json.loads((source/'character.skinres').read_text())
    plain_skin['mesh'] = 'data://plain.mesh'
    (source/'plain.skinres').write_text(json.dumps(plain_skin))
    (source/'rig.skelres').write_text('<skeleton><bones><bone id="0" name="Root"/></bones>'
        '<animations><animation name="Move" length="1"><tracks><track bone="Root" handle="0"><keyframes>'
        '<keyframe time="0"><translate x="0" y="0" z="0"/></keyframe>'
        '<keyframe time="1"><translate x="2" y="0" z="0"/></keyframe>'
        '</keyframes></track></tracks></animation></animations></skeleton>')
    image = bytearray(18); image[2] = 2; image[12] = image[14] = 128; image[16] = 32; image[17] = 0x28
    (source/'leaf.tga').write_bytes(image + bytes([0,255,0,255])*128*128)
    (source/'leaf.texres').write_text(json.dumps(dict(version=1, source='data://leaf.tga', mipFilter='colour', atlasColumns=1, alphaCutoff=.5)))
    (source/'surface.matres').write_text(json.dumps(dict(version=1, baseColour=[1,1,1,1], metalness=0, roughness=1, baseColourTexture='data://leaf.texres')))
    (source/'Helper.lua').write_text('return {amount=42}')
    (source/'Actor.lua.deps').write_text('data://Helper.lua\n')
    (source/'Actor.lua').write_text("local helper=require('Helper'); class 'MixedRuntimeActor'; function MixedRuntimeActor:__init(owner) self.owner=owner end; function MixedRuntimeActor:update() mixedRuntimeTicks=(mixedRuntimeTicks or 0)+helper.amount end")
    (source/'layout.uires').write_text(json.dumps(dict(version=1,nodes=[dict(type='container',id=1,parent=-1,label='PackagedUI',position=[.02,.04],size=[.4,.2],visible=True,enabled=True),dict(type='text',id=2,parent=0,label='Package ready',position=[.02,.04],size=[.3,.1],visible=True,enabled=True)])))
    pcm = bytes(2*1024*1024)
    (source/'voice.wav').write_bytes(struct.pack('<4sI4s4sIHHIIHH4sI',b'RIFF',len(pcm)+36,b'WAVE',b'fmt ',16,1,1,48000,96000,2,16,b'data',len(pcm))+pcm)
    (source/'mixed.fbscene').write_text('{}')
    (source/'plain.fbscene').write_text('{}')
    (source/'audio.fbscene').write_text('{}')
    extensions = {'mesh','skinres','skelres','texres','matres','lua','fbscene','uires','wav'}
    entries = [(str(uuid.uuid4()), p.name, p.suffix[1:]) for p in source.iterdir() if p.suffix[1:] in extensions]
    bindings = {name: identity for identity, name, _ in entries}
    def component(name, **fields):
        return dict(handle_name='component', componentType='workphone::scene::'+name, uuid=str(uuid.uuid4()), children=[], **fields)
    actor = dict(handle_name='actors', uuid=str(uuid.uuid4()), name='MixedRuntimeActor', children=[
        component('Mesh', **{'Mesh Resource':bindings['character.skinres'], 'Skeleton':bindings['rig.skelres']}),
        component('MeshRenderer'), component('Animator', autoPlay='false', animationTime='0.25'),
        component('Script', scriptAssetUuid=bindings['Actor.lua'], className='MixedRuntimeActor', updateInEditMode='true'),
        component('Material', **{'Material':bindings['surface.matres'], 'index':'0'}),
        component('Skybox', **{face:bindings['leaf.texres'] for face in ('Front','Back','Up','Down','Right','Left')}),
        component('Camera', isActive='false', fov='60', nearClip='0.1', farClip='100'),
        component('Light', lightType='1', intensity='1', range='10'),
        component('UIComponent', uiAssetUuid=bindings['layout.uires'])])
    def lod_actor():
        children, levels = [], []
        for index, name in enumerate(('high.mesh', 'coarse.mesh')):
            renderer = component('MeshRenderer', meshAssetUuid=bindings[name], streamMesh='true' if index == 0 else 'false')
            children.append(dict(handle_name='child', uuid=str(uuid.uuid4()), name='PackagedLOD'+str(index), children=[renderer]))
            levels.append(dict(handle_name='lodLevel', screenRelativeHeight='0.5' if index == 0 else '0',
                               fadeTransitionWidth='0', renderers=renderer['uuid']+';', children=[]))
        group = component('LODGroup', lodLevels='2', size='2', forcedLOD='1')
        group['children'] = levels
        return dict(handle_name='actors', uuid=str(uuid.uuid4()), name='PackagedLOD', children=[group, *children])
    (source/'mixed.fbscene').write_text(json.dumps(dict(children=[actor, lod_actor()])))
    plain_actor = json.loads(json.dumps(actor))
    plain_actor['uuid'] = str(uuid.uuid4())
    plain_actor['children'] = [value for value in plain_actor['children'] if value['componentType'] not in ('workphone::scene::Material','workphone::scene::Skybox')]
    for value in plain_actor['children']:
        value['uuid'] = str(uuid.uuid4())
        if value['componentType'] == 'workphone::scene::Mesh':
            value['Mesh Resource'] = bindings['plain.skinres']
    (source/'plain.fbscene').write_text(json.dumps(dict(children=[plain_actor, lod_actor()])))
    audio_actor = dict(handle_name='actors',uuid=str(uuid.uuid4()),name='PackagedAudio',children=[component('AudioEmitter',Sound=bindings['voice.wav'])])
    (source/'audio.fbscene').write_text(json.dumps(dict(children=[audio_actor])))
    catalog = root/'catalog.db'
    with closing(sqlite3.connect(catalog)) as database, database:
        database.execute('CREATE TABLE resources(id INTEGER PRIMARY KEY,uuid TEXT,path TEXT,type TEXT)')
        database.executemany('INSERT INTO resources(uuid,path,type) VALUES(?,?,?)', entries)
    args = [source, catalog, cooked, package, 'pc', '--root', bindings['mixed.fbscene'], '--root', bindings['plain.fbscene'], '--root', bindings['audio.fbscene'],
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
            assert {key: report[key] for key in ('status', 'actors', 'components', 'scripts', 'meshes', 'residentMeshes', 'streamingMeshes')} == {
                'status': 'loaded', 'actors': 4, 'components': 12 if renderer=='--dx11' else 10,
                'scripts': 1, 'meshes': 3, 'residentMeshes': 2, 'streamingMeshes': 1}, report
            launches.append((renderer, loaded))
        unavailable = run(editor, [*args, '--software'], workspace, expected=1)
        report = json.loads(next(line.split('=', 1)[1] for line in unavailable.stdout.splitlines()
                                 if line.startswith('RuntimeSceneBootstrap=')))
        assert report['status'] == 'failed' and 'active DX11' in report['diagnostic'], report
        audio_args=[*args,'--dx11'];audio_args[audio_args.index('--runtime-scene-uuid')+1]=bindings['audio.fbscene']
        audio=subprocess.run([str(editor),*map(str,audio_args)],cwd=workspace,capture_output=True,text=True,timeout=90)
        report=json.loads(next(line.split('=',1)[1] for line in audio.stdout.splitlines() if line.startswith('RuntimeSceneBootstrap=')))
        if audio.returncode==0:
            assert report['status']=='loaded' and report['components']==1,report
            print('PASS: native packaged AudioEmitter creates its streamed voice')
        else:
            assert audio.returncode==1 and report['status']=='failed' and 'Runtime audio output device is unavailable' in report['diagnostic'],(audio.returncode,audio.stdout,audio.stderr)
            print('UNAVAILABLE: native audio output device; packaged AudioEmitter reported device unavailability and retired cleanly')
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
