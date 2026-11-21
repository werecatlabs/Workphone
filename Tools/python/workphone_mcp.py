"""Serve one local Workphone editor over the MCP stdio transport."""

import argparse
import json
import math
import sys
import traceback
import uuid
from pathlib import Path


MODERN_VERSION = "2026-07-28"
LEGACY_VERSIONS = ("2025-11-25", "2025-06-18", "2025-03-26", "2024-11-05")
SUPPORTED_VERSIONS = [MODERN_VERSION, *LEGACY_VERSIONS]
MCP_META = "io.modelcontextprotocol/"
SERVER_INFO = {"name": "workphone", "version": "1.0"}
WORKSPACE_TOOLS = {"workspace_list", "script_read", "script_write"}
MAX_MESSAGE_BYTES = 16 * 1024 * 1024


def validate_argument(value, schema, path):
    """Validate the JSON Schema keywords used in Workphone's own tool catalog."""
    kind = schema.get("type")
    types = {"object": dict, "array": list, "boolean": bool, "string": str,
             "integer": int, "number": (int, float), "null": type(None)}
    if kind and (not isinstance(value, types[kind])
                 or (kind in ("number", "integer") and isinstance(value, bool))):
        raise ValueError(f"{path} must be of type {kind}")
    if "enum" in schema and value not in schema["enum"]:
        raise ValueError(f"{path} must be one of {schema['enum']}")
    if isinstance(value, dict):
        for key in schema.get("required", []):
            if key not in value:
                raise ValueError(f"{path}.{key} is required")
        fields = schema.get("properties", {})
        for key, child in value.items():
            rule = fields.get(key, schema.get("additionalProperties", True))
            if rule is False:
                raise ValueError(f"{path}.{key} is not a supported property")
            if isinstance(rule, dict):
                validate_argument(child, rule, f"{path}.{key}")
    elif isinstance(value, list):
        if not schema.get("minItems", 0) <= len(value) <= schema.get("maxItems", math.inf):
            raise ValueError(f"{path} has an invalid number of items")
        if "items" in schema:
            for index, child in enumerate(value):
                validate_argument(child, schema["items"], f"{path}[{index}]")
    elif isinstance(value, str):
        if not schema.get("minLength", 0) <= len(value) <= schema.get("maxLength", math.inf):
            raise ValueError(f"{path} has an invalid length")
    elif type(value) in (int, float):
        if not schema.get("minimum", -math.inf) <= value <= schema.get("maximum", math.inf):
            raise ValueError(f"{path} is outside the permitted numeric range")


def error(request_id, code, message, data=None):
    details = {"code": code, "message": message}
    if data is not None:
        details["data"] = data
    return {"jsonrpc": "2.0", "id": request_id, "error": details}


def editor_catalog(tools):
    catalog = {"editor_connect": {
        "name": "editor_connect",
        "description": "Get the editorId for this server's local editor. Pass it to every editor tool. "
                       "It expires on editor_close, loss of the editor process, or server shutdown.",
        "inputSchema": {"type": "object", "additionalProperties": False},
    }}
    for name, tool in tools.items():
        if name not in WORKSPACE_TOOLS:
            schema = tool["inputSchema"]
            fields = {"editorId": {"type": "string", "description": "Handle returned by editor_connect"},
                      **schema.get("properties", {})}
            tool = {**tool, "inputSchema": {**schema, "properties": fields,
                                           "required": ["editorId", *schema.get("required", [])]}}
        catalog[name] = tool
    return catalog


class McpServer:
    def __init__(self, controller, tools):
        self.controller = controller
        self.tools = {tool["name"]: tool for tool in tools}
        self.modern_tools = editor_catalog(self.tools)
        self.editor_id = None
        self.version = None
        self.ready = False

    def handle(self, message):
        # Only the March 2025 revision uses JSON-RPC batch messages.
        if isinstance(message, list) and self.version == "2025-03-26" and message:
            responses = [self.request(item) for item in message]
            return [response for response in responses if response is not None] or None
        return self.request(message)

    def request(self, message):
        if not isinstance(message, dict) or message.get("jsonrpc") != "2.0":
            return error(None, -32600, "Invalid JSON-RPC request")
        request_id = message.get("id")
        if "id" in message and type(request_id) not in (int, str):
            return error(None, -32600, "Request id must be a string or integer")
        method = message.get("method")
        if not isinstance(method, str):
            return error(request_id, -32600, "Missing or invalid method")
        params = message.get("params", {})
        if "id" not in message:
            if method == "notifications/initialized" and self.version and isinstance(params, dict):
                self.ready = True
            return None
        if not isinstance(params, dict):
            return error(request_id, -32602, "params must be an object")

        meta = params.get("_meta", {})
        if not isinstance(meta, dict):
            return error(request_id, -32602, "_meta must be an object")
        modern = (method == "server/discover" or MCP_META + "protocolVersion" in meta
                  or MCP_META + "clientCapabilities" in meta
                  or (self.version is None and method not in ("initialize", "ping")))
        if modern:
            version = meta.get(MCP_META + "protocolVersion")
            if not isinstance(version, str):
                return error(request_id, -32602, "Missing or invalid _meta protocolVersion")
            if version != MODERN_VERSION:
                return error(request_id, -32022, "Unsupported protocol version",
                             {"supported": SUPPORTED_VERSIONS, "requested": version})
            if not isinstance(meta.get(MCP_META + "clientCapabilities"), dict):
                return error(request_id, -32602, "Missing or invalid _meta clientCapabilities")
            client = meta.get(MCP_META + "clientInfo")
            if MCP_META + "clientInfo" in meta and (
                    not isinstance(client, dict) or not isinstance(client.get("name"), str)
                    or not isinstance(client.get("version"), str)):
                return error(request_id, -32602, "Invalid _meta clientInfo")

        catalog = self.modern_tools if modern else self.tools
        if method == "server/discover":
            result = {"supportedVersions": SUPPORTED_VERSIONS, "capabilities": {"tools": {}},
                      "ttlMs": 60000, "cacheScope": "public"}
        elif method == "ping" and not modern:
            result = {}
        elif method == "initialize" and not modern:
            if self.version:
                return error(request_id, -32600, "Session already initialized")
            version = params.get("protocolVersion")
            client = params.get("clientInfo")
            if (not isinstance(version, str) or not isinstance(params.get("capabilities"), dict)
                    or not isinstance(client, dict) or not isinstance(client.get("name"), str)
                    or not isinstance(client.get("version"), str)):
                return error(request_id, -32602, "Expected protocolVersion, capabilities and clientInfo")
            self.version = version if version in LEGACY_VERSIONS else LEGACY_VERSIONS[0]
            result = {"protocolVersion": self.version, "capabilities": {"tools": {}},
                      "serverInfo": SERVER_INFO}
        elif method not in ("tools/list", "tools/call"):
            return error(request_id, -32601, "Method not found")
        elif not modern and not self.ready:
            return error(request_id, -32600, "Initialize the MCP session before using tools")
        elif method == "tools/list":
            if "cursor" in params:
                return error(request_id, -32602, "Unknown tool list cursor")
            result = {"tools": list(catalog.values())}
            if modern:
                result.update(ttlMs=60000, cacheScope="public")
        else:
            name = params.get("name")
            if not isinstance(name, str) or name not in catalog:
                return error(request_id, -32602, "Unknown tool name")
            arguments = params.get("arguments", {})
            if not isinstance(arguments, dict):
                return error(request_id, -32602, "arguments must be an object")
            try:
                validate_argument(arguments, catalog[name]["inputSchema"], "arguments")
                value = self.call(name, arguments, modern)
                if name == "editor_capture":
                    result = {"content": [{"type": "image", "mimeType": value["mimeType"], "data": value["data"]},
                                          {"type": "text", "text": f'{value["width"]} × {value["height"]} pixels'}]}
                else:
                    result = {"content": [{"type": "text", "text": json.dumps(value, allow_nan=False)}]}
                    if modern or (self.version >= "2025-06-18" and isinstance(value, dict)):
                        result["structuredContent"] = value
            except (ValueError, RuntimeError, OSError) as exc:
                result = {"isError": True, "content": [{"type": "text", "text": str(exc)}]}
            except Exception:
                traceback.print_exc(file=sys.stderr)
                return error(request_id, -32603, "Internal editor control error")
        if modern:
            result.update(resultType="complete", _meta={MCP_META + "serverInfo": SERVER_INFO})
        return {"jsonrpc": "2.0", "id": request_id, "result": result}

    def call(self, name, arguments, modern):
        if name == "editor_connect":
            if self.editor_id is None:
                self.editor_id = str(uuid.uuid4())
            return {"editorId": self.editor_id}
        if modern and name not in WORKSPACE_TOOLS:
            arguments = dict(arguments)
            if arguments.pop("editorId") != self.editor_id:
                raise ValueError("Unknown or expired editorId. Call editor_connect to obtain the current handle.")
        previous_process = self.controller.process
        try:
            result = self.controller.call(name, arguments)
        finally:
            if name == "editor_close" or (previous_process is not None and self.controller.process is None):
                self.editor_id = None
        return result


def finite_float(text):
    value = float(text)
    if not math.isfinite(value):
        raise ValueError("Non-finite number")
    return value


def serve(controller, tools, source, sink):
    server = McpServer(controller, tools)
    while line := source.readline(MAX_MESSAGE_BYTES + 1):
        if len(line) > MAX_MESSAGE_BYTES:
            while line and not line.endswith(b"\n"):
                line = source.readline(MAX_MESSAGE_BYTES + 1)
            response = error(None, -32600, "Message exceeds the 16 MiB stdio limit")
        else:
            try:
                message = json.loads(line.decode("utf-8"), parse_float=finite_float, parse_constant=finite_float)
            except (ValueError, RecursionError):
                response = error(None, -32700, "Invalid UTF-8 JSON message")
            else:
                response = server.handle(message)
        if response is not None:
            sink.write(json.dumps(response, separators=(",", ":"), allow_nan=False).encode("utf-8") + b"\n")
            sink.flush()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--editor", type=Path, required=True, help="Workphone editor executable")
    parser.add_argument("--workspace", type=Path, required=True, help="Directory for Workphone game projects")
    args = parser.parse_args()
    from workphone_control import Controller, TOOLS

    try:
        controller = Controller(args.editor.resolve(), args.workspace.resolve())
    except (ValueError, RuntimeError, OSError) as exc:
        parser.error(str(exc))
    try:
        serve(controller, TOOLS, sys.stdin.buffer, sys.stdout.buffer)
    except (BrokenPipeError, KeyboardInterrupt):
        pass
    finally:
        controller.close()


if __name__ == "__main__":
    main()
