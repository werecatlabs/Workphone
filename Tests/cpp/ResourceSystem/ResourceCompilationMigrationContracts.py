"""Run schema migration contracts against a freshly built WPResourceTests executable."""

import pathlib
from contextlib import closing
import sqlite3
import subprocess
import sys
import tempfile


def run(executable):
    executable = pathlib.Path(executable).resolve()
    with tempfile.TemporaryDirectory(prefix="wp-resource-migration-") as temporary:
        for version in (1, 2, 4):
            path = pathlib.Path(temporary) / f"schema-{version}.sqlite"
            with closing(sqlite3.connect(path)) as connection:
                connection.executescript(f"""
                    PRAGMA user_version={version};
                    CREATE TABLE authored_sentinel(uuid TEXT PRIMARY KEY, source TEXT);
                    INSERT INTO authored_sentinel VALUES('retained-uuid','data://source.png');
                    CREATE TABLE compiled_resources(resource_id TEXT PRIMARY KEY);
                    INSERT INTO compiled_resources VALUES('data://legacy.texres');
                    CREATE TABLE compile_dependencies(resource_id TEXT, dependency_path TEXT);
                    INSERT INTO compile_dependencies VALUES('data://legacy.texres','data://source.png');
                """)
                connection.commit()
            result = subprocess.run(
                [str(executable), "--open-index", str(path)],
                cwd=executable.parent,
                capture_output=True,
                text=True,
            )
            assert (result.returncode == 0) == (version < 3), result.stdout + result.stderr
            with closing(sqlite3.connect(path)) as connection:
                assert connection.execute("SELECT * FROM authored_sentinel").fetchall() == [
                    ("retained-uuid", "data://source.png")
                ]
                if version < 3:
                    assert connection.execute("PRAGMA user_version").fetchone() == (3,)
                    assert connection.execute("SELECT * FROM compiled_resources").fetchall() == []
                    assert connection.execute("SELECT * FROM compile_dependencies").fetchall() == []
                    columns = {row[1] for row in connection.execute("PRAGMA table_info(compiled_resources)")}
                    assert {"resource_id", "target", "packaged_build"} <= columns
                else:
                    assert connection.execute("PRAGMA user_version").fetchone() == (4,)
                    assert connection.execute("SELECT * FROM compiled_resources").fetchall() == [
                        ("data://legacy.texres",)
                    ]
    print("Native compilation-index migration and newer-schema rejection contracts passed")


if __name__ == "__main__":
    run(sys.argv[1])
