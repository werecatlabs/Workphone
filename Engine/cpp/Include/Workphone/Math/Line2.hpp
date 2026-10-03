#ifndef __WP_LINE2_H_
#define __WP_LINE2_H_

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    //! A 2D line class between two points with intersection methods.
    template <class T>
    class WPCore_API Line2
    {
    public:
        //! Default constructor.
        Line2();

        //! Constructor with coordinates of start and end points.
        Line2( T xa, T ya, T xb, T yb );

        //! Constructor with start and end vectors.
        Line2( const Vector2<T> &start, const Vector2<T> &end );

        //! Copy constructor.
        Line2( const Line2<T> &other );

        // Operators

        //! Add a point to this line.
        Line2<T> operator+( const Vector2<T> &point ) const;

        //! Add a point to this line.
        Line2<T> &operator+=( const Vector2<T> &point );

        //! Subtract a point from this line.
        Line2<T> operator-( const Vector2<T> &point ) const;

        //! Subtract a point from this line.
        Line2<T> &operator-=( const Vector2<T> &point );

        //! Check if two lines are equal.
        bool operator==( const Line2<T> &other ) const;

        //! Check if two lines are not equal.
        bool operator!=( const Line2<T> &other ) const;

        // Functions

        //! Set the start and end points of this line.
        void setLine( const T &xa, const T &ya, const T &xb, const T &yb );

        //! Set the start and end points of this line using vectors.
        void setLine( const Vector2<T> &start, const Vector2<T> &end );

        //! Set the start and end points of this line using another line.
        void setLine( const Line2<T> &line );

        //! Get the length of this line.
        T getLength() const;

        //! Get the squared length of this line.
        T getLengthSQ() const;

        //! Get the middle point of this line.
        Vector2<T> getMiddle() const;

        //! Get the vector of this line.
        Vector2<T> getVector() const;

        //! Check if this line intersects with another line.
        /*!
            \param l: Other line to test intersection with.
            \param out: If there is an intersection, the location of the intersection will
                be stored in this vector.
            \return Returns true if there is an intersection, false if not.
        */
        bool intersectWith( const Line2<T> &l, Vector2<T> &out ) const;

        //! Check if this line intersects with a sphere.
        bool intersectWith( const Vector2<T> &center, T radius ) const;

        //! Get the unit vector of this line.
        Vector2<T> getUnitVector() const;

        //! Get the direction of this line.
        Vector2<T> getDirection() const;

        //! Get the angle between this line and another line.
        T getAngleWith( const Line2<T> &l );

        //! Get the orientation of a point relative to this line.
        /*!
            \return Returns 0 if the point is on the line, <0 if to the left, or >0 if to the right.
        */
        T getPointOrientation( const Vector2<T> &point );

        //! Check if a point is on this line.
        bool isPointOnLine( const Vector2<T> &point );

        //! Check if a point is between the start and end points of this line.
        bool isPointBetweenStartAndEnd( const Vector2<T> &point ) const;

        //! Get the closest point on this line to a point.
        Vector2<T> getClosestPoint( const Vector2<T> &point ) const;

        //! Returns the starting point of the line.
        Vector2<T> getStart() const;

        //! Sets the starting point of the line to the given vector.
        void setStart( Vector2<T> &start );

        //! Returns the end point of the line.
        Vector2<T> getEnd() const;

        //! Sets the end point of the line to the given vector.
        void setEnd( const Vector2<T> &end );

    private:
        Vector2<T> m_start;  // Starting point of the line
        Vector2<T> m_end;    // End point of the line
    };

    /// A typedef for an integer line.
    using Line2I = Line2<s32>;

    /// A typedef for a float line.
    using Line2F = Line2<f32>;

    /// A typedef for a double line.
    using Line2D = Line2<f64>;

}  // namespace workphone

#endif
