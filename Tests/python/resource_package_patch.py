"""Source-free directory deltas through the installed resource package CLI."""

import pathlib
import shutil
import sqlite3
import struct
import subprocess
import uuid
from contextlib import closing


def check_patch(executable, root, environment):
    root.mkdir()
    source, catalog, cooked = root / "source", root / "catalog.db", root / "cooked"
    source.mkdir()
    names = ["main", "helper", "stable", "retired", "added"]
    identities = {name: str(uuid.uuid4()) for name in names}
    for name in names:
        (source / (name + ".lua")).write_text("return 1\n", encoding="utf-8")
    (source / "main.lua").write_text("return require('helper')\n", encoding="utf-8")
    (source / "main.lua.deps").write_text("data://helper.lua\n", encoding="utf-8")
    with closing(sqlite3.connect(catalog)) as database, database:
        database.execute("CREATE TABLE resources(id INTEGER PRIMARY KEY, uuid VARCHAR, path VARCHAR, type VARCHAR)")
        database.executemany("INSERT INTO resources(uuid,path,type) VALUES(?,?,'script')",
                             [(identities[name], name + ".lua") for name in names])

    def invoke(arguments, expected=0):
        result = subprocess.run([str(executable), *map(str, arguments)], cwd=executable.parent,
                                capture_output=True, text=True, env=environment, timeout=30)
        assert result.returncode == expected, (arguments, result.stdout, result.stderr)
        return result

    def cook(output, roots):
        arguments = [source, catalog, cooked, output, "pc", "--map", "script=lua"]
        for name in roots:
            arguments += ["--root", identities[name]]
        invoke(arguments)

    def tree(directory):
        return {path.relative_to(directory).as_posix(): path.read_bytes()
                for path in directory.rglob("*") if path.is_file()}

    base, target = root / "base", root / "target"
    cook(base, ["main", "stable", "retired"])
    (source / "helper.lua").write_text("return 2\n", encoding="utf-8")
    cook(target, ["main", "stable", "added"])
    base_bytes, target_bytes = tree(base), tree(target)
    assert len(base_bytes) == len(target_bytes) == 5

    # All further operations are independent of cooking inputs, caches and the catalog.
    shutil.rmtree(source)
    shutil.rmtree(cooked)
    catalog.unlink()
    patch, repeated, installed = root / "patch", root / "repeated", root / "installed"
    report = invoke(["--create-patch", base, target, patch, "pc"])
    assert "Changed artifacts 3; reused 1; removed resources 1" in report.stdout, report.stdout
    invoke(["--create-patch", base, target, repeated, "pc"])
    patch_bytes = tree(patch)
    assert patch_bytes == tree(repeated)
    assert patch_bytes["target.wprm"] == target_bytes["runtime.wprm"]
    changed = {path: data for path, data in target_bytes.items()
               if path.endswith(".wprs") and data != base_bytes.get(path)}
    assert len(changed) == 3
    assert {path: data for path, data in patch_bytes.items() if path.endswith(".wprs")} == changed
    assert set(patch_bytes) == {"patch.wprd", "target.wprm", *changed}

    # Re-seal a manifest with a short-name-shaped artifact directory. The
    # manifest and artifact are both valid WPRS files and the input exists.
    # An input with this spelling cannot safely be merged into a Windows stage.
    aliased = root / "alias-target"
    shutil.copytree(target, aliased)
    added_path = next(pathlib.PurePosixPath(path) for path, data in changed.items()
                      if b"data://added.lua" in data)
    alias_path = added_path.parent.with_name("shortd~1") / added_path.name
    (aliased / added_path.parent).rename(aliased / alias_path.parent)
    manifest = target_bytes["runtime.wprm"]
    payload_size = struct.unpack_from("<Q", manifest, 32)[0]
    header, payload = bytearray(manifest[:-payload_size]), manifest[-payload_size:]

    def encoded_path(path):
        text = path.as_posix().encode("ascii")
        return struct.pack("<Q", len(text)) + text

    assert payload.count(encoded_path(added_path)) == 1
    payload = payload.replace(encoded_path(added_path), encoded_path(alias_path), 1)
    checksum = 14695981039346656037
    for byte in payload:
        checksum = ((checksum ^ byte) * 1099511628211) & 0xffffffffffffffff
    struct.pack_into("<QQQ", header, 16, checksum, checksum, len(payload))
    (aliased / "runtime.wprm").write_bytes(header + payload)
    invoke(["--verify", aliased, "pc", identities["added"]])
    alias_output = root / "alias-patch"
    denied = invoke(["--create-patch", base, aliased, alias_output, "pc"], expected=1)
    assert "unsafe, aliased or non-package artifact path" in denied.stderr, denied.stderr
    assert not alias_output.exists() and not list(root.glob(".wp-package-*"))

    def rejected(destination, message=None, old=base, delta=patch, platform="pc"):
        failed = invoke(["--apply-patch", old, delta, destination, platform], expected=1)
        if message:
            assert message in failed.stderr, failed.stderr
        assert not destination.exists()
        assert not list(root.glob(".wp-package-*"))

    rejected(root / "wrong-generation", "base generation", old=target)
    rejected(root / "wrong-target", "requested target", platform="android")
    rejected(base / "nested", "separate directories")
    existing = root / "existing"
    existing.mkdir()
    (existing / "keep").write_bytes(b"previous generation")
    invoke(["--apply-patch", base, patch, existing, "pc"], expected=1)
    assert tree(existing) == {"keep": b"previous generation"}

    artifact_name, artifact_bytes = next(iter(changed.items()))
    artifact = patch / pathlib.PurePosixPath(artifact_name)
    artifact.unlink()
    rejected(root / "incomplete")
    artifact.write_bytes(artifact_bytes[:-1] + bytes([artifact_bytes[-1] ^ 1]))
    rejected(root / "corrupt-artifact", "changed or failed verification")
    artifact.write_bytes(artifact_bytes)
    descriptor = patch / "patch.wprd"
    descriptor_bytes = patch_bytes["patch.wprd"]
    descriptor.write_bytes(descriptor_bytes[:-1] + bytes([descriptor_bytes[-1] ^ 1]))
    rejected(root / "corrupt-descriptor")
    descriptor.write_bytes(descriptor_bytes)
    target_manifest = patch / "target.wprm"
    target_manifest.write_bytes(base_bytes["runtime.wprm"])
    rejected(root / "corrupt-target")
    target_manifest.write_bytes(patch_bytes["target.wprm"])
    retired_artifact = next(path for path, data in base_bytes.items()
                            if path.endswith(".wprs") and b"data://retired.lua" in data)
    retired = base / pathlib.PurePosixPath(retired_artifact)
    retired_data = base_bytes[retired_artifact]
    retired.write_bytes(retired_data[:-1] + bytes([retired_data[-1] ^ 1]))
    rejected(root / "corrupt-base", "artifact failed verification")
    retired.write_bytes(retired_data)

    # Retry with the original immutable inputs, then prove both generations still mount.
    invoke(["--apply-patch", base, patch, installed, "pc"])
    assert tree(installed) == target_bytes
    invoke(["--verify", installed, "pc", *[identities[name] for name in ("main", "helper", "stable", "added")]])
    invoke(["--verify", installed, "pc", identities["retired"]], expected=1)
    invoke(["--verify", base, "pc", identities["main"], identities["retired"]])
    assert tree(base) == base_bytes and tree(target) == target_bytes and tree(patch) == patch_bytes

    identical, noop_patch, noop_install = root / "identical", root / "noop-patch", root / "noop-installed"
    shutil.copytree(target, identical)
    noop = invoke(["--create-patch", target, identical, noop_patch, "pc"])
    assert "Changed artifacts 0; reused 4; removed resources 0" in noop.stdout, noop.stdout
    assert set(tree(noop_patch)) == {"patch.wprd", "target.wprm"}
    invoke(["--apply-patch", target, noop_patch, noop_install, "pc"])
    assert tree(noop_install) == target_bytes
