"""Cook an actual imported character through the project catalog and draw its package."""

import hashlib
import json
import os
from pathlib import Path
import shutil
import sqlite3
import subprocess
import sys
import tempfile
from contextlib import closing


cooker, consumer, fixture = (Path(arg).resolve(strict=True) for arg in sys.argv[1:])
environment = {key: value for key, value in os.environ.items()
               if not key.upper().startswith(("WP_", "LUA_PATH", "LUA_CPATH"))}
windows = Path(environment.get("SYSTEMROOT", r"C:\Windows"))
environment["PATH"] = os.pathsep.join(map(str, (windows / "System32", windows)))


def run(executable, args, expected=0):
    process = subprocess.run([str(executable), *map(str, args)], cwd=executable.parent,
                             capture_output=True, encoding="utf-8", errors="replace",
                             env=environment, timeout=60, check=False)
    assert process.returncode == expected, (args, process.returncode, process.stdout, process.stderr)
    return process


with tempfile.TemporaryDirectory(prefix="wp-character-") as temporary:
    root = Path(temporary).resolve()
    published = root / "published character package"
    package = root / "runtime package café"
    with tempfile.TemporaryDirectory(prefix="authoring café ", dir=root) as project_folder:
        project = Path(project_folder).resolve()
        assert project.parent == root
        assets = project / "Assets"
        assets.mkdir()
        source = project / "source café.gltf"
        shutil.copyfile(fixture, source)
        original = hashlib.sha256(source.read_bytes()).hexdigest()
        catalog = project / "AssetDatabase.db"
        with closing(sqlite3.connect(catalog)) as db, db:
            db.execute("CREATE TABLE resources(id INTEGER PRIMARY KEY, uuid VARCHAR, path VARCHAR, type VARCHAR)")

        arguments = ["--import-character", project, catalog, source]
        destination = "Assets/Imported café"
        result = run(cooker, [*arguments, destination])
        imported = json.loads(result.stdout.splitlines()[-1])
        rows = imported["assets"]
        assert imported["version"] == 1 and len(rows) == 4
        assert len({entry["uuid"] for entry in rows}) == 4
        types = {entry["type"] for entry in rows}
        assert types == {"mesh", "skelres", "skinres", "fbscene"}
        assert imported["sceneUuid"] == next(row["uuid"] for row in rows if row["type"] == "fbscene")
        for row in rows:
            assert row["path"].startswith(destination + "/")
            assert row["resourceId"] == "data://" + row["path"]
        with closing(sqlite3.connect(catalog)) as db:
            committed = set(db.execute("SELECT uuid,path,type,kind FROM resources"))
        assert committed == {(row["uuid"], row["path"], row["type"], "file") for row in rows}
        generated = {row["path"]: (project / row["path"]).read_bytes() for row in rows}

        repeat = run(cooker, [*arguments, destination], expected=1)
        assert "requires a new destination" in repeat.stderr
        assert all((project / path).read_bytes() == contents for path, contents in generated.items())

        # Reject the last insertion to prove earlier rows are rolled back by the real batch transaction.
        with closing(sqlite3.connect(catalog)) as db, db:
            db.execute("CREATE TRIGGER reject_import_scene BEFORE INSERT ON resources "
                       "WHEN NEW.type='fbscene' BEGIN SELECT RAISE(ABORT, 'fixture rejection'); END")
        rejected = run(cooker, [*arguments, "Assets/Rejected"], expected=1)
        assert "Catalog registration failed." in rejected.stderr
        assert len(list((assets / "Rejected").iterdir())) == 4
        with closing(sqlite3.connect(catalog)) as db, db:
            assert set(db.execute("SELECT uuid,path,type,kind FROM resources")) == committed
            db.execute("DROP TRIGGER reject_import_scene")
        assert not list(assets.glob(".wp-character-import-*"))
        assert hashlib.sha256(source.read_bytes()).hexdigest() == original

        cook = [project, catalog, project / "cooked", published, "pc", "--root", imported["sceneUuid"]]
        for kind in sorted(types):
            cook.extend(("--map", f"{kind}={kind}"))
        run(cooker, cook)

    assert not project.exists()
    assert published.resolve(strict=True).parent == root and published.resolve() == published
    assert package.parent == root and not package.exists()
    published.rename(package)
    assert not published.exists()

    def payload_hashes():
        return {path.relative_to(package).as_posix(): hashlib.sha256(path.read_bytes()).hexdigest()
                for path in package.rglob("*") if path.is_file()}

    payload = payload_hashes()
    assert len(payload) == 5 and "runtime.wprm" in payload
    assert sum(path.endswith(".wprs") for path in payload) == 4
    run(cooker, ["--verify", package, "pc", *(row["uuid"] for row in rows)])
    assert payload_hashes() == payload
    drawn = run(consumer, ["--skin-scene-checked-package-consume", package, imported["sceneUuid"]])
    assert "PASS: imported cooked scene Animator reaches DX11 pixels" in drawn.stdout, drawn.stdout
    assert payload_hashes() == payload
    print(drawn.stdout, end="")
    print("Verified all five relocated package files retained their SHA256 hashes.")
