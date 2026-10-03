#include "WPNavmesh/WPNavmeshPCH.hpp"
#include "WPNavmesh/System_NavmeshWorld.hpp"
#include <Workphone/Math/Math.hpp>

namespace workphone
{

    NavmeshWorldSystem::NavmeshWorldSystem()
    {
    }

    NavmeshWorldSystem::~NavmeshWorldSystem()
    {
        WP_ASSERT( !m_isInitialized && "NavmeshWorldSystem must be shut down before destruction" );
    }

    void NavmeshWorldSystem::Initialize( worentity::EntityWorld *pWorld )
    {
        WP_ASSERT( pWorld != nullptr );
        WP_ASSERT( !m_isInitialized && "NavmeshWorldSystem already initialized" );

        m_pWorld = pWorld;

        // Initialize the underlying navmesh system
        // This would typically create the NavPower instance or similar
        m_navmeshSpace = nullptr;  // TODO: Initialize navmesh space

        m_isInitialized = true;
    }

    void NavmeshWorldSystem::Shutdown()
    {
        if( !m_isInitialized )
        {
            return;
        }

        // Unregister all navmeshes
        for( auto *pComponent : m_registeredNavmeshes )
        {
            UnregisterNavmesh( pComponent );
        }

        // Clear arrays
        m_navmeshComponents.Clear();
        m_registeredNavmeshes.Clear();
        m_inclusionVolumes.Clear();
        m_exclusionVolumes.Clear();

        // Shutdown underlying system
        m_navmeshSpace = nullptr;

        m_pWorld = nullptr;
        m_isInitialized = false;
    }

    void NavmeshWorldSystem::RegisterComponent( NavmeshComponent *pComponent )
    {
        if( !pComponent )
        {
            return;
        }

        m_navmeshComponents.Add( pComponent );

        // If component already has navmesh data, register it
        if( pComponent->HasNavmeshData() )
        {
            RegisterNavmesh( pComponent );
        }
    }

    void NavmeshWorldSystem::UnregisterComponent( NavmeshComponent *pComponent )
    {
        if( !pComponent )
        {
            return;
        }

        // Unregister if registered
        if( m_registeredNavmeshes.Contains( pComponent ) )
        {
            UnregisterNavmesh( pComponent );
        }

        m_navmeshComponents.Remove( pComponent );
    }

    void NavmeshWorldSystem::RegisterNavmesh( NavmeshComponent *pComponent )
    {
        WP_ASSERT( pComponent != nullptr );
        WP_ASSERT( pComponent->HasNavmeshData() );

        if( m_registeredNavmeshes.Contains( pComponent ) )
        {
            return;  // Already registered
        }

        m_registeredNavmeshes.Add( pComponent );

        // TODO: Add navmesh to the underlying system
        // This would typically:
        // 1. Copy the navmesh data
        // 2. Apply the component's world transform
        // 3. Add to the navmesh space
    }

    void NavmeshWorldSystem::UnregisterNavmesh( NavmeshComponent *pComponent )
    {
        WP_ASSERT( pComponent != nullptr );

        if( !m_registeredNavmeshes.Contains( pComponent ) )
        {
            return;
        }

        m_registeredNavmeshes.Remove( pComponent );

        // TODO: Remove navmesh from the underlying system
    }

    void NavmeshWorldSystem::Update( const worentity::EntityWorldUpdateContext &context )
    {
        // Update navmesh system (e.g., dynamic obstacles)
        // TODO: Call navmesh system update

        // Check for components that now have navmesh data and should be registered
        for( auto *pComponent : m_navmeshComponents )
        {
            if( pComponent->HasNavmeshData() && !m_registeredNavmeshes.Contains( pComponent ) )
            {
                RegisterNavmesh( pComponent );
            }
        }
    }

    WPCore::BoundingBox NavmeshWorldSystem::GetNavmeshBounds( uint32_t layerIndex ) const
    {
        WPCore::BoundingBox bounds;

        // TODO: Query underlying navmesh system for bounds
        // For now, compute from registered components
        for( auto *pComponent : m_registeredNavmeshes )
        {
            if( pComponent->GetNavmeshData() )
            {
                WPCore::BoundingBox componentBounds = pComponent->GetNavmeshData()->GetBounds();
                bounds.Merge( componentBounds );
            }
        }

        return bounds;
    }

    bool NavmeshWorldSystem::FindPath( WPCore::Vector3 start, WPCore::Vector3 end, NavPath &outPath )
    {
        // TODO: Implement pathfinding using underlying navmesh system
        // This would typically call NavPower or similar

        // Placeholder: Create a straight line path
        if( ( end - start ).LengthSquared() > 0.0001f )
        {
            outPath = NavPath( start, end );
            return true;
        }

        return false;
    }

    bool NavmeshWorldSystem::IsPointOnNavmesh( WPCore::Vector3 point, uint32_t layerMask ) const
    {
        // TODO: Implement point-on-navmesh query
        return false;
    }

    bool NavmeshWorldSystem::GetClosestPointOnNavmesh( WPCore::Vector3 point,
                                                       WPCore::Vector3 &outClosestPoint,
                                                       uint32_t layerMask ) const
    {
        // TODO: Implement closest point query
        return false;
    }

    bool NavmeshWorldSystem::Raycast( WPCore::Vector3 origin, WPCore::Vector3 direction,
                                      float maxDistance, WPCore::Vector3 &outHitPoint,
                                      float &outHitDistance, uint32_t layerMask ) const
    {
        // TODO: Implement navmesh raycast
        return false;
    }

}  // namespace workphone
