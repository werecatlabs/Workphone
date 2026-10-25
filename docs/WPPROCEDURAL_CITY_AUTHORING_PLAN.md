# Editable city roads and terrain placement

Status, 25 October 2026: the native **Window > City Roads** tool, strict v1/v2 source codec, terrain-cell road fitting and single-lane roundabout solver connect to generated grid/L-system cities through **City Terrain Roads**. Initial generation promotes roads once into the scene-owned canonical source; regeneration preserves road edits and the bound terrain. Whole-parcel support, fitted footways, connected roundabout pedestrian edges and production activation/readiness guards are implemented. Builds 28/29 complete the Sample/native dependencies, Editor and renderer-test target. The final authoring/viewport/anchor plus six Play cases pass 9/9 in 71.96 s; the renderer suite passes in 30.76 s. Nine other geometry cases passed tests 21. Final native run 16 passes functional retention/contact with exit 0 in 249.76 s; captures 13 grid street, 14 natural boulevard and 15 natural overview also validate and exit 0. All three capture inventories match snapshot 16's 11 runtime hashes. Native 09's historical shutdown fault remains unexplained, and the original 180-second timing gate remains open. Recorded Editor mouse/DPI acceptance, project source assets, terrain cut/fill, bridges, atomic terrain/city replacement, source-free reconstruction, hardware performance and AAA visual acceptance remain open. The independent grid review rates the scene a prototype with commercial quality **not met**. See [the production plan](WPPROCEDURAL_PRODUCTION_PLAN.md) and [implementation evidence](WPPROCEDURAL_IMPLEMENTATION_STATUS.md).

The intended workflow is to generate a city, draw and edit road splines in the existing Editor viewport, preserve those edits through regeneration and save/reopen, and fit roads, junctions, building pads and collision to the selected terrain. A generated graph and its sampled meshes are derived caches. They must not become competing editable documents. The connected [river, road/rail bridge and train-track design](WPPROCEDURAL_CITY_TRANSPORT_WATER_PLAN.md) extends this same source and terrain publication model.

## Existing paths to extend

| Concern | Actual source and current limit |
|---|---|
| Scene viewport input | [ImGuiApplication.cpp](../Engine/cpp/Source/WPImGui/ImGuiApplication.cpp) computes a world ray from the exact scene image rectangle. The `cityRoadViewportTool` hook queues bounded inputs to the Application task, draws a cached preview, and arbitrates against terrain painting, ImGuizmo and ordinary picking. |
| Editor listener lifecycle | [CityRoadWindow.cpp](../Tools/cpp/Editor/src/ui/CityRoadWindow.cpp) follows [TerrainWindow.cpp](../Tools/cpp/Editor/src/ui/TerrainWindow.cpp) ownership: attach to the current viewport, invalidate queued input on tool/selection changes, cancel on hide/unload/focus loss, and release listeners. `SceneWindow` is the hierarchy; `FBGameEditorWx` is not the active ImGui path. |
| Editor windows | [UIManager.cpp](../Tools/cpp/Editor/src/ui/UIManager.cpp) creates the native `CityRoadWindow` and exposes it through **Window > City Roads**. |
| Commands and scene persistence | `CityRoadEditCommand` implements execute/undo/redo through `ICommandManager`, validates scene/project lifetime and expected authored state, then applies with the current revision token. **Save Scene** uses the existing scene-save job and serializes the component's canonical source and terrain binding. |
| Project assets | [AssetDatabaseManager.hpp](../Engine/cpp/Include/Workphone/Database/AssetDatabaseManager.hpp) provides UUID lookup, catalog generations, current-entry checks and journaled file operations. Register the city source through existing project asset identity; do not invent a second catalog or manually rewrite sidecar identities. |
| Road generation | [ProceduralRoad.cpp](../Engine/cpp/Source/Workphone/Scene/Components/ProceduralRoad.cpp) currently exposes Start/End and generates a segment through `IRoadSystem`. [CRoadMeshElement.cpp](../Engine/cpp/Source/WPProcedural/CRoadMeshElement.cpp) sweeps a supplied polyline into left/right vertex pairs. Its `Points` and endpoint tangents are not an editable spline model. |
| Curve math | [LinearSpline3.hpp](../Engine/cpp/Include/Workphone/Math/LinearSpline3.hpp) supplies existing Hermite/Catmull–Rom interpolation. Explicit authoring handles are cubic Bezier controls; bounded De Casteljau sampling is a small compatible CPU helper, not a replacement scene lifecycle. |
| Terrain queries | [TerrainData.hpp](../Engine/cpp/Include/Workphone/Graphics/TerrainData.hpp) defines immutable snapshots, `sampleTerrainHeight` and `intersectTerrain`, with the same triangle diagonal as render/collision. Holes and outside-footprint queries fail. Terrain placement validation supports translation, positive scale and normalized yaw; do not silently accept unsupported tilt. |
| Terrain edits/publication | [TerrainSystem.hpp](../Engine/cpp/Include/Workphone/Scene/Components/Terrain/TerrainSystem.hpp) exposes expected-revision terrain updates and begin/update/commit/cancel edit strokes. [Terrain.hpp](../Engine/cpp/Include/Workphone/Graphics/Terrain.hpp) separates requested/active snapshots and exposes a publication gate for prepared render/physics consumers. Reuse those contracts. |
| Authored road publication | [CityRoadAuthoringRuntime.cpp](../Engine/cpp/Source/Workphone/Scene/Components/CityRoadAuthoringRuntime.cpp) prepares matching render/mesh-collision inventories and swaps them under render/physics locks. Generated actors are disposable and excluded from scene saving. The separate `ProceduralRaceScene` whole-city rebuild still clears its assets first. |
| Generated city integration | [ProceduralRaceSceneBuilder.cpp](../Engine/cpp/Source/Workphone/Scene/Components/ProceduralRaceSceneBuilder.cpp) promotes the selected grid/L-system graph into canonical v2 roads and eligible roundabouts, then derives building pads, dressing and foliage from that source and retained terrain. Scene-owned terrain/source anchors survive regeneration; generated graph indices remain disposable. |

## Current practical workflow

1. Enable the `ProceduralRaceScene` **City Terrain Roads** property, or start `SampleVehicleAdvanced` with `--city-terrain-roads` (implies `--city`). The existing capture script accepts `-CityTerrainRoads`. Initial generation creates persistent terrain/source anchors and promotes the chosen graph once. Refresh the hierarchy if needed, select **Editable generated city roads**, then open **Window > City Roads**.
2. For an independently authored source, select a terrain component or its actor and choose **Bind Selected Terrain**. This also rebinds the selected source when changing terrain. Controls use world metres and require an identity authoring-actor transform.
3. Choose **Draw Road**. Click supported terrain to add Bezier knots; click-drag sets handles. Set **Road width**, **Maximum grade** and **Closed spline** as needed. **Enter** or **Commit Road** commits the draft; **Escape** or **Cancel Draft** discards it. During an unfinished draw, Ctrl+Z removes the last draft knot.
4. Choose **Move Knot**, **In Handle** or **Out Handle**, then drag the corresponding point on terrain. A completed drag is one command. Select a knot before **Apply Road Fields** or **Delete Selected Road**. Referenced roundabout approaches are guarded against individual deletion: select an approach knot and use **Delete Selected Roundabout** to remove the junction and connected approach roads with one undo step.
5. Use **Undo**/**Redo** or viewport Ctrl+Z/Ctrl+Y for committed edits. Regeneration rebuilds derived car, buildings, pads, dressing and foliage against the current source/terrain; it preserves hand edits and the bound terrain. Seed or city-size changes do not replace a promoted road source. **Save Scene** stores the canonical source, terrain binding and vehicle's source-anchor UUID; reopen resolves the anchors in the current scene and reconstructs disposable geometry.
6. To add a junction, set **Roundabout outer radius**, **Island radius**, **Clockwise circulation** and a road width of 4..12 m, choose **Place Roundabout (4 approaches)**, then click terrain. The tool derives a local grade plane and four connected approach roads. It conservatively rejects holes, excessive slope/deviation, unsupported footprints or centre/edge join mismatches; it does not grade terrain to force a fit. Defaults are outer radius 14 m and island radius 8 m; the circulating lane must be 3..7 m wide.

Once terrain-city anchors are active, switching that component back to the flat city is explicitly unavailable; use a fresh scene for flat generation. This prevents two active ground worlds. If road edits no longer support the pinned generated race route, racing is unavailable with a diagnostic while the static city and authored source remain; no substitute route is invented.

The CPU roundabout tier supports three to six ports, a raised central island/kerb, concrete-edged splitter refuges where the approach is wide/long enough, and directed entry/exit routes. Generated junction selection is bounded to eight source roundabouts and four in the sample. Adaptive approaches retain source-road clearance; fitted outer walks share mitered joins with approach sidewalks where full width fits. Ordinary junctions meet geometrically but do not provide solved lane topology. Design-vehicle swept-path approval, country/signage profiles and arbitrary junction editing remain future work. Current native evidence is six-block grid/natural captures; recorded Editor pointer acceptance remains open.

Captures 04/05 preserve the earlier clean-exit baseline. Diagnostic 07 exposed cached FSM flag storage; current locked manager-slot access and unloaded/deferred-reference guards fix that boundary. Diagnostic 08 timed out under CDB. Native 09 then exposed incomplete static-body activation and a different shutdown fault; 10 reproduced the foundation assertion with clean failure exit 1, leaving that crash's caller unknown. Separately, CDB 11 identified null scripting-manager polling in Editor Play tests and a test-exit orphan-manager/type-pool cleanup defect; both source fixes are included in builds 28/29, and all six Play cases now pass. Snapshot 16 retains the final runtime binaries, matching symbols and scoped source hashes. Its normal native run preserves the edited GUID/11.75 m width, rebinding, old-terrain retirement and snapshot refresh, passing unchanged retention/contact assertions in 249.76 s under a separate 600 s limit. Captures 13/14/15 verify the latest grid/natural contacts and parcel culls with matching runtime hashes and clean exits. Native 09's cause remains unknown, and the original 180-second timing gate remains open.

## One editable source and stable identity

The implemented tier stores **one canonical JSON source inline in the scene's `CityRoadAuthoring` component**, with its terrain-component binding. Terrain and source actors are scene-owned anchors, independent of regenerated city assets; `ProceduralRaceScene` persists its **City Terrain Roads** flag and **City Road Authoring UUID** reference to the source component. Its coordinates are world metres; the authoring actor is not a second transform. Runtime meshes, colliders and overlays are derived caches. A project-owned `.cityroads.json` asset, catalog registration, importer, external-file conflict handling and project-source-asset UUID reference remain future work; they must replace this ownership deliberately rather than add a second editable copy.

Current sources, road splines, control points and roundabout junctions have canonical lowercase GUIDs. Editing a point or width preserves its ID. Future split/join and pinned-route tools must preserve stable references: splitting retains one road ID and creates another; joining changes explicit references rather than relying on nearest positions. Duplicate IDs and unresolved references fail current validation.

Roundabouts are typed junctions in this same source, with stable spline approach ports and directed ring movements. The [roundabout design and T0–T5 gates](WPPROCEDURAL_CITY_TRANSPORT_WATER_PLAN.md#roundabouts-and-junction-authoring) cover island-avoiding routes, terrain/collision fitting and explicit country/driving-side profiles; a decorative ring mesh does not implement those contracts.

The current generated-city tier promotes the initial grid/L-system graph into strict v2 road controls and real roundabout records once, then treats that canonical source as authored input. Regeneration and seed/size changes preserve it. The future v3 generator-edit tier must add provenance, stable production lineage, persistent locks and deletion tombstones before supporting controlled regrowth or source replacement. Those generator overrides are not present v2 functionality.

The codec retains strict **version 1** compatibility for source/terrain identity, spline/control-point GUIDs, Bezier controls and road profiles. Actual **version 2** adds a bounded `roundabouts` array with explicit radii, grade-plane/sampling limits, circulation and ports referencing existing road GUIDs; it also permits an empty source. Its only top-level fields are `schema`, `version`, `id`, `revision`, `terrain`, `roads` and `roundabouts`. Unknown fields, versions and unresolved/duplicate references fail. Generator overrides, tombstones, editable pinned-route records, project asset file I/O and the complete schema below remain unimplemented.

[The hand-editable version 1 example](examples/city-road-source-v1.cityroads.json) is accepted by this codec. Its terrain UUID is example identity, not an included or resolved terrain asset; integration must replace/bind it to the selected project terrain. `CityRoadSplineSourceTests` accepts the example file path as its first argument to validate loading and save/reopen without an Editor session.

## Proposed complete source schema

This **version 3 target example is design data**, rejected by the current v1/v2 codec. It reserves a separate future version for the additional fields below; it is not the implemented minimal v2 roundabout format. Numeric distances are metres; grades are rise/run, not degrees. Handles are vectors relative to their control point. Terrain placement is explicit in this proposed future asset format; observed runtime revision is diagnostic and must be refreshed after reopening, never reused as a durable concurrency token.

```json
{
  "schema": "workphone.city-road-source",
  "version": 3,
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

1. Resolve the bound terrain component, retain its immutable snapshot and validated placement, and evaluate the current world-metre source against it. A future city-local asset transform is part of the v3 design. Pick using `intersectTerrain`; never substitute a generic Y=0 plane after a terrain miss.
2. Flatten each Bezier span with bounded geometric tolerance, maximum segment length, recursion/work limit and total sample budget. Preserve original controls/IDs; flattened points and cross-sections are disposable outputs.
3. **Drape** samples terrain height and checks grade/crossfall, then the native terrain mesh helper clips the complete ribbon against the retained terrain's actual cell triangles and holes. Its supported surface bias is at most 0.03 m. **Fixed** retains the authored 3D profile and rejects unsupported floating/cut profiles instead of inventing a bridge. Roundabouts retain one grade plane and validate the entire supported footprint plus approach centre/edge seams. **Bridge** remains explicitly unavailable until deck, abutments, clearance and collision are implemented.
4. A future optional cut/fill solver prepares a separate terrain candidate with bounded excavation/fill and feathered shoulders. Keep the underlying terrain source intact or store a reversible grading layer. Junction approaches solve one compatible elevation/crossfall; normal smoothing cannot fix a physical step.
5. [CityRoadParcelSupport](../Engine/cpp/Source/Workphone/Scene/Components/CityRoadParcelSupport.cpp) checks the entire parcel against actual terrain cells and edited road/roundabout reservations. Holes, uncovered cells, excessive cell grade or relief above 1.25 m cull the parcel. Accepted parcels retain minimum/maximum support heights for supported foundations and pads; new buildings are culled from edited road corridors. This does not cut or fill terrain.
6. Rendering, collision, picking, surface queries and foliage exclusions consume the retained fitted source/terrain revision. Terrain-city mode uses the bound terrain rather than the broad flat-city ground. Existing foliage uses an identity standalone carrier with holes and no rendered ground, world positions and pad heights; it must not apply the bound terrain transform a second time. Road/terrain holes remove pick and collision surfaces consistently; elevation-separated crossings do not become intersections merely because XZ segments cross.

The generic CPU height callback alone still checks only accepted centre/edge samples and cannot prove continuous terrain coverage. The implemented native [CityRoadTerrainMesh](../Engine/cpp/Source/Workphone/Scene/Components/CityRoadTerrainMesh.cpp) adds exact cell clipping, full-footprint support/deviation and cell-grade checks before preparing matching mesh collision; parcel support reuses that coverage calculation. Local offset edges must advance along each centreline step; a tight curve that folds a wide road fails. Global spline intersection/lane solving and terrain cut/fill remain outside this tier.

## Source edits, undo and publication

Each completed draw/drag/field edit/delete or roundabout placement uses `CityRoadEditCommand` with bounded before/after source snapshots and scene/project guards. It compares expected authored state while allowing monotonic runtime revision changes, then applies with the current revision token. Preview motion does not append history or create persistent actors. Ctrl+Z reverses a completed command; during an unfinished draw it removes the most recent draft knot. Escape and tool/selection/focus/project changes cancel the draft. Split/join commands remain future work.

Canonical source and active generation revisions are distinct. For a source edit against unchanged terrain, validation/preparation failure retains the accepted source and road generation; a prepared render/collision candidate publishes together under task locks after lifetime/revision checks. **Terrain edits are not a joint atomic city transaction:** terrain may publish first; if dependent road recooking fails, the old roads are deactivated and the diagnostic remains visible. Keeping a complete old-or-new terrain/city bundle is future work. Do not wait for physics while holding a source/preview mutex.

Derived-city automatic refresh watches the current canonical source revision together with the retained terrain snapshot and placement. It attempts a changed tuple once, rather than using a stale initial generator bake or retrying a rejected tuple every frame. Building/pad/dressing/foliage refresh preserves both scene-owned anchors. The whole derived-city replacement remains clear-first; this refresh is not a joint terrain/city transaction. Unsupported pinned-route edits disable racing while keeping the static city/source available.

Production activation runs on the Application task under render/physics locks. It resolves parent transforms, activates generated actors and persistent source/terrain anchors through their normal actor/component lifecycle, and verifies attached native static bodies, shapes and world poses before setting driving readiness. Until the scene and owner hierarchy are attached and ready, driving stays braked; failures report a diagnostic. Deferred Edit/Play queues discard unloaded, removed or transferred actors and events for the wrong scene state while admitting live children through a current-scene ancestor. The regression exercises real root/child queues, repeated drains, persistence and FSM retirement. Final run 16 and captures 13/14/15 pass the sampled native retention/contact checks; the complete restored driving-scene and recorded pointer sessions remain separate gates.

The implemented **Save Scene** path serializes one canonical inline source and terrain binding; generated road actors carry `DontSave`. Save/reopen refreshes runtime terrain snapshots and rebuilds those caches. Undo history is session state. The future asset tier must add canonical-file replacement, catalog/dependency updates and external conflict handling before switching the scene to a project-source-asset UUID reference. Tombstones and generator locks are proposed persistent fields, not present v2 fields.

When the future project-asset tier permits external edits, dirty-file conflicts must stop automatic overwrite and offer explicit reload/copy or a supported stable-ID merge. That external-file workflow is not implemented. Current commands already reject closed scenes/projects or authored-state conflicts; positional proximity is not identity.

## Practical implementation sequence

| Slice | Concrete change | Acceptance |
|---|---|---|
| A: CPU source foundation | Implemented strict JSON v1/v2, GUID identities, Bezier sampling and explicit Fixed/Drape modes; v2 adds minimal roundabouts and empty documents. Bridge fails unsupported input. | The nine other geometry cases passed tests 21; authoring passes the final tests 23 rerun. These checks do not establish interactive acceptance. |
| B: Scene/asset binding | Implemented inline scene source, persistent terrain/source anchors, vehicle UUID binding, prepared road render/collision publication and generated-city activation/readiness barriers. Project catalog/importer/source-asset references remain future work. | Final tests 23 pass authoring/viewport/anchor plus all six Play cases, including real root/child queue activation and FSM/persistence. Native 16 functional retention/rebinding and 13/14/15 grid/natural contacts pass with clean exits. The original 180 s timing and recorded pointer gates remain open; failed terrain recooks deactivate roads. |
| C: First actual draw tool | Implemented **Window > City Roads**, click/drag, Enter commit, Escape cancel, handle/field edits, commands and roundabout placement. | Final actual Editor pointer/DPI/docked-image tests and recorded draw/edit/undo/reopen session remain pending. |
| D: Topology and generator edits | Initial graph promotion into v2 and bounded real roundabout selection are implemented; later regeneration preserves authored roads. Add solved ordinary junction lanes, split/join, provenance, locks, tombstones and editable route records in v3. | Current seed/size changes must preserve promoted controls/IDs; unsupported pinned routes report racing unavailable. Full regrowth/override tests remain future work. |
| E: Terrain grade/pad publication | Full-parcel cell coverage, support extrema, edited-corridor exclusions, full 2.45 m pavement reservation and Source+Terrain tuple refresh are implemented. Joint cut/fill/road/pad/physics publication remains future work. | CPU pavement/parcel regressions, native 16 retention/rebind/contact and 13/14/15 grid/natural contact/cull checks pass. The original 180 s timing gate and a complete restored driving scene remain open; later atomic failure gates must retain a complete old-or-new terrain/city bundle. |

The implemented city dispatch arbitrates with terrain painting, asset dragging, ordinary picking and transform gizmos using the existing viewport image rectangle and world ray. Camera navigation remains available outside an active left-button edit. Actual interaction, focus/lifetime recovery and DPI/docked-viewport behaviour still need the final recorded acceptance session.

Release gates include unsupported terrain transform/hole diagnostics, full-footprint pad/road clearances, grade limits and metre-scale texture continuity, bounded work/source/GPU residency, save conflicts, Ctrl+Z lifetime checks, render/collision revision agreement, and fresh Editor/package captures. None of the current city captures establishes that those authoring gates are complete.
