#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Physics/BoxShape2.hpp>
#include <Workphone/State/States/BoxShapeStateData.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Math/MathUtil.hpp>

namespace workphone
{
    namespace physics
    {
        WP_CLASS_REGISTER_DERIVED( workphone::physics, BoxShape2, IBoxShape2 );

        BoxShape2::BoxShape2() = default;

        BoxShape2::~BoxShape2() = default;

        void BoxShape2::load( SmartPtr<ISharedObject> data )
        {
            PhysicsShape2::load( data );
        }

        void BoxShape2::unload( SmartPtr<ISharedObject> data )
        {
            PhysicsShape2::unload( data );
        }

        bool BoxShape2::isValid() const
        {
            if( isLoaded() )
            {
                auto extents = getExtents();
                if( extents.x > 0 && extents.y > 0 )
                {
                    return true;
                }
            }

            return false;
        }

        Vector2<real_Num> BoxShape2::getExtents() const
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->getStateData<BoxShapeStateData>() )
                {
                    return Vector2<real_Num>( state->extents.x, state->extents.y );
                }
            }

            return Vector2<real_Num>::unit();
        }

        void BoxShape2::setExtents( const Vector2<real_Num> &extents )
        {
            if( !MathUtil<real_Num>::equals( extents, getExtents() ) )
            {
                if( auto stateContext = getStateContext() )
                {
                    if( auto state = stateContext->invalidateStateData<BoxShapeStateData>() )
                    {
                        state->extents = Vector3<real_Num>( extents.x, extents.y, state->extents.z );
                    }
                }
            }
        }

        AABB2<real_Num> BoxShape2::getAABB() const
        {
            auto extents = getExtents();
            return AABB2<real_Num>( -extents, extents );
        }

        void BoxShape2::setAABB( const AABB2<real_Num> &box )
        {
            auto extents = box.getHalfSize();
            setExtents( extents );
        }

        Sphere2<real_Num> BoxShape2::getSphere() const
        {
            auto extents = getExtents();
            auto radius = extents.length();
            return Sphere2<real_Num>( Vector2<real_Num>::zero(), radius );
        }

        void BoxShape2::getPoints( Array<Vector2<real_Num>> &points ) const
        {
            auto extents = getExtents();
            points.clear();
            points.reserve( 4 );

            // Four corners of the box
            points.push_back( Vector2<real_Num>( -extents.x, -extents.y ) );
            points.push_back( Vector2<real_Num>( extents.x, -extents.y ) );
            points.push_back( Vector2<real_Num>( extents.x, extents.y ) );
            points.push_back( Vector2<real_Num>( -extents.x, extents.y ) );
        }

        void BoxShape2::computeMass( SmartPtr<IMassData2> massData, real_Num density ) const
        {
            if( massData )
            {
                auto extents = getExtents();
                auto area = extents.x * extents.y * 4.0f;  // Full width and height
                auto mass = density * area;

                massData->setMass( mass );
                massData->setCenter( Vector2<real_Num>::zero() );

                // Moment of inertia for a rectangle: I = (1/12) * mass * (width^2 + height^2)
                auto width = extents.x * 2.0f;
                auto height = extents.y * 2.0f;
                auto inertia = ( mass / 12.0f ) * ( width * width + height * height );
                massData->setInertia( inertia );
            }
        }

    }  // namespace physics
}  // namespace workphone
