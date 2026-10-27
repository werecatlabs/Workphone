#ifndef WorkphoneProjectLocalization_hpp
#define WorkphoneProjectLocalization_hpp

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{
    WPCore_API String getProjectLanguageTag();
    // Dictionaries are flat UTF-8 JSON objects in Localization/<language>.json.
    // Missing translations fall back to English, then the supplied text or key.
    WPCore_API String localizeProjectString( const String &key, const String &fallback );
    // Text widgets retain their source token so switching language updates live.
    WPCore_API String localizeProjectText( const String &text );
    WPCore_API void releaseProjectLocalization();
}
#endif
