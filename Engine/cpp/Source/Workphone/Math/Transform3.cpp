#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    template <class T>
    auto Transform3<T>::transformAABB( const AABB3<T> &aabb ) const -> AABB3<T>
    {
        WP_ASSERT( isSane() );

        // Do nothing if current null or infinite
        if( !aabb.isFinite() )
        {
            return AABB3<T>();
        }

        AABB3<T> box;

        Vector3<T> currentCorner = Vector3<T>::zero();

        auto oldMin = aabb.getMinimum();
        auto oldMax = aabb.getMaximum();

        // We sequentially compute the corners in the following order :
        // 0, 6, 5, 1, 2, 4 ,7 , 3
        // This sequence allows us to only change one member at a time to get at all corners.

        // For each one, we transform it using the matrix
        // Which gives the resulting point and merge the resulting point.

        // First corner
        // min min min
        currentCorner = oldMin;
        box.merge( transformPoint( currentCorner ) );

        // min,min,max
        currentCorner.Z() = oldMax.Z();
        box.merge( transformPoint( currentCorner ) );

        // min max max
        currentCorner.Y() = oldMax.Y();
        box.merge( transformPoint( currentCorner ) );

        // min max min
        currentCorner.Z() = oldMin.Z();
        box.merge( transformPoint( currentCorner ) );

        // max max min
        currentCorner.X() = oldMax.X();
        box.merge( transformPoint( currentCorner ) );

        // max max max
        currentCorner.Z() = oldMax.Z();
        box.merge( transformPoint( currentCorner ) );

        // max min max
        currentCorner.Y() = oldMin.Y();
        box.merge( transformPoint( currentCorner ) );

        // max min min
        currentCorner.Z() = oldMin.Z();
        box.merge( transformPoint( currentCorner ) );

        return box;
    }

    template <class T>
    auto Transform3<T>::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = workphone::make_ptr<Properties>();

        properties->setProperty( "Position", m_position );

        // auto rotation = getRotation();
        // properties->setProperty( "Rotation", rotation );

        properties->setProperty( "Orientation", m_orientation );
        properties->setProperty( "Scale", m_scale );

        return properties;
    }

    template <class T>
    void Transform3<T>::setProperties( SmartPtr<Properties> properties )
    {
        if( properties )
        {
            if( auto child = properties->getChild( "position" ) )
            {
                child->getPropertyValue( "x", m_position.x );
                child->getPropertyValue( "y", m_position.y );
                child->getPropertyValue( "z", m_position.z );
            }
            else
            {
                properties->getPropertyValue( "Position", m_position );
            }

            Vector3<T> rotation;

            if( auto child = properties->getChild( "rotation" ) )
            {
                child->getPropertyValue( "x", rotation.x );
                child->getPropertyValue( "y", rotation.y );
                child->getPropertyValue( "z", rotation.z );

                setRotation( rotation );
            }
            else
            {
                if( properties->getPropertyValue( "Rotation", rotation ) )
                {
                    setRotation( rotation );
                }
            }

            if( auto child = properties->getChild( "orientation" ) )
            {
                child->getPropertyValue( "x", m_orientation.x );
                child->getPropertyValue( "y", m_orientation.y );
                child->getPropertyValue( "z", m_orientation.z );
                child->getPropertyValue( "w", m_orientation.w );
            }
            else
            {
                properties->getPropertyValue( "Orientation", m_orientation );
            }

            if( auto child = properties->getChild( "scale" ) )
            {
                child->getPropertyValue( "x", m_scale.x );
                child->getPropertyValue( "y", m_scale.y );
                child->getPropertyValue( "z", m_scale.z );
            }
            else
            {
                properties->getPropertyValue( "Scale", m_scale );
            }
        }
    }

    template <class T>
    void Transform3<T>::fromWorldToLocal( const Transform3<T> &parentTransform,
                                          const Transform3<T> &worldTransform )
    {
        m_position = parentTransform.convertWorldToLocalPosition( worldTransform.getPosition() );
        m_orientation =
            parentTransform.convertWorldToLocalOrientation( worldTransform.getOrientation() );
        m_scale = worldTransform.getScale();
    }

    template <class T>
    Transform3<T>::Transform3() :
        m_position( Vector3<T>::zero() ),
        m_scale( Vector3<T>::unit() ),
        m_orientation( Quaternion<T>::identity() )
    {
    }

    template <class T>
    Transform3<T>::Transform3( const Transform3 &t )
    {
        *this = t;
    }

    template <class T>
    Transform3<T>::Transform3( const Quaternion<T> &orientation ) :
        m_position( Vector3<T>::zero() ),
        m_scale( Vector3<T>::unit() ),
        m_orientation( orientation )
    {
        WP_ASSERT( isSane() );
    }

    template <class T>
    Transform3<T>::Transform3( const Vector3<T> &position, const Quaternion<T> &orientation ) :
        m_position( position ),
        m_scale( Vector3<T>::unit() ),
        m_orientation( orientation )
    {
        WP_ASSERT( isSane() );
    }

    template <class T>
    Transform3<T>::Transform3( const Vector3<T> &position, const Quaternion<T> &orientation,
                               const Vector3<T> &scale ) :
        m_position( position ),
        m_scale( scale ),
        m_orientation( orientation )
    {
        WP_ASSERT( isSane() );
    }

    template <class T>
    Transform3<T>::~Transform3()
    {
    }

    template <class T>
    Vector3<T> &Transform3<T>::getPosition()
    {
        WP_ASSERT( isSane() );
        return m_position;
    }

    template <class T>
    const Vector3<T> &Transform3<T>::getPosition() const
    {
        WP_ASSERT( isSane() );
        return m_position;
    }

    template <class T>
    void Transform3<T>::setPosition( const Vector3<T> &position )
    {
        WP_ASSERT( isSane() );
        WP_ASSERT( position.isFinite() );
        m_position = position;
    }

    template <class T>
    Vector3<T> &Transform3<T>::getScale()
    {
        WP_ASSERT( isSane() );
        return m_scale;
    }

    template <class T>
    const Vector3<T> &Transform3<T>::getScale() const
    {
        WP_ASSERT( isSane() );
        return m_scale;
    }

    template <class T>
    void Transform3<T>::setScale( const Vector3<T> &scale )
    {
        WP_ASSERT( isSane() );
        WP_ASSERT( scale.isFinite() );
        m_scale = scale;
    }

    template <class T>
    Quaternion<T> &Transform3<T>::getOrientation()
    {
        WP_ASSERT( isSane() );
        return m_orientation;
    }

    template <class T>
    const Quaternion<T> &Transform3<T>::getOrientation() const
    {
        WP_ASSERT( isSane() );
        return m_orientation;
    }

    template <class T>
    void Transform3<T>::setOrientation( const Quaternion<T> &orientation )
    {
        WP_ASSERT( isSane() );
        WP_ASSERT( orientation.isSane() );
        m_orientation = orientation;
        WP_ASSERT( isSane() );
    }

    template <class T>
    Vector3<T> Transform3<T>::getRotation() const
    {
        WP_ASSERT( isSane() );

        Euler<T> euler( m_orientation );
        return euler.toDegrees();
    }

    template <class T>
    void Transform3<T>::setRotation( const Vector3<T> &rotation )
    {
        WP_ASSERT( isSane() );

        auto rads = rotation * Math<T>::deg_to_rad();

        Euler<T> euler( rads );
        m_orientation = euler.toQuaternion();
    }

    template <class T>
    Matrix4<T> Transform3<T>::getTransformationMatrix() const
    {
        WP_ASSERT( isSane() );

        Matrix4<T> mat;
        mat.makeTransform( m_position, m_scale, m_orientation );
        return mat;
    }

    template <class T>
    void Transform3<T>::operator=( const Transform3<T> &transform )
    {
        WP_ASSERT( transform.isSane() );
        m_position = transform.m_position;
        m_scale = transform.m_scale;
        m_orientation = transform.m_orientation;
    }

    template <class T>
    bool Transform3<T>::operator==( const Transform3<T> &other ) const
    {
        return m_position == other.m_position && m_orientation == other.m_orientation &&
               m_scale == other.m_scale;
    }

    template <class T>
    bool Transform3<T>::operator!=( const Transform3<T> &other ) const
    {
        return !( m_position == other.m_position && m_orientation == other.m_orientation &&
                  m_scale == other.m_scale );
    }

    template <class T>
    Vector3<T> Transform3<T>::transformPoint( const Vector3<T> &p ) const
    {
        WP_ASSERT( isSane() );
        return ( ( m_orientation * p ) + m_position ) * m_scale;
    }

    template <class T>
    Vector3<T> Transform3<T>::transformVector( const Vector3<T> &dir ) const
    {
        WP_ASSERT( isSane() );
        return m_orientation * dir;
    }

    template <class T>
    Vector3<T> Transform3<T>::inverseTransformPoint( const Vector3<T> &p ) const
    {
        WP_ASSERT( isSane() );
        return m_orientation.inverse() * ( p - m_position ) / m_scale;
    }

    template <class T>
    Vector3<T> Transform3<T>::inverseTransformVector( const Vector3<T> &dir ) const
    {
        WP_ASSERT( isSane() );
        Quaternion<T> inverseOrientation = m_orientation.inverse();
        return inverseOrientation * dir;
    }

    template <class T>
    Vector3<T> Transform3<T>::inverseRotate( const Vector3<T> &p ) const
    {
        WP_ASSERT( isSane() );
        return m_orientation.rotateInv( p );
    }

    template <class T>
    Vector3<T> Transform3<T>::up() const
    {
        WP_ASSERT( isSane() );
        return m_orientation * Vector3<T>::up();
    }

    template <class T>
    Vector3<T> Transform3<T>::forward() const
    {
        WP_ASSERT( isSane() );
        return m_orientation * Vector3<T>::forward();
    }

    template <class T>
    Vector3<T> Transform3<T>::right() const
    {
        WP_ASSERT( isSane() );
        return m_orientation * Vector3<T>::right();
    }

    template <class T>
    void Transform3<T>::setTransform( const Vector3<T> &pos, const Quaternion<T> &rot,
                                      const Vector3<T> &scale )
    {
        WP_ASSERT( isSane() );

        m_position = pos;
        m_orientation = rot;
        m_scale = scale;
    }

    template <class T>
    void Transform3<T>::setTransform( const Transform3 &t )
    {
        WP_ASSERT( isSane() );

        m_position = t.m_position;
        m_scale = t.m_scale;
        m_orientation = t.m_orientation;
    }

    template <class T>
    void Transform3<T>::getTransform( Vector3<T> &pos, Quaternion<T> &rot, Vector3<T> &scale )
    {
        WP_ASSERT( isSane() );

        pos = m_position;
        rot = m_orientation;
        scale = m_scale;
    }

    template <class T>
    void Transform3<T>::transformFromParent( const Transform3 &parent, const Transform3 &local )
    {
        WP_ASSERT( isSane() );

        const auto &scale = parent.m_scale;
        m_scale = scale * local.m_scale;
        m_orientation = parent.m_orientation * local.m_orientation;
        m_position = parent.m_orientation * ( scale * local.m_position );
        m_position += parent.m_position;
    }

    template <class T>
    void Transform3<T>::transformFromParent( const SmartPtr<Transform3<T>> &parent,
                                             const SmartPtr<Transform3<T>> &local )
    {
        WP_ASSERT( local );
        WP_ASSERT( isSane() );

        if( parent )
        {
            const Vector3<T> &scale = parent->m_scale;
            m_scale = scale * local->m_scale;
            m_orientation = parent->m_orientation * local->m_orientation;
            m_position = parent->m_orientation * ( scale * local->m_position );
            m_position += parent->m_position;
        }
        else
        {
            m_scale = local->m_scale;
            m_orientation = local->m_orientation;
            m_position = local->m_position;
        }
    }

    template <class T>
    Vector3<T> Transform3<T>::convertWorldToLocalPosition( const Vector3<T> &worldPos ) const
    {
        auto scale = getScale();
        auto pos = getPosition();
        auto rot = getOrientation();

        auto invScale = Vector3<T>::zero();
        if( Math<T>::Abs( scale.X() ) > std::numeric_limits<T>::epsilon() )
        {
            invScale.X() = T( 1.0 ) / scale.X();
        }

        if( Math<T>::Abs( scale.Y() ) > std::numeric_limits<T>::epsilon() )
        {
            invScale.Y() = T( 1.0 ) / scale.Y();
        }

        if( Math<T>::Abs( scale.Z() ) > std::numeric_limits<T>::epsilon() )
        {
            invScale.Z() = T( 1.0 ) / scale.Z();
        }

        return ( rot.inverse() * ( worldPos - pos ) ) * invScale;
    }

    template <class T>
    Quaternion<T> Transform3<T>::convertWorldToLocalOrientation(
        const Quaternion<T> &worldOrientation ) const
    {
        auto rot = getOrientation();
        return rot.inverse() * worldOrientation;
    }

    template <class T>
    Vector3<T> Transform3<T>::convertLocalToWorldPosition( const Vector3<T> &localPos ) const
    {
        auto scale = getScale();
        auto pos = getPosition();
        auto rot = getOrientation();

        return ( rot * ( scale * localPos ) ) + pos;
    }

    template <class T>
    Quaternion<T> Transform3<T>::convertLocalToWorldOrientation(
        const Quaternion<T> &localOrientation ) const
    {
        auto rot = getOrientation();
        return rot * localOrientation;
    }

    template <class T>
    bool Transform3<T>::isValid() const
    {
        return m_position.isFinite() && m_orientation.isFinite() && m_orientation.isUnit() &&
               m_scale.isFinite();
    }

    template <class T>
    bool Transform3<T>::isSane() const
    {
        return isFinite() && m_orientation.isSane();
    }

    template <class T>
    bool Transform3<T>::isFinite() const
    {
        return m_position.isFinite() && m_orientation.isFinite() && m_scale.isFinite();
    }

    template <class T>
    Transform3<T> Transform3<T>::getInverse() const
    {
        WP_ASSERT( isFinite() );
        return Transform3<T>( m_orientation.rotateInv( -m_position ), m_orientation.getConjugate() );
    }

    template <class T>
    Transform3<T> Transform3<T>::identity()
    {
        return Transform3<T>( Vector3<T>::zero(), Quaternion<T>::identity(), Vector3<T>::unit() );
    }

    template class Transform3<f32>;
    template class Transform3<f64>;

}  // namespace workphone
