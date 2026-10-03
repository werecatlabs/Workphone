#pragma once

#include "WPNavmesh/WPNavmesh.hpp"
#include "WPNavmesh/NavmeshData.hpp"
#include "WPNavmesh/Component_Navmesh.hpp"
#include "WPNavmesh/NavmeshPath.hpp"
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Math/BoundingBox.hpp>

namespace worentity
{
    class EntityWorld;
    class EntityWorldUpdateContext;
    struct EntityWorldSystemUpdateContext;
}  // namespace worentity

namespace workphone
{

    // Forward declarations
    class NavmeshComponent;
    class INavmeshQuery;

    /**
     * @class NavmeshWorldSystem
     * @brief Main system for managing navmeshes within a world.
     *        Handles registration, queries, and updates of navmesh components.
     */
    class WPNETWORK_API NavmeshWorldSystem
    {
    public:
        static constexpr const char *GetSystemName()
        {
            return "NavmeshWorldSystem";
        }

    public:
        NavmeshWorldSystem();
        virtual ~NavmeshWorldSystem();

        // Initialization
        void Initialize( worentity::EntityWorld *pWorld );
        void Shutdown();

        // Component registration
        void RegisterComponent( NavmeshComponent *pComponent );
        void UnregisterComponent( NavmeshComponent *pComponent );

        // Update
        void Update( const worentity::EntityWorldUpdateContext &context );

        // Navmesh bounds
        WPCore::BoundingBox GetNavmeshBounds( uint32_t layerIndex = 0 ) const;

        // Query interface
        // Returns true if a path was found from start to end
        bool FindPath( WPCore::Vector3 start, WPCore::Vector3 end, NavPath &outPath );

        // Check if a point is on the navmesh
        bool IsPointOnNavmesh( WPCore::Vector3 point, uint32_t layerMask = 0xFFFFFFFF ) const;

        // Get the closest point on the navmesh to a given point
        bool GetClosestPointOnNavmesh( WPCore::Vector3 point, WPCore::Vector3 &outClosestPoint,
                                       uint32_t layerMask = 0xFFFFFFFF ) const;

        // Raycast on navmesh
        bool Raycast( WPCore::Vector3 origin, WPCore::Vector3 direction, float maxDistance,
                      WPCore::Vector3 &outHitPoint, float &outHitDistance,
                      uint32_t layerMask = 0xFFFFFFFF ) const;

    private:
        void RegisterNavmesh( NavmeshComponent *pComponent );
        void UnregisterNavmesh( NavmeshComponent *pComponent );

        // Internal query helpers
        void *GetNavmeshSpace() const
        {
            return m_navmeshSpace;
        }

    private:
        worentity::EntityWorld *m_pWorld = nullptr;
        void *m_navmeshSpace = nullptr;  // Opaque handle to navmesh system

        // Registered components
        WPCore::Array<NavmeshComponent *> m_navmeshComponents;
        WPCore::Array<NavmeshComponent *> m_registeredNavmeshes;

        // Volume components
        WPCore::Array<NavmeshInclusionVolumeComponent *> m_inclusionVolumes;
        WPCore::Array<NavmeshExclusionVolumeComponent *> m_exclusionVolumes;

        // Update context
        bool m_isInitialized = false;
    };

}  // namespace workphone
