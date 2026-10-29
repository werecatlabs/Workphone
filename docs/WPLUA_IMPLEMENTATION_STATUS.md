# Lua implementation status and usage

Updated 28 October 2026. This records the implemented foundation, not certification of the full [production plan](WPLUA_PRODUCTION_PLAN.md).

## Script assets and runtime resources

Lua source is cataloged as a `ScriptAsset` with a durable UUID and asset type `script`. Import/reimport only records source metadata; it does not execute the file. Existing catalog entries retain their UUID. `ResourceDatabase::loadResource` resolves script metadata by UUID or path without requiring a renderer. Metadata remains separate from a VM and a live script instance.

Register `workphone::LuaScriptCompiler` in the existing `resource::ResourceCompilerRegistry`, initialize `resource::ResourceSystem` with project source and compiled roots, and call `LuaManager::configureScriptResources(catalog, resources)`. Load an asset with `loadScriptAsset(uuid)`. This configuration is explicit; project startup does not yet automatically construct and configure the compilation service.

The compiler validates syntax without opening libraries or executing source, removes a UTF-8 BOM, preserves shebang line numbers, and produces a versioned resource containing portable source. Individual source payloads are limited to 8 MiB. This is an immutable source payload rather than cross-version Lua bytecode.

Declare installed module dependencies in a neighboring `.lua.deps` manifest:

```text
# Actor.lua.deps
data://logic/Helper.lua
```

`require('logic.Helper')` and `require('logic/Helper')` resolve the installed module. Dependencies are compiled through the existing resource graph and installed into `package.preload`. Ambiguous aliases are rejected. Each load accepts at most 256 Lua resources and 64 MiB of source. Manifest dependencies are explicit: dynamic `require` calls are not statically inferred. Use one canonical spelling of each module name; Lua caches different `require` names separately.

For packaged execution, call `configureScriptResources(catalog, resources, true)` before loading the manager. Compilation and loose-file fallback are disabled; module discovery uses preloads. The catalog and compiled dependency resources must be shipped together. This module policy is not a security sandbox: existing native bindings and standard libraries still have their normal permissions.

Script components persist `scriptAssetUuid` alongside `className`, including deserialization before actor attachment/runtime startup and partial property updates. An asset-backed component loads the asset before constructing its class. Existing components with no asset UUID continue to use already loaded global classes. The Editor still needs a dedicated asset picker and project startup integration for this workflow.

## Runtime and reload

Source execution, global/static/member calls and constructors run through protected Lua boundaries with named diagnostics and stack restoration. Optional missing callbacks remain compatible. Constructors publish instances only after success; a failed replacement preserves the current instance. Queue draining, incremental collection, balanced class enumeration and VM-reference release are implemented.

Reload stages source/assets and constructors in a candidate VM on the Application task, then promotes the candidate after validation. Syntax, dependency or constructor failure keeps the previous VM active. Outstanding legacy raw instances prevent promotion. Editor reload waits through the existing Primary coroutine queue, without sleeping while holding Render/Physics locks, and refreshes windows after completion.

Remaining reload requirements include persisted override/runtime-state migration, generation-checked public handles, native side-effect staging and rollback, and comprehensive owner-thread enforcement. Top-level Lua and constructors can call native services during candidate validation; those side effects cannot currently be rolled back. The binding fork remains constrained to one real execution thread. These are open production gates.

## Optional LuaJIT

`WP_LUA_BACKEND=Lua` remains the default bundled Lua 5.5 engine. `WP_LUA_BACKEND=LuaJIT` now selects the bundled LuaJIT 2.1 engine for WPLua, Luabind, CJSON, C script hosts, resource compilation and the Editor. This configuration supports MSVC x64 with the Visual Studio generator and bundled dependencies. The `luajit-windows` preset enables it in `project_luajit`.

```powershell
cmake --preset luajit-windows
cmake --build project_luajit --config RelWithDebInfo --target Editor LuaRuntimeTests LuaDebuggerTests WorkphoneScriptTests --parallel 2
ctest --test-dir project_luajit -C RelWithDebInfo -R 'WPLua.(runtime_contracts|debugger_integration|luajit_backend_contracts)' --output-on-failure
```

LuaJIT builds use `bin/windows/v145-luajit/x64/MD/RelWithDebInfo` and matching libraries, keeping native plugins separate from the default `v145` engine. The exact toolset segment follows the installed MSVC toolset. Generated VM files remain in a private per-configuration source copy. `WP_BUILD_LUAJIT=ON` with the default backend still builds only the standalone VM. The legacy `WPLuaJit` directory remains unsupported.

The compatibility adapter preserves exact integer conversion and the existing binding API. LuaJIT still has Lua 5.1 language semantics and double precision numbers; Lua 5.3+ syntax, integer precision and standard library differences need project-level compatibility checks. Compiler cache versions differ by backend. Native libraries exporting `bindLibrary` must export a matching `wp_lua_backend_abi` for LuaJIT; default Lua tolerates older untagged plugins. See [the debugging and backend guide](WPLUA_DEBUGGING.md).

## Debugger, Editor and Visual Studio extension

The engine now provides an authenticated, loopback-only Debug Adapter Protocol service with bounded Content-Length frames. It supports breakpoints, conditions, hit counts, logpoints, pause/resume, step in/over/out, stack frames, locals/upvalues/globals, paged tables, read-only watch expressions, loaded sources, print output and protected-boundary exception stops. Lua operations stay on the owning execution thread; the transport thread only handles bytes and messages. Coroutine hooks and stale inspection handles are covered by the host integration fixture.

`WP_LUA_DEBUGGER=1` starts the Editor in its single-executor debugging profile before loading project scripts. LuaJIT compilation is disabled and flushed for this profile. The Editor publishes `.workphone/lua-debug.json` after its pause pump is ready. Its restricted pump keeps messages, input and rendering active while gameplay callbacks and simulation stay suspended. Suspension time is excluded from the engine clock. Stop/close releases the paused VM; VM reload transfers debugger ownership and source identities.

Window > Lua Debugger exposes local breakpoints and conditions/logpoints, exception policy, stack/frame selection, scopes, table expansion, a read-only watch, source context and console output. It persists preferences atomically in `.workphone/lua-breakpoints.json`. Visual Studio owns breakpoint configuration while attached; the Editor restores local preferences after disconnect.

The VSIX packages a .NET Framework 4.8 stdio adapter and registers Workphone Lua with Visual Studio's DebugAdapterHost. The tool window loads an endpoint or launches the Editor, then requests a standard Visual Studio debug session. Breakpoints, Call Stack, Locals, Watch, stepping and Exception Settings use that session. The old newline client is excluded from the extension build. Build/installation/interactive verification are reported separately below; source integration alone is not certification of a live debug session.

## Validation

### Live Editor and Visual Studio, 28 October 2026

A real Lua 5.5 RelWithDebInfo Editor running DX11 in an isolated workspace was attached to the installed adapter in Visual Studio 18 Insiders, using its separate `LuaQA` instance. This exercised an Editor Lua script, not the standalone headless debugger host.

| Live check | Result |
| --- | --- |
| Attach and breakpoint on a physical source file | Pass: stopped in `LuaDebuggerPanel:update` at line 200; Visual Studio opened the physical file |
| Step Into / Step Out / Step Over | Pass: entered `manager` at line 9, returned at 201, advanced to 202 |
| Visual Studio keyboard Step Over | Pass: F10 moved the current statement from 202 to 203, confirmed through accessibility and DTE |
| Locals and Watch | Pass: `now`, `ok`, and `state` appeared with their types; a scalar watch and `state.port > 0 and state.connected` evaluated correctly |
| Table inspection | Pass: expanded the `state` table and inspected its members through Visual Studio's debugger API |
| Read-only evaluation | Pass: `os.exit()` returned an invalid expression without executing it |
| Continue / Pause / Detach | Pass; Visual Studio returned to Design mode, and the Editor's endpoint was removed during clean shutdown |
| Pixel capture and paused Editor rendering | Unverified: Windows capture timed out. Accessibility confirmed the IDE breakpoint/current statement/Locals controls, but does not certify the rendered Editor frame |

The run exposed a real source-identity defect. Folder/archive and native/indexed streams discarded their resolved file metadata, and LuaManager used `getFileName()` (a basename) as the debug source. This prevented breakpoints on physical files from binding and also lost the Editor-directory identity used during paused callback dispatch. Streams now retain their resolved metadata, initial load and reload use that identity, and existing local files are advertised with `sourceReference=0` so Visual Studio uses physical documents. Regression fixtures cover folder/native/indexed loads, reload identity and physical versus virtual DAP sources.

The selected runtime, debugger, Editor panel and package-job CTest suites pass **4/4** on the final default-backend binaries. Evidence: `project_x64/lua-live-final-ctest.log`, `project_x64/lua-vs-live/result.json` and the neighboring `final-*.json` snapshots. The reproducible DTE fixture is [LuaDebuggerVisualStudio.ps1](../Tests/powershell/LuaDebuggerVisualStudio.ps1). It refuses to operate on an IDE without `/RootSuffix LuaQA` and does not log authentication tokens.

The VSIX tool window was opened and its endpoint/Attach controls were confirmed through accessibility. Clicking those form buttons remained blocked by `coordinate input geometry is unavailable`; their handlers were not exercised by UI input. This run did not repeat the LuaJIT matrix, exercise a gameplay Play/Stop cycle, or click the VSIX launch/attach buttons. Attach was issued through the same registered DebugAdapterHost command/configuration used by the plugin; stepping and inspection used the real IDE APIs, with an additional F10 and Locals accessibility check. The previous backend matrix below remains dated evidence.

### Earlier backend matrix, 25 October 2026

Observed on Windows x64, MSVC v145, RelWithDebInfo, 25 October 2026:

| Check | Result |
| --- | --- |
| Default Lua CTest runtime, debugger and Editor panel contracts | 3/3 pass |
| LuaJIT CTest runtime, debugger, Editor panel and standalone backend contracts | 4/4 pass |
| `WorkphoneScriptTests` C host | Pass on both backends |
| Native Editor build | Pass on both backends |
| Editor startup/teardown smoke with `WP_LUA_DEBUGGER=1` | Pass on both backends, with isolated workspace/profile and shared source media; verifies real panel/native widgets, CJSON, preference bridge and endpoint removal |
| Shipped source syntax under LuaJIT | 225 files accepted; syntax-only evidence |
| .NET stdio adapter fixture | Pass: fragmented UTF-8, protocol stdout, EOF and invalid arguments |
| VSIX build/archive | Pass, zero build warnings/errors; DLL, registration and adapter executable present |
| VSIX deployment | Deployed and enabled in the separate `LuaQA` experimental instance; registration imported and IDE started |
| Interactive Visual Studio attach/stepping and paused Editor rendering | Unverified: Windows capture repeatedly failed with `window capture timed out: timed out waiting on channel`; no interactive pass claimed |

CTest evidence is in `lua-final-ctest.log` and `luajit-final-ctest.log` at the repository root. Editor smoke logs are `project_x64/lua-editor-bridge-smoke.log` and `project_luajit/lua-editor-smoke.log`. The latter smoke profile uses the same built binaries and source media; it is not a relocated distribution or clean-machine certification. The installable extension is `project_x64/vsix/bin/LuaDebuggerVsix.vsix`. Follow the [debugging guide](WPLUA_DEBUGGING.md) for attach/launch and inspection limits.

- `LuaRuntimeTests` uses the production C++ host and bindings, with initialized `TypeManager`. Its SQLite-backed asset checks cover source identity, headless lookup, component property round trips, generated class syntax, dependency installation, incremental compilation, failed/successful reload, packaged loading with source files absent, invalid payloads and module-name collisions.
- `VehicleLuaBindingTests` covers the existing presentation binding contract.
- `LuaDebuggerTests` exercises the production LuaManager over fragmented authenticated DAP sessions: conditions/logpoints, stepping, coroutine inspection, read-only watches, integer precision, NaN semantics, table paging, exceptions, disconnect recovery, callback suppression, suspension clock exclusion and local Editor RPC/preferences.
- `Tests/python/LuaDebugAdapterTests.py` exercises the packaged stdio adapter with fragmented UTF-8 frames, stdout integrity, EOF and invalid arguments.
- `Tests/lua/LuaDebuggerPanelContracts.lua` checks Editor control wiring, asynchronous inspection, preference saving and bounded requests using mock widgets. It does not replace a live Editor test.
- `Tests/csharp/LuaDebuggerTransportTests` retains coverage of the old transport; that client is no longer used by the extension.
- `Tests/lua/RuntimeBackendContracts.lua` checks the optional standalone LuaJIT VM's diagnostics, coroutines, collection and JIT toggling.
- `editor_script_window_properties_bridge_test` exercises typed properties through a real Editor fixture. It requires a Debug Editor test build and has not yet been run for this implementation.

Use the production plan's remaining acceptance gates for scene/prefab persistence, Play/Stop, live Editor reload, actual debugger installation/attach and shipped applications. Focused test passes do not certify those interactive flows.
