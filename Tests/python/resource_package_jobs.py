"""Run the native manager/job contracts with the real package child process."""
import json
import pathlib
import sqlite3
import subprocess
import sys
import tempfile
import uuid
import shutil
from contextlib import closing
import ctypes
from ctypes import wintypes
import os

host = pathlib.Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory(prefix="wp-package-jobs-") as temporary:
    project = pathlib.Path(temporary) / "project café with spaces"
    project.mkdir()
    (project / "main.lua").write_text("assert(require('helper').value == 42)\n", encoding="utf-8")
    (project / "helper.lua").write_text("return {value=42}\n", encoding="utf-8")
    (project / "main.lua.deps").write_text("data://helper.lua\n", encoding="utf-8")
    with closing(sqlite3.connect(project / "catalog.db")) as db, db:
        db.execute("CREATE TABLE resources(id INTEGER PRIMARY KEY, uuid VARCHAR, path VARCHAR, type VARCHAR)")
        root_id = str(uuid.uuid4())
        db.executemany("INSERT INTO resources(uuid,path,type) VALUES(?,?,'script')",
                       [(root_id, "main.lua"), (str(uuid.uuid4()), "helper.lua")])
    profile = project / "shipping.json"
    profile.write_text(json.dumps(dict(version=1, sourceRoot=".", catalog="catalog.db",
        cookedRoot="cooked", output="initial-output", target="pc",
        mappings=[dict(catalogType="script", extension="lua")], roots=[root_id])), encoding="utf-8")
    # Exercise the actual inherited-event protocol used by the job's child.
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel.CreateEventW.argtypes = [ctypes.c_void_p, wintypes.BOOL, wintypes.BOOL, wintypes.LPCWSTR]
    kernel.CreateEventW.restype = wintypes.HANDLE
    kernel.CloseHandle.argtypes = [wintypes.HANDLE]
    event = kernel.CreateEventW(None, True, True, None)
    assert event
    try:
        os.set_handle_inheritable(event, True)
        startup = subprocess.STARTUPINFO()
        startup.lpAttributeList = {"handle_list": [event]}
        cancelled = subprocess.run([str(host.with_name("WorkphoneResourcePackage.exe")), "--profile",
            str(profile), "--cancel-handle", str(event)], cwd=host.parent, startupinfo=startup,
            close_fds=True, capture_output=True, text=True, timeout=30)
        assert cancelled.returncode == 3, (cancelled.returncode, cancelled.stdout, cancelled.stderr)
        assert not (project / "initial-output").exists() and not (project / "cooked").exists()
    finally:
        kernel.CloseHandle(event)
    result = subprocess.run([str(host), str(project), str(profile)], cwd=host.parent,
                            capture_output=True, text=True, timeout=110)
    print(result.stdout)
    print(result.stderr, file=sys.stderr)
    if result.returncode:
        for log in (project / ".workphone" / "package-jobs").glob("*.log"):
            print(log.name, log.read_text(errors="replace"), file=sys.stderr)
    assert result.returncode == 0, result.returncode
    relocated = pathlib.Path(temporary) / "read-only-relocated"
    shutil.copytree(project.parent / "success-output", relocated)
    # No authoring sources, catalog or cooking cache remain for the consumer process.
    for name in ("main.lua", "helper.lua", "main.lua.deps"):
        (project / name).unlink()
    for path in project.glob("catalog.db*"):
        path.unlink()
    shutil.rmtree(project / "cooked")
    files = [p for p in relocated.rglob("*") if p.is_file()]
    before = {p: p.read_bytes() for p in files}
    try:
        for path in files:
            path.chmod(0o444)
        result = subprocess.run([str(host), "--consume", str(relocated), "pc", root_id],
                                cwd=host.parent, capture_output=True, text=True, timeout=30)
        print(result.stdout)
        print(result.stderr, file=sys.stderr)
        assert result.returncode == 0, result.returncode
        assert all(p.read_bytes() == content for p, content in before.items())
    finally:
        for path in files:
            path.chmod(0o666)
