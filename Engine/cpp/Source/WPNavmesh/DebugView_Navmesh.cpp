#include "WPNavmesh/WPNavmeshPCH.hpp"
#include "WPNavmesh/DebugView_Navmesh.hpp"

namespace workphone
{

    NavmeshDebugView::NavmeshDebugView()
    {
    }

    NavmeshDebugView::~NavmeshDebugView()
    {
        EE_ASSERT( m_pNavmeshSystem == nullptr && "DebugView must be shutdown before destruction" );
    }

    void NavmeshDebugView::Initialize( NavmeshWorldSystem *pNavmeshSystem )
    {
        EE_ASSERT( pNavmeshSystem != nullptr );
        EE_ASSERT( m_pNavmeshSystem == nullptr && "DebugView already initialized" );

        m_pNavmeshSystem = pNavmeshSystem;
    }

    void NavmeshDebugView::Shutdown()
    {
        m_pNavmeshSystem = nullptr;
    }

    void NavmeshDebugView::DrawMenu( const worentity::EntityWorldUpdateContext &context )
    {
        // This would typically draw an ImGui menu with debug options
        // For now, this is a placeholder that can be extended
    }

}  // namespace workphone
