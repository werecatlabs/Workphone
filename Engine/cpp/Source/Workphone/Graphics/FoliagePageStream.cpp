#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/FoliagePageStream.hpp>
#include <WorkphoneGraphics/workphone_graphics_paged_geometry.h>
#include <WorkphoneGraphics/workphone_graphics_camera.h>
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>

namespace workphone::render
{
    struct FoliagePageStream::Impl
    {
        struct SourceOwner { StreamBudget budget; FoliageSnapshot snapshot; };
        struct Node
        {
            FoliageCellKey cell;
            wp_geometry_page *page = nullptr;
            u64 token = 0;
            bool wanted = false;
        };
        struct View
        {
            StreamBudget budget;
            wp_camera *camera = nullptr;
            wp_paged_geometry *pager = nullptr;
            Array<Node *> nodes;
            f32 range = 0;
            ~View() { wp_paged_geometry_destroy( pager ); wp_camera_destroy( camera ); }
            static void *create( void *data )
            {
                auto self = static_cast<View *>( data );
                try { auto node = std::make_unique<Node>(); self->nodes.push_back( node.get() ); return node.release(); }
                catch( ... ) { return nullptr; }
            }
            static void destroy( void *data ) { delete static_cast<Node *>( data ); }
            static void load( void *, wp_geometry_page *page, wp_geometry_page_info *info )
            {
                auto node = static_cast<Node *>( wp_geometry_page_get_data( page ) );
                node->cell = { info->x_index, info->z_index }; node->page = page;
                node->token = wp_geometry_page_get_load_generation( page ); node->wanted = true;
                info->user_data = node;
                wp_geometry_page_defer_load( page );
            }
            static void unload( void *, wp_geometry_page_info *info )
            {
                if( auto node = static_cast<Node *>( info->user_data ) ) node->wanted = false;
            }
        };
        std::shared_ptr<WorldStreamingScheduler> scheduler = WorldStreamingScheduler::shared();
        std::shared_ptr<WorldStreamProducer> producer;
        FoliageStreamSource input;
        Array<FoliageCellKey> requested;
        std::set<FoliageCellKey> available, outstanding;
        std::map<FoliageCellKey, u32> counts;
        std::map<FoliageCellKey, std::weak_ptr<const FoliagePageData>> delivered;
        Array<std::unique_ptr<View>> views;
        Impl()
        {
            WorldStreamProducerLimits limits;
            limits.maximumReadyBytes = 64ull << 20;
            String error;
            producer = scheduler->createProducer( limits, error );
            if( !producer ) throw std::runtime_error( error.c_str() );
        }
        ~Impl() { producer->drain(); }
        void acknowledge( FoliageCellKey cell, bool success )
        {
            for( const auto &view : views ) for( auto node : view->nodes )
                if( node->wanted && node->cell == cell )
                {
                    if( success && wp_geometry_page_get_load_state( node->page ) == WORKPHONE_PAGE_FAILED )
                    {
                        wp_paged_geometry_reload_page( view->pager, wp_geometry_page_get_center( node->page ) );
                        wp_paged_geometry_update( view->pager, 0 );
                    }
                    wp_geometry_page_complete_load( node->page, node->token, success ? 1 : 0 );
                }
        }
    };
    FoliagePageStream::FoliagePageStream() : m_impl( std::make_unique<Impl>() ) {}
    FoliagePageStream::~FoliagePageStream() = default;
    bool FoliagePageStream::setSource( FoliageSnapshot source, String &error )
    {
        if( !source || !validateFoliageWorld( *source, error ) ) return false;
        try
        {
            struct IndexOwner
            {
                StreamBudget budget;
                std::map<FoliageCellKey, Array<u32>> indices;
            };
            auto indexOwner = std::make_shared<IndexOwner>();
            indexOwner->budget = m_impl->producer->reserve( StreamMemoryClass::SourceCpu,
                source->instances.size() * sizeof( u32 ) * 2 + foliageMaximumPages * 128ull + 4096, error );
            if( !indexOwner->budget ) return false;
            auto indices = std::shared_ptr<std::map<FoliageCellKey, Array<u32>>>( indexOwner, &indexOwner->indices );
            FoliageStreamSource input;
            input.snapshot = source;
            for( u32 i = 0; i < source->instances.size(); ++i )
            {
                FoliageCellKey cell;
                if( !foliageCellForPosition( source->instances[i].position, source->pageSize, cell, error ) ) return false;
                ( *indices )[cell].push_back( i );
            }
            for( const auto &entry : *indices ) { input.cells.push_back( entry.first ); input.pageInstanceCounts.push_back( u32( entry.second.size() ) ); }
            input.load = [source, indices]( FoliageCellKey cell, const StreamCancellation &cancel, String &error ) {
                auto page = std::make_shared<FoliagePageData>(); page->cell = cell; page->revision = source->revision;
                const auto found = indices->find( cell );
                if( found != indices->end() )
                {
                    page->instances.reserve( found->second.size() );
                    for( auto index : found->second )
                    {
                        if( cancel.cancelled() ) { error = "Foliage page preparation cancelled"; return std::shared_ptr<const FoliagePageData>(); }
                        page->instances.push_back( source->instances[index] );
                    }
                }
                error.clear(); return std::shared_ptr<const FoliagePageData>( std::move( page ) );
            };
            return setSource( std::move( input ), error );
        }
        catch( const std::exception &e ) { error = e.what(); return false; }
    }
    bool FoliagePageStream::setSource( FoliageStreamSource input, String &error )
    {
        error.clear();
        if( !input.snapshot || !input.load || input.cells.size() > foliageMaximumPages || input.cells.size() != input.pageInstanceCounts.size() ||
            !validateFoliageWorld( *input.snapshot, error ) )
        { if( error.empty() ) error = "Invalid foliage page source"; return false; }
        if(input.maximumGeometryRadius < 0 || input.maximumBendRadius < 0)
        {
            if(input.snapshot->instances.empty() && !input.cells.empty())
            {error="Lazy foliage sources require conservative instance extent metadata";return false;}
            if(!foliageInstanceRadii(*input.snapshot,input.snapshot->instances,
                input.maximumGeometryRadius,input.maximumBendRadius,error))return false;
        }
        if(!std::isfinite(input.maximumGeometryRadius)||!std::isfinite(input.maximumBendRadius)||
            input.maximumGeometryRadius<0||input.maximumBendRadius<0)
        {error="Invalid foliage source extent metadata";return false;}
        try
        {
            auto owner = std::make_shared<Impl::SourceOwner>();
            const auto bytes = foliageSourceBytes( *input.snapshot ) + input.cells.size() * 512 + 4096;
            owner->budget = m_impl->producer->reserve( StreamMemoryClass::SourceCpu, bytes, error );
            if( !owner->budget ) return false;
            owner->snapshot = input.snapshot;
            input.snapshot = FoliageSnapshot( owner, owner->snapshot.get() );
            std::set<FoliageCellKey> available( input.cells.begin(), input.cells.end() );
            if( available.size() != input.cells.size() ) { error = "Duplicate foliage source cell"; return false; }
            std::map<FoliageCellKey, u32> counts;
            for( size_t i = 0; i < input.cells.size(); ++i )
            {
                if( input.pageInstanceCounts[i] > foliageMaximumPageInstances ) { error = "Foliage page instance count exceeds its limit"; return false; }
                counts.emplace( input.cells[i], input.pageInstanceCounts[i] );
            }
            m_impl->producer->cancel(); m_impl->views.clear();
            m_impl->input = std::move( input ); m_impl->available = std::move( available );
            m_impl->counts = std::move( counts );
            m_impl->requested.clear(); m_impl->outstanding.clear(); m_impl->delivered.clear();
            return true;
        }
        catch( const std::exception &e ) { error = e.what(); return false; }
    }
    bool FoliagePageStream::setDemand( const Array<FoliageCellKey> &cells, String &error )
    {
        error.clear(); auto &p = *m_impl;
        if( !p.input.snapshot || cells.size() > foliageMaximumPages )
        { error = "Foliage demand requires a source and at most 2048 cells"; return false; }
        try
        {
            auto wanted = cells; std::sort( wanted.begin(), wanted.end() );
            wanted.erase( std::unique( wanted.begin(), wanted.end() ), wanted.end() );
            wanted.erase( std::remove_if( wanted.begin(), wanted.end(), [&]( auto key ) { return !p.available.count( key ); } ), wanted.end() );
            u64 residentBytes = 0;
            for( auto cell : wanted ) residentBytes += 1024 + u64( p.counts.at( cell ) ) * sizeof( FoliageInstance );
            if( residentBytes > p.scheduler->stats().limits[size_t( StreamMemoryClass::ResultCpu )] )
            { error = "Foliage resident page demand exceeds the shared CPU result budget"; return false; }
            const bool changed = wanted != p.requested;
            if( !changed && ( p.producer->stats().pending || p.producer->stats().active || p.producer->stats().ready ) ) return true;
            Array<WorldStreamRequest> requests;
            std::set<FoliageCellKey> outstanding;
            u64 newBytes = 0;
            for( auto cell : wanted )
            {
                const auto found = p.delivered.find( cell );
                if( found == p.delivered.end() || found->second.expired() )
                    newBytes += 1024 + u64( p.counts.at( cell ) ) * sizeof( FoliageInstance );
            }
            const auto budget = p.scheduler->stats();
            const auto category = size_t( StreamMemoryClass::ResultCpu );
            if( newBytes > budget.limits[category] - budget.used[category] )
            { error = "Foliage page demand cannot fit alongside retained CPU page results"; return false; }
            for( auto cell : wanted )
            {
                const auto found = p.delivered.find( cell );
                if( found != p.delivered.end() && !found->second.expired() ) { p.acknowledge( cell, true ); continue; }
                if( requests.size() == 256 ) break;
                WorldStreamRequest request;
                request.key = foliageCellIdentity( cell ); request.priority = s32( 256 - requests.size() );
                const auto expectedCount = p.counts.at( cell );
                request.workingBytes = ( 1ull << 20 ) + u64( expectedCount ) * 256;
                request.maximumResultBytes = 1024 + u64( expectedCount ) * sizeof( FoliageInstance );
                request.execute = [source = p.input.snapshot, load = p.input.load, cell, expectedCount,
                    geometryRadius=p.input.maximumGeometryRadius,bendRadius=p.input.maximumBendRadius]( const StreamCancellation &cancel ) {
                    WorldStreamResult result;
                    if( cancel.cancelled() ) return result;
                    auto page = load( cell, cancel, result.error );
                    if( !page ) { if( result.error.empty() ) result.error = "Foliage page loader returned no page"; return result; }
                    if( page->cell != cell || page->instances.size() != expectedCount || !validateFoliagePage( *page, *source, result.error ) )
                    { if( result.error.empty() ) result.error = "Foliage page identity mismatch"; return result; }
                    f32 geometry=0,bend=0;
                    if(!foliageInstanceRadii(*source,page->instances,geometry,bend,result.error)||
                        geometry>geometryRadius*1.00001f+.0001f||bend>bendRadius*1.00001f+.0001f)
                    {result.error="Foliage page exceeds its declared paging extents";return result;}
                    auto typed = std::make_shared<FoliagePageResult>(); typed->source = source; typed->cell = cell; typed->page = page;
                    result.bytes = sizeof( FoliagePageResult ) + sizeof( FoliagePageData ) + page->instances.capacity() * sizeof( FoliageInstance );
                    result.payload = std::move( typed ); return result;
                };
                requests.push_back( std::move( request ) ); outstanding.insert( cell );
            }
            if( !changed && requests.empty() ) return true;
            if( !p.producer->replace( requests, error ) ) return false;
            p.requested = std::move( wanted ); p.outstanding = std::move( outstanding );
            for( auto it = p.delivered.begin(); it != p.delivered.end(); )
                if( !std::binary_search( p.requested.begin(), p.requested.end(), it->first ) ) it = p.delivered.erase( it );
                else ++it;
            return true;
        }
        catch( const std::exception &e ) { error = e.what(); return false; }
    }
    bool FoliagePageStream::updateViews( const Array<FoliagePageView> &inputs, String &error )
    {
        auto &p = *m_impl;
        if( !p.input.snapshot || inputs.empty() || inputs.size() > 16 )
        { error = "Foliage paging requires one to sixteen views and a source"; return false; }
        try
        {
            for( const auto &input : inputs )
                if( !std::isfinite( input.position.x ) || !std::isfinite( input.position.y ) || !std::isfinite( input.position.z ) ||
                    !std::isfinite( input.range ) || input.range <= 0 || input.range / p.input.snapshot->pageSize + 1 > 30 )
                { error = "Foliage view range plus its page fringe must fit thirty page widths"; return false; }
            bool recreate = p.views.size() != inputs.size();
            for( size_t i = 0; !recreate && i < inputs.size(); ++i ) recreate = inputs[i].range != p.views[i]->range;
            if( recreate )
            {
                Array<std::unique_ptr<Impl::View>> candidate;
                for( const auto &input : inputs )
                {
                    auto view = std::make_unique<Impl::View>(); view->range = input.range;
                    const auto nativeRange = input.range + p.input.snapshot->pageSize;
                    const auto side = u64( 2 * nativeRange / p.input.snapshot->pageSize ) + 4;
                    view->budget = p.producer->reserve( StreamMemoryClass::SourceCpu, side * side * 512 + 4096, error );
                    if( !view->budget ) return false;
                    view->camera = wp_camera_create();
                    view->pager = wp_paged_geometry_create( view->camera, p.input.snapshot->pageSize );
                    if( !view->camera || !view->pager ) { error = "Native foliage pager allocation failed"; return false; }
                    wp_geometry_page_callbacks callbacks{}; callbacks.create_page = Impl::View::create; callbacks.destroy_page = Impl::View::destroy;
                    wp_page_loader_callbacks loader{}; loader.load_page = Impl::View::load; loader.unload_page = Impl::View::unload;
                    wp_paged_geometry_set_page_loader( view->pager, &loader, view.get() );
                    // Native demand tests page centres. Include a complete page
                    // fringe so a short range at a grid corner still loads its cell.
                    if( !wp_paged_geometry_add_detail_level( view->pager, nativeRange, 0, &callbacks, view.get(), nullptr, 0 ) )
                    { error = "Native foliage pager grid exceeds its bounded allocation"; return false; }
                    candidate.push_back( std::move( view ) );
                }
                p.views.swap( candidate );
            }
            Array<FoliageCellKey> wanted;
            std::set<FoliageCellKey> unique;
            for( size_t i = 0; i < inputs.size(); ++i )
            {
                auto &view = *p.views[i]; const auto &input = inputs[i];
                wp_camera_set_position( view.camera, { input.position.x, input.position.y, input.position.z } );
                wp_paged_geometry_update( view.pager, 16 );
                for( auto node : view.nodes )
                    if( node->wanted && p.available.count( node->cell ) )
                    {
                        const auto centre = wp_geometry_page_get_center( node->page );
                        const auto dx = double( centre.x ) - input.position.x, dz = double( centre.z ) - input.position.z;
                        const auto range = double( input.range ) + p.input.snapshot->pageSize;
                        if( dx * dx + dz * dz <= range * range ) unique.insert( node->cell );
                    }
            }
            for( const auto &key : unique ) wanted.push_back( key );
            return setDemand( wanted, error );
        }
        catch( const std::exception &e ) { error = e.what(); return false; }
    }
    const Array<FoliageCellKey> &FoliagePageStream::demand() const { return m_impl->requested; }
    FoliageSnapshot FoliagePageStream::source() const { return m_impl->input.snapshot; }
    f32 FoliagePageStream::maximumGeometryRadius() const { return m_impl->input.maximumGeometryRadius; }
    f32 FoliagePageStream::maximumBendRadius() const { return m_impl->input.maximumBendRadius; }
    bool FoliagePageStream::take( FoliagePageResult &result )
    {
        auto &p = *m_impl; WorldStreamResult raw;
        if( !p.producer->take( raw ) ) return false;
        result = {}; result.source = p.input.snapshot; result.generation = raw.generation; result.error = raw.error;
        for( auto key : p.outstanding ) if( foliageCellIdentity( key ) == raw.key ) { result.cell = key; break; }
        if( raw.payload )
        {
            const auto typed = std::static_pointer_cast<const FoliagePageResult>( raw.payload );
            result = *typed; result.generation = raw.generation;
            result.page = std::shared_ptr<const FoliagePageData>( raw.payload, typed->page.get() );
            p.delivered[result.cell] = result.page;
        }
        p.outstanding.erase( result.cell ); p.acknowledge( result.cell, result.page != nullptr );
        return true;
    }
    void FoliagePageStream::cancel()
    {
        m_impl->producer->cancel(); m_impl->requested.clear(); m_impl->outstanding.clear();
        m_impl->delivered.clear(); m_impl->views.clear();
    }
    void FoliagePageStream::drain() { cancel(); m_impl->producer->drain(); m_impl->input = {}; m_impl->available.clear(); m_impl->counts.clear(); }
    WorldStreamProducerStats FoliagePageStream::stats() const { return m_impl->producer->stats(); }
}
