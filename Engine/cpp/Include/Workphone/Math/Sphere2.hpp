#ifndef Sphere2_h__
#define Sphere2_h__

#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    /**
     * @brief A 2D sphere.
     *
     * @tparam T The numeric type used for the sphere's position and radius.
     */
    template <class T>
    class WPCore_API Sphere2
    {
    public:
        /**
         * @brief Constructs a sphere with a center at the origin and a radius of 0.
         */
        Sphere2();

        /**
         * @brief Constructs a sphere with the given center and radius.
         *
         * @param center The center of the sphere.
         * @param radius The radius of the sphere.
         */
        Sphere2( const Vector2<T> &center, f32 radius );

        /**
         * @brief Copy constructor.
         *
         * @param other The sphere to copy from.
         */
        Sphere2( const Sphere2<T> &other );

        // Operators

        /**
         * @brief Copy assignment operator.
         *
         * @param other The sphere to copy from.
         *
         * @return A reference to the assigned sphere.
         */
        Sphere2<T> &operator=( const Sphere2<T> &other );

        /**
         * @brief Checks if two spheres are equal.
         *
         * @param other The sphere to compare with.
         *
         * @return `true` if the two spheres are equal, `false` otherwise.
         */
        bool operator==( const Sphere2<T> &other ) const;

        /**
         * @brief Checks if two spheres are not equal.
         *
         * @param other The sphere to compare with.
         *
         * @return `true` if the two spheres are not equal, `false` otherwise.
         */
        bool operator!=( const Sphere2<T> &other ) const;

        // Functions

        /**
         * @brief Returns the center of the sphere.
         *
         * @return The center of the sphere.
         */
        Vector2<T> getCenter() const;

        /**
         * @brief Sets the center of the sphere.
         *
         * @param center The new center of the sphere.
         */
        void setCenter( Vector2<T> center );

        /**
         * @brief Returns the radius of the sphere.
         *
         * @return The radius of the sphere.
         */
        T getRadius() const;

        /**
         * @brief Sets the radius of the sphere.
         *
         * @param radius The new radius of the sphere.
         */
        void setRadius( T radius );

        /**
         * @brief Tests if this sphere intersects with another sphere.
         *
         * @param other The sphere to test intersection with.
         *
         * @return `true` if the spheres intersect, `false` otherwise.
         */
        bool intersects( const Sphere2<T> &other ) const;

        /**
         * @brief Tests if this sphere intersects with another sphere.
         *
         * @param other The sphere to test intersection with.
         *
         * @return `true` if the spheres intersect, `false` otherwise.
         *
         * This uses the squared distance of centers and squared sum of radii for speed.
         */
        bool intersectsSQ( const Sphere2<T> &other ) const;

    private:
        Vector2<T> m_center = Vector2<T>::zero(); /**< The center of the sphere. */
        T m_radius = T( 0.0 );                    /**< The radius of the sphere. */
    };

    using Sphere2F = Sphere2<f32>;
    using Sphere2D = Sphere2<f64>;

}  // namespace workphone

#endif  // Sphere2_h__
