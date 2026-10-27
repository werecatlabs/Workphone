#ifndef WorkphoneProjectRuntimeSettings_hpp
#define WorkphoneProjectRuntimeSettings_hpp

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{
    // Active profiles are immutable snapshots. Publish a new Properties object
    // rather than changing a bag while worker/audio/render threads read it.
    WPCore_API void loadProjectRuntimeSettings( const String &projectDirectory );
    WPCore_API u32 projectWorkerCount( u32 applicationDefault );
    WPCore_API bool projectRunsBackgroundTasks();
    WPCore_API void applyProjectWorkerPolicy( SmartPtr<Properties> settings, u32 workerIndex );
    WPCore_API bool projectLogEnabled( u32 severity );
    WPCore_API bool projectProfilingEnabled();
}

#endif
