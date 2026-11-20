#pragma once

#include <algorithm>
#include <cmath>

namespace workphone::scene::cityRoadFootprint
{
    struct Rectangle
    {
        double x, z, width, depth;
    };

    struct Segment
    {
        double ax, az, bx, bz, clearance;
    };

    inline double pointSegmentSquared( double x, double z, const Segment &segment )
    {
        const double dx = segment.bx - segment.ax, dz = segment.bz - segment.az;
        const double length = dx * dx + dz * dz;
        const double t =
            length > 0 ? std::clamp( ( ( x - segment.ax ) * dx + ( z - segment.az ) * dz ) / length,
                                     0., 1. )
                       : 0;
        const double ox = x - segment.ax - dx * t, oz = z - segment.az - dz * t;
        return ox * ox + oz * oz;
    }

    inline double segmentRectangleSquared( const Segment &segment, const Rectangle &r )
    {
        const double x0 = r.x - r.width * .5, x1 = r.x + r.width * .5;
        const double z0 = r.z - r.depth * .5, z1 = r.z + r.depth * .5;
        double first = 0, last = 1;
        const auto clip = [&]( double p, double q ) {
            if( std::abs( p ) < 1e-12 )
            {
                return q >= 0;
            }
            const double t = q / p;
            if( p < 0 )
            {
                first = std::max( first, t );
            }
            else
            {
                last = std::min( last, t );
            }
            return first <= last;
        };
        if( clip( segment.ax - segment.bx, segment.ax - x0 ) &&
            clip( segment.bx - segment.ax, x1 - segment.ax ) &&
            clip( segment.az - segment.bz, segment.az - z0 ) &&
            clip( segment.bz - segment.az, z1 - segment.az ) )
        {
            return 0;
        }
        const auto pointBox = [&]( double x, double z ) {
            const double dx = std::max( { x0 - x, 0., x - x1 } ),
                         dz = std::max( { z0 - z, 0., z - z1 } );
            return dx * dx + dz * dz;
        };
        return std::min(
            { pointBox( segment.ax, segment.az ), pointBox( segment.bx, segment.bz ),
              pointSegmentSquared( x0, z0, segment ), pointSegmentSquared( x1, z0, segment ),
              pointSegmentSquared( x0, z1, segment ), pointSegmentSquared( x1, z1, segment ) } );
    }

    inline bool overlaps( const Segment &segment, const Rectangle &footprint )
    {
        return segmentRectangleSquared( segment, footprint ) <= segment.clearance * segment.clearance;
    }
}  // namespace workphone::scene::cityRoadFootprint
