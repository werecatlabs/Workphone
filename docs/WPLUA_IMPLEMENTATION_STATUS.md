# Lua implementation status and usage

Updated 10 October 2026. This records the implemented foundation, not certification of the full [production plan](WPLUA_PRODUCTION_PLAN.md).

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

`WP_BUILD_LUAJIT` defaults to `OFF`. On MSVC x64 with a Visual Studio CMake generator, enabling it adds an isolated upstream LuaJIT build and compatibility checks:

```powershell
cmake -S . -B project_x64 -DWP_BUILD_LUAJIT=ON
cmake --build project_x64 --config RelWithDebInfo --target LuaJitBuild
ctest --test-dir project_x64 -C RelWithDebInfo -R WPLua.luajit_backend_contracts --output-on-failure
```

VM code generation runs against a private source copy for each configuration. The default engine continues to use bundled Lua 5.5. The option currently builds/tests the standalone LuaJIT VM; it does **not** select LuaJIT for `WPLua`, Luabind, CJSON or native plugins. A selectable engine backend still requires a consistent Lua 5.1 API adapter, binding/conversion validation, and separate ABI-compatible build outputs. Do not link Lua 5.5 and LuaJIT into the same host. The legacy `WPLuaJit` directory remains unsupported.

## Visual Studio extension

The VSIX now has an SDK-style project, manifest, compiled Tools menu command and tool-window registration. Build it with the `LuaDebuggerVsix` CMake target when C# tools are enabled, or build `Tools/csharp/LuaDebuggerVsix/LuaDebuggerVsix.csproj` with Visual Studio MSBuild.

The connection client limits UTF-8 newline-delimited JSON frames to 64 KiB, serializes sends, closes malformed/truncated sessions, and isolates readers across reconnects. The WPF log is bounded and the window releases its connection. Protocol serialization uses .NET Framework libraries, avoiding an extra JSON runtime deployment requirement.

This remains a debugger connection tool. The engine debugger backend, DAP integration, normal Visual Studio breakpoint/stack/locals/watch UI, responsive paused Editor execution, step in/over/out, exception stops, request correlation and authenticated attach are still open. Building a VSIX does not verify installation or an interactive debugging session.

## Validation

- `LuaRuntimeTests` uses the production C++ host and bindings, with initialized `TypeManager`. Its SQLite-backed asset checks cover source identity, headless lookup, component property round trips, generated class syntax, dependency installation, incremental compilation, failed/successful reload, packaged loading with source files absent, invalid payloads and module-name collisions.
- `VehicleLuaBindingTests` covers the existing presentation binding contract.
- `Tests/csharp/LuaDebuggerTransportTests` uses the actual transport/protocol sources with loopback sockets, fragmented UTF-8, concurrent sends, repeated reconnects, malformed/oversized frames and disposal.
- `Tests/lua/RuntimeBackendContracts.lua` checks the optional standalone LuaJIT VM's diagnostics, coroutines, collection and JIT toggling.
- `editor_script_window_properties_bridge_test` exercises typed properties through a real Editor fixture. It requires a Debug Editor test build and has not yet been run for this implementation.

Use the production plan's remaining acceptance gates for scene/prefab persistence, Play/Stop, live Editor reload, actual debugger installation/attach and shipped applications. Focused test passes do not certify those interactive flows.
