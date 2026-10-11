# Foliage workflow

The supported foliage path is `FoliageAuthoring` → immutable `FoliageWorldData`
→ `FoliagePageStream` → `ClawFoliageWorld` on DX11. Terrain components publish
their grounded foliage through `Terrain::setFoliageSnapshot`; standalone worlds
can be added to `ClawScene`. The older `ClawFoliageBatch` API remains available
for fixed-LOD populations and the original benchmark.

## Authoring and terrain

Use the terrain window's Foliage controls to create species, set scatter rules,
generate or regenerate instances, and paint or erase them in the viewport.
Native commands own the data and stroke transaction. Cancel restores the stroke's
starting state; undo/redo retain the recipe, species and manual edits. Save the
scene to persist the terrain component's `foliageDataV1` property, or export/import
the standalone authoring document through the foliage controls.

Scatter uses signed global candidate coordinates and a stable rank for spacing
conflicts, including neighbours outside the requested cells. Reversing cell order
does not change the generated IDs or spacing. Species mixture, density, slope,
height, layer weight and moisture rules combine with density/biome masks and
polygon or road/river corridor exclusions. Manual instances, locks, removed IDs
and promoted IDs survive regeneration. A terrain hole suppresses the runtime
instance without erasing its authoring record. Manual vertical offsets survive
terrain resampling.

Native plant recipes generate separate near, middle and far meshes, with bark and
leaf/clump sections, UVs, normals, bounds and analytic height-based wind weights.
`importFoliageMesh` adapts indexed CPU meshes with explicit materials. Imported
geometry initially occupies all three LOD slots; supply authored reduced meshes
when appropriate. This does not depend on optional ngPlant integration.

## Rendering

`ClawFoliageWorld::prepare` gathers the union of active views and consumes immutable
page results. `render` builds independent culling and LOD decisions for each view.
LOD is projected-height based with hysteresis and supports forced levels. The
transition policy is a hard switch with hysteresis. Shadow distance and LOD bias
are species properties. Grass and ground cover use clumps/cards with stable
deterministic density subsets; they do not create one impostor per blade.

Visible instances from different cells share species/LOD/material-section bins.
Capacity splits use the renderer's reusable 4,096-instance buffer. The dedicated
foliage stream is 160 bytes per instance and leaves the existing 112-byte instanced
geometry ABI unchanged. Statistics separate visible populations, LOD counts,
draws, submitted triangles, uploads, resident pages and resource reservations.
Scene failures are available through `ClawScene::getFoliageError` and logged when
the message changes.

Materials support albedo, normal and ORM maps, tint, cutout threshold and two-sided
lighting. Colour and shadow passes share the same frame time and wind deformation.
The bounds include the declared wind and interaction displacement. Up to sixteen
world-space interaction spheres can bend nearby foliage through
`ClawScene::setFoliageInteractions` or a standalone world's `setInteractions`.
The material/geometry upload owner is the render thread.

## Impostors

`bakeFoliageImpostor` consumes a validated species and immutable CPU texture
snapshots. It rasterizes multiple views into albedo, capture-tangent normal, ORM
and normalized depth atlases. All views use the same enclosing-sphere square;
the runtime billboard uses the same projection. Gutters and isolated mip
generation protect adjacent views. Cancellation, malformed textures and work
budget failures retain the caller's previous output.

The graphics plugin installs the Editor's CPU texture resolver. It accepts decoded
source images and retained portable cooked mip data. DDS-only and render-target
textures without a CPU image report an unavailable bake source; the baker does
not read a live GPU texture back implicitly.

`saveFoliageImpostorSource` writes four TGA images and `.texres` descriptors into a
fresh directory, then publishes the `.folspec` descriptor. It rejects replacement
of an existing directory to avoid partially overwriting a species. Relightable
normal/ORM shading is supported; depth is exported as data, without a depth-relief
or parallax reconstruction shader.

Editor bake destinations map to the parent of the `data://` resource prefix under
the project root. Leaving the directory empty derives that location; an explicit
directory must match. This keeps saved atlas IDs resolvable on reopening and by
the resource compiler.

Source export saves atlas base images and texture recipes. Recooking rebuilds
isolated mips using cutout albedo, normalized normals and linear packed ORM/depth.
It does not retain the baker's green-only roughness-filtered mip chain: the existing
roughness texture policy applies to all RGB channels and would alter packed
occlusion/metalness channels.

## Cooking and installed packages

Register `GraphicsResourceCompiler` and `FoliageResourceCompiler` with the existing
resource compiler registry. `writeFoliageSources` exports a validated world into
the registry's source root. It writes immutable generation-named species/page
descriptors before publishing the world manifest. Resource extensions are:

| Extension | Contents |
| --- | --- |
| `.folspec` | One species, three meshes, sections, material/atlas references and policies |
| `.folpage` | One signed cell of stable instance records and pinned species dependencies |
| `.folworld` | Page directory, counts, pinned species/page dependencies and page size |

The existing resource type identifier supports at most eight characters, hence
`.folspec`, rather than `.folspecies`. Species reference ordinary `.texres`
assets. The installed dependency closure includes all pages and textures, while
runtime demand reads page payloads only when requested. Shallow runtime reads
validate IDs, pins, target/build identity and payload hashes; they are not a
substitute for installing the full dependency closure.

Source descriptors remain version one. Cooked foliage payloads/compiler outputs
are version two and include conservative geometry/bend extents in the world
directory. Earlier cooked foliage requests report a recook requirement. Generation
files use short immutable `.foliage/<generation>/sN.folspec` and `pN.folpage` paths;
the shared resource cache uses compact target/resource keys while validating full
identities and pins. Existing cache paths and installed manifests remain readable.

Queue or directly decode the cooked world, then call `stageCookedFoliageWorld`
on the render owner thread. Pass the returned source and texture resolver to
`ClawFoliageWorld::setSource`. Raw files are unnecessary once the package is
installed. The staging path uses the existing portable texture and DDS validation
and upload contracts.

## Physics and gameplay

`TerrainSystem` updates a nearby collision population on the physics task using
the active camera. For additional players or simulation observers, use
`FoliagePhysics` directly and pass their positions to `update`. Up to eight
observers share the nearest eligible set. Tree/bush species opt in with collision
radius, height and distance; grass and ground cover never create solids.
Conservative trunk boxes support translated, rotated and nonuniformly scaled
terrain placements. Collision distance is independent of visual LOD and shadows.
Capacity pressure is reported in the collision statistics.

Terrain foliage publication stages replacement collision bodies before switching
the visible source and placement. A busy scene, rejected body or exhausted budget
retains the previous complete generation. Physics and render publication share the
native scene boundary. A standalone cooked page stream currently supplies visual
residency; applications needing collision must also provide `FoliagePhysics` with
an authoritative instance snapshot.

`FoliageGameplay` accepts an application's actor/prefab acquire and release
callbacks. Promotion creates a gameplay actor and flags its compact instance;
demotion releases the actor and returns the instance; removal leaves a persistent
tombstone. Call `TerrainSystem::refreshFoliage` after changing its authoring service,
or republish the new snapshot for a standalone world. Acquire must return a staged
actor and release must not throw. Save promoted actors as part of the game state,
or demote them before saving. The helper releases owned actors on destruction.

## Bounds and validation

- World: 64 species, 2,048 cells, 1,048,576 total instances, 65,536 per cell and
  128 MiB of source data. Page size is 1–4,096 metres.
- Authoring: 65,536 instances and 56 MiB of authored source, 262,144 candidate
  samples including neighbour halos, eight undo entries within 64 MiB. Increase
  candidate spacing when generation exceeds
  its work limit; a rejected generation retains the existing world.
- Paging: at most sixteen finite views; each local-space range is at most thirty
  page widths, including off-centre/scaled geometry, flutter, bend padding and one
  page of fringe for cell-centre demand.
  Source/placement combinations outside that range fail explicitly. Custom lazy
  sources supply `maximumGeometryRadius`/`maximumBendRadius`; loaded pages that
  exceed these declared extents fail before publication.
- Rendering: at most 262,144 visible instances per view, bounded view history,
  shared scheduler reservations for CPU work, retained results and GPU resources.
- Collision: 512 nearby solids per terrain by default, configurable up to 2,048;
  at most 256 promoted actors per gameplay helper.
- Impostors: bounded view count, atlas dimensions and raster samples. Existing
  three-view `MeshImposterGenerator` callers retain their previous API.
- Authored texture descriptors: at most 32 MiB of decoded portable mip data per
  source texture. Cooked species JSON must fit the decode scratch budget (2 MiB
  with the default 128 MiB working reservation); larger assets require explicitly
  increased decode and shared scheduler budgets.

The shared scheduler also serves terrain. Cancellation generations and retained
leases prevent old page results from publishing into a reused slot and keep
memory charged until the final consumer releases it. Resource exhaustion and
unsupported operations report errors rather than successful no-op results.

Authoring reads/writes are serialized around complete immutable snapshots; terrain
grounding releases component locks before reading authoring state, then checks for
intervening edits before queuing a result. Pending refreshes retry on the physics
update. Building without the resource system leaves raw texture paths available
and reports authored `.texres` resolution as unavailable.

Run `Tools/BuildWPGraphicsBaseline.ps1 -IncludeEditor` to build and validate Debug
and RelWithDebInfo. The baseline includes the new foliage CPU/physics, DX11,
cooked-resource and Lua workflow contracts, together with terrain and existing
graphics tests. Release certification still requires representative forest frame
timings, interactive Editor inspection and the plan's eight-hour soak; automated
contracts do not substitute for those checks.
