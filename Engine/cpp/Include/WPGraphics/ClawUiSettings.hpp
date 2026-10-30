#pragma once

#include <WPGraphics/WPClawHammerPrerequisites.hpp>

namespace workphone::render
{
    /** Configure before graphics/UI initialization. Defaults to enabled and is
     * retained across context recreation. Disabling prevents automatic ImGui
     * settings disk reads/writes and removes its default log file destination.
     * Existing contexts are unchanged. */
    WPGraphics_API void setClawUiSettingsPersistence( bool enabled );
}
