# Native cull CPU benchmark

Run from the repository root in PowerShell 7 on Windows, using the configured
`project_graphics_contracts` build:

```powershell
pwsh -NoProfile -File Tools/MeasureNativeCull.ps1 -Smoke
pwsh -NoProfile -File Tools/MeasureNativeCull.ps1
pwsh -NoProfile -File Tools/MeasureNativeCull.ps1 -Smoke -ProfilePhases -Objects 10000 -WarmupSeconds 1 -MeasurementSeconds 5
```

The second command rebuilds the production C90 scene-job contract executable in
RelWithDebInfo and measures five alternating flat/spatial pairs. Every variant
gets five seconds of warmup and thirty seconds of measurement. It takes about
six minutes. `-Objects`, `-Seed`, `-Workers`, `-Configuration`, `-BuildDirectory`
and `-OutputDirectory` select explicit variants. Worker count zero runs transforms
inline; positive counts use the existing borrowed native transform pool.
Keep unrelated builds and tests outside the measurement interval.

The fixture contains 10,000 objects by default, interleaved across ten static
clusters under a shared parent. Exactly `ceil(objects/10)` objects are visible
through the fixed default camera. Meshes share authored bounds; queue/z-order
values exercise sorting. Flat mode enables sphere and SAT tests. Spatial mode
adds the existing opt-in spatial groups. Both modes rebuild native capture,
transforms, bounds, visibility and sorted results each iteration; this measures
the complete preparation cost, including spatial construction and validation,
rather than only saved geometry tests.

`-ProfilePhases` enables eight disjoint native elapsed phases: entry/ancestor
capture, transform dispatch/wait, world bounds, group construction, view/frustum
setup, visibility, sorting and publication. Their per-run distributions appear
under `phases` in `summary.json`; raw phase columns include an explicit
`phases_available` flag. Without the switch those columns are zero and the phase
summary is empty. Native phase timestamps add instrumentation overhead, so compare
runs with the same switch and do not assume overhead meets the plan's 2% target.
The third command is a short diagnostic, not a timing-protocol performance run.

Six `transform_*` subphases are nested inside `transform_ns`: hierarchy traversal,
global reference deduplication (`unique`), local-matrix capture, prefix
planning, submission and wait/publication. Do not add these to the eight main
phases. Prefix planning now uses a dense lookup in existing reference scratch,
removing its second comparison sort without another allocation. With inline
transforms, `transform_submit_ns` includes matrix composition; with a pool it
includes task submission and any inline fallback while workers may execute
concurrently. `transform_wait_ns` measures remaining wait and destination copy.
These are owner intervals; no clock is called on a worker and they do not report
individual worker CPU time. Rejected dispatch preserves the last accepted timing
sample; cancelled/running/unprofiled/invalid-clock samples are unavailable.

Global deduplication now uses chained hash lookup in the existing reference
scratch, with packed unique nodes and bucket heads stored in independent fields.
No extra buffer is allocated. Sixty-four unequal comparisons in one lookup
trigger the retained comparison-sort fallback. `hash_probes`, `hash_collisions`
and `hash_fallbacks` are copied per frame; the summary also counts frames that
fell back. Probes count node comparisons and collisions count unequal comparisons.
On fallback these counters cover the attempted hash work, not sort comparisons.
Local indices follow first encounter on the hash path and pointer order on the
fallback; public output order and root-to-leaf matrix arithmetic stay unchanged.

Ancestor revisions now occupy one exact-sized array per shared cull publication;
entry records point into slices of it. Schema 5 records `entry_bytes`,
`ancestor_bytes`, `ancestor_refs` and `ancestor_allocs`. Ancestor allocation count
is zero for unowned/empty captures and one when revisions exist. Derived views
report the root publication's metadata; count these allocations once across its
views. This is a census of that array's allocation only, excluding other capture,
transform, result, sort and allocator work. Publication array bytes are entries
plus ancestors plus group/map arrays. Result retirement frees the ancestor array
only when the last retained view releases the publication.

Capture includes entry and ancestor allocations. Transform includes node-array
setup, dispatch and wait, rather than pure matrix arithmetic. Visibility includes
execute/result/index/classification allocations. Sorting includes order checks
and scratch allocation; publication covers ownership transfer and cleanup.
Scene-wrapper setup and the configure/execute gap are outside native phases;
result destruction remains separately measured by the outer harness. Phase
medians need not sum to the total median. Invalid-clock samples fail an explicitly
phase-profiled benchmark; ordinary native results remain usable without timings.

Windows QPC directly measures preparation and result destruction separately;
`lifecycle_ns` sums those intervals. Copying work/storage counters is outside
both phases. Warmup, CSV output and flat/spatial object-order plus bit-identical
world-matrix parity checks are outside measured intervals. The fixed collector
buffer is allocated and prefaulted before warmup; exceeding its capacity fails
the run instead of truncating it. Allocation failures and parity failures also
invalidate the run. This uses the production native algorithms with a recording
renderer boundary: no GPU work, draw submission, image readback, Present or
Editor frame is timed. It supplies a synthetic CPU baseline, not a representative
city/Editor or hardware performance acceptance result.

Each new evidence directory contains:

- `frames.csv`: every measured frame, phase timings, geometry work and selected
  publication/view-array byte counts. These bytes exclude allocator overhead,
  transform storage and other capture temporaries.
- `summary.json`: per-run nearest-rank p50/p95/p99, counts above twice that run's
  median, and paired median lifecycle changes. Frames are not pooled to infer
  an aggregate speedup.
- `manifest.json`: completion status, fixture/configuration, CPU/OS, compiler
  flags, Git state and source/executable/output hashes.
- `build.log` and `run.log`: build outcome and per-variant progress.

Existing output directories are rejected. Sources are hashed before/after the
build and measurement; an intervening source or executable change invalidates
the evidence. Short `-Smoke` runs and `-SkipBuild` runs set
`protocol_eligible=false`; smoke verifies the harness without satisfying the
timing protocol. A `COMPLETE` manifest with `protocol_eligible=true` establishes
only that this CPU fixture completed the timing protocol. It does not satisfy
the optimization plan's GPU, dynamic-hierarchy, multi-view or playable-scene gates.
