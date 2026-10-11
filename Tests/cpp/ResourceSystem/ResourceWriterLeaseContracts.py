"""Exercise native authoring writer admission across independent processes."""

import pathlib
import subprocess
import sys
import tempfile
import time


def run(executable):
    executable = pathlib.Path(executable).resolve()
    with tempfile.TemporaryDirectory(prefix="wp-resource-writer-") as temporary:
        root = pathlib.Path(temporary)
        source, cooked = root / "source", root / "cooked"
        source.mkdir()
        cooked.mkdir()

        def probe(target="windows", output=cooked, database=None):
            arguments = [str(executable), "--probe-writer", str(source), str(output), target]
            if database is not None:
                arguments.append(str(database))
            return subprocess.run(arguments, cwd=executable.parent, capture_output=True, text=True, timeout=15)

        def start_owner(marker):
            process = subprocess.Popen(
                [str(executable), "--hold-writer", str(source), str(cooked), "windows", str(marker)],
                cwd=executable.parent, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
            )
            deadline = time.monotonic() + 15
            try:
                while not marker.exists():
                    if process.poll() is not None:
                        raise AssertionError("Owner failed before readiness: " + "".join(process.communicate()))
                    if time.monotonic() >= deadline:
                        raise AssertionError("Owner readiness timed out")
                    time.sleep(0.02)
            except BaseException:
                if process.poll() is None:
                    process.kill()
                process.communicate(timeout=15)
                raise
            return process

        owner = start_owner(root / "ready-one")
        try:
            rejected = probe()
            assert rejected.returncode != 0 and "writer lease" in rejected.stderr, rejected.stdout + rejected.stderr
            assert probe("linux").returncode == 0, "Different targets must coexist"
            alias = cooked / "alias"
            alias.mkdir()
            assert probe(output=alias / "..").returncode != 0, "Canonical-root aliases must not bypass writer ownership"
            owner.communicate(input="\n", timeout=15)
            assert owner.returncode == 0, "Graceful owner shutdown failed"
        finally:
            if owner.poll() is None:
                owner.kill()
            owner.communicate(timeout=15)
        assert probe().returncode == 0, "Graceful shutdown must release writer ownership"

        owner = start_owner(root / "ready-two")
        try:
            owner.kill()
            owner.communicate(timeout=15)
        finally:
            if owner.poll() is None:
                owner.kill()
            owner.communicate(timeout=15)
        assert probe().returncode == 0, "Process termination must release ownership without deleting lock files"

        outside_database = root / "outside.sqlite"
        assert probe(database=outside_database).returncode != 0 and not outside_database.exists(), \
            "External index paths must be rejected without creating a database"
        other_cooked = root / "other-cooked"
        other_cooked.mkdir()
        assert probe(output=other_cooked, database=cooked / ".resource_compilation.db").returncode != 0, \
            "Another cooked root must not bypass writer admission by sharing an index"
        invalid_database = cooked / "database-is-a-directory"
        invalid_database.mkdir()
        assert probe(database=invalid_database).returncode != 0, "Fixture must fail database initialization"
        assert probe().returncode == 0, "Failed initialization must release its acquired writer lease"
    print("Native cross-process writer admission and release contracts passed")


if __name__ == "__main__":
    run(sys.argv[1])
