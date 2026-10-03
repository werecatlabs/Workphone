#ifndef WPNetwork_h__
#define WPNetwork_h__

#include <WPNetwork/WPNetworkManager.hpp>
#include <WPNetwork/WPNetworkPacket.hpp>
#include <WPNetwork/WPNetworkSystemAddress.hpp>
#include <WPNetwork/WPNetworkListener.hpp>
#include <WPNetwork/WPNetworkPlayer.hpp>
#include <WPNetwork/WPNetworkStream.hpp>
#include <WPNetwork/WPNetworkView.hpp>

namespace workphone
{
    WPNetwork_API SmartPtr<INetworkManager> WP_CALL_CONV createWPNetworkServer( const String &filePath );
    WPNetwork_API SmartPtr<INetworkManager> WP_CALL_CONV createWPNetworkClient( const String &filePath );
}  // namespace workphone

#endif  // WPNetwork_h__
