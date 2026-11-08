#ifndef ProjectSettingsGraphicsBackend_h__
#define ProjectSettingsGraphicsBackend_h__
#include <EditorPrerequisites.hpp>
namespace workphone::editor
{
    void describeProjectSettingsGraphicsBackend( SmartPtr<Properties> response );
    SmartPtr<Properties> validateProjectSettingsGraphicsBackend( SmartPtr<Properties> settings );
    SmartPtr<Properties> applyProjectSettingsGraphicsBackend( SmartPtr<Properties> settings,
                                                             const Properties *changes = nullptr );
    SmartPtr<Properties> projectGraphicsBackendProperties( SmartPtr<Properties> settings,
                                                          const Properties *changes = nullptr );
}
#endif
