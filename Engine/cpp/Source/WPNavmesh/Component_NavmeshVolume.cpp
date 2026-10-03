#include "WPNavmesh/WPNavmeshPCH.hpp"
#include "WPNavmesh/Component_NavmeshVolume.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{

    //=============================================================================
    // NavmeshVolumeComponent
    //=============================================================================

    NavmeshVolumeComponent::NavmeshVolumeComponent()
    {
    }

    NavmeshVolumeComponent::~NavmeshVolumeComponent()
    {
    }

    OBB3F NavmeshVolumeComponent::GetLocalBounds() const
    {
        return OBB3F( Vector3F::zero(), m_localExtents, QuaternionF::identity() );
    }

    OBB3F NavmeshVolumeComponent::GetWorldBounds() const
    {
        OBB3F localBounds = GetLocalBounds();

        // Transform OBB by world transform
        WPCore::Matrix4x4 transformMat = m_worldTransform.ToMatrix();

        OBB3F worldBounds;
        worldBounds.m_center = transformMat.TransformPoint( localBounds.m_center );

        // Transform half extents (simplified - doesn't account for rotation properly)
        WPCore::Vector3 scaledExtents = localBounds.m_halfExtents;
        worldBounds.m_halfExtents = m_worldTransform.GetScale() * scaledExtents;

        // Transform orientation
        worldBounds.m_rotation = m_worldTransform.GetRotation() * localBounds.m_rotation;

        return worldBounds;
    }

    //=============================================================================
    // NavmeshInclusionVolumeComponent
    //=============================================================================

    NavmeshInclusionVolumeComponent::NavmeshInclusionVolumeComponent()
    {
    }

    NavmeshInclusionVolumeComponent::~NavmeshInclusionVolumeComponent()
    {
    }

    //=============================================================================
    // NavmeshExclusionVolumeComponent
    //=============================================================================

    NavmeshExclusionVolumeComponent::NavmeshExclusionVolumeComponent()
    {
    }

    NavmeshExclusionVolumeComponent::~NavmeshExclusionVolumeComponent()
    {
    }

}  // namespace workphone
