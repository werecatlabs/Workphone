"""Exercise Workphone's MCP transport without starting a renderer."""

import io
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import Mock

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "Tools" / "python"))
from workphone_mcp import McpServer, serve


TOOL = {"name": "move", "description": "Apply movement", "inputSchema": {
    "type": "object", "additionalProperties": False, "required": ["axes"],
    "properties": {"axes": {"type": "array", "minItems": 1, "maxItems": 2,
                             "items": {"type": "number", "minimum": -1, "maximum": 1}}}}}


def request(method, params=None, request_id=1):
    message = {"jsonrpc": "2.0", "method": method}
    if request_id is not None:
        message["id"] = request_id
    if params is not None:
        message["params"] = params
    return message


def initialize(version):
    return request("initialize", {"protocolVersion": version, "capabilities": {},
                                  "clientInfo": {"name": "test", "version": "1"}})


class ProtocolTests(unittest.TestCase):
    def setUp(self):
        self.controller = Mock()
        self.controller.call.return_value = {"moved": True}
        self.server = McpServer(self.controller, [TOOL])

    def handshake(self, version="2025-11-25"):
        response = self.server.handle(initialize(version))
        self.assertIsNone(self.server.handle(request("notifications/initialized", request_id=None)))
        return response["result"]

    def test_handshake_negotiation_and_discovery(self):
        self.assertEqual(self.server.handle(request("server/discover"))["error"]["code"], -32602)
        self.assertIn("error", self.server.handle(request("tools/list")))
        self.assertEqual(self.handshake("unknown-version")["protocolVersion"], "2025-11-25")
        self.assertEqual(self.server.handle(request("tools/list", request_id="list"))["result"], {"tools": [TOOL]})
        self.assertIn("error", self.server.handle(initialize("2025-11-25")))

    def test_call_and_tool_execution_error(self):
        self.handshake()
        params = {"name": "move", "arguments": {"axes": [0.5]}}
        result = self.server.handle(request("tools/call", params, request_id="call"))["result"]
        self.controller.call.assert_called_once_with("move", {"axes": [0.5]})
        self.assertEqual(json.loads(result["content"][0]["text"]), {"moved": True})
        self.assertEqual(result["structuredContent"], {"moved": True})
        self.controller.call.side_effect = RuntimeError("Editor closed")
        result = self.server.handle(request("tools/call", params, 2))["result"]
        self.assertTrue(result["isError"])
        self.assertIn("Editor closed", result["content"][0]["text"])

    def test_invalid_and_notification_calls_do_not_mutate_editor(self):
        self.handshake()
        for arguments in ({}, {"axes": []}, {"axes": [True]}, {"axes": [2]}, {"axes": [1], "extra": 1}):
            with self.subTest(arguments=arguments):
                result = self.server.handle(request("tools/call", {"name": "move", "arguments": arguments}))
                self.assertTrue(result["result"]["isError"])
        params = {"name": "move", "arguments": {"axes": [-1]}}
        self.assertIsNone(self.server.handle(request("tools/call", params, request_id=None)))
        self.assertEqual(self.server.handle(request("tools/call", {"name": "typo"}))["error"]["code"], -32602)
        self.assertEqual(self.server.handle(request("tools/call", {"name": "move", "arguments": []}))["error"]["code"], -32602)
        self.assertEqual(self.server.handle(request("tools/call", params, True))["error"]["code"], -32600)
        self.controller.call.assert_not_called()

    def test_march_2025_batch_and_text_result(self):
        self.handshake("2025-03-26")
        batch = [request("ping", request_id=2),
                 request("notifications/cancelled", {"requestId": 1}, request_id=None),
                 request("tools/call", {"name": "move", "arguments": {"axes": [0]}}, 3)]
        ping, call = self.server.handle(batch)
        self.assertEqual(ping, {"jsonrpc": "2.0", "id": 2, "result": {}})
        self.assertEqual(call["id"], 3)
        self.assertNotIn("structuredContent", call["result"])

    def test_capture_returns_image_without_repeating_payload(self):
        self.server = McpServer(self.controller, [{"name": "editor_capture", "inputSchema": {"type": "object"}}])
        self.controller.call.return_value = {"mimeType": "image/png", "data": "AA==", "width": 4, "height": 3}
        self.handshake()
        result = self.server.handle(request("tools/call", {"name": "editor_capture"}))["result"]
        self.assertEqual(result["content"][0], {"type": "image", "mimeType": "image/png", "data": "AA=="})
        self.assertEqual(json.dumps(result).count("AA=="), 1)

    def modern(self, method, params=None, protocol="2026-07-28"):
        meta = {"io.modelcontextprotocol/protocolVersion": protocol,
                "io.modelcontextprotocol/clientCapabilities": {}}
        return self.server.handle(request(method, {**(params or {}), "_meta": meta}))

    def test_modern_discovery_and_version_errors(self):
        info = self.modern("server/discover")["result"]
        self.assertEqual(info["resultType"], "complete")
        self.assertIn("2026-07-28", info["supportedVersions"])
        self.assertIn("2025-11-25", info["supportedVersions"])
        self.assertEqual(info["_meta"]["io.modelcontextprotocol/serverInfo"]["name"], "workphone")
        self.assertEqual(info["capabilities"], {"tools": {}})
        unsupported = self.modern("tools/list", protocol="2099-01-01")["error"]
        self.assertEqual(unsupported["code"], -32022)
        self.assertEqual(unsupported["data"], {"requested": "2099-01-01", "supported": info["supportedVersions"]})
        no_capabilities = {"_meta": {"io.modelcontextprotocol/protocolVersion": "2026-07-28"}}
        self.assertEqual(self.server.handle(request("tools/list", no_capabilities))["error"]["code"], -32602)
        self.assertEqual(self.modern("ping")["error"]["code"], -32601)

    def test_modern_calls_require_explicit_editor_handle(self):
        close = {"name": "editor_close", "inputSchema": {"type": "object", "additionalProperties": False}}
        workspace = {**close, "name": "workspace_list"}
        self.server = McpServer(self.controller, [TOOL, close, workspace])

        def call(name, **arguments):
            return self.modern("tools/call", {"name": name, "arguments": arguments})["result"]

        listing = self.modern("tools/list")["result"]
        self.assertGreaterEqual(listing["ttlMs"], 0)
        self.assertEqual(listing["cacheScope"], "public")
        schemas = {t["name"]: t["inputSchema"] for t in listing["tools"]}
        self.assertIn("editorId", schemas["move"]["required"])
        self.assertNotIn("editorId", schemas["workspace_list"].get("properties", {}))
        self.assertTrue(call("move", axes=[0])["isError"])
        self.assertTrue(call("move", axes=[0], editorId="wrong")["isError"])
        self.controller.call.assert_not_called()
        editor = call("editor_connect")["structuredContent"]["editorId"]
        self.assertEqual(call("editor_connect")["structuredContent"]["editorId"], editor)
        self.assertEqual(call("move", axes=[0], editorId=editor)["resultType"], "complete")
        self.controller.call.assert_called_once_with("move", {"axes": [0]})
        call("workspace_list")
        self.controller.call.assert_called_with("workspace_list", {})
        call("editor_close", editorId=editor)
        self.assertTrue(call("move", axes=[0], editorId=editor)["isError"])
        self.assertNotEqual(call("editor_connect")["structuredContent"]["editorId"], editor)
        self.handshake()
        legacy_tools = self.server.handle(request("tools/list"))["result"]["tools"]
        self.assertEqual(legacy_tools, [TOOL, close, workspace])

    def test_stdio_recovers_after_invalid_frames(self):
        frames = [b"{invalid", b"\xff", b"NaN", b"1e9999",
                  json.dumps(initialize("2024-11-05")).encode(),
                  json.dumps(request("notifications/initialized", request_id=None)).encode(),
                  json.dumps(request("tools/list", request_id="catalog")).encode()]
        sink = io.BytesIO()
        serve(self.controller, [TOOL], io.BytesIO(b"\n".join(frames) + b"\n"), sink)
        replies = [json.loads(line) for line in sink.getvalue().splitlines()]
        self.assertEqual([r["error"]["code"] for r in replies[:4]], [-32700] * 4)
        self.assertEqual(replies[4]["result"]["protocolVersion"], "2024-11-05")
        self.assertEqual(replies[5], {"jsonrpc": "2.0", "id": "catalog", "result": {"tools": [TOOL]}})
        self.assertEqual(len(replies), 6)

    def test_process_catalog_and_workspace_script_round_trip(self):
        server = Path(__file__).resolve().parents[2] / "Tools" / "python" / "workphone_mcp.py"
        script = "-- première ligne\nreturn '第二行'\n"
        messages = [
            initialize("2025-11-25"), request("notifications/initialized", request_id=None),
            request("tools/list", request_id=2),
            request("tools/call", {"name": "script_write", "arguments": {
                "path": "Game/Assets/Play.lua", "source": script}}, 3),
            request("tools/call", {"name": "script_read", "arguments": {"path": "Game/Assets/Play.lua"}}, 4),
            request("tools/call", {"name": "script_read", "arguments": {"path": "../Outside.lua"}}, 5),
        ]
        wire = "".join(json.dumps(m, ensure_ascii=False) + "\n" for m in messages).encode("utf-8")
        # Workspace-only tools do not need to launch the editor.
        with tempfile.TemporaryDirectory() as workspace:
            process = subprocess.run([sys.executable, "-B", str(server), "--workspace", workspace,
                                      "--editor", sys.executable], input=wire, capture_output=True,
                                     check=True, timeout=15)
            self.assertEqual((Path(workspace) / "Game/Assets/Play.lua").read_text(encoding="utf-8"), script)
        initialize_reply, catalog, write, read, outside = map(json.loads, process.stdout.splitlines())
        self.assertEqual(initialize_reply["id"], 1)
        self.assertIn("play_start", {t["name"] for t in catalog["result"]["tools"]})
        self.assertEqual(write["result"]["structuredContent"]["path"], "Game/Assets/Play.lua")
        self.assertEqual(read["result"]["structuredContent"]["source"], script)
        self.assertTrue(outside["result"]["isError"])


if __name__ == "__main__":
    unittest.main()
