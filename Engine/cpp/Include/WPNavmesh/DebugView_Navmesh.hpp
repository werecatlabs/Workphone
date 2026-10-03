#pragma once

#include "WPNavmesh/WPNavmesh.hpp"
#include "WPNavmesh/System_NavmeshWorld.hpp"
#include <Workphone/Math/Vector3.hpp>

namespace worentity
{
    class EntityWorld;
    class EntityWorldUpdateContext;
}  // namespace worentity

namespace workphone
{

    /**
     * @class NavmeshDebugView
     * @brief Debug visualization for navmeshes.
     *        Provides on-screen controls and visual debugging of navmesh data.
     */
    class WPNETWORK_API NavmeshDebugView
    {
    public:
        NavmeshDebugView();
        virtual ~NavmeshDebugView();

        // Initialize with world system
        void Initialize( NavmeshWorldSystem *pNavmeshSystem );
        void Shutdown();

        // Menu/drawing
        void DrawMenu( const worentity::EntityWorldUpdateContext &context );

        // Settings
        bool IsDebugDrawEnabled() const
        {
            return m_drawDebug;
        }
        void SetDebugDrawEnabled( bool enabled )
        {
            m_drawDebug = enabled;
        }

        bool IsDrawRawPath() const
        {
            return m_drawRawPath;
        }
        void SetDrawRawPath( bool draw )
        {
            m_drawRawPath = draw;
        }

        bool IsDrawSmoothPath() const
        {
            return m_drawSmoothPath;
        }
        void SetDrawSmoothPath( bool draw )
        {
            m_drawSmoothPath = draw;
        }

        bool IsDrawNavmeshAreas() const
        {
            return m_drawNavmeshAreas;
        }
        void SetDrawNavmeshAreas( bool draw )
        {
            m_drawNavmeshAreas = draw;
        }

    private:
        NavmeshWorldSystem *m_pNavmeshSystem = nullptr;

        // Debug drawing options
        bool m_drawDebug = false;
        bool m_drawRawPath = true;
        bool m_drawSmoothPath = true;
        bool m_drawNavmeshAreas = false;

        // Runtime settings
        bool m_depthTestEnabled = true;
    };

}  // namespace workphone
