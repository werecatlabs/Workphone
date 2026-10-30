# WPNetwork implementation status

Updated: 30 October 2026. Implements the foundation and development-workflow increments of [the production plan](WPNETWORK_PRODUCTION_PLAN.md). R1 and R2 are not certified or complete.

## Development workflow increment, 30 October

- Repaired test registration after networking contracts moved to `Tests/cpp/NetworkProduction`: the isolated preset and full build now use the correct source and include paths.
- Added immutable `WPNetworkManager::getDiagnostics()` snapshots: session state, actual bound endpoint, local identity, peer/event counts, transport counters, capabilities and retained error text. No raw context pointer is needed; snapshots remain readable after stop.
- Added explicit client disconnect/pending cancellation and immediate reconnect. Cancellation reserves a fresh socket before releasing the old endpoint, preventing delayed control traffic from the previous attempt entering the next one. Repeated calls preserve the single queued disconnect callback. Native cancellation sends a best-effort release to a server that may have admitted the request; delivery remains unreliable.
- Added native and wrapper IPv4 interface binding. Development factories default to loopback, with a strictly validated `bindAddress` profile field. Direct manager construction and legacy native startup retain wildcard binding unless configured explicitly. Server/client fixtures verify their actual loopback bindings.
- Added scene-wide NetworkView validation for unassigned/duplicate IDs, multiple views per actor, invalid owners, unsupported authority/delivery and unreliable ownership requests. The Editor rejects network Play before scene capture/flag changes and shows actionable Inspector text. Offline Play without a network manager remains available.
- Added `WPNetworkProcessTests` and `Scripts/Run-WPNetworkLocal.ps1`: one server process plus 2-8 real clients, bounded deadlines, validated request/reply data and distinct IDs, disconnect/teardown checks, owned-process cleanup, retained logs and JSON reports. This development smoke launcher does not complete the Editor launcher UI or impairment/security certification.
- Recorded the [transport decision](WPNETWORK_TRANSPORT_DECISION.md): GNS behind the Workphone facade with project-controlled self-hosted certificate issuance. The adapter, immutable dependency audit and trust conformance tests remain pending.

## Implemented foundation

- Native socket handles preserve Windows SOCKET width. Socket/runtime teardown is idempotent; bind failures and unsupported features are explicit. Exclusive server binding prevents ambiguous endpoint ownership on Windows.
- WGP2 connection requests carry a random attempt nonce; accepts must match the outstanding attempt and server endpoint. The server allocates distinct, non-reused player IDs within a running session. Connection retries, heartbeat/idle timeout and connect timeout use a monotonic clock and wrap-safe intervals.
- Datagram sizes, receive work per poll, message/string allocation and packet event queues are bounded. The event queue reserves space for control events and records dropped events. This is partial overload hardening, not a flood-resistance certification.
- Packets and both production stream implementations share a little-endian scalar/IEEE binary32 codec. Reads validate lengths and Boolean encodings; string decoding and buffer replacement preserve prior state on validation failure. `WPNetworkStream::setData()` now copies its input.
- Manager construction is inert. Explicit startup balances the socket runtime; unload clears subscriptions and closes sockets. Manager operations serialize access to the native context, including concurrent poll/stop and callback reentry. Raw context inspection remains restricted to exclusive application-task ownership. Capability reporting advertises only unreliable delivery. Reliable sends, password authentication, blind packet relay, peer mode, client synchronized time, RakNet GUIDs and unmeasured RTT fail explicitly instead of claiming unsupported behavior.
- The engine polls networking on its Application task before scene update. Scene receive handlers require Play mode, enabled replication, a connected sender and the authored authority policy. Snapshots reject truncation, trailing bytes, unknown channels and non-finite transform values before changing an actor or advancing sequence state. Sequence comparison handles wraparound.
- Only the server may issue ownership transfers; client requests must identify their connected sender. A failed transfer send leaves local ownership unchanged. Component listeners detach before unload; one failing listener no longer prevents subsequent connection/disconnection callbacks.
- The Editor's Add Network View action goes through the existing component command workflow. Lua exposes listener registration/removal, capabilities and sender lookup. These changes need the Editor validation described below.
- Factories consume optional JSON development profiles, reject malformed/unknown/duplicate fields and invalid limits, and explicitly start the configured socket. Profile reads are limited to 8 KiB. A production environment or unknown backend is rejected. Empty paths select defaults. Direct `WPNetworkManager` construction remains inert until `setServer()` or `connect()`.

## Development use

`Samples/cpp/Network/Config/network.development.json` selects the current UDP development backend. `port: 0` asks the OS for an ephemeral server port; use `net_get_bound_port()` to inspect the resulting native endpoint. `maxClients` is 1 through `NET_MAX_PEERS`, and `eventsPerPoll` is 1 through `NET_MAX_EVENTS`. A client factory starts an unconnected socket; call `connect(address, port, "")` explicitly and poll until established or failed. Nonempty passwords are rejected.

All libraries consuming the changed native context or C++ manager interface must be rebuilt together. The native handshake now uses WGP2; old WGP1 peers are incompatible. The gameplay envelope remains version 1.

The selected online-services direction is **self-hosted services with local development providers**. No authentication, lobby, matchmaking, signaling, relay or allocation service adapter has been implemented in this increment. Service selection is not represented as a working profile feature yet.

The native UDP backend is unencrypted and unreliable. The nonce binds connection accepts to an attempt; it does not authenticate a peer. Sender lookup associates packets with active endpoints; it is not cryptographic identity or replay protection. Do not treat these repairs as the secure transport required by NET-02.

## Validation

Recorded on Windows x64/MSVC, 30 October 2026: `Scripts/Validate-WPNetwork.ps1 -IncludeEditor` passes. Isolated native/codec/authoring contracts pass **2/2 in Debug** and **2/2 in RelWithDebInfo**; production scene/diagnostic/lifecycle integration and the three-client separate-process scenario pass **2/2 in RelWithDebInfo**. WPNetwork, WPLuaBind and Editor build successfully. The integration fixture uses the production timer required by the current transform lifecycle. These remain headless/build results; interactive Inspector, Play/Stop and persistence acceptance are open.


Recorded on Windows x64/MSVC, 10 October 2026: native/codec suites pass **2/2 in Debug** and **2/2 in RelWithDebInfo**; the production integration suite passes **1/1 in RelWithDebInfo**. Workphone, WPNetwork, WPLuaBind and Editor build successfully in RelWithDebInfo, including the changed network binding and ActorWindow command path. The validation runner and `git diff --check` pass. These results cover this foundation increment, not the R1/R2 release gates. Interactive Editor and Lua runtime behavior remain unverified.

The isolated `network-contracts-debug` and `network-contracts` presets build and run native socket and portable codec tests without the renderer/Editor dependency graph. CTest treats zero selected tests as an error. Native fixtures use real loopback UDP endpoints and cover distinct client IDs, unsolicited/wrong-endpoint accepts, exclusive bind, invalid/oversized input, receive budgets/pressure, reconnect, retry/timeout wraparound and teardown/rebind. Codec fixtures cover golden bytes and atomic parse/capacity failures.

`WPNetworkIntegrationTests` links the real Workphone and WPNetwork libraries. It exercises actual packet/stream interoperability, profile factories/bind errors, multiple clients and sender association, callback isolation, scene transform rejection/acceptance, ownership rejection, detached listeners and repeated concurrent poll/stop. The scene fixture injects a real manager into a NetworkView attached to a real actor/transform; it does not run the full scene loader, renderer or Editor workflow.

Run `Scripts/Validate-WPNetwork.ps1` for both isolated configurations. Add `-IncludeEngine` for the production integration and separate-process tests; add `-IncludeEditor` to build Editor and Lua bindings too. The full configuration uses `project_x64` and its configured dependencies. Missing binaries, a failed dependency build or absent fixtures are validation gaps, not passes.

For a manual local smoke run after building `WPNetworkProcessTests`, run:

```powershell
./Scripts/Run-WPNetworkLocal.ps1 -Clients 3
```

The default executable is the RelWithDebInfo Windows x64 build; pass `-Executable` for another build path and `-OutputDirectory` for a new/empty report directory. The launcher prints its report path. Each report records actual process IDs, server-assigned identities, loopback bindings and successful teardown. It retains child stdout/stderr logs and terminates only processes it started if the scenario fails or times out. This exercises fixture traffic, not a playable game scene or loss/jitter simulation.

Editor acceptance remains: add/configure/undo/redo a Network View; save/reopen a scene and prefab; repeat Play/Stop; remove components during traffic; verify no networking starts in Edit mode and no subscriptions survive teardown. A compilation or headless scene test does not establish this interactive behavior.

## Remaining plan work

| Package | Status and next concrete dependency |
|---|---|
| NET-00 | Transport direction recorded; baseline and separate-process harness added; dependency pin/audit, backend comparison and performance baseline remain |
| NET-01 | Socket/codec/lifecycle repairs implemented; fuzz/sanitizer coverage and comprehensive event-overload certification remain |
| NET-02 | Pending: authenticated encrypted transport, reliable channels, congestion/fragmentation, IPv6/asynchronous DNS, admission and replay protection |
| NET-03 | Polling, distinct player IDs, diagnostics, cancellation/reconnect and timeout repairs added; session service/facade, connection state model, roster/readiness and synchronized clock remain |
| NET-04/05 | Pending: registry, prefab allowlist, spawn/despawn, late-join baselines, ownership epochs, typed state/RPC and relevance |
| NET-06 | Existing immediate transform replication hardened; interpolation, physics/character authority and prediction remain |
| NET-07/08 | Existing component/command path hardened; scene preflight, Inspector diagnostics and standalone local process launcher added; full component catalogue, NetworkSession authoring, validators, local process launcher and diagnostics remain |
| NET-09 | Pending: dedicated server packaging, health/metrics, operating budgets and R1 certification |
| NET-10/11/12 | Pending: richer gameplay, self-hosted/local service providers, reconnect/streaming/replay and R2 certification |

Continue with the NET-00 dependency pin/audit and transport conformance comparison and NET-02 security/delivery implementation before expanding automatic spawning or shipping Editor launch controls. Those features depend on a reliable, authenticated control channel.
