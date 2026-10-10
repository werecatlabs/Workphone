# WPAudio production implementation plan

Date: 10 October 2026. Reviewed checkout: `ccb890ed6`. Status: proposed implementation plan; no runtime implementation or production certification is included in this document change.

## 1. Objective and delivery scope

Make WPAudio a complete, dependable audio system for shipped Workphone games, with usable scene components and an end-to-end Editor authoring workflow. Production readiness includes correct audible output, safe lifetime and concurrency, resource cooking and packaging, predictable budgets, accessible user settings, diagnostics, documentation, and reproducible release evidence.

Start with Windows x64/MSVC, C++17, and XAudio2 output. Retain the existing Workphone interfaces and Lua authoring investment through compatibility adapters. Certify other operating systems and output layouts separately; platform branches or successful initialization do not establish working playback parity.

| Delivery | Required scope |
|---|---|
| **R1: production game audio** | Reliable buffered and streamed playback; independent playback instances; voice limits and virtualization; stereo/mono output; 2D/3D panning, attenuation, cones and Doppler; buses, sends, snapshots and core DSP; authored sound events and parameters; baseline music and dialogue; scene emitter/listener/zone components; import/cook/package/reimport; functional SoundEditor and project settings; C++/Lua controls; offline tests, device tests, profiling and release gates |
| **R2: comprehensive authoring and platform expansion** | Adaptive music, richer event graphs, localization and sequencing tools, rooms/portals and richer propagation, HRTF, surround output, advanced DSP, recording/capture, gameplay audio components, state restoration, and macOS/iOS/Android certification |
| **Separately gated extensions** | Network voice chat with transport/jitter/AEC, console SDK backends, ambisonics/object audio, convolution acoustic baking, plugin hosting, generative audio and physically based wave propagation. These need their own product requirements, dependencies and acceptance gates |

R1 is a shippable feature set, not the end of the comprehensive program. Every R2 feature has a named milestone below. Unsupported APIs must report their capability/error explicitly; no release may imply that recording, spatialization, effects or Editor actions work merely because a method or control exists.

## 2. Current checkout: evidence and gaps

This was a targeted source and build-definition review. It is not a full parser, concurrency, DSP or platform audit. Findings describe the reviewed paths, not every possible application configuration.

| Area | Source evidence | Required action |
|---|---|---|
| C++ playback | [WPAudioSound.cpp](../Engine/cpp/Source/WPAudio/WPAudioSound.cpp) reads WAV data into memory and creates XAudio2, Apple Audio Queue or OpenSL ES playback objects | Preserve useful playback behavior, then add bounded streaming, decoding, seeking, scheduling and explicit lifecycle states |
| WAV validation | The reader checks file/chunk bounds and basic format fields, but ignores the RIFF size and copies whole format/data chunks into vectors | Complete container/format validation, allocation limits, endian handling and malformed-input tests before exposing broad import/runtime decoding |
| Playback semantics | Windows pause stops a queued voice; Apple/Android `play()` resets or clears/requeues data. Windows `setLoop()` changes the stored submission descriptor | Specify resume versus restart, loop-change behavior, queued-buffer replacement and completion semantics uniformly; verify each backend |
| Master gain | [WPAudioManager.cpp](../Engine/cpp/Source/WPAudio/WPAudioManager.cpp) writes master gain into each sound and into the XAudio2 mastering voice | Apply master gain once; preserve per-instance gain and mute state. With unit clip gain, setting master to 0.5 must yield 0.5, not 0.25 |
| Resource ownership | [SoundManager.cpp](../Engine/cpp/Source/Workphone/Sound/SoundManager.cpp) owns a resource collection; WPAudioManager also declares its own sound/listener collections. `create()` and `addSound()` use different paths | Consolidate tracking, failure handling, update, destruction and reference ownership; cover every factory/resource entry point |
| Factory/plugin integration | [WPAudio.cpp](../Engine/cpp/Source/WPAudio/WPAudio.cpp) registers manager/sound factories; [Application.cpp](../Engine/cpp/Source/Workphone/Application.cpp) creates the interface manager. [EditorApplication.cpp](../Tools/cpp/Editor/src/EditorApplication.cpp) still contains FBAudio references in its static plugin path | Verify interface-to-concrete creation, static and DLL startup, exports and all Editor/sample configurations. Register event/project/listener types as required |
| C backend duplication | [Native manager](../Engine/c/Source/WorkphoneAudio/workphone_audio_mgr.c) textually includes a platform `.c`/`.m`; [native CMake](../Engine/c/Project/WorkphoneAudio/CMakeLists.txt) also lists a platform translation unit, omits the sound/event/project/parameter/listener implementations, and names an iOS `.c` while the source is `.m` | Repair explicit source lists, single backend compilation and Objective-C configuration. Audit native dispatch before declaring it authoritative |
| C sound API | [workphone_audio_sound.c](../Engine/c/Source/WorkphoneAudio/workphone_audio_sound.c) contains placeholder load/play/volume operations; it can set loaded/playing state without decoding/output | Implement real dispatch and data ownership or explicitly mark the legacy path unsupported during migration |
| Layer separation | [C++ CMake](../Engine/cpp/Project/WPAudio/CMakeLists.txt) links platform APIs directly and does not link WorkphoneAudio | Choose one final playback/mixing owner; avoid maintaining two independent production engines |
| 3D sound | [Listener implementation](../Engine/cpp/Source/WPAudio/WPAudioSoundListener.cpp) maintains listener data; inherited [Sound setters](../Engine/cpp/Source/Workphone/Sound/Sound.cpp) store pan, position and distances. Reviewed WPAudio playback does not calculate/apply spatial output matrices or Doppler | Implement audible spatialization end to end; validate coordinate, velocity and output-layout conventions |
| Events | [WPSoundEvent](../Engine/cpp/Source/WPAudio/WPSoundEvent.cpp) can start associated sounds and propagate volume/position. [Parameters](../Engine/cpp/Source/WPAudio/WPSoundEventParam.cpp) store clamped values. Manager event-file loading returns false | Preserve compatibility while adding authored definitions, independent instances, parameter bindings, completion, selection and concurrency semantics |
| Mixing/DSP | [Volume effect](../Engine/cpp/Source/WPAudio/WPAudioEffectVolume.cpp) multiplies samples but bypass writes no separate output; [delay](../Engine/cpp/Source/WPAudio/WPAudioEffectDelay.cpp) processing is empty. [Project](../Engine/cpp/Source/WPAudio/WPSoundProject.cpp) and [groups](../Engine/cpp/Source/WPAudio/WPSoundEventGroup.cpp) provide partial gain/reverb scaffolding | Build an actual graph and define buffer/channel/bypass/tail contracts; replace destructive gain propagation with composable gain stages |
| Capture/analysis | Manager recording methods are placeholders; base `Sound::getSpectrum()` returns zero-filled data; WPAudioManager `isRealtime()` always returns false | Expose accurate capabilities/status; implement analysis and later capture through separate services |
| Scene component | [AudioEmitter](../Engine/cpp/Source/Workphone/Scene/Components/AudioEmitter.cpp) exposes a sound pointer and play/stop/pause/unpause property buttons | Extend it with stable asset references, owned instances, transform updates, enable/disable/scene lifecycle and serialization. Reviewed component has no listener arbitration or spatial update path |
| Asset preview | [SoundResourceDirector](../Engine/cpp/Source/Workphone/Scene/Directors/SoundResourceDirector.cpp) loads/plays a sound resource and destroys it through the manager on unload; [PropertiesWindow](../Tools/cpp/Editor/src/ui/PropertiesWindow.cpp) has sound resource handling | Separate asset lifetime from preview-instance lifetime; integrate import settings, validation, undo and resource publication |
| Existing Editor | [SoundEditor.lua](../bin/Media/Scripts/Lua/Editor/SoundEditor.lua), opened through [UIManager.cpp](../Tools/cpp/Editor/src/ui/UIManager.cpp), already has event, parameter, mixer/effect, bank, snapshot and debug controls | Extend this editor in place. Several actions currently update Lua tables or status text; bank tracking may report loaded without a successful runtime resource. Replace each with verified service results |
| Project settings | [ProjectSettings.lua](../bin/Media/Scripts/Lua/Editor/ProjectSettings.lua) exposes backend/device/sample-rate/buffer/voice/streaming/occlusion options; reviewed apply path sets runtime master volume | Trace persisted settings through startup and runtime reconfiguration; display requested versus negotiated device settings and restart requirements |
| Lua | [SoundBind.cpp](../Engine/cpp/Source/WPLuabind/Bindings/SoundBind.cpp) binds basic playback/listener/manager APIs; [ComponentBind.cpp](../Engine/cpp/Source/WPLuaBind/Bindings/ComponentBind.cpp) exposes AudioEmitter | Add thin typed bindings for new services. SoundEditor attempts legacy `addSound2`/`addSound3` calls; replace or adapt them through the actual supported API |
| Gameplay/tests | [VehicleAudio.cpp](../Engine/cpp/Source/Workphone/Vehicle/VehicleAudio.cpp) manages engine/tyre loops; [VehicleAudioMixTests.cpp](../Tests/cpp/VehicleAudioMixTests.cpp), registered as `WPVehicle.audio_mix` in [test CMake](../Tests/cpp/CMakeLists.txt), tests target gains and smoothing | Retain this behavior as a migration fixture. Gain math tests do not establish native playback, streaming, spatialization or Editor correctness |

No audio build, test executable, hardware playback, listening session, memory check or Editor UI validation was run for this planning change. Existing binaries and generated CTest entries are not evidence of current-source production readiness.

## 3. Architecture and runtime contracts

### 3.1 One runtime owner, with a staged migration

**Recommended destination:** WorkphoneAudio owns a portable DSP/voice/scheduling core and thin platform output adapters. WPAudio is the C++17 engine adapter for resources, components, commands, diagnostics and Lua integration. This is a proposed implementation decision, not a description of the current C backend's completeness.

M0 must compare this destination against keeping the portable core inside WPAudio using a minimal offline-render and XAudio2-output spike. Record measured complexity, latency, ABI impact and migration cost in an architecture decision. Default to the native-core destination if the spike meets its gates; change the plan explicitly if it does not. Do not implement both engines in parallel as permanent alternatives.

| Responsibility | Owner and contract |
|---|---|
| Asset identity/discovery | Existing AssetDatabaseManager/catalog and directors; durable UUIDs and canonical ResourceIDs, source provenance and dependencies |
| Cook/cache/package | Existing [ResourceSystem](../Engine/cpp/Project/Workphone/ResourceSystem.md), compiler registry and compilation database; audio adds typed compilers, not another database/cache |
| Clip/event/mix data | Immutable, versioned resources pinned by generation; a clip contains data/metadata and no shared playback cursor |
| Voices, event instances and DSP | One selected audio runtime; bounded pools, frame clock, resampling, routing, virtualization and scheduling |
| Device IO | Platform adapter owns device handles and negotiation/recovery. The common renderer produces blocks for one output stream per device/context configuration |
| Game-scene state | Workphone scene audio service owns context, listener selection and coherent emitter/listener snapshots; components own handles, not backend pointers |
| Editor preview | Separate preview context using the same resources and runtime; a dedicated preview bus and instance ownership prevent gameplay/preview interference |
| Diagnostics | Audio thread publishes bounded counters/records; UI/logging consumes snapshots on non-audio threads |

Use the existing wrappers as compatibility surfaces while moving behavior behind them. New descriptor/handle APIs must distinguish `AudioClip`, `SoundEventDefinition`, `VoiceHandle`, `EventInstanceHandle`, `BusHandle`, `SnapshotHandle` and `AudioContextHandle` (proposed names). Return typed result/status/error information; invalid handles, unsupported capabilities and pool exhaustion are distinguishable.

Opaque C structures should have versioned/size-tagged descriptors and explicit destruction rules. Prefer additive interfaces over silently changing C ABI or C++ virtual tables. Where interface changes are necessary, version them and rebuild dependent DLLs, bindings, Editor and samples together. Audit exported WPAudio classes, factory registration, static linking, audio-disabled builds and platform SDK leakage from public headers.

### 3.2 Playback state and lifetime

Separate resource states (`Unloaded`, `Loading`, `Ready`, `Failed`) from playback states (`Pending`, `Playing`, `Paused`, `Virtual`, `Stopping`, `Finished`, `Failed`). Virtual voices also retain their logical pause/play state. Each request has a handle generation and context generation; stale callbacks/jobs cannot mutate a reused handle or a new scene.

- `play` creates/starts an instance; `resume` preserves its cursor; `restart` resets it; `stop` defines fade and cursor reset; `release` relinquishes the handle after completion/retirement. Legacy `play()` while paused resumes, with a distinct restart API.
- Natural completion, requested stop, stealing, missing content and device failure carry distinct reasons. Deliver one terminal notification per instance on the designated game/control thread, never inside a backend callback.
- Two emitters or previews referencing one clip must have independent cursor, volume, pitch, loop state and stop behavior. Repeated one-shots from one emitter may overlap within its concurrency limit.
- Manager ownership of immutable assets is independent of voice lifetime. Avoid manager/sound reference cycles; audit existing owner pointers and factory/global lifetimes. Remove registry entries for every creation path.
- Unload rejects new commands, cancels IO/decode, stops voices, fences callbacks, retires buffers and then destroys the device. No referenced sample memory or callback context is freed early. Destruction must be idempotent after partial initialization/failure.
- Compatible hot reload pins old data for existing instances and publishes new data atomically for new instances; an optional explicit crossfade can replace a running instance. Incompatible reload stops/restarts with a documented reason. Failed reload retains the last good resource and reports failure.

### 3.3 Threads, clocks and bounded work

Scene/game threads publish commands and timestamped spatial snapshots. Decode/IO workers fill per-stream bounded buffers. A designated audio control owner applies resource/voice/graph changes at block boundaries; the render callback consumes prepared data and runs DSP. Map these roles to the selected backend in M0; a frame-driven `update()` must not be responsible for supplying every audible block.

No file IO, decoding with unbounded work, heap allocation/free, blocking mutex acquisition, game/Lua callbacks, logging, database access or arbitrary intrusive-object destruction on the realtime callback. Use preallocated pools, bounded queues and deferred reclamation. Reserve capacity for stop/shutdown; coalesce parameter/transform changes; reject excess start requests visibly. Publish overflow/drop counts and limits rather than growing queues indefinitely.

One audio sample-frame clock schedules starts, stops, fades, loop points, musical transitions and markers. At 48 kHz a 256-frame block is approximately 5.33 ms; this is a render deadline, not a claim about speaker latency. Subdivide blocks for within-block events when sample accuracy is requested. Define game/animation/cutscene clock conversion, timestamp horizons, late-command handling, pause/time-scale rules and device-restart clock discontinuities.

Pin immutable graph snapshots until render completion. Prepare graph edits off-thread, validate routing/cycles and capacities, and swap at a boundary. Support variable device callback frame counts with bounded adaptation; reset sample-rate-dependent DSP safely when the negotiated rate changes.

Microsoft's [XAudio2 callback guidance](https://learn.microsoft.com/en-us/windows/desktop/xaudio2/xaudio2-callbacks) requires prompt callbacks and recommends transferring work to another thread; this supports the queued completion/control design. The common offline renderer must execute the same mixing/scheduling path as device playback so tests cover production processing.

### 3.4 Signal, space and user settings

Use floating-point processing with explicit sample rate, frame count, channel count/layout and interleaved/planar buffer contracts. Frame count is not sample count or byte count. Validate finite values, ranges, alignment and maximum sizes at the boundary. Decide channel maps, downmix/headroom, stereo pan law, resampler quality and denormal handling; test with known signals.

Keep authored clip gain, per-instance gain, event modulation, bus ancestry, snapshot contribution and user master gain separate. Apply each once at its owned stage. Distance/cone/occlusion affect the appropriate spatial path; sends define pre/post-fader behavior explicitly. Muting never overwrites stored gain. Smooth gain/pitch/filter changes and handle sample-rate/loop/steal discontinuities with bounded ramps. Define permitted amplification in dB separately from legacy `[0,1]` volume setters; master limiting is observable and configurable.

Document world axes, handedness, world-unit-to-metre conversion, forward/up basis, velocity in metres/second, distance curve endpoints and speed-of-sound units. Produce orthonormal listener bases, reject NaNs and handle zero/parallel axes. Teleport, scene origin shift and camera cut reset velocity history; paused/zero-delta frames cannot create Doppler spikes. Define mono point-source behavior, stereo 2D music behavior and an explicit stereo-spatial policy rather than silently interpreting all clips identically.

Project defaults and user preferences are distinct layers. Persist per-category volume, mute, dynamic-range preset, output preference, mono/downmix and spatial mode; apply them at startup and report negotiated device values. Distinguish mute, logical pause, focus loss, background suspension and disabled output. An intentional null/offline backend may advance logical playback; accidental device failure must surface a status even when the game continues.

## 4. Comprehensive runtime and authoring feature matrix

A feature is complete only when runtime processing, resource/schema support, authoring, diagnostics and acceptance evidence exist. R2 and extension rows remain unsupported until their gates pass.

| Capability | R1 implementation | R2 / extension implementation | Required authoring and evidence |
|---|---|---|---|
| Buffered playback | Shared immutable PCM clips; independent instances; play/pause/resume/stop/restart; pitch, pan, seek and cursor/duration | Richer playback regions and reverse playback only where certified | Waveform/transport; distinguish resume/restart; captured output and cursor tests |
| Import/codecs | Strict PCM/float WAV import; offline compressed-source decoding through a pinned, reviewed codec set; a selected compressed runtime stream format | Additional music/dialogue formats and platform codec profiles | Supported-format matrix, conversion report, codec versions/licenses, malformed/truncated fixtures |
| Streaming | Worker decode/IO, bounded rings, pre-roll, async cancellation, seek tables, loops and underrun policy | Adaptive buffering and prioritized large-world prefetch | Buffer health, IO/decode cost, seek/loop markers; stalled-storage and repeated seek tests |
| Voice management | Global/per-event/per-owner limits, priorities, stealing with ramps; continue/restart/pause virtualization policy | Richer listener/category budgets and cost-aware prioritization | Active/virtual voice browser, steal reason and residency; burst/exhaustion tests |
| Spatial output | Mono 3D point sources, stereo equal-power pan, distance curves, cones, Doppler and listener arbitration | HRTF/headphone mode, certified surround/layouts and multi-listener split-screen mixing | Attenuation/cone gizmos, listener view and orbit tests; layout/channel impulse tests |
| Occlusion | Budgeted physics rays outside the audio callback, masks, smoothed gain/low-pass results and fallback | Multiple rays, obstruction/material transmission, diffraction approximation and cached propagation | Occlusion rays/results and query-budget view; moving-door/wall fixtures |
| Environment | Reverb sends, simple overlapping box/sphere zones, priority/weight blending and preset transitions | Rooms/portals, ambient propagation and optional baked/convolution response | Zone gizmos, wet/dry preview, overlap/portal diagnostic scene |
| Mixer | Bus tree, Master/Music/SFX/Dialogue/Ambience/UI defaults, sends/returns, solo/mute, meter taps | Richer routing, groups/VCAs and authoring conveniences | Saved mixer topology; cycle rejection; audible routing/solo tests |
| DSP | Gain, pan, low/high-pass, EQ, compressor/ducking, limiter, delay and algorithmic reverb; defined bypass/tails | Time stretch, convolution, distortion/modulation, richer sidechains and procedural sources | Effect rack/curves; impulse, sine, bypass, tail and clipping tests |
| Snapshots | Weighted/faded mix overrides, priority and restore semantics; pause/underwater/dialogue examples | Richer automation/blend modes and snapshot stacks | Before/after mix values, conflict display and nested acquire/release tests |
| Sound events | Clip/random/sequence/layer/switch/blend nodes; typed parameters, ranges/defaults/smoothing, pitch/gain randomization, cooldown and concurrency | Nested reusable graphs, richer modulation/timelines and procedural event nodes | Event editor, live parameters/selected branches; validation and reproducible seeded tests |
| Music | Streamed tracks, crossfades, synchronized stems, beat/bar markers and scheduled transitions | Adaptive state graph, sections, stingers, tension layers and authorable transition rules | Music transport/beat ruler; frame-accurate offline transitions and long-run drift tests |
| Dialogue | Queue/priority/interrupt rules, speaker attachment, localization key and subtitle timing events; dialogue ducking | Dedicated dialogue asset/editor, alternate takes, lip-sync markers and timeline authoring | Missing-language fallback and subtitle/voice synchronization tests |
| Animation/physics/gameplay | Animation event-to-audio mapping and basic surface/impact parameter routing; migrate current vehicle loops | Reusable footstep/impact/vehicle/ambience/music/dialogue components | Mapping tables, parameter monitor and actual gameplay samples |
| Timeline | Schedule audio from cutscene time; pause/seek/stop policies and preview side-effect controls | Audio tracks, waveforms, marker editing, music-aware cutscene sequencing and offline capture | Extend existing cutscene tooling; scrub/replay does not duplicate uncontrolled one-shots |
| Analysis/profiling | Per-bus/voice peak/RMS, clipping, spectrum taps, counters and offline output capture | Loudness/true-peak analysis, longer history and mix-comparison tools | Read-only telemetry; measured signal analysis, no placeholder sliders |
| Capture | Capability reports unsupported capture until implemented | Input devices, permission handling, format negotiation, bounded buffers, monitor/record/export and device recovery | Recording panel and captured-file integrity/latency/permission tests |
| Save/replay/network | Stable event IDs/parameters and timestamped gameplay request hooks; no sample payload replication | Versioned logical-state save/replay and product-specific replicated audio events | Cursor/seed/parameter restore tests; deduplicate predicted/replayed requests |
| Platforms | Windows XAudio2 and null/offline output; explicit failure for unsupported paths | macOS AudioUnit/CoreAudio, iOS session-aware output, Android AAudio/Oboe evaluation; Linux if required | Per-platform device/lifecycle matrix, real-device profiling and listening sign-off |
| Accessibility | Music/SFX/dialogue/UI controls, subtitle events, mono option and night/dynamic-range presets | Optional sound captions/directional indicators and additional accessibility presets | Test actual bus routing, persistent settings and alternate-language dialogue |

Events and clips are different resource types. Native authored banks are Workphone packages of cooked definitions and data; do not imply compatibility with FMOD `.bank` files or `event:/` identifiers simply because current Editor sample strings use them. Optional middleware adapters need their own capability mapping, dependency and licensing decision.

## 5. Resources, import, cooking and packaging

Extend existing catalog/director/compiler infrastructure and coordinate with [the resource/asset plan](WPRESOURCE_ASSET_PRODUCTION_PLAN.md). Proposed names/extensions below must pass the ResourceID grammar and registry collision checks before becoming public formats.

| Resource | Proposed content |
|---|---|
| Clip (`.audclip`) | Source dependency, channel/layout/rate, frame count, loop/cue ranges, gain metadata, codec/quality/platform profile, resident/streaming policy, seek blocks and checksum |
| Event (`.audevt`) | Stable node/parameter IDs, typed/default/ranged parameters, clips/sub-events, routing, attenuation, concurrency, selection seed policy, markers and dependency list |
| Mixer (`.audmix`) | Stable bus IDs, parent/sends, effect descriptors, defaults, category mapping and routing validation |
| Snapshot (`.audsnap`) | Target bus/effect overrides, weights, priority/blend rules and transition defaults |
| Environment (`.audenv`) | Reverb/filter presets, zone response and later room/material/portal metadata |
| Music/dialogue (`.audmus`, `.auddlg`) | Tempo/time-signature/section/transition descriptors; localization/speaker/subtitle/marker metadata |
| Bank (`.audbank`) | Build manifest, platform/profile/version, dependency closure, resident metadata and independently streamable payload entries |

Implementation work:

1. Register typed importers/compilers/loaders; maintain durable catalog IDs and canonical ResourceIDs. Preserve source paths for diagnostics and legacy migration, not as the only persistent scene identity.
2. Validate complete container length, chunk padding/order/duplicates, valid format tags, block alignment, channel mask, frame alignment and arithmetic bounds. Validate compressed decoder output and bound allocations, duration, channels and decoder work. Reject unsupported content before publication.
3. Cook canonical PCM for short effects and a chosen compressed stream format for long music/dialogue. Compare decode CPU, quality, seek/loop precision and package size before locking the runtime codec. Commit dependency versions, licenses and build reproducibility; do not inherit the broad bundled FFmpeg surface by accident.
4. Separate source edits from derived data. Store trim/loop/normalization/downmix/resample/compression/streaming settings non-destructively. Preserve authored headroom; normalization is opt-in and reported. Waveform caches and analysis are derived artifacts.
5. Include source/dependency hashes, compiler/schema/codec versions and target profile in cache keys. Publish consistent clip/event/mix generations, keep the last good artifact on failure, and cancel obsolete jobs on project/selection/unload changes.
6. Build transitive bank/install manifests; detect missing/cyclic references, duplicate IDs and unsupported runtime formats. Support chunked packaged IO and streamable offsets without unpacking entire banks. Pin entries while voices or decode jobs use them.
7. Report progress, warnings, memory/disk sizes and cancellation. Reimport twice with changed compression, loop and streaming settings; prove the second change reaches reused resources, previews and packaged output.
8. Provide command-line validation/cooking with structured reports and failing exit codes. Test a clean-machine packaged runtime with authoring/source files absent. Add a documented migration report for legacy sound pointers, paths and sound-map/event definitions.

## 6. Game-scene components and behavior

Retain and extend `scene::AudioEmitter`. New names below are proposed. Component schemas must participate in factory registration, reflection/properties, scene/prefab save/load, duplication, undo, resource dependency discovery and thin Lua bindings. Runtime handles/cursors/meters are transient and must not be serialized as authored state.

| Component/service | Delivery | Authored properties and runtime behavior | Editor experience |
|---|---|---|---|
| **AudioEmitter** | R1 | Clip/event reference, 2D/3D mode, bus, local offset, gain/pitch, loop/region, play-on-start, enabled, pause policy, priority/concurrency, attenuation/cones, parameter overrides, occlusion/send settings; owns independent instances and follows actor transform | Asset drag/drop, event/clip picker, transport, parameter controls, attenuation/cone gizmos, missing-reference diagnostics |
| **AudioListener** | R1 | Camera/actor attachment, local basis/offset, enabled, priority and context; velocity source and teleport reset. Select exactly one effective listener per R1 context | Make-active action, active-listener badge, camera/listener gizmo and competing-listener warnings |
| **AudioZone** | R1 | Box/sphere volume, environment/snapshot reference, priority, blend distance and weight; evaluated for the selected listener | Resize/rotate gizmos, overlap/blend preview, reverb/snapshot contribution display |
| **AudioTrigger** | R1 | Trigger enter/exit/interaction-to-event mapping, filters, cooldown, once/retrigger policy and owner/detached playback choice | Existing trigger/event integration, event picker and fire/test action in an isolated preview |
| **Scene audio service/settings** | R1 | One context per scene/play session; mix reference, listener policy, distance units, category defaults, budgets and lifecycle integration | Scene defaults, current listener/context and effective budgets |
| **AudioParameterDriver** | R2 | Typed parameter mappings from speed/RPM/health/animation curves, remap/clamp/smoothing and update rate; one declared owner per target or explicit combination | Source/target picker, mapping curve and live input/output plot |
| **FootstepAudio / ImpactAudio** | R2 | Animation/contact events, surface-to-event maps, intensity/speed/material parameters, duplicate suppression and cooldown | Surface table, contact/animation debug markers and test fixtures |
| **VehicleAudio** | R2 | Reusable component adapter for existing engine/rolling/squeal behavior, RPM/load/gear/slip/surface parameters and interior/exterior routing | Parameter preview, engine blend ranges and existing vehicle sample comparison |
| **AmbientAudio / AmbientArea** | R2 | Area/point emitters, random intervals/positions, density/limits and stream-aware activation | Area gizmos, distribution preview, audition and voice-budget display |
| **MusicController** | R2 | Music graph reference, state/parameters, scheduled transition/stinger requests and persistence scope | State/section/beat display, transition preview and timeline hooks |
| **DialogueSpeaker** | R2 | Speaker/localization references, queue priority, subtitle/lip-sync routing and attachment socket | Language/take preview, speaker assignment and marker display |
| **AudioRoom / AudioPortal** | R2 | Room volume/environment and portal topology/open amount/transmission; streaming-safe references | Room/portal connection overlays, leak/invalid connection diagnostics and door preview |

### 6.1 Scene lifecycle contract

| Transition | Required behavior |
|---|---|
| Load/instantiate | Resolve asset asynchronously, register component/context generation; never autoplay solely because the Inspector queried properties |
| Enter Play/start | Start play-on-start emitters once after readiness. A late load starts only if the component is still enabled in the same play session |
| Enable/disable | Apply authored stop/fade/pause policy; listener/zone membership updates immediately at the next command boundary; disable cannot leave an unowned loop |
| Game pause/time scale | Gameplay voices pause/fade/continue by policy; UI and selected music may continue. Pitch does not automatically follow time scale unless authored |
| Transform/reparent/socket | Submit final coherent world pose/velocity after scene/animation/physics ownership resolves it; never advance a sound twice for two viewports |
| Teleport/origin shift | Reset velocity history and update spatial state without a Doppler burst |
| Stop Play | Destroy play-context voices, pending starts, snapshots and listener selection; restore authored properties and previous Editor listener/preview policy |
| Destroy/scene unload | Invalidate handles/jobs/subscriptions, stop owned instances and release resources after audio retirement. Detached one-shots transfer to the scene service with a bounded lifetime; scene unload still retires them |
| Prefab duplicate/reload | Share immutable assets, allocate fresh runtime instances, remap actor references and preserve overrides. Undo/delete must not restore stale runtime handles |
| Scene streaming | Preload dependencies by policy, arbitrate listeners across additive scenes, and explicitly designate any persistent music context; all other scene-owned audio is retired |

Register no-listener/multiple-listener diagnostics with a documented fallback: R1 2D audio continues; 3D output uses an explicit configured fallback or is muted/virtualized. Editor camera listening is an explicit preview choice and cannot silently replace the gameplay listener.

### 6.2 Integration boundaries

Animation produces stable event IDs, positions and parameters; physics provides completed contact/query snapshots; gameplay decides which event to request. Audio executes neither scene mutation nor physics queries on its callback. Bound raycasts by importance/update cadence and cache results; unavailable physics leaves dry/unoccluded sound with a diagnostic capability status.

Match [animation event and timeline semantics](WPANIMATION_PRODUCTION_PLAN.md): preview/seek must not replay uncontrolled gameplay effects, loop boundaries must not duplicate markers, and blend-weight/event ownership must be defined. C++ and Lua invoke the same services and receive typed status plus safe completion notifications. Script unload removes subscriptions and invalidates queued callbacks.

## 7. Editor workflow

Extend `SoundEditor.lua`, `ProjectSettings.lua`, the existing PropertiesWindow/resource directors, ScriptWindow integration and scene gizmo infrastructure. Follow [Editor conventions](../Tools/cpp/Editor/docs/CONVENTIONS.md): authoring UI stays in the existing Lua layer; C++17 supplies runtime services, jobs, native gizmos and thin bindings. Do not build a second C++ SoundEditor.

### 7.1 Complete user journey

1. **Import.** Drop source audio into Project Assets. Show duration, channels, rate, source/decoded size, peak level, format and warnings. Select effect/music/dialogue presets, resident/streamed policy and platform quality; import runs as a cancellable job.
2. **Inspect and audition.** Open the clip in SoundEditor; view waveform, cursor and loop/cue regions, scrub/seek, pause/resume, pitch/gain and A/B cooked/source output where supported. Preview has its own gain/context; selecting an asset does not autoplay by default.
3. **Author events.** Create a clip/random/sequence/layer/switch/blend event using asset pickers and a connected node/timeline view. Add typed parameters, ranges/curves, randomization, cooldown/concurrency and bus routing; validate missing references and cycles before save.
4. **Mix.** Open the bus hierarchy, drag routes, insert/reorder/bypass effects, edit sends and use solo/mute/meters. Create pause/underwater/dialogue snapshots and audition their transitions. Conflicts show effective values and owning snapshot.
5. **Place in scene.** Drag an audio asset onto an actor to add/configure AudioEmitter through AddComponentCmd/undo. Add a camera AudioListener and AudioZone; edit attenuation/cones/zone boundaries with gizmos and preview listener placement.
6. **Play and debug.** Enter Play, move emitter/listener, trigger events and monitor voice state, parameters, audibility, occlusion, bus level and stream health. Clearly separate temporary live tweaks from saved defaults; applying live changes to the asset is an explicit command.
7. **Save and reimport.** Save assets/scenes/prefabs, restart the Editor, and verify the same behavior. Reimport changed source/settings repeatedly with last-good retention and preview invalidation; show stale/out-of-date resource generations.
8. **Build and package.** Build banks for the selected target through ResourceSystem jobs. Show dependency/memory/streaming reports and real errors. Launch the packaged scene with no source files and verify event IDs, localized content, routing and playback.

### 7.2 Required tool behavior

| Surface | Implementation and completion criteria |
|---|---|
| Clip Inspector | Asset reference/source provenance, import settings, loop/cue editor, waveform and transport; save/undo/reload and failed import retain correct state |
| Event Editor | Actual catalog-backed lists and resource-backed parameters/nodes; runtime acknowledgements and branch/instance view. Replace built-in demo entries as the authoritative model |
| Mixer/effects | Real bus/effect handles and graph commands; persistence/undo; graph validation; bypass copies/aliases correctly and meters show signal rather than editable fake values |
| Banks | Build/load/unload jobs backed by package/runtime services; dependency and residency display; no local-table success when the runtime failed |
| Snapshots | Resource-backed authoring plus isolated audition; stack/weight/priority visibility; closing preview releases its snapshot contributions |
| Scene Inspector/gizmos | Multi-select/batch edit, prefab overrides, undo/redo, missing asset feedback, listener badges, attenuation/cones/zones/occlusion overlays |
| Project Audio settings | Supported backend/device discovery, category mapping, rate/block/voice/stream/decode budgets and quality profiles; requested versus actual settings, clear restart/reopen behavior |
| Profiler | Read-only timestamped voice/bus/stream/decode/CPU/memory/callback/device telemetry with filter/capture/export and actionable overflow/underrun reasons |
| Music/dialogue/timeline (R2) | Tempo/section/transition authoring, localization/speaker/subtitle/lip-sync tables, audio timeline tracks and frame/sample alignment preview |

Preview must stop/release on close, project switch and owner unload. Asset selection changes cancel pending work; a late result cannot start the previously selected sound. Keep preview/gameplay gains, snapshots and listeners isolated even if both use one physical device. Lua `pcall` success only means the call did not raise: use returned operation status and actual runtime state before reporting successful playback/build/routing.

Every authored mutation participates in the existing command/undo and dirty-state system. Save writes versioned resources atomically, multi-asset edits identify unsaved dependencies, and recovery preserves user edits without serializing transient playback. Record automated persistence tests plus manual Editor evidence for these workflows.

## 8. Platform, device and capture readiness

**Windows R1:** select XAudio2 as the certified output backend. Repair unsupported configuration selection so a WASAPI initialization branch cannot imply sound support when its playback path is absent. Implement endpoint selection/default-device policy, no-device status, notifications/recovery, sample-rate/layout negotiation, device-loss/error handling, suspend/resume and clean COM thread ownership. UI settings must reflect the actual running backend. Keep WASAPI as an explicitly gated alternative if a product needs it.

**macOS/iOS R2:** unify output around the selected common renderer rather than one Audio Queue per sound plus an unrelated manager AudioUnit. Fix platform macros, frameworks, Objective-C sources and device ownership. On iOS, handle session activation, route changes, interruptions, focus/background policy and capture permissions where enabled. Apple documents [route-change handling](https://developer.apple.com/documentation/avfaudio/responding-to-audio-route-changes); exercise these transitions on physical devices, not only the simulator.

**Android R2:** evaluate AAudio through Oboe against a direct AAudio adapter, including supported OS/device range, negotiated burst/rate, latency and disconnect behavior. Google's [low-latency audio guidance](https://developer.android.com/games/sdk/oboe/low-latency-audio) documents Oboe's AAudio/OpenSL ES selection and device-sensitive configuration. Keep the existing OpenSL ES path only with an explicit compatibility/certification policy; handle focus, route changes and background lifecycle.

**Capture R2:** implement an independent input service with device enumeration, explicit consent/permissions, input format negotiation, bounded queues, discontinuity timestamps and overflow policy. Recording/export/monitoring run through worker/control services; default monitoring is off to avoid feedback. Diagnose permission denial, input loss and sample-clock drift. Keep exported file writing off the callback. A loopback/offline output capture mode is separate from microphone input.

**Voice-chat extension:** capture alone is not voice chat. Require codec/packet framing, jitter buffer, clock drift, network scheduling, participant policy, gain/ducking, echo cancellation/noise handling and privacy/transport requirements before advertising that feature.

## 9. Implementation milestones and dependencies

Each milestone includes implementation, integration, documentation and its tests. Runtime, tools and content work may proceed as separate workstreams after shared contracts stabilize. Do not call a milestone complete while its Editor control is still a mock or its runtime target is skipped in CI.

| Milestone | Delivery and concrete work | Dependencies | Exit gate |
|---|---|---|---|
| **M0: audit and decisions** | Inventory factories/APIs/settings/platform paths; reproduce master-gain/resume/loop/resource-sharing cases; establish baseline audio capture; select core owner, output strategy, codec profile, schemas and hardware budgets through a small offline/XAudio2 spike | None | Source-matched Debug/RelWithDebInfo baseline, architecture decision, capability matrix and tracked regression list; no unsupported-success APIs |
| **M1: lifetime and common core** | Repair native/C++ CMake and platform source ownership; implement context/clip/voice handles, pools/queues, common offline renderer and XAudio2 adapter; unify registries, exports and teardown; add compatibility wrappers | M0 | Two voices share a clip independently; master applied once; queued destruction and 1,000 create/start/stop/unload cycles pass with no stale callback or retained object |
| **M2: assets and streaming** | Typed clip compiler/loader, parser validation, selected runtime codec, bounded IO/decode, loop/seek/pre-roll, waveform analysis, cancellation, bank manifest/packaged IO and reimport generations | M1 | Buffered/streamed captures agree within defined tolerance; repeated seek/loop/reimport/cancel and clean-package playback pass under IO stalls |
| **M3: mixer and DSP** | Buffer/layout contracts, bus tree/sends, independent gain stages, ramps, core effects/ducking/limiter, snapshot blending, meters and immutable graph swaps | M1; M2 for packaged mix assets | Signal/impulse tests, bypass/tail tests, routing/solo and nested snapshot restoration pass; graph mutation does not stall callback |
| **M4: events and scheduling** | Event compiler/runtime, instance completion, random/sequence/layer/switch/blend, parameter curves, cooldown/concurrency, voice stealing/virtualization and sample-frame scheduling | M2, M3 | Independent event instances, seeded selections, scheduled starts/fades/markers, virtual resume and exhaustion policies pass |
| **M5: scene spatial integration** | Extend AudioEmitter; add AudioListener, AudioZone, AudioTrigger and scene audio service; attenuation/cones/Doppler, simple occlusion/reverb blending, prefab/scene lifecycle, C++/Lua integration | M3, M4 | Orbit/teleport/door scenes, save/duplicate/undo, Play/Stop/pause, additive scene unload and independent emitter ownership pass |
| **M6: Editor production workflow** | Extend existing Lua tools and directors; real import/event/mixer/bank/snapshot actions, clip transport/waveform, asset drag/drop, gizmos, undo/save/reload, preview contexts, settings and profiler | M2-M5; UI work can start after M0 contracts | Entire import-to-package journey in section 7 passes with restart and two reimports; no status-only feature remains advertised |
| **M7: R1 music/dialogue/gameplay** | Streamed music, synchronized stems/beat transitions, dialogue queue/localization/subtitle events/ducking, animation/surface mapping and existing vehicle migration | M4-M6 | Gameplay reference scenes and long-duration timing/cursor tests; existing vehicle mix behavior preserved and audible through new services |
| **M8: R1 hardening/certification** | Device recovery/fault injection, package/startup modes, headless/null builds, parser fuzzing, memory/race checks, performance/soak, manual listening/Editor sign-off and support docs | M1-M7 | Every R1 release gate in section 12 passes on recorded hardware/builds |
| **M9: R2 authoring/gameplay** | Adaptive music graph, dedicated dialogue/localization/lip-sync tooling, audio timeline, parameter/footstep/impact/vehicle/ambient/music/dialogue components, versioned state restore/replay | R1 | Each new feature has asset/editor/runtime/acceptance parity and migration coverage |
| **M10: R2 acoustics/output/DSP** | Rooms/portals/transmission, HRTF, surround, richer DSP/analysis and optional convolution prototype | R1, M9 where event/tool contracts are needed | Measured quality/CPU/layout/propagation tests and listening approval per feature/profile |
| **M11: R2 platform/input certification** | macOS/iOS/Android backend migration, lifecycle/device lab, capture/record/export, platform packaging and CI | R1; can run independently of M9/M10 | Real-device playback/recovery/capture tests and published supported-format/layout/latency matrix per platform |

Critical path: M0 -> M1 -> M2/M3 -> M4 -> M5 -> M6/M7 -> M8. M9-M11 extend the certified R1 baseline and do not retroactively make untested backends supported.

### First implementation batch after approval

1. Add isolated audio contract/offline test targets and reproducible fixtures. Capture existing master-gain, resume, dynamic loop, duplicate-emitter and shutdown behavior before replacing internals.
2. Repair [native CMake](../Engine/c/Project/WorkphoneAudio/CMakeLists.txt), [C++ CMake](../Engine/cpp/Project/WPAudio/CMakeLists.txt), registration and static/DLL startup against the selected ownership decision; preserve audio-disabled/headless configurations.
3. Introduce immutable clip data plus independent voice handles behind the current ISound API. Consolidate manager tracking and remove destructive master/event/group gain propagation.
4. Add deterministic offline rendering through the same core as XAudio2 output; implement transport state/completion and callback-safe teardown before advanced effects or authoring.
5. Demonstrate one clip on two actors and an isolated Editor preview, including different gains/cursors, pause/resume, deletion and Stop Play. Only then expand streaming, mixing and event features.

These are proposed work items. This planning request does not implement them.

## 10. Validation strategy

Add isolated C/C++ test targets around the selected core and engine adapter. Proposed CTest names: `WPAudio.contracts`, `.decode`, `.render`, `.streaming`, `.mixer`, `.events`, `.spatial`, `.scene`, `.editor`, `.device` and `.performance`. Register tests through the existing CMake test structure; group hardware-dependent tests separately. Mandatory offline tests fail if the backend is absent; hardware absence is an explicit skip/status, never a successful playback assertion.

| Test layer | Required cases and evidence |
|---|---|
| API/lifetime | Failure at each init/allocation/device step; invalid/stale/cross-context handles; all factory creation paths; duplicate destroy; callbacks racing stop/unload; plugin reload; shared asset independent instances; no manager/resource cycles |
| Parsing/cooking | PCM/float/channel/rate fixtures, chunks before/after data, odd padding, RIFF/format mismatch, overflow/oversize/empty/truncated files, bad compressed data, unsupported layouts; fuzzed inputs have bounded work/memory and useful errors |
| Transport/scheduling | Zero/short/end-of-buffer clips, natural completion once, pause/resume exact cursor, stop/restart, seek at boundaries, loop changes while active, sample-accurate start/marker/fade, late commands and clock resets |
| Streaming | Worker starvation, slow/failed packaged reads, underrun silence/recovery policy, cancellation during seek/unload, looping across chunks, decoder priming/padding, virtual-to-real recovery and rate/pitch changes |
| DSP/mix | Master 0.5 produces expected amplitude once; mute restores each prior gain; group/snapshot updates do not compound gain; impulses/sines/noise verify filters, delay, reverb, dynamics and pan; in/out-of-place bypass, tails, graph cycles and sidechain ordering |
| Events/voices | Typed parameter validation, smoothing/remaps, deterministic seeds/no-repeat, branches/layers, cooldown/limits, stealing and virtual timeline semantics, graph reload, terminal reasons and no duplicate callbacks |
| Spatial/acoustics | Left/right/front/back and rotated listener, unit conversion, curve endpoints/cones, zero distance, extreme velocities/teleports, invalid basis/NaNs, zone overlaps, occlusion budget and missing physics fallback |
| Scene/serialization | Enable/disable/destroy, play-on-start before/after async load, prefab duplication/overrides, multi-select/undo, additive scenes, persistent music scope, listeners, Play/Stop restoration and Lua subscription teardown |
| Editor | Real commands/status, asset drag/drop, import progress/cancel, transport, graph/mix save/undo/restart, isolated preview, changed selection with pending work, repeated reimport and failed-last-good retention |
| Devices/platforms | No device, default change, unplug/replug, loss during playback/capture, format/layout/rate change, suspend/focus/interruption, output negotiation, failed reopen and recovery with explicit diagnostics |
| Packaging/migration | Fresh cooked-only runtime, missing/corrupt bank/version mismatch, dependency closure, language fallback, static/DLL/audio-disabled startup and legacy scene/resource conversion |
| Performance/soak | Bursts, dense emitters, streams, effect-heavy mix, repeated scene/project reload, overnight music playback, output reconnect and bounded queue/pool exhaustion; record callback timing, memory and counters |

Use generated impulses, phase-known tones, silence, channel impulses and loopable signals for reproducible offline comparisons. Record fixture, compiler/codec settings, source revision, rate/layout/block size, tolerances and expected sample indices. Use exact sample-index assertions for scheduling and documented amplitude/frequency/error tolerances for DSP; avoid brittle universal file hashes for floating-point DSP or lossy codecs.

Hardware loopback/capture tests establish signal output and end-to-end latency separately from logical states and offline processing. Manual listening verifies clicks, gaps, pumping, clipping, spatial behavior, dialogue intelligibility and music transitions. Manual Editor verification uses the exact authored/cooked sample scenes. Keep compile, link, offline tests, device output, listening, Editor, memory/races and performance results distinct in the release report.

Suggested certification sequence after targets exist: fresh Debug and RelWithDebInfo builds of the chosen core, Workphone, WPAudio, bindings, isolated tests and Editor; focused mandatory offline tests; integration regressions including `WPVehicle.audio_mix`; memory/fault checks; device/playback tests; Editor workflow; performance/soak and packaged-build verification. Do not overlap builds of the same target or treat existing binaries as source-matched validation.

## 11. Performance and observability budgets

The following are **proposed starting acceptance targets**, not current measurements or universal platform promises. M0 records a reference Windows machine/storage/device and validates or revises them before feature work. Mobile/HRTF/surround/capture need separate profiles.

| Metric/profile | Initial target and measurement |
|---|---|
| Default internal mix | 48 kHz float processing, stereo; 256-frame preferred block. Negotiate actual device block/rate and report adaptation/latency explicitly |
| Standard stress mix | 128 physical voices, 1,024 logical/virtual voices, 8 compressed streams, 16 buses and 2 reverb returns; include decode and memory limits in the fixture |
| Realtime deadline | For the standard fixture, p99.9 callback work <25% of block period and maximum <50% over a 30-minute reference run; zero deadline misses under the declared supported load |
| IO stalls | Zero underruns under normal reference storage workload. Inject 100/250/500 ms stalls to measure pre-roll/queue policy and verify bounded silence/recovery beyond supported buffering |
| Realtime allocation/blocking | Zero heap allocations/frees, blocking locks, file operations or arbitrary callbacks after warm-up; verify using instrumentation, not code inspection alone |
| Memory | Initial desktop caps: 64 MiB resident decoded clip cache and 16 MiB aggregate stream/decode buffers; separately report immutable bank metadata, voice/graph/DSP pools, decoder workspace and transient cook/preview memory |
| Voice/event capacity | Hard physical/virtual/start/command limits with visible rejection/steal counters; burst tests cannot cause unbounded growth or starve shutdown |
| Spatial queries | Start with at most 64 occlusion rays per game frame, prioritized/cached at 10-20 Hz per relevant emitter; record CPU and audibility tradeoffs and adapt to project quality profile |
| Interaction latency | Candidate wired-device target: event submission to captured output p95 <=50 ms on the reference profile; measure loopback path separately and publish Bluetooth/device-specific results |
| Long-run stability | Eight-hour mixed/streaming soak plus 1,000 scene/play/project lifecycle cycles: no crash, leaked handles or monotonic retained-memory growth; cache occupancy settles within configured caps |

Counters include active/virtual/pending/stealing voices, starts/rejections, command backlog/high-water marks, underruns, decoded/queued frames, IO/decode timings, graph/effect cost, callback percentiles/max/deadline misses, resident/stream/DSP bytes, bus peak/RMS/clipping and device recovery attempts. Capture resource/context generations and termination reasons for debugging. Telemetry sampling and export must themselves remain bounded and off the callback.

## 12. Release gates and deliverables

R1 is production ready only when all of these gates pass:

- [ ] All R1 feature-matrix rows have working audible behavior, resource support, appropriate Editor controls and mandatory tests; unsupported features report explicit capability status.
- [ ] One runtime owner and one gain/routing model are used by C++, Lua, Editor previews and packaged games; independent emitters never share mutable playback state.
- [ ] Fresh Debug/RelWithDebInfo Windows x64 builds link the required core, adapter, engine, bindings, tests and Editor; static/DLL/audio-disabled configurations work or are explicitly outside the supported matrix.
- [ ] Mandatory offline and integration tests pass; hardware tests record actual captured output. No-device and device-loss paths fail/recover according to policy without silent success.
- [ ] Lifetime, concurrency, parser fault/fuzz and memory tests pass; realtime instrumentation shows no prohibited operations, stale callbacks or unbounded queues.
- [ ] The complete Editor import -> event -> mix -> scene -> Play -> save/restart -> two reimports -> package workflow passes with undo, missing-content errors and preview isolation.
- [ ] Performance targets are met on recorded reference hardware, with shipping profile limits, overload behavior and eight-hour soak evidence.
- [ ] Reviewed listening captures and reference scenes demonstrate clean transport/loops/fades, 3D movement/occlusion/zones, category mixing, music timing and dialogue/subtitles.
- [ ] Cooked-only packaged output works without source/authoring data; dependency/codec licenses, versions, manifests and migration reports are complete.
- [ ] API/component/schema docs, Editor tutorials, sample assets/scenes, platform/capability matrix, troubleshooting and certification report are published with known limits and remaining R2 work.

Required reference scenes: (1) transport and shared-clip independence, (2) moving/orbiting 3D emitter/listener and teleport, (3) door occlusion and overlapping reverb zones, (4) mixer/snapshot pause and dialogue ducking, (5) synchronized streaming music/stems, (6) footsteps/impacts and existing vehicle audio, (7) dense voice/stream stress, and (8) Editor preview/Play/Stop/reimport/scene-stream lifecycle. Keep fixtures reproducible and included in packaged acceptance runs.

R2 features and platforms require the same applicable gates, plus HRTF/surround quality/layout evidence, room/portal propagation tests, music/dialogue/timeline workflow tests and capture/platform lifecycle certification. Do not mark all of R2 complete because its APIs compile on Windows.

## 13. Risks, sequencing and remaining decisions

| Risk | Mitigation / decision point |
|---|---|
| Two existing backend implementations obscure ownership | M0 architecture spike and decision; M1 removes duplicated lifetime/output responsibility while preserving compatibility |
| A software renderer changes latency/CPU and legacy native-voice access | Benchmark early; document/deprecate `getSourceVoice`/replacement ownership; do not expose a misleading native voice for a logical instance |
| Broad codec scope inflates dependencies and attack surface | Select a small pinned codec profile in M0/M2, fuzz/limit input and publish license/format support; add formats only with their own gates |
| Editor controls advertise more than the backend implements | Replace demo lists/table-only actions incrementally; gate controls by capabilities and real result/telemetry; retain existing Lua class/API |
| Asset reload races with voices/decode jobs/previews | Immutable generations, pinned old data, cancellation tokens and commit-time context checks; repeated reimport tests |
| Audio scheduling drifts from animation/physics/cutscenes | Explicit clock conversion, late-event policy and ownership; long-run synchronization tests and game-thread completion delivery |
| Physics queries or acoustic features consume unbounded frame time | Budget/prioritize/cache queries outside callback; certify simple occlusion/zones first, richer propagation in M10 |
| Advanced DSP/spatial features exceed mobile budgets | Platform quality profiles and measured fallback; separate M10/M11 certification |
| New component types duplicate existing triggers/vehicle behavior | Reuse trigger/event infrastructure and adapt existing vehicle mix; reserve components for reusable scene authoring needs |
| Platform SDK/build branches are stale | Explicit CMake/backend inventory, static/DLL startup tests and real-device gates; avoid claiming parity from branch presence |

M0 must close: runtime owner and migration strategy; minimum OS/device/toolchain matrix; reference hardware/budgets; source/runtime codec set; compressed seek/loop precision; event schema and compatibility; listener fallback/persistent context rules; settings that apply live versus require reopen/restart; and which R2 platforms/extensions the first product actually needs. These decisions refine implementation without reducing the R1 gates.

Estimate M0 first, then size M1-M8 from the spike and measured baseline. Track runtime/DSP, resource/build, scene/gameplay, Lua/Editor, platform and QA/audio-content ownership separately. A calendar estimate before this audit would hide substantial unknowns in native dispatch, ABI compatibility and device/tool validation.

Recommended starting priority is M0-M1: make ownership, transport, gain and teardown correct and testable. M2-M7 then build the comprehensive runtime and authoring workflows on that foundation; M8 certifies the supported production release.
