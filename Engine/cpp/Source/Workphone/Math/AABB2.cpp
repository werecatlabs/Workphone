#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/AABB2.hpp>

namespace workphone
{

    template <class T>
    AABB2<T>::AABB2() = default;

    template <class T>
    void AABB2<T>::setMax( const Vector2<T> &maximum )
    {
        m_maximum = maximum;
    }

    template <class T>
    workphone::Vector2<T> AABB2<T>::getMax() const
    {
        return m_maximum;
    }

    template <class T>
    void AABB2<T>::setMin( const Vector2<T> &minimum )
    {
        m_minimum = minimum;
    }

    template <class T>
    workphone::Vector2<T> AABB2<T>::getMin() const
    {
        return m_minimum;
    }

    template <class T>
    void AABB2<T>::addInternalPoint( const Vector2<T> &p )
    {
        if( p.X() > m_maximum.X() )
        {
            m_maximum.X() = p.X();
        }

        if( p.Y() > m_maximum.Y() )
        {
            m_maximum.Y() = p.Y();
        }

        if( p.X() < m_minimum.X() )
        {
            m_minimum.X() = p.X();
        }

        if( p.Y() < m_minimum.Y() )
        {
            m_minimum.Y() = p.Y();
        }
    }

    template <class T>
    workphone::Vector2<T> AABB2<T>::getHalfSize() const
    {
        return getSize() / T( 2.0 );
    }

    template <class T>
    bool AABB2<T>::isValid() const
    {
        // thx to jox for a coraabbox2dion to this method
        auto xd = m_maximum.X() - m_minimum.X();
        auto yd = m_maximum.Y() - m_minimum.Y();

        return !( xd <= 0 || yd <= T( 0.0 ) || ( xd == 0 && yd == T( 0.0 ) ) );
    }

    template <class T>
    T AABB2<T>::getWidth() const
    {
        return m_maximum.X() - m_minimum.X();
    }

    template <class T>
    bool AABB2<T>::constrainTo( const AABB2<T> &other )
    {
        if( other.getWidth() < getWidth() || other.getHeight() < getHeight() )
        {
            return false;
        }

        auto diff = other.m_maximum.X() - m_maximum.X();
        if( diff < 0 )
        {
            m_maximum.X() += diff;
            m_minimum.X() += diff;
        }

        diff = other.m_maximum.Y() - m_maximum.Y();
        if( diff < 0 )
        {
            m_maximum.Y() += diff;
            m_minimum.Y() += diff;
        }

        diff = m_minimum.X() - other.m_minimum.X();
        if( diff < 0 )
        {
            m_minimum.X() -= diff;
            m_maximum.X() -= diff;
        }

        diff = m_minimum.Y() - other.m_minimum.Y();
        if( diff < 0 )
        {
            m_minimum.Y() -= diff;
            m_maximum.Y() -= diff;
        }

        return true;
    }

    template <class T>
    void AABB2<T>::clipAgainst( const AABB2<T> &other )
    {
        if( other.getMax().X() < m_maximum.X() )
        {
            m_maximum.X() = other.getMax().X();
        }

        if( other.getMax().Y() < m_maximum.Y() )
        {
            m_maximum.Y() = other.getMax().Y();
        }

        if( other.getMin().X() > m_minimum.X() )
        {
            m_minimum.X() = other.getMin().X();
        }

        if( other.getMin().Y() > m_minimum.Y() )
        {
            m_minimum.Y() = other.getMin().Y();
        }

        // correct possible invalid rect resulting from clipping
        if( m_minimum.Y() > m_maximum.Y() )
        {
            m_minimum.Y() = m_maximum.Y();
        }

        if( m_minimum.X() > m_maximum.X() )
        {
            m_minimum.X() = m_maximum.X();
        }
    }

    template <class T>
    bool AABB2<T>::intersects( const Line2<T> &line ) const
    {
        auto diff = line.getStart() - getCenter();
        auto perp = line.getDirection().perp();

        auto LHS = Math<T>::Abs( perp.dotProduct( diff ) );

        auto extent = getSize();
        auto RHS = extent[0] + extent[1];

        return LHS <= RHS;
    }

    template <class T>
    bool AABB2<T>::operator<( const AABB2<T> &other ) const
    {
        return getArea() < other.getArea();
    }

    template <class T>
    const workphone::AABB2<T> &AABB2<T>::operator=( const AABB2<T> &other )
    {
        m_minimum = other.m_minimum;
        m_maximum = other.m_maximum;
        return *this;
    }

    template <class T>
    bool AABB2<T>::operator!=( const AABB2<T> &other ) const
    {
        return ( m_minimum != other.m_minimum || m_maximum != other.m_maximum );
    }

    template <class T>
    bool AABB2<T>::operator==( const AABB2<T> &other ) const
    {
        return ( m_minimum == other.m_minimum && m_maximum == other.m_maximum );
    }

    template <class T>
    const workphone::AABB2<T> &AABB2<T>::operator-=( const Vector2<T> &pos )
    {
        m_minimum -= pos;
        m_maximum -= pos;
        return *this;
    }

    template <class T>
    workphone::AABB2<T> AABB2<T>::operator-( const Vector2<T> &pos ) const
    {
        AABB2<T> ret( *this );
        ret.m_minimum -= pos;
        ret.m_maximum -= pos;
        return ret;
    }

    template <class T>
    const workphone::AABB2<T> &AABB2<T>::operator+=( const Vector2<T> &pos )
    {
        m_minimum += pos;
        m_maximum += pos;
        return *this;
    }

    template <class T>
    workphone::AABB2<T> AABB2<T>::operator+( const Vector2<T> &pos ) const
    {
        AABB2<T> ret( *this );
        ret.m_minimum += pos;
        ret.m_maximum += pos;
        return ret;
    }

    template <class T>
    AABB2<T>::AABB2( const Vector2<T> &pos, const Vector2<T> &size, bool useExtents )
    {
        if( !useExtents )
        {
            m_minimum = pos;
            m_maximum = Vector2<T>( pos.X() + size.X(), pos.Y() + size.Y() );
        }
        else
        {
            m_minimum = pos - size;
            m_maximum = pos + size;
        }
    }

    template <class T>
    AABB2<T>::AABB2( T x, T y, T x2, T y2 ) : m_minimum( x, y ), m_maximum( x2, y2 )
    {
    }

    template <class T>
    AABB2<T>::AABB2( const Vector2<T> &min, const Vector2<T> &max ) : m_minimum( min ), m_maximum( max )
    {
    }

    template <class T>
    AABB2<T>::AABB2( const AABB2<T> &other ) : m_minimum( other.m_minimum ), m_maximum( other.m_maximum )
    {
    }

    template <class T>
    T AABB2<T>::getArea() const
    {
        return getWidth() * getHeight();
    }

    template <class T>
    bool AABB2<T>::isInside( const Vector2<T> &pos ) const
    {
        return ( m_minimum.X() <= pos.X() && m_minimum.Y() <= pos.Y() && m_maximum.X() >= pos.X() &&
                 m_maximum.Y() >= pos.Y() );
    }

    template <class T>
    bool AABB2<T>::intersects( const AABB2<T> &other ) const
    {
        return ( m_maximum.Y() > other.m_minimum.Y() && m_minimum.Y() < other.m_maximum.Y() &&
                 m_maximum.X() > other.m_minimum.X() && m_minimum.X() < other.m_maximum.X() );
    }

    template <class T>
    T AABB2<T>::getHeight() const
    {
        return m_maximum.Y() - m_minimum.Y();
    }

    template <class T>
    void AABB2<T>::repair()
    {
        if( m_maximum.X() < m_minimum.X() )
        {
            auto t = m_maximum.X();
            m_maximum.X() = m_minimum.X();
            m_minimum.X() = t;
        }

        if( m_maximum.Y() < m_minimum.Y() )
        {
            auto t = m_maximum.Y();
            m_maximum.Y() = m_minimum.Y();
            m_minimum.Y() = t;
        }
    }

    template <class T>
    workphone::Vector2<T> AABB2<T>::getCenter() const
    {
        return Vector2<T>( ( m_minimum.X() + m_maximum.X() ) / T( 2.0 ),
                           ( m_minimum.Y() + m_maximum.Y() ) / T( 2.0 ) );
    }

    template <class T>
    workphone::Vector2<T> AABB2<T>::getSize() const
    {
        return Vector2<T>( getWidth(), getHeight() );
    }

    template class AABB2<s32>;
    template class AABB2<f32>;
    template class AABB2<f64>;

}  // namespace workphone
