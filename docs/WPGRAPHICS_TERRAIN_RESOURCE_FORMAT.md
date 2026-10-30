# Terrain assets and runtime publication

This describes the rectangular terrain source, DX11 material and cooked resource contracts. Test results and release gates are recorded separately in [implementation status](WPGRAPHICS_IMPLEMENTATION_STATUS.md).

## Authoring source

`serializeTerrainData` writes JSON version 3. `deserializeTerrainData` accepts versions 1, 2 and 3 and replaces its output only after validation. Serialized source JSON is bounded to 128 MiB. Version 1 migrates to four default layer descriptions, an implicit layer-zero control map and solid cells. Version 2 retains those authored values and receives appearance defaults that preserve its previous rendering.

- Height samples are row-major: `z * width + x`. Local position is `(origin.x + x * spacing.x, height * heightScale, origin.y + z * spacing.y)`.
- Dimensions are at least 2 by 2, each at most 8193, with at most 4,194,304 samples. Rectangular grids are supported. Spacing is finite and positive; height scale may be zero or negative. Scaled positions must remain finite.
- Exactly four layer descriptions carry albedo, tangent normal and packed ORM texture names; UV scale in texture cycles per local metre; tint; roughness and metallic factors; and normal strength. ORM uses red for occlusion, green for roughness and blue for metallic. Normal and ORM data remain linear; albedo samples are converted to linear colour before blending.
- An empty `layerWeights` array means layer zero everywhere. Otherwise there is one four-component control sample per height sample. Nonzero authored weights must sum to one within the validation tolerance; all-zero weights use layer zero. The shader also normalizes positive sampled weights defensively.
- An empty `holes` array means solid terrain. Otherwise it contains one 0/1 flag per cell, at `z * (width - 1) + x`. A hole omits both source triangles from rendering, gameplay picking, collision and exported navigation input. The Editor's Fill Hole repair query picks the retained authored heightfield inside hole cells; runtime geometry and collision change only through normal repair publication.
- Version 3 adds terrain-wide `appearance` settings and per-layer height scale/bias. Optional `surfaceMasks` contain one wetness/snow pair per height sample, both in `[0,1]`; empty masks mean `(1,1)`. Global wetness and snow default to zero.
- Supported placement is finite translation, positive nonzero scale and normalized yaw rotation. Runtime source revisions are publication tokens assigned by the terrain owner; persisted numbers must not be reused as those tokens.

The material is opaque and has four layers. Missing map names use white albedo, the unperturbed surface normal and scalar material factors. The legacy direct texture at slot zero remains the layer-zero albedo fallback when that layer has no authored albedo name.

Appearance settings enable planar or triplanar projection, ORM-alpha height blending, a shared detail-normal texture, seeded low-frequency macro tint, and masked wetness/snow. Height blending applies each layer's height scale/bias before the global transition width; missing ORM contributes zero height. Triplanar sharpness is bounded to `[1,16]`. Macro variation uses terrain-local coordinates and a stored integer seed. Wetness darkens albedo and changes roughness; snow blends colour/roughness and softens mapped normals toward the surface normal. These are surface appearance controls, with no implied weather simulation.

## Cooked resource

`TerrainResourceCompiler` compiles `.terrain` sources through `ResourceSystem`. Nonempty layer texture names and the optional detail-normal name must be whole `data://...texres` resource IDs. Their sorted, deduplicated set is both the compile dependency set and the exact installed texture closure, with at most thirteen resources. Albedo mip chains require colour filtering, normal/detail maps require normal filtering, and ORM requires data filtering; textures without generated mips are also accepted.

The current compiler output and payload version is **3**. The compiled-resource header stores the standard `IResourceCompiler::versionFor` fingerprint of container version, output version and resource type; that fingerprint is not the literal number 3. The binary payload has stable type magic `WPTERR01`, little-endian integers and IEEE float32 tile values:

| Field | Representation |
|---|---|
| Type magic | Eight bytes, `WPTERR01` |
| Payload version; packaged-build flag | Two uint32 values |
| Target; serialized terrain JSON | Each a uint32 byte length followed by UTF-8 bytes |
| Tile manifest header | Five uint32 values: manifest version 1, tile cells 64, LOD count 4, border policy 1, tile count |
| Each tile | ID and inclusive first/last sample coordinates (five uint32); minimum/maximum local XYZ and four conservative LOD errors (ten float32); hole flag (uint32) |
| Texture dependency count | uint32 |
| Each dependency | Length-prefixed resource ID; compiler version, source hash and payload hash (three uint64) |

The payload limit is 256 MiB. Target strings are at most 128 bytes, IDs at most 4096 bytes, and the tile manifest contains at most 2048 records. The compiler reserves 256 KiB of the payload limit for metadata and dependency records. The surrounding compiled-resource header and runtime manifest also pin resource identity, payload length/hash and the exact install closure.

Border policy 1 retains every source sample on each tile boundary at every LOD. Coarse interiors use fans that meet this fixed boundary, so neighboring LODs share the same border positions, normals and UVs. Tiles containing holes use exact LOD0 geometry. Collision and authoritative queries continue to use the full-resolution source.

The reader validates tile coverage, coordinates, bounds, errors and hole flags against descriptions derived from the decoded source. Recomputing the outer payload hash does not make forged tile metadata acceptable. It also rejects unknown policy/version values, stale texture generations, wrong target/build mode, inappropriate texture mip semantics, missing or extra dependencies and trailing bytes. Payload/compiler versions 1 and 2 remain readable; version 1's missing tile manifest is derived during CPU decoding.

## Loading and publication

Use the explicit preparation split for streamed assets:

1. Load the immutable terrain resource closure through a runtime-mounted `ResourceSystem`. This requires the compiled package and its manifest; it needs no compiler registry, authoring database or source tree.
2. Call `decodeCookedTerrainResource(resource, target, packagedBuild, error)` on a CPU worker. This validates the source and tile manifest and decodes the texture mip payloads. It returns a sealed, immutable `DecodedCookedTerrain` and does not use a graphics device.
3. Call `stageCookedTerrainMaterial(renderer, decoded, error)` on the render owner. This uploads the already decoded texture levels and returns an immutable `CookedTerrainCandidate`. It performs no source/tile description or texture payload decoding.
4. Call `ClawTerrain::applyCookedTerrain(candidate, error)`. The terrain retains the exact texture closure and small verified tile metadata, bound to its requested source snapshot. Caller decode/candidate handles may then be released.
5. Render-owner tile preparation generates missing geometry through the bounded worker pool and uploads within the configured budget. Single-tile assets use a bounded synchronous preparation path with shared working/result memory admission. `getTerrainResidency().usedCookedTileManifest` reports use of verified metadata; the first renderer preparation does not rescan the source to describe cooked tiles. Subsequent authored edits invalidate this metadata and derive a fresh description.

The convenience overload taking a raw runtime resource performs CPU decoding synchronously before GPU staging. It is appropriate for small assets or loading screens; it does not provide asynchronous loading merely because it accepts a cooked resource.

For actual scheduled decoding, create a dedicated producer with
`WorldStreamingScheduler::shared()->createProducer(...)`, then call
`queueCookedTerrainDecode(producer, resource, target, packagedBuild, options, error)`.
Set the producer's ready-byte allowance for the selected decode budget; the
generic default of 8 MiB is intended for tile results. The helper reserves retained
input bytes and checks conservative parser/result bounds before replacing demand.
Options default to 256 MiB of inputs, 128 MiB of working storage and 64 MiB of
results, subject to shared availability and the producer's limits. Larger sources
can require larger explicitly configured budgets because JSON parsing uses more
memory than the final arrays. Take the immutable result on the owner thread and
cast its payload to `const DecodedCookedTerrain` before GPU staging. The payload
and uploaded dependency resolver retain their budget leases; cancellation discards
obsolete decodes. This schedules decoding of an already loaded immutable closure;
it does not disk-page the authoritative heightfield.

Tile source, collision, material and renderer publications use a common boundary. A staged replacement becomes active only when its demanded tiles and material are complete and validated. Pending work or failed texture resolution, decoding, allocation or publication retains the previous complete generation. Before an initial publication, or after unload, there is no active terrain surface. GPU references retired by a successful publication are released after source/physics publication locks are left.

The default renderer bounds are 2048 demanded tiles, 128 MiB each for active and staged geometry, and four tile uploads per renderer frame. Configuration accepts 1–2048 tiles, 512 KiB–512 MiB per generation and 1–16 uploads per frame. Direct preparation calls outside a renderer frame use that allowance per call. Demand membership is canonical: distance order changes do not cancel identical requests, and matching staged tiles survive view-demand changes. Up to sixteen views can contribute to one demand. Each terrain producer has at most 256 queued requests, one active job, eight ready tiles and 8 MiB of reserved result capacity in the shared two-worker scheduler. GPU admission also obeys the shared budget across participating tile buffers, owned control/weather maps and cooked textures; active/staged reuse keeps one reservation per actual allocation. Terrain draws release their context bindings before those allocations can retire. See the [workflow's shared budgets](TERRAIN_WORKFLOW.md#runtime-tiles-and-cooked-assets) for source, work, result and collision accounting.

Inspect `getTerrainResidency()` for requested/resident/staged counts, byte totals, upload totals, source revisions and worker counters. `getTerrainResidencyError()` reports the latest preparation failure and clears after a successful or normally pending preparation. These counters describe bounded work; they do not constitute a mixed-scene frame-time or release certification claim.

## Focused verification

- `ClawTerrainMaterialContracts.hpp`: four actual albedo maps, control normalization and zero fallback, normal/ORM and expanded appearance responses, shared shadow/fog, local-metre tiling and border continuity, invalid/stale candidate retention.
- `ClawTerrainStreamingContracts.hpp` and `ClawTerrainMotionContracts.hpp`: bounded native/GPU tile residency, full-generation publication, moving-view demand, overlapping staged work, per-frame upload allowance and visible output.
- `CookedTerrainContracts.hpp`: pinned layer/detail textures, source relocation, runtime load without compiler/database, worker decode, legacy migration, valid-hash forged manifest rejection, actual four-layer pixels and retained resource lifetime.
- `TerrainIntegrationContracts.hpp`: dirty ground placements, hole-aware navigation/path/shoreline inputs and real export retention on failure.

Run these through their registered parent test targets. Presence of a fixture or API is not evidence that it has passed on a given build, device or configuration.
