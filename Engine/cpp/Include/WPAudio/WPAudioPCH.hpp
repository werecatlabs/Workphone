#ifndef WPAudioPCH_h__
#define WPAudioPCH_h__

#if WP_USE_PRECOMPILED_HEADERS
// Establish the audio backend and Windows SDK version before platform headers.
#    include <WPAudio/WPAudioPrerequisites.hpp>

#    include <algorithm>
#    include <atomic>
#    include <cmath>
#    include <cstring>
#    include <new>
#    include <vector>

#    include <Workphone/Workphone.hpp>
#endif

#endif  // WPAudioPCH_h__
