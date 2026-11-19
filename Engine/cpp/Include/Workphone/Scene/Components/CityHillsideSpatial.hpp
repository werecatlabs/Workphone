#pragma once

// Spatial helpers shared by the foliage planting and the city street builders.
// Included by the scene generation internal header, so every function is inline.
namespace cityHillside
{
    using Cell = std::pair<s32, s32>;

    inline Cell cellFor( Vector3F point )
    {
        return { s32( std::floor( point.x / 16.f ) ), s32( std::floor( point.z / 16.f ) ) };
    }

    template <class Visit>
    bool visitCells( Vector3F low, Vector3F high, const Visit &visit )
    {
        const auto first = cellFor( low ), last = cellFor( high );
        for( auto x = first.first; x <= last.first; ++x )
        {
            for( auto z = first.second; z <= last.second; ++z )
            {
                if( !visit( Cell( x, z ) ) )
                {
                    return false;
                }
            }
        }
        return true;
    }

    inline float unit( u64 key )
    {
        return float( render::foliageStableHash( key ) >> 40 ) / 16777216.f;
    }

    inline float cross( Vector3F a, Vector3F b )
    {
        return a.x * b.z - a.z * b.x;
    }

    inline float segmentDistance( Vector3F point, Vector3F a, Vector3F b )
    {
        auto direction = b - a;
        auto offset = point - a;
        direction.y = offset.y = 0;
        const float length = direction.lengthSquared();
        if( length > 0 )
        {
            offset -= direction * std::clamp( direction.dotProduct( offset ) / length, 0.f, 1.f );
        }
        return offset.lengthSquared();
    }
}  // namespace cityHillside
