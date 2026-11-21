"""Tool catalog and process owner for a Workphone editor MCP session."""

import base64
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import time
import zlib


def tool(name, description, properties=None, required=()):
    schema = {"type": "object", "properties": properties or {}, "additionalProperties": False}
    if required:
        schema["required"] = list(required)
    return {"name": name, "description": description, "inputSchema": schema}


TEXT = {"type": "string", "minLength": 1, "maxLength": 1024}
SOURCE = {"type": "string", "maxLength": 262144}
BOOL = {"type": "boolean"}
VECTOR = {"type": "array", "items": {"type": "number"}, "minItems": 3, "maxItems": 3}
TRANSFORM = {"position": VECTOR, "rotation": VECTOR, "scale": VECTOR, "enabled": BOOL}
COMPONENT = {"id": TEXT, "component": TEXT}
TOOLS = [
    tool("workspace_list", "List one directory under the projects workspace.", {"path": TEXT}),
    tool("project_create", "Create a native game project and its first scene in a new or empty workspace-relative directory.",
         {"path": TEXT}, ["path"]),
    tool("project_open", "Open a workspace-relative .fbproject or .fbp and its startup scene. Save current edits first.",
         {"path": TEXT}, ["path"]),
    tool("project_save", "Save the open scene, scripts list and game project."),
    tool("scene_create", "Create and select a new project-relative scene. Save current edits first.",
         {"path": TEXT}, ["path"]),
    tool("scene_open", "Load a project-relative scene and make it the startup scene. Save current edits first.",
         {"path": TEXT}, ["path"]),
    tool("scene_inspect", "Inspect the live scene hierarchy, actor UUIDs, local transforms and attached components."),
    tool("actor_create", "Create an actor, optionally under a parent UUID. Rotation is local Euler degrees.",
         {"name": TEXT, "kind": {"type": "string", "enum": [
             "empty", "cube", "plane", "physics_cube", "camera", "directional_light", "point_light"]},
          "parent": TEXT, **TRANSFORM}, ["name"]),
    tool("actor_edit", "Rename, enable or transform an actor by UUID. Rotation is local Euler degrees.",
         {"id": TEXT, "name": TEXT, **TRANSFORM}, ["id"]),
    tool("actor_remove", "Remove an actor from the current scene by UUID.", {"id": TEXT}, ["id"]),
    tool("component_types", "List the component types registered by this editor and its plugins."),
    tool("component_add", "Add a registered component type to an actor UUID.",
         {"id": TEXT, "type": TEXT}, ["id", "type"]),
    tool("component_inspect", "Read the inspector properties of a component UUID on an actor UUID.",
         COMPONENT, COMPONENT),
    tool("component_edit", "Set component inspector fields using their displayed names and string values.",
         {**COMPONENT, "properties": {"type": "object", "additionalProperties": {"type": "string"}}},
         ["id", "component", "properties"]),
    tool("component_remove", "Remove a component UUID from an actor UUID.", COMPONENT, COMPONENT),
    tool("script_read", "Read a UTF-8 Lua file by workspace-relative path.", {"path": TEXT}, ["path"]),
    tool("script_write", "Create or replace a UTF-8 .lua file at a workspace-relative path.",
         {"path": TEXT, "source": SOURCE}, ["path", "source"]),
    tool("script_attach", "Load a project-relative .lua file and attach its gameplay class to an actor UUID.",
         {"id": TEXT, "path": TEXT, "className": TEXT}, ["id", "path", "className"]),
    tool("editor_lua", "Run trusted Lua in the live editor VM with the editor's normal permissions. Return a JSON-compatible value.",
         {"source": SOURCE}, ["source"]),
    tool("play_start", "Enter editor Play mode and take a restorable scene snapshot."),
    tool("play_input", "Press or release a virtual key in the game. Held keys persist across steps (32 = Space, 65 = A).",
         {"key": {"type": "integer", "minimum": 1, "maximum": 255}, "down": BOOL}, ["key", "down"]),
    tool("play_pointer", "Move, press, release or scroll in the game. x/y are normalized to the full editor capture from top left. "
         "Supply button for press/release, or a nonzero wheelDelta for wheel (120 per notch).",
         {"action": {"type": "string", "enum": ["move", "press", "release", "wheel"]},
          "x": {"type": "number", "minimum": 0, "maximum": 1},
          "y": {"type": "number", "minimum": 0, "maximum": 1},
          "button": {"type": "string", "enum": ["left", "right", "middle"]},
          "wheelDelta": {"type": "number", "minimum": -12000, "maximum": 12000}}, ["action", "x", "y"]),
    tool("play_step", "Advance the running game by application ticks. Defaults to one.",
         {"frames": {"type": "integer", "minimum": 1, "maximum": 120}}),
    tool("play_status", "Read the current gameplay tick, keyboard availability and play state."),
    tool("play_stop", "Leave Play mode and restore the scene captured before play."),
    tool("editor_capture", "Read the last rendered editor back buffer as a PNG image."),
    tool("editor_log", "Read the latest editor process diagnostics."),
    tool("editor_close", "Close the owned editor and discard unsaved edits. Use project_open to reload saved projects and Lua from disk."),
]


def frame_png(path, width, height):
    if not (0 < width <= 16384 and 0 < height <= 16384 and width * height <= 32 * 1024 * 1024):
        raise ValueError("Invalid capture dimensions")
    if path.stat().st_size != width * height * 4:
        raise ValueError("Incomplete frame capture")
    compressor = zlib.compressobj(3)
    compressed = bytearray()
    with path.open("rb") as frame:
        for _ in range(height):
            row = bytearray(frame.read(width * 4))
            row[3::4] = b"\xff" * width
            compressed.extend(compressor.compress(b"\x00" + row))
    compressed.extend(compressor.flush())

    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(data, zlib.crc32(kind)))

    header = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    return b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header) + chunk(b"IDAT", compressed) + chunk(b"IEND", b"")


class Controller:
    def __init__(self, editor, workspace):
        if not editor.is_file():
            raise ValueError(f"Editor executable not found: {editor}")
        self.editor = editor
        self.workspace = workspace
        self.workspace.mkdir(parents=True, exist_ok=True)
        self.process = None
        self.session = None

    def workspace_path(self, name):
        raw = Path(name)
        if raw.anchor or ":" in name or "\x00" in name:
            raise ValueError("Expected a workspace-relative path")
        path = (self.workspace / raw).resolve()
        if not path.is_relative_to(self.workspace):
            raise ValueError("Path is outside the workspace")
        return path

    def log_tail(self):
        if self.session is None:
            return ""
        with (Path(self.session.name) / "editor.log").open("rb") as stream:
            stream.seek(max(0, stream.seek(0, 2) - 8192))
            return stream.read().decode("utf-8", errors="replace")

    def wait_reply(self, timeout):
        reply = Path(self.session.name) / "reply.json"
        deadline = time.monotonic() + timeout
        while not reply.exists():
            code = self.process.poll()
            if code is not None:
                raise RuntimeError(f"Editor exited with code {code}. {self.log_tail()}")
            if time.monotonic() >= deadline:
                raise RuntimeError(f"Editor did not respond within {timeout} seconds. {self.log_tail()}")
            time.sleep(0.02)
        message = json.loads(reply.read_bytes())
        reply.unlink()
        return message

    def start(self):
        self.session = tempfile.TemporaryDirectory(prefix="workphone-mcp-")
        folder = Path(self.session.name)
        # The inherited input pipe also signals shutdown on MCP EOF. All engine
        # logs go to a session-local file, separate from the client's stdout.
        with (folder / "editor.log").open("wb") as log:
            self.process = subprocess.Popen(
                [str(self.editor), "--mcp-workspace", str(self.workspace), "--workspace", str(folder)],
                cwd=folder, stdin=subprocess.PIPE, stdout=log, stderr=subprocess.STDOUT,
                creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
        if self.wait_reply(120).get("ready") is not True:
            raise RuntimeError("Editor startup handshake failed")

    def call(self, name, arguments):
        if name == "workspace_list":
            folder = self.workspace_path(arguments.get("path", "."))
            return {"entries": [{"name": p.name, "directory": p.is_dir()} for p in sorted(folder.iterdir())]}
        if name in ("script_read", "script_write"):
            file = self.workspace_path(arguments["path"])
            if file.suffix.lower() != ".lua":
                raise ValueError("Expected a .lua script")
            if name == "script_read":
                if file.stat().st_size > 1024 * 1024:
                    raise ValueError("Script exceeds 1 MiB")
                return {"source": file.read_text(encoding="utf-8-sig")}
            file.parent.mkdir(parents=True, exist_ok=True)
            temp = None
            try:
                with tempfile.NamedTemporaryFile(mode="wb", dir=file.parent, delete=False) as stream:
                    temp = Path(stream.name)
                    stream.write(arguments["source"].encode("utf-8"))
                os.replace(temp, file)
            finally:
                if temp is not None:
                    temp.unlink(missing_ok=True)
            return {"path": file.relative_to(self.workspace).as_posix()}
        if name == "editor_log":
            return {"log": self.log_tail()}
        if name == "editor_close":
            self.close()
            return {"closed": True}

        if name in ("project_create", "project_open") and self.process is not None:
            if self.native_call("play_status", {})["playing"]:
                raise ValueError("Stop the current playtest before switching projects")
            # Project loads add script classes and asset search paths to the VM.
            # Start the next project in a clean editor. Keep the previous editor
            # usable if the new descriptor fails to open.
            next_editor = Controller(self.editor, self.workspace)
            try:
                result = next_editor.native_call(name, arguments)
                self.close()
                self.process, self.session = next_editor.process, next_editor.session
                next_editor.process = next_editor.session = None
                return result
            finally:
                next_editor.close()
        return self.native_call(name, arguments)

    def native_call(self, name, arguments):
        try:
            if self.process is None:
                self.start()
            request = json.dumps({"name": name, "arguments": arguments}, allow_nan=False).encode() + b"\n"
            self.process.stdin.write(request)
            self.process.stdin.flush()
            reply = self.wait_reply(60)
        except (ValueError, RuntimeError, OSError):
            self.close()
            raise
        if "error" in reply:
            raise RuntimeError(reply["error"])
        result = reply["result"]
        if name == "editor_capture":
            png = frame_png(Path(self.session.name) / "frame.rgba", result["width"], result["height"])
            result.update(mimeType="image/png", data=base64.b64encode(png).decode("ascii"))
        return result

    def close(self):
        if self.process is not None:
            try:
                self.process.stdin.close()
            except OSError:
                pass
            try:
                self.process.wait(5)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait()
            self.process = None
        if self.session is not None:
            self.session.cleanup()
            self.session = None
