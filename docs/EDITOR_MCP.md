# Create and playtest games over MCP

The Workphone MCP server lets a local AI client drive a dedicated Windows Editor. Projects saved through MCP open in the ordinary editor as `.fbproject` files. The server uses Python 3.9 or newer and its standard library.

## Connect a client

Use `bin/Editor.exe` and `Tools/python/workphone_mcp.py` from the same Windows Editor package. For a source build, run these commands from a Visual Studio 2026 x64 development environment at the repository root.

```powershell
cmake --preset wpgraphics-baseline -DWP_BUILD_TOOLS=ON -DWP_BUILD_EDITOR=ON
cmake --build --preset wpgraphics-baseline --target Editor --config RelWithDebInfo
```

Configure a local **stdio** server in your MCP client. The following example uses the common `mcpServers` JSON format. Substitute absolute paths for your Python executable, checkout, editor build, and the directory where you want to keep games. The Windows Editor needs its matching plugins and Media directory.

```json
{
  "mcpServers": {
    "workphone": {
      "command": "C:/Python313/python.exe",
      "args": [
        "-B",
        "C:/src/Workphone/Tools/python/workphone_mcp.py",
        "--editor", "C:/src/Workphone/bin/windows/v145/x64/MD/RelWithDebInfo/Editor.exe",
        "--workspace", "C:/Games/WorkphoneProjects"
      ]
    }
  }
}
```

The server supports MCP 2026-07-28 and the four earlier revisions from 2024-11-05 through 2025-11-25. On 2026-07-28, call `editor_connect` and pass its `editorId` to editor tools. Earlier clients use their initialized session. All clients can discover tool arguments through `tools/list`. [MCP version negotiation](https://modelcontextprotocol.io/specification/2026-07-28/basic/versioning)

The first engine call launches an Editor instance with private preferences and logs. Use a trusted client and trusted projects. Both gameplay scripts and `editor_lua` run with the local editor account's permissions. The workspace scopes project and script paths, while Lua has the normal editor bindings and standard libraries.

## Try a playable game

Try this prompt in your connected AI client.

> Create a project called CoinGame. Give it a player, a coin two steps to the right, a camera and a light. Write a Lua component that moves the player one step per D press and disables the coin when the player reaches it. Save the project. Play it, press D twice with a release between presses, check the player's position and the coin's state, and show me a frame. Stop and verify that the coin is back.

| Operation | Tools and conventions |
| --- | --- |
| Projects and scenes | `workspace_list`, `project_create`, `project_open`, `project_save`, `scene_create`, `scene_open`. Give project tools workspace-relative paths. Scene paths are relative to the open project. Save current edits before switching. |
| Actors and components | `scene_inspect` returns actor and component UUIDs. Use them with `actor_edit`, `actor_remove`, and the component tools. `actor_create` accepts primitive meshes, cameras, lights, hierarchy and transforms. `component_types` discovers installed types. `component_inspect` returns the fields accepted by `component_edit`. |
| Gameplay code | `script_write` and `script_read` use workspace-relative Lua paths, such as `CoinGame/Assets/CoinRunner.lua`. Pass `Assets/CoinRunner.lua`, the registered class name, and the actor UUID to `script_attach`. Use `project_save` to persist the attachment. Reopen the saved project to pick up edits to a previously loaded class in a fresh Lua VM. |
| Playtesting | `play_start` captures the edit scene. `play_input` sends a key down or up. `play_pointer` moves, presses, releases, or scrolls using normalized coordinates measured from the top left of the captured window. `play_step` advances 1–120 application ticks while held inputs remain down. The game waits between calls. `play_status` reports actual ticks and input availability. `play_stop` restores the captured scene. Virtual input uses the built-in Workphone input backend. |
| Observation and advanced control | `scene_inspect` works during play. `editor_capture` returns a PNG of the editor window from the DX11 back buffer. `editor_lua` executes code against the live Lua engine bindings and returns a JSON-compatible value. For example, `return IApplicationManager.instance():getGameManager():getActorByName("Player"):getLocalPosition().x`. |
| Diagnostics and shutdown | `editor_log` reads recent process diagnostics. `editor_close` closes the owned editor and discards unsaved edits. Disconnecting the client also closes the editor. |

## Run the end-to-end test

From a source checkout, the test creates a disposable coin game, drives it through MCP, checks the win condition and restoration, captures a frame, and replays the saved project in a new editor process.

```powershell
python -B Tests/python/workphone_mcp_game.py --editor bin/windows/v145/x64/MD/RelWithDebInfo/Editor.exe
python -B -m unittest discover -s Tests/python -p test_workphone_mcp_protocol.py
```

To keep the game, add `--workspace C:/Games/WorkphoneProjects` to the first command. Open `CoinGame/project.fbproject` in the Editor and press D twice in Play mode.
