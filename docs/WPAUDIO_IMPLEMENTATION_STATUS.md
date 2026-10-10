# WPAudio implementation status

10 October 2026. Initial baseline `f02dac9ac`; continuation builds were verified
against checkout `eead5c43d` plus the working-tree changes described here.
This is an implementation ledger for [the production plan](WPAUDIO_PRODUCTION_PLAN.md),
not a production-readiness certificate. M0 and M1 are **in progress**; M2-M11 remain open.

## Delivered foundation

- `workphone_audio_core.h/.c`: additive SDK-free C11 API, explicit result codes,
  size-tagged descriptors, strict bounded RIFF validation, immutable decoded clips,
  atomically pinned sample ownership, independent voice cursors, context/generation
  handles, bounded voice pools, sample-frame scheduling, linear resampling,
  stereo balance, gain/mute, seek, pause/resume/restart/stop, dynamic loops and
  consumable terminal reasons. Rendering writes interleaved stereo float blocks.
- `WPAudioSound` uses the same validator for device resources. The supported
  initial profile is mono/stereo PCM 8/16/24/32-bit and IEEE float32, 8-192 kHz.
  RIFF/container, chunk, padding, byte-rate, block-alignment and finite float
  checks reject malformed/unsupported content. Source and decoded data each have
  a 64 MiB bound. The C++ device adapter now interns immutable clip generations
  through a shared weak cache with a 64 MiB resident sample/format-data budget
  and 1,024-entry limit. Identical validated content shares storage across
  emitters/previews, including path aliases. Existing voices pin old generations;
  changed content admits a new generation. Transient parsing memory, backend
  buffers, metadata and the candidate core's decoded clips are accounted separately.
  Failed explicit reloads retain the previous usable clip and expose
  `hasReloadError()`. A successful direct asset reload replaces that sound's
  transport; existing cloned voices keep playing their pinned old generation.
- Windows master gain is applied only on the mastering voice. Mute preserves
  per-sound gain and master changes made while muted. Initialization reapplies
  saved gain. Other existing adapters compose master and local gain at the source;
  those branches have not been tested on devices.
- One engine sound/listener registry covers resource factories and `addSound`.
  Resource destruction unloads and unregisters sounds, shutdown drains pending
  queues, failed creates return null, and weak owner references break manager cycles.
- Sound cloning and `AudioEmitter` playback use separate transports. Reassignment,
  unload and destruction retire emitter instances. Asset previews create their
  own resource/transport rather than playing and destroying a cached gameplay resource.
- Apple/Android pause resumption preserves the existing queued buffer. Windows
  disabling an active loop calls `ExitLoop`. Flushed buffer callbacks no longer
  mark a newly started voice stopped. Non-Windows behavior is source-only evidence.
- Volume-effect bypass copies separate output buffers. Recording requests report
  unsupported status in the log. Static Editor plugin references name WPAudio.
- Native CMake explicitly builds core, manager, sound, listener, event, parameter
  and project translation units. The listener now has implementations. The old
  C playback backend sources are excluded instead of being textually included and
  compiled again. Legacy C manager/sound loading returns failure rather than
  claiming playback without output. The C++ adapter links WorkphoneAudio.
- Manager, sound, listener and volume-effect exports are explicit; WPAudio's
  export definition is private to its DLL build. Engine DLL clients must rebuild:
  the sound owner and emitter layouts changed.
- Native-core publication uses a fixed 128-request SPSC queue, with eight
  admission credits reserved for stop/release operations. Starts return a voice
  handle through an acknowledgement; gain/loop/pause/resume/seek/restart/stop and
  release apply at render-block boundaries. Queue or pool exhaustion is visible.
  Clips are pinned before queued starts are published, and released references
  are retired on the polling control thread. Quiescent destruction cancels queued
  starts and drains unpolled retirement references. No clip reclamation occurs in
  render. Direct context APIs remain exclusive-owner operations.
- Native-core allocation counters verify balanced payload memory after every
  test suite, no native allocations/frees during the 128-voice render loop or
  queued start/release rendering, and reclamation only when control polls release
  acknowledgements. This instrumentation excludes XAudio2, adapters and the C++ cache.
- SoundEditor previews use typed Lua `createSound`/`destroySound` bindings and
  keep their original manager for cleanup across selection/project changes.
  Unload/finalization retire previews. Play/resume checks actual transport state;
  unavailable bank services report failure, and unsupported child-bus controls
  no longer change master mute. Other authored Editor services remain open.

## M0 architecture spike and decision boundary

The candidate destination remains the plan's native-core design. The portable
renderer is implemented once, and `XAudio2CoreSpike.cpp` feeds its blocks through
one XAudio2 source voice and a fixed three-buffer ring. Callbacks only publish
completion/error state and signal an event. The control owner renders and submits
blocks; voice destruction fences callbacks before freeing the ring and clip.

**The production C++ transport still uses its existing native voices.** Only WAV
validation is shared with the candidate core today. This staged overlap is for
the requested M0 comparison, not two certified engines. Switching production
transport is pending device output, latency and lifecycle evidence. The spike
does not implement streaming, device recovery or captured-output assertions.

The local host has an Intel Core i9-9820X and Windows 10 build 19045, MSVC
19.51.36260 and Windows SDK 10.0.26100.0. A RelWithDebInfo offline run with 128
looping mono voices, stereo 48 kHz output and 1,000 256-frame blocks measured
approximately 0.30-0.32 ms average per block. This is a simple local workload;
it measures neither callback percentiles, allocation behavior, speaker latency
nor the standard mixed-stream/DSP stress profile.

The device spike returns explicit skip code 77: `CreateMasteringVoice` reports
HRESULT `0x80070490` (no output endpoint available in this session). Therefore
M0's latency/device gates and its final ownership decision remain open.

## Reproducible validation

The isolated project needs no graphics, Editor, content pack or sound device.
It builds the bundled Lua interpreter for the headless Editor logic fixture;
no separately installed Lua runtime is required:

```powershell
cmake -S Tests/AudioProduction -B project_audio_contracts -G "Visual Studio 18 2026" -A x64
cmake --build project_audio_contracts --config Debug --parallel 8
ctest --test-dir project_audio_contracts -C Debug -L mandatory --output-on-failure
cmake --build project_audio_contracts --config RelWithDebInfo --parallel 8
ctest --test-dir project_audio_contracts -C RelWithDebInfo --output-on-failure
```

Mandatory `WPAudio.contracts`, `WPAudio.decode`, `WPAudio.render`, `WPAudio.commands`,
`WPAudio.clip_cache` and `WPAudio.editor_contracts` pass in Debug
and RelWithDebInfo. Tests generate fixtures and cover every truncation boundary,
container/format mismatches, reordered/trailing chunks, odd padding, PCM widths,
float NaNs, a deterministic byte-mutation corpus, voice independence, pool limits,
stale/cross-context handles, clip pinning, master/mute math, pause cursors,
sample-index scheduling, loop changes, completion consumption, stereo mapping,
fractional resampling and block partition invariance. `WPAudio.device` is labeled
hardware/spike and is skipped on this host, independently of offline test results.
Command tests additionally exercise 1,000 queued start/release cycles, cancellation
with the producer's clip reference released, reserved stop admission, FIFO/error
acknowledgements and a 10,001-command producer/render-thread stress exchange.
Cache tests cover generation pinning, content reuse, budget exhaustion/recovery,
reclamation and concurrent admission of identical content.
The headless Editor fixture executes the actual SoundEditor Lua methods with
mock sound services. It covers owned preview retirement, project-manager changes,
playback failure, unavailable bank/child-bus operations and repeated cleanup;
it does not exercise native bindings, widgets or audible output.

The main test graph additionally registers `WPAudio.scene` and `WPAudio.adapter`
when their engine targets are available. The scene fixture runs production
emitter/clone/registry logic with device-independent sounds; it cannot prove
audible scene behavior. The adapter fixture tests concrete manager gain/registry
behavior and volume-effect bypass without opening a device.

RelWithDebInfo Workphone/WPAudio, WPLuaBind/WPLua, Editor and the scene/adapter
test executables build successfully in `project_x64`. All six mandatory suites,
both engine audio tests and `WPVehicle.audio_mix` pass in that graph (nine passes,
one device skip). `WPLua.runtime_contracts` additionally passes with a fixture
using production LuaManager and SoundBind: typed create/destroy conversions,
independent state, failed-load cleanup, cross-manager rejection and empty final
registries are verified without device output.
All six standalone RelWithDebInfo mandatory suites pass under MSVC AddressSanitizer
(including the byte-mutation corpus and clip lifetime tests). This is not a
concurrency, leak-soak or realtime-allocation certification.

The AddressSanitizer configuration used:

```powershell
cmake -S Tests/AudioProduction -B project_audio_contracts/asan -G "Visual Studio 18 2026" -A x64 `
  -DCMAKE_C_FLAGS=/fsanitize=address "-DCMAKE_CXX_FLAGS=/fsanitize=address /EHsc" `
  -DCMAKE_EXE_LINKER_FLAGS=/INCREMENTAL:NO
cmake --build project_audio_contracts/asan --config RelWithDebInfo --parallel 8
# Put the matching MSVC bin/Hostx64/x64 directory on PATH for the ASan runtime.
ctest --test-dir project_audio_contracts/asan -C RelWithDebInfo -L mandatory --output-on-failure
```

## Outstanding work and limits

1. Complete M0 hardware spike, measured ABI/migration comparison, output-device
   matrix, codec/license choices, negotiated settings and reference budgets.
2. Complete M1 production migration to native-core voice handles and the common
   output stream. Shared immutable storage and bounded cross-thread commands are
   implemented; scene/manager compatibility wrappers still use native voices.
   Terminal-event publication is still separate from command acknowledgements;
   game-thread completion delivery must be integrated. Core gains are immediate
   rather than ramped. Native C
   legacy event/project scaffolding is not an authoritative playback path.
3. Windows enabling looping on a buffer already submitted takes effect on the
   next submission; only disabling an active loop is updated in place. Complete
   uniform transport, terminal reasons and callback-race stress in the adapter.
4. Complete emitter transform/enable/listener/scene context integration, durable
   asset identity, serialization/undo and preview buses. Independent transports
   are implemented, but audible two-actor/preview/Stop Play evidence is pending.
5. M2-M7: bounded streaming/decoders, full mixer/DSP/virtualization, 3D acoustics,
   authored events/music/dialogue, cooking/packaging/reimport and functional Lua
   Editor/project settings services remain to be implemented.
6. M8 certification and all R2 milestones remain open: device capture/listening,
   Editor workflows, static/audio-disabled matrix, race/fault injection, realtime
   platform/adapter allocation instrumentation, packaged runs and long-duration soak are required.

No release gate in section 12 of the plan is marked passed by this batch.
