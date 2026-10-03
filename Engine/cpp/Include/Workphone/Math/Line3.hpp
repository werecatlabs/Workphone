// Copyright (C) 2002-2007 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __WP_LINE_3D_H_INCLUDED__
#define __WP_LINE_3D_H_INCLUDED__

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{
    /** A 3D line between two points with intersection methods.
     */
    template <class T>
    class WPCore_API Line3
    {
    public:
        /** Default constructor. */
        Line3();

        /** Constructor. */
        Line3( T xa, T ya, T za, T xb, T yb, T zb );

        /** Constructor. */
        Line3( const Vector3<T> &start, const Vector3<T> &end );

        /** Copy constructor. */
        Line3( const Line3<T> &other );

        // Operators

        /** Addition operator. */
        Line3<T> operator+( const Vector3<T> &point ) const;

        /** Addition assignment operator. */
        Line3<T> &operator+=( const Vector3<T> &point );

        /** Subtraction operator. */
        Line3<T> operator-( const Vector3<T> &point ) const;

        /** Subtraction assignment operator. */
        Line3<T> &operator-=( const Vector3<T> &point );

        /** Equality operator. */
        bool operator==( const Line3<T> &other ) const;

        /** Inequality operator. */
        bool operator!=( const Line3<T> &other ) const;

        // Functions

        /** Set the line. */
        void setLine( const T &xa, const T &ya, const T &za, const T &xb, const T &yb, const T &zb );

        /** Set the line. */
        void setLine( const Vector3<T> &nstart, const Vector3<T> &nend );

        /** Set the line. */
        void setLine( const Line3<T> &line );

        /** Get the length of the line. */
        f64 getLength() const;

        /** Get the squared length of the line. */
        T getLengthSQ() const;

        /** Get the middle of the line. */
        Vector3<T> getMiddle() const;

        /** Get the vector of the line. */
        Vector3<T> getVector() const;

        /** Check if a point is between the start and end points of the line. */
        bool isPointBetweenStartAndEnd( const Vector3<T> &point ) const;

        /** Get the closest point on the line to a point. */
        Vector3<T> getClosestPoint( const Vector3<T> &point ) const;

        /** Check if the line intersects with a sphere. */
        bool getIntersectionWithSphere( Vector3<T> sorigin, T sradius, f64 &outdistance ) const;

        /** Get the start point of the line. */
        Vector3<T> getStart() const;

        /** Set the start point of the line. */
        void setStart( Vector3<T> value );

        /** Get the end point of the line. */
        Vector3<T> getEnd() const;

        /** Set the end point of the line. */
        void setEnd( Vector3<T> end );

        /** Get the direction of the line. */
        Vector3<T> getDirection() const;

    private:
        /// < Start point of the line.
        Vector3<T> m_start;

        /// < End point of the line.
        Vector3<T> m_end;
    };

    //! Typedef for a f64 line.
    using Line3D = Line3<f64>;

    //! Typedef for a f32 line.
    using Line3F = Line3<f32>;

    //! Typedef for a s32 line.
    using Line3I = Line3<s32>;

}  // namespace workphone

#endif
