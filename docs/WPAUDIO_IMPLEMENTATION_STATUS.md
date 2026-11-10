# WPAudio implementation status

Updated 10 November 2026. Current source baseline `96db9f5fd` plus this batch.
The October validation below is historical unless explicitly refreshed here.
This is an implementation ledger for [the production plan](WPAUDIO_PRODUCTION_PLAN.md).
M0's offline architecture decisions are recorded; M1 and M2 are **in progress**.
Device certification and the production common-output migration remain open.

## November continuation

- [Architecture decision](WPAUDIO_ARCHITECTURE.md): selected native renderer,
  single output stream per context, compatibility/ABI migration, codec profile,
  budgets, settings and platform capability boundaries. Physical-device latency,
  capture and negotiated-output evidence remain pending.
- Native terminal notifications reserve one of 2,048 bounded credits per playback
  run. Start/restart exhaustion is explicit; stop/release/restart/natural endings
  publish exactly once with context, handle generation, playback ID and frame.
  The control thread polls independently of command acknowledgements. Master
  gain/mute commands and atomic queue/notification telemetry are implemented.
- Device failure seals command admission, acknowledges pending starts as failed,
  publishes one device-failure terminal per active run, and keeps failed renders
  silent without advancing the clock. In-flight publication during failure is
  drained by control polling; no failure-time sample reclamation occurs in render.
- `CoreOutputPump` owns a dedicated render/submission worker and three preallocated
  blocks. Both the XAudio2 probe and fake-output tests use it. Callbacks publish
  atomics only; shutdown joins the worker and fences sink callbacks before
  reclaiming buffers/context. Failure and 1,000 startup/shutdown cycles are tested.
  **WPAudioSound still uses native per-sound transport**: this worker is a tested
  migration component, not a completed production switch.
- Foreign-manager `removeSound` now respects registry ownership. Delay explicitly
  reports unsupported and copies bypass samples; legacy sound-name maps log
  unavailability. Lua demo parameter, routing, effect and snapshot actions report
  unavailable services instead of claiming engine changes.
- Existing `AudioResourceCompiler`, `AudioResourceStream`, `AudioPcmRing` and
  ResourceSystem package/range infrastructure were already present in this
  checkout. This batch extends those implementations. Compiler output version 2
  converts validated PCM8/16/24/32 and float32 source to canonical PCM16 WAV,
  preserving rate/layout, rounding without dither and reporting finite clipping.
  Old version-1 caches/packages require recooking.
- Tool-side waveform analysis returns up to 4,096 per-channel extrema/RMS bins;
  invalid inputs retain the prior result. Editor waveform widgets/persisted
  analysis metadata are still pending.
- `WPAudio.assets` now registers the previously orphaned package stream fixture.
  It exercises cooked-only mounts after deleting sources, exact streamed sample
  comparison against common-core buffered rendering, controlled residency
  pressure/silence/retry, cold cancellation, repeated superseding seeks, loop
  endpoint handling, retirement, three pinned reimport generations and malformed
  replacement. The test uses package/range IO with a simulated consumer; it does
  not capture a hardware endpoint.

## Current validation

Eight standalone mandatory suites pass in current Debug and RelWithDebInfo,
including under MSVC AddressSanitizer (19.52.37025, Windows SDK 10.0.26100.0).
`WPAudio.device` skips with endpoint absence. The standalone project also exposes
`WP_AUDIO_BUILD_DEVICE_TESTS=OFF` to exclude platform output probes from headless
builds; all eight suites also pass in that separate RelWithDebInfo configuration.
The old `Tests/AudioProduction` directory moved to
`Tests/cpp/AudioProduction`.

Current RelWithDebInfo Workphone, WPAudio, WPSQLite and the asset/adapter/scene
test executables build successfully. All three integration suites pass.
`WPAudio.assets` passes (1.2-1.8 seconds on this host), including
the real ResourceSystem compiler, source-deleted package mount, bounded range IO
and simulated PCM consumer. This is an offline package test, not a packaged-game
or hardware-output certification.

```powershell
cmake -S Tests/cpp/AudioProduction -B project_audio_contracts -G "Visual Studio 18 2026" -A x64
cmake --build project_audio_contracts --config Debug --parallel 4
ctest --test-dir project_audio_contracts -C Debug -L mandatory --output-on-failure
cmake --build project_x64 --config RelWithDebInfo --target WPAudioAssetTests WPAudioAdapterTests WPAudioSceneTests --parallel 8
ctest --test-dir project_x64 -C RelWithDebInfo -R '^WPAudio.(assets|adapter|scene)$' --output-on-failure
```

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

## October architecture spike evidence (historical)

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
M0's latency/device gates remain open. The current ownership decision is recorded
in the November architecture document linked above.

## October validation and reproducible commands

The isolated project needs no graphics, Editor, content pack or sound device.
It builds the bundled Lua interpreter for the headless Editor logic fixture;
no separately installed Lua runtime is required:

```powershell
cmake -S Tests/cpp/AudioProduction -B project_audio_contracts -G "Visual Studio 18 2026" -A x64
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
cmake -S Tests/cpp/AudioProduction -B project_audio_contracts/asan -G "Visual Studio 18 2026" -A x64 `
  -DCMAKE_C_FLAGS=/fsanitize=address "-DCMAKE_CXX_FLAGS=/fsanitize=address /EHsc" `
  -DCMAKE_EXE_LINKER_FLAGS=/INCREMENTAL:NO
cmake --build project_audio_contracts/asan --config RelWithDebInfo --parallel 8
# Put the matching MSVC bin/Hostx64/x64 directory on PATH for the ASan runtime.
ctest --test-dir project_audio_contracts/asan -C RelWithDebInfo -L mandatory --output-on-failure
```

## Outstanding work and limits

1. M0 hardware baseline capture, negotiated-output/device matrix and measured
   latency/ownership comparison remain open. Offline architecture choices do
   not substitute for those measurements.
2. M1 production migration remains: connect compatibility sounds and the existing
   streamed resource provider to core voice handles, route all output through
   the common worker, and wire terminal delivery into the engine control owner.
   The offline core/worker and production per-sound adapter currently coexist.
   Migration can be developed offline but still needs implementation and device
   verification; it is not blocked solely by unavailable hardware.
3. M2 still needs persisted waveform/import settings, authored bank grouping,
   Editor reimport workflow integration, broader stream/rate/loop matrices and
   aggregate streaming budget acceptance. PCM package/sample parity is covered;
   device captures and clean packaged-game playback remain unverified. Compressed
   codecs are outside the selected initial PCM profile.
4. M3-M7 mixer/event/spatial/Editor/music features retain their plan gates.
   M8 certification and all R2 milestones remain open. Static/audio-disabled
   engine startup, callback/race instrumentation, physical device recovery,
   listening and long-duration soak evidence must be recorded separately.

No production release gate in section 12 of the plan is marked passed by this batch.
