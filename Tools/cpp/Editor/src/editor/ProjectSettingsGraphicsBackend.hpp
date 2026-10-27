#ifndef ProjectSettingsGraphicsBackend_h__
#define ProjectSettingsGraphicsBackend_h__
#include <EditorPrerequisites.hpp>
namespace workphone::editor
{
    void describeProjectSettingsGraphicsBackend( SmartPtr<Properties> response );
    SmartPtr<Properties> validateProjectSettingsGraphicsBackend( SmartPtr<Properties> settings );
    SmartPtr<Properties> applyProjectSettingsGraphicsBackend( SmartPtr<Properties> settings );
    SmartPtr<Properties> projectGraphicsBackendProperties( SmartPtr<Properties> settings );
}
#endif
