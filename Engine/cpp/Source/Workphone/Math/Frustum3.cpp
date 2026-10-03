#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/Frustum3.hpp>

namespace workphone
{
    template <class T>
    Frustum3<T>::Frustum3() = default;

    template <class T>
    Frustum3<T>::~Frustum3() = default;

    template <class T>
    void Frustum3<T>::setViewProjection( const Matrix4<T> &viewProjection )
    {
        m_planes[static_cast<u8>( FrustumPlane::FRUSTUM_PLANE_LEFT )].getNormal().X() =
            viewProjection[3][0] + viewProjection[0][0];
        m_planes[static_cast<u8>( FrustumPlane::FRUSTUM_PLANE_LEFT )].getNormal().Y() =
            viewProjection[3][1] + viewProjection[0][1];
        m_planes[static_cast<u8>( FrustumPlane::FRUSTUM_PLANE_LEFT )].getNormal().Z() =
            viewProjection[3][2] + viewProjection[0][2];
        m_planes[static_cast<u8>( FrustumPlane::FRUSTUM_PLANE_LEFT )].setDistance(
            viewProjection[3][3] + viewProjection[0][3] );

        m_planes[static_cast<u8>( FrustumPlane::FRUSTUM_PLANE_RIGHT )].getNormal().X() =
            viewProjection[3][0] - viewProjection[0][0];
        m_planes[static_cast<u8>( FrustumPlane::FRUSTUM_PLANE_RIGHT )].getNormal().Y() =
            viewProjection[3][1] - viewProjection[0][1];
        m_planes[static_cast<u8>( FrustumPlane::FRUSTUM_PLANE_RIGHT )].getNormal().Z() =
            viewProjection[3][2] - viewProjection[0][2];
        m_planes[static_cast<u8>( FrustumPlane::FRUSTUM_PLANE_RIGHT )].setDistance(
            viewProjection[3][3] - viewProjection[0][3] );

        m_planes[static_cast<u8>( FrustumPlane::FRUSTUM_PLANE_TOP )].getNormal().X() =
            viewProjection[3][0] - viewProjection[1][0];
        m_planes[static_cast<u8>( FrustumPlane::FRUSTUM_PLANE_TOP )].getNormal().Y() =
            viewProjection[3][1] - viewProjection[1][1];
        m_planes[static_cast<u8>( FrustumPlane::FRUSTUM_PLANE_TOP )].getNormal().Z() =
            viewProjection[3][2] - viewProjection[1][2];
        m_planes[static_cast<u8>( FrustumPlane::FRUSTUM_PLANE_TOP )].setDistance( viewProjection[3][3] -
                                                                                  viewProjection[1][3] );

        m_planes[static_cast<u8>( FrustumPlane::FRUSTUM_PLANE_BOTTOM )].getNormal().X() =
            viewProjection[3][0] + viewProjection[1][0];
        m_planes[static_cast<u8>( FrustumPlane::FRUSTUM_PLANE_BOTTOM )].getNormal().Y() =
            viewProjection[3][1] + viewProjection[1][1];
        m_planes[static_cast<u8>( FrustumPlane::FRUSTUM_PLANE_BOTTOM )].getNormal().Z() =
            viewProjection[3][2] + viewProjection[1][2];
        m_planes[static_cast<u8>( FrustumPlane::FRUSTUM_PLANE_BOTTOM )].setDistance(
            viewProjection[3][3] + viewProjection[1][3] );

        m_planes[static_cast<u8>( FrustumPlane::FRUSTUM_PLANE_NEAR )].getNormal().X() =
            viewProjection[3][0] + viewProjection[2][0];
        m_planes[static_cast<u8>( FrustumPlane::FRUSTUM_PLANE_NEAR )].getNormal().Y() =
            viewProjection[3][1] + viewProjection[2][1];
        m_planes[static_cast<u8>( FrustumPlane::FRUSTUM_PLANE_NEAR )].getNormal().Z() =
            viewProjection[3][2] + viewProjection[2][2];
        m_planes[static_cast<u8>( FrustumPlane::FRUSTUM_PLANE_NEAR )].setDistance(
            viewProjection[3][3] + viewProjection[2][3] );

        m_planes[static_cast<u8>( FrustumPlane::FRUSTUM_PLANE_FAR )].getNormal().X() =
            viewProjection[3][0] - viewProjection[2][0];
        m_planes[static_cast<u8>( FrustumPlane::FRUSTUM_PLANE_FAR )].getNormal().Y() =
            viewProjection[3][1] - viewProjection[2][1];
        m_planes[static_cast<u8>( FrustumPlane::FRUSTUM_PLANE_FAR )].getNormal().Z() =
            viewProjection[3][2] - viewProjection[2][2];
        m_planes[static_cast<u8>( FrustumPlane::FRUSTUM_PLANE_FAR )].setDistance( viewProjection[3][3] -
                                                                                  viewProjection[2][3] );

        // Renormalise any normals which were not unit length
        for( int i = 0; i < 6; i++ )
        {
            T length = m_planes[i].getNormal().normaliseLength();
            m_planes[i].setDistance( m_planes[i].getDistance() / length );
        }
    }

    template <class T>
    bool Frustum3<T>::intersects( const Vector3<T> &centre, const Vector3<T> &extents ) const
    {
        // For each plane, see if all points are on the negative side
        // If so, object is not visible
        for( s32 plane = 0; plane < 6; ++plane )
        {
            PlaneSide side = m_planes[plane].getSide( centre, extents );
            if( side == PlaneSide::NEGATIVE_SIDE )
            {
                return false;
            }
        }

        return true;
    }

    // explicit instantiation
    template class Frustum3<f32>;
    template class Frustum3<f64>;
}  // namespace workphone
