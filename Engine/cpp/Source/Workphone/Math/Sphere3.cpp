#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/Sphere3.hpp>

namespace workphone
{
    template <class T>
    bool Sphere3<T>::intersects( const Vector3<T> &point ) const
    {
        Vector3<T> vec = m_center - point;
        if( vec.length() > m_radius )
        {
            return false;
        }
        return true;
    }

    template <class T>
    Sphere3<T>::Sphere3() = default;

    template <class T>
    Sphere3<T>::Sphere3( const Vector3<T> &center, T radius ) : m_center( center ), m_radius( radius )
    {
    }

    template <class T>
    Sphere3<T>::Sphere3( const Sphere3<T> &other ) :
        m_center( other.getCenter() ),
        m_radius( other.getRadius() )
    {
    }

    template <class T>
    Sphere3<T> &Sphere3<T>::operator=( const Sphere3<T> &other )
    {
        m_center = other.getCenter();
        m_radius = other.getRadius();
        return *this;
    }

    template <class T>
    bool Sphere3<T>::operator==( const Sphere3<T> &other ) const
    {
        return ( m_center == other.getCenter() && m_radius == other.getRadius() );
    }

    template <class T>
    bool Sphere3<T>::operator!=( const Sphere3<T> &other ) const
    {
        return !( m_center == other.getCenter() && m_radius == other.getRadius() );
    }

    template <class T>
    Vector3<T> Sphere3<T>::getCenter() const
    {
        return m_center;
    }

    template <class T>
    void Sphere3<T>::setCenter( Vector3<T> center )
    {
        m_center = center;
    }

    template <class T>
    T Sphere3<T>::getRadius() const
    {
        return m_radius;
    }

    template <class T>
    void Sphere3<T>::setRadius( T radius )
    {
        m_radius = radius;
    }

    template <class T>
    bool Sphere3<T>::intersects( const Sphere3<T> &other ) const
    {
        Vector3<T> vec = m_center - other.getCenter();
        T radii = m_radius + other.getRadius();
        if( vec.length() > radii )
        {
            return false;
        }

        return true;
    }

    template <class T>
    bool Sphere3<T>::intersectsSQ( const Sphere3<T> &other ) const
    {
        Vector3<T> vec = m_center - other.getCenter();
        T radii = m_radius + other.getRadius();
        if( vec.lengthSquared() > ( radii * radii ) )
        {
            return false;
        }
        return true;
    }

    /*
    template <class T>
    bool Sphere3<T>::intersects(const SCone& cone) const
    {
        f32 invSin = 1.0f / cone.sinAngle;
        f32 cosSQ = cone.cosAngle * cone.cosAngle;

        vector3<T> centerVertex = center - cone.vertex;
        vector3<T> kd = centerVertex + (cone.axis * (Radius * invSin));
        f64 kdSQ = kd.squaredLength();

        f64 E = kd.dotProduct(cone.axis);
        if(E > 0.0f && E * E >= kdSQ * cosSQ)
        {
            f64 sinSQ = cone.sinAngle * cone.sinAngle;
            kdSQ = centerVertex.squaredLength();
            E = -centerVertex.dotProduct(cone.axis);
            if( E > 0.0f && E * E >= kdSQ * sinSQ)
            {
                f32 rSQ = Radius * Radius;
                return kdSQ <= rSQ;
            }
            return true;
        }
        return false;
    }
    */

    template <class T>
    bool Sphere3<T>::intersects( const AABB3<T> &box ) const
    {
        Vector3<T> center = box.getCenter();
        Vector3<T> halfSize = box.getExtent() * 0.5f;

        Vector3<T> dir = center - m_center;
        T distanceToCenter = dir.normaliseLength();

        T distanceBetween = dir.dotProduct( halfSize ) + m_radius;

        if( fabs( distanceToCenter ) > fabs( distanceBetween ) )
        {
            return false;
        }

        return true;
    }

    // explicit instantiation
    template class Sphere3<s32>;
    template class Sphere3<f32>;
    template class Sphere3<f64>;
}  // namespace workphone
