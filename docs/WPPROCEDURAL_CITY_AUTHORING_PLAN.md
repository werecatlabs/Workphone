# Editable city roads and terrain placement

Status, 24 October 2026: implementation design plus a bounded CPU authoring foundation. The rendered city currently uses a flat road/physics plane and grid or seeded L-system topology. There is no verified city road drawing tool, terrain-conforming city scene, bridge system, or atomic city/terrain replacement. See [the production plan](WPPROCEDURAL_PRODUCTION_PLAN.md) and [implementation evidence](WPPROCEDURAL_IMPLEMENTATION_STATUS.md).

The intended workflow is to generate a city, draw and edit road splines in the existing Editor viewport, preserve those edits through regeneration and save/reopen, and fit roads, junctions, building pads and collision to the selected terrain. A generated graph and its sampled meshes are derived caches. They must not become competing editable documents. The connected [river, road/rail bridge and train-track design](WPPROCEDURAL_CITY_TRANSPORT_WATER_PLAN.md) extends this same source and terrain publication model.

## Existing paths to extend

| Concern | Actual source and current limit |
|---|---|
| Scene viewport input | [ImGuiApplication.cpp](../Engine/cpp/Source/WPImGui/ImGuiApplication.cpp) computes a world ray from the exact scene image rectangle. Its `terrainViewportBrush` event carries hover/begin/update/commit/cancel and ray origin/direction. Tool replies disable ImGuizmo and ordinary picking. This is an existing terrain hook, not a city tool registry. |
| Editor listener lifecycle | [TerrainWindow.cpp](../Tools/cpp/Editor/src/ui/TerrainWindow.cpp) attaches to the scene render window, cancels on selection/hide/unload, and forwards to the terrain editor. Reuse that ownership pattern. `SceneWindow` is the hierarchy, and `FBGameEditorWx` is not the active ImGui path. |
| Editor windows | [UIManager.cpp](../Tools/cpp/Editor/src/ui/UIManager.cpp) creates native and script windows. A proposed city authoring window can follow `TerrainWindow`/`ScriptWindow`; no such city window is claimed here. |
| Commands and scene persistence | [Command.hpp](../Tools/cpp/Editor/src/commands/Command.hpp), [ModifyPropertyCmd.cpp](../Tools/cpp/Editor/src/commands/ModifyPropertyCmd.cpp) and `ICommandManager` provide primary-task commands, project-session checking and dirty marking. Base `Command` undo/redo are empty; the road edit command needs concrete implementations. UIManager's save path calls `scene->saveScene()`. |
| Project assets | [AssetDatabaseManager.hpp](../Engine/cpp/Include/Workphone/Database/AssetDatabaseManager.hpp) provides UUID lookup, catalog generations, current-entry checks and journaled file operations. Register the city source through existing project asset identity; do not invent a second catalog or manually rewrite sidecar identities. |
| Road generation | [ProceduralRoad.cpp](../Engine/cpp/Source/Workphone/Scene/Components/ProceduralRoad.cpp) currently exposes Start/End and generates a segment through `IRoadSystem`. [CRoadMeshElement.cpp](../Engine/cpp/Source/WPProcedural/CRoadMeshElement.cpp) sweeps a supplied polyline into left/right vertex pairs. Its `Points` and endpoint tangents are not an editable spline model. |
| Curve math | [LinearSpline3.hpp](../Engine/cpp/Include/Workphone/Math/LinearSpline3.hpp) supplies existing Hermite/Catmull–Rom interpolation. Explicit authoring handles are cubic Bezier controls; bounded De Casteljau sampling is a small compatible CPU helper, not a replacement scene lifecycle. |
| Terrain queries | [TerrainData.hpp](../Engine/cpp/Include/Workphone/Graphics/TerrainData.hpp) defines immutable snapshots, `sampleTerrainHeight` and `intersectTerrain`, with the same triangle diagonal as render/collision. Holes and outside-footprint queries fail. Terrain placement validation supports translation, positive scale and normalized yaw; do not silently accept unsupported tilt. |
| Terrain edits/publication | [TerrainSystem.hpp](../Engine/cpp/Include/Workphone/Scene/Components/Terrain/TerrainSystem.hpp) exposes expected-revision terrain updates and begin/update/commit/cancel edit strokes. [Terrain.hpp](../Engine/cpp/Include/Workphone/Graphics/Terrain.hpp) separates requested/active snapshots and exposes a publication gate for prepared render/physics consumers. Reuse those contracts. |
| City replacement | [ProceduralRaceScene.cpp](../Engine/cpp/Source/Workphone/Scene/Components/ProceduralRaceScene.cpp) currently stops render/physics and clears generated assets before rebuilding. It does not yet retain the previous city on a subsequent generation failure. That must change before interactive regeneration can claim last-good publication. |

## One editable source and stable identity

Use one project-owned city road source asset, proposed suffix `.cityroads.json`, referenced by resource UUID from the scene. The suffix/importer registration is proposed, not an already supported asset type. A scene instance stores its city-to-world placement and terrain instance reference; it does not serialize another editable copy of the generated road mesh. Editing the racing vehicle transform must not move the city source's coordinate frame.

Each source asset, road spline, control point, junction and pinned route has a canonical lowercase GUID. Editing a point, width or label preserves its ID. Splitting a road retains the original ID for one piece and creates a new ID for the other; the junction gets its own ID. Joining endpoints changes explicit references, not an unstable nearest-position lookup. Duplicate IDs and unresolved references fail validation.

Roundabouts are typed junctions in this same source, with stable spline approach ports and directed ring movements. The [roundabout design and T0–T5 gates](WPPROCEDURAL_CITY_TRANSPORT_WATER_PLAN.md#roundabouts-and-junction-authoring) cover island-avoiding routes, terrain/collision fitting and explicit country/driving-side profiles; a decorative ring mesh does not implement those contracts.

Seeded children derive identity from source namespace, generator revision and production lineage, rather than a traversal index or floating-point position. Promoting a generated road to an authored/locked road stores its full controls and provenance in the same source. Generator passes honor locks and deletion tombstones before growing new children. Regeneration may replace unlocked generated caches; it must not recreate a tombstoned road or erase an authored spline.

The CPU foundation uses a strict **version 1** subset: source GUID/revision, terrain resource GUID/observed revision, spline/control-point GUIDs, Bezier positions/relative handles, width, closure, height mode, height offset and grade limit. It does not yet implement junction editing, generator overrides, tombstones, pinned routes, file I/O or scene publication. Unsupported versions/fields must fail instead of being discarded on save.

[The hand-editable version 1 example](examples/city-road-source-v1.cityroads.json) is accepted by this codec. Its terrain UUID is example identity, not an included or resolved terrain asset; integration must replace/bind it to the selected project terrain. `CityRoadSplineSourceTests` accepts the example file path as its first argument to validate loading and save/reopen without an Editor session.

## Proposed complete source schema

This **version 2 target example is design data**, not a file supported by the version 1 foundation. It shows how later authoring features extend the same canonical source. Numeric distances are metres; grades are rise/run, not degrees. Handles are vectors relative to their control point. Terrain placement is explicit; observed runtime revision is diagnostic and must be refreshed after reopening, never reused as a durable concurrency token.

```json
{
  "schema": "workphone.city-road-source",
  "version": 2,
  "id": "c4ea2578-70f4-4d82-8e98-207d4f5c9a10",
  "revision": 12,
  "units": "metres",
  "up": "+Y",
  "space": "city-local",
  "terrain": {
    "resourceUuid": "24d53b66-8bbf-44db-b7ce-588e742e60e0",
    "instanceUuid": "89753f3f-ea5f-4a2e-84dc-4db675ed58d2",
    "sourceHash": "sha256:replace-with-current-terrain-content-hash",
    "observedRevision": 44,
    "worldTransform": {
      "position": [100, 0, 50],
      "rotation": {"w": 0.991444861, "x": 0, "y": 0.130526192, "z": 0},
      "scale": [1, 1, 1]
    }
  },
  "generator": {
    "kind": "lsystem",
    "revision": 1,
    "seed": 42,
    "namespace": "c4ea2578-70f4-4d82-8e98-207d4f5c9a10",
    "lockedGeneratedRoadIds": ["e64e6934-0bf2-4f02-b85b-c4418d447e32"]
  },
  "junctions": [
    {"id": "d2b01b6c-41c0-4434-abcc-7f88eb42d112", "position": [0, 0, 0]},
    {"id": "0aa34e24-2bd9-4ce3-91d0-41d233e36e22", "position": [0, 0, -80]},
    {"id": "103f849c-8fa8-4dc1-bdfe-453b37638333", "position": [60, 4, -80]},
    {"id": "1c141bc7-2292-4d53-b32c-39f4d4044444", "position": [120, 4, -80]}
  ],
  "roads": [
    {
      "id": "f45db713-f086-4162-9ef6-92567c521001",
      "origin": "authored",
      "locked": true,
      "roadClass": "Arterial",
      "width": 12,
      "closed": false,
      "heightMode": "Drape",
      "heightOffset": 0.012,
      "maxGrade": 0.08,
      "maxCrossSlope": 0.025,
      "startJunctionId": "d2b01b6c-41c0-4434-abcc-7f88eb42d112",
      "endJunctionId": "0aa34e24-2bd9-4ce3-91d0-41d233e36e22",
      "knots": [
        {"id": "8d15fd63-e5b8-4d63-b3a9-6b99ddba1001", "position": [0, 0, 0], "inHandle": [0, 0, 0], "outHandle": [0, 0, -25]},
        {"id": "8d15fd63-e5b8-4d63-b3a9-6b99ddba1002", "position": [0, 0, -80], "inHandle": [-8, 0, 25], "outHandle": [0, 0, 0]}
      ]
    },
    {
      "id": "e64e6934-0bf2-4f02-b85b-c4418d447e32",
      "origin": "locked-generated",
      "generatedFrom": {"seed": 42, "productionPath": "root/arterial-0/child-2", "generatorRevision": 1},
      "locked": true,
      "roadClass": "Residential",
      "width": 8,
      "closed": false,
      "heightMode": "Fixed",
      "heightOffset": 0,
      "maxGrade": 0.08,
      "maxCrossSlope": 0.025,
      "startJunctionId": "0aa34e24-2bd9-4ce3-91d0-41d233e36e22",
      "endJunctionId": "103f849c-8fa8-4dc1-bdfe-453b37638333",
      "knots": [
        {"id": "8d15fd63-e5b8-4d63-b3a9-6b99ddba2001", "position": [0, 0, -80], "inHandle": [0, 0, 0], "outHandle": [20, 1.3, 0]},
        {"id": "8d15fd63-e5b8-4d63-b3a9-6b99ddba2002", "position": [60, 4, -80], "inHandle": [-20, -1.3, 0], "outHandle": [0, 0, 0]}
      ]
    },
    {
      "id": "ad100792-633d-4d9d-9727-3c8c7fc73003",
      "origin": "authored",
      "locked": true,
      "roadClass": "Residential",
      "width": 8,
      "closed": false,
      "heightMode": "Bridge",
      "heightOffset": 0,
      "maxGrade": 0.06,
      "maxCrossSlope": 0.025,
      "startJunctionId": "103f849c-8fa8-4dc1-bdfe-453b37638333",
      "endJunctionId": "1c141bc7-2292-4d53-b32c-39f4d4044444",
      "knots": [
        {"id": "8d15fd63-e5b8-4d63-b3a9-6b99ddba3001", "position": [60, 4, -80], "inHandle": [0, 0, 0], "outHandle": [20, 0, 0]},
        {"id": "8d15fd63-e5b8-4d63-b3a9-6b99ddba3002", "position": [120, 4, -80], "inHandle": [-20, 0, 0], "outHandle": [0, 0, 0]}
      ],
      "bridge": {"minimumClearance": 3, "supports": "explicit-abutments-required"}
    }
  ],
  "routes": [
    {"id": "fe8c352a-d433-47fd-abba-382894ba9001", "mode": "Pinned", "closed": false,
     "orderedRoads": [{"id": "f45db713-f086-4162-9ef6-92567c521001", "reverse": false}, {"id": "e64e6934-0bf2-4f02-b85b-c4418d447e32", "reverse": false}]},
    {"id": "fe8c352a-d433-47fd-abba-382894ba9002", "mode": "Auto", "seed": 73}
  ],
  "tombstones": {
    "roadIds": ["1d5659f1-fdac-42a4-804b-56cdb4de0001"],
    "junctionIds": [],
    "controlPointIds": []
  }
}
```

A Fixed/Drape connection needs a solved common junction height; the example's authored heights are intent, not proof that an arbitrary terrain permits that grade. A pinned route with a missing or disconnected road becomes invalid with a visible diagnostic; it must not silently switch to an automatic route. Route identity/signature must change lap-record scope after topology edits without overwriting the existing grid/seed record files.

## Terrain fit and collision contract

1. Resolve the selected terrain resource and instance, retain its immutable snapshot and validated placement, and convert city-local samples through the scene instance transform. Pick using `intersectTerrain`; never substitute a generic Y=0 plane after a terrain miss.
2. Flatten each Bezier span with bounded geometric tolerance, maximum segment length, recursion/work limit and total sample budget. Preserve original controls/IDs; flattened points and cross-sections are disposable outputs.
3. **Drape** samples terrain height at the centre and road edges, adds the authored surface offset and checks grade/crossfall. The first CPU tier rejects a steep, missing, hole or nonfinite sample; it does not claim terrain grading by merely clamping Y. **Fixed** retains the authored 3D profile and validates grade. It may require later supports/cut-fill work. **Bridge** is explicitly unavailable until deck, abutments, clearance and collision are implemented; do not treat a floating Fixed ribbon as a completed bridge.
4. A future optional cut/fill solver prepares a separate terrain candidate with bounded excavation/fill and feathered shoulders. Keep the underlying terrain source intact or store a reversible grading layer. Junction approaches solve one compatible elevation/crossfall; normal smoothing cannot fix a physical step.
5. Building pads need the entire footprint's heights, slope and cut/fill bounds. A centre-height lookup alone can leave floating corners or buried entrances. Derive one pad plane and retaining/foundation policy, or reject the parcel. Road shoulders, sidewalks, entries and pad transitions must meet that same solved surface.
6. Rendering, collision, picking, surface queries and foliage exclusions consume the same fitted revision. Replace the current broad flat collision plane when accepting nonflat city geometry. Road/terrain holes remove pick and collision surfaces consistently; elevation-separated crossings do not become intersections just because XZ segments cross.

The foundation's generic height callback checks accepted centre/edge samples and their connecting grades. Local offset edges must advance along each centreline step; a wide road folding around a tight curve fails instead of emitting a reversed ribbon. This is not a global self-intersection or junction solver. Point queries cannot detect a terrain ridge, hole or missing cell between queries, and the Bezier tolerance/maximum step apply before terrain fitting. Runtime integration needs terrain-cell-aware subdivision/coverage and error checks against the retained snapshot before claiming continuous surface fit or absence of implicit bridging.

## Source edits, undo and publication

Each completed draw/drag/split/delete is one concrete primary-task command, proposed `EditCityRoadCmd`, with before/after immutable source snapshots, expected document revision and project/catalog session. Preview mouse motion does not append history entries or create persistent scene actors. Concrete execute/undo/redo validate the expected source revision before mutation and mark the owning actor/asset dirty through existing Editor paths. Ctrl+Z reverses a completed command; during an unfinished draw it removes the most recent preview knot, without changing persistent source. Escape and tool/selection/focus/project changes cancel the draft.

Keep canonical source revision and active visible generation revision distinct. A valid edit can remain dirty while an asynchronous bake fails; the previous visible city remains intact and reports its older revision. Validation or loading failure must not overwrite the previous source. Prepared road meshes, intersections, terrain grading, pads, colliders and foliage exclusions must commit as one generation, with no frame using a new surface and old collider. Check project/scene lifetime, source revision and terrain revision before work and before publication. Reuse TerrainSystem's publication/physics lock ordering and no-fail swap pattern; do not wait for physics while holding a source mutex.

Save one canonical file through the project asset path with content-hash/revision checks, temporary-file replacement and catalog update. The scene stores the source UUID/reference. Save/reopen refreshes runtime snapshot revisions. Undo history is session state and is not another JSON source. Tombstones and locks are persistent source state.

If an external editor changes the source while local edits are dirty, stop automatic overwriting and report the conflicting file/revision. Offer reload/discard local, keep local as a new source, or a supported three-way merge by stable IDs. Commands from a closed project or incompatible external revision cannot replay silently. A future merge must expose simultaneous edits to the same knot/road; positional proximity is not identity.

## Practical implementation sequence

| Slice | Concrete change | Acceptance |
|---|---|---|
| A: CPU source foundation | `CityRoadSplineSource.hpp/.cpp`: bounded strict JSON v1, GUID identities, Bezier controls, Fixed/Drape modes, terrain callback fitting and sampled cross-sections. Bridge returns an explicit unsupported error. | Codec roundtrip retains manual edits/IDs; invalid/duplicate/oversized input leaves last-good output; curved sampling, finite frames, plane/slope/hole/outside/grade tests. This alone is not Editor drawing or rendered terrain fit. |
| B: Scene/asset binding | Add a city source UUID/reference and explicit terrain instance binding to the existing procedural scene workflow; read/validate the source and regenerate from its immutable candidate. Wire project save/reopen before adding drawing UI. | Programmatic source edit changes the same scene after reload; source/terrain failures retain old source/visible generation; duplication remaps instance identity without duplicating source identity. |
| C: First actual draw tool | Add a contextual city authoring window/controller through UIManager, following TerrainWindow listener ownership. Extend the scene-image ray hook with a city event/active-tool arbitration. Left-click adds a knot; click-drag adjusts its Bezier handle; Enter/double-click commits one road; Escape cancels. Render only a lightweight preview until commit. Start with one Drape spline on one selected terrain and explicit width. | Actual Editor pointer/DPI/docked-image tests and an interactive recorded session: draw, adjust handles, undo/redo, cancel, save and reopen the same curve. Do not claim this from mocked Lua or CPU codec tests. |
| D: Topology and generator edits | Add endpoint snapping, junction IDs, split/join, locked seeded roads, tombstones and pinned routes to the same source model. | Seed regeneration preserves hand edits and IDs; unrelated generated branches remain stable; invalid pinned routes are diagnosed. |
| E: Terrain grade/pad publication | Stage road profile, junctions, cut/fill candidate, building pads and physics before one revision swap. | Native ray heights and driving contacts match the rendered grade; failed upload/physics preparation, terrain change, unload and reopen retain or cancel the correct generation. |

The initial UI must arbitrate with terrain painting, asset dragging, normal actor picking and transform gizmos; only the active tool consumes the pointer. Camera navigation remains available outside an active left-button edit. Reuse the viewport's exact content rectangle and world ray instead of another screen-to-world conversion. Existing terrain event names are not a general city API; new city dispatch and controller ownership need explicit implementation and tests.

Release gates include unsupported terrain transform/hole diagnostics, full-footprint pad/road clearances, grade limits and metre-scale texture continuity, bounded work/source/GPU residency, save conflicts, Ctrl+Z lifetime checks, render/collision revision agreement, and fresh Editor/package captures. None of the current city captures establishes that those authoring gates are complete.
