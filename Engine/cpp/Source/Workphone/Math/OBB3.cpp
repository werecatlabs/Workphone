#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Math/OBB3.hpp>

namespace workphone
{
    template <class T>
    const Vector3<T> OBB3<T>::s_referenceBoxCorners[8] = {
        Vector3<T>( -1, -1, 1 ), Vector3<T>( 1, -1, 1 ),   Vector3<T>( 1, 1, 1 ),
        Vector3<T>( -1, 1, 1 ),  Vector3<T>( -1, -1, -1 ), Vector3<T>( 1, -1, -1 ),
        Vector3<T>( 1, 1, -1 ),  Vector3<T>( -1, 1, -1 )
    };

    //-------------------------------------------------------------------------
    // Inline implementations
    //-------------------------------------------------------------------------

    template <class T>
    OBB3<T>::OBB3() :
        m_center( Vector3<T>::zero() ),
        m_halfExtents( Vector3<T>::zero() ),
        m_orientation( Quaternion<T>::identity() )
    {
    }

    template <class T>
    OBB3<T>::OBB3( const OBB3<T> &other ) :
        m_center( other.m_center ),
        m_halfExtents( other.m_halfExtents ),
        m_orientation( other.m_orientation )
    {
    }

    template <class T>
    OBB3<T>::OBB3( const Vector3<T> &center, const Vector3<T> &halfExtents ) :
        m_center( center ),
        m_halfExtents( halfExtents ),
        m_orientation( Quaternion<T>::identity() )
    {
        WP_ASSERT( isValid() );
    }

    template <class T>
    OBB3<T>::OBB3( const Vector3<T> &center, const Vector3<T> &halfExtents,
                   const Quaternion<T> &orientation ) :
        m_center( center ),
        m_halfExtents( halfExtents ),
        m_orientation( orientation )
    {
        WP_ASSERT( isValid() );
    }

    template <class T>
    OBB3<T>::OBB3( const AABB3<T> &aabb ) :
        m_center( aabb.getCenter() ),
        m_halfExtents( aabb.getSize() ),
        m_orientation( Quaternion<T>::identity() )
    {
    }

    template <class T>
    OBB3<T>::OBB3( const AABB3<T> &aabb, const Transform3<T> &transform )
    {
        // Treat the AABB as a local-space box placed by the transform.
        const auto &scale = transform.getScale();
        const auto absoluteScale = Vector3<T>( Math<T>::Abs( scale.X() ), Math<T>::Abs( scale.Y() ),
                                               Math<T>::Abs( scale.Z() ) );
        m_halfExtents = aabb.getSize() * absoluteScale;
        m_center =
            transform.getPosition() + transform.getOrientation().rotate( aabb.getCenter() * scale );
        m_orientation = transform.getOrientation();
    }

    // Find the approximate minimum oriented bounding box containing a set of points. This is the
    // inertia-tensor / eigenvector approach used by DirectXMath's BoundingOrientedBox::CreateFromPoints
    // and by the Esoterica OBB point constructor. A small self-contained Jacobi eigensolver is used
    // so this header has no external dependencies beyond Quaternion/Vector3.
    template <class T>
    OBB3<T>::OBB3( const Vector3<T> *pPoints, u32 numPoints )
    {
        WP_ASSERT( pPoints != nullptr );
        WP_ASSERT( numPoints > 0 );

        // Compute the center of mass of the points.
        Vector3<T> centerOfMass = Vector3<T>::zero();
        for( u32 i = 0; i < numPoints; ++i )
        {
            centerOfMass += pPoints[i];
        }

        centerOfMass /= static_cast<T>( numPoints );

        // Compute the covariance/inertia tensor of the points around the center of mass.
        T xx = T( 0.0 ), yy = T( 0.0 ), zz = T( 0.0 );
        T xy = T( 0.0 ), xz = T( 0.0 ), yz = T( 0.0 );

        for( u32 i = 0; i < numPoints; ++i )
        {
            Vector3<T> p = pPoints[i] - centerOfMass;
            xx += p.X() * p.X();
            yy += p.Y() * p.Y();
            zz += p.Z() * p.Z();
            xy += p.X() * p.Y();
            xz += p.X() * p.Z();
            yz += p.Y() * p.Z();
        }

        // Solve the eigenvectors of the symmetric 3x3 covariance matrix using Jacobi rotations.
        T A[3][3] = { { xx, xy, xz }, { xy, yy, yz }, { xz, yz, zz } };
        T V[3][3] = { { T( 1.0 ), T( 0.0 ), T( 0.0 ) },
                      { T( 0.0 ), T( 1.0 ), T( 0.0 ) },
                      { T( 0.0 ), T( 0.0 ), T( 1.0 ) } };

        const u32 maxIterations = 64;
        for( u32 iter = 0; iter < maxIterations; ++iter )
        {
            // Find the largest off-diagonal element.
            T maxOff = T( 0.0 );
            int p = 0, q = 1;
            for( int i = 0; i < 3; ++i )
            {
                for( int j = i + 1; j < 3; ++j )
                {
                    T a = Math<T>::Abs( A[i][j] );
                    if( a > maxOff )
                    {
                        maxOff = a;
                        p = i;
                        q = j;
                    }
                }
            }

            if( maxOff <= T( 1e-12 ) )
            {
                break;
            }

            // Compute the Jacobi rotation that zeroes A[p][q].
            T app = A[p][p];
            T aqq = A[q][q];
            T apq = A[p][q];
            T phi = T( 0.5 ) * Math<T>::ATan2( T( 2.0 ) * apq, aqq - app );
            T c = Math<T>::Cos( phi );
            T s = Math<T>::Sin( phi );

            // Update A, maintaining symmetry (only the off-diagonal rows/cols for k != p,q).
            for( int k = 0; k < 3; ++k )
            {
                if( k == p || k == q )
                {
                    continue;
                }

                T akp = A[k][p];
                T akq = A[k][q];
                A[k][p] = akp * c + akq * s;
                A[p][k] = A[k][p];
                A[k][q] = -akp * s + akq * c;
                A[q][k] = A[k][q];
            }
            A[p][p] = app * c * c + aqq * s * s + T( 2.0 ) * apq * c * s;
            A[q][q] = app * s * s + aqq * c * c - T( 2.0 ) * apq * c * s;
            A[p][q] = T( 0.0 );
            A[q][p] = T( 0.0 );

            // Update V.
            for( int k = 0; k < 3; ++k )
            {
                T vkp = V[k][p];
                T vkq = V[k][q];
                V[k][p] = vkp * c + vkq * s;
                V[k][q] = -vkp * s + vkq * c;
            }
        }

        // The eigenvectors are the columns of V. Build a rotation matrix whose rows are the
        // eigenvectors, normalize them and ensure a right-handed basis.
        Vector3<T> v1( V[0][0], V[1][0], V[2][0] );
        Vector3<T> v2( V[0][1], V[1][1], V[2][1] );
        Vector3<T> v3( V[0][2], V[1][2], V[2][2] );

        v1.normalise();
        v2.normalise();
        v3 = v1.crossProduct( v2 );
        v3.normalise();

        Matrix3<T> R;
        R[0][0] = v1.X();
        R[0][1] = v1.Y();
        R[0][2] = v1.Z();
        R[1][0] = v2.X();
        R[1][1] = v2.Y();
        R[1][2] = v2.Z();
        R[2][0] = v3.X();
        R[2][1] = v3.Y();
        R[2][2] = v3.Z();

        Quaternion<T> orientation( R );
        orientation.normalise();

        // Project all points into the box's local space (orientation maps local->world, so
        // rotateInv maps world->local) and find the min/max.
        Vector3<T> vMin = orientation.rotateInv( pPoints[0] - centerOfMass );
        Vector3<T> vMax = vMin;

        for( u32 i = 1; i < numPoints; ++i )
        {
            Vector3<T> localPoint = orientation.rotateInv( pPoints[i] - centerOfMass );
            vMin.makeFloor( localPoint );
            vMax.makeCeil( localPoint );
        }

        m_halfExtents = ( vMax - vMin ) * T( 0.5 );
        m_orientation = orientation;

        // Rotate the local center back into world space and add the center of mass.
        Vector3<T> localCenter = vMin + m_halfExtents;
        m_center = centerOfMass + orientation.rotate( localCenter );
    }

    template <class T>
    bool OBB3<T>::operator==( const OBB3<T> &other ) const
    {
        return m_center == other.m_center && m_halfExtents == other.m_halfExtents &&
               m_orientation == other.m_orientation;
    }

    template <class T>
    bool OBB3<T>::operator!=( const OBB3<T> &other ) const
    {
        return !( *this == other );
    }

    template <class T>
    const Vector3<T> &OBB3<T>::getCenter() const
    {
        return m_center;
    }

    template <class T>
    const Vector3<T> &OBB3<T>::getHalfExtents() const
    {
        return m_halfExtents;
    }

    template <class T>
    Vector3<T> OBB3<T>::getExtent() const
    {
        return m_halfExtents * T( 2.0 );
    }

    template <class T>
    const Quaternion<T> &OBB3<T>::getOrientation() const
    {
        return m_orientation;
    }

    template <class T>
    Vector3<T> OBB3<T>::getAxis( u32 axisIndex ) const
    {
        WP_ASSERT( axisIndex < 3 );
        const Vector3<T> basis[3] = { Vector3<T>::unitX(), Vector3<T>::unitY(), Vector3<T>::unitZ() };
        return m_orientation.rotate( basis[axisIndex] );
    }

    template <class T>
    void OBB3<T>::setCenter( const Vector3<T> &center )
    {
        m_center = center;
    }

    template <class T>
    void OBB3<T>::setHalfExtents( const Vector3<T> &halfExtents )
    {
        m_halfExtents = halfExtents;
    }

    template <class T>
    void OBB3<T>::setOrientation( const Quaternion<T> &orientation )
    {
        m_orientation = orientation;
    }

    template <class T>
    void OBB3<T>::translate( const Vector3<T> &delta )
    {
        m_center += delta;
    }

    template <class T>
    void OBB3<T>::rotate( const Quaternion<T> &deltaRotation )
    {
        m_orientation = deltaRotation * m_orientation;
    }

    template <class T>
    void OBB3<T>::getCorners( Vector3<T> *corners ) const
    {
        WP_ASSERT( corners != nullptr );

        const Vector3<T> basis[3] = { Vector3<T>::unitX(), Vector3<T>::unitY(), Vector3<T>::unitZ() };
        Vector3<T> axisX = m_orientation.rotate( basis[0] );
        Vector3<T> axisY = m_orientation.rotate( basis[1] );
        Vector3<T> axisZ = m_orientation.rotate( basis[2] );

        for( int i = 0; i < 8; ++i )
        {
            const Vector3<T> &signs = s_referenceBoxCorners[i];  // components are -1 or +1
            Vector3<T> offset = axisX * ( m_halfExtents.X() * signs.X() ) +
                                axisY * ( m_halfExtents.Y() * signs.Y() ) +
                                axisZ * ( m_halfExtents.Z() * signs.Z() );
            corners[i] = m_center + offset;
        }
    }

    template <class T>
    AABB3<T> OBB3<T>::getAABB() const
    {
        Vector3<T> corners[8];
        getCorners( corners );

        Vector3<T> vMin = corners[0];
        Vector3<T> vMax = corners[0];
        for( int i = 1; i < 8; ++i )
        {
            vMin.makeFloor( corners[i] );
            vMax.makeCeil( corners[i] );
        }

        AABB3<T> aabb;
        aabb.setExtents( vMin, vMax );
        return aabb;
    }

    template <class T>
    bool OBB3<T>::containsPoint( const Vector3<T> &point ) const
    {
        // Transform the point into the box's local space and check it lies within the half extents.
        Vector3<T> localPoint = m_orientation.rotateInv( point - m_center );
        return ( Math<T>::Abs( localPoint.X() ) <= m_halfExtents.X() ) &&
               ( Math<T>::Abs( localPoint.Y() ) <= m_halfExtents.Y() ) &&
               ( Math<T>::Abs( localPoint.Z() ) <= m_halfExtents.Z() );
    }

    // Separating axis theorem overlap test for two oriented boxes. Checks the 3 axes of each box
    // and the 9 axes formed by cross products of pairs of axes (15 axes in total). The axes are
    // obtained directly from the orientations via Quaternion::rotate, so no matrix multiply is
    // required.
    template <class T>
    bool OBB3<T>::overlaps( const OBB3<T> &other ) const
    {
        const Vector3<T> basis[3] = { Vector3<T>::unitX(), Vector3<T>::unitY(), Vector3<T>::unitZ() };

        Vector3<T> aAxis[3] = { m_orientation.rotate( basis[0] ), m_orientation.rotate( basis[1] ),
                                m_orientation.rotate( basis[2] ) };
        Vector3<T> bAxis[3] = { other.m_orientation.rotate( basis[0] ),
                                other.m_orientation.rotate( basis[1] ),
                                other.m_orientation.rotate( basis[2] ) };

        Vector3<T> t = other.m_center - m_center;

        const T epsilon = T( 1e-6 );

        // Helper: project both boxes onto a unit axis and test for separation.
        auto testAxis = [&]( const Vector3<T> &axis, T ra, T rb ) -> bool {
            T d = Math<T>::Abs( t.dotProduct( axis ) );
            return d <= ( ra + rb );
        };

        // Precompute the absolute dot products of each axis pair: R[i][j] = aAxis[i] . bAxis[j].
        T R[3][3];
        T AbsR[3][3];
        for( int i = 0; i < 3; ++i )
        {
            for( int j = 0; j < 3; ++j )
            {
                R[i][j] = aAxis[i].dotProduct( bAxis[j] );
                AbsR[i][j] = Math<T>::Abs( R[i][j] ) + epsilon;
            }
        }

        const Vector3<T> &hA = m_halfExtents;
        const Vector3<T> &hB = other.m_halfExtents;

        // Axes from box A.
        for( int i = 0; i < 3; ++i )
        {
            T ra = hA[i];
            T rb = hB.X() * AbsR[i][0] + hB.Y() * AbsR[i][1] + hB.Z() * AbsR[i][2];
            if( !testAxis( aAxis[i], ra, rb ) )
            {
                return false;
            }
        }

        // Axes from box B.
        for( int j = 0; j < 3; ++j )
        {
            T ra = hA.X() * AbsR[0][j] + hA.Y() * AbsR[1][j] + hA.Z() * AbsR[2][j];
            T rb = hB[j];
            if( !testAxis( bAxis[j], ra, rb ) )
            {
                return false;
            }
        }

        // Axes from cross products of pairs of local axes.
        for( int i = 0; i < 3; ++i )
        {
            for( int j = 0; j < 3; ++j )
            {
                Vector3<T> axis = aAxis[i].crossProduct( bAxis[j] );
                T len2 = axis.dotProduct( axis );
                if( len2 <= epsilon * epsilon )
                {
                    continue;  // Skip degenerate cross-product axes.
                }

                axis /= Math<T>::Sqrt( len2 );

                int i1 = ( i + 1 ) % 3;
                int i2 = ( i + 2 ) % 3;
                int j1 = ( j + 1 ) % 3;
                int j2 = ( j + 2 ) % 3;

                T ra = hA[i1] * AbsR[i2][j] + hA[i2] * AbsR[i1][j];
                T rb = hB[j1] * AbsR[i][j2] + hB[j2] * AbsR[i][j1];

                if( !testAxis( axis, ra, rb ) )
                {
                    return false;
                }
            }
        }

        return true;
    }

    template <class T>
    bool OBB3<T>::overlaps( const AABB3<T> &aabb ) const
    {
        OBB3<T> other( aabb );
        return overlaps( other );
    }

    template <class T>
    void OBB3<T>::applyTransform( const Transform3<T> &transform )
    {
        // Treat the box as a local-space box placed by the transform: scale the half extents in
        // the box's local frame, rotate/scale the center and translate it, then compose rotations.
        const Vector3<T> &position = transform.getPosition();
        const Quaternion<T> &rotation = transform.getOrientation();
        const Vector3<T> &scale = transform.getScale();

        const auto absoluteScale = Vector3<T>( Math<T>::Abs( scale.X() ), Math<T>::Abs( scale.Y() ),
                                               Math<T>::Abs( scale.Z() ) );
        m_halfExtents = m_halfExtents * absoluteScale;
        m_center = position + rotation.rotate( m_center * scale );
        m_orientation = rotation * m_orientation;
    }

    template <class T>
    void OBB3<T>::applyScale( const Vector3<T> &scale )
    {
        m_halfExtents = m_halfExtents * Vector3<T>( Math<T>::Abs( scale.X() ), Math<T>::Abs( scale.Y() ),
                                                    Math<T>::Abs( scale.Z() ) );
    }

    template <class T>
    OBB3<T> OBB3<T>::getTransformed( const Transform3<T> &transform ) const
    {
        OBB3<T> result( *this );
        result.applyTransform( transform );
        return result;
    }

    template <class T>
    bool OBB3<T>::isValid() const
    {
        return m_halfExtents.X() >= T( 0.0 ) && m_halfExtents.Y() >= T( 0.0 ) &&
               m_halfExtents.Z() >= T( 0.0 ) && m_halfExtents.isValid();
    }

    template <class T>
    void OBB3<T>::reset()
    {
        m_center = Vector3<T>::zero();
        m_halfExtents = Vector3<T>::zero();
        m_orientation = Quaternion<T>::identity();
    }

    // explicit instantiation
    template class OBB3<f32>;
    template class OBB3<f64>;
}  // namespace workphone
