#ifndef FBAudioPrerequisites_h__
#define FBAudioPrerequisites_h__

#include <WPAudio/WPAudioConfig.hpp>
#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/WorkphoneConfig.hpp>

#ifndef WP_USE_WASAPI
#    define WP_USE_WASAPI 0
#endif

#ifndef WP_USE_XAUDIO2
#    if defined WP_PLATFORM_WIN32
#        define WP_USE_XAUDIO2 1
#    else
#        define WP_USE_XAUDIO2 0
#    endif
#endif

#if defined WP_PLATFORM_WIN32 && WP_USE_XAUDIO2
#    if defined _WIN32_WINNT && _WIN32_WINNT < 0x0A00
#        undef _WIN32_WINNT
#    endif
#    ifndef _WIN32_WINNT
#        define _WIN32_WINNT 0x0A00  // Windows 10 is required by the Windows SDK XAudio2 headers.
#    endif
#endif

#if defined WP_PLATFORM_WIN32
#    ifndef EXTERN_C
#        define EXTERN_C extern "C"
#    endif
#    ifndef DECLSPEC_SELECTANY
#        define DECLSPEC_SELECTANY __declspec( selectany )
#    endif
#    include <guiddef.h>
#endif

namespace workphone
{
}  // namespace workphone

#endif  //
