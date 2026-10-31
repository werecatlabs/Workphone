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
                assert list(project.glob(".wp-package-*")) and not output.exists()
            if phase=="published":
                assert (output/"runtime.wprm").is_file() and len(contents(output))==2
            process.kill(); process.communicate(timeout=10)
        finally:
            if process.poll() is None:
                process.kill(); process.communicate(timeout=10)
        assert contents(old)==last_good,"Interrupted work changed the previous published package"
        assert output.exists()==(phase=="published"),"Package visibility must follow the commit boundary"
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
