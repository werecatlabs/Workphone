#ifndef WPFoliagePageStream_h__
#define WPFoliagePageStream_h__

#include <Workphone/Graphics/FoliageData.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <functional>

namespace workphone::render
{
    using FoliagePageLoader = std::function<std::shared_ptr<const FoliagePageData>(
        FoliageCellKey, const StreamCancellation &, String & )>;
    struct FoliageStreamSource
    {
        FoliageSnapshot snapshot;  // Shared species; cooked sources need no eager instance population.
        Array<FoliageCellKey> cells;
        Array<u32> pageInstanceCounts;  // Exact counts, positionally matching cells.
        FoliagePageLoader load;
        // Bounds relative to instance origins, over every page. Negative values
        // derive bounds from a complete authored snapshot; lazy sources supply them.
        f32 maximumGeometryRadius = -1, maximumBendRadius = -1;
    };
    struct FoliagePageView
    {
        Vector3F position;
        f32 range = 500;
    };
    struct FoliagePageResult
    {
        FoliageSnapshot source;
        FoliageCellKey cell;
        u64 generation = 0;
        std::shared_ptr<const FoliagePageData> page;
        String error;
    };
    /** Owner-thread pager/consumer with immutable worker inputs. Native pager slots
     * identify demand only; their generations never authorize stale publication.
     * Results retain source/result leases after take, cancellation and destruction.
     */
    class WPCore_API FoliagePageStream final
    {
    public:
        FoliagePageStream();
        ~FoliagePageStream();
        bool setSource( FoliageSnapshot, String &error );
        bool setSource( FoliageStreamSource, String &error );
        bool setDemand( const Array<FoliageCellKey> &, String &error );
        // At most sixteen finite views; range/page-size <= 30. Missing/empty cells
        // are discarded. A canonical union avoids cancellation on camera reorder.
        bool updateViews( const Array<FoliagePageView> &, String &error );
        const Array<FoliageCellKey> &demand() const;
        FoliageSnapshot source() const;
        f32 maximumGeometryRadius() const;
        f32 maximumBendRadius() const;
        bool take( FoliagePageResult & );
        void cancel();
        void drain();
        WorldStreamProducerStats stats() const;
        FoliagePageStream( const FoliagePageStream & ) = delete;
        FoliagePageStream &operator=( const FoliagePageStream & ) = delete;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
}  // namespace workphone::render
#endif
