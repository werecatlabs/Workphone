#ifndef WP_CLAW_DRAW_SORT_HPP
#define WP_CLAW_DRAW_SORT_HPP
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace workphone::render
{
    enum class ClawDrawSortStrategy : uint32_t
    {
        QueueOrder, FrontToBack, MaterialFirst, DepthBuckets
    };

    /** Numeric owner-thread sort keys. Depth is positive camera-forward distance
     * to the object origin. Barriers retain their positions; queue/z-order runs
     * never cross. Equal keys retain submission order without sort allocations. */
    struct ClawDrawSortKey
    {
        uint32_t index = 0, queue = 0, zOrder = 0, material = 0;
        double depth = 0;
        bool transparent = false, barrier = true;
    };

    inline bool validDrawSort( ClawDrawSortStrategy strategy, double bucketSize )
    {
        return strategy >= ClawDrawSortStrategy::QueueOrder &&
            strategy <= ClawDrawSortStrategy::DepthBuckets &&
            std::isfinite( bucketSize ) && bucketSize > 0;
    }

    inline void sortDrawKeys( ClawDrawSortKey *keys, size_t count,
                              ClawDrawSortStrategy strategy, double bucketSize = 10.0 )
    {
        if( !validDrawSort( strategy, bucketSize ) || strategy == ClawDrawSortStrategy::QueueOrder ) return;
        size_t begin = 0;
        while( begin < count )
        {
            // Nonfinite depths must not enter a floating-point comparator.
            if( keys[begin].barrier || !std::isfinite( keys[begin].depth ) ) { ++begin; continue; }
            auto end = begin + 1;
            while( end < count && !keys[end].barrier && std::isfinite( keys[end].depth ) &&
                   keys[end].queue == keys[begin].queue && keys[end].zOrder == keys[begin].zOrder ) ++end;
            std::sort( keys + begin, keys + end, [=]( const auto &a, const auto &b ) {
                if( a.transparent != b.transparent ) return !a.transparent;
                if( a.transparent )
                {
                    if( a.depth != b.depth ) return a.depth > b.depth;
                }
                else
                {
                    if( strategy == ClawDrawSortStrategy::FrontToBack && a.depth != b.depth )
                        return a.depth < b.depth;
                    if( strategy == ClawDrawSortStrategy::DepthBuckets )
                    {
                        const auto ad = std::floor( a.depth / bucketSize ), bd = std::floor( b.depth / bucketSize );
                        if( ad != bd ) return ad < bd;
                    }
                    if( a.material != b.material ) return a.material < b.material;
                    if( a.depth != b.depth ) return a.depth < b.depth;
                }
                return a.index < b.index;
            } );
            begin = end;
        }
    }
}
#endif
