#pragma once

#include "WPNavmesh/WPNavmesh.hpp"
#include <cstdint>

namespace worentity
{
    class EntityWorld;
    class Viewport;
}  // namespace worentity

namespace workphone
{

    /**
     * @class NavmeshViewportSettings
     * @brief Per-viewport settings for navmesh visualization and debugging.
     */
    class WPNETWORK_API NavmeshViewportSettings
    {
    public:
        NavmeshViewportSettings();
        virtual ~NavmeshViewportSettings();

        // Settings
        bool m_drawDebug = false;               // Master toggle for debug drawing
        bool m_drawNavmeshSurface = true;       // Draw navmesh surface
        bool m_drawNavmeshEdges = true;         // Draw navmesh edges
        bool m_drawNavmeshNodes = false;        // Draw navmesh node centers
        bool m_drawNavmeshConnections = false;  // Draw connections between nodes
        bool m_drawPathRequests = true;         // Draw active pathfinding requests
        bool m_drawInclusionVolumes = true;     // Draw inclusion volumes
        bool m_drawExclusionVolumes = true;     // Draw exclusion volumes

        // Visualization options
        uint32_t m_layerMask = 0xFFFFFFFF;    // Which layers to visualize
        float m_debugDrawDistance = 1000.0f;  // Max distance for debug drawing

        // Colors
        float m_navmeshColor[4] = { 0.2f, 0.6f, 1.0f, 0.5f };          // Navmesh surface color
        float m_edgeColor[4] = { 0.0f, 0.8f, 1.0f, 1.0f };             // Edge color
        float m_inclusionVolumeColor[4] = { 0.0f, 1.0f, 0.0f, 0.3f };  // Inclusion volume color
        float m_exclusionVolumeColor[4] = { 1.0f, 0.0f, 0.0f, 0.3f };  // Exclusion volume color
    };

    /**
     * @class NavmeshSettings
     * @brief Global navmesh settings.
     */
    class WPNETWORK_API NavmeshSettings
    {
    public:
        static NavmeshSettings &Get();

        // Global enable/disable
        bool m_navigationEnabled = true;

        // Default build settings
        float m_defaultAgentRadius = 0.45f;
        float m_defaultAgentHeight = 2.0f;
        float m_defaultAgentMaxSlope = 40.0f;
        float m_defaultAgentStepHeight = 0.2f;

        // Debug settings
        bool m_autoUpdateDebugDraw = true;

    private:
        NavmeshSettings();
        ~NavmeshSettings();

        NavmeshSettings( const NavmeshSettings & ) = delete;
        NavmeshSettings &operator=( const NavmeshSettings & ) = delete;
    };

}  // namespace workphone
