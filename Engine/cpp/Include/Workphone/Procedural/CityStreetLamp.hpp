#pragma once

#include <Workphone/Mesh/Geometry.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <array>
#include <cmath>

namespace workphone::scene::cityStreetLamp
{
    // A continuous tubular shaft bends into a side-entry lantern. All offsets are
    // in the pole's vertical plane, with positive reach towards the carriageway.
    inline void build( Geometry &steel, Geometry &housing, Geometry &glass, Vector3F base,
                       Vector3F roadward, bool pedestrian = false )
    {
        const Vector3F up( 0, 1, 0 ), side( roadward.z, 0, -roadward.x );
        const float height = pedestrian ? 3.6f : 6.2f;
        const float reach = pedestrian ? .65f : 1.38f;

        struct Section
        {
            float reach, height, radius;
        };

        constexpr size_t bendSegments = 8;
        std::array<Section, bendSegments + 5> mast{};
        mast[0] = { 0, 0, .078f };
        mast[1] = { 0, .65f, .078f };
        mast[2] = { 0, .75f, .065f };
        const float rise = pedestrian ? .39f : .60f;
        for( size_t i = 0; i <= bendSegments; ++i )
        {
            const float t = float( i ) / bendSegments;
            const float angle = t * 1.57079633f;
            mast[3 + i] = { reach * .66f * ( 1.f - std::cos( angle ) ),
                            height - rise * ( 1.f - std::sin( angle ) ), .047f - .007f * t };
        }
        mast.back() = { reach, height, .04f };
        const auto station = [&]( size_t index ) {
            return base + roadward * mast[index].reach + up * mast[index].height;
        };
        constexpr u32 sides = 8;
        const u32 first = u32( steel.positions.size() );
        for( size_t j = 0; j < mast.size(); ++j )
        {
            const auto tangent =
                ( station( j + 1 < mast.size() ? j + 1 : j ) - station( j > 0 ? j - 1 : j ) )
                    .normaliseCopy();
            const auto radial = side.crossProduct( tangent );
            for( u32 i = 0; i < sides; ++i )
            {
                const float angle = float( i ) * .7853981634f;
                const auto normal = side * std::cos( angle ) + radial * std::sin( angle );
                steel.positions.push_back( station( j ) + normal * mast[j].radius );
                steel.normals.push_back( normal );
                steel.uv.push_back( { float( i ) / sides, float( j ), 0 } );
                steel.tangents.push_back( { tangent.x, tangent.y, tangent.z, 1 } );
                if( j > 0 )
                {
                    const u32 a = first + u32( j - 1 ) * sides + i;
                    const u32 b = first + u32( j - 1 ) * sides + ( i + 1 ) % sides;
                    for( const auto index : { a, a + sides, b, b, a + sides, b + sides } )
                    {
                        steel.indices.push_back( index );
                    }
                }
            }
        }

        housing.box( base + up * .018f, { .278f, .036f, .278f } );
        for( float x : { -.104f, .104f } )
        {
            for( float z : { -.104f, .104f } )
            {
                steel.box( base + Vector3F( x, .045f, z ), { .034f, .018f, .034f } );
            }
        }
        // The service door follows two facets of the shaft and opens to the pavement.
        const auto front = -side * .70710678f - roadward * .70710678f;
        const auto left = base - side * .080f, right = base - roadward * .080f;
        const auto middle = base + front * .081f;
        for( const auto end : { left, right } )
        {
            housing.quad( end + up * .21f, middle + up * .19f, middle + up * .56f, end + up * .54f,
                          ( end - base + front * .08f ).normaliseCopy() );
        }

        // The casting has a narrow spigot end, a broad optical compartment
        // and a rolled lip around the inset, downward-facing diffuser.
        const auto centre = station( mast.size() - 1 ) + roadward * ( pedestrian ? .18f : .28f );
        const float length = pedestrian ? .32f : .47f, width = pedestrian ? .16f : .215f;
        const std::array<Vector2F, 8> outline = { { { -1, -.37f },
                                                    { -.32f, -.94f },
                                                    { .62f, -1 },
                                                    { 1, -.63f },
                                                    { 1, .63f },
                                                    { .62f, 1 },
                                                    { -.32f, .94f },
                                                    { -1, .37f } } };
        const auto ring = [&]( float scale, float elevation ) {
            std::array<Vector3F, 8> points;
            for( size_t i = 0; i < points.size(); ++i )
            {
                points[i] = centre + roadward * ( outline[i].x * length * scale ) +
                            side * ( outline[i].y * width * scale ) + up * elevation;
            }
            return points;
        };
        const auto crown = ring( .72f, .095f ), rim = ring( 1, .022f );
        const auto lip = ring( .98f, -.052f );
        auto lens = ring( .75f, -.039f );
        for( auto &p : lens )
        {
            p += roadward * ( length * .11f );
        }
        for( size_t i = 0; i < outline.size(); ++i )
        {
            const size_t j = ( i + 1 ) % outline.size();
            const auto edge = rim[j] - rim[i];
            const auto facing = edge.crossProduct( up ).normaliseCopy();
            housing.quad( crown[i], crown[j], rim[j], rim[i],
                          ( rim[i] - crown[i] ).crossProduct( edge ).normaliseCopy() );
            housing.quad( rim[i], rim[j], lip[j], lip[i], facing );
            housing.quad( lip[i], lip[j], lens[j], lens[i], -up );
        }
        for( size_t i = 1; i < outline.size() - 1; i += 2 )
        {
            housing.quad( crown[0], crown[i], crown[i + 1], crown[i + 2], up );
            const auto firstUV = glass.uv.size();
            glass.quad( lens[0], lens[i], lens[i + 1], lens[i + 2], -up );
            size_t uv = firstUV;
            for( const size_t corner : { size_t( 0 ), i, i + 1, i + 2 } )
            {
                glass.uv[uv++] = { .5f + .5f * outline[corner].x, .5f + .5f * outline[corner].y, 0 };
            }
        }
    }
}  // namespace workphone::scene::cityStreetLamp
