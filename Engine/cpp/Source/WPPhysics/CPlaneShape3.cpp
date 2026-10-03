#include <WPPhysics/WPPhysicsPCH.hpp>
#include <WPPhysics/CPlaneShape3.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::physics
{
    CPlaneShape3::CPlaneShape3() : CPhysicsShape3Adapter( WORKPHONE_COLLISION_SHAPE_PLANE )
    {
    }

    real_Num CPlaneShape3::getDistance() const
    {
        return static_cast<real_Num>( wp_collision_shape_get_plane_offset( getShape() ) );
    }

    void CPlaneShape3::setDistance( real_Num distance )
    {
        wp_collision_shape_set_plane( getShape(), detail::toWp( getNormal() ),
                                      static_cast<wp_f32>( distance ) );
    }

    Vector3<real_Num> CPlaneShape3::getNormal() const
    {
        return detail::fromWp( wp_collision_shape_get_plane_normal( getShape() ) );
    }

    void CPlaneShape3::setNormal( const Vector3<real_Num> &normal )
    {
        auto normalized = normal;
        if( normalized.lengthSquared() <= Math<real_Num>::epsilon() )
        {
            WP_LOG_WARNING( "CPlaneShape3::setNormal: zero-length normal rejected." );
            return;
        }
        normalized.normalise();
        wp_collision_shape_set_plane( getShape(), detail::toWp( normalized ),
                                      static_cast<wp_f32>( getDistance() ) );
    }

    Plane3<real_Num> CPlaneShape3::getPlane() const
    {
        auto plane = Plane3<real_Num>();
        plane.setPlane( getNormal(), getDistance() );
        return plane;
    }

    SmartPtr<IPhysicsShape3> CPlaneShape3::clone()
    {
        auto shape = workphone::make_ptr<CPlaneShape3>();
        shape->setNormal( getNormal() );
        shape->setDistance( getDistance() );
        shape->setLocalPose( getLocalPose() );
        shape->setSimulationFilterData( getSimulationFilterData() );
        shape->setMaterial( getMaterial() );
        shape->setEnabled( isEnabled() );
        shape->setTrigger( isTrigger() );
        return shape;
    }
} // namespace workphone::physics
