#ifndef Sphere3_h__
#define Sphere3_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/AABB3.hpp>

namespace workphone
{
    /**
     * A 3D sphere class.
     */
    template <class T>
    class WPCore_API Sphere3
    {
    public:
        /**
         * Default constructor.
         */
        Sphere3();

        /**
         * Constructs a new sphere with the given center and radius.
         *
         * @param center The center of the sphere.
         * @param radius The radius of the sphere.
         */
        Sphere3( const Vector3<T> &center, T radius );

        /**
         * Copy constructor.
         *
         * @param other The sphere to copy.
         */
        Sphere3( const Sphere3<T> &other );

        /**
         * Assignment operator.
         *
         * @param other The sphere to copy.
         * @return A reference to this sphere.
         */
        Sphere3<T> &operator=( const Sphere3<T> &other );

        /**
         * Equality operator.
         *
         * @param other The sphere to compare to.
         * @return `true` if the spheres are equal, `false` otherwise.
         */
        bool operator==( const Sphere3<T> &other ) const;

        /**
         * Inequality operator.
         *
         * @param other The sphere to compare to.
         * @return `true` if the spheres are not equal, `false` otherwise.
         */
        bool operator!=( const Sphere3<T> &other ) const;

        /**
         * Returns the center of the sphere.
         *
         * @return The center of the sphere.
         */
        Vector3<T> getCenter() const;

        /**
         * Sets the center of the sphere.
         *
         * @param center The new center of the sphere.
         */
        void setCenter( Vector3<T> center );

        /**
         * Returns the radius of the sphere.
         *
         * @return The radius of the sphere.
         */
        T getRadius() const;

        /**
         * Sets the radius of the sphere.
         *
         * @param radius The new radius of the sphere.
         */
        void setRadius( T radius );

        bool intersects( const Vector3<T> &point ) const;

        /**
         * Tests if this sphere intersects with another sphere.
         *
         * @param other The sphere to test intersection with.
         * @return `true` if the spheres intersect, `false` otherwise.
         */
        bool intersects( const Sphere3<T> &other ) const;

        /**
         * Tests if this sphere intersects with another sphere.
         *
         * This method is faster than `intersects` since it avoids computing square roots.
         *
         * @param other The sphere to test intersection with.
         * @return `true` if the spheres intersect, `false` otherwise.
         */
        bool intersectsSQ( const Sphere3<T> &other ) const;

        /**
         * Tests if this sphere intersects with an axis-aligned bounding box.
         *
         * @param box The box to test intersection with.
         * @return `true` if the sphere and box intersect, `false` otherwise.
         */
        bool intersects( const AABB3<T> &box ) const;

    private:
        Vector3<T> m_center = Vector3<T>::zero();  //!< The center of the sphere.
        T m_radius = T( 0.0 );                     //!< The radius of the sphere.
    };

    using Sphere3F = Sphere3<f32>;
    using Sphere3D = Sphere3<f64>;

}  // namespace workphone

#endif  // Sphere3_h__
