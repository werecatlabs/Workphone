#ifndef ProjectSettingsInteractionBackend_hpp
#define ProjectSettingsInteractionBackend_hpp

#include <EditorPrerequisites.hpp>

namespace workphone::editor
{
    void describeProjectSettingsInteractionCapabilities( SmartPtr<Properties> response );
    SmartPtr<Properties> validateProjectSettingsInteraction( SmartPtr<Properties> settings );
    /** Applies native physics, audio mixer and input settings. Device selection
     * and mastering rate are consumed on audio startup; reports pendingKeys when
     * a subsystem or endpoint is absent. Validation never mutates managers. */
    SmartPtr<Properties> applyProjectSettingsInteraction( SmartPtr<Properties> settings );
}

#endif
