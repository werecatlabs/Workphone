# WPNetwork transport decision

Date: 30 October 2026. Scope: NET-00 and NET-02 in the [production plan](WPNETWORK_PRODUCTION_PLAN.md).

## Decision and release boundary

Use **GameNetworkingSockets behind the Workphone networking facade** as the R1 transport direction, with project-controlled certificate issuance hosted by the game operator. Retain native WorkphoneNetwork UDP for explicit development/reference use. Do not add native retransmission or a new cryptographic protocol as the shipping solution.

This selects an implementation direction. Shipping enablement still requires a pinned dependency set, an operating certificate issuer, the conformance comparison and trust/failure tests below. The current WPNetwork implementation remains the native development backend; this decision does not enable or certify GNS.

The user selected self-hosted services with local development providers. R1 starts with direct dedicated/listen server connections. Matchmaking, signaling, NAT traversal and relay adapters follow separately. Open-source GNS supplies transport capabilities without requiring Steam, but does not grant access to Steam authentication or relay services. It also leaves entity replication and compression to the game. [GNS scope](https://github.com/ValveSoftware/GameNetworkingSockets#readme).

## Why this direction

WPNetwork already has a bounded codec, a manager facade, scene listeners and a development UDP regression suite. The shipping gap combines delivery, congestion, fragmentation, security and platform behavior. Using an established transport concentrates Workphone work on admission, scene state and gameplay authorization. Keeping a second production protocol would duplicate maintenance and certification work.

GNS documents reliable/unreliable messages, fragmentation, encryption, IPv6 and independent traffic lanes. These features match the transport experiment; their behavior through the Workphone adapter must still be measured. [Upstream feature description](https://github.com/ValveSoftware/GameNetworkingSockets#readme).

## Dependency and build contract

The Windows x64 experiment uses GNS static linkage with its `BCrypt` crypto backend and the engine's DLL CRT setting. Disable ICE and WebRTC initially; direct connections do not justify that dependency expansion. The upstream build supports BCrypt, requires protobuf, and requires matching `protoc` and protobuf runtime versions. Recent protobuf adds Abseil. [Build requirements](https://github.com/ValveSoftware/GameNetworkingSockets/blob/master/BUILDING.md).

Before adding the adapter, record an immutable GNS commit/archive checksum, protobuf/protoc versions, any Abseil revision, upstream submodule revisions, licenses and all local patches. **That pin and dependency audit are pending.** A branch name or an unbounded `find_package()` search is insufficient for the shipping build. Recheck all API and trust details below against the chosen revision.

Build the audited dependency separately with these intended settings:

```text
BUILD_STATIC_LIB=ON
BUILD_SHARED_LIB=OFF
USE_CRYPTO=BCrypt
ENABLE_ICE=OFF
USE_STEAMWEBRTC=OFF
BUILD_EXAMPLES=OFF
BUILD_TESTS=ON
```

Build issuer tools in their own configuration if needed; do not distribute an issuer private key with an engine dependency package. GNS exposes target-scoped crypto and platform linkage and a separate certificate-tool target. [Upstream CMake implementation](https://github.com/ValveSoftware/GameNetworkingSockets/blob/master/src/CMakeLists.txt).

The Workphone integration should consume the audited installed package through an opt-in target. The following is the intended CMake shape, not an existing option:

```cmake
option(WP_NETWORK_GNS "Build the audited GNS transport adapter" OFF)
if(WP_NETWORK_GNS)
    find_package(GameNetworkingSockets CONFIG REQUIRED)
    add_library(WPNetworkGNS STATIC ${WP_NETWORK_GNS_SOURCES})
    target_link_libraries(WPNetworkGNS PRIVATE GameNetworkingSockets::static)
    target_compile_features(WPNetworkGNS PRIVATE cxx_std_17)
    target_link_libraries(WPNetwork PRIVATE WPNetworkGNS)
endif()
```

`GameNetworkingSockets::static` is an upstream package target. [Upstream package example](https://github.com/ValveSoftware/GameNetworkingSockets/blob/master/examples/vcpkg_example_chat/CMakeLists.txt). Keep GNS/protobuf headers out of public Workphone interfaces, preserve configuration-specific Debug/RelWithDebInfo CRT compatibility, and prohibit configure-time dependency downloads in the release preset. A requested missing backend must fail configuration/startup; never fall back silently to native UDP.

## Adapter design

Introduce an internal transport contract shared by the native reference adapter and GNS adapter. It owns connection handles/generations, typed send results, bounded events and endpoint metadata. `WPNetworkManager` continues exposing the legacy facade and multiplexing listeners. Preserve `WPNetworkPacket` and the existing gameplay codec. Native `NetContext` remains available only through the development backend.

The proposed API mapping is:

| Workphone operation | GNS operation |
|---|---|
| Acquire/release process transport runtime | `GameNetworkingSockets_Init` / `GameNetworkingSockets_Kill` |
| Start listener/connect | `CreateListenSocketIP` / `ConnectByIPAddress` |
| Incoming attempt decision | `AcceptConnection` / `CloseConnection` |
| Poll connection state | `RunCallbacks` routed to the owning session |
| Receive bounded messages | `CreatePollGroup`, `SetConnectionPollGroup`, `ReceiveMessagesOnPollGroup` |
| Send reliable/unreliable | `SendMessageToConnection`; use `SendMessages` for explicit lanes |
| Configure lane policy | `ConfigureConnectionLanes` |
| Close/drain/stop | `CloseConnection`, `CloseListenSocket`, `DestroyPollGroup` |

These methods are defined by the [socket interface](https://github.com/ValveSoftware/GameNetworkingSockets/blob/master/include/steam/isteamnetworkingsockets.h) and [process runtime interface](https://github.com/ValveSoftware/GameNetworkingSockets/blob/master/include/steam/steamnetworkingsockets.h).

Workphone must own these additional rules:

- Reference-count the process runtime across local managers. Route callbacks using an owned handle-to-session registry and generations, not a raw owner pointer captured by an old callback. Closing one manager must leave others usable.
- Deliver engine callbacks on the Application task. Bound receive and dispatch work, reserve control-event capacity and reject admission when capacity cannot support the terminal lifecycle events. Prevent recursive polling from nesting dispatch.
- Use a generation-bearing connection identifier as sender context. Resolve a server-issued player ID only after admission. The sender must never come from a payload or an IPv4 address comparison.
- Preserve the existing `u16` player-ID facade, including server 0 and disconnected 0xffff, through a session mapping. Transport handles are a separate namespace. Send admission/player assignment over the reliable control lane before publishing the gameplay-connected event.
- Add a family-aware endpoint representation. `ISystemAddress::getBinaryAddress()` and `getClientAddress()` cannot encode IPv6; their compatibility path must reject unsupported conversions explicitly rather than truncate.
- Keep clock synchronization in the session layer. Transport timestamps/RTT do not establish a synchronized game clock or gameplay readiness.

Start with control, gameplay RPC, input, snapshot and bulk lanes. Give control reserved budget and bulk a bounded share. Application sequencing must discard stale snapshots and inputs; unreliable transport alone does not supply that gameplay policy. Different lanes must not imply a global ordering guarantee. Upstream guarantees reliable order within a lane. [Lane contract](https://github.com/ValveSoftware/GameNetworkingSockets/blob/master/include/steam/isteamnetworkingsockets.h).

Prevalidate message size/state before sending. Map queue saturation to `Backpressure`; map not-connected/state failures to `NotReady` or a terminal reason; preserve distinct invalid-argument/oversize results. Acceptance means local queueing. The existing void facade should throw/report the same typed failure through its compatibility path. GNS limits a sent message to 512 KiB in the reviewed source, below the codec's 1 MiB allocation ceiling. Set explicit send/receive byte and message budgets, receive-message limits and a 1200-byte total UDP payload ceiling; bound smaller snapshot payloads separately. [Message and configuration limits](https://github.com/ValveSoftware/GameNetworkingSockets/blob/master/include/steam/steamnetworkingtypes.h).

## Self-hosted trust and admission

A vanilla direct GNS connection is not sufficient for `AuthenticatedEncryption`. Without validated certificates or an out-of-band shared secret, its default encryption does not authenticate the peer. [Direct connection security contract](https://github.com/ValveSoftware/GameNetworkingSockets/blob/master/include/steam/isteamnetworkingsockets.h).

Choose a project CA and provision certificates through the operator's authenticated issuer. The reviewed GNS cert store defaults to a Valve root; a project build can supply `STEAMNETWORKINGSOCKETS_HARDCODED_ROOT_CA_KEY`. Do not ship with dynamic trust of arbitrary self-signed certificates. Parsing a CA certificate is not validation. [Trust-store declarations](https://github.com/ValveSoftware/GameNetworkingSockets/blob/master/src/steamnetworkingsockets/steamnetworkingsockets_certstore.h).

The issuer holds the CA private key. Clients and servers hold only their own private keys/certificates and the required public trust material. Local development uses a disposable issuer/trust root isolated from production. Public keys may be packaged; private keys, bearer credentials and issuer secrets must stay out of scenes, profiles, source control and logs. Certificate request/provisioning uses `GetCertificateRequest` and `SetCertificate`; validate the exact issuance workflow against the pinned upstream tools.

For shipping profiles, force `IP_AllowWithoutAuth=0`, `IPLocalHost_AllowWithoutAuth=0`, and `Unencrypted=0`; reject overrides from a scene or user-editable profile. Both authorization settings matter because the localhost override can otherwise permit a weaker connection. [Security configuration definitions](https://github.com/ValveSoftware/GameNetworkingSockets/blob/master/include/steam/steamnetworkingtypes.h).

Certificate-chain acceptance alone is not session admission. Match the authenticated certificate identity to the expected server and application; validate the client identity, audience, expiry, session/build/schema compatibility and admission policy before allocating a player. A valid certificate for a different game/server must fail. Short-lived admission credentials need replay/reservation policy. Keep role/ownership/gameplay authorization separate from transport authentication.

Define certificate rotation and revocation behavior, issuer outage behavior and clock-skew policy. The reviewed upstream store allows an expiry grace interval by default; choose and test the permitted interval for this deployment instead of assuming exact expiration enforcement. [Expiry handling](https://github.com/ValveSoftware/GameNetworkingSockets/blob/master/src/steamnetworkingsockets/steamnetworkingsockets_certstore.cpp). Advertise `AuthenticatedEncryption` only for a validated strict-trust configuration; synchronized server time remains a separate capability.

## Conformance and release obligations

Implement a common headless harness before selecting a shipping default. Run the same meaningful lifecycle/delivery cases against the native reference backend and GNS; record native reliability/authentication as explicit unsupported outcomes. An absent required executable or zero selected tests is a failure.

Required evidence for the GNS adapter:

1. Real multi-process loopback and LAN connections, multiple local managers, failed bind, cancellation, stale callbacks, disconnect/restart and teardown during receive. All accepted messages must release their upstream message allocation on every path.
2. Ordered duplicate-free reliable delivery or explicit connection failure under loss/reorder/duplication; lane isolation; queue saturation; bounded memory and bulk transfer. Verify stale snapshot discard and measured snapshot age rather than treating packet receipt as movement quality.
3. Trusted identities accepted; wrong CA, absent certificate, wrong server/application identity, expired/revoked credentials, altered transcripts, replayed admission and weak-auth overrides rejected. Exercise localhost and non-local endpoints. No untrusted connection may mutate a scene.
4. Two issuers/roots demonstrating development/production separation, renewal/rotation, issuer outage and controlled clock skew. Verify that no issuer key appears in a client/server artifact or capture.
5. IPv4/IPv6 address round trips, DNS cancellation and connection timeouts. Rebuild and run Debug and RelWithDebInfo packages from a clean machine/cache using the pinned dependency bundle.
6. Native-versus-GNS CPU, memory, bytes, queue age, throughput and loss recovery on recorded hardware/topologies, including process callback budgets and reliable-control traffic under bulk load. Freeze supported operating limits from those results.

Only after these pass should the shipping profile select GNS by default and the session/spawn/late-join work depend on its reliable authenticated control channel. Record the exact adapter/dependency revisions and proof artifacts in [implementation status](WPNETWORK_IMPLEMENTATION_STATUS.md). Existing native UDP tests and this decision record do not satisfy NET-02 certification.
