"""Run the product Editor against a checked, source-free mixed scene package."""
import json
import argparse
import pathlib
import shutil
import sqlite3
import struct
import subprocess
import sys
import tempfile
import uuid
from contextlib import closing

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('editor', type=pathlib.Path)
parser.add_argument('tool', type=pathlib.Path)
parser.add_argument('--retain-fixture', type=pathlib.Path)
options = parser.parse_args()
editor, tool = options.editor.resolve(strict=True), options.tool.resolve(strict=True)
retained = None
if options.retain_fixture:
    requested = options.retain_fixture.absolute()
    if requested.exists() or requested.is_symlink():
        parser.error('--retain-fixture requires a fresh directory')
    retained = requested.resolve()
product = editor.parent.parent

def run(executable, args, cwd, expected=0):
    result = subprocess.run([str(executable), *map(str, args)], cwd=cwd,
                            capture_output=True, text=True, timeout=90)
    assert result.returncode == expected, (result.returncode, result.stdout, result.stderr)
    return result

def bootstrap_report(result):
    reports = [line.split('=', 1)[1] for line in result.stdout.splitlines()
               if line.startswith('RuntimeSceneBootstrap=')]
    assert len(reports) == 1, (result.returncode, result.stdout, result.stderr)
    return json.loads(reports[0])

def package_bytes(directory):
    return {path.relative_to(directory): path.read_bytes()
            for path in directory.rglob('*') if path.is_file()}

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
    (source/'root.amask').write_text(json.dumps(dict(version=1, rig='data://rig.skelres',
        defaultWeight=1, bones=[dict(bone='Root', weight=.5)])))
    (source/'actor.agraph').write_text(json.dumps(dict(version=1, rig='data://rig.skelres',
        mask='data://root.amask', initialState='Move',
        clips=[dict(id='move', clip='Move', speed=1, loop=True)], parameters=[],
        states=[dict(id='Move', clip='move', speed=1)], transitions=[])))
    image = bytearray(18); image[2] = 2; image[12] = image[14] = 128; image[16] = 32; image[17] = 0x28
    (source/'leaf.tga').write_bytes(image + bytes([0,255,0,255])*128*128)
    (source/'leaf.texres').write_text(json.dumps(dict(version=1, source='data://leaf.tga', mipFilter='colour', atlasColumns=1, alphaCutoff=.5)))
    (source/'surface.matres').write_text(json.dumps(dict(version=1, baseColour=[1,1,1,1], metalness=0, roughness=1, baseColourTexture='data://leaf.texres')))
    (source/'Helper.lua').write_text('return {amount=42}')
    (source/'Actor.lua.deps').write_text('data://Helper.lua\n')
    (source/'Actor.lua').write_text("local helper=require('Helper'); class 'MixedRuntimeActor'; function MixedRuntimeActor:__init(owner) self.owner=owner end; function MixedRuntimeActor:update() mixedRuntimeTicks=(mixedRuntimeTicks or 0)+helper.amount end")
    atlas = bytearray(18); atlas[2] = 2; atlas[12] = atlas[14] = 16; atlas[16] = 32; atlas[17] = 0x28
    (source/'atlas.tga').write_bytes(atlas + bytes([255,255,255,255])*16*16)
    (source/'atlas.texres').write_text(json.dumps(dict(version=1, source='data://atlas.tga', mipFilter='none', atlasColumns=1, alphaCutoff=.5)))
    (source/'face.fontres').write_text(json.dumps(dict(version=1, atlas='data://atlas.texres', height=16,
        ascent=12, descent=-4, fallback=63, whitePixel=[0,0], glyphs=[dict(codepoint=codepoint, advance=advance,
        bounds=[0,0,8,16], uv=[0,0,.5,1]) for codepoint, advance in ((63,8),(65,9))])))
    (source/'hud.uistyle').write_text(json.dumps(dict(version=1, styles=[dict(name='hud', colour=[.25,.5,.75,1], textSize=23)])))
    (source/'hud.locres').write_text(json.dumps(dict(version=1, fallback='en', locales={
        'en': {'greeting':'Package ready', 'confirm':'Confirm'},
        'fr': {'greeting':'Bonjour capturé', 'confirm':'Confirmer'}})))
    (source/'layout.uires').write_text('{}')
    (source/'plain.uires').write_text(json.dumps(dict(version=1,nodes=[dict(type='container',id=1,parent=-1,label='PackagedUI',position=[.02,.04],size=[.4,.2],visible=True,enabled=True),dict(type='text',id=2,parent=0,label='Package ready',position=[.02,.04],size=[.3,.1],visible=True,enabled=True)])))
    pcm = bytes(2*1024*1024)
    (source/'voice.wav').write_bytes(struct.pack('<4sI4s4sIHHIIHH4sI',b'RIFF',len(pcm)+36,b'WAVE',b'fmt ',16,1,1,48000,96000,2,16,b'data',len(pcm))+pcm)
    (source/'mixed.fbscene').write_text('{}')
    (source/'plain.fbscene').write_text('{}')
    (source/'audio.fbscene').write_text('{}')
    extensions = {'mesh','skinres','skelres','agraph','amask','texres','matres','lua','fbscene','uires','fontres','uistyle','locres','wav'}
    entries = [(str(uuid.uuid4()), p.name, p.suffix[1:]) for p in source.iterdir() if p.suffix[1:] in extensions]
    bindings = {name: identity for identity, name, _ in entries}
    (source/'layout.uires').write_text(json.dumps(dict(version=2,
        styleAssetUuid=bindings['hud.uistyle'], localizationAssetUuid=bindings['hud.locres'], locale='en', nodes=[
        dict(type='container',id=1,parent=-1,label='Packaged dependency UI',position=[.02,.04],size=[.4,.3],visible=True,enabled=True),
        dict(type='text',id=2,parent=0,label='Source greeting',localeKey='greeting',styleClass='hud',fontAssetUuid=bindings['face.fontres'],position=[.02,.02],size=[.36,.08],visible=True,enabled=True),
        dict(type='image',id=3,parent=0,label='Captured image',textureAssetUuid=bindings['leaf.texres'],position=[.02,.12],size=[.1,.1],visible=True,enabled=True),
        dict(type='button',id=4,parent=0,label='Source confirm',localeKey='confirm',fontAssetUuid=bindings['face.fontres'],position=[.14,.12],size=[.2,.08],visible=True,enabled=True)])))
    def component(name, **fields):
        return dict(handle_name='component', componentType='workphone::scene::'+name, uuid=str(uuid.uuid4()), children=[], **fields)
    actor = dict(handle_name='actors', uuid=str(uuid.uuid4()), name='MixedRuntimeActor', children=[
        component('Mesh', **{'Mesh Resource':bindings['character.skinres'], 'Skeleton':bindings['rig.skelres']}),
        component('MeshRenderer'), component('Animator', animationAssetUuid=bindings['actor.agraph'], autoPlay='false', animationTime='0.25'),
        component('Script', scriptAssetUuid=bindings['Actor.lua'], className='MixedRuntimeActor', updateInEditMode='true'),
        component('Material', **{'Material':bindings['surface.matres'], 'index':'0'}),
        component('Skybox', **{face:bindings['leaf.texres'] for face in ('Front','Back','Up','Down','Right','Left')}),
        component('Camera', isActive='false', fov='60', nearClip='0.1', farClip='100'),
        component('Light', lightType='1', intensity='1', range='10'),
        component('UIComponent', uiAssetUuid=bindings['layout.uires'], uiLocale='fr')])
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
        if value['componentType'] == 'workphone::scene::UIComponent':
            value['uiAssetUuid'] = bindings['plain.uires']
            value.pop('uiLocale', None)
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
    overlay = root/'runtime overlay package'
    shutil.copytree(package, overlay)
    before = {directory: package_bytes(directory) for directory in (package, overlay)}
    try:
        for directory, files in before.items():
            for path in files: (directory/path).chmod(0o444)
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
            report = bootstrap_report(loaded)
            assert {key: report[key] for key in ('status', 'actors', 'components', 'scripts', 'meshes', 'residentMeshes', 'streamingMeshes')} == {
                'status': 'loaded', 'actors': 4, 'components': 12 if renderer=='--dx11' else 10,
                'scripts': 1, 'meshes': 3, 'residentMeshes': 2, 'streamingMeshes': 1}, report
            assert report['mountCount'] == 1 and report['rootMount'] == 'base' and report['rootShadowedMounts'] == 0, report
            launches.append((renderer, loaded))
        composed_args = [*args, '--dx11', '--runtime-scene-overlay', overlay]
        duplicate = bootstrap_report(run(editor, composed_args, workspace, expected=1))
        assert duplicate['status'] == 'failed' and duplicate['mountCount'] == 2 and 'collision' in duplicate['diagnostic'].lower(), duplicate
        for policy in ('identical', 'prefer-earlier'):
            loaded = run(editor, [*composed_args, '--runtime-scene-collisions', policy], workspace)
            report = bootstrap_report(loaded)
            assert report['status'] == 'loaded' and report['mountCount'] == 2 and report['rootMount'] == 'overlay-0' and report['rootShadowedMounts'] == 1, report
            assert report['actors'] == 4 and report['components'] == 12 and report['scripts'] == 1 and report['residentMeshes'] == 2 and report['streamingMeshes'] == 1, report
            launches.append(('composition '+policy, loaded))
        unavailable = run(editor, [*args, '--software'], workspace, expected=1)
        report = bootstrap_report(unavailable)
        assert report['status'] == 'failed' and 'active DX11' in report['diagnostic'], report
        audio_args=[*args,'--dx11'];audio_args[audio_args.index('--runtime-scene-uuid')+1]=bindings['audio.fbscene']
        audio=subprocess.run([str(editor),*map(str,audio_args)],cwd=workspace,capture_output=True,text=True,timeout=90)
        report=bootstrap_report(audio)
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
        report = bootstrap_report(rejected)
        assert report['status'] == 'failed' and 'UUID' in report['diagnostic'], report
        assert before == {directory: package_bytes(directory) for directory in (package, overlay)}
        if retained:
            # Publish only after every original source-free/read-only check has
            # passed. The retained inputs contain no authoring or build stores.
            def counts(components, actors=4, scripts=1, meshes=3, resident=2, streaming=1):
                return dict(actors=actors, components=components, scripts=scripts, meshes=meshes,
                            residentMeshes=resident, streamingMeshes=streaming)
            fixture = dict(schema='workphone.mixed-scene-fixture.v1', target='pc', scenes={
                'dx11': dict(uuid=bindings['mixed.fbscene'], expected=counts(12)),
                'software': dict(uuid=bindings['plain.fbscene'], expected=counts(10)),
                'audio': dict(uuid=bindings['audio.fbscene'], expected=counts(1, 1, 0, 0, 0, 0))})
            encoded = json.dumps(fixture, ensure_ascii=False, allow_nan=False, sort_keys=True).encode('utf-8')
            assert len(encoded) <= 65536
            retained.mkdir(parents=True, exist_ok=False)
            shutil.copytree(package, retained/'package')
            shutil.copytree(overlay, retained/'overlay')
            (retained/'fixture.json').write_bytes(encoded)
            (retained/'fixture.json').chmod(0o444)
            assert package_bytes(retained/'package') == before[package]
            assert package_bytes(retained/'overlay') == before[overlay]
            assert {path.name for path in retained.iterdir()} == {'fixture.json', 'package', 'overlay'}
            print('RETAINED: source-free mixed scene fixture', retained)
        print('PASS: product Editor source-free styled/localized/font/image DX11 UI and v1 Software UI; duplicate package reject/identical/prefer-earlier policies and overlay provenance; explicit Software material and missing UUID rejection; both read-only packages preserved')
        for renderer, loaded in launches:
            print(renderer, loaded.stdout)
    finally:
        for directory, files in before.items():
            for path in files: (directory/path).chmod(0o666)
