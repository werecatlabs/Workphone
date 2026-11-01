#pragma once
#include <WPGraphics/WPClawHammerPrerequisites.hpp>

namespace workphone::render
{
    class ClawImguiManager;
    namespace detail
    {
        // CPP-private recovery shared by the production frame driver and its
        // native contract host, which compiles the same internal manager CPP.
#if defined(WP_IMGUI_PRIVATE_CONTRACT_HOST)
        void discardInterruptedImGuiFrame(ClawImguiManager &);
#else
        WPGraphics_API void discardInterruptedImGuiFrame(ClawImguiManager &);
#endif
    }
}
