"""Checked catalog export through existing terrain/foliage typed consumers."""
import pathlib
import shutil
import sqlite3
import subprocess
import sys
import tempfile
import uuid

host, tool = map(lambda p: pathlib.Path(p).resolve(), sys.argv[1:3])

def run(executable, args):
    result = subprocess.run([str(executable), *map(str, args)], cwd=executable.parent,
                            capture_output=True, text=True, timeout=90)
    assert result.returncode == 0, (result.returncode, result.stdout, result.stderr)
    return result

with tempfile.TemporaryDirectory(prefix="wp-typed-packages-") as temporary:
    root = pathlib.Path(temporary)
    for family in ("terrain", "foliage"):
        folder = root / family
        folder.mkdir()
        if family == "terrain":
            run(host, ["--terrain-package-export", folder])
            source = folder / "terrain-package" / "authoring-offline"
            root_name, target = "ground.terrain", "terrain-dx11-contract"
            extensions = {"terrain", "texres"}
        else:
            source = folder / "authoring"
            run(host, ["--foliage-source-export", source])
            root_name, target = "forest.folworld", "windows-dx11-dds-bgra8-v1"
            extensions = {"folworld", "folpage", "folspec", "texres"}
        entries = [(str(uuid.uuid4()), p.relative_to(source).as_posix(), p.suffix[1:])
                   for p in source.rglob("*") if p.is_file() and p.suffix[1:] in extensions]
        root_uuid = next(identity for identity, path, kind in entries if path == root_name)
        catalog = folder / "catalog.db"
        with sqlite3.connect(catalog) as db:
            db.execute("CREATE TABLE resources(id INTEGER PRIMARY KEY,uuid TEXT,path TEXT,type TEXT)")
            db.executemany("INSERT INTO resources(uuid,path,type) VALUES(?,?,?)", entries)
        package, cooked = folder / "checked-package", folder / "checked-cooked"
        args = [source, catalog, cooked, package, target, "--root", root_uuid,
                "--build-profile", "family-shipping", "--toolchain", "family-contract-tools"]
        for extension in sorted(extensions):
            args += ["--map", extension + "=" + extension]
        run(tool, args)
        assert source.resolve().is_relative_to(root.resolve()) and cooked.resolve().is_relative_to(root.resolve())
        shutil.rmtree(source)
        shutil.rmtree(cooked)
        before = {p.relative_to(package): p.read_bytes() for p in package.rglob("*") if p.is_file()}
        try:
            for path in before:
                (package / path).chmod(0o444)
            run(tool, ["--verify", package, target, *[identity for identity, _, _ in entries]])
            if family == "terrain":
                run(host, ["--terrain-checked-package-consume", package])
            else:
                run(host, ["--foliage-checked-package-consume", package, root_uuid])
            assert before == {p.relative_to(package): p.read_bytes() for p in package.rglob("*") if p.is_file()}
        finally:
            for path in before:
                (package / path).chmod(0o666)
        print(f"PASS: checked {family} UUID closure, read-only source-free native consumer")
