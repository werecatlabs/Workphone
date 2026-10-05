#ifndef SphereShape2_h__
#define SphereShape2_h__

#include <Workphone/Physics/PhysicsShape2.hpp>
#include <Workphone/Interface/Physics/ISphereShape2.hpp>
#include <algorithm>
#include <cmath>

namespace workphone::physics
{
    /** Backend-independent circle geometry for 2D physics implementations. */
    class SphereShape2 : public PhysicsShape2<ISphereShape2>
    {
    public:
        void setRadius( real_Num radius ) override
        {
            m_radius = std::max( radius, static_cast<real_Num>( 0 ) );
        }

        real_Num getRadius() const override
        {
            return m_radius;
        }

        Sphere2<real_Num> getSphere() const override
        {
            return Sphere2<real_Num>( Vector2<real_Num>::zero(), getRadius() );
        }

        AABB2<real_Num> getAABB() const override
        {
            const auto radius = getRadius();
            return AABB2<real_Num>( Vector2<real_Num>( -radius, -radius ),
                                    Vector2<real_Num>( radius, radius ) );
        }

        void getPoints( Array<Vector2<real_Num>> &points ) const override
        {
            points.clear();
            constexpr u32 count = 16;
            points.reserve( count );
            const auto twoPi = static_cast<real_Num>( 6.28318530717958647692 );
            for( u32 i = 0; i < count; ++i )
            {
                const auto angle = twoPi * static_cast<real_Num>( i ) / count;
                points.emplace_back( std::cos( angle ) * getRadius(),
                                     std::sin( angle ) * getRadius() );
            }
        }

        void computeMass( SmartPtr<IMassData2> massData, real_Num density ) const override
        {
            if( !massData ) return;
            const auto radius = getRadius();
            const auto pi = static_cast<real_Num>( 3.14159265358979323846 );
            const auto mass = pi * radius * radius * std::max( density, static_cast<real_Num>( 0 ) );
            massData->setMass( static_cast<f32>( mass ) );
            massData->setCenter( Vector2<real_Num>::zero() );
            massData->setInertia( static_cast<f32>( static_cast<real_Num>( 0.5 ) * mass * radius * radius ) );
        }

    private:
        real_Num m_radius = static_cast<real_Num>( 0.5 );
    };
}
#endif