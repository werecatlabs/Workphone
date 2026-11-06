#ifndef WP_CLAW_DRAW_SORT_HPP
#define WP_CLAW_DRAW_SORT_HPP
#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <WorkphoneGraphics/workphone_graphics_cull_job.h>
#include <cstdint>

namespace workphone::render
{
    enum class ClawDrawSortStrategy : uint32_t
    {
        QueueOrder = WP_GRAPHICS_DRAW_QUEUE_ORDER,
        FrontToBack = WP_GRAPHICS_DRAW_FRONT_TO_BACK,
        MaterialFirst = WP_GRAPHICS_DRAW_MATERIAL_FIRST,
        DepthBuckets = WP_GRAPHICS_DRAW_DEPTH_BUCKETS
    };

    using ClawDrawSortKey = wp_graphics_draw_key;

    WPGraphics_API bool validDrawSort( ClawDrawSortStrategy strategy, double bucketSize );
    WPGraphics_API void sortDrawKeys( ClawDrawSortKey *keys, size_t count, ClawDrawSortStrategy strategy,
                                      double bucketSize = 10.0 );
}  // namespace workphone::render
#endif
