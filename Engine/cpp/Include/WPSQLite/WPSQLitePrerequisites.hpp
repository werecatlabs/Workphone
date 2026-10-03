#ifndef WPSQLitePrerequisites_h__
#define WPSQLitePrerequisites_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/WorkphoneConfig.hpp>

#ifdef WP_PLATFORM_WIN32
#    ifndef _WP_STATIC_LIB_
#        ifdef WPSQLite_EXPORTS
#            define WPSQLite_API __declspec( dllexport )
#        else
#            define WPSQLite_API __declspec( dllimport )
#        endif  // WP_EXPORT
#    else
#        define WPSQLite_API
#    endif  // _WP_STATIC_LIB_
#else
#    define WPSQLite_API
#endif

// forward decs
class CppSQLite3DB;

#endif  // WPSQLitePrerequisites_h__
