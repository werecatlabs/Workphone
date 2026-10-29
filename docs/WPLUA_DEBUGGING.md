# Lua backends and debugging

Updated 29 October 2026. See [implementation status](WPLUA_IMPLEMENTATION_STATUS.md) for the wider production gates.

## Build the engine

Default Lua 5.5 builds are unchanged. For LuaJIT, use the `luajit-windows` CMake configure preset, then build `Editor` and the desired script hosts. Outputs are isolated under the `v145-luajit` toolset folder (the number follows the compiler). Run the runtime and debugger CTest contracts in both build directories. Each host and native plugin must be built for the same backend. Never copy the default Lua DLLs into the LuaJIT output directory.

LuaJIT uses Lua 5.1 source semantics. Scripts that use later Lua syntax or depend on 64-bit Lua integer precision need adaptation. Compiled script resources contain source and use a backend-specific compiler cache version; they are validated by the selected backend. They are not interchangeable Lua bytecode.

A plugin exposing `bindLibrary` can declare its backend ABI using the selected engine headers:

```cpp
#include <WPLuabind/LuaCompat.hpp>
extern "C" __declspec(dllexport) unsigned wp_lua_backend_abi()
{
    return wpLuaBackendAbi;
}
```

LuaJIT requires this export for binding plugins and rejects mismatches. Default Lua also rejects explicitly mismatched tags, while retaining compatibility with untagged legacy plugins. This tag checks the Lua ABI; plugins still need the normal engine/compiler ABI compatibility.

## Start a debug session

1. Build the chosen Editor backend. Start it with `WP_LUA_DEBUGGER=1` in its environment. The development executable uses its normal configured project and media paths. A packaged Editor uses its existing `--workspace` option.
2. Open Window > Lua Debugger for local debugging. Enter a source path and line, optionally a condition, hit rule or log message, then set a breakpoint. Use the frame/scope buttons to inspect a stop; selecting a frame updates source context and the watch. Native panel shortcuts are F5 Continue, F10 Step Over, F11 Step Into, Shift+F11 Step Out and F8 Show stack. They require enabled visible controls and Editor focus. Continue/step have a reserved request slot so inspection cannot starve them. Physical-file source preview uses the registered source snapshot.
3. For Visual Studio, build `Tools/csharp/LuaDebuggerVsix/LuaDebuggerVsix.csproj` with Visual Studio MSBuild. Install the resulting VSIX in Visual Studio 2022/2026 x64 and open Tools > Lua Debugger.
4. Paste the project's `.workphone/lua-debug.json` path and use Load (Alt+L), then Attach (Alt+A). A blank path opens a file picker. Alternatively paste the matching `Editor.exe` path and use Launch Editor (Alt+U); a blank executable path opens its picker. For installed packages, enter the workspace folder first. Launch passes the package root explicitly, creates a unique endpoint file and waits up to 90 seconds.
5. Use Visual Studio's normal Breakpoints, Call Stack, Locals, Watch, stepping and Exception Settings windows. Set breakpoints before entering Play. Stop Debugging disconnects; the Editor resumes suspended Lua.

The session listens only on loopback and requires its random token. Treat the endpoint file as session credentials. `WP_LUA_DEBUG_ENDPOINT` overrides its location, used by the launcher to distinguish simultaneous Editors. The Editor removes only its own endpoint during unload. Local breakpoint preferences live separately in `.workphone/lua-breakpoints.json`, bounded to 64 KiB and published atomically. Visual Studio owns breakpoint configuration while attached; disconnect restores saved local settings.

Debug mode forces one real execution thread because this Luabind fork has global callbacks. Its service starts before scripts/coroutines are created. LuaJIT debugging disables and flushes JIT compilation for reliable line hooks; it remains disabled for the debug VM after disconnect. Restart without `WP_LUA_DEBUGGER` for performance measurements. Existing external Lua states/coroutines created before the service are outside the supported hook lifecycle.

At a safe Application-task stop, the Editor pumps native window messages, debugger controls and presentation frames. The normal application scene/physics/job loop, gameplay input and game UI interaction remain suspended. Native gameplay event queues are discarded on entry and resume; input received during the pause is not replayed after Continue. The paused graphics path does not update particles, scene objects or animation callbacks; native frame delta is zero, foliage wind uses simulation time, and the engine clock stays fixed until continuation. XAudio2 processing is suspended without changing individual voice pause intent. Resume excludes the wall-clock wait and restores the interrupted frame timing. Shutdown cancels a paused script at a protected boundary.

In the debug profile, built-in Editor callbacks originating in Render are queued until the frame retires, then executed on Application. A breakpoint in such a callback can redraw safely; paused drains accept debugger controls and retain authoring actions until resume. Synchronous Terrain getters use cached values in this profile. Keep debugging UI callbacks under the Editor script directory. Custom native plugins that invoke Lua synchronously inside an active render frame must use the same bridge; the fallback guard pumps window messages and reports redraw unavailable for those calls. Independent plugin threads and unsupported audio backends require their own suspension policy. This profile is intended for development, not shipping or performance profiling.

Set `WP_LUA_DEBUGGER_ACTIVITY_REPORT` to an absolute JSON filename for opt-in paused-frame diagnostics. Snapshots publish at most four times per second plus pause/resume transitions, and contain completed pumps, native frame serials and DX11 frame/Present counters. Snapshots also include frozen timer values, native frame delta, simulation/audio suspension flags, deferred callback origin and debugger widget state. Scene profiling is temporarily enabled for redraw-capable pauses and restored on resume. `redrawAvailable=false` with `renderCallbackSuspended` identifies a pause that cannot redraw; it is not a responsiveness pass. No session credentials are written. Use a separate report path per Editor, and share deletion access when reading on Windows so the atomic replacement can complete. These counters establish rendering activity, not pixel correctness.

## Repeat a live IDE check

`Tests/powershell/LuaDebuggerVisualStudio.ps1` uses the supported DTE debugger API against an already running Visual Studio instance started with `/RootSuffix LuaQA`. It requires that exact test suffix, an explicit IDE process ID, and the installed Workphone adapter. It never selects the user's normal IDE instance automatically.

With a debug-enabled Editor running, set a breakpoint using `-Action Breakpoint -Source <physical-file> -Line <line>`, then attach using `-Action Attach -Endpoint <endpoint-json> -WaitForBreak`. `StepInto`, `StepOver`, `StepOut` and `Pause` also accept `-WaitForBreak`, with a 20-second limit. When paused diagnostics are enabled, supply `-ActivityReport <activity-json>` so step waits require a new pause sequence rather than the previous asynchronous IDE Break state. `Inspect -Expression <expression>` records the result and bounded table members. `ShowLocals` opens the IDE's normal Locals window. `Continue`, `ClearBreakpoints` and `Disconnect` complete the run. `-Report <json-file>` saves a snapshot without the session token; the generated launch JSON beside the endpoint does contain credentials and should be removed after testing.

`Tests/powershell/LuaDebuggerPausedActivity.ps1 -ActivityReport <activity-json> -Report <evidence-json>` samples one uninterrupted pause. It requires completed pumps, native update/render serials, DX11 frames and Present attempts to advance, with no new failures or occlusion. Use -RequireFrozenSimulation, -RequireAudioSuspended, -RequireRenderOriginCallback and -RequireEditorDebuggerUI to require the corresponding opt-in fields. The UI gate reads native widget state and source-preview availability; it does not certify pixels. The collector opens snapshots with delete sharing, saves a failed gate before throwing, and does not inspect pixels or send UI input. LuaJIT can emit extra same-line hooks around nested calls: compare pause sequences to distinguish instruction progress from an old stop.

If Visual Studio opens a mixed-line-ending source dialog, choose **No** to preserve the file. A modal dialog blocks DTE automation until dismissed. An inaccessible/busy automation object is reported explicitly by the fixture. `OpenToolWindow` invokes the extension's registered tool-window command, but this fixture does not click its WPF attach/launch buttons.

The 29 October LuaJIT run exercised the VSIX Launch/Load/Attach button handlers through keyboard access keys, physical-file breakpoints, all three step APIs, inspection, Continue/Pause/Detach, paused native redraw counters, report-file sharing recovery and clean Editor/IDE shutdown. The 28 October default-Lua run also verified F10 through UI input. See the [implementation status](WPLUA_IMPLEMENTATION_STATUS.md) for exact evidence and limitations. Pixel-level Editor rendering, pointer hit targets, interactive ImGui controls, gameplay Play/Stop and suspended render-callback redraw remain separate gates. The adapter emits a DAP termination event on clean engine EOF so closing the Editor ends the IDE session normally.

## Inspection semantics and limits

Watches and conditions inspect locals, upvalues and raw globals/table members. Supported expressions include identifiers, literal table indexes, scalar literals, parentheses, `not`, unary minus, string length, arithmetic, comparisons, boolean short circuit and string concatenation. Examples: `actorState.speed`, `values[2]`, `count >= 3 and enabled`. Calls, assignments, metamethods and native property getters are deliberately unavailable; errors are reported to the debugger. Strings used as operands are bounded. Lua 5.5 integer arithmetic retains integer precision; LuaJIT follows its number representation.

Hit rules are `N`, `==N`, `>=N`, `>N` or `%N`, with a positive count. Logpoints interpolate `{expression}`; doubled braces escape a literal brace. Invalid rules are reported as unverified breakpoints. A runtime condition/logpoint error disables that point and reports the diagnostic.

The service exposes one logical Lua executor thread, including its active coroutine. Inspection references expire on continue/step/reload. Tables are paged; expand `[more variables]` to continue. Enumeration visits at most 4096 keys per table; `[inspection limit]` explicitly reports truncation, and a watch can inspect a specific key beyond that budget. Native userdata is displayed without invoking native code. Sources are registered from real stream/catalog paths, limited to 256 entries and 64 MiB total. Large source requests ask the IDE to open the local file. Protocol requests/responses are limited to 64 KiB, with bounded queues and inspection handles. Console output is bounded.

Exception stops occur at the host's protected error handler before stack unwind. Errors caught entirely inside Lua with `pcall` are not a break-on-every-throw facility. There is no native mixed-mode stepping, coroutine scheduler UI, variable mutation or arbitrary evaluation. Breakpoint lines are matched against runtime hooks; a non-executable line does not relocate automatically.

## Verification boundary

`LuaRuntimeTests`, `LuaDebuggerTests`, the C script tests, the Lua panel fixture and stdio adapter tests cover the implemented paths. VSIX compilation and archive inspection establish packaging; they do not establish interactive IDE behavior. Record actual live Editor/Visual Studio results separately in the implementation status. Remaining production-plan gates include scene/prefab asset-picker workflow, broader runtime-state reload migration, native side-effect rollback and shipping validation.

The adapter registration follows Microsoft's [VSDebugAdapterHost](https://github.com/microsoft/VSDebugAdapterHost) integration and the [Debug Adapter Protocol](https://microsoft.github.io/debug-adapter-protocol/overview.html).
