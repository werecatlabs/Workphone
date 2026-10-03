# Editor AAA Upgrade — Implementation Plan (rev. 4 — tooling-first)

> Owner: Casper
> Scope: `Tools\cpp\Editor\src` (C++17) + `Bin\Media\Scripts\Lua\Editor` (Lua, hot-reloadable)
> Contract: `Engine\cpp\Include\Workphone\Interface` (read-only)
> Runtime backends (unchanged): `WPGraphicsOgreNext`, `WPGraphics`, `WPGraphicsOgre`
> Reference: `G:\Esoterica-main\Esoterica-main\Code`
> Date: 2026-08-19 (rev 4)

---

## 0. What changed in rev 4 (priority flip)

Per your direction, the priority is now **essential editor functionality** — procedural modelling
and texturing, scene creation, the animation system, and the IK system. The renderer AAA pass work
from rev 3 is demoted to a **secondary "nice-to-have" track** that runs after the tooling track (or
in parallel only if it never blocks tooling).

Existing state that shapes the plan (so we extend, not duplicate):
- **Procedural** is already strong: Interface has `IProceduralManager` + city/L-system/road/terrain/
  mesh/texture generators (`IProcedural*`); `ProceduralModelEditor.lua` (110 KB) is already a
  ProBuilder-style tool (extrude/inset/bevel/connect/bridge/subdivide/weld/detach, snap, edit modes).
- **Animation** has a C++ editor: `AnimationGraphWindow` (Esoterica-derived graph with 1D blend
  spaces, selectors, transitions, parameters) + `AnimationWindow` + `CutsceneWindow`; Lua
  `AnimationEditor.lua` (31 KB) handles clip authoring/preview.
- **IK exists in the runtime but is NOT surfaced in the editor.** Interface has `AnimationIKSystem.hpp`,
  `TwoBoneIK.hpp`; Esoterica has the AAA reference: `Animation_RuntimeGraphNode_FootIK`,
  `Animation_Task_TwoBoneIK`, `TwoBoneIK`, ragdoll, target warp, bone masks, blend 2D.
- **Scene creation** has `SceneWindow` (79 KB) + `IGameScene`/`IGameSceneBuilder`/`IGamePrefab`/
  `IProceduralScene`/`IWorldGenerator`.
- Lua-first rule (CONVENTIONS.md §10) still applies: authoring UI goes in Lua unless it needs native
  render/device access; the old C++ material editor is superseded, not extended.

---

## 1. Two tracks

### Track A — Essential tooling (primary, this is what "AAA editor" means now)
A1 Procedural modelling (extend `ProceduralModelEditor.lua` + C++ procedural bindings)
A2 Procedural texturing (new `ProceduralTextureEditor.lua` + `IProceduralTexture` bindings)
A3 Scene creation (new `SceneBuilderEditor.lua` + C++ `IGameSceneBuilder`/prefab bindings)
A4 Animation system (extend `AnimationEditor.lua`/`AnimationGraphWindow` to full blend 2D, layers, bone masks, root motion, target warp)
A5 IK system (new IK editor + bindings; surface `AnimationIKSystem`/`TwoBoneIK`; port FootIK/TwoBoneIK authoring from Esoterica)
A6 Ragdoll / physics-pose bridge (port `PhysicsRagdoll_*` authoring; optional within A5)

### Track B — Renderer (secondary, nice-to-have, never blocks A)
B1..B9 = the rev-3 renderer units (viewport, compositor driver, GTAO/SMAA/ACES/IBL/Forward+/debug/bindings/integrate), unchanged. Started only after Track A's blocking units land, and only in parallel where disjoint.

---

## 2. Current state evidence (tooling focus)

| Area | State | Evidence |
|---|---|---|
| Procedural modelling | Lua tool exists, ProBuilder-style | `ProceduralModelEditor.lua` (110 KB) |
| Procedural texturing | Interface only, no editor | `IProceduralTexture.hpp`; no `ProceduralTextureEditor.lua` |
| Scene creation | Scene tree UI + builder interface | `SceneWindow.*` (79 KB), `IGameSceneBuilder.hpp`, `IProceduralScene.hpp` |
| Animation graph | C++ editor, Esoterica-derived | `AnimationGraphWindow.*`, `AnimationGraph.hpp/.cpp`, `AnimationGraphNodes.hpp` |
| Animation clips | Lua editor | `AnimationEditor.lua` (31 KB) |
| IK | **Runtime yes, editor no** | `AnimationIKSystem.hpp`, `TwoBoneIK.hpp`; no IK editor anywhere |
| Ragdoll | Runtime reference | Esoterica `PhysicsRagdoll_Definition/Instance`, `Animation_Task_Ragdoll` |
| Reference | Strong AAA | Esoterica `Engine\Animation` + `EngineTools\Animation` (FootIK, TwoBoneIK, Blend2D, bone masks, target warp) |

---

## 3. Architecture (target, tooling-first)

```
C++ editor (Tools\cpp\Editor\src)
├─ procedural/                          ← NEW: native procedural + IK bindings
│   ├─ ProceduralBindings.cpp            Bind IProceduralManager/generators/LSystem/road/city to Lua
│   ├─ ProceduralTextureBindings.cpp     Bind IProceduralTexture (noise, splat, erosion, atlases)
│   ├─ IKBindings.cpp                    Bind AnimationIKSystem + TwoBoneIK (chain, pole, weight, mask)
│   └─ RagdollBindings.cpp               Bind PhysicsRagdoll definition authoring (within A5/A6)
├─ scene/
│   ├─ SceneBuilderBindings.cpp          Bind IGameSceneBuilder + IGamePrefab + IProceduralScene
│   └─ (extend SceneWindow.cpp if needed for native scene-graph ops only)
├─ animation/
│   ├─ IKNodeBindings.cpp                Bind IK as animation-graph nodes (FootIK, TwoBoneIK)
│   └─ (extend AnimationGraphWindow.cpp only to register IK node UI; keep authoring in Lua)
└─ (Track B render/ as in rev 3, deferred)

Lua editor (Bin\Media\Scripts\Lua\Editor)
├─ ProceduralModelEditor.lua             (EXTEND: more generators, UV unwrap, edge loop, booleans)
├─ ProceduralTextureEditor.lua           NEW: node-graph texture authoring (noise/splat/erosion/atlas)
├─ SceneBuilderEditor.lua                NEW: procedural scene/city/world builder UI
├─ AnimationEditor.lua                   (EXTEND: blend 2D, layers, bone masks, root motion, target warp)
├─ AnimationGraphEditor.lua              NEW/extend: graph authoring of IK + blend2D + state machines
├─ IKEditor.lua                          NEW: two-bone IK + foot IK + pole-vector + mask authoring + preview
├─ RagdollEditor.lua                     NEW (optional): ragdoll definition authoring
└─ render/default_render_graph.lua       (Track B, deferred)
```

---

## 4. Work breakdown — Track A (disjoint write sets)

| # | Unit | Write set | Builds on | Lang |
|---|---|---|---|---|
| A1 | Procedural modelling extensions | `ProceduralModelEditor.lua` (extend) + `procedural/ProceduralBindings.cpp` | existing Lua + Interface `IProceduralMesh`/`IMeshGenerator`/`ILSystem`; Esoterica `RenderGeometryBuilder` | Lua+C++ |
| A2 | Procedural texturing editor | `ProceduralTextureEditor.lua` (new) + `procedural/ProceduralTextureBindings.cpp` | `IProceduralTexture.hpp`; AAA: noise/splat/erosion/atlas | Lua+C++ |
| A3 | Scene creation editor | `SceneBuilderEditor.lua` (new) + `scene/SceneBuilderBindings.cpp` | `IGameSceneBuilder`/`IGamePrefab`/`IProceduralScene`/`IWorldGenerator`; Esoterica `MapEditor` | Lua+C++ |
| A4 | Animation system extensions | `AnimationEditor.lua` + `AnimationGraphEditor.lua` (extend/new) + `animation/IKNodeBindings.cpp` (blend2D/layers/masks/root-motion/target-warp) | existing + Esoterica `Animation_ToolsGraphNode_Blend2D`, `_Layers`, `_BoneMasks`, `_TargetWarp`, `_Parameters` | Lua+C++ |
| A5 | IK system | `IKEditor.lua` (new) + `procedural/IKBindings.cpp` + `animation/IKNodeBindings.cpp` | `AnimationIKSystem.hpp`, `TwoBoneIK.hpp`; Esoterica `Animation_RuntimeGraphNode_FootIK`, `TwoBoneIK`, `Animation_Task_TwoBoneIK` | Lua+C++ |
| A6 | Ragdoll/physics-pose bridge (optional) | `RagdollEditor.lua` (new) + `procedural/RagdollBindings.cpp` | Esoterica `PhysicsRagdoll_Definition/Instance`, `Animation_Task_Ragdoll` | Lua+C++ |

Track B (rev 3 renderer units B1–B9) is listed in §5, executed only after Track A's blocking units.

**Parallelization:** A1, A2, A3 are independent (disjoint: model / texture / scene) → parallel.
A4 depends on nothing but shares the animation graph; A5 depends on A4's graph-node bindings
pattern → A4 then A5, or A5 after A4's `IKNodeBindings.cpp` contract is fixed. A6 optional after A5.

---

## 5. Track B (renderer, secondary — unchanged from rev 3)

B1 viewport+compositor+passgraph, B2 GTAO, B3 SMAA, B4 ACES, B5 IBL, B6 Forward+, B7 debug,
B8 bindings, B9 integrate. Started after A1–A3 land and only where disjoint from Track A writes.
Track B never blocks Track A.

---

## 6. Critic loop (per unit) — unchanged, plus tooling rubric

Implement → harsh critic (CONVENTIONS.md §8 + new tooling criteria below) → loop if < 90 → cap 5.

Tooling-specific rubric (added):
- **Feature parity with AAA reference** (Esoterica/ProBuilder): modelling ops complete and correct;
  texturing supports node-graph + procedural noise/erosion/splat; scene builder covers
  prefab/layer/streaming; animation has blend2D/layers/bone-masks/root-motion/target-warp;
  IK has two-bone + foot IK + pole vectors + masks + live preview.
- **Lua-first**: authoring in Lua, hot-reloadable, `BaseEditor` pattern; C++ is bindings only.
- **No duplication**: doesn't re-implement existing Lua editors; extends them additively.
- **Drives existing runtime**: uses `IProcedural*`/`IGameSceneBuilder`/`AnimationIKSystem` rather
  than inventing parallel systems.
- **Live preview**: every tool previews against the scene/character without a full recompile.

---

## 7. Execution order (rev 4)

1. ✅ Plan rev 4 + CONVENTIONS.md updated.
2. **A1, A2, A3** in parallel (procedural model, texture, scene — disjoint).
3. **A4** (animation extensions) — can overlap A1–A3 (disjoint animation write set).
4. **A5** (IK) after A4's graph-node binding contract is fixed.
5. **A6** optional after A5.
6. **Track B** (renderer) after A1–A3 land, parallel where disjoint — nice-to-have.
7. Whole-project critic pass; honest report (what's AAA-complete, what's deferred).

---

## 8. "AAA editor" operationalized (rev 4)

- **Procedural modelling**: ProBuilder-class ops (extrude/inset/bevel/connect/bridge/subdivide/
  weld/detach + UV unwrap + edge loops + boolean ops) driving `IProceduralMesh`, live preview.
- **Procedural texturing**: node-graph (noise/splat/erosion/edge-wear/atlases) → `IProceduralTexture`,
  export to runtime material, live preview.
- **Scene creation**: prefab placement, layering, procedural city/world via `IWorldGenerator`/
  `IProceduralScene`, streaming-aware, live preview.
- **Animation**: blend 1D+2D, layers, bone masks, root motion, target warp, state machines,
  parameter authoring — parity with Esoterica's `EngineTools\Animation` graph editor.
- **IK**: two-bone IK + foot IK, pole vectors, weight masks, plant/ground alignment, live preview
  on the selected skeleton; parity with Esoterica `FootIK`/`TwoBoneIK`.

Critic claims "implements AAA tooling to spec, drives existing runtime via Interface + Lua
bindings, hot-reloadable"; does not claim rendered GPU output (no game/GPU running here).

---

## 9. Out of scope (explicit, rev 4)

- Renderer AAA passes are now secondary/deferred (Track B); may not complete this session.
- Re-implementing existing Lua editors in C++ (CONVENTIONS.md §10).
- Modifying backend plugin source or adding `Interface\Graphics` headers.
- Modifying runtime `AnimationIKSystem`/`TwoBoneIK` implementations — we surface and author them,
  not rewrite them (unless a binding genuinely needs a new hook, flagged and deferred).
- A true blind A/B vs a running CoD build.
