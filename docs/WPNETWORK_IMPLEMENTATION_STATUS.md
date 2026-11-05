# WPNetwork implementation status

Updated: 5 November 2026. Covers the foundation, development workflow and first engine GNS integration from [the production plan](WPNETWORK_PRODUCTION_PLAN.md). R1 and R2 remain open.

## Engine GNS integration, 5 November

`WP_NETWORK_GNS=ON` adds an opt-in GameNetworkingSockets manager behind `INetworkManager`. It supports direct numeric IPv4/IPv6, ordered reliable and unreliable messages, server-issued player IDs, directed sends, client RTT, disconnect and reconnect. The engine still uses the same packet codec and scene listeners. The manager bounds receive work, rejects oversized payloads, and invalidates a received address when its connection closes. Reliable sends report local queue acceptance; broadcast can fail after queueing to some peers.

Pass `Samples/cpp/Network/Config/network.gns-loopback.json` to `createWPNetworkServer()` to start a listener on `127.0.0.1:28080`. Call `createWPNetworkClient()` with the same profile, then call `connect("127.0.0.1", 28080, "")`. Install each manager through its application's lifecycle to use the existing scene validation and Play path. The Inspector shows the view's delivery mode and whether a session manager is available. The project still has no automatic Editor session or process launch UI. Builds with GNS disabled reject this profile explicitly; native UDP remains the default.

Use `./Scripts/Validate-WPNetwork.ps1 -IncludeGNS` to build the pinned standalone dependency and exercise the real engine adapter. The engine fixture runs two IPv6 clients, checks assigned IDs and reliable ordering, reassembles a 60,000-byte payload, and drives a real NetworkView transform update and ownership transfer. It also covers disconnect/reconnect, stale endpoints, and unloading from a packet callback. The launcher verifies a separate server and three client processes over GNS IPv4, and the scene preflight fixture verifies that reliable authoring is accepted with GNS and rejected with native UDP. These tests do not run the Editor interactively.

The strict C++ mode requires a certificate generated for the stopped manager's own request and a named peer identity allowlist. It checks the SDK's peer authentication and encryption state before delivering gameplay, rejects simultaneous connections for the same identity, and sets the pinned SDK's expired certificate grace period to zero. A process using a provisioned identity has one GNS manager. The loopback profile continues to select development mode. The manager leaves the release capability `AuthenticatedEncryption` unadvertised pending the production release gates.

## Operator certificate tools

`Scripts/Validate-WPNetworkTransport.ps1` builds the offline `WPNetworkGNSIssuer` executable in `.codex/network/build/<configuration>`. `generate-key --out <new PEM>` creates an Ed25519 signing key. `root --key <PEM> --out <new public file>` exports the public root. Use that public file with `Scripts/Build-WPNetworkGNSTrust.ps1 -RootKeyFile <public file>` to build the hash-checked GNS archive against the pinned dependencies. The build applies a locked patch so the standalone SDK can start a strict listener with a current offline certificate. Certificate verification stays required. Configure `WP_NETWORK_GNS=ON` and point `GameNetworkingSockets_DIR` at the resulting package's `lib/cmake/GameNetworkingSockets` directory. Keep the pinned protobuf and OpenSSL dependencies on the CMake prefix path. Each operator package contains a single library configuration. Set `CMAKE_CONFIGURATION_TYPES` to the engine configuration you selected when building the package.

Each game process calls `getCertificateRequest()` while stopped and keeps the same manager alive. The operator verifies the requesting account and runs `WPNetworkGNSIssuer issue --request <request> --identity str:<approved-id> --key <CA PEM> --out <new certificate>`. The application imports the returned certificate into the same manager and starts the strict session. The issuer uses the standalone GNS AppID 0 and a leaf lifetime of one hour by default. Keep the CA private key on the issuer host with restricted file access. The certificate request supplies a public key. Account verification, revocation, and ticket authorization are separate from issuance and remain open.

`Scripts/Validate-WPNetworkGNSTrust.ps1` generates disposable issuer keys and builds the trust fixture in a separate output directory. The RelWithDebInfo run passed all six scenarios. Trusted peers exchanged gameplay in both directions. The wrong identity and unsigned client failed during the handshake, the wrong root failed at import, and expired or missing local credentials failed at startup. The server recorded zero gameplay packets and zero admitted players in every rejection case. The runner records binary and dependency hashes, restores the engine's build settings, and removes the disposable signing keys after the run.

A separate RelWithDebInfo probe confirmed that a valid signed certificate fails import in a process that holds a different private key. A concurrent build blocked the original Debug engine run. The expanded Debug suite below now covers the certificate cases. Run the trust runner with `-Configuration Debug` when the shared `project_x64` build directory is idle.

## Strict session admission

The server now sends a fresh 32-byte challenge over the authenticated, encrypted GNS connection. It assigns a player ID only after receiving the matching reliable response. Pending peers have a five-second deadline and a separate limit of 16. They cannot send gameplay or generate join callbacks. Update strict servers and clients together because this adds frames to the strict handshake. The development loopback profile retains its existing handshake.

A strict server can set `WPNetworkGNSConfig::requireAdmissionTickets` before startup. Its trusted caller can then use `issueAdmissionTicket(peerIdentity, lifetimeSeconds)` for an allowlisted identity and pass the returned token to the authenticated account through an out-of-band channel. The client allowlists exactly one server identity, calls `setAdmissionTicket()` before dialing, and sends the token in the encrypted response to that server. Tickets expire after 1–120 seconds, are accepted once, and are scoped to the identity and the issuing listener instance. The server stores only token hashes and holds at most 256 outstanding tickets. Stopping the listener invalidates outstanding tickets.

The ticket store runs inside the listening server. Remote issuance still needs an account-backed service and a trusted delivery channel. A listener restart discards outstanding tickets, and clients need a fresh ticket to reconnect to a ticket-required listener.

The expanded trust runner passed in Debug and RelWithDebInfo. It retains the certificate checks and adds four raw signed-client probes. It verifies remote rejection of a withheld response, a wrong nonce, early gameplay and a nonce from an earlier connection. The same ticket-required listener rejects five invalid attempts and accepts the valid ticket as player ID 1. The four stock engine and loopback regressions passed in RelWithDebInfo. Reports are under `.codex/network/trust-runs`.

## Transport conformance work, 4 November

The standalone prerequisite established a pinned package manifest, source-hash/license inventory, isolated CMake target and repeatable `Scripts/Validate-WPNetworkTransport.ps1` entry point. It exercises IPv6, reliable ordering under simulated loss, fragmentation/reassembly and a client failing closed without credentials (the connection times out). The first package link exposed a protobuf static/DLL-import mismatch, now repaired by a hash-locked GNS port overlay. The conformance target passes in Debug and RelWithDebInfo; this experiment does not verify operator certificates. Native/codec suites also pass 2/2 in both configurations in this checkout. JUnit and per-configuration hash reports are retained under `.codex/network/build`. See the [transport decision](WPNETWORK_TRANSPORT_DECISION.md) for the exact boundary.

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

Recorded on Windows x64/MSVC, 5 November 2026: `Scripts/Validate-WPNetwork.ps1 -IncludeGNS` passes in Debug and RelWithDebInfo, with **4/4 engine scenarios** and **1/1 standalone GNS scenario** in each configuration. The RelWithDebInfo run also built Editor and WPLuaBind with `-IncludeEditor`. An eight-client GNS launcher run passed. Interactive Editor acceptance, operator certificate trust and release certification remain open.

Recorded on Windows x64/MSVC, 30 October 2026: `Scripts/Validate-WPNetwork.ps1 -IncludeEditor` passes. Isolated native/codec/authoring contracts pass **2/2 in Debug** and **2/2 in RelWithDebInfo**; production scene/diagnostic/lifecycle integration and the three-client separate-process scenario pass **2/2 in RelWithDebInfo**. WPNetwork, WPLuaBind and Editor build successfully. The integration fixture uses the production timer required by the current transform lifecycle. These remain headless/build results; interactive Inspector, Play/Stop and persistence acceptance are open.


Recorded on Windows x64/MSVC, 10 October 2026: native/codec suites pass **2/2 in Debug** and **2/2 in RelWithDebInfo**; the production integration suite passes **1/1 in RelWithDebInfo**. Workphone, WPNetwork, WPLuaBind and Editor build successfully in RelWithDebInfo, including the changed network binding and ActorWindow command path. The validation runner and `git diff --check` pass. These results cover this foundation increment, not the R1/R2 release gates. Interactive Editor and Lua runtime behavior remain unverified.

The isolated `network-contracts-debug` and `network-contracts` presets build and run native socket and portable codec tests without the renderer/Editor dependency graph. CTest treats zero selected tests as an error. Native fixtures use real loopback UDP endpoints and cover distinct client IDs, unsolicited/wrong-endpoint accepts, exclusive bind, invalid/oversized input, receive budgets/pressure, reconnect, retry/timeout wraparound and teardown/rebind. Codec fixtures cover golden bytes and atomic parse/capacity failures.

`WPNetworkIntegrationTests` links the real Workphone and WPNetwork libraries. It exercises actual packet/stream interoperability, profile factories/bind errors, multiple clients and sender association, callback isolation, scene transform rejection/acceptance, ownership rejection, detached listeners and repeated concurrent poll/stop. The scene fixture injects a real manager into a NetworkView attached to a real actor/transform; it does not run the full scene loader, renderer or Editor workflow.

Run `Scripts/Validate-WPNetwork.ps1` for both isolated configurations. Add `-IncludeEngine` for the production integration and separate-process tests; add `-IncludeEditor` to build Editor and Lua bindings too. The full configuration uses `project_x64` and its configured dependencies. Missing binaries, a failed dependency build or absent fixtures are validation gaps, not passes.

For a manual local smoke run after building `WPNetworkProcessTests`, run:

```powershell
./Scripts/Run-WPNetworkLocal.ps1 -Clients 3
./Scripts/Run-WPNetworkLocal.ps1 -Clients 3 -Backend gns_loopback
```

The GNS run requires a fixture built with `WP_NETWORK_GNS=ON` and uses reliable messages without application retries. The default executable is the RelWithDebInfo Windows x64 build; pass `-Executable` for another build path and `-OutputDirectory` for a new/empty report directory. Each report records actual process IDs, server-assigned identities, verified loopback peer traffic and successful teardown. The launcher retains child stdout/stderr logs and terminates only processes it started if the scenario fails or times out. This exercises fixture traffic rather than a playable game scene or loss/jitter simulation.

Editor acceptance remains: add/configure/undo/redo a Network View; save/reopen a scene and prefab; repeat Play/Stop; remove components during traffic; verify no networking starts in Edit mode and no subscriptions survive teardown. A compilation or headless scene test does not establish this interactive behavior.

## Remaining plan work

| Package | Status and next concrete dependency |
|---|---|
| NET-00 | Transport direction and initial dependency lock recorded; baseline, separate-process and standalone GNS conformance harnesses added; complete dependency audit, adapter comparison and performance baseline remain |
| NET-01 | Socket/codec/lifecycle repairs implemented; fuzz/sanitizer coverage and comprehensive event-overload certification remain |
| NET-02 | Engine adapter supports ordered reliable and unreliable delivery, fragmentation and direct IPv4/IPv6. It has an operator-root package, encrypted confirmation before player assignment and optional single-use tickets scoped to a named identity and listener. Service-backed issuance, certificate rotation and revocation, explicit lanes, overload evidence and asynchronous DNS remain |
| NET-03 | Polling, distinct player IDs (including reliable assignment in GNS), native diagnostics, cancellation/reconnect and timeout repairs added; session service/facade, connection state model, roster/readiness and synchronized clock remain |
| NET-04/05 | Pending: registry, prefab allowlist, spawn/despawn, late-join baselines, ownership epochs, typed state/RPC and relevance |
| NET-06 | Existing immediate transform replication hardened; interpolation, physics/character authority and prediction remain |
| NET-07/08 | Scene preflight, Inspector diagnostics and standalone process launcher added; NetworkView reliable transform and ownership transfer verified through the GNS adapter. Component catalogue, NetworkSession authoring, Editor process controls and interactive acceptance remain |
| NET-09 | Pending: dedicated server packaging, health/metrics, operating budgets and R1 certification |
| NET-10/11/12 | Pending: richer gameplay, self-hosted/local service providers, reconnect/streaming/replay and R2 certification |

The next admission step is to connect ticket issuance to account authentication and session selection. Certificate expiry governs new connections. Renewal and revocation during active sessions remain open.
