#pragma once
#include <WPNetwork/WPNetworkPrerequisites.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{
    enum class WPNetworkSessionState
    {
        Idle,
        Listening,
        Connecting,
        Connected,
        Disconnected,
        Failed,
        Stopped
    };

    /** Copy of one manager's state, captured under its context lock. No native
     * pointers escape; tools may retain this value while a session is stopped.
     * Counters cover the current socket lifetime and include transport traffic.
     */
    struct WPNetworkDiagnostics
    {
        WPNetworkSessionState state = WPNetworkSessionState::Idle;
        bool server = false;
        u16 boundPort = 0;
        u32 boundHost = 0;
        u16 localPlayerId = 0xffffu;
        u32 peerCount = 0;
        u32 queuedEvents = 0;
        u32 droppedEvents = 0;
        u32 receivedDatagrams = 0;
        u32 malformedDatagrams = 0;
        u32 capabilities = 0;
        String lastError;
    };
}  // namespace workphone
