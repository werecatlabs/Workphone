#pragma once

#include "WPNavmesh/WPNavmesh.hpp"
#include "WPNavmesh/NavmeshData.hpp"
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Transform.hpp>
#include <Workphone/Memory/SmartPtr.hpp>

// Forward declarations for entity system
namespace worentity
{
    class Entity;
    class EntityComponent;
}  // namespace worentity

namespace workphone
{

    /**
     * @struct NavmeshLayerBuildSettings
     * @brief Per-layer build settings for navmesh generation.
     *        All settings are exposed to the editor for configuration.
     */
    struct WPNETWORK_API NavmeshLayerBuildSettings
    {
        // Metadata
        uint32_t m_layerID = 0;  // User supplied layer ID

        // Voxel settings
        float m_voxSize = 0.1f;  // Size of auto-generation voxels

        // Agent settings
        float m_radius = 0.45f;        // Body radius of creatures using this navmesh
        float m_height = 2.0f;         // Clearance height for creatures
        float m_step = 0.2f;           // Maximum step height for surface movement
        float m_dropOffRadius = 0.2f;  // Distance to retract from drop-offs/ledges

        // Slope settings
        float m_maxWalkableSlope = 40.0f;  // Maximum walkable slope in degrees

        // Island filtering
        int32_t m_maxNumIslands = 0;                     // Max islands (0 = disabled)
        float m_minIslandSurfaceArea = 2.0f;             // Minimum island area to keep
        bool m_leaveSmallIslandsTouchingPortals = true;  // Keep islands touching portals

        // Optimization
        float m_additionalInwardsSmoothingDist = 0.0f;  // Additional edge smoothing distance
        bool m_useEnhancedTerrainTracking = true;       // Better terrain following
        bool m_optimizeForAxisAligned = false;          // Optimize for axis-aligned levels
        bool m_tessellateForPathingAccuracy = false;    // More accurate pathing at cost of performance

        // Vertical offset
        float m_verticalOffsetDist = 0.1f;  // Vertical offset from ground
    };

    /**
     * @struct NavmeshBuildSettings
     * @brief Complete build settings for a navmesh.
     */
    struct WPNETWORK_API NavmeshBuildSettings
    {
        NavmeshLayerBuildSettings m_defaultLayerSettings;

        WPCore::Array<NavmeshLayerBuildSettings> m_additionalLayers;

        bool m_enableBuildLogging = false;
    };

    /**
     * @class NavmeshComponent
     * @brief Entity component for attaching a navmesh to an entity.
     *        Provides editor-exposed build settings and navmesh data binding.
     */
    class WPNETWORK_API NavmeshComponent
    {
    public:
        NavmeshComponent();
        NavmeshComponent( const NavPath &navmeshResourcePath );
        ~NavmeshComponent();

        // Navmesh data
        bool HasNavmeshData() const;
        NavmeshData *GetNavmeshData() const
        {
            return m_navmeshData;
        }
        void SetNavmeshData( NavmeshDataPtr navmeshData );
        void SetNavmeshResourcePath( const NavPath &path );
        const NavPath &GetNavmeshResourcePath() const
        {
            return m_resourcePath;
        }

        // Build settings (exposed to editor)
        NavmeshBuildSettings &GetBuildSettings()
        {
            return m_buildSettings;
        }
        const NavmeshBuildSettings &GetBuildSettings() const
        {
            return m_buildSettings;
        }
        void SetBuildSettings( const NavmeshBuildSettings &settings )
        {
            m_buildSettings = settings;
        }

        // Layer settings helpers
        NavmeshLayerBuildSettings &GetDefaultLayerSettings()
        {
            return m_buildSettings.m_defaultLayerSettings;
        }
        const NavmeshLayerBuildSettings &GetDefaultLayerSettings() const
        {
            return m_buildSettings.m_defaultLayerSettings;
        }

        void AddAdditionalLayer( const NavmeshLayerBuildSettings &layer );
        void RemoveAdditionalLayer( uint32_t index );

        // Transform
        const WPCore::Transform &GetWorldTransform() const
        {
            return m_worldTransform;
        }
        void SetWorldTransform( const WPCore::Transform &transform )
        {
            m_worldTransform = transform;
        }

        // Entity binding
        void SetOwnerEntity( worentity::Entity *entity )
        {
            m_ownerEntity = entity;
        }
        worentity::Entity *GetOwnerEntity() const
        {
            return m_ownerEntity;
        }

        // Component ID
        void SetID( uint64_t id )
        {
            m_componentID = id;
        }
        uint64_t GetID() const
        {
            return m_componentID;
        }

    private:
        worentity::Entity *m_ownerEntity = nullptr;
        uint64_t m_componentID = 0;

        NavPath m_resourcePath;
        NavmeshDataPtr m_navmeshData;

        NavmeshBuildSettings m_buildSettings;
        WPCore::Transform m_worldTransform;
    };

}  // namespace workphone
