#ifndef WorkphoneProjectSettingsEditorBackend_hpp
#define WorkphoneProjectSettingsEditorBackend_hpp
#include <EditorPrerequisites.hpp>
namespace workphone::editor
{
    void describeProjectSettingsEditorBackend( SmartPtr<Properties> response );
    SmartPtr<Properties> validateProjectSettingsEditorBackend( SmartPtr<Properties> settings );
    SmartPtr<Properties> applyProjectSettingsEditorBackend( SmartPtr<Properties> settings );
    void updateProjectSettingsEditorBackend( f64 timeSeconds );
}
#endif
