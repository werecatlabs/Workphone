#ifndef ProjectSettingsRuntime_h__
#define ProjectSettingsRuntime_h__

#include <EditorPrerequisites.hpp>

namespace workphone::editor
{
    /** Describes the implemented project settings, including their application boundary. */
    void describeProjectSettingsCapabilities( SmartPtr<Properties> response );

    /** Validates all implemented runtime options without changing any engine objects. */
    SmartPtr<Properties> validateProjectSettingsRuntime( SmartPtr<Properties> settings );

    /** Copies supported game graphics options into the project's existing graphics director. */
    void updateProjectGraphicsSettings(
        SmartPtr<Properties> settings, SmartPtr<scene::GraphicsSettingsDirector> director );

    /**
     * Validates supported options before applying live settings. The result contains success,
     * status, appliedKeys and unavailableKeys. Call on the primary editor thread.
     */
    SmartPtr<Properties> applyProjectSettingsRuntime( SmartPtr<Properties> settings );
}

#endif
