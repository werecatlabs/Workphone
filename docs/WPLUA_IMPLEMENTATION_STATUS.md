# Lua implementation status and usage

Updated 29 October 2026. This records the implemented foundation, not certification of the full [production plan](WPLUA_PRODUCTION_PLAN.md).

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

`WP_LUA_DEBUGGER=1` starts the Editor in its single-executor debugging profile before loading project scripts. LuaJIT compilation is disabled and flushed for this profile. The Editor publishes `.workphone/lua-debug.json` after its pause pump is ready. Safe Application-task pauses keep native window messages, debugger controls and presentation active while simulation, gameplay input and authoring actions are suspended. Paused frames skip particles and scene/object updates, use zero native delta and frozen foliage time, and hold XAudio2 processing. Engine time stays fixed and excludes suspension on resume. Built-in Editor render callbacks defer to Application after frame retirement, allowing redraw during their breakpoints. Custom synchronous render Lua retains the explicit redraw-unavailable fallback. Stop/close releases the paused VM; VM reload transfers debugger ownership and source identities.

Window > Lua Debugger exposes local breakpoints and conditions/logpoints, exception policy, stack/frame selection, scopes, table expansion, a read-only watch, source context and console output. It persists preferences atomically in `.workphone/lua-breakpoints.json`. Visual Studio owns breakpoint configuration while attached; the Editor restores local preferences after disconnect.

The VSIX packages a .NET Framework 4.8 stdio adapter and registers Workphone Lua with Visual Studio's DebugAdapterHost. The tool window loads an endpoint or launches the Editor, then requests a standard Visual Studio debug session. Breakpoints, Call Stack, Locals, Watch, stepping and Exception Settings use that session. The old newline client is excluded from the extension build. Build/installation/interactive verification are reported separately below; source integration alone is not certification of a live debug session.

## Validation

### Pause isolation and debugger controls, 29 October 2026 follow-up

The pause path now uses a presentation-only Claw frame, a nested timer suspension and XAudio2 engine suspension. Native particles, scene/object callbacks and foliage wind remain fixed during presentation; game UI draws read-only. Native input holds discard task event queues on pause/resume, replace transient snapshots, release the Editor's own mouse capture and block paused gameplay publication, avoiding queued click/key replay. Built-in Editor Render callbacks run on Application after their native frame retires. Paused callback drains reserve space for debugger controls and retain authoring work. Terrain synchronous getters use a cached snapshot in the debug profile. Direct synchronous Lua calls made by custom native render plugins retain the explicit `redrawAvailable=false` guard.

The panel now previews physical sources from the engine's registered snapshot, invalidates stale stop inspection, disables unavailable controls and prioritizes Continue/step over bounded inspection queues. It requests first-use floating placement and a viewport-clamped size instead of the Project side dock; existing ini geometry/docking is preserved. An existing saved narrow dock therefore requires undocking the panel or resetting that layout. The two-row control bar separates Pause/Continue from the step controls. Native ImGui button shortcuts use the same event path as clicks and check focus, modifiers, enablement and key edges. Native contract tests cover clipping, held keys, disabled/hidden controls, focus loss, click dispatch and queue saturation. LuaJIT stepping also filters duplicate caller-line hooks at instruction return boundaries, using public count hooks only during a step. Regression cases cover nested calls with active breakpoints, tail calls into C, Step Into/Out, coroutine execution and per-iteration inspection of ordinary, call-based and tail-C single-line loops. The Win32 ImGui host also consumes SC_KEYMENU, matching the bundled ImGui example, while preserving native close and non-ImGui handling. A standalone hidden-HWND regression passes 10 assertions; bypassing consumption fails at the expected generated F10 menu command without hanging. The full native fixture contains the same regression.

The latest live LuaJIT run uses `project_luajit/lua-debugger-remaining-20261029-step-fixed` and VSIX 0.3.1 in the isolated LuaQA IDE. The VSIX Launch Editor button succeeded through its Alt+U access key, attached automatically and stopped at the physical `LuaDebuggerPanel.lua:240` breakpoint. A read-only `jit.version` watch returned `LuaJIT 2.1.1788856981`. The strict `application-redraw.json` gate passed across 93 actual native update/render/Present frames and 93 native debugger-button render frames, with no new Present failures or occlusion. Engine and fixed time remained exactly constant, both deltas were zero, the executor worker count was zero, simulation was disabled, and native input and audio remained held. Native widget state confirmed the stopped status, source preview, enabled Continue/Step Over and disabled Pause throughout.

The completed step-fixed session also verified these controls against the real Editor and Visual Studio session:

| Live check | Result |
| --- | --- |
| IDE Step Over | Physical Lua source advanced from line 240 to 250 to 251, with pause sequences 1, 2 and 3 |
| Editor F10 | Exactly one native key edge and button activation advanced line 240 to 250 and pause sequence 4 to 5; IDE locals remained valid |
| IDE Step Into / Out / Over | Entered `manager:10` at sequence 7, returned to caller line 240 at sequence 8, then advanced to line 250 at sequence 9 |
| Editor F8 Render-origin callback | Show stack reached `handleEvent:309` at sequence 10 after native frame retirement; the strict `render-origin-redraw.json` gate passed across 205 native update/render/Present frames and 205 native debugger-button render frames |
| VSIX Load endpoint, then Attach | Alt+L and Alt+A reattached and stopped at line 240, sequence 11 |
| Native close while paused | Alt+F4 exited the Editor with code 0, removed its endpoint and returned Visual Studio to Design mode |

The Render-origin gate retained `deferredRenderCallbackActive=true`, frozen engine/fixed clocks, zero deltas, zero executor workers, disabled simulation and held input/audio throughout. The earlier 145-frame evidence remains at `project_luajit/lua-debugger-remaining-20261029/final-render-origin-redraw.json`. Physical Visual Studio F10 remains unverified: the latest injected F10 did not dispatch an IDE step command or advance pause sequence 11, and the separate manual check has no confirmed result. The passing IDE step commands and Editor F10 result do not establish that physical IDE shortcut. These are native widget-state and presentation-counter checks; rendered pixels and pointer hit targets are not certified.

The current release-gate CTest selection passed 10/10 for LuaJIT and 9/9 for standard Lua. Logs are `project_luajit/lua-remaining-20261029-release-gate-ctest.log` and `project_x64/lua-remaining-20261029-release-gate-ctest.log`; they include debugger/runtime/panel, deferred Editor dispatch, native ImGui input/placement, paused Claw frames/game UI and audio adapter checks. Live launch, breakpoint, watch, stepping, reattach, close and strict redraw evidence is in `project_luajit/lua-debugger-remaining-20261029-step-fixed`. The packaged adapter fixture passes against the current executable in `Tools/csharp/LuaDebugAdapter/bin/Release/net48`; the old loose `project_x64/vsix/bin/WorkphoneLuaDebugAdapter.exe` predates this work and is not the adapter installed in VSIX 0.3.1.

A later LuaJIT cleanup fix restores the normal count-hook interval of 1000 on explicit DAP disconnect, even while the client retains its TCP connection. Its regression inspects both the main thread and a coroutine before another Lua line or transport teardown can restore the interval incidentally. Both Editor/DLL configurations rebuilt successfully, and focused debugger integration passed again (LuaJIT 6.00 seconds, standard Lua 4.86 seconds); logs use the `lua-remaining-20261029-disconnect-link` prefix in each build directory. A fresh session on the final linked LuaJIT DLL passed VSIX launch/attach, line 240 to 250 stepping and the strict paused gate across another 68 native/button redraws. Native close exited with code 0, removed the endpoint and returned the IDE to Design mode. The isolated IDE line-ending preference was restored and owned launch credentials were removed. Evidence uses `final-linked-*.json` in the step-fixed workspace.

The broader `WPGraphics.claw_production_render` run reports the CPU skin-array budget and superseded terrain streaming checks (`project_luajit/lua-remaining-20261029-broad-graphics-ctest.log`). Its fixture and timeout were not relaxed for this debugger work. The added focused pause case passes; this is not a pass for the whole graphics suite. Hardware audio cursor behavior and a representative gameplay Play/Stop scene remain separate validation gates.

### Live LuaJIT Editor and Visual Studio, initial 29 October 2026 run before the follow-up

The rebuilt LuaJIT RelWithDebInfo Editor ran with DX11 in an isolated workspace, using the existing junction-based package fixture and matching `v145-luajit` DLLs. The installed VSIX 0.3.1 ran in Visual Studio 18 Insiders with `/RootSuffix LuaQA`. A live read-only watch returned `LuaJIT 2.1.1788856981`. This was the native Editor, with its debugger panel's Application-task callback suspended.

| Live check | Result |
| --- | --- |
| VSIX Launch Editor and attach | Pass through its WPF button's Alt+U access key; started the selected LuaJIT executable with explicit package root/workspace and attached automatically |
| VSIX Load endpoint, then Attach | Pass through Alt+L and Alt+A; loaded the running Editor's generated endpoint and established a second debug session |
| Physical-source breakpoint | Pass at `LuaDebuggerPanel.lua:200`; IDE source and breakpoint/current-statement controls were observed |
| Step Into / Out / Over | Pass through the real IDE debugger API, with increasing native pause sequences: entered `manager:9`, returned to caller line 200, advanced to 201 and 202 |
| LuaJIT same-line hooks | One additional stop occurred on line 201 around its nested calls before advancing; verified as a new pause, not a stalled command |
| Locals, tables and read-only watches | Pass: `now`, `ok`, `state`, expanded table members, `jit.version`, and `state.port > 0 and state.connected`; `os.exit()` was rejected without execution |
| Continue / Pause / Detach | Pass; Pause created a new native pause sequence and Detach returned the IDE to Design mode |
| Paused native redraw | Pass: after deliberately holding the report without delete sharing for 750 ms, two measured intervals each advanced 64 completed pumps, native updates/renders, DX11 frames and Present attempts; no new Present failures or occlusion |
| Native close input while paused | Pass: Alt+F4 cancelled the suspended Lua call, Editor exited with code 0, and its endpoint was removed |
| IDE behavior after Editor close | Pass on the rebuilt adapter: Visual Studio returned to Design mode without an unexpected-adapter-exit modal |

The run fixed two defects found by the live checks. The stdio adapter previously exited on engine EOF without sending a DAP `terminated` event, causing Visual Studio to report an unexpected exit. It now preserves framed engine messages, emits termination once on clean EOF, and retains errors for truncated messages or connection failures. Its expanded Python fixture covers these paths plus fragmented UTF-8 messages and IDE-initiated EOF. Optional paused diagnostics initially disabled after a transient replacement failure; bounded retries now cover Windows sharing, lock and access-denied errors, with no blocking sleep in the pump. The deliberate lock/recovery check passed after this fix.

The VSIX now accepts accessible endpoint/executable path entry, retains blank-field file pickers, and scrolls in small panes. Its launch mnemonic uses Alt+U to avoid the IDE's Edit menu. An opt-in `WP_LUA_DEBUGGER_ACTIVITY_REPORT` file records existing native counters at 4 Hz plus transitions, temporarily enabling scene profiling for redraw-capable pauses and restoring its previous flag afterward. Render-task pauses skip graphics/state/panel reentry and report `renderCallbackSuspended`.

Validation: four selected LuaJIT CTest gates passed (backend, runtime, debugger integration, Editor panel); the native graphics frame contract also passed. The VSIX rebuilt/deployed with zero warnings/errors, and the packaged adapter's Python tests passed. Evidence is in `project_luajit/lua-live-20261029-final-ctest.log`, `lua-live-20261029-frame-ctest.log`, `lua-live-20261029-adapter-tests.log`, `project_x64/vsix/lua-20261029-final-build.log`, and `project_luajit/lua-debugger-live-20261029/accepted-*.json`. The latter includes the final backend, step, variable, frame-progress, exit-code and cleanup snapshots. Generated session credentials were removed during cleanup.

The live scope remains narrower than full production certification. Windows pixel capture failed after a fresh target selection and retry (`window capture timed out: timed out waiting on channel`), and pointer input lacked coordinate geometry. Button handlers were exercised by keyboard access keys; pointer hit targets, rendered pixels and interactive ImGui controls were not certified. F10 injection did not establish a new pause in this run; supported IDE step APIs were used for the stepping pass. The render-callback guard compiled and the native recursion contract passed, but an actual suspended render callback was not exercised. Gameplay Play/Stop and full native simulation freeze remain separate gates. The successful redraw measurements used the fixture's small default viewport, not a representative gameplay scene or performance benchmark.

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
