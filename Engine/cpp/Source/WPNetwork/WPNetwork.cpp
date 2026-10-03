#include <WPNetwork/WPNetwork.hpp>

namespace workphone
{
    SmartPtr<INetworkManager> WP_CALL_CONV createWPNetworkServer( const String &filePath )
    {
        (void)filePath;
        return SmartPtr<INetworkManager>( new WPNetworkManager( false ) );
    }

    SmartPtr<INetworkManager> WP_CALL_CONV createWPNetworkClient( const String &filePath )
    {
        (void)filePath;
        return SmartPtr<INetworkManager>( new WPNetworkManager( true ) );
    }
}  // namespace workphone
