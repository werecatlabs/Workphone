#include "WPNavmesh/WPNavmeshPCH.hpp"
#include "WPNavmesh/Settings_NavmeshViewport.hpp"

namespace workphone
{

    //=============================================================================
    // NavmeshViewportSettings
    //=============================================================================

    NavmeshViewportSettings::NavmeshViewportSettings()
    {
    }

    NavmeshViewportSettings::~NavmeshViewportSettings()
    {
    }

    //=============================================================================
    // NavmeshSettings
    //=============================================================================

    NavmeshSettings::NavmeshSettings()
    {
        m_navigationEnabled = true;
        m_defaultAgentRadius = 0.45f;
        m_defaultAgentHeight = 2.0f;
        m_defaultAgentMaxSlope = 40.0f;
        m_defaultAgentStepHeight = 0.2f;
        m_autoUpdateDebugDraw = true;
    }

    NavmeshSettings::~NavmeshSettings()
    {
    }

    NavmeshSettings &NavmeshSettings::Get()
    {
        static NavmeshSettings s_instance;
        return s_instance;
    }

}  // namespace workphone
