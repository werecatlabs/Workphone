#ifndef __WPRakNet__H
#define __WPRakNet__H

#define _WINSOCKAPI_  // stops windows.h including winsock.h

#include "Workphone/WorkphoneAutolink.hpp"

#if WP_USE_AUTO_LINK
#    ifdef _DEBUG
#        pragma comment( lib, "RakNetLibStaticDebug.lib" )
#        pragma comment( lib, "ws2_32.lib" )
#        pragma comment( lib, "WPRakNet_d.lib" )
#    elif NDEBUG
#        pragma comment( lib, "RakNetLibStatic.lib" )
#        pragma comment( lib, "ws2_32.lib" )
#        pragma comment( lib, "WPRakNet.lib" )
#    else
#        pragma comment( lib, "RakNetLibStatic.lib" )
#        pragma comment( lib, "ws2_32.lib" )
#        pragma comment( lib, "WPRakNet.lib" )
#    endif
#endif

#include "Workphone/Interface/Net/INetworkManager.hpp"
#include "WPRakNet/WPRakNetMessageIdentifiers.hpp"
//#include <MessageIdentifiers.hpp>

#ifndef _WP_STATIC_LIB_
#    define WP_NET_API __declspec( dllexport )
#else
#    define WP_NET_API
#endif

namespace workphone
{
    WP_NET_API SmartPtr<INetworkManager> WP_CALL_CONV createRakNetServer( const String &filePath );
    WP_NET_API SmartPtr<INetworkManager> WP_CALL_CONV createRakNetClient( const String &filePath );
}  // namespace workphone

#endif
