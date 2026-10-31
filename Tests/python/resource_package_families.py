"""Checked catalog export through existing terrain/foliage typed consumers."""
import pathlib
import shutil
import sqlite3
import subprocess
import sys
import tempfile
import uuid
import struct
import json
from contextlib import closing

host, tool = map(lambda p: pathlib.Path(p).resolve(), sys.argv[1:3])

def run(executable, args):
    result = subprocess.run([str(executable), *map(str, args)], cwd=executable.parent,
                            capture_output=True, text=True, timeout=90)
    assert result.returncode == 0, (result.returncode, result.stdout, result.stderr)
    for line in result.stdout.splitlines():
        if line.startswith(("DX11 ", "PASS:")):
            print(line)
    return result

with tempfile.TemporaryDirectory(prefix="wp-typed-packages-") as temporary:
    root = pathlib.Path(temporary)
    for family in ("terrain", "foliage", "mesh", "mesh_materials", "skeleton", "skin", "scene", "prefab", "ui"):
        folder = root / family
        folder.mkdir()
        if family == "terrain":
            run(host, ["--terrain-package-export", folder])
            source = folder / "terrain-package" / "authoring-offline"
            root_name, target = "ground.terrain", "terrain-dx11-contract"
            extensions = {"terrain", "texres"}
        elif family == "foliage":
            source = folder / "authoring"
            run(host, ["--foliage-source-export", source])
            root_name, target = "forest.folworld", "windows-dx11-dds-bgra8-v1"
            extensions = {"folworld", "folpage", "folspec", "texres"}
        elif family in ("skeleton", "skin"):
            source=folder / "authoring"; source.mkdir()
            root_name,target,extensions="rig.skelres","pc",{"skelres"}
            (source / root_name).write_text('<skeleton><bones><bone id="0" name="Root"><position x="0" y="0" z="0"/>'
                '</bone><bone id="1" name="Child"><position x="0" y="1" z="0"/></bone></bones>'
                '<bonehierarchy><boneparent bone="Child" parent="Root"/></bonehierarchy>'
                '<animations><animation name="Move" length="1"><tracks><track bone="Root" handle="0"><keyframes>'
                '<keyframe time="0"><translate x="0" y="0" z="0"/></keyframe>'
                '<keyframe time="1"><translate x="2" y="0" z="0"/></keyframe>'
                '</keyframes></track></tracks></animation></animations></skeleton>')
            if family == "skin":
                root_name="character.skinres"; extensions={"skinres","mesh","skelres","matres","texres"}
                positions=[(-.2,-.2),(.2,-.2),(0,.2)]
                mesh=struct.pack('<4s7I6f',b'WPM1',1,3,1,0,3,3,1,-.2,-.2,.5,.2,.2,.5)
                mesh+=b''.join(struct.pack('<8f',x,y,.5,0,0,-1,.5,.5) for x,y in positions)
                mesh+=struct.pack('<6I',0,1,2,0,3,42)
                (source / 'character.mesh').write_bytes(mesh)
                (source / 'character.mesh.materials').write_text('["data://surface.matres"]')
                influences=[dict(joints=[0,0,0,0],weights=[1,0,0,0]) for _ in positions]
                (source / root_name).write_text(json.dumps(dict(version=1,mesh='data://character.mesh',
                    skeleton='data://rig.skelres',vertices=influences)))
                header=bytearray(18);header[2]=2;header[12]=header[14]=2;header[16]=32;header[17]=0x28
                (source / 'leaf.tga').write_bytes(header+bytes([0,255,0,255])*4)
                (source / 'leaf.texres').write_text(json.dumps(dict(version=1,source='data://leaf.tga',mipFilter='colour',atlasColumns=1,alphaCutoff=.5)))
                (source / 'surface.matres').write_text(json.dumps(dict(version=1,baseColour=[1,1,1,1],metalness=0,roughness=1,baseColourTexture='data://leaf.texres')))
        elif family in ("scene", "prefab"):
            source=folder / "authoring"; source.mkdir()
            extension="fbscene" if family=="scene" else "prefab"
            root_name,target,extensions="graph."+extension,"pc",{extension}
            actor=dict(handle_name="actors",uuid="f846b5f3-d6da-4c9c-984a-e739b61759f3",name="Packaged actor",
                children=[dict(handle_name="localTransform",Position="2 3 4",Scale="1 1 1",Orientation="1 0 0 0",children=[])])
            (source / root_name).write_text(json.dumps(dict(children=[actor]) if family=="scene" else actor))
        elif family == "ui":
            source=folder / "authoring"; source.mkdir()
            root_name,target,extensions="hud.uires","pc",{"uires"}
            nodes=[dict(type="window",id=1,parent=-1,label="Packaged UI",position=[0,0],size=[320,180],visible=True,enabled=True),
                dict(type="text",id=2,parent=0,label="Source-free text",position=[10,10],size=[100,20],visible=True,enabled=True,textSize=18)]
            (source / root_name).write_text(json.dumps(dict(version=1,nodes=nodes)))
        else:
            source=folder / "authoring"
            source.mkdir()
            root_name, target, extensions = "triangle.mesh", "pc", {"mesh"}
            if family == "mesh":
                (source / root_name).write_bytes(struct.pack('<4s7I6f',b'WPM1',1,0,1,0,3,3,1,
                    0,0,0,1,1,0)+struct.pack('<9f6I',0,0,0,1,0,0,0,1,0,0,1,2,0,3,0))
            else:
                extensions={"mesh","matres","texres"}
                vertices=[(-.9,-.8),(-.1,-.8),(-.5,.8),(.1,-.8),(.9,-.8),(.5,.8)]
                geometry=struct.pack('<4s7I6f',b'WPM1',1,3,1,0,6,6,2,-.9,-.8,.5,.9,.8,.5)
                geometry+=b''.join(struct.pack('<8f',x,y,.5,0,0,-1,.5,.5) for x,y in vertices)
                geometry+=struct.pack('<12I',0,1,2,3,4,5,0,3,99123,3,3,99456)
                (source / root_name).write_bytes(geometry)
                (source / (root_name+'.materials')).write_text(json.dumps(['data://red.matres','data://green.matres']))
                for color,pixel in [('red',bytes([0,0,255,255])),('green',bytes([0,255,0,255]))]:
                    header=bytearray(18); header[2]=2; header[12]=header[14]=2; header[16]=32; header[17]=0x28
                    (source / (color+'.tga')).write_bytes(header+pixel*4)
                    (source / (color+'.texres')).write_text(json.dumps(dict(version=1,source='data://'+color+'.tga',
                        mipFilter='colour',atlasColumns=1,alphaCutoff=.5)))
                    (source / (color+'.matres')).write_text(json.dumps(dict(version=1,baseColour=[1,1,1,1],
                        metalness=0,roughness=1,baseColourTexture='data://'+color+'.texres')))
        entries = [(str(uuid.uuid4()), p.relative_to(source).as_posix(), p.suffix[1:])
                   for p in source.rglob("*") if p.is_file() and p.suffix[1:] in extensions]
        root_uuid = next(identity for identity, path, kind in entries if path == root_name)
        catalog = folder / "catalog.db"
        with closing(sqlite3.connect(catalog)) as db, db:
            db.execute("CREATE TABLE resources(id INTEGER PRIMARY KEY,uuid TEXT,path TEXT,type TEXT)")
            db.executemany("INSERT INTO resources(uuid,path,type) VALUES(?,?,?)", entries)
        package, cooked = folder / "checked-package", folder / "checked-cooked"
        args = [source, catalog, cooked, package, target, "--root", root_uuid,
                "--build-profile", "family-shipping", "--toolchain", "family-contract-tools"]
        for extension in sorted(extensions):
            args += ["--map", extension + "=" + extension]
        run(tool, args)
        if family == "mesh":
            original=(source / root_name).read_bytes()
            for corrupt in (b"broken", original[:-4]+struct.pack('<I',99)):
                (source / root_name).write_bytes(corrupt)
                rejected=list(args)
                rejected[3]=folder / "rejected-package"
                failure=subprocess.run([str(tool),*map(str,rejected)],cwd=tool.parent,
                    capture_output=True,text=True,timeout=90)
                assert failure.returncode != 0 and not rejected[3].exists(), (failure.stdout,failure.stderr)
            (source / root_name).write_bytes(original)
        if family == "mesh_materials":
            sidecar=source / (root_name+'.materials'); original=sidecar.read_bytes()
            for invalid in [['data://red.matres'],['data://red.texres','data://green.matres'],
                            ['data://missing.matres','data://green.matres'],{'slots':[]}]:
                sidecar.write_text(json.dumps(invalid)); rejected=list(args); rejected[3]=folder / "rejected-package"
                failure=subprocess.run([str(tool),*map(str,rejected)],cwd=tool.parent,capture_output=True,text=True,timeout=90)
                assert failure.returncode != 0 and not rejected[3].exists(),(failure.stdout,failure.stderr)
            sidecar.write_bytes(original)
        if family == "skin":
            original=(source/root_name).read_bytes(); document=json.loads(original)
            malformed=[]
            bad=json.loads(original);bad['vertices'][0]['weights']=[.5,0,0,0];malformed.append(bad)
            bad=json.loads(original);bad['vertices'][0]['joints']=[7,0,0,0];malformed.append(bad)
            bad=json.loads(original);bad['vertices'].pop();malformed.append(bad)
            for bad in malformed:
                (source/root_name).write_text(json.dumps(bad)); rejected=list(args);rejected[3]=folder/'rejected-package'
                failure=subprocess.run([str(tool),*map(str,rejected)],cwd=tool.parent,capture_output=True,text=True,timeout=90)
                assert failure.returncode!=0 and not rejected[3].exists(),(failure.stdout,failure.stderr)
            (source/root_name).write_bytes(original)
        assert source.resolve().is_relative_to(root.resolve()) and cooked.resolve().is_relative_to(root.resolve())
        shutil.rmtree(source)
        shutil.rmtree(cooked)
        catalog.unlink()
        before = {p.relative_to(package): p.read_bytes() for p in package.rglob("*") if p.is_file()}
        try:
            for path in before:
                (package / path).chmod(0o444)
            run(tool, ["--verify", package, target, *[identity for identity, _, _ in entries]])
            if family == "terrain":
                run(host, ["--terrain-checked-package-consume", package, root_uuid])
            elif family == "foliage":
                run(host, ["--foliage-checked-package-consume", package, root_uuid])
            elif family == "skeleton":
                run(host, ["--skeleton-checked-package-consume", package, root_uuid])
            elif family == "skin":
                run(host, ["--skin-checked-package-consume", package, root_uuid])
            elif family in ("scene", "prefab", "ui"):
                pass  # Real Editor consumers have separate native test cases.
            else:
                run(host, ["--mesh-checked-package-consume", package, root_uuid])
            assert before == {p.relative_to(package): p.read_bytes() for p in package.rglob("*") if p.is_file()}
        finally:
            for path in before:
                (package / path).chmod(0o666)
        boundary="source-free UUID closure; native Editor installation tested separately" if family in ("scene","prefab","ui") else "read-only source-free native consumer"
        print(f"PASS: checked {family} {boundary}")
