#pragma once

#include <WPNetwork/WPNetworkPrerequisites.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{
    /** Validated local development profile. No credentials belong in this file.
     * Loading a profile never opens sockets. The factory starts the configured
     * manager explicitly after validation. Shipping profiles require a future
     * authenticated transport and are rejected by this backend.
     */
    struct WPNetwork_API WPNetworkConfig
    {
        u32 port = 15822;
        u32 bindAddress = 0x7f000001u;  ///< Server interface defaults to loopback.
        u32 maxClients = 32;
        u16 eventsPerPoll = 128;
        bool verbose = false;

        static WPNetworkConfig load( const String &filePath );
    };
}  // namespace workphone
