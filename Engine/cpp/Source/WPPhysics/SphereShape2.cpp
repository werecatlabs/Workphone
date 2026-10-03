#include "WPPhysics/WPPhysicsPCH.hpp"
#include "WPPhysics/SphereShape2.hpp"
#include <Workphone/Workphone.hpp>
#include <algorithm>

namespace workphone
{
    namespace physics
    {
        // The WorkphonePhysics C API exposes radius as wp_f32; bridge to the C++ real_Num.
        static inline wp_f32 to_wp_f32( real_Num value )
        {
            return static_cast<wp_f32>( static_cast<double>( value ) );
        }

        static inline real_Num from_wp_f32( wp_f32 value )
        {
            return static_cast<real_Num>( static_cast<double>( value ) );
        }

        SphereShape2::SphereShape2()
            : m_shape( wp_collision_shape2_create_sphere( to_wp_f32( 0.01 ) ) )
        {
        }

        SphereShape2::~SphereShape2()
        {
            wp_collision_shape_destroy( m_shape );
            m_shape = nullptr;
        }

        Sphere2<real_Num> SphereShape2::getSphere() const
        {
            return Sphere2<real_Num>( Vector2<real_Num>( 0.0, 0.0 ), static_cast<f32>( getRadius() ) );
        }

        workphone::AABB2<workphone::real_Num> SphereShape2::getAABB() const
        {
            const auto radius = getRadius();
            const auto extent = Vector2<real_Num>( radius, radius );
            return AABB2<real_Num>( -extent, extent );
        }

        void SphereShape2::setRadius( real_Num radius )
        {
            if( !m_shape )
            {
                return;
            }
            const auto safeRadius = std::max( radius, static_cast<real_Num>( 0.001 ) );
            wp_collision_shape2_set_sphere_radius( m_shape, to_wp_f32( safeRadius ) );
        }

        real_Num SphereShape2::getRadius() const
        {
            if( !m_shape )
            {
                return 0.0;
            }
            return from_wp_f32( wp_collision_shape2_get_sphere_radius( m_shape ) );
        }

        u8 SphereShape2::getType() const
        {
            if( !m_shape )
            {
                return static_cast<u8>( WORKPHONE_COLLISION_SHAPE_SPHERE );
            }
            return static_cast<u8>( wp_collision_shape_get_type( m_shape ) );
        }

        void SphereShape2::getPoints( Array<Vector2<real_Num>> &points ) const
        {
            constexpr u32 pointCount = 16;
            const auto    radius = getRadius();
            points.resize( pointCount );
            for( u32 i = 0; i < pointCount; ++i )
            {
                const auto angle = Math<real_Num>::two_pi() * static_cast<real_Num>( i ) /
                                   static_cast<real_Num>( pointCount );
                points[i] = Vector2<real_Num>( Math<real_Num>::Cos( angle ) * radius,
                                              Math<real_Num>::Sin( angle ) * radius );
            }
        }

        void SphereShape2::getPoints( Array<Vector2<real_Num>>   &points,
                                      const Transform2<real_Num> &transform ) const
        {
            getPoints( points );
            const auto cosine = Math<real_Num>::Cos( transform.getRotation() );
            const auto sine = Math<real_Num>::Sin( transform.getRotation() );
            for( auto &point : points )
            {
                const auto x = point.X() * cosine - point.Y() * sine;
                const auto y = point.X() * sine + point.Y() * cosine;
                point = Vector2<real_Num>( x, y ) + transform.getPosition();
            }
        }

        void SphereShape2::computeMass( SmartPtr<IMassData2> massData, real_Num density ) const
        {
            if( !massData )
            {
                return;
            }
            const auto radius = getRadius();
            const auto mass = std::max( density, static_cast<real_Num>( 0.0 ) ) * Math<real_Num>::pi() *
                              radius * radius;
            massData->setMass( static_cast<f32>( mass ) );
            massData->setCenter( getSphere().getCenter() );
            massData->setInertia(
                static_cast<f32>( mass * radius * radius * static_cast<real_Num>( 0.5 ) ) );
        }

        void SphereShape2::_getObject( void **ppObject ) const
        {
            if( ppObject )
            {
                *ppObject = m_shape;
            }
        }

        SmartPtr<Properties> SphereShape2::getProperties() const
        {
            auto properties = workphone::make_ptr<Properties>();
            properties->setProperty( "radius", static_cast<f32>( getRadius() ) );
            properties->setProperty( "enabled", isEnabled() );
            return properties;
        }

        void SphereShape2::setProperties( SmartPtr<Properties> properties )
        {
            if( !properties )
            {
                return;
            }
            auto radius = static_cast<f32>( getRadius() );
            properties->getPropertyValue( "radius", radius );
            auto enabled = isEnabled();
            properties->getPropertyValue( "enabled", enabled );
            setRadius( static_cast<real_Num>( radius ) );
            setEnabled( enabled );
        }

        bool SphereShape2::isEnabled() const
        {
            if( !m_shape )
            {
                return false;
            }
            return wp_collision_shape_is_enabled( m_shape ) != 0;
        }

        void SphereShape2::setEnabled( bool enabled )
        {
            if( !m_shape )
            {
                return;
            }
            wp_collision_shape_set_enabled( m_shape, enabled );
        }

    } // namespace physics
} // namespace workphone
