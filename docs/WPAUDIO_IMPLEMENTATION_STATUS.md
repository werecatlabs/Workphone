# WPAudio implementation status

10 October 2026. Baseline `f02dac9ac` plus the working-tree changes described here.
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
  a 64 MiB bound; this is a per-clip guard, not the planned aggregate cache budget.
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

The isolated project needs no graphics, Lua, Editor, content pack or sound device:

```powershell
cmake -S Tests/AudioProduction -B project_audio_contracts -G "Visual Studio 18 2026" -A x64
cmake --build project_audio_contracts --config Debug --parallel 8
ctest --test-dir project_audio_contracts -C Debug -L mandatory --output-on-failure
cmake --build project_audio_contracts --config RelWithDebInfo --parallel 8
ctest --test-dir project_audio_contracts -C RelWithDebInfo --output-on-failure
```

Mandatory `WPAudio.contracts`, `WPAudio.decode` and `WPAudio.render` pass in Debug
and RelWithDebInfo. Tests generate fixtures and cover every truncation boundary,
container/format mismatches, reordered/trailing chunks, odd padding, PCM widths,
float NaNs, a deterministic byte-mutation corpus, voice independence, pool limits,
stale/cross-context handles, clip pinning, master/mute math, pause cursors,
sample-index scheduling, loop changes, completion consumption, stereo mapping,
fractional resampling and block partition invariance. `WPAudio.device` is labeled
hardware/spike and is skipped on this host, independently of offline test results.

The main test graph additionally registers `WPAudio.scene` and `WPAudio.adapter`
when their engine targets are available. The scene fixture runs production
emitter/clone/registry logic with device-independent sounds; it cannot prove
audible scene behavior. The adapter fixture tests concrete manager gain/registry
behavior and volume-effect bypass without opening a device.

RelWithDebInfo Workphone/WPAudio and the scene/adapter test executables build
successfully in `project_x64`. Both engine tests and `WPVehicle.audio_mix` pass.
The three mandatory native suites also pass in the main project test graph.
The standalone RelWithDebInfo mandatory suites pass under MSVC AddressSanitizer
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
2. Complete M1 production migration to immutable shared clips and native-core
   voice handles. Existing C++ clones currently decode their own sample memory.
   Native C core commands require one serialized control owner per context;
   cross-thread bounded command publication and deferred completion delivery are
   not yet implemented. Core gains are immediate rather than ramped. Native C
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
   allocation instrumentation, packaged runs and long-duration soak are required.

No release gate in section 12 of the plan is marked passed by this batch.
