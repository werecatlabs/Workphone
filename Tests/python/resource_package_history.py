"""Independent owned-history safety and restart contracts for the actual CLI."""
import ctypes
import json
import pathlib
import subprocess
import uuid
from ctypes import wintypes


def fingerprint(data):
    value = 14695981039346656037
    for byte in data:
        value = ((value ^ byte) * 1099511628211) & ((1 << 64) - 1)
    return value


def validate_receipts(project):
    receipts = list((project / ".workphone" / "package-jobs").glob("*.completed.json"))
    assert len(receipts) >= 4, "Real completed native jobs must publish receipts"
    for path in receipts:
        receipt = json.loads(path.read_bytes())
        assert receipt["kind"] == "workphone-package-job" and receipt["version"] == 1
        assert pathlib.Path(receipt["project"]).samefile(project)
        assert receipt["status"] in ("succeeded", "failed", "cancelled", "timed out")
        for entry in receipt["files"]:
            data = (path.parent / entry["name"]).read_bytes()
            assert len(data) == entry["size"] and fingerprint(data) == entry["hash"]


def check_history(exe, root, environment=None):
    root.mkdir()
    jobs = root / ".workphone" / "package-jobs"
    jobs.mkdir(parents=True)

    def invoke(apply=False, count=0, budget=0, expected=0):
        result = subprocess.run([str(exe), "--apply-prune-jobs" if apply else "--prune-jobs",
                                 str(root), str(count), str(budget)], cwd=exe.parent,
                                capture_output=True, text=True, env=environment, timeout=30)
        assert result.returncode == expected, (result.returncode, result.stdout, result.stderr)
        return result

    def bundle(stamp):
        identity = str(uuid.uuid4())
        files = []
        for suffix, data in ((".log", b"owned log\n"), (".preview.json", b"{\"owned\":true}")):
            path = jobs / (identity + suffix)
            path.write_bytes(data)
            files.append(dict(name=path.name, size=len(data), hash=fingerprint(data)))
        receipt = dict(version=1, kind="workphone-package-job", id=identity,
                       project=str(root), status="succeeded", completedMs=stamp, files=files)
        path = jobs / (identity + ".completed.json")
        path.write_text(json.dumps(receipt), encoding="utf-8")
        return identity, path

    ordinary = [bundle(10000 + index) for index in range(6)]
    modified, modified_receipt = bundle(0)
    (jobs / (modified + ".log")).write_bytes(b"user modified this log")
    uncertain, uncertain_receipt = bundle(0)
    (jobs / (uncertain + ".interrupted.json")).write_bytes(b"inspect publication outcome")
    foreign, foreign_receipt = bundle(0)
    other = root.parent / "other-history-project"
    other.mkdir()
    foreign_data = json.loads(foreign_receipt.read_bytes())
    foreign_data["project"] = str(other)
    foreign_receipt.write_text(json.dumps(foreign_data), encoding="utf-8")
    unknown, unknown_receipt = bundle(0)
    (jobs / (unknown + ".pruning")).write_bytes(b"unknown intent")
    escaped, escaped_receipt = bundle(0)
    authored = root / "authored-source.lua"
    authored.write_bytes(b"return 'authored data must survive history cleanup'")
    escaped_data = json.loads(escaped_receipt.read_bytes())
    escaped_data["files"][0]["name"] = "../authored-source.lua"
    escaped_receipt.write_text(json.dumps(escaped_data), encoding="utf-8")
    other_producer, other_receipt = bundle(0)
    other_data = json.loads(other_receipt.read_bytes())
    other_data["kind"] = "unrelated-tool"
    other_receipt.write_text(json.dumps(other_data), encoding="utf-8")
    resumed, resumed_receipt = bundle(0)
    (jobs / (resumed + ".pruning")).write_bytes(
        f"WORKPHONE_PACKAGE_PRUNE_V1\n{fingerprint(resumed_receipt.read_bytes())}\n".encode("utf-8"))
    (jobs / (resumed + ".log")).unlink()  # Prior process stopped after deleting its first member.
    malformed = jobs / (str(uuid.uuid4()) + ".completed.json")
    malformed.write_bytes(b"{}")
    legacy = jobs / (str(uuid.uuid4()) + ".log")
    legacy.write_bytes(b"legacy log without ownership receipt")
    protected = {path: path.read_bytes() for path in jobs.iterdir() if path.is_file() and
                 any(path.name.startswith(identity) for identity in (modified, uncertain, foreign, unknown, escaped, other_producer))}
    protected[malformed] = malformed.read_bytes()
    protected[legacy] = legacy.read_bytes()
    protected[authored] = authored.read_bytes()
    before = {path: path.read_bytes() for path in jobs.iterdir() if path.is_file()}
    dry = invoke()
    assert "removed 0" in dry.stdout
    assert before == {path: path.read_bytes() for path in jobs.iterdir() if path.is_file()}
    applied = invoke(True, count=2, budget=1024 * 1024)
    assert "removed 5" in applied.stdout, (applied.stdout, applied.stderr)  # Four old bundles + resumed bundle.
    for identity, path in ordinary[:4]:
        assert not path.exists() and not (jobs / (identity + ".log")).exists()
    for identity, path in ordinary[4:]:
        assert path.exists() and (jobs / (identity + ".log")).is_file()
    assert not resumed_receipt.exists() and not (jobs / (resumed + ".pruning")).exists()
    assert all(path.read_bytes() == data for path, data in protected.items())
    assert "removed 0" in invoke(True, count=2, budget=1024 * 1024).stdout

    # An open UI log viewer and an active request lock both prevent pruning.
    opened, opened_receipt = bundle(0)
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel.CreateFileW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD,
                                  ctypes.c_void_p, wintypes.DWORD, wintypes.DWORD, wintypes.HANDLE]
    kernel.CreateFileW.restype = wintypes.HANDLE
    kernel.CloseHandle.argtypes = [wintypes.HANDLE]
    handle = kernel.CreateFileW(str(jobs / (opened + ".log")), 0x80000000, 1, None, 3, 0x80, None)
    assert handle and handle != wintypes.HANDLE(-1).value
    try:
        invoke(True, count=2, budget=1024 * 1024)
        assert opened_receipt.exists() and (jobs / (opened + ".log")).is_file()
    finally:
        kernel.CloseHandle(handle)
    lock_path = jobs / (opened + ".json.lock")
    handle = kernel.CreateFileW(str(lock_path), 0xC0010000, 0, None, 1, 0x04000000, None)
    assert handle and handle != wintypes.HANDLE(-1).value
    try:
        assert "skipped 1" in invoke(True, count=2, budget=1024 * 1024).stdout
        assert opened_receipt.exists()
    finally:
        kernel.CloseHandle(handle)
    invoke(True, count=2, budget=1024 * 1024)
    assert not opened_receipt.exists()
    # A zero byte budget is an explicit request to prune every eligible bundle.
    invoke(True, count=4096, budget=0)
    assert not any(path.exists() for _, path in ordinary)
    assert all(path.read_bytes() == data for path, data in protected.items())
    invoke(count=-1, expected=1)
    invoke(count=4097, expected=1)
    invoke(budget=1024 * 1024 * 1024 + 1, expected=1)
    assert not list(jobs.glob("*.lock"))
    print("PASS: owned history count/byte retention, dry-run, modification/lock guards and prune restart")
