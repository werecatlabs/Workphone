"""Author a two-step coin game over MCP, play it, then reopen it in a fresh editor.

Run with python Tests/python/workphone_mcp_game.py --editor <path to Editor.exe>.
"""

import argparse
import base64
from contextlib import contextmanager
import json
from pathlib import Path
import queue
import struct
import subprocess
import sys
import tempfile
import threading


SERVER = Path(__file__).resolve().parents[2] / "Tools" / "python" / "workphone_mcp.py"

# A press moves one square. The second press reaches the coin and collects it.
GAME_SCRIPT = """class 'CoinRunner'

function CoinRunner:__init(component)
    self.component = component
    self.wasDown = false
end

function CoinRunner:update()
    local app = IApplicationManager.instance()
    local down = app:getInputDeviceManager():isKeyPressed(68) -- D
    if down and not self.wasDown then
        local player = self.component:getActor()
        local coin = app:getGameManager():getActorByName("Coin")
        local finish = coin:getLocalPosition().x
        local x = math.min(player:getLocalPosition().x + 1, finish)
        player:setLocalPosition(Vector3F(x, 0, 0))
        if x == finish then coin:setEnabled(false, false) end
    end
    self.wasDown = down
end
"""


class Client:
    def __init__(self, process, modern):
        self.process = process
        self.modern = modern
        self.editor_id = None
        self.editor_tools = set()
        self.last_id = 0
        self.lines = queue.Queue()
        self.reader = threading.Thread(target=self.read, daemon=True)
        self.reader.start()

    def read(self):
        try:
            for line in self.process.stdout:
                self.lines.put(line)
        finally:
            self.lines.put(None)

    def send(self, message):
        self.process.stdin.write(json.dumps(message).encode("utf-8") + b"\n")
        self.process.stdin.flush()

    def request(self, method, params):
        if self.modern:
            params = {**params, "_meta": {
                "io.modelcontextprotocol/protocolVersion": "2026-07-28",
                "io.modelcontextprotocol/clientCapabilities": {},
                "io.modelcontextprotocol/clientInfo": {"name": "coin-game-test", "version": "1"},
            }}
        self.last_id += 1
        self.send({"jsonrpc": "2.0", "method": method, "id": self.last_id, "params": params})
        label = params.get("name", method)
        try:
            line = self.lines.get(timeout=120)
        except queue.Empty:
            raise TimeoutError(f"Timed out waiting for {label}") from None
        if line is None:
            raise RuntimeError(f"MCP server closed stdout while running {label}")
        response = json.loads(line)
        if response.get("id") != self.last_id or "result" not in response:
            raise RuntimeError(f"{label} failed: {response}")
        if self.modern:
            assert response["result"]["resultType"] == "complete", label
        return response["result"]

    def call(self, tool, **arguments):
        if tool in self.editor_tools:
            arguments["editorId"] = self.editor_id
        result = self.request("tools/call", {"name": tool, "arguments": arguments})
        if result.get("isError"):
            log_args = {"editorId": self.editor_id} if self.modern else {}
            diagnostics = self.request("tools/call", {"name": "editor_log", "arguments": log_args})
            raise RuntimeError(f"{tool} failed: {result['content']}\n"
                               f"{diagnostics.get('structuredContent', {}).get('log', '')}")
        return result

    def data(self, tool, **arguments):
        return self.call(tool, **arguments)["structuredContent"]


@contextmanager
def connect(editor, workspace, modern=False):
    process = subprocess.Popen(
        [sys.executable, "-B", str(SERVER), "--editor", str(editor), "--workspace", str(workspace)],
        stdin=subprocess.PIPE, stdout=subprocess.PIPE,
    )
    client = Client(process, modern)
    try:
        if modern:
            info = client.request("server/discover", {})
            assert "2026-07-28" in info["supportedVersions"] and "tools" in info["capabilities"], info
            tools = client.request("tools/list", {})["tools"]
            client.editor_tools = {t["name"] for t in tools if "editorId" in t["inputSchema"].get("required", [])}
            client.editor_id = client.data("editor_connect")["editorId"]
        else:
            init = client.request("initialize", {"protocolVersion": "2025-11-25", "capabilities": {},
                                                 "clientInfo": {"name": "coin-game-test", "version": "1"}})
            assert init["protocolVersion"] == "2025-11-25" and "tools" in init["capabilities"], init
            client.send({"jsonrpc": "2.0", "method": "notifications/initialized"})
        yield client
    finally:
        process.stdin.close()
        try:
            process.wait(timeout=45)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
            raise RuntimeError("MCP server did not exit after stdin closed") from None
        finally:
            client.reader.join(timeout=5)
            process.stdout.close()
    if process.returncode:
        raise RuntimeError(f"MCP server exited with status {process.returncode}")


def inspect_game(client, player_id, coin_id, position, coin_enabled):
    actors = {actor["id"]: actor for actor in client.data("scene_inspect")["actors"]}
    for actor_id in (player_id, coin_id):
        assert actor_id in actors, f"Expected actor {actor_id}, got {list(actors)}"
    player, coin = actors[player_id], actors[coin_id]
    assert player["position"] == [position, 0, 0], f"Unexpected player position {player['position']}"
    assert coin["enabled"] is coin_enabled, f"Unexpected coin state {coin}"
    assert any(c.get("className") == "CoinRunner" for c in player["components"]), player


def play(client, player_id, coin_id, unsaved_id=None):
    start = client.data("play_start")
    assert start["playing"] and start["keyboardAvailable"] and start["pointerAvailable"], start
    if unsaved_id:
        assert any(a["id"] == unsaved_id for a in client.data("scene_inspect")["actors"]), "Lost edit on Play"
    client.data("play_step", frames=3)
    inspect_game(client, player_id, coin_id, 0, True)
    client.data("play_input", key=68, down=True)
    ticks = client.data("play_step", frames=4)
    assert ticks["frames"] >= 4, ticks
    inspect_game(client, player_id, coin_id, 1, True)
    client.data("play_input", key=68, down=False)
    client.data("play_step", frames=3)
    client.data("play_input", key=68, down=True)
    client.data("play_step", frames=3)
    inspect_game(client, player_id, coin_id, 2, False)
    observed = client.data("editor_lua", source=(
        'local player = IApplicationManager.instance():getGameManager():getActorByName("Player"); '
        'return player:getLocalPosition().x'
    ))
    assert observed["value"] == 2, f"Editor Lua observed an unexpected finish position {observed}"
    client.data("play_input", key=68, down=False)
    pointer = {"button": "left", "x": 0.5, "y": 0.5}
    held = client.data("play_pointer", action="press", **pointer)
    assert held["buttons"] == ["left"], held
    poll = "return IApplicationManager.instance():getInputDeviceManager():isMouseButtonDown(0)"
    assert client.data("editor_lua", source=poll)["value"] is True
    client.data("play_pointer", action="release", **pointer)
    assert client.data("editor_lua", source=poll)["value"] is False

    capture = client.call("editor_capture")
    image = next(item for item in capture["content"] if item["type"] == "image")
    assert image["mimeType"] == "image/png", image["mimeType"]
    png = base64.b64decode(image["data"], validate=True)
    assert png.startswith(b"\x89PNG\r\n\x1a\n") and png[12:16] == b"IHDR", "Invalid captured PNG"
    width, height = struct.unpack(">II", png[16:24])
    assert width > 0 and height > 0, (width, height)

    assert not client.data("play_stop")["playing"], "Play mode remained active"
    inspect_game(client, player_id, coin_id, 0, True)
    if unsaved_id:
        assert any(a["id"] == unsaved_id for a in client.data("scene_inspect")["actors"]), "Lost edit on Stop"
    return width, height


def run(editor, workspace):
    with connect(editor, workspace) as client:
        project = client.data("project_create", path="CoinGame")
        client.data("actor_create", name="Camera", kind="camera", position=[1, 0, 8])
        client.data("actor_create", name="Sun", kind="directional_light", rotation=[-35, 20, 0])
        player = client.data("actor_create", name="Player", kind="cube", position=[0, 0, 0])
        coin = client.data("actor_create", name="Coin", kind="cube", position=[2, 0, 0], scale=[0.4, 0.4, 0.4])
        # This game moves between fixed squares. Keep the rendered cubes and
        # remove the physics components provided by the editor's cube preset.
        for actor in (player, coin):
            for component in actor["components"]:
                if component["type"].split("::")[-1] in ("Rigidbody", "CollisionMesh"):
                    client.data("component_remove", id=actor["id"], component=component["id"])
        client.data("script_write", path="CoinGame/Assets/CoinRunner.lua", source=GAME_SCRIPT)
        client.data("script_attach", id=player["id"], path="Assets/CoinRunner.lua", className="CoinRunner")
        client.data("project_save")
        inspect_game(client, player["id"], coin["id"], 0, True)
        unsaved = client.data("actor_create", name="Unsaved edit")["id"]
        size = play(client, player["id"], coin["id"], unsaved)
    with connect(editor, workspace, modern=True) as reopened:
        reopened.data("project_open", path=project["project"])
        assert all(a["id"] != unsaved for a in reopened.data("scene_inspect")["actors"]), "Play saved an unsaved edit"
        inspect_game(reopened, player["id"], coin["id"], 0, True)
        play(reopened, player["id"], coin["id"])
    print(f"Coin game collected on the second key press. Scene restored and replayed after reopening. "
          f"Captured {size[0]} x {size[1]} pixels.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--editor", required=True, type=Path)
    parser.add_argument("--workspace", type=Path, help="Keep the generated CoinGame project in this directory")
    args = parser.parse_args()
    if not args.editor.is_file():
        parser.error("Expected an existing Workphone editor executable")
    if args.workspace:
        run(args.editor.resolve(), args.workspace.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix="workphone-mcp-game-") as directory:
            run(args.editor.resolve(), Path(directory))
