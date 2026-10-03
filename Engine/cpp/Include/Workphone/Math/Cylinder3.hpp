#ifndef Cylinder3_h__
#define Cylinder3_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Math/Line3.hpp>

namespace workphone
{

    /**
     * @brief A 3D cylinder defined by an axis, radius, and height.
     *
     * The `Cylinder3` class is a representation of a 3D cylinder, defined by a central axis,
     * radius, and height. It is useful for many 3D graphics and physics applications.
     *
     * @tparam T The numeric type of the cylinder's dimensions (e.g. float, double).
     */
    template <typename T>
    class WPCore_API Cylinder3
    {
    public:
        /**
         * @brief Default constructor for the `Cylinder3` class.
         */
        Cylinder3();

        /**
         * @brief Constructs a new `Cylinder3` object with the given axis, radius, and height.
         *
         * @param axis The central axis of the cylinder.
         * @param radius The radius of the cylinder.
         * @param height The height of the cylinder.
         */
        Cylinder3( const Line3<T> &axis, T radius, T height );

        /**
         * @brief Returns the central axis of the cylinder.
         *
         * @return The central axis of the cylinder.
         */
        Line3<T> getAxis() const;

        /**
         * @brief Sets the central axis of the cylinder to the given axis.
         *
         * @param axis The new central axis of the cylinder.
         */
        void setAxis( const Line3<T> &axis );

        /**
         * @brief Returns the radius of the cylinder.
         *
         * @return The radius of the cylinder.
         */
        T getRadius() const;

        /**
         * @brief Sets the radius of the cylinder to the given value.
         *
         * @param radius The new radius of the cylinder.
         */
        void setRadius( T radius );

        /**
         * @brief Returns the height of the cylinder.
         *
         * @return The height of the cylinder.
         */
        T getHeight() const;

        /**
         * @brief Sets the height of the cylinder to the given value.
         *
         * @param height The new height of the cylinder.
         */
        void setHeight( T height );

    private:
        /** The central axis of the cylinder. */
        Line3<T> m_axis;

        /** The radius of the cylinder. */
        T m_radius = T( 0.0 );

        /** The height of the cylinder. */
        T m_height = T( 0.0 );
    };

    //! Typedef for a f64 line.
    using Cylinder3D = Cylinder3<f64>;

    //! Typedef for a f32 line.
    using Cylinder3F = Cylinder3<f32>;

    //! Typedef for a s32 line.
    using Cylinder3I = Cylinder3<s32>;

}  // namespace workphone

#endif  // Cylinder3_h__
