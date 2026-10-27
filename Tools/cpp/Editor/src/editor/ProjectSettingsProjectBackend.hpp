#pragma once
#include <EditorPrerequisites.hpp>

namespace workphone::editor
{
    SmartPtr<Properties> validateProjectSettingsProjectBackend( SmartPtr<Properties> settings );
    void describeProjectSettingsProjectBackend( SmartPtr<Properties> response );
    SmartPtr<Properties> applyProjectSettingsProjectBackend( SmartPtr<Properties> settings );
    /** Refresh service/network adapters after the active immutable profile is published. */
    void updateProjectSettingsProjectBackend();
    /** Materializes an executable CMake profile and checked staging manifest, without running tools. */
    Array<String> prepareProjectSettingsBuild( SmartPtr<Project> project, SmartPtr<Properties> settings );
    SmartPtr<Properties> buildProjectSettingsProjectBackend( SmartPtr<Properties> settings );
}
