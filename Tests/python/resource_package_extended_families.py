"""Cook new typed families with the real CLI, then verify a source-free package.

This checks catalog/UUID capture, compiler dependency closures and sealed package
I/O. Native animation, deformation, UI/particle installation and device behavior
are covered by the separate C++ consumers, not by --verify.
"""

from contextlib import closing
from copy import deepcopy
import argparse
import json
import os
import pathlib
import shutil
import sqlite3
import struct
import subprocess
import tempfile
import time
import uuid


ROOTS = (
    "actor.agraph", "upper.amask", "move.aretgt", "reach.arig", "face.amorph",
    "face.fontres", "theme.uistyle", "translations.locres", "layout.uires",
    "effect.partres",
)
EXTENSIONS = {
    "agraph", "amask", "aretgt", "arig", "amorph", "skelres", "mesh",
    "fontres", "uistyle", "locres", "uires", "partres", "texres", "matres",
}


def write_json(path, value):
    path.write_text(json.dumps(value, ensure_ascii=False, allow_nan=False), encoding="utf-8")


def rig_source(target=False):
    names = ("Base", "Elbow", "Tip") if target else ("Root", "Mid", "Hand")
    bones = "".join(
        f'<bone id="{index}" name="{name}"><position x="{int(index != 0)}" y="0" z="0"/></bone>'
        for index, name in enumerate(names)
    )
    hierarchy = "".join(
        f'<boneparent bone="{names[index]}" parent="{names[index - 1]}"/>'
        for index in (1, 2)
    )
    animations = ""
    if not target:
        for clip in ("Idle", "Move"):
            tracks = []
            for index, name in enumerate(names):
                start = float(index != 0)
                finish = .2 if index == 0 and clip == "Move" else start
                keys = "".join(
                    f'<keyframe time="{at}"><translate x="{position}" y="0" z="0"/></keyframe>'
                    for at, position in ((0, start), (1, finish))
                )
                tracks.append(f'<track bone="{name}" handle="{index}"><keyframes>{keys}</keyframes></track>')
            animations += f'<animation name="{clip}" length="1"><tracks>{"".join(tracks)}</tracks></animation>'
        animations = f"<animations>{animations}</animations>"
    return f"<skeleton><bones>{bones}</bones><bonehierarchy>{hierarchy}</bonehierarchy>{animations}</skeleton>"


def create_sources(source):
    """Return canonical captured UUIDs and immutable valid JSON recipes."""
    source.mkdir()
    (source / "rig.skelres").write_text(rig_source(), encoding="utf-8")
    (source / "target.skelres").write_text(rig_source(target=True), encoding="utf-8")
    positions = ((-.2, -.2), (.2, -.2), (0, .2))
    mesh = struct.pack("<4s7I6f", b"WPM1", 1, 3, 1, 0, 3, 3, 1, -.2, -.2, .5, .2, .2, .5)
    mesh += b"".join(struct.pack("<8f", x, y, .5, 0, 0, -1, .5, .5) for x, y in positions)
    (source / "base.mesh").write_bytes(mesh + struct.pack("<6I", 0, 1, 2, 0, 3, 0))
    image = bytearray(18)
    image[2], image[12], image[14], image[16], image[17] = 2, 16, 16, 32, 0x28
    (source / "atlas.tga").write_bytes(image + bytes((255, 255, 255, 255)) * 16 * 16)
    recipes = {
        "upper.amask": dict(version=1, rig="data://rig.skelres", defaultWeight=1,
                            bones=[dict(bone="Root", weight=.5), dict(bone="Hand", weight=.25)]),
        "actor.agraph": dict(
            version=1, rig="data://rig.skelres", mask="data://upper.amask", initialState="Idle",
            clips=[dict(id="idle", clip="Idle", speed=1, loop=True), dict(id="move", clip="Move", speed=1, loop=True)],
            parameters=[dict(name="blend", type="float", value=.5)],
            nodes=[dict(id="idlePose", type="clip", clip="idle", speed=1, loop=True),
                   dict(id="movePose", type="clip", clip="move", speed=1, loop=True),
                   dict(id="blendPose", type="blend1d", parameter="blend", children=["idlePose", "movePose"],
                        ranges=[dict(input0=0, input1=1, low=0, high=1)])],
            states=[dict(id="Idle", node="blendPose", speed=1)], transitions=[]),
        "move.aretgt": dict(version=1, source="data://rig.skelres", target="data://target.skelres",
                            clips=[dict(source="Move", target="Walk")],
                            bones=[dict(source=a, target=b, translationScale=1)
                                   for a, b in zip(("Root", "Mid", "Hand"), ("Base", "Elbow", "Tip"))]),
        "reach.arig": dict(version=1, animation="data://actor.agraph", constraints=[dict(
            id="Reach", effector="Hand", targetPosition=[1, 1, 0], targetRotation=[1, 0, 0, 0],
            pole=[0, 0, 1], weight=1, chainRotationWeight=0, blendMode="effector",
            matchTargetOrientation=False, enabled=True)]),
        "face.amorph": dict(version=1, mesh="data://base.mesh", clip="Face", duration=1,
                            targets=[dict(id="Smile", deltas=[dict(vertex=0, position=[0, .2, 0], normal=[1, 0, 0])])],
                            keys=[dict(time=0, weights=[0]), dict(time=1, weights=[1])]),
        "atlas.texres": dict(version=1, source="data://atlas.tga", mipFilter="none", atlasColumns=1, alphaCutoff=.5),
        "surface.matres": dict(version=1, baseColour=[1, 1, 1, 1], metalness=0, roughness=1,
                               baseColourTexture="data://atlas.texres"),
        "face.fontres": dict(version=1, atlas="data://atlas.texres", whitePixel=[0, 0], height=16,
                             ascent=12, descent=-4, fallback=63, glyphs=[dict(
                                 codepoint=codepoint, advance=advance, bounds=[0, 0, 8, 16], uv=[0, 0, .5, 1])
                                 for codepoint, advance in ((63, 8), (65, 9))]),
        "theme.uistyle": dict(version=1, styles=[dict(name="hud", colour=[.25, .5, .75, 1], textSize=23)]),
        "translations.locres": dict(version=1, fallback="en", locales={
            "en": dict(greeting="Package ready", confirm="Confirm"),
            "fr": dict(greeting="Bonjour capturé", confirm="Confirmer")}),
        "effect.partres": dict(version=1, capacity=128, seed=7, rate=32, lifetime=[1, 1], size=[.5, .5],
                               velocity=[0, 0, 0], gravity=[0, 0, 0], colourStart=[1, 1, 1, 1],
                               colourEnd=[1, 1, 1, 1], duration=0, looping=True, material="data://surface.matres"),
        "layout.uires": {},
    }
    for name, recipe in recipes.items():
        write_json(source / name, recipe)
    bindings = {
        path.name: str(uuid.uuid5(uuid.NAMESPACE_URL, "workphone:extended-family:" + path.name))
        for path in source.iterdir() if path.suffix[1:] in EXTENSIONS
    }
    recipes["layout.uires"] = dict(version=2, styleAssetUuid=bindings["theme.uistyle"],
        localizationAssetUuid=bindings["translations.locres"], locale="fr", nodes=[
            dict(type="container", id=1, parent=-1, label="Captured family UI", position=[0, 0],
                 size=[.5, .3], visible=True, enabled=True),
            dict(type="text", id=2, parent=0, label="Source text", localeKey="greeting", styleClass="hud",
                 fontAssetUuid=bindings["face.fontres"], position=[.02, .02], size=[.4, .08], visible=True, enabled=True),
            dict(type="image", id=3, parent=0, label="Captured image", textureAssetUuid=bindings["atlas.texres"],
                 position=[.02, .12], size=[.1, .1], visible=True, enabled=True),
            dict(type="button", id=4, parent=0, label="Source confirm", localeKey="confirm",
                 fontAssetUuid=bindings["face.fontres"], position=[.16, .12], size=[.25, .08], visible=True, enabled=True)])
    write_json(source / "layout.uires", recipes["layout.uires"])
    assert len(bindings) == 15 and set(ROOTS) <= bindings.keys()
    return bindings, recipes


def package_bytes(directory):
    return {path.relative_to(directory): path.read_bytes() for path in directory.rglob("*") if path.is_file()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("tool", type=pathlib.Path, help="WorkphoneResourcePackage.exe")
    parser.add_argument("--retain-fixture", type=pathlib.Path,
                        help="fresh directory for the verified source-free package and fixture.json")
    options = parser.parse_args()
    tool = options.tool.resolve(strict=True)
    assert tool.is_file(), tool
    retained = None
    if options.retain_fixture is not None:
        requested = options.retain_fixture.absolute()
        if requested.exists() or requested.is_symlink():
            raise FileExistsError(f"Retained fixture destination must be fresh: {requested}")
        retained = requested.resolve()
    environment = dict(os.environ)
    system_root = environment.get("SYSTEMROOT", r"C:\Windows")
    environment["PATH"] = str(pathlib.Path(system_root) / "System32") + os.pathsep + system_root
    for key in tuple(environment):
        if key.upper().startswith(("WP_", "LUA_PATH", "LUA_CPATH")):
            environment.pop(key)
    started = time.monotonic()

    def run(arguments, success=True, diagnostic=None):
        result = subprocess.run([str(tool), *map(str, arguments)], cwd=tool.parent,
                                capture_output=True, text=True, encoding="utf-8", errors="replace",
                                env=environment, timeout=90)
        output = result.stdout + "\n" + result.stderr
        assert (result.returncode == 0) if success else (result.returncode == 1), (
            result.returncode, list(map(str, arguments)), output)
        if diagnostic:
            assert diagnostic.lower() in output.lower(), (diagnostic, output)
        return result

    with tempfile.TemporaryDirectory(prefix="wp-extended-families-") as temporary:
        root = pathlib.Path(temporary)
        source, cooked, package = (root / name for name in ("authoring files", "cooked cache", "checked package"))
        bindings, recipes = create_sources(source)
        catalog = root / "canonical catalog.db"
        with closing(sqlite3.connect(catalog)) as database, database:
            database.execute("CREATE TABLE resources(id INTEGER PRIMARY KEY,uuid TEXT,path TEXT,type TEXT)")
            database.executemany("INSERT INTO resources(uuid,path,type) VALUES(?,?,?)", (
                (identity, name, pathlib.Path(name).suffix[1:]) for name, identity in sorted(bindings.items())))

        def build_arguments(output, roots=ROOTS):
            args = [source, catalog, cooked, output, "pc", "--build-profile", "extended-family-shipping",
                    "--toolchain", "extended-family-native-cooker-v1", "--build-settings",
                    json.dumps(dict(animation="typed", ui="captured", quality="test"), sort_keys=True)]
            for name in roots:
                args += ["--root", bindings[name]]
            for extension in sorted(EXTENSIONS):
                args += ["--map", extension + "=" + extension]
            return args

        run(build_arguments(package))
        before = package_bytes(package)
        assert pathlib.Path("runtime.wprm") in before and len(before) == len(bindings) + 1, before.keys()
        assert all(path.name == "runtime.wprm" or path.suffix == ".wprs" for path in before), before.keys()

        failures = []
        graph = deepcopy(recipes["actor.agraph"])
        graph["nodes"][2]["children"][0] = "blendPose"
        failures.append(("actor.agraph", graph, "Cyclic animation pose graph"))
        retarget = deepcopy(recipes["move.aretgt"])
        retarget["bones"][1]["target"], retarget["bones"][2]["target"] = "Tip", "Elbow"
        failures.append(("move.aretgt", retarget, "immediate source parent"))
        morph = deepcopy(recipes["face.amorph"])
        morph["targets"][0]["deltas"][0]["normal"] = [0, 0, 2]
        failures.append(("face.amorph", morph, "degenerate native normal"))
        ui = deepcopy(recipes["layout.uires"])
        ui["nodes"][1]["fontAssetUuid"] = bindings["upper.amask"]
        failures.append(("layout.uires", ui, "UI asset UUID is missing or has the wrong type"))
        particle = deepcopy(recipes["effect.partres"])
        particle["authoringPath"] = "untracked.particle"
        failures.append(("effect.partres", particle, "Unsupported particle descriptor field"))
        for index, (name, invalid, diagnostic) in enumerate(failures):
            rejected = root / f"rejected package {index}"
            try:
                write_json(source / name, invalid)
                run(build_arguments(rejected, (name,)), success=False, diagnostic=diagnostic)
                assert not rejected.exists(), f"Invalid {name} published a package"
            finally:
                write_json(source / name, recipes[name])
            assert package_bytes(package) == before, f"Rejected {name} changed the prior successful package"

        # Only explicitly named children of this temporary workspace are removed.
        assert source.parent == root and cooked.parent == root and catalog.parent == root
        shutil.rmtree(source)
        shutil.rmtree(cooked)
        catalog.unlink()
        for sidecar in (pathlib.Path(str(catalog) + suffix) for suffix in ("-wal", "-shm", "-journal")):
            sidecar.unlink(missing_ok=True)
        assert not source.exists() and not cooked.exists() and not catalog.exists()
        try:
            for path in before:
                (package / path).chmod(0o444)
            run(["--verify", package, "pc", *[bindings[name] for name in sorted(bindings)]])
            assert package_bytes(package) == before, "Source-free UUID verification changed read-only package membership/bytes"
            if retained is not None:
                # Publish only verified runtime files. The authoring tree, cooking
                # cache, catalog and SQLite sidecars have already been removed.
                retained.mkdir(parents=True, exist_ok=False)
                shutil.copytree(package, retained / "package")
                descriptor = dict(schema="workphone.extended-typed-fixture.v1", target="pc",
                                  uuids=sorted(bindings.values()))
                assert all(str(uuid.UUID(identity)) == identity for identity in descriptor["uuids"])
                with (retained / "fixture.json").open("x", encoding="utf-8") as output:
                    json.dump(descriptor, output, ensure_ascii=False, sort_keys=True)
                    output.write("\n")
                for path in before:
                    (retained / "package" / path).chmod(0o444)
                (retained / "fixture.json").chmod(0o444)
                assert package_bytes(retained / "package") == before, "Retained package differs from verified runtime bytes"
                assert package_bytes(package) == before, "Fixture retention changed the verified read-only source package"
                assert {path.name for path in retained.iterdir()} == {"package", "fixture.json"}
                assert json.loads((retained / "fixture.json").read_text(encoding="utf-8")) == descriptor
        finally:
            for path in before:
                (package / path).chmod(0o666)
        print(f"PASS: {len(ROOTS)} extended typed roots, {len(bindings)} captured closure UUIDs, "
              f"{len(failures)} rejected semantic/schema replacements; source-free read-only CLI verification "
              f"({time.monotonic() - started:.2f}s)")
        print("BOUNDARY: native animation/deformation, UI/particle installation and hardware behavior use separate C++ consumers.")
        if retained is not None:
            print(f"Retained source-free fixture: {retained}")


if __name__ == "__main__":
    main()
