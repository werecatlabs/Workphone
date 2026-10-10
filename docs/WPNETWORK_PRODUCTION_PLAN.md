# WPNetwork production implementation plan

Date: 10 October 2026. Reviewed checkout: `4593215cd`; the working tree was clean at the start of this review.

Status: implementation in progress. WPNetwork is not production certified. See [implementation status](WPNETWORK_IMPLEMENTATION_STATUS.md) for completed repairs, validation evidence and outstanding packages; the feature catalogue below is the target scope.

Scope: native `WorkphoneNetwork`, C++ `WPNetwork`, engine networking interfaces, game scene components, Lua bindings, Editor authoring and multiplayer testing, dedicated server packaging, and online-service integration boundaries.

Online-services direction selected by the user: self-hosted services with local development providers. NET-11 must deliver runnable local providers and deployable self-hosted adapters; Steamworks and PlayFab are not required providers for this implementation.

Related work: [physics production](WPPHYSICS_PRODUCTION_PLAN.md), [resource and asset production](WPRESOURCE_ASSET_PRODUCTION_PLAN.md), [animation production](WPANIMATION_PRODUCTION_PLAN.md), [Lua production](WPLUA_PRODUCTION_PLAN.md), and [graphics production](WPGRAPHICS_PRODUCTION_PLAN.md). Reuse those systems and their lifecycle contracts.

## 1. Intended outcome and release scope

Deliver networking that can support a shipped Workphone game: secure connections, truthful delivery guarantees, authoritative gameplay, replicated scene and prefab lifecycles, smooth presentation, useful authoring tools, reproducible multiplayer tests, and measurable operating limits.

Start certification with Windows x64/MSVC clients, listen servers and headless dedicated servers. Other platforms, transports and service providers require their own evidence. A successful loopback packet exchange or property round trip does not certify multiplayer gameplay.

Use two releases, with specialist extensions identified separately:

- **R1: production multiplayer foundation.** Secure direct client/server connections; reliable ordered and unreliable sequenced messaging; validated portable serialization; real connection states and time synchronization; server-issued identities; spawning/despawning, ownership, initial state and late joining; typed state/RPC APIs; transform interpolation; authoritative rigid-body and baseline character integration; C++/Lua parity for core operations; scene/prefab persistence; Editor configuration, validation, local multiplayer launcher and diagnostics; headless packaging and release tests. Include bounded spatial relevance and bandwidth budgets from the start.
- **R2: comprehensive game networking.** Lobby/room and matchmaking adapters, LAN discovery, NAT traversal and relay integration, reconnect/resume, scene streaming, richer replication and interest policies, animator/vehicle/projectile integration, prediction improvements and lag compensation, spectators, replay tools, scale certification and service operations. Host migration is an explicit opt-in profile with its own gate.
- **Separate extensions.** Fully decentralized peer simulation, cross-platform deterministic rollback/lockstep, seamless server-to-server world handoff, MMO persistence/sharding, voice/video transport and platform-specific cross-play certification. Keep extension points without making every specialist feature a prerequisite for R1.

R1 must be usable for a complete small multiplayer game. R2 must provide the broader gameplay and online workflow, rather than consisting only of additional transport accessors.

### Feature coverage

Every required feature includes runtime behavior, failure handling, diagnostics, documentation, tests and a sample. Authorable features also include serialization, Inspector support and undo/redo.

| Feature family | R1 production foundation | R2 comprehensive release | Separate extension |
|---|---|---|---|
| Topology | Offline, dedicated server, listen server, direct client; local host player uses the same authorization rules | Host migration profile, spectators and reconnect reservations | Fully decentralized authoritative peers |
| Connectivity | IPv4/IPv6, asynchronous DNS/cancellation, handshake retries, rejection reasons, heartbeat, timeout and graceful close | LAN discovery, NAT traversal, relay fallback and endpoint changes | Browser/consoles/mobile certification |
| Delivery | Reliable ordered control/RPC channels, unreliable sequenced snapshots/input, bounded queues and explicit send results | Additional independent lanes, reliable unordered where justified, bulk transfer lane | Alternative transports with independent certification |
| Protocol | Portable bounded codec, version/capability negotiation, schema/content compatibility, MTU accounting | Quantization profiles, richer schema evolution and measured compression | Cross-version live world upgrades |
| Security | Authenticated encryption, server identity verification, authenticated admission, replay protection, rate limits and authority checks | Provider tickets, moderation adapters, credential/key rotation and relay policy | Third-party anti-cheat integrations |
| Session | Distinct connection/player/object IDs, roster, server clock, ready state, join/leave and disconnect policy | Room metadata, teams, matchmaking, resumable sessions and reservations | Durable account/economy services |
| Objects | Scene object registration, prefab allowlist, server spawn/despawn, ownership epochs, complete late-join baseline | Pooling, streaming subscriptions, richer parent graphs and reconnect resync | Cross-server entity migration |
| Replication | Typed variables, change tracking, full/delta snapshots, per-client acknowledged baselines, bounded relevance | Adaptive priorities, dormancy tuning, dependency groups and large-world origin support | Massive distributed replication |
| Gameplay messages | Typed RPC/event registry, direction/permission checks, quotas, transient versus durable state semantics | Requests/results, richer batching and replay annotations | Arbitrary distributed execution |
| Movement | Timestamped transform snapshots, quaternion interpolation, teleport/reset, capped extrapolation | Adaptive buffers and richer hierarchy/local-space support | General networked animation/physics rollback |
| Physics/characters | Server simulation, remote presentation mode, input sequencing and baseline character prediction/reconciliation | Vehicle prediction, projectile rewind validation and broader movement profiles | Bitwise cross-platform physics determinism |
| Animation/presentation | Local authority gates for camera/input/UI; gameplay event hooks | Animator parameters/state, root-motion ownership, effects/audio event deduplication | Full pose streaming/voice |
| Authoring | Network components, project profiles, prefab catalog, validation, save/reopen and build-time checks | Schema authoring tools, interest visualization and migration reports | Collaborative scene editing |
| Testing/tools | Server + multiple client processes, network impairment, live object/traffic inspection, reproducible reports | Replay/scrubbing, scenario automation and service test environments | Distributed multi-region load infrastructure |
| Operations | Headless package, health/readiness, structured logs, metrics, bounded resource use and clean shutdown | Region/service adapters, allocation/draining, version rollout and recovery runbooks | MMO orchestration/persistent worlds |

## 2. Evidence from the current checkout

This is a targeted review of the active native implementation, wrappers, scene path, Editor entry point and existing tests. Findings describe inspected source behavior or a risk requiring verification; unreviewed code is not assumed absent.

Priority labels: **P0** blocks a secure/correct production session, **P1** is required production reliability or integration work, and **P2** expands comprehensive gameplay, scale or tools.

| Priority | Area and source evidence | Required action |
|---|---|---|
| P0 | [Native UDP implementation](../Engine/c/Source/WorkphoneNetwork/workphone_network.c) writes sequence/ACK fields, but sends ACK fields as zero and does not implement resend, receive ordering or duplicate suppression | Implement real delivery contracts in the selected transport; do not advertise reliability from header fields alone |
| P0 | [C++ manager](../Engine/cpp/Source/WPNetwork/WPNetworkManager.cpp) sends reliable and unreliable calls through the same native `net_send` path | Separate delivery modes and expose backpressure/failure; correct the current Inspector claim that reliable delivery is guaranteed |
| P0 | Native connection requests are accepted by address; `net_handle_connect_accept` accepts a client-side reply without matching an authenticated outstanding attempt. `connect()` ignores its password in the wrapper | Add authenticated, versioned admission and connection-attempt binding before allocating a gameplay participant |
| P0 | Native packets have a public protocol ID but no cryptographic integrity, encryption or authenticated identity | Select an established secure transport/library; add replay and resource-exhaustion protections |
| P0 | [Scene NetworkView](../Engine/cpp/Source/Workphone/Scene/Components/NetworkView.cpp) checks authority before sending, but its receive path routes by view ID and applies state/ownership without validating authenticated sender authority | Validate sender, direction, session, object generation and ownership epoch before decoding/applying gameplay state |
| P0 | Scene snapshot deserialization writes transform fields as they are read | Decode and validate a complete message before committing; a truncated rotation/scale must not leave a partially updated actor |
| P0 | `getPlayerNumber()` returns `server_peer_id`. Native clients each allocate their server in local slot 1, while the server allocates different peer slots | Introduce server-issued player IDs independent of local connection handles; test multiple simultaneous clients |
| P0 | Windows native `SOCKET` is converted to `int` and stored in `NetContext::socket_handle` | Preserve the platform socket type through an opaque implementation or pointer-width handle; certify x64. Microsoft's [Winsock type documentation](https://learn.microsoft.com/en-us/windows/win32/winsock/socket-data-type-2) specifically warns about `SOCKET`/`int` assumptions |
| P1 | Native connect sends one request; peer timestamps are stored but reviewed update code has no retry/idle timeout scheduling. Disconnect removal does not consistently reset client connection state or emit a local terminal event | Implement a bounded connection state machine, retry/backoff, heartbeat and one terminal notification per attempt/session |
| P1 | Native receive drains until `recvfrom` stops; the 256-entry event ring silently drops on overflow. C++ `setNetIterations()` limits callback dispatch, not socket receive work | Bound receive, decode and dispatch separately; reserve control capacity and report all pressure/drop conditions |
| P1 | Native address handling is IPv4-only with legacy synchronous DNS; `net_address_to_string()` accepts a size but uses `sprintf`; send allows nonzero size with null data | Add dual-stack address types, cancellable DNS, length-safe formatting, checked inputs and explicit OS error handling |
| P1 | Native clock uses `clock()` and a 32-bit millisecond result; wrapper `getServerTime()` returns local time and `getPing()` returns zero | Use an injectable monotonic clock, measured RTT/jitter and a real synchronized server timeline |
| P1 | [Factory entry points](../Engine/cpp/Source/WPNetwork/WPNetwork.cpp) ignore configuration paths; manager construction sets a role without starting a session. Reviewed [Application](../Engine/cpp/Source/Workphone/Application.cpp)/[ApplicationManager](../Engine/cpp/Source/Workphone/System/ApplicationManager.cpp) paths do not establish an explicit network update owner; shutdown calls inherited `unload()` while native closure is in the manager destructor | Define explicit configure/start/update/stop/unload behavior and engine scheduling; prove startup and teardown end to end |
| P1 | [WPNetworkStream](../Engine/cpp/Source/WPNetwork/WPNetworkStream.cpp) clears input in `setData()` with its assignment commented out; packet and both stream implementations copy host representations | Repair the concrete wrapper, unify one portable codec and test both public implementation paths |
| P1 | [WPNetworkView](../Engine/cpp/Source/WPNetwork/WPNetworkView.cpp) has empty serialization/ownership-request methods, while scene NetworkView implements a separate partial path. Player types are data holders; [WPNetworkListener](../Engine/cpp/Source/WPNetwork/WPNetworkListener.cpp) callbacks are empty | Consolidate canonical runtime behavior, preserve adapters deliberately, and distinguish player/listener/stream services from actual attachable components |
| P1 | Scene NetworkView sends position/Euler rotation/scale with a sequence and frame-delta rate accumulator; it has no timestamped presentation buffer, spawn catalog, initial-state barrier or session object registry in this path. `RemoteProcedureCall` is declared but not dispatched by this component | Complete lifecycle, registry, typed RPC and snapshot scheduling before adding more state flags |
| P1 | [NetworkListener](../Engine/cpp/Source/Workphone/Scene/Components/NetworkListener.cpp) usefully snapshots listeners and resets packet cursors, but broadcasts every packet to every listener; component listeners retain raw owner pointers | Preserve independent parsing while introducing routed dispatch and safe subscription lifetimes; test concurrent unload and callback reentrancy |
| P1 | [ActorWindow](../Tools/cpp/Editor/src/ui/ActorWindow.cpp) already adds/selects NetworkView and shows network properties; addition bypasses the existing command path. Reviewed play/stop jobs have no networking process/session lifecycle | Extend this entry point with undo, validation, project settings, session launch and guaranteed cleanup |
| P1 | [Lua NetBind](../Engine/cpp/Source/WPLuabind/Bindings/NetBind.cpp) exposes packet/manager/view operations, but manager listener multiplexing is not exposed there. [RacingNetworkTransport.lua](../bin/Media/Scripts/Lua/Game/Racing/Racing/Race/System/RacingNetworkTransport.lua) already reserves user message IDs 64/65 and has epoch/sequence/authorization policies | Preserve application IDs and useful racing contracts; supply real subscription/lifecycle APIs and test concrete bindings |
| P1 | [ComponentTestsNetwork](../Tests/cpp/UnitTests/ComponentTestsNetwork.cpp) covers scene stream values, player storage, listener management and view properties/attachment. [NetworkServerTests](../Tests/cpp/UnitTests/NetworkServerTests.cpp) and [NetworkClientTests](../Tests/cpp/UnitTests/NetworkClientTests.cpp) contain no test bodies | Add independent native/wrapper, adversarial and multi-process tests; existing coverage does not establish gameplay replication |
| P1 | [Native CMake](../Engine/c/Project/WorkphoneNetwork/CMakeLists.txt) includes unrelated graphics definitions. [C++ CMake](../Engine/cpp/Project/WPNetwork/CMakeLists.txt) combines target links with Windows `.lib` names and `WPPhysics` link-directory references | Make dependencies target-based, configuration-correct and suitable for standalone headless tests; validate shared and static configurations |

### Verification actually performed

Read the sources above and checked the configured test inventory:

```powershell
ctest --test-dir project_x64 -C RelWithDebInfo -N -R '(Network|network)'
```

Result: **0 matching registered CTest tests** in this build directory. This is an inventory check, not a passing test run; component tests may exist inside the monolithic UnitTests executable without individual CTest registration.

No binaries were rebuilt, no network tests were executed, and no Editor, multi-process, WAN, security, performance or package validation was performed for this planning change. Existing DLL/PDB files do not establish source-to-binary correspondence.

## 3. Architecture and ownership

Keep the public Workphone networking boundary and scene integration. Replace incomplete implementation behind it where needed; avoid creating a second gameplay networking stack for Lua or the Editor.

```text
Editor profiles / C++ gameplay / Lua gameplay
                  |
       NetworkSession + scene components
                  |
 object registry / spawn service / replication / RPC / relevance
                  |
   INetworkManager compatibility facade + versioned control API
                  |
       selected transport + platform sockets/security
                  |
     local loopback, direct IP, or optional relay/provider
```

1. **One transport implementation per connection.** Transport owns connection handles, packet protection, delivery, congestion response and transport statistics. Replication does not implement a second ACK/retransmission system for the same messages.
2. **One session authority.** Server assigns players, objects, ownership and scene transitions. A listen server uses the same validated command path for its local player. Client identity never comes from a claimed packet field or an IP address alone.
3. **Distinct identifiers.** Separate authenticated principal, connection handle with generation, session player ID, session epoch, runtime object ID/generation, ownership epoch and durable scene/prefab identity. Document serialization width and reuse rules for each.
4. **One simulation clock.** Integrate with the engine's fixed simulation schedule and WPPhysics plan. Start with 60 Hz simulation and 20 Hz snapshots as configurable profiles; input transmission and render rate remain distinct. Networking must not step physics a second time.
5. **One mutation boundary.** The I/O owner publishes bounded immutable events; session/scene changes and callbacks execute on the game/application task at defined tick boundaries. Socket callbacks do not mutate actors, Lua, UI or physics directly.
6. **One presentation owner.** Rendering interpolates committed server states. Physics, animation, local prediction and remote interpolation have explicit transform ownership, including camera/hierarchy sampling.
7. **One routed message service.** Decode a bounded authenticated envelope once, route to session/object/component/schema handlers, and provide immutable message views or independent cursors. Keep legacy packet listeners as an explicit application adapter.
8. **One authoring and resource path.** Persist validated configuration and durable scene/prefab references through current Properties, scene and asset services. Session handles, current owners, snapshot buffers and secrets are runtime data.
9. **Truthful capabilities and errors.** Requested features either execute, return a structured unsupported/error result, or use a named supported fallback. Empty RPC/serialization methods and successful no-ops cannot pass a feature gate.
10. **Deliberate compatibility.** Keep `INetworkManager`, `IPacket`, `INetworkView` and existing message IDs through adapters where practical. Use additive/versioned interfaces for richer descriptors and results. ABI changes require version bumps and rebuilding consumers; wire compatibility is negotiated separately.

### Transport decision before broad feature development

NET-00 must compare two concrete implementations behind the same conformance harness:

- Harden/complete native WorkphoneNetwork with an established security implementation, accepting ownership of reliable delivery, congestion response, socket portability and long-term maintenance.
- Adapt a suitable established game transport behind the Workphone facade. A candidate is [Valve GameNetworkingSockets](https://github.com/ValveSoftware/GameNetworkingSockets), whose documented scope includes reliable/unreliable messages, fragmentation, encryption, IPv6 and multiple traffic lanes. It does not supply Workphone entity replication or compression. Its open-source transport does not automatically provide Steam authentication, signaling or relay services.

Prefer the adapter route if the experiment meets licensing, offline build, dependency, performance, platform and packaging requirements. Retain native UDP as a development/reference backend only if useful; certify one shipping default first. Do not maintain two complete production transports without a concrete product need.

The decision record must name a pinned version, dependencies, security update owner, supported capability matrix, deployment requirements and migration path. A library choice is not itself security certification. If a custom native transport remains the default, its congestion behavior and packet sizing must follow a reviewed design informed by [RFC 8085 UDP usage guidance](https://www.rfc-editor.org/rfc/rfc8085.html).

## 4. Transport, wire format and security contracts

### 4.1 Lifetime and connection states

Provide explicit configuration, start, connect, cancel, disconnect, drain, stop and unload operations. Constructors allocate inert objects; authoring a component never opens a socket.

States: `Stopped -> Starting -> Listening` for a server, and `Stopped -> Resolving -> Connecting -> Authenticating -> Synchronizing -> Ready` for a client. Both support `Closing -> Stopped` and terminal failure with a typed reason. Reconnection starts a new attempt; `Ready` means the gameplay baseline is committed, not merely that a UDP reply arrived.

- Retry handshake messages with bounded backoff and cancellation; match endpoint, attempt nonce and authenticated transcript. Reject unsolicited accepts and stale attempt replies.
- Track heartbeat, idle timeout, peer drain and close deadlines using monotonic time. Keep server listening state separate from an individual player's connection state.
- Reference-count platform startup safely across managers/contexts. Make repeated start/stop, failed bind, destructor-after-unload and multiple local sessions safe.
- Use platform-correct socket types; report would-block, truncation, unreachable and fatal errors distinctly. Bind policy must avoid unintended port sharing and support ephemeral local test ports.
- Ensure one ordered terminal event per attempt/connection, including locally initiated kicks/closes. Invalidate generations before resources can be reused.

### 4.2 Delivery, channels and flow control

| Channel | Proposed behavior | Examples |
|---|---|---|
| Session/control | Reliable ordered, high priority, reserved capacity | Admission, scene readiness, spawn/despawn, ownership |
| Gameplay RPC | Reliable ordered per declared lane, quotas | Interact request, authoritative result, inventory command |
| Input | Unreliable sequenced, limited redundancy and input acknowledgments | Character/vehicle input frames |
| Snapshot | Unreliable sequenced; discard obsolete pending state | Transforms, velocities, replaceable state deltas |
| Bulk | Reliable throttled, independent of gameplay ordering | Initial baseline chunks and approved auxiliary data |
| Cosmetic event | Unreliable sequenced or bounded reliable according to schema | Effects and noncritical sound triggers |

Channels are proposed logical contracts; map them to the selected transport's actual guarantees. Do not claim lane independence if the backend serializes all traffic behind one blocked ordered stream.

- Define reliable delivery as ordered, duplicate-free message delivery within a live connection, or explicit connection/message failure. It is not exactly-once business execution across reconnects; requests with durable effects need idempotency keys and server transaction policy.
- If implemented natively, specify ACK windows, wrap-safe sequence comparisons, retransmission timeouts, duplicate suppression, receive windows and bounded reassembly. Authenticate control/ACK data through the selected security layer.
- Bound outgoing bytes/messages, receive bytes/datagrams, reassembly memory and callback work per peer and globally. Reserve capacity for control; overload causes backpressure, state coalescing or explicit disconnect, never silent loss of a required lifecycle event.
- Return `Accepted`, `Backpressure`, `TooLarge`, `NotReady`, `Unsupported`, `Cancelled` or typed failure. Acceptance means queued locally, not delivered remotely. Legacy void methods log/report through a compatible error channel.
- Pace and adapt sends to congestion, observed loss and queue age. Retransmissions and baseline traffic share the bandwidth budget. Use fairness across peers so one slow receiver cannot stall the session.
- Define payload versus datagram limits separately. Start with a conservative **1200-byte total UDP payload budget including transport/security headers** where the backend allows it, then certify path behavior. The current 1200-byte application limit plus header is a different quantity. Avoid IP fragmentation; use bounded transport-level fragmentation only for approved larger reliable messages.

### 4.3 Portable serialization and versioning

Replace raw host memory copies with one codec used by `WPNetworkPacket`, `WPNetworkStream` and scene `NetworkStream`.

- Specify integer widths, byte order, Boolean representation, UTF-8 strings, explicit float representation and quaternion encoding. Do not put `sizeof(real_Num)`, native structs, pointers or reflection memory layouts on the wire.
- Implement checked readers/writers with remaining-byte checks, overflow-safe arithmetic, bounded string/array/nesting counts, null-pointer validation and capacity limits. Prevent untrusted sizes from driving allocations or decompression.
- Decode into temporary validated values, then commit atomically. Reject NaN/Inf, impossible scales/velocities and out-of-range values according to the message schema. Isolate failure to the offending message/peer with rate-limited diagnostics.
- Negotiate protocol major/minor, features, game build compatibility, replication schema fingerprint and scene/prefab catalog hashes. Reject incompatible major versions clearly; unknown optional fields can be skipped only under an explicit length-delimited contract.
- Keep transport connection metadata separate from the gameplay envelope. The envelope includes kind/schema, bounded length, session epoch, relevant object generation/ownership epoch, tick and message sequence. Authenticated sender context is supplied by the transport, not accepted from the payload.
- Preserve `UserMessageBegin = 64` and existing racing message allocations during migration. Version or adapt old application payloads; do not silently reinterpret saved scenes or old packets.
- Produce golden byte fixtures and cross-language round trips. Schema changes must regenerate fingerprints and compatibility tests reproducibly.

### 4.4 Security and authority

Internet release requires an established authenticated-encryption implementation, peer/server authentication and reviewed key/credential handling. If the native route uses DTLS, integrate a supported implementation of the protocol defined in [RFC 9147](https://www.rfc-editor.org/rfc/rfc9147.html); DTLS protection does not replace application delivery, player authentication or gameplay authorization.

- Verify server identity using a configured trust/pinning/provider policy. Authenticate clients using short-lived admission tickets or a documented secure private-session credential flow. Do not send plaintext passwords or persist secrets in scenes/projects.
- Use secure random generation, unique connection/key epochs, replay windows, authenticated negotiation and library-supported key rotation. Derive authorization context only after authentication succeeds.
- Rate-limit unauthenticated work, challenges, connection attempts, RPCs, spawn requests, bytes and malformed messages. Bound half-open state and use address validation/anti-amplification controls before large replies or baselines.
- Authorize each command against current role, ownership epoch, target visibility and game rules. Treat client movement, inventory, damage, score and race outcomes as requests/inputs for validation.
- Make `AnyPeer` an explicit development/trusted-session policy; secure shipping profiles reject it for authoritative state unless an audited rule permits it. Ownership grants come from the server and are observable events.
- Disable blind global relay in secure profiles. Forward only validated messages with authorized recipients, quotas and original authenticated attribution.
- Include graceful/hard kick behavior, reconnect eligibility and moderation hooks. Network identity/authentication is separate from anti-cheat and from durable account/economy services.
- Redact credentials, tickets, keys and unnecessary personal/address data from logs, captures and crash reports. Development loopback credentials must be scoped to that run and excluded from shipping packages.

## 5. Session, scene and replication contracts

### 5.1 Player and object identity

Create one authoritative roster with unique player IDs independent of local socket slots. `NetworkPlayer` remains a session record implementing `INetworkPlayer`, rather than becoming an actor component by inheritance accident. Local/master/server status is derived from the session and cannot be changed by an untrusted property message.

For authored actors, identify a network object using durable scene asset identity, scene instance identity and actor identity. For dynamic actors, the server allocates runtime object IDs with generations and a session epoch. Prefab asset IDs identify templates, not individual spawned instances.

The registry validates uniqueness, tracks parent/scene/owner/component schemas and offers bounded lookup. Duplicate, paste, prefab instantiation and scene duplication must remap authored identity and internal references consistently. Reusing a runtime slot must never accept an old queued packet for the previous object.

### 5.2 Spawn, despawn, ownership and pooling

- Register approved network prefabs by stable asset ID and cooked schema/content hash. The server chooses a catalog entry and validated initial data; incoming packets never select arbitrary file paths, script source or types.
- Validate spawn requests against permissions, quotas, scene readiness and resource availability. Instantiate on the scene task, resolve dependencies, then publish one `OnNetworkSpawn` callback when initial state is complete.
- Carry spawn generation, parent reference, authority policy and initial component state. Resolve missing parents with a bounded dependency queue and deadline; handle despawn-before-spawn and cancellation explicitly.
- Treat despawn as an ordered authoritative lifecycle operation with tombstones or equivalent generation rejection. Remove subscriptions and pending state before releasing actors. Define disable, unload and despawn as distinct operations.
- Ownership transfer is a server transaction with an ownership epoch, effective tick, reason and policy for disconnect/reassignment. Reset input/prediction baselines at the handoff; old owners lose write permission immediately at that tick.
- For pooled objects, issue a new runtime generation and reset replicated variables, sequence tracking, interpolation/prediction buffers, script hooks and subscriptions before reuse. Do not let memory reuse imply network identity reuse.

### 5.3 Initial state, late joining and scene transitions

Use a coherent snapshot barrier:

```text
authenticate + negotiate content/schema
  -> load approved scene/assets
  -> exchange scene/catalog readiness
  -> transfer roster + object manifest + initial state at tick T
  -> resolve parents/references and commit baseline
  -> acknowledge baseline/readiness
  -> apply queued post-T lifecycle/state messages
  -> enter Ready and enable gameplay input
```

Bound bytes/time/object counts for initial synchronization. Freeze or version a baseline while the live game continues; record post-baseline spawns, despawns, ownership and state changes so the joining client cannot revive an already destroyed object. Abort cleanly on missing assets, incompatible content, disconnect or scene cancellation.

Scene travel is server initiated with per-client loading state and deadlines. Support single-scene travel in R1; additive streaming and relevance subscriptions follow in R2. Preserve only explicitly persistent session/player actors. Treat scene-instance and world-origin changes as epochs that invalidate unsuitable history. Reconnect resumes only after authentication and content validation, then revalidates/resends state; it does not reuse an address as proof of identity.

### 5.4 Replicated state, RPC and relevance

- Introduce a typed `NetworkBehaviour`/replication descriptor model with stable component/field IDs, write permissions, serializers, validation, change callbacks, update group and initial/default values. Type names below are proposed APIs, subject to NET-00 compatibility review.
- Distinguish persistent state (door open, score, inventory), transient events (play effect), commands (request interaction) and results. Late join receives current durable state; old transient events are not replayed accidentally.
- Maintain **per-client acknowledged** baselines and field versions for deltas. Never encode against the last sent snapshot if it may have been lost. Recover with a full keyframe after missing/expired baselines.
- Batch relevant objects within the byte budget; use quantization, change thresholds, periodic refresh and dormancy with wake rules. Measure compressed size and CPU cost before enabling compression.
- Register RPC schemas explicitly with ID, direction, target rule, allowed caller role, argument limits, delivery policy, rate and timeout. Reject unknown IDs/signatures; do not invoke arbitrary reflected methods or evaluate script text from the wire.
- Keep object lifecycle/control ordered relative to relevant state, even across channels. Gate state/RPC application on a committed spawn generation and ownership epoch; expire any buffering with hard limits.
- R1 relevance supports bounded all-visible mode for small scenes plus a measured spatial grid and explicit owner/team/always-relevant rules. R2 adds scene/room/portal and dependency policies, distance priorities and dormancy tuning.
- Relevance exit hides/unsubscribes an object with clear lifecycle semantics; reentry sends a fresh baseline. Prevent stale hidden state from resurrecting actors. Relevance also limits information exposure, although it does not replace authorization.

### 5.5 Time, interpolation, prediction and lag compensation

Publish server tick/time, RTT, jitter, offset estimate and confidence. Slew the estimated timeline without making it run backward. Test clock drift, suspend/resume, long uptime and sequence/timer wrap using injected time.

Remote transform snapshots carry tick/time, pose, optional velocity and discontinuity markers. Use quaternion interpolation, an adaptive but bounded buffer, capped extrapolation and clear freeze/recovery behavior. Teleports, reparenting, ownership changes and scene epochs reset unsuitable history. Define local/world space and scale policy; do not apply stale parent-relative state after hierarchy changes.

R1 character prediction records input sequence/tick, sends bounded redundant input history, receives authoritative state plus last processed input, restores state and replays remaining inputs. Separate visual smoothing from authoritative collision state. Validate input age/rate and movement limits; suppress duplicate effects during replay. Prediction supports a defined movement profile and does not claim deterministic general physics.

R2 lag compensation keeps a bounded server history for approved hit queries. Clamp requested rewind to a published window, validate client timing against the synchronized clock, and query historical collision representations without mutating the live simulation. Vehicle and more complex physics prediction receive separate correction/stability tests; cross-platform deterministic rollback remains a separate extension.

## 6. Game scene components

Extend `scene::NetworkView` as the compatibility-facing network identity/authority component. Consolidate duplicated `WPNetworkView` behavior into the canonical session/object path; migrate its public calls through adapters. Factor new concerns into focused components instead of expanding one class into the session, transport and gameplay controller.

`scene::NetworkPlayer`, `scene::NetworkStream` and `scene::NetworkListener` currently implement networking interfaces rather than `Component`. Preserve them as records/serialization/dispatch services and expose actor-facing behavior through explicit components.

| Component or service | Release | Authoring properties | Runtime responsibility and acceptance evidence |
|---|---|---|---|
| `NetworkSession` component facade + session service | R1 | Project/profile reference, offline/client/listen/dedicated mode, scene/prefab catalog, join/start policy | One session service per world; inert in Edit; start/cancel/stop and ready/failure hooks; repeat play/stop without sockets or callbacks leaking |
| Existing `NetworkView` / network identity | R1 | Durable identity, authority policy, ownership-request policy, replication enabled, profile | Register actor identity, expose read-only runtime IDs/owner, route typed components, server-granted ownership; test duplicate IDs and forged/old-owner messages |
| `NetworkTransform` | R1 | Position/rotation/scale channels, local/world space, rate, thresholds, quantization, interpolation/extrapolation, teleport limits | Timestamped presentation with one transform owner; smooth two-client movement, hierarchy correctness and discontinuity recovery |
| `NetworkBehaviour` | R1 | Registered schema, replicated fields, RPC permissions, update group | Typed custom state/RPC hooks for C++/Lua; initial state before callbacks; atomic validation, change notification and safe unsubscribe |
| `NetworkSpawnManager` facade | R1 | Approved prefab catalog, quotas, player prefab, disconnect/despawn policy | Server spawn/despawn registry, dependency resolution and late-join replay; reject unavailable/unapproved prefabs and stale generations |
| `NetworkSpawnPoint` | R1 | Team/tag, priority, occupancy rule and spawn transform | Server selects validated spawn positions; collision/occupied fallback and late-join/respawn tests |
| `NetworkPlayerController` | R1 | Input action mapping, player actor association, team/ready defaults, local presentation policy | Bind roster player to actor; activate local input/camera/UI only for that player; prove distinct identities with at least three clients |
| `NetworkScene` facade | R1/R2 | Scene/catalog reference, persistent actor policy, load timeout; streaming groups in R2 | Scene readiness barrier, cancellation and transition cleanup; no callbacks to unloaded scene actors |
| `NetworkInterest` | R1/R2 | Grid/distance, team/owner/always-relevant, priority, update range; dependency/room rules in R2 | Per-client relevance sets, fresh baseline on reentry, bounded candidate work and information filtering |
| `NetworkRigidbody` | R1 | Authoritative/predicted/remote mode, velocity channels, sleep/teleport thresholds, correction policy | Server body owns simulation; remote bodies follow a documented kinematic/query/presentation policy; no competing transform writers or double simulation |
| `NetworkCharacter` | R1 | Supported movement profile, input rate/history, prediction/reconciliation limits | Baseline grounded locomotion/jump prediction, moving-platform behavior and visual correction; test latency/loss, replay side effects and authority |
| `NetworkAnimator` | R2 | Parameter/state schema, trigger policy, layer settings and root-motion authority | Replicate semantic state/timing and deduplicated triggers through existing animation systems; joining mid-transition yields a valid pose |
| `NetworkVehicle` | R2 | Existing vehicle component/profile, input/state schema, ownership/seat policy, correction settings | Extend current vehicle/racing systems; synchronize inputs, authoritative chassis/wheel presentation, race state and ownership safely under loss |
| `NetworkProjectile` / lag-compensated interaction adapter | R2 | Approved prefab, prediction mode, spawn reconciliation and rewind policy | Authoritative hit/damage validation; predicted effect reconciliation and no duplicate projectiles/hits |
| `NetworkLobby` facade | R2 | Provider/profile, room filters, team/ready and host/start policy | Lobby-to-session state machine, service cancellation/failure and incompatible-build rejection; reusable UI hooks |
| `NetworkEventBridge` | R1/R2 | Registered gameplay event mapping, recipient policy, reliability, duplicate policy | Bridge approved interaction/effect/audio events to existing event systems; never network arbitrary Editor/UI objects or arbitrary script functions |
| `NetworkDiagnostics` development component | R1 | Overlay enabled, capture/profile reference, selected connection/object | Show state/traffic/corrections with bounded overhead; safe attach/remove and automatic exclusion or disabling in shipping builds |

### Universal component requirements

- Register actual components with the existing factory/type system, component palette and scene serializer. Extend [WorkphonePlugin](../Engine/cpp/Source/Workphone/WorkphonePlugin.cpp), [TypeManager](../Engine/cpp/Source/Workphone/Memory/TypeManager.cpp) and binding paths consistently.
- Define attach/load/enable/play/pause/stop/disable/unload/destroy behavior. Registration is idempotent and independent of whether the session or actor loads first. Rebind safely after manager/session replacement.
- Use generation-checked weak targets or revocable subscription tokens for queued work; remove all callbacks before destruction. An old listener snapshot cannot invoke a destroyed raw owner.
- Save only authoring configuration and durable references. Display runtime object/player IDs, owners, RTT and authority as read-only diagnostics. Never save a current remote owner's ID as next session's assignment.
- Provide migrations for current `viewId`, `ownerId`, authority, delivery and transform flags. Preserve intended authored policy, convert manual IDs to registry identities, and report ambiguity/collisions rather than guessing.
- Support component dependency validation and sensible presets. A replicated rigid body without a compatible transform/motion policy must fail preflight with a direct fix action.
- Verify clone, prefab override, duplicate/paste, save/reopen, undo/redo, asynchronous scene load, scene unload and repeated play/stop for each authorable component.

## 7. Editor workflow

Build on the existing **Configure Networking / Edit Network View** actor action and Properties metadata. Use the Editor command system for addition/removal and configuration changes, including [AddComponentCmd](../Tools/cpp/Editor/src/commands/AddComponentCmd.cpp) and [ModifyPropertyCmd](../Tools/cpp/Editor/src/commands/ModifyPropertyCmd.cpp).

### 7.1 Intended authoring-to-play journey

1. **Configure the project.** Project Settings > Networking selects the certified backend, development/shipping profiles, game/protocol/schema versions, tick/snapshot rates, bandwidth/capacity limits, authentication provider, default scene and prefab catalog. Secrets are referenced from environment/secure provider configuration, with separate local development credentials.
2. **Create a networked scene.** Add a NetworkSession facade and spawn manager through a template/wizard. Select player prefab and spawn points. Existing offline scenes remain playable with networking disabled; editing never connects to a service.
3. **Mark actors and prefabs.** The actor action adds/selects NetworkView using undo. Presets such as Server Prop, Player Character, Interactable and Vehicle add validated companion components. Durable identity is assigned automatically; runtime owner/connection IDs are diagnostic fields.
4. **Author replication.** Inspector chooses permitted authority, delivery/profile, synchronized fields, interpolation and interest. A schema tool lists replicated fields and typed RPC permissions; estimated traffic is labelled as an estimate until measured. Prefab catalog editing validates asset identity, dependencies and build inclusion.
5. **Validate.** A preflight report links problems to actors/assets/properties and offers concrete fixes. Run it on demand, before multiplayer play and during packaging. Hard errors block the affected profile; unsupported features cannot appear successfully enabled.
6. **Launch a local session.** Multiplayer Play selects dedicated server + N clients or listen server + N clients, defaulting to two clients. Launch independent processes from one selected build/configuration with explicit scene/profile/run IDs, ephemeral ports and scoped development authentication. Collect startup/readiness/failure states and provide per-process logs.
7. **Inspect and reproduce.** A session window displays roster, roles, object registry, ownership, relevance, traffic, RPC rejections, snapshot age and corrections. Select a remote object to inspect its local replica. Apply seeded impairment per connection and export a redacted reproduction bundle.
8. **Stop, save and package.** Stop cancels pending loads, revokes callbacks, disconnects sessions, joins workers and terminates only processes owned by that test run before restoring the Editor scene. Save/reopen retains authoring data. Build Client / Build Dedicated Server validates and cooks approved schemas/assets into source-free packages.

### 7.2 Required Editor surfaces

| Surface | Required behavior | Acceptance evidence |
|---|---|---|
| Project networking settings | Validated profiles, backend capabilities, credential references and documented restart-required changes | Save/reopen and malformed profile migration; no secrets in serialized project data |
| Actor/Prefab Inspector | Identity/authority, component dependencies, channel/rate policy and read-only live status | Undo/redo, prefab override/revert, multi-selection and duplicate remapping |
| Network prefab catalog | Stable asset references, schema/content fingerprints, dependencies and inclusion rules | Move/rename/delete/reimport, duplicate prefab instances and packaged lookup |
| Replication/RPC schema editor | Supported field types, permissions, ranges, IDs, version and compatibility report | Generate/validate a working C++/Lua schema; incompatible change is detected before play/build |
| Multiplayer Play window | Mode, process count, readiness, scene/profile, local ports, impairment and stop controls | At least server + two clients; bind conflict, crash and cancellation recover without orphan processes |
| Network session/object inspector | Roster, authority, object generation, scene, parent, relevant recipients and rejection reason | Selecting actors/components agrees with live session state and remains safe during despawn |
| Profiler/traffic view | Per-channel/object/component bytes, RTT/jitter/loss, queue pressure, resends, baselines and correction history | Counters agree with test fixtures within documented header/accounting rules |
| Validation/build report | Clickable actor/asset/property diagnostics, explicit unsupported feature and content mismatch failures | Save/reopen and source-free package reports identify the same invalid configuration |
| R2 replay/interest tools | Timeline, object/state/correction tracks and spatial relevance visualization | Replay reproduces a captured scenario within its declared limits; hidden objects and reentry are inspectable |

The schema editor can start as a validated Properties-backed list with previews in R1; a richer visual editor follows in R2. The runtime schema/permissions must already work in R1.

### 7.3 Editor lifecycle rules

- Integrate startup/teardown with [PlaymodeJob](../Tools/cpp/Editor/src/jobs/PlaymodeJob.cpp), [LeavePlaymodeJob](../Tools/cpp/Editor/src/jobs/LeavePlaymodeJob.cpp), project close and Editor shutdown. Keep process/DNS/handshake waits asynchronous; do not hold Render/Physics/Application task locks while waiting for external processes or sockets.
- Because the engine uses global application-manager access, use independent processes as the R1 multiplayer test baseline. Do not promise multiple isolated worlds in one Editor process until singleton/context isolation is implemented and tested.
- Define pause semantics explicitly: the Editor may pause local presentation, but heartbeats and live server time continue; coordinated pause/single-step is only available in an isolated local test session whose participants all acknowledge it.
- If startup fails midway, close all resources acquired by that run and restore the authored scene. Track child process handles/run IDs rather than killing processes by executable name or port.
- Runtime changes do not dirty or overwrite the saved scene. Any optional Apply Runtime Changes operation must be an explicit, filtered undoable authoring command.
- Export build ID, protocol/schema/catalog hashes, seed, profiles, logs, frame/tick timing and aggregate metrics. Captured payloads are opt-in, redacted and bounded.

### 7.4 Preflight validation rules

Check duplicate/unresolved authored IDs, multiple session owners in one world, duplicate NetworkView components, missing/unapproved player/prefab assets, invalid parent/reference graphs, incompatible schemas, unsupported delivery modes, unsafe authority/relay policy, missing authentication configuration, conflicting transform/physics/root-motion writers, invalid rates/capacities, missing spawn points and missing server package dependencies.

Keep diagnostics actionable: identify the actor/component/asset, explain the runtime consequence and offer a supported repair. Avoid displaying backend implementation details unless they help the author choose a supported configuration.

## 8. Engine, gameplay, Lua and package integration

### Simulation and physics

Define the tick pipeline together with the fixed-step physics work:

```text
bounded network ingress -> authenticate/validate -> apply session/lifecycle commands
  -> select authoritative or predicted inputs -> run gameplay + one physics step
  -> capture committed state -> relevance/delta build -> budgeted transport egress
  -> publish state/events/statistics -> render interpolation and local presentation
```

Document the exact job/task affinity and previous/current tick boundaries. Test body activation, sleeping, collision/trigger events, moving platforms, teleport and ownership transfer. Remote replicas must have an explicit collision policy; rendering interpolation must not move authoritative collision shapes during a query. Headless servers load required physics/gameplay services without creating a window, audio device or graphics backend.

### Existing vehicle, racing and procedural systems

Extend existing vehicle presentation and racing lifecycle rather than implementing new parallel sample logic. Reuse `SampleVehicleAdvanced`, `RacingGameFull`, the open-city/procedural race systems and `RacingNetworkTransport` where applicable after confirming their actual runtime dependencies.

Server owns race start/countdown, checkpoint/lap validation, finish/rank and match results. Client UI and effects consume replicated results. Validate two independently controlled vehicles, late joining, disconnect/ownership cleanup and race restart. Reuse existing transform/camera smoothing instead of applying two smoothing buffers accidentally.

For procedural/open-city scenes, replicate approved generation configuration/seed and content version, verify the generated manifest, and replicate dynamic authoritative state. A seed alone is not a guarantee that different builds produce matching geometry/collision. Streamed cells and origin changes need scene/relevance epochs; detailed open-world support is R2.

### Animation, effects and audio

Use existing animation state/graph/parameter services. Choose root-motion authority explicitly; avoid animation and NetworkTransform both advancing an actor. R2 NetworkAnimator replicates semantic state and timing before considering high-volume pose streaming. Give transient effects/audio events IDs and expiry policies so input replay, resync and duplicate delivery do not play them twice. Local cameras, Editor gizmos and HUD ownership remain local.

### Lua

Extend real `NetBind.cpp` and relevant scene/script bindings with session lifecycle, subscription tokens, typed variables/RPCs, spawn requests, player/object queries, authority, time and diagnostics. Expose safe integer/identifier representations, structured results and cancellation. Remove arbitrary authoritative setters from untrusted script paths while retaining supported administrative APIs.

Guarantee callbacks on the script task after complete state commits; define `OnNetworkSpawn`, `OnNetworkDespawn`, `OnOwnershipChanged`, `OnPlayerJoined/Left`, `OnSessionReady/Stopped` and replicated-field change hooks. Catch script failures per handler, preserve the session, revoke callbacks on unload/reload and prevent duplicate effects during prediction replay.

Test the actual production bindings with initialized engine type/factory systems and concrete managers/packets, including standalone scripts and hot reload. Mock managers or accessor-only tests do not certify real packet, listener or lifecycle behavior.

### Assets and packaging

Use the existing catalog/resource plan for durable prefab/scene identity, cooked dependencies, async residency and last-good reload. Networking owns its typed schema/catalog contents and compatibility checks, not a second asset database or remote asset loader.

Package compatible client and server manifests containing game/protocol/schema/content IDs. Dedicated server includes collision/gameplay assets and approved scripts without client-only rendering/audio requirements. Reject required content mismatch before gameplay; optional cosmetics need an explicit fallback policy. Loading arbitrary assets from peers is outside R1/R2.

Use target-based CMake dependencies and independent presets/test targets for networking. Add a shipping network profile, pinned third-party notices/dependency manifest, compiler/configuration coverage and sample launch instructions. Never require developer source paths or current working-directory accidents for startup.

## 9. Implementation work packages

Owners below are roles to assign, not assumed staffing. Each package should land in small reviewable increments with source, fixtures, validation, documentation and measured limitations. No calendar estimate is credible until NET-00 establishes transport choice and team capacity.

### NET-00 — Baseline, contracts and transport decision

**Dependencies:** none. **Owner:** networking lead with build/security reviewers.

- Inventory all factories, interface consumers, runtime update paths, serialized fields, Lua/racing message schemas and current binary/configuration dependencies. Reproduce multiple-client ID behavior and concrete stream input handling.
- Create minimal native and concrete C++ test targets independent of rendering/audio and a seeded fake socket/clock harness. Record build hashes and the initial failures instead of treating absent targets as passes.
- Implement the transport adapter experiment and compare it to the cost of completing native UDP. Record capability, licensing, build, security, maintenance and headless/package tradeoffs.
- Publish ADRs for transport, identifiers, authority, clocks/task ownership, wire compatibility and release capacity profiles.

**Exit:** reproducible baseline and selected shipping transport with an approved conformance contract; known P0 findings have regression fixtures or explicit reproduction instructions.

### NET-01 — Native/wrapper safety and portable codec

**Dependencies:** NET-00. **Owner:** runtime/build.

- Repair socket-handle width, bounded formatting, null/size validation, OS error classification, context lifecycle and `WPNetworkStream::setData()` on retained paths.
- Implement the shared checked codec, golden wire vectors, bounded readers/writers and staged validation/atomic commit. Remove duplicate codec implementations through adapters.
- Clean CMake dependencies/export/configuration behavior and add actual native/packet/stream CTest registration with no-tests-as-error.
- Introduce versioned configuration/results/capability APIs and documented legacy behavior.

**Exit:** freshly built concrete native/wrapper tests pass in Debug and RelWithDebInfo; malformed/truncated messages cannot partially mutate state or grow allocations without limit.

### NET-02 — Secure transport and truthful delivery

**Dependencies:** NET-01 and selected backend. **Owner:** transport/security.

- Integrate reliable ordered and unreliable sequenced channels, bounded fragmentation, pacing/congestion behavior, backpressure and statistics through one backend.
- Add authenticated connection state machine, identity verification, admission, replay protection, retry/timeout/close and credential handling.
- Implement IPv4/IPv6, cancellable DNS and explicit update/work budgets. Prevent blind relay and unauthenticated resource/amplification abuse.
- Run the same delivery/security conformance suite against every advertised backend; keep uncertified backends disabled in shipping profiles.

**Exit:** independent-process connections authenticate, recover delivery under impairment or fail explicitly, reject forged/stale control, and retain bounded CPU/memory under hostile input.

### NET-03 — Session, identity, time and engine scheduling

**Dependencies:** NET-02. **Owner:** runtime/gameplay.

- Implement session/roster service, server-issued player IDs, generated handles/epochs and ready/failure/terminal events.
- Wire one engine update owner and bounded I/O-to-application event delivery. Add explicit stop/unload behavior and safe revocable subscriptions.
- Implement monotonic time, RTT/jitter/offset, server tick mapping and configuration profiles.
- Prove client/dedicated/listen/offline lifecycle, local-host authorization and at least three distinct simultaneous client identities.

**Exit:** sessions start/stop repeatedly without stale callbacks/handles; client readiness, player identity and server time are meaningful and observable.

### NET-04 — Object registry, prefabs, scenes and late join

**Dependencies:** NET-03; shared asset identity/loading contracts. **Owner:** scene/resources.

- Implement durable authored identities, runtime registry/generations, approved prefab catalog and server spawn/despawn/ownership transactions.
- Add initial-state barrier, parent/reference resolution, baseline chunks and bounded post-baseline lifecycle buffering.
- Integrate single-scene travel, resource failures, persistent-player policy, cancellation and disconnect cleanup.
- Add duplicate/paste/prefab/scene migration and save/reopen tests, plus late-join/despawn-race multi-process scenarios.

**Exit:** a late client receives exactly the live approved scene/object roster, cannot revive despawned objects, and can leave/rejoin without leaks or identity collisions.

### NET-05 — Typed replication, RPC and bounded relevance

**Dependencies:** NET-04. **Owner:** gameplay/networking.

- Implement NetworkBehaviour schemas, variables, change callbacks, typed RPC direction/permissions and server validation.
- Add per-client acknowledged baselines, full/delta recovery, object/component batching and field quantization profiles.
- Add spatial relevance, priorities, dormancy basics, reentry baselines and per-peer/global budgets.
- Migrate scene NetworkView and WPNetworkView onto the canonical path; preserve racing application IDs and legacy adapters deliberately.

**Exit:** lost deltas recover, stale ownership/generations are rejected, unauthorized RPC/state has no effect, and relevance reentry restores current state within fixed memory budgets.

### NET-06 — Movement, physics and baseline character play

**Dependencies:** NET-05 and applicable physics fixed-step/controller work. **Owner:** physics/gameplay.

- Implement NetworkTransform timestamps/quaternions/buffers, teleport/reset and hierarchy contracts.
- Add server rigid-body integration and remote replica policy with single motion ownership.
- Implement the bounded character input/prediction/reconciliation profile and local player camera/input/UI gates.
- Create a complete two-player interaction/character scene and headless gameplay scenario, including late join and disconnect.

**Exit:** two real clients can independently move/interact under the typical impairment profile; correction is measured, collision state remains authoritative, and local presentation belongs to the correct player.

### NET-07 — Components, persistence and Lua authoring

**Dependencies:** NET-04/05; coordinate with NET-06. **Owner:** scene/scripting/Editor.

- Register R1 components, dependency rules, versioned properties, presets and durable references.
- Extend actor/prefab Inspector and undoable commands; add identity migration/collision diagnostics and project/profile/catalog persistence.
- Implement real C++/Lua core API parity, callbacks/subscriptions, typed schemas and safe hot reload.
- Add factory/type, serialization, undo/redo, clone/prefab, Lua and play/stop lifecycle coverage.

**Exit:** an author can build/save/reopen a working multiplayer scene through supported components and scripts without manual runtime-ID editing.

### NET-08 — Editor multiplayer launcher and diagnostics

**Dependencies:** NET-03/07 plus a launchable NET-06 sample. **Owner:** Editor/tools.

- Add project settings, preflight reports and a process-owned dedicated/listen multiplayer launcher.
- Implement asynchronous readiness/failure/cancel/stop, isolated local credentials and Editor scene restoration.
- Add session/object inspection, Profiler traffic/correction views, seeded impairment and reproduction bundles.
- Extend Editor play-mode tests and perform interactive validation, including startup failures, child crashes and repeated stop/restart.

**Exit:** Editor launches server + two clients, authors inspect actual replicated gameplay, and every failure/stop path restores the scene and releases owned processes/ports.

### NET-09 — R1 packaging, operations and certification

**Dependencies:** NET-01 through NET-08. **Owner:** release/QA/operations.

- Build source-free client/headless server packages, manifests, configuration presets, structured logs, health/readiness and bounded server drain/shutdown.
- Run required native/concrete-wrapper/Lua/multi-process/Editor/package/impairment/security/soak/scale suites from fresh binaries.
- Publish protocol/component/authoring documentation, troubleshooting, deployment/credential runbooks, performance envelopes and support matrix.
- Record unresolved limits and remove/disable unsupported claims/features in release profiles.

**Exit:** every R1 gate in sections 10/11 passes with reproducible artifacts; no required test is skipped or satisfied solely by mocks, compile success or loopback.

### NET-10 — Comprehensive gameplay and scale

**Dependencies:** R1; animation/vehicle/physics capability gates. **Owner:** gameplay/performance.

- Add NetworkAnimator, NetworkVehicle, projectile/hit rewind, richer prediction and event/effect deduplication.
- Add spectators, pooled-object lifecycle, advanced relevance/dormancy, adaptive snapshot budgets, scene streaming and world-origin epochs.
- Extend existing vehicle/racing/open-city samples for authoritative race flow, late join, restart and reconnect cleanup.
- Implement capture/replay/interest visualization and certify the R2 capacity profile.

**Exit:** each gameplay adapter has real multi-process scenes and impairment evidence; scale improvements preserve correctness and measured budgets.

### NET-11 — Lobby, discovery, NAT/relay and online workflow

**Dependencies:** R1; provider/environment decision. **Owner:** online services/tools.

- Define narrow asynchronous interfaces for authentication, lobby, matchmaking, signaling, relay and server allocation; implement local test providers plus selected real provider adapters.
- Add LAN discovery, room metadata/filtering, teams/ready, join codes/invites, lobby-to-game transition and content/version admission rules.
- Add NAT traversal with relay fallback, cancellable service requests, quotas, retry/backoff, reconnect/resume and clean outage/rejection behavior.
- For opt-in host migration, transfer authoritative checkpoint plus ordered event state, elect/authorize a replacement, issue a new session authority/key epoch and test host loss during spawn/travel/RPC. Otherwise terminate clearly; do not silently imply migration.

**Exit:** clients on separate real networks complete the chosen online workflow, relay fallback and failure recovery; service dependencies, credentials, costs and operating limits are documented. A simulated provider cannot satisfy this gate.

### NET-12 — R2 release certification and operational hardening

**Dependencies:** NET-10/11 for advertised scope. **Owner:** release/QA/operations.

- Run R1 regressions plus R2 gameplay, WAN/NAT/provider, reconnect, streaming, replay and capacity suites.
- Validate allocation/readiness/draining, secret rotation, provider outage and incompatible-client rollout on the supported deployment profile.
- Finalize per-platform/backend/provider feature matrices and support runbooks. Record host migration separately if it is shipped.
- Publish source/configuration/fixture hashes, captures, benchmark methodology, issues and explicit extension exclusions.

**Exit:** comprehensive features have complete authoring-to-packaged-game paths and independent production evidence.

### Dependency order

```text
NET-00 -> NET-01 -> NET-02 -> NET-03 -> NET-04 -> NET-05 -> NET-06
                                       |          |          |
                                       +-------> NET-07 -> NET-08
                                                            |
                    NET-01..08 ---------------------------> NET-09 (R1)
                                                            |
                                                      NET-10 + NET-11
                                                            |
                                                         NET-12 (R2)
```

Tests, security review, documentation and diagnostics accompany each package. The release packages aggregate evidence; they are not the first time failures or performance are tested.

## 10. Validation strategy

The suite catalogue below is the target release scope. The foundation increment registers native, codec and production integration tests as recorded in the implementation status; the remaining proposed suites still need implementation. CI must fail when a required suite selects zero tests or lacks its executable/fixture.

| Suite | Required coverage | Evidence |
|---|---|---|
| `WorkphoneNetworkTests` | Real retained native/backend facade, socket errors, lifecycle, dual-stack addresses, time, budgets and delivery | Fresh native build plus loopback sockets and deterministic fake-clock/loss fixtures |
| `WPNetworkTests` | Concrete manager/packet/stream adapters, configuration, ownership of resources, truthful results and callbacks | Production C++ sources, initialized type/factory system, both stream implementations/adapters |
| `WPNetworkProtocolTests` | Golden wire bytes, bounds, version/schema negotiation, wrap, replay, fragmentation and atomic state commit | Deterministic fixtures, cross-codec vectors, fuzz corpus and memory/error reports |
| `WPNetworkSceneTests` | Identity, prefab/scene lifecycle, ownership, initial state, baseline loss, RPC authorization and relevance | Real scene/resource components plus invalid/stale/missing-dependency cases |
| `WPNetworkLuaTests` | Core API parity, callback exceptions, revoke/unload/hot reload, racing schema migration and malformed input | Real Lua host and production bindings with concrete manager/packet path |
| `WPNetworkProcessTests` | Server plus 2/3/16 clients, distinct players, join/play/late join/leave/restart and close | Independent executable processes, random ports, bounded readiness and run-owned cleanup |
| Editor networking tests | Add/configure/undo/save/reopen/prefab overrides, preflight and play/stop failure recovery | Automated Editor tests plus recorded interactive workflow and visible replicated movement |
| Gameplay scenes | Character, physics props, interaction/door state; R2 vehicles/race/animation/projectiles/streaming | Playable client/server fixtures under declared impairment, with corrections and outcome assertions |
| Security/fuzz | Spoofed accepts, unauthenticated input, replay, malformed lengths, huge allocations, flood, authority/RPC abuse | Threat-model cases, parser/codec/state-machine fuzzing and resource ceilings |
| Headless/package tests | Source-free startup, required assets/scripts, no device/window dependency, configuration and shutdown | Fresh client and server install directories and actual package launch |
| Soak/scale | Join churn, queue pressure, slow clients, packet floods, long uptime and resource reuse | Metrics over fixed workloads, memory/handle/socket/thread trends and latency percentiles |
| WAN/provider R2 | Different external networks, IPv6, NAT variants, relay fallback, service outages and reconnect | Real endpoint/environment records and redacted service/transport logs |

Use a seeded in-process impairment layer for repeatable tests and an external network shaper/process proxy for end-to-end validation. The latter must exercise real sockets and distinguish RTT from one-way delay. Cover asymmetric loss/delay, burst loss, duplication, reordering, bandwidth limits, temporary blackouts and MTU/truncation failures.

Use injected clocks for timer wrap, long uptime, drift and suspend cases; do not wait days to exercise them. Run ASan/UBSan or supported equivalents for native/codec targets on available toolchains, and race detection on a supported platform where applicable. Record unavailable instrumentation as a gap rather than claiming coverage.

Multiplayer tests must compare server/client object sets, generations, owners, authoritative state, replicated durable values and processed input sequences. Seeing packets or moving one local actor is insufficient. Ensure test failures, crashes and timeouts clean up their own processes and temporary resources.

## 11. Measurable release gates

### 11.1 Proposed certification profiles

These are **initial acceptance targets**, not measured current capability or universal promises. NET-00 records reference hardware/builds and freezes attainable budgets with justification; later changes require a documented tradeoff and rerun. Counts refer to the entire session unless stated otherwise.

| Profile | Players/objects | Tick and traffic targets | Required duration |
|---|---|---|---|
| R1 small match | 16 connected gameplay clients; 1,000 registered objects; up to 100 moving relevant objects per client | 60 Hz simulation, 20 Hz snapshots; average per-client downlink <=64 KiB/s and input/uplink <=16 KiB/s; server network/replication CPU p95 <=2 ms per simulation tick on declared hardware | 2-hour gameplay/churn run and separate 24-hour connection/lifecycle soak |
| R2 scale | 64 clients; 10,000 registered objects; up to 250 relevant objects per client, with published moving/dormant ratio | Configurable 30/60 Hz simulation and 10/20 Hz snapshots; establish explicit CPU/memory/bandwidth envelopes before claiming support | 8-hour mixed gameplay/churn run plus 24-hour soak |
| Minimal headless | One packaged server and two packaged clients | No graphics/audio device required by server; same protocol/security/authority path as Editor tests | Complete match, late join, leave/rejoin, restart and graceful shutdown |

Measure application, transport/security and IP overhead separately; include retransmission/baseline traffic in total transport budgets. Report averages and p95/p99 bursts, queue age, allocation rate and both baseline and steady-state costs. Publish the workload's field schema, motion pattern and relevance distribution. Do not claim 64-player support merely because `NET_MAX_PEERS` is 64.

### 11.2 Network impairment profiles

| Profile | Conditions | Expected outcome |
|---|---|---|
| Clean | 30 ms RTT, 5 ms jitter, no injected loss | Correct full gameplay; bounded interpolation and no unexplained periodic corrections |
| Typical | 150 ms RTT, 30 ms jitter, 3% packet loss plus controlled duplication/reordering | Remains playable within the chosen character/vehicle profile; eventual durable-state convergence and reliable delivery or explicit failure |
| Stress | 300 ms RTT, 100 ms jitter, 10% loss including bursts and bandwidth caps | No crash, deadlock, unauthorized state, unbounded queues or silent durable-state loss; degradation is observable and timeout policy is respected |
| Disruption | 5-second blackout, server/client crash, address change, slow loading and bind conflict | R1 closes/rejoins cleanly as configured; R2 resumes only after reauthentication/resynchronization; Editor and package cleanup succeed |

Treat jitter values as the declared shaper distribution/range in the fixture, not an unspecified marketing number. Document latency percentiles and correlated/asymmetric profiles in results.

### 11.3 Required pass conditions

| Gate | Pass condition |
|---|---|
| G0 Build/reproducibility | Fresh Debug and RelWithDebInfo native/concrete-wrapper builds and required suites pass; supported shared/static and headless package configurations link/run; release manifests identify source, dependencies and fixtures |
| G1 Delivery/lifecycle | Reliable messages deliver in order without duplicates within a live connection or fail explicitly; obsolete unreliable snapshots never roll state backward; all queues/reassembly are bounded; no lost mandatory lifecycle events |
| G2 Security/authority | Forged/stale/replayed packets, unknown schema/RPC, unauthorized owner changes, client authoritative writes and blind relay attempts have no gameplay effect; authenticated encryption and credential handling are reviewed |
| G3 Session identity | Three simultaneous clients have distinct server-issued identities on all peers; reconnect/slot reuse/session restart cannot accept old state; each attempt/connection reports one terminal outcome |
| G4 Scene correctness | Spawn/parent/ownership/despawn, late join and scene travel converge to server state under impairment; required missing/incompatible assets fail clearly without partially committed actors |
| G5 Replication correctness | Lost/expired delta baseline recovers, relevance reentry sends current state, typed durable fields converge, and truncated/invalid packets make no partial changes |
| G6 Playable flow | Server + two clients complete join, independent movement, interaction, late join, leave and restart. R2 additionally completes animation/vehicle/race/projectile and online flows with measured correction behavior |
| G7 Presentation/prediction | NetworkTransform and character movement meet scene-specific correction/error budgets recorded before testing; teleports/handoffs reset history; replay does not duplicate events or compete with physics/animation writers |
| G8 Authoring | Component addition/configuration, undo/redo, duplicate/paste, prefab overrides, save/reopen and package validation preserve durable identity and intended policy; runtime data never contaminates authored scenes |
| G9 Editor lifecycle | 100 launch/stop/failure cycles return owned process/socket/thread/handle counts to baseline; scene restore and project close succeed; no queued callback reaches an unloaded actor or Lua state |
| G10 Capacity/soak | Published profile CPU/bandwidth/memory targets pass; slow/flooding peers cannot monopolize work; memory remains within fixed pools/declared caches after warm-up, with no positive leak trend after join/despawn churn |
| G11 Packaging/operations | Source-free compatible client/server packages launch, authenticate, play and shut down; headless health/readiness/drain and redacted diagnostics work; unsupported profiles fail explicitly |
| G12 R2 online services | Selected real provider/environment passes lobby/join/version rejection/NAT/relay/outage/reconnect tests across external networks; optional host migration has separate authority/state/key-epoch evidence |

Freeze numerical correction/error budgets per movement fixture in NET-06/10. For example, record p95 correction distance, maximum correction after teleport exclusion, input-to-server acceptance delay, remote snapshot age and time to durable-state convergence. Averages alone must not conceal large visible snaps or prolonged stalls.

Required evidence includes commands, exit codes, source/build/configuration hashes, fixture versions, seeded impairment settings, client/server logs, profiler metrics and Editor/gameplay captures. Missing external environments, fixtures, executables or device access remain explicit release gaps. Compilation, synthetic codec tests and loopback results cannot substitute for their corresponding gameplay/Editor/WAN gates.

## 12. Dependencies, risks and first implementation increment

| Dependency/risk | Required resolution |
|---|---|
| Transport maintenance cost | Decide in NET-00 and pin one shipping backend; cost custom reliability/congestion/security ownership explicitly |
| Existing API/serialized/wire compatibility | Inventory consumers and racing schemas; use adapters/migrations and explicit ABI/protocol versioning |
| Fixed-step physics/controller readiness | Coordinate with WPPhysics plan; certify a supported character profile and keep broader rollback claims separate |
| Asset identity/cooking/async loading | Use shared resource/catalog services; network IDs are separate; missing baseline assets block readiness |
| Global managers/singletons | Use independent local processes first; context isolation is a separate prerequisite for in-process multi-world testing |
| Editor job/task locks and scene restoration | Keep external waits asynchronous and revoke session callbacks before actor teardown/scene restore |
| Service provider choice/access | Build narrow adapters and local fixtures; real provider/WAN certification remains mandatory for advertised online features |
| Authority bypass via legacy APIs | Route old view/packet setters and global relay through explicit supported policies; shipping profiles reject unsafe paths |
| Capacity creep | Publish one measurable R1 profile and a separately measured R2 profile; align object complexity with traffic/CPU budgets |
| Headless transitive dependencies | Audit actual startup and required libraries/assets; prove device-free package operation rather than relying on a build flag |

The first concrete increment should deliver NET-00's baseline harness and decision record, then NET-01's socket/codec/stream/lifecycle repairs. It should include regression cases for distinct client identity, spoofed connection acceptance, `WPNetworkStream::setData()`, malformed partial transforms and bounded event overflow. Next integrate secure delivery and session identity before expanding gameplay replication or Editor controls that would otherwise expose incomplete guarantees.

Track progress in a subsequent implementation-status document containing package status, actual changes, evidence and remaining gaps. Mark R1/R2 complete only after their release gates pass; a feature checklist or set of populated classes is not completion evidence.
