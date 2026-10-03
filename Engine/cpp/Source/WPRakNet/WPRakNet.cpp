#include "WPRakNet/WPRakNet.hpp"
#include "WPRakNet/CNetworkManager.hpp"
#include <Workphone/Core/Handle.hpp>

namespace workphone
{
    //--------------------------------------------
    WP_NET_API SmartPtr<INetworkManager> WP_CALL_CONV createRakNetServer( const String &filePath )
    {
        auto networkManager = SmartPtr<CNetworkManager>( new CNetworkManager() );
        return networkManager;
    }

    //--------------------------------------------
    WP_NET_API SmartPtr<INetworkManager> WP_CALL_CONV createRakNetClient( const String &filePath )
    {
        auto networkManager = SmartPtr<INetworkManager>( new CNetworkManager( true ) );
        return networkManager;
    }
}  // namespace workphone
