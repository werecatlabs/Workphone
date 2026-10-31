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
import time
from resource_package_history import check_history, validate_receipts

host = pathlib.Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory(prefix="wp-package-jobs-") as temporary:
    project = pathlib.Path(temporary) / "project café with spaces"
    project.mkdir()
    (project / "main.lua").write_text("assert(require('helper').value == 42)\n", encoding="utf-8")
    (project / "helper.lua").write_text("return {value=42}\n", encoding="utf-8")
    (project / "main.lua.deps").write_text("data://helper.lua\n", encoding="utf-8")
    # A real exported closure crosses both the old 100-row display limit and
    # the new 25-row page boundary, including a partial last page.
    (project / "pages").mkdir()
    for index in range(125):
        (project / "pages" / f"item{index:03}.lua").write_text(f"return {index}\n", encoding="utf-8")
    with (project / "main.lua.deps").open("a", encoding="utf-8") as dependencies:
        dependencies.writelines(f"data://pages/item{index:03}.lua\n" for index in range(125))
    with closing(sqlite3.connect(project / "catalog.db")) as db, db:
        db.execute("CREATE TABLE resources(id INTEGER PRIMARY KEY, uuid VARCHAR, path VARCHAR, type VARCHAR)")
        root_id = str(uuid.uuid4())
        db.executemany("INSERT INTO resources(uuid,path,type) VALUES(?,?,'script')",
                       [(root_id, "main.lua"), (str(uuid.uuid4()), "helper.lua")])
        # Catalog-only rows exercise bounded root search without adding unrelated
        # authoring inputs to the exported install closure.
        db.executemany("INSERT INTO resources(uuid,path,type) VALUES(?,?,'script')",
                       [(str(uuid.uuid4()), f"picker-only/{i:02}.lua") for i in range(30)])
        db.executemany("INSERT INTO resources(uuid,path,type) VALUES(?,?,'script')",
                       [(str(uuid.uuid4()), f"pages/item{i:03}.lua") for i in range(125)])
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
        preview_report = project / "cancelled-preview.json"
        preview_cancelled = subprocess.run([str(host.with_name("WorkphoneResourcePackage.exe")), "--preview-profile",
            str(profile), str(preview_report), "--cancel-handle", str(event)], cwd=host.parent, startupinfo=startup,
            close_fds=True, capture_output=True, text=True, timeout=30)
        assert preview_cancelled.returncode == 3 and not preview_report.exists(), preview_cancelled.stderr
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
    keyed = project / "profile-v2.json"
    keyed_values = dict(json.loads(profile.read_bytes()), version=2, output="../profile-v2-output",
        buildProfile=dict(name="desktop", toolchain="job-contract-tools", settings={"quality":"high","empty":""}))
    keyed.write_text(json.dumps(keyed_values), encoding="utf-8")
    profiled = subprocess.run([str(host), "--run-profile", str(project), str(keyed)], cwd=host.parent,
                             capture_output=True, text=True, timeout=60)
    assert profiled.returncode == 0, (profiled.stdout, profiled.stderr)
    assert (project.parent / "profile-v2-output" / "runtime.wprm").is_file()
    validate_receipts(project)
    check_history(host.with_name("WorkphoneResourcePackage.exe"), pathlib.Path(temporary) / "history café")
    # Kill an actual native job owner with a live (deliberately blocking) child.
    # This certifies request/child lifetime handling, not a crash inside a cook
    # or the package-directory publication window.
    tool = host.with_name("WorkphoneResourcePackage.exe")
    jobs = project / ".workphone" / "package-jobs"
    supervised = subprocess.run([str(host), "--watchdog", str(project), str(profile)], cwd=host.parent,
                                capture_output=True, text=True, timeout=30)
    print(supervised.stdout)
    assert supervised.returncode == 0, (supervised.stdout, supervised.stderr)
    uncertain = jobs / "d08c0000-0000-4000-8000-000000000005.json"
    uncertain_bytes = uncertain.read_bytes()
    reconciled = subprocess.run([str(tool), "--recover-jobs", str(project)], cwd=host.parent,
                                capture_output=True, text=True, timeout=30)
    assert reconciled.returncode == 0 and "Recovered 1" in reconciled.stdout, (reconciled.stdout, reconciled.stderr)
    assert not uncertain.exists() and uncertain.with_name(uncertain.stem + ".interrupted.json").read_bytes() == uncertain_bytes
    assert not (project / "initial-output").exists()
    snapshot = jobs / "0c01438b-6391-4c51-8383-62f8f06388de.json"
    interrupted = snapshot.with_name(snapshot.stem + ".interrupted.json")
    marker = project / "crash-child.pid"
    marker.unlink(missing_ok=True)  # The preceding watchdog cases use the same fixture marker.
    kernel.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
    kernel.OpenProcess.restype = wintypes.HANDLE
    kernel.WaitForSingleObject.argtypes = [wintypes.HANDLE, wintypes.DWORD]
    kernel.WaitForSingleObject.restype = wintypes.DWORD
    owner = subprocess.Popen([str(host), "--crash-owner", str(project), str(profile)],
                             cwd=host.parent, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    child = None
    try:
        deadline = time.monotonic() + 20
        while not marker.exists() and owner.poll() is None and time.monotonic() < deadline:
            time.sleep(0.05)
        assert marker.exists() and owner.poll() is None, "Blocking native child did not start"
        child = kernel.OpenProcess(0x00100000, False, int(marker.read_text()))  # SYNCHRONIZE
        assert child
        before = snapshot.read_bytes()
        ownership = snapshot.with_suffix(".owner")
        assert ownership.read_bytes() == b"WORKPHONE_PACKAGE_REQUEST_V1\n"
        active = subprocess.run([str(tool), "--recover-jobs", str(project)], cwd=host.parent,
                                capture_output=True, text=True, timeout=30)
        assert active.returncode == 0 and "locked 1" in active.stdout, (active.stdout, active.stderr)
        assert snapshot.read_bytes() == before and not interrupted.exists()
        owner.kill(); owner.communicate(timeout=10)
        assert kernel.WaitForSingleObject(child, 5000) == 0, "Owner death must terminate its package child"
        assert snapshot.exists() and not snapshot.with_suffix(".json.lock").exists()
        recovered = subprocess.run([str(tool), "--recover-jobs", str(project)], cwd=host.parent,
                                   capture_output=True, text=True, timeout=30)
        assert recovered.returncode == 0 and "Recovered 1" in recovered.stdout, (recovered.stdout, recovered.stderr)
        assert not snapshot.exists() and interrupted.read_bytes() == before
        assert ownership.read_bytes() == b"WORKPHONE_PACKAGE_REQUEST_V1\n"
        repeated = subprocess.run([str(tool), "--recover-jobs", str(project)], cwd=host.parent,
                                  capture_output=True, text=True, timeout=30)
        assert repeated.returncode == 0 and "Recovered 0" in repeated.stdout
        automatic = jobs / (str(uuid.uuid4()) + ".json")
        automatic.write_bytes(before)
        automatic.with_suffix(".owner").write_bytes(b"WORKPHONE_PACKAGE_REQUEST_V1\n")
        # Restart the real compiler/exporter with an explicit fresh destination.
        retry = project / "crash-retry.json"
        retry.write_text(json.dumps(dict(json.loads(profile.read_bytes()), output="../crash-retry-output")), encoding="utf-8")
        resumed = subprocess.run([str(host), "--run-profile", str(project), str(retry)], cwd=host.parent,
                                 capture_output=True, text=True, timeout=60)
        assert resumed.returncode == 0, (resumed.stdout, resumed.stderr)
        assert "1 archived" in resumed.stdout, (resumed.stdout, resumed.stderr)
        assert not automatic.exists() and automatic.with_name(automatic.stem + ".interrupted.json").read_bytes() == before
        assert (project.parent / "crash-retry-output" / "runtime.wprm").is_file()
        assert interrupted.read_bytes() == before
        print("PASS: active request preserved, killed owner's child exited, snapshot archived and explicit retry exported")
    finally:
        if owner.poll() is None:
            owner.kill(); owner.communicate(timeout=10)
        if child:
            kernel.CloseHandle(child)
    relocated = pathlib.Path(temporary) / "read-only-relocated"
    shutil.copytree(project.parent / "success-output", relocated)
    # No authoring sources, catalog or cooking cache remain for the consumer process.
    for name in ("main.lua", "helper.lua", "main.lua.deps"):
        (project / name).unlink()
    shutil.rmtree(project / "pages")
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
