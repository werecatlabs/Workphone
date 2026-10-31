"""Checked catalog export through existing terrain/foliage typed consumers."""
import pathlib
import shutil
import sqlite3
import subprocess
import sys
import tempfile
import uuid
import struct
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
    for family in ("terrain", "foliage", "mesh"):
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
        else:
            source=folder / "authoring"
            source.mkdir()
            root_name, target, extensions = "triangle.mesh", "pc", {"mesh"}
            (source / root_name).write_bytes(struct.pack('<4s7I6f',b'WPM1',1,0,1,0,3,3,1,
                0,0,0,1,1,0)+struct.pack('<9f6I',0,0,0,1,0,0,0,1,0,0,1,2,0,3,0))
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
            else:
                run(host, ["--mesh-checked-package-consume", package, root_uuid])
            assert before == {p.relative_to(package): p.read_bytes() for p in package.rglob("*") if p.is_file()}
        finally:
            for path in before:
                (package / path).chmod(0o666)
        print(f"PASS: checked {family} UUID closure, read-only source-free native consumer")
