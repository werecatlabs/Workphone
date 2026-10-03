#ifndef _AABBOX2D_H_
#define _AABBOX2D_H_

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Line2.hpp>

namespace workphone
{
    /** Axis aligned bounding box in 2d space.
     */
    template <class T>
    class WPCore_API AABB2
    {
    public:
        /** Default constructor.
         */
        AABB2();

        /** Constructor.
         * @param x: Position on the x-axis.
         * @param y: Position on the y-axis.
         * @param x2: Position on the x-axis of the other corner.
         * @param y2: Position on the y-axis of the other corner.
         */
        AABB2( T x, T y, T x2, T y2 );

        /** Constructor.
         * @param min: Position of one corner.
         * @param max: Position of the other corner.
         */
        AABB2( const Vector2<T> &min, const Vector2<T> &max );

        /** Constructor.
         * @param other: Another AABB2.
         */
        AABB2( const AABB2<T> &other );

        /** Constructor.
         * @param pos: Position of one corner.
         * @param size: Size of the AABB2.
         * @param useExtents: If true, 'size' represents extents of the box. Otherwise, it's the maximum
         * position.
         */
        AABB2( const Vector2<T> &pos, const Vector2<T> &size, bool useExtents );

        /** Adds a Vector2 to the box and returns a new box that contains both boxes.
         * @param pos: The Vector2 to add to the box.
         * @return Returns a new box that contains both boxes.
         */
        AABB2<T> operator+( const Vector2<T> &pos ) const;

        /** Adds a Vector2 to the box and returns a reference to the box.
         * @param pos: The Vector2 to add to the box.
         * @return Returns a reference to the box.
         */
        const AABB2<T> &operator+=( const Vector2<T> &pos );

        /** Subtracts a Vector2 from the box and returns a new box that contains both boxes.
         * @param pos: The Vector2 to subtract from the box.
         * @return Returns a new box that contains both boxes.
         */
        AABB2<T> operator-( const Vector2<T> &pos ) const;

        /** Subtracts a Vector2 from the box and returns a reference to the box.
         * @param pos: The Vector2 to subtract from the box.
         * @return Returns a reference to the box.
         */
        const AABB2<T> &operator-=( const Vector2<T> &pos );

        /** Checks if two AABB2 are the same.
         * @param other: Another AABB2.
         * @return Returns true if both AABB2 are the same, false if not.
         */
        bool operator==( const AABB2<T> &other ) const;

        /** Checks if two AABB2 are different.
         * @param other: Another AABB2.
         * @return Returns true if both AABB2 are different, false if not.
         */
        bool operator!=( const AABB2<T> &other ) const;

        /** Copies an AABB2 to this one.
         * @param other: Another AABB2.
         * @return Returns a reference to this AABB2.
         */
        const AABB2<T> &operator=( const AABB2<T> &other );

        /** Compares the size of two AABB2.
         * @param other: Another AABB2.
         * @return Returns true if this AABB2 is smaller than 'other', false if not.
         */
        bool operator<( const AABB2<T> &other ) const;

        /**
         * @brief Calculates the area of the AABB2.
         *
         * @return T The area of the AABB2.
         */
        T getArea() const;

        /**
         * @brief Determines if a point is inside the AABB2.
         *
         * @param pos The point to test.
         * @return true If the point is inside the AABB2.
         * @return false If the point is outside the AABB2.
         */
        bool isInside( const Vector2<T> &pos ) const;

        /**
         * @brief Determines if this AABB2 intersects with another AABB2.
         *
         * @param other The AABB2 to test for intersection.
         * @return true If the AABB2s intersect.
         * @return false If the AABB2s do not intersect.
         */
        bool intersects( const AABB2<T> &other ) const;

        /**
         * @brief Determines if this AABB2 intersects with a line segment.
         *
         * @param line The line segment to test for intersection.
         * @return true If the AABB2 intersects with the line segment.
         * @return false If the AABB2 does not intersect with the line segment.
         */
        bool intersects( const Line2<T> &line ) const;

        /**
         * @brief Clips this AABB2 with another AABB2.
         *
         * @param other The AABB2 to clip against.
         */
        void clipAgainst( const AABB2<T> &other );

        /**
         * @brief Constrains this AABB2 to fit inside another AABB2.
         *
         * @param other The AABB2 to fit inside.
         * @return true If this AABB2 can fit inside the other AABB2.
         * @return false If this AABB2 cannot fit inside the other AABB2.
         */
        bool constrainTo( const AABB2<T> &other );

        /**
         * @brief Returns the width of the AABB2.
         *
         * @return T The width of the AABB2.
         */
        T getWidth() const;

        /**
         * @brief Returns the height of the AABB2.
         *
         * @return T The height of the AABB2.
         */
        T getHeight() const;

        /**
         * @brief Swaps the minimum and maximum values if necessary to ensure that the minimum is
         * in the lower left and the maximum is in the upper right.
         */
        void repair();

        /**
         * @brief Determines if the AABB2 is valid to draw. An AABB2 is not valid if the minimum value
         * is to the right or above the maximum value, or if the area of the AABB2 is zero.
         *
         * @return true If the AABB2 is valid to draw.
         * @return false If the AABB2 is not valid to draw.
         */
        bool isValid() const;

        /**
         * @brief Returns the center point of the AABB2.
         *
         * @return Vector2<T> The center point of the AABB2.
         */
        Vector2<T> getCenter() const;

        /**
         * @brief Returns the dimensions of the AABB2.
         *
         * @return Vector2<T> The dimensions of the AABB2.
         */
        Vector2<T> getSize() const;

        /**
         * Gets half the size of the box.
         *
         * @return Half the size of the box.
         */
        Vector2<T> getHalfSize( void ) const;

        /**
         * @brief Adds a point to the AABB2, causing it to grow bigger if the point is outside of the
         * box.
         *
         * @param p The point to add.
         */
        void addInternalPoint( const Vector2<T> &p );

        /**
         * @brief Returns the minimum point of the AABB2.
         *
         * @return Vector2<T> The minimum point of the AABB2.
         */
        Vector2<T> getMin() const;

        /**
         * @brief Sets the minimum point of the AABB2.
         *
         * @param minimum The new minimum point.
         */
        void setMin( const Vector2<T> &minimum );

        /**
         * @brief Returns the maximum point of the AABB2.
         *
         * @return Vector2<T> The maximum point of the AABB2.
         */
        Vector2<T> getMax() const;

        /**
         * @brief Sets the maximum point of the AABB2.
         *
         * @param maximum The new maximum point.
         */
        void setMax( const Vector2<T> &maximum );

    private:
        Vector2<T> m_minimum;
        Vector2<T> m_maximum;
    };

    using AABB2I = AABB2<s32>;
    using AABB2F = AABB2<f32>;
    using AABB2D = AABB2<f64>;

}  // namespace workphone

#endif
