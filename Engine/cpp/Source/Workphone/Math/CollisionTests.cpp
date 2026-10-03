#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/CollisionTests.hpp>

namespace workphone
{

    template <class T>
    bool CollisionTests<T>::test( const Cylinder3<T> &a, const AABB3<T> &b )
    {
        return ( a.getAxis().getStart().X() <= b.getMaximum().X() &&
                 a.getAxis().getEnd().X() >= b.getMinimum().X() &&
                 a.getAxis().getStart().Y() <= b.getMaximum().Y() &&
                 a.getAxis().getEnd().Y() >= b.getMinimum().Y() &&
                 a.getAxis().getStart().Z() <= b.getMaximum().Z() &&
                 a.getAxis().getEnd().Z() >= b.getMinimum().Z() );
    }

    template <class T>
    bool CollisionTests<T>::test( const AABB2<T> &a, const AABB2<T> &b )
    {
        return ( a.getMin().X() <= b.getMax().X() && a.getMax().X() >= b.getMin().X() &&
                 a.getMin().Y() <= b.getMax().Y() && a.getMax().Y() >= b.getMin().Y() );
    }

    template <class T>
    bool CollisionTests<T>::test( const AABB3<T> &a, const AABB3<T> &b )
    {
        return ( a.getMinimum().X() <= b.getMaximum().X() && a.getMaximum().X() >= b.getMinimum().X() &&
                 a.getMinimum().Y() <= b.getMaximum().Y() && a.getMaximum().Y() >= b.getMinimum().Y() &&
                 a.getMinimum().Z() <= b.getMaximum().Z() && a.getMaximum().Z() >= b.getMinimum().Z() );
    }

    template <class T>
    bool CollisionTests<T>::test( const Sphere3<T> &a, const Sphere3<T> &b )
    {
        auto distanceSquared = ( a.getCenter() - b.getCenter() ).lengthSquared();
        auto radiusSum = a.getRadius() + b.getRadius();
        return distanceSquared <= ( radiusSum * radiusSum );
    }

    template <class T>
    bool CollisionTests<T>::test( const Line2<T> &line, const AABB2<T> &box )
    {
        auto a_rectangleMinX = box.getMin().X();
        auto a_rectangleMinY = box.getMin().Y();
        auto a_rectangleMaxX = box.getMax().X();
        auto a_rectangleMaxY = box.getMax().Y();
        auto a_p1x = line.getStart().X();
        auto a_p1y = line.getStart().Y();
        auto a_p2x = line.getEnd().X();
        auto a_p2y = line.getEnd().Y();

        // Find min and max X for the segment
        auto minX = a_p1x;
        auto maxX = a_p2x;

        if( a_p1x > a_p2x )
        {
            minX = a_p2x;
            maxX = a_p1x;
        }

        // Find the intersection of the segment's and rectangle's x-projections

        if( maxX > a_rectangleMaxX )
        {
            maxX = a_rectangleMaxX;
        }

        if( minX < a_rectangleMinX )
        {
            minX = a_rectangleMinX;
        }

        if( minX > maxX )  // If their projections do not intersect return false
        {
            return false;
        }

        // Find corresponding min and max Y for min and max X we found before
        auto minY = a_p1y;
        auto maxY = a_p2y;

        auto dx = a_p2x - a_p1x;

        if( Math<T>::Abs( dx ) > T( 0.0000001 ) )
        {
            auto a = ( a_p2y - a_p1y ) / dx;
            auto b = a_p1y - a * a_p1x;
            minY = a * minX + b;
            maxY = a * maxX + b;
        }

        if( minY > maxY )
        {
            T tmp = maxY;
            maxY = minY;
            minY = tmp;
        }

        // Find the intersection of the segment's and rectangle's y-projections
        if( maxY > a_rectangleMaxY )
        {
            maxY = a_rectangleMaxY;
        }

        if( minY < a_rectangleMinY )
        {
            minY = a_rectangleMinY;
        }

        if( minY > maxY )  // If Y-projections do not intersect return false
        {
            return false;
        }

        return true;
    }

    // explicit instantiation
    template class CollisionTests<f32>;
    template class CollisionTests<f64>;

}  // namespace workphone
