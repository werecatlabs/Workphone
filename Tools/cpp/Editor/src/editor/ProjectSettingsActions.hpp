#ifndef WorkphoneEditorProjectSettingsActions_hpp
#define WorkphoneEditorProjectSettingsActions_hpp

#include <EditorPrerequisites.hpp>

namespace workphone::editor
{
    // Requests and responses use typed, dotted project settings at the root.
    // Import returns a validated draft; Apply and Save commit that draft.
    void processProjectSettingsAction( SmartPtr<Properties> request );
}

#endif
