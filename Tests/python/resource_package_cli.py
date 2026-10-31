"""Exercise the actual package CLI with independent SQLite and filesystem fixtures."""
import pathlib
import sqlite3
import subprocess
import sys
import tempfile
import uuid
import json
import shutil
import os
from contextlib import closing
from resource_package_history import check_history

exe = pathlib.Path(sys.argv[1]).resolve()
child_environment = dict(os.environ)
system_root = child_environment.get("SYSTEMROOT", r"C:\Windows")
child_environment["PATH"] = str(pathlib.Path(system_root) / "System32") + os.pathsep + system_root
for key in tuple(child_environment):
    if key.upper().startswith(("WP_", "LUA_PATH", "LUA_CPATH")):
        child_environment.pop(key)

with tempfile.TemporaryDirectory(prefix="wp-package-cli-") as temporary:
    check_history(exe, pathlib.Path(temporary) / "history-only café", child_environment)
    root = pathlib.Path(temporary) / "project café"
    root.mkdir()
    source = root / "sources"
    source.mkdir()
    (source / "main.lua").write_text("return require('helper')\n", encoding="utf-8")
    (source / "helper.lua").write_text("return 42\n", encoding="utf-8")
    (source / "main.lua.deps").write_text("data://helper.lua\n", encoding="utf-8")
    catalog = root / "catalog.db"
    main_id, helper_id = str(uuid.uuid4()), str(uuid.uuid4())
    with closing(sqlite3.connect(catalog)) as db, db:
        db.execute("CREATE TABLE resources(id INTEGER PRIMARY KEY, uuid VARCHAR, path VARCHAR, type VARCHAR)")
        db.executemany("INSERT INTO resources(uuid,path,type) VALUES(?,?,'script')",
                       [(main_id, "main.lua"), (helper_id, "helper.lua")])

    def package(output, identity=main_id, catalog_path=catalog, expected=0):
        result = subprocess.run([str(exe), str(source), str(catalog_path), str(root / "cooked"),
                                 str(output), "pc", "--map", "script=lua", "--root", identity],
                                cwd=exe.parent, capture_output=True, text=True, env=child_environment)
        assert result.returncode == expected, (result.returncode, result.stdout, result.stderr)

    output = root / "package"
    package(output)
    files = {p.relative_to(output).as_posix(): p.read_bytes() for p in output.rglob("*") if p.is_file()}
    assert "runtime.wprm" in files and len(files) == 3, files.keys()
    assert all(p == "runtime.wprm" or p.endswith(".wprs") for p in files)
    def invoke(arguments, expected=0):
        result = subprocess.run([str(exe), *map(str, arguments)], cwd=exe.parent,
                                capture_output=True, text=True, env=child_environment)
        assert result.returncode == expected, (result.returncode, result.stdout, result.stderr)
        return result

    # Recovery archives only valid, project-bound captured requests; everything
    # ambiguous is preserved with diagnostics, and no authoring database is opened.
    jobs = source / ".workphone" / "package-jobs"
    jobs.mkdir(parents=True)
    recovery_settings = dict(version=1, sourceRoot=str(source), catalog=str(catalog),
        cookedRoot=str(root / "recovery-cooked"), output=str(root / "recovery-output"), target="pc",
        mappings=[dict(catalogType="script", extension="lua")], roots=[main_id])
    abandoned = jobs / (str(uuid.uuid4()) + ".json")
    abandoned.write_text(json.dumps(recovery_settings), encoding="utf-8")
    abandoned_bytes = abandoned.read_bytes()
    invalid = jobs / (str(uuid.uuid4()) + ".json")
    invalid.write_bytes(b"{}")
    wrong = jobs / (str(uuid.uuid4()) + ".json")
    wrong.write_text(json.dumps(dict(recovery_settings, sourceRoot=str(root))), encoding="utf-8")
    conflict = jobs / (str(uuid.uuid4()) + ".json")
    conflict.write_bytes(abandoned_bytes)
    conflict_archive = conflict.with_name(conflict.stem + ".interrupted.json")
    conflict_archive.write_bytes(b"retained history")
    for path in (abandoned, invalid, wrong, conflict):
        path.with_suffix(".owner").write_bytes(b"WORKPHONE_PACKAGE_REQUEST_V1\n")
    legacy = jobs / (str(uuid.uuid4()) + ".json")
    legacy.write_bytes(abandoned_bytes)
    untrusted = jobs / (str(uuid.uuid4()) + ".json")
    untrusted.write_bytes(abandoned_bytes)
    untrusted.with_suffix(".owner").write_bytes(b"WORKPHONE_PACKAGE_REQUEST_V1\ntrailing-data")
    log = jobs / (abandoned.stem + ".log")
    report = jobs / (abandoned.stem + ".preview.json")
    log.write_bytes(b"retained log"); report.write_bytes(b"retained report")
    ignored = jobs / "not-a-request.json"
    ignored.write_bytes(b"unrelated file")
    preserved = {path: path.read_bytes() for path in (invalid, wrong, conflict, conflict_archive, legacy,
                 untrusted, untrusted.with_suffix(".owner"), log, report, ignored)}
    database_before = catalog.read_bytes()
    recovery = invoke(["--recover-jobs", source])
    assert "Recovered 1" in recovery.stdout and "warnings 5" in recovery.stdout, (recovery.stdout, recovery.stderr)
    archive = abandoned.with_name(abandoned.stem + ".interrupted.json")
    assert archive.read_bytes() == abandoned_bytes and not abandoned.exists()
    assert all(path.read_bytes() == contents for path, contents in preserved.items())
    assert catalog.read_bytes() == database_before and not (root / "recovery-cooked").exists()
    assert not (root / "recovery-output").exists() and not list(jobs.glob("*.lock"))
    assert "Recovered 0" in invoke(["--recover-jobs", source]).stdout
    missing_jobs = root / "no-jobs-project"
    missing_jobs.mkdir()
    invoke(["--recover-jobs", missing_jobs])
    assert not (missing_jobs / ".workphone").exists()
    invoke(["--recover-jobs", root / "missing-project"], expected=1)
    redirected = root / "redirected-project"
    redirected.mkdir()
    try:
        (redirected / ".workphone").symlink_to(jobs.parent, target_is_directory=True)
    except OSError:
        pass  # Symlink creation can require developer mode/privilege on Windows.
    else:
        invoke(["--recover-jobs", redirected], expected=1)
        assert all(path.read_bytes() == contents for path, contents in preserved.items())

    profile = root / "shipping café.json"
    profile_output = root / "profile-output"
    request = [source, catalog, root / "profile-cooked", profile_output, "pc",
               "--map", "script=lua", "--root", main_id]
    invoke(["--save-profile", profile, *request])
    assert not profile_output.exists() and not (root / "profile-cooked").exists()
    saved = profile.read_bytes()
    settings = json.loads(saved)
    assert settings["version"] == 1 and settings["sourceRoot"] == "sources"
    assert settings["roots"] == [main_id] and settings["target"] == "pc"
    invoke(["--save-profile", profile, *request], expected=1)
    assert profile.read_bytes() == saved
    preview = root / "preview café.json"
    invoke(["--preview-profile", profile, ""], expected=1)
    assert not profile_output.exists()
    invoke(["--preview-profile", profile, preview])
    assert not profile_output.exists()
    report = json.loads(preview.read_bytes())
    by_id = {entry["resourceId"]: entry for entry in report["entries"]}
    assert report["version"] == 1 and report["target"] == "pc" and len(by_id) == 2
    assert by_id["data://main.lua"]["uuid"] == main_id and by_id["data://main.lua"]["root"]
    assert by_id["data://main.lua"]["installDependencies"] == ["data://helper.lua"]
    assert by_id["data://helper.lua"]["uuid"] == helper_id and not by_id["data://helper.lua"]["root"]
    assert all(entry["payloadBytes"] > 0 for entry in by_id.values())
    preview_bytes = preview.read_bytes()
    invoke(["--preview-profile", profile, preview], expected=1)
    assert preview.read_bytes() == preview_bytes and not profile_output.exists()
    invoke(["--profile", profile])
    assert len([p for p in profile_output.rglob("*") if p.is_file()]) == 3
    invoke(["--profile", profile], expected=1)
    invoke(["--preview-profile", profile, root / "preview-occupied-output.json"])
    assert preview_bytes == (root / "preview-occupied-output.json").read_bytes()
    relocated = root / "relocated"
    relocated.mkdir()
    shutil.copytree(source, relocated / "sources")
    shutil.copy2(catalog, relocated / "catalog.db")
    shutil.copy2(profile, relocated / profile.name)
    invoke(["--profile", relocated / profile.name])
    assert (relocated / "profile-output" / "runtime.wprm").is_file()
    shutil.rmtree(relocated / "sources")
    shutil.rmtree(relocated / "profile-cooked")
    for path in relocated.glob("catalog.db*"):
        path.unlink()
    verification_files = [p for p in (relocated / "profile-output").rglob("*") if p.is_file()]
    before_verify = {p: p.read_bytes() for p in verification_files}
    try:
        for path in verification_files:
            path.chmod(0o444)
        invoke(["--verify", relocated / "profile-output", "pc", main_id, helper_id])
        invoke(["--verify", relocated / "profile-output", "wrong-target", main_id], expected=1)
        invoke(["--verify", relocated / "profile-output", "pc", str(uuid.uuid4())], expected=1)
        assert all(p.read_bytes() == content for p, content in before_verify.items())
        assert set(before_verify) == {p for p in (relocated / "profile-output").rglob("*") if p.is_file()}
        assert not (relocated / "catalog.db").exists() and not (relocated / "profile-cooked").exists()
    finally:
        for path in verification_files:
            path.chmod(0o666)
    corrupt = next(p for p in verification_files if p.suffix == ".wprs")
    corrupt.write_bytes(b"corrupt")
    invoke(["--verify", relocated / "profile-output", "pc", main_id, helper_id], expected=1)
    # Every rejected profile must fail before opening a catalog or creating a cache.
    bad = root / "invalid.json"
    rejected = root / "rejected-output"
    invalid_base = dict(settings, output="rejected-output", cookedRoot="rejected-cooked")
    invalid_profiles = [
        dict(invalid_base, version=2), dict(invalid_base, unexpected=True),
        dict(invalid_base, roots=[]), dict(invalid_base, roots=[main_id, main_id.upper()]),
        dict(invalid_base, mappings=settings["mappings"] * 2),
        dict(invalid_base, sourceRoot="bad\0path"), dict(invalid_base, roots=[main_id] * 513),
    ]
    for invalid in invalid_profiles:
        bad.write_text(json.dumps(invalid), encoding="utf-8")
        invoke(["--profile", bad], expected=1)
        assert not rejected.exists() and not (root / "rejected-cooked").exists()
    for contents in [b'{"version":1,"version":1}', b'{"version":1}\x00garbage',
                     b'{"version":1, "roots":"\xff"}', b' ' * 65537,
                     json.dumps(invalid_base).encode() + b'\x00garbage',
                     json.dumps(invalid_base).replace('"version": 1', '"version": 1, "version": 1').encode(),
                     json.dumps(invalid_base).encode() + b' trailing']:
        bad.write_bytes(contents)
        invoke(["--profile", bad], expected=1)
        assert not rejected.exists() and not (root / "rejected-cooked").exists()
    assert not list(root.glob(".wp-profile-*"))
    package(output, expected=1)
    assert files == {p.relative_to(output).as_posix(): p.read_bytes() for p in output.rglob("*") if p.is_file()}
    package(root / "bad-uuid", identity=str(uuid.uuid4()), expected=1)
    assert not (root / "bad-uuid").exists()
    missing = root / "missing.db"
    package(root / "missing-output", catalog_path=missing, expected=1)
    assert not missing.exists()
    (source / "main.lua").write_text("function broken(\n", encoding="utf-8")
    package(root / "compile-failed", expected=1)
    assert not (root / "compile-failed").exists()
    (source / "main.lua").write_text("return require('helper')\n", encoding="utf-8")
    # Unsupported policy metadata fails closed through checked catalog admission.
    with closing(sqlite3.connect(catalog)) as db, db:
        db.execute("CREATE TABLE wp_asset_package_schema(id INTEGER PRIMARY KEY,version INTEGER)")
        db.execute("INSERT INTO wp_asset_package_schema VALUES(1,999)")
    package(root / "bad-policy", expected=1)
    assert not (root / "bad-policy").exists()
    with closing(sqlite3.connect(catalog)) as db, db:
        db.execute("DROP TABLE wp_asset_package_schema")
        db.execute("CREATE TABLE wp_asset_package_schema(id INTEGER PRIMARY KEY CHECK(id=1),version INTEGER NOT NULL)")
        db.execute("CREATE TABLE wp_asset_package_usage(uuid TEXT NOT NULL PRIMARY KEY,usage TEXT NOT NULL CHECK(usage IN ('runtime','editor-only','excluded')))")
        for table in ("wp_asset_package_schema", "wp_asset_package_usage"):
            for event in ("INSERT", "UPDATE", "DELETE"):
                db.execute(f"CREATE TRIGGER {table}_{event} AFTER {event} ON {table} BEGIN UPDATE wp_asset_catalog_changes SET revision=revision+1 WHERE id=1; END")
        db.execute("INSERT INTO wp_asset_package_schema VALUES(1,1)")
        db.execute("INSERT INTO wp_asset_package_usage VALUES(?,'editor-only')", (helper_id,))
    package(root / "editor-only", expected=1)
    assert not (root / "editor-only").exists()
    ineligible_profile = root / "ineligible.json"
    ineligible_profile.write_text(json.dumps(dict(settings, output="profile-ineligible")), encoding="utf-8")
    invoke(["--profile", ineligible_profile], expected=1)
    rejected_report = root / "ineligible-preview.json"
    failed_preview = invoke(["--preview-profile", ineligible_profile, rejected_report], expected=1)
    assert "Editor-only" in failed_preview.stderr and not rejected_report.exists()
    assert not (root / "profile-ineligible").exists()
    with closing(sqlite3.connect(catalog)) as db, db:
        db.execute("UPDATE wp_asset_package_usage SET usage='excluded' WHERE uuid=?", (helper_id,))
    package(root / "excluded", expected=1)
    excluded_preview = invoke(["--preview-profile", ineligible_profile, rejected_report], expected=1)
    assert "excluded" in excluded_preview.stderr and not rejected_report.exists()
    assert not (root / "excluded").exists()
    with closing(sqlite3.connect(catalog)) as db, db:
        db.execute("UPDATE wp_asset_package_usage SET usage='runtime' WHERE uuid=?", (helper_id,))
    package(root / "restored-runtime")
    # A dependency removed from the catalog must prevent publication even when cooked.
    with closing(sqlite3.connect(catalog)) as db, db:
        db.execute("DELETE FROM resources WHERE uuid=?", (helper_id,))
    package(root / "uncataloged", expected=1)
    invoke(["--preview-profile", ineligible_profile, rejected_report], expected=1)
    assert not rejected_report.exists()
    assert not (root / "uncataloged").exists()
    assert not list(root.glob(".wp-package-*"))
    assert not list(root.glob(".wp-preview-*"))
    assert not list((root / "profile-cooked").glob(".wp-package-*"))
print("Resource package CLI contracts passed")
