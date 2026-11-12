#pragma once

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Memory/SmartPtr.hpp>

namespace workphone::editor
{
    /** User credentials are edited by native controls, outside project properties and Lua data. */
    void createOpenAISettingsControls( SmartPtr<ui::IUIElement> parent );
    void refreshOpenAISettingsControls( SmartPtr<ui::IUIElement> parent );
    void createMeshySettingsControls( SmartPtr<ui::IUIElement> parent );
    void refreshMeshySettingsControls( SmartPtr<ui::IUIElement> parent );
    void createTrellis2SettingsControls( SmartPtr<ui::IUIElement> parent );
    void refreshTrellis2SettingsControls( SmartPtr<ui::IUIElement> parent );
}
