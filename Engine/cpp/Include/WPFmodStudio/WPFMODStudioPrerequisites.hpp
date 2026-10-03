#ifndef WPFMODStudioPrerequisites_h__
#define WPFMODStudioPrerequisites_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/WorkphoneConfig.hpp>

#ifdef WP_PLATFORM_WIN32
#    ifndef _WP_STATIC_LIB_
#        ifdef WPFMODStudio_EXPORTS
#            define WPFMODStudio_API __declspec( dllexport )
#        else
#            define WPFMODStudio_API __declspec( dllimport )
#        endif  // WP_EXPORT
#    else
#        define WPFMODStudio_API
#    endif  // _WP_STATIC_LIB_
#else
#    define WPFMODStudio_API
#endif

namespace workphone
{
}  // namespace workphone

#endif  // WPFMODStudioPrerequisites_h__
