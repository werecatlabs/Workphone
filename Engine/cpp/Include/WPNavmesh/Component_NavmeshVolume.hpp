#pragma once

#include "WPNavmesh/WPNavmesh.hpp"
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/OBB.hpp>

namespace workphone
{

    /**
     * @class NavmeshVolumeComponent
     * @brief Base class for navmesh volume components.
     *        Used to define areas that affect navmesh generation.
     */
    class WPNETWORK_API NavmeshVolumeComponent
    {
    public:
        NavmeshVolumeComponent();
        virtual ~NavmeshVolumeComponent();

        // Volume properties
        WPCore::Vector3 GetLocalExtents() const
        {
            return m_localExtents;
        }
        void SetLocalExtents( const WPCore::Vector3 &extents )
        {
            m_localExtents = extents;
        }

        const WPCore::Transform &GetWorldTransform() const
        {
            return m_worldTransform;
        }
        void SetWorldTransform( const WPCore::Transform &transform )
        {
            m_worldTransform = transform;
        }

        // Bounds calculation
        OBB3F GetLocalBounds() const;
        OBB3F GetWorldBounds() const;

        // Virtual for derived volume types
        virtual bool IsInclusionVolume() const
        {
            return false;
        }
        virtual bool IsExclusionVolume() const
        {
            return false;
        }

        // Entity binding
        void SetOwnerEntity( void *entity )
        {
            m_ownerEntity = entity;
        }
        void *GetOwnerEntity() const
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

    protected:
        void *m_ownerEntity = nullptr;
        uint64_t m_componentID = 0;

        WPCore::Vector3 m_localExtents = WPCore::Vector3::One;
        WPCore::Transform m_worldTransform;
    };

    /**
     * @class NavmeshInclusionVolumeComponent
     * @brief Volume that marks areas to be included in navmesh generation.
     *        Useful for creating navmesh in specific regions.
     */
    class WPNETWORK_API NavmeshInclusionVolumeComponent : public NavmeshVolumeComponent
    {
    public:
        NavmeshInclusionVolumeComponent();
        virtual ~NavmeshInclusionVolumeComponent();

        virtual bool IsInclusionVolume() const override
        {
            return true;
        }

        // Priority for when multiple inclusion volumes overlap
        uint32_t GetPriority() const
        {
            return m_priority;
        }
        void SetPriority( uint32_t priority )
        {
            m_priority = priority;
        }

    private:
        uint32_t m_priority = 0;
    };

    /**
     * @class NavmeshExclusionVolumeComponent
     * @brief Volume that marks areas to be excluded from navmesh generation.
     *        Useful for carving holes in existing navmesh.
     */
    class WPNETWORK_API NavmeshExclusionVolumeComponent : public NavmeshVolumeComponent
    {
    public:
        NavmeshExclusionVolumeComponent();
        virtual ~NavmeshExclusionVolumeComponent();

        virtual bool IsExclusionVolume() const override
        {
            return true;
        }
    };

}  // namespace workphone
