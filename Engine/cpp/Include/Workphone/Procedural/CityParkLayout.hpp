#pragma once

namespace cityParkLandscape
{
    struct Path
    {
        Vector3F a, b;
        float halfWidth = 0;
    };

    struct Layout
    {
        const OpenCityLayout::Lot *lot = nullptr;
        Vector3F entry, centre;
        SourceBudgetVector<Path> paths;
        bool hasEntry = false;
        bool hasSeat = false;
        Vector3F seat, seatForward;
        float seatHalfSpan = 1.30f;
        std::array<float, 4> seatFootOffsets{};
    };

    inline bool clearOfPaths( const Layout &layout, Vector3F point, float radius )
    {
        for( const auto &path : layout.paths )
        {
            auto d = path.b - path.a, v = point - path.a;
            d.y = v.y = 0;
            const auto length = d.lengthSquared();
            if( length > 1e-8f )
            {
                v -= d * std::clamp( v.dotProduct( d ) / length, 0.f, 1.f );
            }
            const auto clearance = path.halfWidth + radius;
            if( v.lengthSquared() < clearance * clearance )
            {
                return false;
            }
        }
        if( layout.hasSeat )
        {
            const auto v = point - layout.seat;
            const Vector3F side( -layout.seatForward.z, 0, layout.seatForward.x );
            if( std::abs( v.dotProduct( side ) ) < layout.seatHalfSpan + radius &&
                std::abs( v.dotProduct( layout.seatForward ) ) < .83f + radius )
            {
                return false;
            }
        }
        return true;
    }
}  // namespace cityParkLandscape
