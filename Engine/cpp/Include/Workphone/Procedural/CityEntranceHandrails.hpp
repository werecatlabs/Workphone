#pragma once

namespace cityEntranceHandrails
{
    constexpr float height = .91f;
    constexpr float radius = .024f;
    constexpr float offset = .088f;
    constexpr float clearance = .27f;

    struct Stair
    {
        Vector3F floor, along, outward;
        float halfWidth, start, landing, tread, riser;
        int count;
    };

    struct Batches
    {
        Geometry metal;
        size_t lots = 0, posts = 0;
    };

    template <size_t Size>
    void tube( Geometry &metal, const std::array<Vector3F, Size> &points, size_t count,
               float tubeRadius )
    {
        constexpr u32 sides = 10;
        constexpr float tau = 6.28318530718f;
        const auto first = u32( metal.positions.size() );
        const auto axisAt = [&]( size_t i ) {
            return ( points[std::min( i + 1, count - 1 )] - points[i ? i - 1 : 0] ).normaliseCopy();
        };
        const auto vertex = [&]( Vector3F p, Vector3F normal, Vector3F tangent, float u, float v ) {
            metal.positions.push_back( p );
            metal.normals.push_back( normal );
            metal.uv.push_back( { u, v, 0 } );
            metal.tangents.push_back( { tangent.x, tangent.y, tangent.z, 1 } );
        };
        for( size_t i = 0; i < count; ++i )
        {
            const auto axis = axisAt( i );
            const auto up = std::abs( axis.y ) > .94f ? Vector3F::unitX() : Vector3F::unitY();
            const auto a = up.crossProduct( axis ).normaliseCopy(), b = axis.crossProduct( a );
            for( u32 side = 0; side < sides; ++side )
            {
                const auto angle = tau * side / sides;
                const auto normal = a * std::cos( angle ) + b * std::sin( angle );
                vertex( points[i] + normal * tubeRadius, normal, axis, float( side ) / sides,
                        float( i ) );
                if( i + 1 == count )
                {
                    continue;
                }
                const auto x = first + u32( i ) * sides + side;
                const auto y = first + u32( i ) * sides + ( side + 1 ) % sides;
                for( const auto index : { x, y, y + sides, x, y + sides, x + sides } )
                {
                    metal.indices.push_back( index );
                }
            }
        }
        for( const auto i : { size_t( 0 ), count - 1 } )
        {
            const auto normal = axisAt( i ) * ( i == 0 ? -1.f : 1.f );
            const auto tangent = ( std::abs( normal.y ) > .94f ? Vector3F::unitX() : Vector3F::unitY() )
                                     .crossProduct( normal )
                                     .normaliseCopy();
            const auto cap = u32( metal.positions.size() );
            const auto ring = first + u32( i ) * sides;
            vertex( points[i], normal, tangent, .5f, .5f );
            for( u32 side = 0; side < sides; ++side )
            {
                const auto p = metal.positions[ring + side];
                vertex( p, normal, tangent, 0, 0 );
                const auto a = cap + 1 + side, b = cap + 1 + ( side + 1 ) % sides;
                metal.indices.push_back( cap );
                metal.indices.push_back( i == 0 ? b : a );
                metal.indices.push_back( i == 0 ? a : b );
            }
        }
    }

    void handrail( Geometry &metal, const std::array<Vector3F, 6> &corners )
    {
        std::array<Vector3F, 24> curve{};
        size_t count = 0;
        curve[count++] = corners.front();
        for( size_t i = 1; i + 1 < corners.size(); ++i )
        {
            const auto previous = corners[i - 1] - corners[i], next = corners[i + 1] - corners[i];
            const auto a = previous.length(), b = next.length();
            const float corner = std::min( { .13f, .34f * a, .34f * b } );
            const auto start = corners[i] + previous * ( corner / a );
            const auto end = corners[i] + next * ( corner / b );
            for( int step = 0; step <= 4; ++step )
            {
                const float t = step * .25f, inv = 1.f - t;
                curve[count++] =
                    start * ( inv * inv ) + corners[i] * ( 2.f * inv * t ) + end * ( t * t );
            }
        }
        curve[count++] = corners.back();
        tube( metal, curve, count, radius );
    }

    template <class Surface, class Clear>
    bool append( Batches &batch, u64 lotId, const Stair &stair, const Surface &surface,
                 const Clear &clear )
    {
        if( stair.count < 2 || batch.lots >= 64 )
        {
            return false;
        }
        const auto firstRiser = stair.start + stair.landing;
        const auto apron = firstRiser + stair.count * stair.tread;
        const auto base = stair.floor.y - stair.count * stair.riser;
        const auto at = [&]( float station, float lateral, float elevation ) {
            auto p = stair.floor + stair.along * lateral + stair.outward * station;
            p.y = elevation;
            return p;
        };
        constexpr float initial = .20f, lower = .30f, ret = .13f;
        if( !clear( at( stair.start + initial - radius, -stair.halfWidth - clearance, 0 ),
                    at( apron + lower + radius, stair.halfWidth + clearance, 0 ) ) )
        {
            return false;
        }

        const auto railY = [&]( float station ) {
            const float t = std::clamp( ( station - firstRiser ) / ( apron - firstRiser ), 0.f, 1.f );
            return stair.floor.y + ( base - stair.floor.y ) * t + height;
        };
        const size_t postCount = stair.count >= 5 ? 3 : 2;
        const std::array stations{ stair.start + .46f, apron - stair.tread * .56f,
                                   firstRiser + float( stair.count / 2 ) * stair.tread -
                                       stair.tread * .47f };
        float ground[2][3]{};
        for( size_t side = 0; side < 2; ++side )
        {
            for( size_t i = 0; i < postCount; ++i )
            {
                const float direction = side == 0 ? -1.f : 1.f;
                auto p = at( stations[i], direction * ( stair.halfWidth + offset ), 0 );
                if( !surface( p ) || p.y > railY( stations[i] ) - .46f )
                {
                    return false;
                }
                ground[side][i] = p.y;
            }
        }

        const auto before = batch.metal.indices.size() / 3;
        for( size_t side = 0; side < 2; ++side )
        {
            const float direction = side == 0 ? -1.f : 1.f;
            const auto lateral = direction * ( stair.halfWidth + offset );
            handrail( batch.metal,
                      { at( stair.start + initial, lateral + direction * ret, stair.floor.y + height ),
                        at( stair.start + initial, lateral, stair.floor.y + height ),
                        at( firstRiser, lateral, stair.floor.y + height ),
                        at( apron, lateral, base + height ), at( apron + lower, lateral, base + height ),
                        at( apron + lower, lateral + direction * ret, base + height ) } );
            for( size_t i = 0; i < postCount; ++i )
            {
                const float station = stations[i];
                const int level = std::clamp(
                    int( std::floor( ( station - firstRiser ) / stair.tread ) ) + 1, 0, stair.count );
                const float treadHeight = stair.floor.y - stair.riser * level;
                const bool cheek = treadHeight - ground[side][i] >= .115f;
                const float mountHeight = cheek ? treadHeight - .070f : ground[side][i];
                const auto foot = at( station, lateral, mountHeight );
                const auto head = at( station, lateral, railY( station ) );
                tube( batch.metal, std::array{ foot, head }, 2, .017f );
                if( cheek )
                {
                    const auto face =
                        at( station, direction * ( stair.halfWidth + .014f ), mountHeight );
                    const Vector3F plate(
                        std::abs( stair.along.x ) * .028f + std::abs( stair.outward.x ) * .105f, .096f,
                        std::abs( stair.along.z ) * .028f + std::abs( stair.outward.z ) * .105f );
                    batch.metal.box( face, plate );
                    tube( batch.metal, std::array{ face, foot }, 2, .018f );
                }
                else
                {
                    const auto up = Vector3F::unitY();
                    tube( batch.metal, std::array{ foot - up * .027f, foot + up * .006f }, 2, .052f );
                }
            }
        }
        ++batch.lots;
        batch.posts += 2 * postCount;
        const auto faces = batch.metal.indices.size() / 3 - before;
        WP_LOG( String( "Terrain city entrance handrails lot=" ) + StringUtil::toString( lotId ) +
                ", risers=" + StringUtil::toString( stair.count ) + ", rails=2, posts=" +
                StringUtil::toString( 2 * postCount ) + ", triangles=" + StringUtil::toString( faces ) );
        return true;
    }

    void publish( SceneAssets &assets, const Batches &batch )
    {
        if( !batch.lots )
        {
            return;
        }
        auto finish = material( assets, { .152f, .173f, .179f, 1 }, .48f, .27f );
        const auto actor = mesh( assets, "Terrain city entrance handrails", batch.metal, finish );
        if( !actor )
        {
            throw std::runtime_error( "Cannot publish terrain-city entrance handrails." );
        }
        WP_LOG( String( "Terrain city entrance handrails lots=" ) + StringUtil::toString( batch.lots ) +
                ", rails=" + StringUtil::toString( 2 * batch.lots ) +
                ", posts=" + StringUtil::toString( batch.posts ) +
                ", triangles=" + StringUtil::toString( batch.metal.indices.size() / 3 ) );
    }
}  // namespace cityEntranceHandrails
