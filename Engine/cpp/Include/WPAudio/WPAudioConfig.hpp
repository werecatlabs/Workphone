#ifndef __WPAudioConfig_h__
#define __WPAudioConfig_h__

#include <Workphone/WorkphoneConfig.hpp>

#ifdef WP_PLATFORM_WIN32
#    ifndef _WP_STATIC_LIB_
#        ifdef WPAudio_EXPORTS
#            define WPAudio_API __declspec( dllexport )
#        else
#            define WPAudio_API __declspec( dllimport )
#        endif  // WPAudio_EXPORTS
#    else
#        define WPAudio_API
#    endif  // _WP_STATIC_LIB_
#else
#    define WPAudio_API
#endif

#endif  // WPAudioConfig_h__
