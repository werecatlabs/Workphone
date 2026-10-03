#include "WPPhysics/WPPhysicsPCH.hpp"
#include "WPPhysics/CBoxShape2.hpp"
#include <Workphone/Workphone.hpp>
#include <WorkphonePhysics/workphone_physics_2d.h>
#include <WorkphonePhysics/workphone_physics_collisionshape.h>

namespace workphone::physics
{

    // Helper functions to convert between real_Num and wp_f32
    static inline wp_f32 to_wp_f32( real_Num value )
    {
        return static_cast<wp_f32>( static_cast<double>( value ) );
    }

    static inline real_Num from_wp_f32( wp_f32 value )
    {
        return static_cast<real_Num>( static_cast<double>( value ) );
    }

    static inline wp_vec2f to_wp_vec2f( const Vector2<real_Num> &vec )
    {
        return wp_vec2f_make( to_wp_f32( vec.X() ), to_wp_f32( vec.Y() ) );
    }

    static inline Vector2<real_Num> from_wp_vec2f( const wp_vec2f &vec )
    {
        return Vector2<real_Num>( from_wp_f32( vec.x ), from_wp_f32( vec.y ) );
    }

    CBoxShape2::CBoxShape2() : m_polygonShape( nullptr )
    {
        m_box = AABB2<real_Num>( Vector2<real_Num>( -0.5, -0.5 ), Vector2<real_Num>( 0.5, 0.5 ) );
        
        // Create the collision shape using WorkphonePhysics c89 API
        const auto halfExtents = to_wp_vec2f( m_box.getHalfSize() );
        m_polygonShape = wp_collision_shape2_create_box( halfExtents );
        
        if( m_polygonShape )
        {
            // Set default collision filtering
            wp_filter_data filterData;
            filterData.word0 = m_collisionType;
            filterData.word1 = m_collisionMask;
            filterData.word2 = 0;
            filterData.word3 = 0;
            wp_collision_shape_set_filter_data( m_polygonShape, filterData );
            
            // Set initial enabled and trigger state
            wp_collision_shape_set_enabled( m_polygonShape, m_enabled ? 1 : 0 );
            wp_collision_shape_set_trigger( m_polygonShape, m_trigger ? 1 : 0 );
        }
    }

    CBoxShape2::~CBoxShape2()
    {
        if( m_polygonShape )
        {
            wp_collision_shape_destroy( m_polygonShape );
            m_polygonShape = nullptr;
        }
    }

    bool CBoxShape2::isAttached() const
    {
        if( !m_polygonShape )
        {
            return false;
        }
        return wp_collision_shape_is_attached( m_polygonShape ) != 0;
    }

    void CBoxShape2::setAABB( const AABB2<real_Num> &box )
    {
        m_box = box;

        if( m_polygonShape )
        {
            const auto halfExtents = to_wp_vec2f( m_box.getHalfSize() );
            wp_collision_shape2_set_box_half_extents( m_polygonShape, halfExtents );
        }
    }

    void CBoxShape2::getPoints( Array<Vector2<real_Num>> &points ) const
    {
        points.resize( 4 );
        points[0] = m_box.getMin();
        points[1] = Vector2<real_Num>( m_box.getMax().X(), m_box.getMin().Y() );
        points[2] = m_box.getMax();
        points[3] = Vector2<real_Num>( m_box.getMin().X(), m_box.getMax().Y() );
    }

    void CBoxShape2::getPoints( Array<Vector2<real_Num>>   &points,
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

    u8 CBoxShape2::getType() const
    {
        if( !m_polygonShape )
        {
            return static_cast<u8>( WORKPHONE_COLLISION_SHAPE_BOX );
        }
        return static_cast<u8>( wp_collision_shape_get_type( m_polygonShape ) );
    }

    SmartPtr<Properties> CBoxShape2::getProperties() const
    {
        auto properties = workphone::make_ptr<Properties>();
        properties->setProperty( "center", m_box.getCenter() );
        properties->setProperty( "size", m_box.getSize() );
        properties->setProperty( "enabled", m_enabled );
        properties->setProperty( "trigger", m_trigger );
        properties->setProperty( "collisionType", m_collisionType );
        properties->setProperty( "collisionMask", m_collisionMask );
        return properties;
    }

    void CBoxShape2::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
        {
            return;
        }

        auto center = m_box.getCenter();
        auto size = m_box.getSize();
        properties->getPropertyValue( "center", center );
        properties->getPropertyValue( "size", size );
        properties->getPropertyValue( "enabled", m_enabled );
        properties->getPropertyValue( "trigger", m_trigger );
        properties->getPropertyValue( "collisionType", m_collisionType );
        properties->getPropertyValue( "collisionMask", m_collisionMask );
        size.X() = std::max( Math<real_Num>::Abs( size.X() ), static_cast<real_Num>( 0.002 ) );
        size.Y() = std::max( Math<real_Num>::Abs( size.Y() ), static_cast<real_Num>( 0.002 ) );
        const auto halfSize = size * static_cast<real_Num>( 0.5 );
        m_box = AABB2<real_Num>( center - halfSize, center + halfSize );
        
        if( m_polygonShape )
        {
            const auto halfExtents = to_wp_vec2f( m_box.getHalfSize() );
            wp_collision_shape2_set_box_half_extents( m_polygonShape, halfExtents );
        }
    }

    bool CBoxShape2::intersects( const SmartPtr<IBoxShape2> boxA, const Transform2<real_Num> &tranformA,
                                 const SmartPtr<IBoxShape2> boxB, const Transform2<real_Num> &tranformB )
    {
        if( !boxA || !boxB )
        {
            return false;
        }

        const auto transformedBounds =
            []( const SmartPtr<IBoxShape2> &box, const Transform2<real_Num> &transform )
        {
            Array<Vector2<real_Num>> points;
            const auto               bounds = box->getAABB();
            points.push_back( bounds.getMin() );
            points.push_back( Vector2<real_Num>( bounds.getMax().X(), bounds.getMin().Y() ) );
            points.push_back( bounds.getMax() );
            points.push_back( Vector2<real_Num>( bounds.getMin().X(), bounds.getMax().Y() ) );
            const auto cosine = Math<real_Num>::Cos( transform.getRotation() );
            const auto sine = Math<real_Num>::Sin( transform.getRotation() );
            auto       result = AABB2<real_Num>();
            for( size_t i = 0; i < points.size(); ++i )
            {
                const auto point = Vector2<real_Num>( points[i].X() * cosine - points[i].Y() * sine,
                                                      points[i].X() * sine + points[i].Y() * cosine ) +
                                   transform.getPosition();
                if( i == 0 )
                {
                    result = AABB2<real_Num>( point, point );
                }
                else
                {
                    result.addInternalPoint( point );
                }
            }
            return result;
        };

        return transformedBounds( boxA, tranformA ).intersects( transformedBounds( boxB, tranformB ) );
    }

    void CBoxShape2::computeMass( SmartPtr<IMassData2> massData, real_Num density ) const
    {
        if( !massData )
        {
            return;
        }
        const auto size = m_box.getSize();
        const auto safeDensity = std::max( density, static_cast<real_Num>( 0.0 ) );
        const auto mass = safeDensity * size.X() * size.Y();
        massData->setMass( static_cast<f32>( mass ) );
        massData->setCenter( m_box.getCenter() );
        massData->setInertia( static_cast<f32>( mass * ( size.X() * size.X() + size.Y() * size.Y() ) /
                                                static_cast<real_Num>( 12.0 ) ) );
    }

    void CBoxShape2::_getObject( void **ppObject ) const
    {
        if( ppObject )
        {
            *ppObject = m_polygonShape;
        }
    }

    void CBoxShape2::setPolyPoints()
    {
        if( !m_polygonShape )
        {
            return;
        }
        
        const auto halfExtents = to_wp_vec2f( m_box.getHalfSize() );
        wp_collision_shape2_set_box_half_extents( m_polygonShape, halfExtents );
        
        // Update trigger state
        wp_collision_shape_set_trigger( m_polygonShape, m_trigger ? 1 : 0 );
        
        // Update collision filtering
        wp_filter_data filterData;
        filterData.word0 = m_collisionType;
        filterData.word1 = m_collisionMask;
        filterData.word2 = 0;
        filterData.word3 = 0;
        wp_collision_shape_set_filter_data( m_polygonShape, filterData );
    }

    workphone::AABB2<workphone::real_Num> CBoxShape2::getAABB() const
    {
        return m_box;
    }

    Sphere2<real_Num> CBoxShape2::getSphere() const
    {
        return Sphere2<real_Num>( m_box.getCenter(), static_cast<f32>( m_box.getHalfSize().length() ) );
    }

    void CBoxShape2::setStateListener( SmartPtr<IStateListener> stateListener )
    {
        m_stateListener = stateListener;
    }

    workphone::SmartPtr<workphone::IStateListener> CBoxShape2::getStateListener() const
    {
        return m_stateListener;
    }

    void CBoxShape2::setStateContext( SmartPtr<IStateContext> stateContext )
    {
        m_stateContext = stateContext;
    }

    workphone::SmartPtr<workphone::IStateContext> CBoxShape2::getStateContext() const
    {
        return m_stateContext;
    }

    void CBoxShape2::setTrigger( bool trigger )
    {
        if( m_trigger != trigger )
        {
            m_trigger = trigger;
            setPolyPoints();
        }
    }

    bool CBoxShape2::isTrigger() const
    {
        if( !m_polygonShape )
        {
            return m_trigger;
        }
        return wp_collision_shape_is_trigger( m_polygonShape ) != 0;
    }

    bool CBoxShape2::isEnabled() const
    {
        if( !m_polygonShape )
        {
            return m_enabled;
        }
        return wp_collision_shape_is_enabled( m_polygonShape ) != 0;
    }

    void CBoxShape2::setEnabled( bool enabled )
    {
        m_enabled = enabled;
        if( m_polygonShape )
        {
            wp_collision_shape_set_enabled( m_polygonShape, enabled ? 1 : 0 );
        }
    }

    u32 CBoxShape2::getCollisionMask() const
    {
        if( !m_polygonShape )
        {
            return m_collisionMask;
        }
        const auto filterData = wp_collision_shape_get_filter_data( m_polygonShape );
        return filterData.word1;
    }

    void CBoxShape2::setCollisionMask( u32 mask )
    {
        m_collisionMask = mask;
        if( m_polygonShape )
        {
            auto filterData = wp_collision_shape_get_filter_data( m_polygonShape );
            filterData.word1 = mask;
            wp_collision_shape_set_filter_data( m_polygonShape, filterData );
        }
    }

    u32 CBoxShape2::getCollisionType() const
    {
        if( !m_polygonShape )
        {
            return m_collisionType;
        }
        const auto filterData = wp_collision_shape_get_filter_data( m_polygonShape );
        return filterData.word0;
    }

    void CBoxShape2::setCollisionType( u32 type )
    {
        m_collisionType = type;
        if( m_polygonShape )
        {
            auto filterData = wp_collision_shape_get_filter_data( m_polygonShape );
            filterData.word0 = type;
            wp_collision_shape_set_filter_data( m_polygonShape, filterData );
        }
    }

} // namespace workphone::physics

// end namespace
