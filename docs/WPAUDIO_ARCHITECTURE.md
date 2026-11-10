# WPAudio architecture decision and capability inventory

Decision date: 10 November 2026. Source inventory: `96db9f5fd` plus the current
audio implementation changes. This records the offline decisions for M0 in
[the production plan](WPAUDIO_PRODUCTION_PLAN.md). Test outcomes belong in
[the implementation ledger](WPAUDIO_IMPLEMENTATION_STATUS.md); this inventory
does not turn a source path into device certification.

## Decision: one native renderer with platform output adapters

`WorkphoneAudio` owns clip decoding, sample clocks, voice state, generation
handles and block rendering. `WPAudio` remains the engine-facing compatibility
and integration layer. A platform adapter submits interleaved float output from
that renderer through one source stream for each output context. Offline capture
uses the same renderer with a memory sink. ResourceSystem owns cooking,
resource identity, manifests, package mounts and range transport.

The alternative considered was moving the portable renderer into the C++
WPAudio DLL. It would simplify access to engine objects but couple the offline
renderer and its lifetime tests to the engine build, intrusive ownership,
factories and platform DLL exports. The existing native implementation already
provides a C11, SDK-free boundary, independent test target and bounded render
work. Reusing it avoids a second implementation of cursor, scheduling and gain
semantics. Platform handles belong in private implementation objects rather than
the C public API.

This is the selected implementation destination. The current production
`WPAudioSound` still owns platform source voices. Migration remains a required
M1 task; a working offline sink does not establish that migration is complete.
The existing native transport may remain during integration but must be removed
from the Windows production path when the common output stream replaces it.
There is no permanent runtime-selection switch between two audio engines.

## Ownership, threading and compatibility

| Concern | Decision and current consequence |
|---|---|
| Immutable samples | A clip owns immutable samples; each voice pins a clip generation and owns its own cursor. Existing C++ shared content cache pins prior generations across reimport. Failed reload keeps the last usable generation. |
| Context access | Direct native context functions require exclusive ownership. The command API supports one producer/poller and one render owner. Multiple engine producers must be serialized before this boundary. |
| Output worker | Output refill runs independently of frame-driven `SoundManager::update()`. Callbacks publish buffer completion/error only. They must not load resources, log, allocate, destroy engine objects or call Lua. |
| Shutdown | Reject publication, stop/join refill and producer work, destroy/fence native output callbacks, drain acknowledgements and retirements, then destroy the context and its pinned clips. A sink's close operation must fence callbacks before releasing buffers. |
| Results | Command acceptance, application acknowledgement and terminal playback notification are distinct. Admission failure is visible. Pool/queue pressure must not silently discard terminal notifications or make failed playback appear active. |
| Compatibility APIs | Preserve `ISound` transport surfaces while replacing their implementation with independent handles. Rebuild DLL clients when C++ object layout changes. Add functions/descriptors rather than changing existing virtual tables for native-core functionality. |
| Native access | `WPAudioSound::getSourceVoice()` has no call sites in Engine, Tools or Samples at this inventory. Deprecate it when migration lands; a logical voice must not expose the shared device stream as its private source voice. |
| Platform headers | `workphone_audio_core.h` stays SDK-free. The existing conditional `IXAudio2SourceVoice` declaration is a compatibility exception pending removal. |
| Factories | WPAudio registers manager, sound and listener types. Keep the one SoundManager registry authoritative for create/add/clone/remove and plugin teardown. Foreign-manager destruction must be harmless. |
| Preview and scenes | Preview owns instances independently of gameplay. Future scene contexts retire on scene unload; persistent audio requires an explicit persistent context. Listener selection uses the selected scene listener, otherwise nonspatial playback; do not silently bind a different scene's listener. |

## Codec and asset profile

The initial profile introduces no third-party codec dependency. Source parsing
uses the in-tree RIFF validator. PCM integer conversion and IEEE float conversion
are implemented by Workphone; there is no compressed-codec version or license to
bundle for this profile.

| Surface | Supported contract |
|---|---|
| Native clip decode / direct WAV load | RIFF WAVE, mono or stereo, PCM 8/16/24/32-bit or finite IEEE float32, 8–192 kHz, nonempty bounded data; malformed containers and unsupported formats fail. |
| Current typed cooked audio | Resource type `wav`, compiler `WorkphonePcmWave`, compiler output version 2: canonical mono/stereo PCM16 WAVE converted from the validated source profile, preserving rate/layout. Finite values clip to PCM16 range, round to nearest without dither; clipping produces an import diagnostic. Runtime validates type, resource identity, compiler version, payload size and hash; subresources/dependencies are rejected by this version. Rebuild version-1 caches/packages. |
| Packaged PCM stream | Existing ResourcePayloadMount/leaf providers, sealed sparse resource snapshots, fixed PCM slots and bounded range requests. Source files are unnecessary after cooking and mounting. |
| Compressed audio | Not supported by this profile. A future codec requires a pinned dependency/license record, bounded decoder workspace, seek table contract and measured loop/seek error before acceptance. |
| Precision | PCM source frames are the seek/loop unit. Native buffered rendering resamples onto the context sample clock. Lossless PCM bytes must compare exactly before mixing; rendered float comparisons use explicit tolerance and sample positions. |

The version-2 import conversion broadens source acceptance while retaining
PCM16 cooked output. Tool-side waveform analysis uses validated source samples,
up to 4,096 bins of per-channel extrema/RMS; it is not yet persisted in resource
metadata or wired to Editor widgets. Authored events, buses and banks must
use versioned ResourceSystem resources when implemented; legacy event/project
scaffolding is not their persistence schema.

## Default budgets and settings

These are implementation defaults and hard limits, not measured shipping
performance guarantees. The production plan's standard stress workload remains
the certification workload.

| Item | Decision / enforcement boundary |
|---|---|
| Default mix | 48 kHz, stereo float, preferred 256-frame render block. Actual device rate/layout and queued frames must be reported independently. |
| Native context limits | 8–192 kHz; 1–1,024 voice slots; maximum block 8,192 frames. A desktop profile should request 128 physical voices; virtualization is later work. |
| Commands | 128 outstanding requests with eight credits reserved for stop/release. Exhaustion returns a result; polling releases credits and deferred sample references. |
| Terminal notifications | 2,048 credits reserved per playback run; start/restart admission fails when exhausted. Control polling returns credits. Generations and playback IDs distinguish retired/restarted instances. Device failure seals admission and publishes explicit terminal reasons. |
| Buffered input | 64 MiB source bound and 64 MiB decoded bound per native decode. C++ immutable sample cache has a separate 64 MiB resident payload budget and 1,024-entry limit. These are not one aggregate process cap. |
| Existing PCM ring | Eight 65,536-byte data slots and one fixed silence block. Cold snapshot bootstrap is at most two data blocks. Metadata and in-flight transport also consume scheduler-accounted memory. |
| Aggregate streaming | ResourceSystem's WorldStreamingScheduler owns admission. The plan's 16 MiB aggregate audio stream/decode target requires an audio profile accounting gate; per-stream ring bounds alone do not prove that total. |
| Live settings | Master/mute and per-instance gain/loop/pause apply through the owner at a block boundary in the selected core. Category gains remain composed with authored gain. |
| Reopen settings | Backend/device/sample-rate/layout changes require output reopen. Requested values and successfully opened endpoint values remain distinct. Failure retains explicit unavailable/error state. |
| Pool settings | Voice, queue, stream and block capacities are fixed at context creation; changing them requires a new context with defined retirement. |
| Unsupported settings | Capturing, external backends, HRTF and advanced DSP must return/report unavailable until a real implementation exists. Merely storing a property is not capability support. |

The current native adapter reports `active.sound.sampleRate`, output device and
backend after successful initialization. Endpoint selection still uses friendly
name categories. Stable endpoint IDs, negotiated-layout reporting and recovery
remain device-integration work.

## Capability and configuration matrix

| Configuration / feature | Source state and acceptance boundary |
|---|---|
| C11 core, headless offline | Independent renderer/test target exists. No platform SDK types or device are required. New offline contracts must fail on missing behavior. |
| Windows x64 / XAudio2 | Primary integration target. Existing per-sound native transport plus common-renderer output spike exist. Endpoint absence is a skipped hardware test, never successful playback. |
| Windows x64 MSVC Debug / RelWithDebInfo | Required source-matched build configurations. Record compiler/SDK and executed tests per batch rather than inheriting old binary results. |
| Minimum Windows/device baseline | Certify the first profile on Windows 10/11 x64 with a working XAudio2 stereo endpoint; older OS versions are outside this initial matrix. Specific physical endpoints and latency budgets remain lab evidence. |
| Static engine / audio disabled | CMake paths exist; fresh configuration/build and startup remain required evidence. Static declaration alone is not validation. |
| WASAPI branch | A manager initialization branch exists, but it is not a certified alternative transport. Keep outside the supported initial profile. |
| Apple / iOS / Android | Legacy output branches exist. Common-core migration and physical-device lifecycle tests remain R2; do not claim parity. |
| Linux device output | No supported production output adapter in this inventory. Offline core remains independently usable. |
| Recording | Unavailable. Start/copy requests report warnings and no capture buffer; returning an empty buffer is not a successful recording. |
| Delay/events/editor authoring | Delay processing and several event/tool actions are scaffolding. Event-file loading returns false. These features need real runtime services and observable failure/capability status before exposure. |

## Regression list and remaining gates

Offline regression coverage must include independent shared clips; single master
gain application; pause/resume cursor preservation; loop changes; stale/cross-
context handles; restart and terminal-event sequencing; exhausted queues; foreign
manager removal; 1,000 create/start/stop/release/unload cycles; shutdown with queued
starts and unpolled retirements; and no render allocation or reclamation.

Remaining source/integration risks identified by this inventory:

- `WPAudioManager::removeSound` bypassed base ownership checks by unloading and
  clearing ownership first; this batch delegates to the guarded base operation
  and adds a concrete adapter regression.
- Delay now reports unsupported and always copies bypass output; attempts to
  enable it log unavailability. Legacy sound-map loading also reports unavailable.
  Demo parameter/routing/effect/snapshot Lua actions report failure rather than
  claiming to have changed engine state.
- Legacy event/group gain propagation overwrites child gain. Move it onto the
  common composable gain model during event/mixer work.
- Native per-sound transport, frame-driven stream filling, source-voice access,
  terminal delivery and failure propagation still require production migration
  even after the common worker and core pass offline tests.

M0's baseline capture, actual device negotiation, end-to-end latency comparison
and output-device matrix require an audio endpoint. M1's production callback
races and audible transport require endpoint-backed integration. M2's clean
package playback and captured buffered/streamed agreement require both the
package/runtime path and captured output. No offline-only result closes these
gates. R2 platforms, surround, HRTF and capture remain outside the initial device
profile; none is silently removed from the full production plan.
