"""Kill actual ResourceSystem requests on both sides of the publication commit."""
import pathlib
import shutil
import subprocess
import sys
import tempfile
import time

host=pathlib.Path(sys.argv[1]).resolve()


def run(arguments, expected=0):
    result=subprocess.run([str(host),*map(str,arguments)],cwd=host.parent,capture_output=True,text=True,timeout=30)
    assert result.returncode==expected,(result.returncode,result.stdout,result.stderr)
    return result.stdout


def contents(folder):
    return {path.relative_to(folder):path.read_bytes() for path in folder.rglob("*") if path.is_file()}


with tempfile.TemporaryDirectory(prefix="wp-publication-crash-") as temporary:
    for phase in ("compile","admission","published"):
        project=pathlib.Path(temporary)/phase
        source=project/"source"
        source.mkdir(parents=True)
        descriptor=source/"asset.crashres"
        descriptor.write_bytes(b"revision-one\n")
        old=project/"last-good-package"
        readiness=project/"ready"
        run(["--export",project,"none",old,readiness])
        last_good=contents(old)
        descriptor.write_bytes(b"revision-two\n")
        output=project/"interrupted-output"
        process=subprocess.Popen([str(host),"--export",str(project),phase,str(output),str(readiness)],
                                 cwd=host.parent,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
        try:
            deadline=time.monotonic()+20
            while not readiness.exists() and process.poll() is None and time.monotonic()<deadline:
                time.sleep(0.02)
            assert readiness.exists() and readiness.read_text()==phase,"Native crash barrier not reached"
            if phase=="admission":
                stages=[path for path in project.glob(".wp-package-*") if path.is_dir()]
                assert len(stages)==1 and not output.exists()
                stage=stages[0]
                owner=stage.with_name(stage.name+".owner")
                assert owner.name in {path.name for path in project.iterdir()}
                assert "Recoverable 0; recovered 0; locked 1; warnings 0" in run(["--inspect-stages",project])
                assert "Recoverable 0; recovered 0; locked 1; warnings 0" in run(["--recover-stages",project])
                assert stage.is_dir() and owner.name in {path.name for path in project.iterdir()}
            if phase=="published":
                assert (output/"runtime.wprm").is_file() and len(contents(output))==2
            process.kill(); process.communicate(timeout=10)
        finally:
            if process.poll() is None:
                process.kill(); process.communicate(timeout=10)
        assert contents(old)==last_good,"Interrupted work changed the previous published package"
        assert output.exists()==(phase=="published"),"Package visibility must follow the commit boundary"
        if phase=="admission":
            staged=contents(stage)
            owner_bytes=owner.read_bytes()
            assert "Recoverable 1; recovered 0; locked 0; warnings 0" in run(["--inspect-stages",project])
            assert contents(stage)==staged and owner.read_bytes()==owner_bytes
            archive=stage.with_name(stage.name+".interrupted")
            archive.mkdir()
            sentinel=archive/"keep"
            sentinel.write_bytes(b"existing archive")
            assert "Recoverable 0; recovered 0; locked 0; warnings 1" in run(["--recover-stages",project])
            assert sentinel.read_bytes()==b"existing archive" and contents(stage)==staged
            sentinel.unlink()
            archive.rmdir()
            parked=project/"original-stage"
            stage.rename(parked)
            stage.mkdir()
            unrelated=stage/"keep"
            unrelated.write_bytes(b"replacement directory")
            assert "Recoverable 0; recovered 0; locked 0; warnings 1" in run(["--recover-stages",project])
            assert unrelated.read_bytes()==b"replacement directory" and owner.read_bytes()==owner_bytes
            unrelated.unlink()
            stage.rmdir()
            parked.rename(stage)
            legacy=project/".wp-package-1-1"
            legacy.mkdir()
            (legacy/"unowned").write_bytes(b"untouched")
            assert "Recoverable 1; recovered 1; locked 0; warnings 1" in run(["--recover-stages",project])
            assert not stage.exists() and not owner.exists() and contents(archive)==staged
            assert (legacy/"unowned").read_bytes()==b"untouched"
            assert "Recoverable 0; recovered 0; locked 0; warnings 1" in run(["--recover-stages",project])
            redirected=project/".wp-package-1-2"
            try:
                redirected.symlink_to(archive, target_is_directory=True)
            except OSError:
                pass  # Creating symlinks can require Windows developer mode.
            else:
                marker=redirected.with_name(redirected.name+".owner")
                marker.write_bytes(owner_bytes.replace(stage.name.encode("ascii"), redirected.name.encode("ascii"), 1))
                assert "Recoverable 0; recovered 0; locked 0; warnings 2" in run(["--recover-stages",project])
                assert redirected.is_symlink() and marker.is_file() and contents(archive)==staged
        else:
            assert "Recoverable 0; recovered 0; locked 0; warnings 0" in run(["--recover-stages",project])
            assert not list(project.glob(".wp-package-*"))
        # Verify with source and cook/index trees unavailable and payload files read-only.
        hidden_source=project/"hidden-source"; hidden_cooked=project/"hidden-cooked"
        source.rename(hidden_source); (project/"cooked").rename(hidden_cooked)
        outputs=[(old,"revision-one")]+([(output,"revision-two")] if phase=="published" else [])
        for package,revision in outputs:
            before=contents(package)
            try:
                for path in package.rglob("*"):
                    if path.is_file(): path.chmod(0o444)
                run(["--verify",package,revision])
                assert contents(package)==before
                assert not source.exists() and not (project/"cooked").exists()
            finally:
                for path in package.rglob("*"):
                    if path.is_file(): path.chmod(0o666)
        hidden_source.rename(source); hidden_cooked.rename(project/"cooked")
        # OS writer admission and the real compilation index must reopen after kill.
        fresh=project/"explicit-retry"
        run(["--export",project,"none",fresh,project/"retry-ready"])
        run(["--verify",fresh,"revision-two"])
        if phase=="published":
            committed=contents(output)
            run(["--export",project,"none",output,project/"retry-ready"],expected=1)
            assert contents(output)==committed,"Retry must not replace committed output"
        assert contents(old)==last_good
        print(f"PASS: kill during {phase}, last-good bytes, read-only source-free mount and restart export")
print("Publication process-kill contracts passed; physical power-loss and typed-compiler interruption are not certified")
