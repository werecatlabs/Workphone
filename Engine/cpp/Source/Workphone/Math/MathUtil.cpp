#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/MathUtil.hpp>

namespace workphone
{
    template <class T>
    auto MathUtil<T>::equals( T f1, T f2 ) -> bool
    {
        return std::fabs( f1 - f2 ) <= std::numeric_limits<T>::epsilon();
    }

    template <class T>
    auto MathUtil<T>::equals( T f1, T f2, T tolerance ) -> bool
    {
        return std::fabs( f1 - f2 ) <= tolerance;
    }

    template <class T>
    auto MathUtil<T>::equals( const Vector2<T> &a, const Vector2<T> &b ) -> bool
    {
        return MathUtil<T>::equals( a.X(), b.X() ) && MathUtil<T>::equals( a.Y(), b.Y() );
    }

    template <class T>
    auto MathUtil<T>::equals( const Vector3<T> &a, const Vector3<T> &b ) -> bool
    {
        return MathUtil<T>::equals( a.X(), b.X() ) && MathUtil<T>::equals( a.Y(), b.Y() ) &&
               MathUtil<T>::equals( a.Z(), b.Z() );
    }

    template <class T>
    auto MathUtil<T>::equals( const Quaternion<T> &a, const Quaternion<T> &b ) -> bool
    {
        return MathUtil<T>::equals( a.W(), b.W() ) && MathUtil<T>::equals( a.X(), b.X() ) &&
               MathUtil<T>::equals( a.Y(), b.Y() ) && MathUtil<T>::equals( a.Z(), b.Z() );
    }

    template <class T>
    auto MathUtil<T>::equals( const Vector3<T> &a, const Vector3<T> &b, T tolerance ) -> bool
    {
        return MathUtil<T>::equals( a.X(), b.X(), tolerance ) &&
               MathUtil<T>::equals( a.Y(), b.Y(), tolerance ) &&
               MathUtil<T>::equals( a.Z(), b.Z(), tolerance );
    }

    template <class T>
    auto MathUtil<T>::equals( const Quaternion<T> &a, const Quaternion<T> &b, T tolerance ) -> bool
    {
        return MathUtil<T>::equals( a.W(), b.W(), tolerance ) &&
               MathUtil<T>::equals( a.X(), b.X(), tolerance ) &&
               MathUtil<T>::equals( a.Y(), b.Y(), tolerance ) &&
               MathUtil<T>::equals( a.Z(), b.Z(), tolerance );
    }

    template <>
    auto MathUtil<s32>::isFinite( const Vector3<s32> &value ) -> bool
    {
        return std::isfinite( static_cast<f32>( value.X() ) ) &&
               std::isfinite( static_cast<f32>( value.Y() ) ) &&
               std::isfinite( static_cast<f32>( value.Z() ) );
    }

    template <>
    auto MathUtil<s32>::isFinite( const Quaternion<s32> &value ) -> bool
    {
        return std::isfinite( static_cast<f32>( value.W() ) ) &&
               std::isfinite( static_cast<f32>( value.X() ) ) &&
               std::isfinite( static_cast<f32>( value.Y() ) ) &&
               std::isfinite( static_cast<f32>( value.Z() ) );
    }

    template <class T>
    auto MathUtil<T>::isFinite( const Vector3<T> &value ) -> bool
    {
        return std::isfinite( value.X() ) && std::isfinite( value.Y() ) && std::isfinite( value.Z() );
    }

    template <class T>
    auto MathUtil<T>::isFinite( const Quaternion<T> &value ) -> bool
    {
        return std::isfinite( value.W() ) && std::isfinite( value.X() ) && std::isfinite( value.Y() ) &&
               std::isfinite( value.Z() );
    }

    template <class T>
    auto MathUtil<T>::getRotationTo( const Vector3<T> &src, const Vector3<T> &dest,
                                     const Vector3<T> &fallbackAxis /*= Vector3<T>::ZERO*/ )
        -> Quaternion<T>
    {
        // Based on Stan Melax's article in Game Programming Gems
        Quaternion<T> q;
        // Copy, since cannot modify local
        Vector3<T> v0 = src;
        Vector3<T> v1 = dest;
        v0.normalise();
        v1.normalise();

        T d = v0.dotProduct( v1 );
        // If dot == 1, vectors are the same
        if( d >= 1.0f )
        {
            return Quaternion<T>::identity();
        }
        if( d < T( 1e-6 - 1.0 ) )
        {
            if( fallbackAxis != Vector3<T>::ZERO )
            {
                // rotate 180 degrees about the fallback axis
                q.fromAngleAxis( Math<T>::pi(), fallbackAxis );
            }
            else
            {
                // Generate an axis
                Vector3<T> axis = Vector3<T>::UNIT_X.crossProduct( src );
                if( axis.isZeroLength() )
                {  // pick another if collinear
                    axis = Vector3<T>::UNIT_Y.crossProduct( src );
                }

                axis.normalise();
                q.fromAngleAxis( Math<T>::pi(), axis );
            }
        }
        else
        {
            T s = Math<T>::Sqrt( ( 1 + d ) * 2 );
            T invs = 1 / s;

            Vector3<T> c = v0.crossProduct( v1 );

            q.X() = c.X() * invs;
            q.Y() = c.Y() * invs;
            q.Z() = c.Z() * invs;
            q.W() = s * T( 0.5 );
            q.normalise();
        }
        return q;
    }

    template <class T>
    Quaternion<T> MathUtil<T>::getOrientationFromDirection( const Vector3<T> &vec )
    {
        // Call the full version with common defaults:
        // - Use negative Z as the local direction (forward direction in many 3D systems)
        // - Enable yaw fixing for stable orientation
        // - Use Y axis as the fixed yaw axis (Y-up convention)
        return getOrientationFromDirection( vec, -Vector3<T>::UNIT_Z, true, Vector3<T>::UNIT_Y );
    }

    template <class T>
    auto MathUtil<T>::getOrientationFromDirection( const Vector3<T> &vec,
                                                   const Vector3<T> &localDirectionVector,
                                                   bool bYawFixed, const Vector3<T> &yawFixedAxis )
        -> Quaternion<T>
    {
        WP_ASSERT( vec.isValid() );
        WP_ASSERT( localDirectionVector.isValid() );
        WP_ASSERT( yawFixedAxis.isValid() );

        // Do nothing if given a zero vector
        if( vec == Vector3<T>::zero() )
        {
            return Quaternion<T>::identity();
        }

        // The direction we want the local direction point to
        auto targetDir = vec.normaliseCopy();

        // Calculate target orientation relative to world space
        Quaternion<T> targetOrientation;

        if( bYawFixed )
        {
            // Calculate the quaternion for rotate local Z to target direction
            auto xVec = yawFixedAxis.crossProduct( targetDir );
            xVec.normalise();

            auto yVec = targetDir.crossProduct( xVec );
            yVec.normalise();

            auto unitZToTarget = Quaternion<T>( xVec, yVec, targetDir );

            if( localDirectionVector == -Vector3<T>::UNIT_Z )
            {
                // Special case for avoid calculate 180 degree turn
                targetOrientation = Quaternion<T>( -unitZToTarget.Y(), -unitZToTarget.Z(),
                                                   unitZToTarget.W(), unitZToTarget.X() );
            }
            else
            {
                // Calculate the quaternion for rotate local direction to target direction
                auto localToUnitZ =
                    Quaternion<T>::getRotationTo( localDirectionVector, Vector3<T>::UNIT_Z );
                targetOrientation = unitZToTarget * localToUnitZ;
            }
        }
        else
        {
            auto currentOrient = Quaternion<T>::identity();

            // Get current local direction relative to world space
            auto currentDir = currentOrient * localDirectionVector;

            if( ( currentDir + targetDir ).lengthSquared() < 0.00005f )
            {
                // Oops, a 180 degree turn (infinite possible rotation axes)
                // Default to yaw i.e. use current UP
                targetOrientation = Quaternion<T>( -currentOrient.Y(), -currentOrient.Z(),
                                                   currentOrient.W(), currentOrient.X() );
            }
            else
            {
                // Derive shortest arc to new direction
                auto rotQuat = Quaternion<T>::getRotationTo( currentDir, targetDir );
                targetOrientation = rotQuat * currentOrient;
            }
        }

        targetOrientation.normalise();

        WP_ASSERT( targetOrientation.isSane() );
        return targetOrientation;
    }

    template <class T>
    auto MathUtil<T>::reflect( const Vector3<T> &vec, const Vector3<T> &normal ) -> Vector3<T>
    {
        return vec - T( 2.0 ) * normal * normal.dotProduct( vec );
    }

    template <class T>
    auto MathUtil<T>::toSpherical( const Vector3<T> &cartesian ) -> Vector3<T>
    {
        T x = cartesian.x;
        T y = cartesian.y;
        T z = cartesian.z;

        T r = (T)std::sqrt( x * x + y * y + z * z );
        T theta = (T)std::atan2( y, x );  // azimuthal angle (0 to 2π)
        T phi = (T)std::acos( z / r );    // polar angle (0 to π)

        return Vector3<T>{ r, theta, phi };
    }

    template <class T>
    auto MathUtil<T>::toCartesian( const Vector3<T> &spherical ) -> Vector3<T>
    {
        T r = spherical.x;
        T theta = spherical.y;
        T phi = spherical.z;

        T sinPhi = (T)std::sin( phi );
        T x = r * sinPhi * (T)std::cos( theta );
        T y = r * sinPhi * (T)std::sin( theta );
        T z = r * (T)std::cos( phi );

        return Vector3<T>{ x, y, z };
    }

    template <class T>
    auto MathUtil<T>::toPosition( const Vector3<T> &rotation, Vector3<T> &up, Vector3<T> &pitchVec,
                                  Vector3<T> &yawVec ) -> Vector3<T>
    {
        Quaternion<T> yaw;
        Quaternion<T> pitch;

        yaw.fromAngleAxis( rotation.Y(), Vector3<T>::UNIT_Y );
        pitch.fromAngleAxis( rotation.Z(), Vector3<T>::UNIT_X );

        yawVec = ( yaw * -Vector3<T>::UNIT_Z );
        pitchVec = ( pitch * -Vector3<T>::UNIT_Z );

        up = ( yaw * pitch ) * Vector3<T>::UNIT_Y;
        return ( ( yaw * pitch ) * -Vector3<T>::UNIT_Z ) * rotation.X();
    }

    template <class T>
    auto MathUtil<T>::toRotation( const Vector3<T> &cartesian, const Vector3<T> &pitchVec,
                                  const Vector3<T> &yawVec ) -> Vector3<T>
    {
        Vector3<T> spherical;
        spherical.X() = cartesian.length();
        spherical.Y() = Math<T>::wrapRadians( Math<T>::ATan2( -yawVec.X(), -yawVec.Z() ) );
        spherical.Z() = Math<T>::wrapRadians( Math<T>::ATan2( pitchVec.Y(), -pitchVec.Z() ) );
        return spherical;
    }

    template <class T>
    auto MathUtil<T>::average( const Array<T> &v ) -> T
    {
        auto sum = T( 0.0 );
        for( auto &value : v )
        {
            sum += value;
        }

        return sum / static_cast<T>( v.size() );
    }

    template <class T>
    auto MathUtil<T>::average( const Deque<T> &v ) -> T
    {
        auto sum = T( 0.0 );
        for( auto &value : v )
        {
            sum += value;
        }

        return sum / static_cast<T>( v.size() );
    }

    template <class T>
    auto MathUtil<T>::latlong_distance( const Vector2<T> &lat_long_1, const Vector2<T> &lat_long_2 ) -> T
    {
        auto lat1 = lat_long_1[0];
        auto lon1 = lat_long_1[1];
        auto lat2 = lat_long_2[0];
        auto lon2 = lat_long_2[1];

        auto dlon = lon2 - lon1;
        auto dlat = lat2 - lat1;

        auto a =
            Math<T>::Sin( dlat / T( 2.0 ) ) * T( 2.0 ) +
            Math<T>::Cos( lat1 ) * Math<T>::Cos( lat2 ) * Math<T>::Sin( dlon / T( 2.0 ) ) * T( 2.0 );
        auto c = T( 2.0 ) * Math<T>::ATan2( Math<T>::Sqrt( a ), Math<T>::Sqrt( T( 1.0 ) - a ) );

        // approximate radius of earth in km
        const auto R = T( 6373.0 );
        auto distance = R * c;
        return distance * T( 1000.0 );
    }

    template <class T>
    void MathUtil<T>::swap( Vector3<T> &a, Vector3<T> &b )
    {
        auto temp = a;
        a = b;
        b = temp;
    }

    template <class T>
    auto MathUtil<T>::round( const Vector3<T> &v, s32 decimals ) -> Vector3<T>
    {
        auto value = Math<T>::Pow( T( 10 ), static_cast<T>( decimals ) );
        return Vector3<T>( Math<T>::round( v.X() * value ) / value,
                           Math<T>::round( v.Y() * value ) / value,
                           Math<T>::round( v.Z() * value ) / value );
    }

    template <class T>
    void MathUtil<T>::repair( Vector3<T> &Min, Vector3<T> &Max )
    {
        T t;

        if( Min.X() > Max.X() )
        {
            t = Min.X();
            Min.X() = Max.X();
            Max.X() = t;
        }

        if( Min.Y() > Max.Y() )
        {
            t = Min.Y();
            Min.Y() = Max.Y();
            Max.Y() = t;
        }

        if( Min.Z() > Max.Z() )
        {
            t = Min.Z();
            Min.Z() = Max.Z();
            Max.Z() = t;
        }
    }

    template <class T>
    template <class IT_TYPE>
    void MathUtil<T>::lowest( IT_TYPE begin, IT_TYPE end, T &value )
    {
        value = T( 1e10 );
        IT_TYPE i = begin;
        for( ; i != end; ++i )
        {
            value = std::min( value, *i );
        }
    }

    template <class T>
    template <class IT_TYPE>
    void MathUtil<T>::highest( IT_TYPE begin, IT_TYPE end, T &value )
    {
        value = T( -1e10 );
        IT_TYPE i = begin;
        for( ; i != end; ++i )
        {
            value = std::max( value, *i );
        }
    }

    template <class T>
    auto MathUtil<T>::transformFromPlaneEquation( const Plane3<T> plane ) -> Transform3<T>
    {
        return Transform3<T>();
    }

    template <class T>
    auto MathUtil<T>::intersects( const Ray3<T> &ray, const AABB3<T> &box ) -> Pair<bool, T>
    {
        if( box.isNull() )
        {
            return workphone::make_pair<bool, T>( false, T( 0.0 ) );
        }

        if( box.isInfinite() )
        {
            return workphone::make_pair<bool, T>( true, T( 0.0 ) );
        }

        auto lowt = T( 0.0 );
        auto t = T( 0.0 );
        auto hit = false;

        Vector3<T> hitpoint;
        auto min = box.getMinimum();
        auto max = box.getMaximum();
        auto rayorig = ray.getOrigin();
        auto raydir = ray.getDirection();

        // Check origin inside first
        if( rayorig > min && rayorig < max )
        {
            return workphone::make_pair<bool, T>( true, T( 0.0 ) );
        }

        // Check each face in turn, only check closest 3
        // Min x
        if( rayorig.X() <= min.X() && raydir.X() > T( 0.0 ) )
        {
            t = ( min.X() - rayorig.X() ) / raydir.X();

            // Substitute t back into ray and check bounds and dist
            hitpoint = rayorig + raydir * t;
            if( hitpoint.Y() >= min.Y() && hitpoint.Y() <= max.Y() && hitpoint.Z() >= min.Z() &&
                hitpoint.Z() <= max.Z() && ( !hit || t < lowt ) )
            {
                hit = true;
                lowt = t;
            }
        }
        // Max x
        if( rayorig.X() >= max.X() && raydir.X() < T( 0.0 ) )
        {
            t = ( max.X() - rayorig.X() ) / raydir.X();

            // Substitute t back into ray and check bounds and dist
            hitpoint = rayorig + raydir * t;
            if( hitpoint.Y() >= min.Y() && hitpoint.Y() <= max.Y() && hitpoint.Z() >= min.Z() &&
                hitpoint.Z() <= max.Z() && ( !hit || t < lowt ) )
            {
                hit = true;
                lowt = t;
            }
        }

        // Min y
        if( rayorig.Y() <= min.Y() && raydir.Y() > T( 0.0 ) )
        {
            t = ( min.Y() - rayorig.Y() ) / raydir.Y();

            // Substitute t back into ray and check bounds and dist
            hitpoint = rayorig + raydir * t;
            if( hitpoint.X() >= min.X() && hitpoint.X() <= max.X() && hitpoint.Z() >= min.Z() &&
                hitpoint.Z() <= max.Z() && ( !hit || t < lowt ) )
            {
                hit = true;
                lowt = t;
            }
        }

        // Max y
        if( rayorig.Y() >= max.Y() && raydir.Y() < T( 0.0 ) )
        {
            t = ( max.Y() - rayorig.Y() ) / raydir.Y();

            // Substitute t back into ray and check bounds and dist
            hitpoint = rayorig + raydir * t;
            if( hitpoint.X() >= min.X() && hitpoint.X() <= max.X() && hitpoint.Z() >= min.Z() &&
                hitpoint.Z() <= max.Z() && ( !hit || t < lowt ) )
            {
                hit = true;
                lowt = t;
            }
        }

        // Min z
        if( rayorig.Z() <= min.Z() && raydir.Z() > T( 0.0 ) )
        {
            t = ( min.Z() - rayorig.Z() ) / raydir.Z();

            // Substitute t back into ray and check bounds and dist
            hitpoint = rayorig + raydir * t;
            if( hitpoint.X() >= min.X() && hitpoint.X() <= max.X() && hitpoint.Y() >= min.Y() &&
                hitpoint.Y() <= max.Y() && ( !hit || t < lowt ) )
            {
                hit = true;
                lowt = t;
            }
        }

        // Max z
        if( rayorig.Z() >= max.Z() && raydir.Z() < T( 0.0 ) )
        {
            t = ( max.Z() - rayorig.Z() ) / raydir.Z();

            // Substitute t back into ray and check bounds and dist
            hitpoint = rayorig + raydir * t;
            if( hitpoint.X() >= min.X() && hitpoint.X() <= max.X() && hitpoint.Y() >= min.Y() &&
                hitpoint.Y() <= max.Y() && ( !hit || t < lowt ) )
            {
                hit = true;
                lowt = t;
            }
        }

        return Pair<bool, T>( hit, lowt );
    }

    template <class T>
    auto MathUtil<T>::intersects( const Ray3<T> &ray, const Sphere3<T> &sphere,
                                  bool discardInside /*= true*/ ) -> Pair<bool, T>
    {
        // Adjust ray origin relative to sphere center
        auto rayorig = ray.getOrigin() - sphere.getCenter();
        auto rayDirection = ray.getDirection();
        auto radius = sphere.getRadius();

        // Check origin inside first
        if( rayorig.lengthSquared() <= radius * radius && discardInside )
        {
            return workphone::make_pair( true, T( 0.0 ) );
        }

        // Mmm, quadratics
        // Build coeffs which can be used with std quadratic solver
        // ie t = (-b +/- sqrt(b*b + 4ac)) / 2a
        auto a = rayDirection.dotProduct( rayDirection );
        auto b = T( 2.0 ) * rayorig.dotProduct( rayDirection );
        auto c = rayorig.dotProduct( rayorig ) - radius * radius;

        // Calc determinant
        auto d = ( b * b ) - ( T( 4.0 ) * a * c );
        if( d < 0 )
        {
            // No intersection
            return workphone::make_pair( false, T( 0.0 ) );
        }
        // BTW, if d=0 there is one intersection, if d > 0 there are 2
        // But we only want the closest one, so that's ok, just use the
        // '-' version of the solver
        auto t = ( -b - Math<T>::Sqrt( d ) ) / ( T( 2.0 ) * a );
        if( t < T( 0.0 ) )
        {
            t = ( -b + Math<T>::Sqrt( d ) ) / ( T( 2.0 ) * a );
        }

        return workphone::make_pair( true, t );
    }

    template <class T>
    auto MathUtil<T>::intersects( const Ray3<T> &ray, const Cylinder3<T> &cylinder ) -> Pair<bool, T>
    {
        T t[2];
        auto cylinderAxis = cylinder.getAxis();
        auto cylinderDirection = cylinderAxis.getDirection();
        auto cylinderHeight = cylinder.getHeight();
        auto cylinderRadius = cylinder.getRadius();

        Vector3<T> U, V, W = cylinderDirection;
        Vector3<T>::generateComplementBasis( U, V, W );
        auto halfHeight = static_cast<T>( 0.5 ) * cylinderHeight;
        auto rSqr = cylinderRadius * cylinderRadius;

        // convert incoming line origin to cylinder coordinates
        auto origin = ray.getOrigin();
        auto dir = ray.getDirection();

        auto diff = origin - cylinderAxis.getStart();
        auto P = Vector3<T>( U.dotProduct( diff ), V.dotProduct( diff ), W.dotProduct( diff ) );

        // Get the z-value, in cylinder coordinates, of the incoming line's
        // unit-length direction.
        auto dz = W.dotProduct( dir );

        if( Math<T>::Abs( dz ) >= T( 1.0 ) - Math<T>::epsilon() )
        {
            // The line is parallel to the cylinder axis.  Determine if the line
            // intersects the cylinder end disks.
            auto radialSqrDist = rSqr - P.X() * P.X() - P.Y() * P.Y();
            if( radialSqrDist < static_cast<T>( 0 ) )
            {
                // Line outside the cylinder, no intersection.
                return workphone::make_pair( false, T( 0.0 ) );
            }

            // Line intersects the cylinder end disks.
            if( dz > static_cast<T>( 0 ) )
            {
                t[0] = -P.Z() - halfHeight;
                t[1] = -P.Z() + halfHeight;
            }
            else
            {
                t[0] = P.Z() - halfHeight;
                t[1] = P.Z() + halfHeight;
            }

            return workphone::make_pair( true, t[0] );
        }

        // convert incoming line unit-length direction to cylinder coordinates
        Vector3<T> D( U.dotProduct( dir ), V.dotProduct( dir ), dz );

        T a0, a1, a2, discr, root, inv, tValue;

        if( Math<T>::Abs( D.Z() ) <= Math<T>::epsilon() )
        {
            // The line is perpendicular to the cylinder axis.
            if( Math<T>::Abs( P.Z() ) > halfHeight )
            {
                // Line is outside the planes of the cylinder end disks.
                return workphone::make_pair( false, T( 0.0 ) );
            }

            // Test intersection of line P+t*D with infinite cylinder
            // x^2+y^2 = r^2.  This reduces to computing the roots of a
            // quadratic equation.  If P = (px,py,pz) and D = (dx,dy,dz),
            // then the quadratic equation is
            //   (dx^2+dy^2)*t^2 + 2*(px*dx+py*dy)*t + (px^2+py^2-r^2) = 0
            a0 = P.X() * P.X() + P.Y() * P.Y() - rSqr;
            a1 = P.X() * D.X() + P.Y() * D.Y();
            a2 = D.X() * D.X() + D.Y() * D.Y();
            discr = a1 * a1 - a0 * a2;
            if( discr < static_cast<T>( 0 ) )
            {
                // Line does not intersect cylinder.
                return workphone::make_pair( false, T( 0.0 ) );
            }
            if( discr > Math<T>::epsilon() )
            {
                // Line intersects cylinder in two places.
                root = Math<T>::Sqrt( discr );
                inv = static_cast<T>( 1 ) / a2;
                t[0] = ( -a1 - root ) * inv;
                t[1] = ( -a1 + root ) * inv;
                return workphone::make_pair( true, t[0] );
            }
            // Line is tangent to the cylinder.
            t[0] = -a1 / a2;
            return workphone::make_pair( true, t[0] );
        }

        // Test plane intersections first.
        int quantity = 0;
        inv = static_cast<T>( 1.0 ) / D.Z();

        auto t0 = ( -halfHeight - P.Z() ) * inv;
        auto xTmp = P.X() + t0 * D.X();
        auto yTmp = P.Y() + t0 * D.Y();
        if( xTmp * xTmp + yTmp * yTmp <= rSqr )
        {
            // Planar intersection inside the top cylinder end disk.
            t[quantity++] = t0;
        }

        auto t1 = ( +halfHeight - P.Z() ) * inv;
        xTmp = P.X() + t1 * D.X();
        yTmp = P.Y() + t1 * D.Y();
        if( xTmp * xTmp + yTmp * yTmp <= rSqr )
        {
            // Planar intersection inside the bottom cylinder end disk.
            t[quantity++] = t1;
        }

        if( quantity == 2 )
        {
            // Line intersects both top and bottom cylinder end disks.
            if( t[0] > t[1] )
            {
                auto save = t[0];
                t[0] = t[1];
                t[1] = save;
            }

            return Pair<bool, T>( true, t[0] );
        }

        // If quantity == 1, then the line must intersect cylinder wall in a
        // single point somewhere between the end disks.  This case is detected
        // in the following code that tests for intersection between line and
        // cylinder wall.
        a0 = P.X() * P.X() + P.Y() * P.Y() - rSqr;
        a1 = P.X() * D.X() + P.Y() * D.Y();
        a2 = D.X() * D.X() + D.Y() * D.Y();
        discr = a1 * a1 - a0 * a2;
        if( discr < static_cast<T>( 0 ) )
        {
            // Line does not intersect cylinder wall.
            WP_ASSERT( quantity == 0 );
            return Pair<bool, T>( false, T( 0.0 ) );
        }
        if( discr > Math<T>::epsilon() )
        {
            root = Math<T>::Sqrt( discr );
            inv = static_cast<T>( 1 ) / a2;
            tValue = ( -a1 - root ) * inv;
            if( t0 <= t1 )
            {
                if( t0 <= tValue && tValue <= t1 )
                {
                    t[quantity++] = tValue;
                }
            }
            else
            {
                if( t1 <= tValue && tValue <= t0 )
                {
                    t[quantity++] = tValue;
                }
            }

            if( quantity == 2 )
            {
                // Line intersects one of the cylinder end disks and once on the
                // cylinder wall.
                if( t[0] > t[1] )
                {
                    auto save = t[0];
                    t[0] = t[1];
                    t[1] = save;
                }

                return Pair<bool, T>( true, t[0] );
            }

            tValue = ( -a1 + root ) * inv;
            if( t0 <= t1 )
            {
                if( t0 <= tValue && tValue <= t1 )
                {
                    t[quantity++] = tValue;
                }
            }
            else
            {
                if( t1 <= tValue && tValue <= t0 )
                {
                    t[quantity++] = tValue;
                }
            }
        }
        else
        {
            tValue = -a1 / a2;
            if( t0 <= t1 )
            {
                if( t0 <= tValue && tValue <= t1 )
                {
                    t[quantity++] = tValue;
                }
            }
            else
            {
                if( t1 <= tValue && tValue <= t0 )
                {
                    t[quantity++] = tValue;
                }
            }
        }

        if( quantity == 2 )
        {
            if( t[0] > t[1] )
            {
                auto save = t[0];
                t[0] = t[1];
                t[1] = save;
            }
        }

        return Pair<bool, T>( quantity != 0, t[0] );
    }

    // explicit instantiation
    template class MathUtil<f32>;

    template class MathUtil<f64>;

    template <class T>
    f32 MathUtil<T>::intervalDistance( f32 minA, f32 maxA, f32 minB, f32 maxB )
    {
        if( minA < minB )
        {
            return minB - maxA;
        }

        return minA - maxB;
    }

    template <class T>
    void MathUtil<T>::getPolygonIntervals( const Polygon3<real_Num> &polygon,
                                           const Vector3<real_Num> &axis, f32 &min, f32 &max )
    {
        min = 1e10;
        max = -1e10;

        auto numPoints = polygon.getNumPoints();
        for( u32 polyIdx = 0; polyIdx < numPoints; ++polyIdx )
        {
            auto curPoint = polygon.getPoint( polyIdx );
            auto distance = axis.dotProduct( curPoint );

            if( min > distance )
            {
                min = distance;
            }

            if( max < distance )
            {
                max = distance;
            }
        }
    }

    template <class T>
    void MathUtil<T>::getPolygonIntervals( const Polygon2<real_Num> &polygon,
                                           const Vector2<real_Num> &axis, f32 &min, f32 &max )
    {
        min = 1e10;
        max = -1e10;

        auto numPoints = polygon.getNumPoints();
        for( u32 polyIdx = 0; polyIdx < numPoints; ++polyIdx )
        {
            auto curPoint = polygon.getPoint( polyIdx );
            auto distance = axis.dotProduct( curPoint );

            if( min > distance )
            {
                min = distance;
            }

            if( max < distance )
            {
                max = distance;
            }
        }
    }

    template <class T>
    workphone::Array<workphone::Vector2<T>> MathUtil<T>::orderPoints(
        const Array<Vector2<T>> &polygonPoints, const Vector2<T> &centerPoint, u8 pointOrdering )
    {
        WP_UNUSED( pointOrdering );

        Array<Vector2<T>> orderedPoints;
        orderedPoints.reserve( polygonPoints.size() );

        auto curAngle = T( 0.0 );
        auto minAngle = std::numeric_limits<T>::max();

        bool finished = false;
        while( !finished )
        {
            finished = true;

            Vector2<T> newPoint;

            auto newAngle = T( 0.0 );
            auto curDiff = std::numeric_limits<T>::max();

            for( u32 i = 0; i < polygonPoints.size(); ++i )
            {
                const auto &point = polygonPoints[i];
                auto vectorFromCenter = ( point - centerPoint ).normaliseCopy();
                auto pointAngle = Math<T>::ATan2( vectorFromCenter.X(), vectorFromCenter.Y() );
                pointAngle = Math<T>::RadToFullDegrees( pointAngle );

                auto diff = pointAngle - curAngle;

                if( diff > Math<T>::epsilon() && diff < curDiff )
                {
                    curDiff = diff;
                    minAngle = pointAngle;
                    newAngle = pointAngle;
                    newPoint = point;
                    finished = false;
                }
            }

            if( !finished )
            {
                curAngle = newAngle;
                orderedPoints.push_back( newPoint );
            }
        }

        return orderedPoints;
    }

    template <class T>
    workphone::Set<workphone::Vector2<T>> MathUtil<T>::orderPoints( const Set<Vector2<T>> &polygonPoints,
                                                                    const Vector2<T> &centerPoint,
                                                                    u8 pointOrdering )
    {
        WP_UNUSED( pointOrdering );

        Set<Vector2<T>> orderedPoints;

        auto curAngle = T( 0.0 );
        auto minAngle = std::numeric_limits<T>::max();

        bool finished = false;
        while( !finished )
        {
            finished = true;

            Vector2<T> newPoint;

            auto newAngle = T( 0.0 );
            auto curDiff = std::numeric_limits<T>::max();

            for( auto point : polygonPoints )
            {
                auto vectorFromCenter = ( point - centerPoint ).normaliseCopy();
                auto pointAngle = Math<T>::ATan2( static_cast<T>( vectorFromCenter.X() ),
                                                  static_cast<T>( vectorFromCenter.Y() ) );
                pointAngle = Math<T>::RadToFullDegrees( pointAngle );

                auto diff = pointAngle - curAngle;

                if( diff > Math<T>::epsilon() && diff < curDiff )
                {
                    curDiff = diff;
                    minAngle = pointAngle;
                    newAngle = pointAngle;
                    newPoint = point;
                    finished = false;
                }
            }

            if( !finished )
            {
                curAngle = newAngle;
                orderedPoints.insert( newPoint );
            }
        }

        return orderedPoints;
    }

    template <class T>
    workphone::Polygon2<T> MathUtil<T>::createPolygon( const Array<Vector2<T>> &points )
    {
        Polygon2<T> polygon( static_cast<u32>( points.size() ) );

        for( size_t polyPointIdx = 0; polyPointIdx < points.size(); ++polyPointIdx )
        {
            const auto &point = points[polyPointIdx];
            polygon.addPoint( point );
        }

        return polygon;
    }

    template <class T>
    workphone::Polygon2<T> MathUtil<T>::createPolygon( const Set<Vector2<T>> &points )
    {
        Polygon2<T> polygon( static_cast<u32>( points.size() ) );

        for( auto point : points )
        {
            polygon.addPoint( point );
        }

        return polygon;
    }

    template <class T>
    void MathUtil<T>::weldPoints( Set<Vector2<T>> &points, T tollerance )
    {
        for( auto pointA : points )
        {
            // check if another vertex is within range
            for( auto pointB : points )
            {
                if( pointA != pointB )
                {
                    // check if another vertex is within range
                    if( ( pointB - pointA ).length() < tollerance )
                    {
                        // points.erase(polyPointIdxB);
                        // polyPointIdx = 0;
                        break;
                    }
                }
            }
        }
    }

    template <class T>
    String MathUtil<T>::printPolygonData( const Polygon2F &polygon )
    {
        String polygonDataStr;
        auto numPoints = polygon.getNumPoints();

        polygonDataStr += String( "NumPoints: " );
        polygonDataStr += StringUtil::toString( numPoints );
        polygonDataStr += String( " \n" );

        for( u32 i = 0; i < numPoints; ++i )
        {
            const auto &point = polygon.getPoint( i );

            polygonDataStr += StringUtil::toString( point );
            polygonDataStr += String( " \n" );
        }

        return polygonDataStr;
    }

    template <class T>
    T MathUtil<T>::safeDivide( T value, T divisor )
    {
        return Math<T>::Abs( divisor ) > Math<T>::epsilon() ? value / divisor : value;
    }

}  // namespace workphone
