# Editor Renderer Upgrade — Conventions & AAA Rubric

> **All sub-agents MUST read this file before writing any code.** Inconsistent patches will be rejected
> by the critic. The existing 202-file codebase is the source of truth for style.

---

## 1. Language & build

- **C++17** (no C++20 features). Use `std::optional`, `std::variant`, `std::string_view`,
  structured bindings, `if constexpr`, `[[nodiscard]]`, `[[maybe_unused]]`, fold expressions.
- Precompiled header: `EditorPCH.hpp` only pulls `<Workphone/Workphone.hpp>` under
  `WP_USE_PRECOMPILED_HEADERS`. Do **not** add heavy includes to the PCH; include what you use.
- Forward declarations go in `EditorPrerequisites.hpp` for cross-window references; new render
  classes must be forward-declared there by the integration unit (U12), not ad hoc.

## 2. Namespaces & file layout

- Outer namespace: `workphone`. Editor code: `workphone::editor`. Render framework:
  `workphone::editor::render`. Tool windows: `workphone::editor` (existing convention).
- One public class per `.hpp` unless tightly coupled (nested listener classes are inline, as in
  `EditorWindow`, `MaterialWindow`, `SceneWindow`).
- Header guard style seen in tree: `#ifndef __Foo_h__` / `#define __Foo_h__`. New render headers
  may use `#ifndef WP_RENDER_FOO_H` / `#define WP_RENDER_FOO_H` for consistency with newer files
  (`WP_MATERIALWINDOW_H`). Pick one per file and be internally consistent.
- Include order: own header, blank, `EditorPrerequisites.hpp`, blank, `Workphone/Interface/*`,
  blank, std/third-party. Sort roughly most-specific → most-general.

## 3. Pointer & ownership patterns (match existing code exactly)

- Smart pointers: `SmartPtr<T>` (intrusive ref), `AtomicWeakPtr<T>` for back-pointers to owners.
- Raw `T*` only for non-owning, short-lived, single-threaded views.
- Listeners nest as `class XListener : public IEventListener` inside the owner class and hold an
  `AtomicWeakPtr<Owner> m_owner` (see `EditorWindow::ApplicationListener`, `SceneWindow::*`).
- Heavy/async work is a `Job` subclass with `execute()` override and an `AtomicWeakPtr<owner>`
  (see `SceneWindow::BuildTreeJob`, `JobRendererSetup`).
- Class registration: every concrete class ends with `WP_CLASS_REGISTER_DECL;` in the header and
  a matching `WP_CLASS_REGISTER_IMPL(...)` in the cpp (search existing files for the macro form).

## 4. Event handling signature (do not invent a new one)

```cpp
Parameter handleEvent( EventType eventType, hash_type eventValue,
                       const Array<Parameter> &arguments,
                       SmartPtr<ISharedObject> sender,
                       SmartPtr<ISharedObject> object,
                       SmartPtr<IEvent> event ) override;
```

## 5. The Interface contract (read-only, do not extend)

- New render code **calls** `Workphone::Interface::Graphics` / `Interface::Script`. It does **not**
  add to those headers in this pass.
- Key types to use: `render::IRenderer`, `render::IRenderTarget`, `render::IViewport`,
  `render::IGraphicsCamera`, `render::IMaterial`, `render::IGraphicsDeferredShading`,
  `render::IGraphicsScene`, `render::ITexture`, `script::IScriptManager`.
- Resolve the graphics system via `core::IApplicationManager::instance()->getGraphicsSystem()`
  (the pattern in the commented `JobRendererSetup`).
- Colour: `ColourF`. Vectors/matrices: `Vector3F`, `Matrix4F` from `Workphone/Math/*`.

## 6. Porting from Esoterica

- Esoterica is a **reference for architecture and math**, not a copy-paste source. It uses a
  different device API (`RenderDevice`, ` drawing context`) and a different ECS.
- Port: pass structure, algorithm, constants, shader math. Adapt: device access → Interface API;
  component iteration → `IGraphicsScene` query; resource IO → your `Resource` system.
- Always cite the source file in a header comment: `// adapted from Esoterica/Engine/Render/RenderPass_GTAO.cpp`.
- Keep Esoterica's quality; drop its device coupling.

## 7. Lua layer

- Bind through `Interface\Script\IScriptManager.hpp`. Expose: pass graph (add/remove/reorder/enable),
  per-pass parameters, material technique selection, viewport camera/light knobs.
- Scripts live under `Editor\src\scripting\scripts\*.lua`.
- Bindings are thin C++ wrappers over the Interface; no game logic in Lua bindings.

## 8. AAA Rubric (critic scores 0–100; pass ≥ 90)

Each render pass / tool is scored on:

| Criterion | Weight | What "10/10" looks like |
|---|---|---|
| **Pass structure & correctness** | 20 | Clear prepare/execute/cleanup; correct render-target flow; no leaks; matches Interface contract |
| **Feature parity with Esoterica reference** | 20 | All algorithm stages of the source pass ported (not stubbed) |
| **AAA algorithm fidelity** | 20 | Forward+ culling not naive forward; GTAO multi-bounce + bilateral; 4-cascade PCF+contact-hardening; SMAA 1x; PBR IBL (prefiltered cubemap + BRDF LUT); ACES + auto-exposure |
| **Performance shape** | 10 | Half-res where valid, batched draws, no per-frame allocation, no redundant state changes |
| **Material model fidelity** | 10 | Metalness/roughness PBR; technique/pass selection; animated texture nodes; runtime param binding |
| **C++17 idiom & conventions** | 10 | This file's rules followed; no C++20; correct smart-ptr/listener/Job usage; `WP_CLASS_REGISTER_*` present |
| **Lua integration** (passes only) | 5 | Pass params/order enable from Lua without recompile |
| **Integration hygiene** | 5 | No ODR clash; forward-decls in `EditorPrerequisites.hpp`; registered with the manager |

A unit at < 90 gets a concrete change list and loops (max 5 iterations → `blocked-on-budget`).

## 9. Honest reporting rule

The critic must **not** claim a pass is "AAA-quality rendered output" because no GPU is running.
It claims: "implements the AAA algorithm to spec, conforms to the Interface contract, and would
produce AAA output given a correct runtime implementation." Visual claims are about *intended*
look vs. a documented CoD/Anvil reference frame, labelled as such.


## 10. Lua-first rule (rev 3 — hard constraint)

The editor already has a mature, hot-reloadable Lua editor layer at
`Bin\Media\Scripts\Lua\Editor`. Existing Lua editors that MUST NOT be re-implemented in C++:
- `MaterialEditor.lua` (77 KB) — full PBR material editor
- `ShaderEditor.lua` (27 KB) — shader/program/material authoring
- `ParticleEditor.lua`, `TerrainEditor.lua`, `AnimationEditor.lua`, `ProceduralModelEditor.lua`,
  `PackageEditor.lua`

**Rules:**
- **Authoring UI / parameters / anything where hot-reload improves iteration → Lua.** New editors
  follow the existing pattern: `class 'FooEditor' (BaseEditor)`, constructed with a C++ `window`,
  bind via `IApplicationManager.instance()` / `getUI()` / `getScriptManager()`.
- **Native render orchestration or a binding Lua needs → C++17.** Viewport, compositor driver,
  pass classes, and thin Lua bindings only.
- **Extending an existing Lua editor = edit it in place, additively.** Do not rewrite it from
  scratch; do not change its class name or break its existing API.
- **New C++ that duplicates an existing Lua editor is an automatic critic fail.**
- **C++ exposes new capability to Lua through `render_bindings/RenderLuaBindings.cpp`**; Lua editors
  consume the bindings rather than reaching into raw `Interface` headers.

### Lua style (match existing files)
- `class 'Name' (BaseEditor)` at top; `NameTypes = { ... }` id table for widgets.
- `local NameDefaults = { ... }` + a `copyDefaults()` helper for reset state.
- `local function nameSafeCall(obj, method, ...)` / `nameSafeGetText(ctrl, fallback)` guards
  (match `ShaderEditor.lua`).
- `:__init(window)`, `:load()`, `:unload()`, `:update()`, `:show()`, `:hide()`,
  `:getProperties(parameters)`, `:setProperties(parameters)` — same lifecycle as `BaseEditor`.
- Resolve singletons via `IApplicationManager.instance()`; get UI via `:getUI()`; create elements
  via `:addElement(typeInfo)`.
- No globals beyond the `class` and the `Types` table.


## 11. Tooling rubric (rev 4 — Track A units)

The critic scores Track A (procedural model/texture, scene, animation, IK) on the existing §8 rubric
**plus** these tooling-specific criteria (AAA editor bar):

| Criterion | What "10/10" looks like |
|---|---|
| Modelling completeness | ProBuilder-class ops present and correct: extrude/inset/bevel/connect/bridge/subdivide/triangulate/weld/detach + UV unwrap + edge loops + boolean ops; drives `IProceduralMesh`/`IMeshGenerator`; live preview |
| Texturing completeness | Node-graph authoring: noise/splat/erosion/edge-wear/atlases → `IProceduralTexture`; export to runtime material; live preview |
| Scene completeness | Prefab placement, layers, procedural city/world via `IWorldGenerator`/`IProceduralScene`; streaming-aware; live preview |
| Animation parity | Blend 1D+2D, layers, bone masks, root motion, target warp, state machines, parameters — parity with Esoterica `EngineTools\Animation` graph editor |
| IK parity | Two-bone IK + foot IK, pole vectors, weight masks, plant/ground alignment, live preview on selected skeleton — parity with Esoterica `FootIK`/`TwoBoneIK` |
| Live preview | Every tool previews against the scene/character without a full C++ recompile |
| Drives existing runtime | Uses `IProcedural*`/`IGameSceneBuilder`/`AnimationIKSystem`/`TwoBoneIK` — does not invent parallel systems |

Same bar: ≥90 to pass, ≤5 iterations, then `blocked-on-budget`.
