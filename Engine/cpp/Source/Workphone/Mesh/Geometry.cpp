#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Mesh/Geometry.hpp>

namespace workphone
{

    void Geometry::quad( Vector3F a, Vector3F b, Vector3F c, Vector3F d, Vector3F normal,
                         float u0 /*= 0*/, float u1 /*= 1*/, float v0 /*= 0*/, float v1 /*= 1 */ )
    {
        const auto base = static_cast<u32>( positions.size() );
        auto tangent = b - a;
        if( tangent.lengthSquared() < 1e-8f )
        {
            tangent = Vector3F( 1, 0, 0 );
        }
        tangent.normalise();
        for( auto p : { a, b, c, d } )
        {
            positions.push_back( p );
            normals.push_back( normal );
            tangents.push_back( { tangent.x, tangent.y, tangent.z, 1 } );
        }
        uv.push_back( { u0, v0, 0 } );
        uv.push_back( { u1, v0, 0 } );
        uv.push_back( { u1, v1, 0 } );
        uv.push_back( { u0, v1, 0 } );
        const auto winding = ( b - a ).crossProduct( c - a ).dotProduct( normal ) >= 0;
        for( auto i :
             winding ? std::array<u32, 6>{ 0, 1, 2, 0, 2, 3 } : std::array<u32, 6>{ 0, 2, 1, 0, 3, 2 } )
        {
            indices.push_back( base + i );
        }
    }

    void Geometry::beam( Vector3F a, Vector3F b, float radius )
    {
        auto axis = b - a;
        if( axis.lengthSquared() < 1e-8f || radius <= 0 )
        {
            return;
        }
        axis.normalise();
        auto u =
            axis.crossProduct( std::abs( axis.y ) > .9f ? Vector3F( 1, 0, 0 ) : Vector3F( 0, 1, 0 ) );
        u.normalise();
        const auto v = axis.crossProduct( u );
        const auto ur = u * radius, vr = v * radius;
        quad( a + ur - vr, b + ur - vr, b + ur + vr, a + ur + vr, u );
        quad( a - ur + vr, b - ur + vr, b - ur - vr, a - ur - vr, -u );
        quad( a + ur + vr, b + ur + vr, b - ur + vr, a - ur + vr, v );
        quad( a - ur - vr, b - ur - vr, b + ur - vr, a + ur - vr, -v );
        quad( a - ur - vr, a + ur - vr, a + ur + vr, a - ur + vr, -axis );
        quad( b - ur - vr, b + ur - vr, b + ur + vr, b - ur + vr, axis );
    }

    void Geometry::box( Vector3F p, Vector3F size )
    {
        auto lo = p - size * 0.5f, hi = p + size * 0.5f;
        quad( { lo.x, hi.y, lo.z }, { hi.x, hi.y, lo.z }, { hi.x, hi.y, hi.z }, { lo.x, hi.y, hi.z },
              { 0, 1, 0 } );
        quad( { lo.x, lo.y, hi.z }, { hi.x, lo.y, hi.z }, { hi.x, lo.y, lo.z }, { lo.x, lo.y, lo.z },
              { 0, -1, 0 } );
        quad( { lo.x, lo.y, lo.z }, { hi.x, lo.y, lo.z }, { hi.x, hi.y, lo.z }, { lo.x, hi.y, lo.z },
              { 0, 0, -1 } );
        quad( { hi.x, lo.y, hi.z }, { lo.x, lo.y, hi.z }, { lo.x, hi.y, hi.z }, { hi.x, hi.y, hi.z },
              { 0, 0, 1 } );
        quad( { lo.x, lo.y, hi.z }, { lo.x, lo.y, lo.z }, { lo.x, hi.y, lo.z }, { lo.x, hi.y, hi.z },
              { -1, 0, 0 } );
        quad( { hi.x, lo.y, lo.z }, { hi.x, lo.y, hi.z }, { hi.x, hi.y, hi.z }, { hi.x, hi.y, lo.z },
              { 1, 0, 0 } );
    }

}  // namespace workphone
