# Lua backends and debugging

Updated 28 October 2026. See [implementation status](WPLUA_IMPLEMENTATION_STATUS.md) for the wider production gates.

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
2. Open Window > Lua Debugger for local debugging. Enter a source path and line, optionally a condition, hit rule or log message, then set a breakpoint. Use the frame/scope buttons to inspect a stop; selecting a frame updates source context and the watch.
3. For Visual Studio, build `Tools/csharp/LuaDebuggerVsix/LuaDebuggerVsix.csproj` with Visual Studio MSBuild. Install the resulting VSIX in Visual Studio 2022/2026 x64 and open Tools > Lua Debugger.
4. Load the project's `.workphone/lua-debug.json`, then Attach. Alternatively use Launch Editor and choose the matching Editor executable. For installed packages, enter the workspace folder first. Launch creates a unique endpoint file and waits up to 90 seconds.
5. Use Visual Studio's normal Breakpoints, Call Stack, Locals, Watch, stepping and Exception Settings windows. Set breakpoints before entering Play. Stop Debugging disconnects; the Editor resumes suspended Lua.

The session listens only on loopback and requires its random token. Treat the endpoint file as session credentials. `WP_LUA_DEBUG_ENDPOINT` overrides its location, used by the launcher to distinguish simultaneous Editors. The Editor removes only its own endpoint during unload. Local breakpoint preferences live separately in `.workphone/lua-breakpoints.json`, bounded to 64 KiB and published atomically. Visual Studio owns breakpoint configuration while attached; disconnect restores saved local settings.

Debug mode forces one real execution thread because this Luabind fork has global callbacks. Its service starts before scripts/coroutines are created. LuaJIT debugging disables and flushes JIT compilation for reliable line hooks; it remains disabled for the debug VM after disconnect. Restart without `WP_LUA_DEBUGGER` for performance measurements. Existing external Lua states/coroutines created before the service are outside the supported hook lifecycle.

At a stop, the Editor pumps messages, input, debugger UI and rendering without advancing scene/physics/jobs or invoking gameplay Lua listeners. The clock excludes the wait. Keep debugging UI callbacks under the Editor script directory. Shutdown cancels a paused script at a protected boundary. This profile is intended for development, not shipping or performance profiling.

## Repeat a live IDE check

`Tests/powershell/LuaDebuggerVisualStudio.ps1` uses the supported DTE debugger API against an already running Visual Studio instance started with `/RootSuffix LuaQA`. It requires that exact test suffix, an explicit IDE process ID, and the installed Workphone adapter. It never selects the user's normal IDE instance automatically.

With a debug-enabled Editor running, set a breakpoint using `-Action Breakpoint -Source <physical-file> -Line <line>`, then attach using `-Action Attach -Endpoint <endpoint-json> -WaitForBreak`. `StepInto`, `StepOver`, `StepOut` and `Pause` also accept `-WaitForBreak`, with a 20-second limit. `Inspect -Expression <expression>` records the result and bounded table members. `ShowLocals` opens the IDE's normal Locals window. `Continue`, `ClearBreakpoints` and `Disconnect` complete the run. `-Report <json-file>` saves a snapshot without the session token; the generated launch JSON beside the endpoint does contain credentials and should be removed after testing.

If Visual Studio opens a mixed-line-ending source dialog, choose **No** to preserve the file. A modal dialog blocks DTE automation until dismissed. An inaccessible/busy automation object is reported explicitly by the fixture. `OpenToolWindow` invokes the extension's registered tool-window command, but this fixture does not click its WPF attach/launch buttons.

The 28 October live run verified a physical-file breakpoint, all three step commands, F10 through the real IDE UI, Locals, table inspection, watch expressions, rejection of function evaluation, Continue/Pause and Detach against the native Editor. See the [implementation status](WPLUA_IMPLEMENTATION_STATUS.md) for exact scope and evidence. Pixel-level Editor rendering, the VSIX form buttons (pointer geometry was unavailable), gameplay Play/Stop and this run's LuaJIT matrix remain separate gates.

## Inspection semantics and limits

Watches and conditions inspect locals, upvalues and raw globals/table members. Supported expressions include identifiers, literal table indexes, scalar literals, parentheses, `not`, unary minus, string length, arithmetic, comparisons, boolean short circuit and string concatenation. Examples: `actorState.speed`, `values[2]`, `count >= 3 and enabled`. Calls, assignments, metamethods and native property getters are deliberately unavailable; errors are reported to the debugger. Strings used as operands are bounded. Lua 5.5 integer arithmetic retains integer precision; LuaJIT follows its number representation.

Hit rules are `N`, `==N`, `>=N`, `>N` or `%N`, with a positive count. Logpoints interpolate `{expression}`; doubled braces escape a literal brace. Invalid rules are reported as unverified breakpoints. A runtime condition/logpoint error disables that point and reports the diagnostic.

The service exposes one logical Lua executor thread, including its active coroutine. Inspection references expire on continue/step/reload. Tables are paged; expand `[more variables]` to continue. Enumeration visits at most 4096 keys per table; `[inspection limit]` explicitly reports truncation, and a watch can inspect a specific key beyond that budget. Native userdata is displayed without invoking native code. Sources are registered from real stream/catalog paths, limited to 256 entries and 64 MiB total. Large source requests ask the IDE to open the local file. Protocol requests/responses are limited to 64 KiB, with bounded queues and inspection handles. Console output is bounded.

Exception stops occur at the host's protected error handler before stack unwind. Errors caught entirely inside Lua with `pcall` are not a break-on-every-throw facility. There is no native mixed-mode stepping, coroutine scheduler UI, variable mutation or arbitrary evaluation. Breakpoint lines are matched against runtime hooks; a non-executable line does not relocate automatically.

## Verification boundary

`LuaRuntimeTests`, `LuaDebuggerTests`, the C script tests, the Lua panel fixture and stdio adapter tests cover the implemented paths. VSIX compilation and archive inspection establish packaging; they do not establish interactive IDE behavior. Record actual live Editor/Visual Studio results separately in the implementation status. Remaining production-plan gates include scene/prefab asset-picker workflow, broader runtime-state reload migration, native side-effect rollback and shipping validation.

The adapter registration follows Microsoft's [VSDebugAdapterHost](https://github.com/microsoft/VSDebugAdapterHost) integration and the [Debug Adapter Protocol](https://microsoft.github.io/debug-adapter-protocol/overview.html).
