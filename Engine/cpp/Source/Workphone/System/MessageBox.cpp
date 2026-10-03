#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/MessageBox.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/StringUtil.hpp>

#if defined WP_PLATFORM_WIN32
#    ifdef _MSC_VER
//#define WIN32_LEAN_AND_MEAN
#        include <wtypes.h>
//#include <winnt.hpp>
#        include <WinUser.h>
#    endif
#endif

namespace workphone
{
    void MessageBoxUtil::show( const String &text )
    {
        show( text.c_str() );
    }

    void MessageBoxUtil::show( const char *text )
    {
#ifdef WP_PLATFORM_WIN32
        auto str = StringUtil::getWString( text );
        MessageBoxW( nullptr, str.c_str(), L"", MB_OK );
        WP_LOG( text );
#else
        WP_LOG( text );
#endif
    }
}  // namespace workphone
