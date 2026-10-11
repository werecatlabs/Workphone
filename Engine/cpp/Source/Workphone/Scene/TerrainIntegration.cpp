#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/TerrainIntegration.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainSystem.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Memory/AtomicWeakPtr.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <mutex>
#ifdef _WIN32
#    include <Windows.h>
#endif

namespace workphone::scene
{
    class TerrainIntegrationPublication : public ISharedObject
    {
    public:
        render::TerrainSnapshot source;
        Transform3<real_Num> placement;
        bool detached = false;
        u64 serial = 0;
        WP_CLASS_REGISTER_DECL;
    };
    WP_CLASS_REGISTER_DERIVED( workphone::scene, TerrainIntegrationPublication, ISharedObject );
    WP_CLASS_REGISTER_DERIVED( workphone::scene, TerrainIntegrationBinding, ISharedObject );

    namespace
    {
        const hash_type terrainPublished = StringUtil::getHash( "terrainPublished" );
        struct NotificationFrame
        {
            const TerrainSystem *owner;
            NotificationFrame *previous;
        };
        thread_local NotificationFrame *notificationFrame = nullptr;

        bool validateIntegrationOptions( const TerrainIntegrationOptions &options, String &error )
        {
            TerrainGroundPopulation checked;
            if( !checked.setSeeds( options.seeds, error ) || !checked.setMasks( options.masks, error ) ) return false;
            if( options.maximumNavigationTriangles > 1048576 || options.maximumShorelineSegments > 1048576 ||
                options.pathAnchors.size() > TerrainGroundPopulation::maximumInstances || !std::isfinite( options.waterLevel ) )
            { error = "Terrain integration budgets exceed bounds or water level is not finite."; return false; }
            for( auto point : options.pathAnchors )
                if( !point.isFinite() ) { error = "Terrain path anchors must be finite."; return false; }
            error.clear(); return true;
        }

        bool buildIntegration( render::TerrainSnapshot source, const Transform3<real_Num> &placement,
            const TerrainIntegrationOptions &options, TerrainIntegrationSnapshot previous,
            bool resetGround, TerrainIntegrationSnapshot &output, String &error )
        {
            if( !source ) { output.reset(); error.clear(); return true; }
            try
            {
                auto candidate = std::make_shared<TerrainIntegrationData>();
                candidate->source = source;
                candidate->placement = placement;
                if( options.ground )
                {
                    if( previous && !resetGround ) candidate->ground = previous->ground;
                    else if( !candidate->ground.setSeeds( options.seeds, error ) ) return false;
                    if( !candidate->ground.setMasks( options.masks, error ) || !candidate->ground.update( source, placement, error ) ) return false;
                }
                if( options.navigation )
                {
                    const auto cells = size_t( source->dimensions.x - 1 ) * ( source->dimensions.y - 1 );
                    const auto solid = source->holes.empty() ? cells : size_t( std::count( source->holes.begin(), source->holes.end(), u8( 0 ) ) );
                    if( solid * 2 > options.maximumNavigationTriangles )
                    { error = "Terrain navigation input exceeds the configured triangle budget."; return false; }
                    if( !buildTerrainNavigationInput( source, placement, candidate->navigation, error ) ) return false;
                }
                if( options.shoreline && !buildTerrainShorelineInput( source, placement, options.waterLevel,
                    candidate->shoreline, error, options.maximumShorelineSegments ) ) return false;
                if( options.path && !sampleTerrainPathInput( source, placement, options.pathAnchors, candidate->path, error ) ) return false;
                output = std::move( candidate ); error.clear(); return true;
            }
            catch( const std::exception &e ) { error = e.what(); return false; }
        }
        Vector4F weightsAt( const render::TerrainData &data, size_t index )
        { return data.layerWeights.empty() ? Vector4F( 1, 0, 0, 0 ) : render::normalizeTerrainWeights( data.layerWeights[index] ); }
        f32 densityAt( const std::shared_ptr<const TerrainGroundMasks> &masks, size_t index )
        { return !masks || masks->density.empty() ? 1 : masks->density[index]; }
        u8 exclusionAt( const std::shared_ptr<const TerrainGroundMasks> &masks, size_t index )
        { return !masks || masks->exclusions.empty() ? 0 : masks->exclusions[index]; }
        template<class Value>
        auto interpolateSample( const render::TerrainData &data, const Vector2F &local, s32 x,
                                s32 z, const Value &value )
        {
            const f32 u = f32( std::clamp( ( double( local.x ) - data.origin.x ) / data.spacing.x - x, 0.0, 1.0 ) );
            const f32 v = f32( std::clamp( ( double( local.y ) - data.origin.y ) / data.spacing.y - z, 0.0, 1.0 ) );
            const auto a = size_t( z ) * data.dimensions.x + x;
            if( u + v <= 1 ) return value( a ) * ( 1-u-v ) + value( a+1 ) * u + value( a+data.dimensions.x ) * v;
            return value( a+data.dimensions.x+1 ) * ( u+v-1 ) + value( a+data.dimensions.x ) * ( 1-u ) + value( a+1 ) * ( 1-v );
        }
        bool acceptsSeed( const render::TerrainData &data, const TerrainGroundSeed &seed,
                          const std::shared_ptr<const TerrainGroundMasks> &masks, s32 x, s32 z )
        {
            if( exclusionAt( masks, size_t( z ) * ( data.dimensions.x - 1 ) + x ) || !seed.allowedLayers ) return false;
            if( seed.minimumLayerWeight > 0 )
            {
                const auto w = interpolateSample( data, seed.localPosition, x, z, [&]( size_t i ) { return weightsAt( data, i ); } );
                const f32 combined = ( seed.allowedLayers & 1 ? w.x : 0 ) + ( seed.allowedLayers & 2 ? w.y : 0 ) +
                                     ( seed.allowedLayers & 4 ? w.z : 0 ) + ( seed.allowedLayers & 8 ? w.w : 0 );
                if( combined + 1.e-6f < seed.minimumLayerWeight ) return false;
            }
            const auto density = interpolateSample( data, seed.localPosition, x, z, [&]( size_t i ) { return densityAt( masks, i ); } );
            u64 hash = seed.id + 0x9e3779b97f4a7c15ull;
            hash = ( hash ^ ( hash >> 30 ) ) * 0xbf58476d1ce4e5b9ull;
            hash = ( hash ^ ( hash >> 27 ) ) * 0x94d049bb133111ebull;
            hash ^= hash >> 31;
            return double( hash >> 11 ) / 9007199254740992.0 < density;
        }
        bool validateSource( render::TerrainSnapshot terrain, const Transform3<real_Num> &placement,
                             String &error )
        {
            if( !terrain )
            {
                error = "Terrain integration requires an authoritative snapshot.";
                return false;
            }
            return render::validateTerrainData( *terrain, error ) &&
                   render::validateTerrainPlacement( *terrain, placement, error );
        }
        Vector3<real_Num> toWorld( const Transform3<real_Num> &placement,
                                   const Vector3<real_Num> &local )
        {
            return placement.getPosition() +
                   placement.getOrientation() * ( local * placement.getScale() );
        }
        Vector3F toFloat( const Vector3<real_Num> &point )
        {
            return { f32( point.x ), f32( point.y ), f32( point.z ) };
        }
        bool localCell( const render::TerrainData &data, const Vector2F &local, s32 &x, s32 &z )
        {
            const double gx = ( double( local.x ) - data.origin.x ) / data.spacing.x;
            const double gz = ( double( local.y ) - data.origin.y ) / data.spacing.y;
            const double tolerance =
                8 * std::numeric_limits<f32>::epsilon() *
                std::max( { 1.0, double( data.dimensions.x ), double( data.dimensions.y ),
                            std::abs( double( data.origin.x ) / data.spacing.x ),
                            std::abs( double( data.origin.y ) / data.spacing.y ) } );
            if( gx < -tolerance || gz < -tolerance || gx > data.dimensions.x - 1 + tolerance ||
                gz > data.dimensions.y - 1 + tolerance )
                return false;
            x = s32( std::clamp( std::floor( gx ), 0.0, double( data.dimensions.x - 2 ) ) );
            z = s32( std::clamp( std::floor( gz ), 0.0, double( data.dimensions.y - 2 ) ) );
            return true;
        }
        bool groundAtLocal( const render::TerrainData &data, const Transform3<real_Num> &placement,
                            const Vector2F &local, Vector3<real_Num> &world, Vector3F &normal )
        {
            s32 x, z;
            if( !localCell( data, local, x, z ) || render::terrainCellIsHole( data, x, z ) )
                return false;
            const double u =
                std::clamp( ( double( local.x ) - data.origin.x ) / data.spacing.x - x, 0.0, 1.0 );
            const double v =
                std::clamp( ( double( local.y ) - data.origin.y ) / data.spacing.y - z, 0.0, 1.0 );
            const auto a = size_t( z ) * data.dimensions.x + x;
            const double ha = double( data.heights[a] ) * data.heightScale;
            const double hb = double( data.heights[a + 1] ) * data.heightScale;
            const double hc = double( data.heights[a + data.dimensions.x] ) * data.heightScale;
            const double hd = double( data.heights[a + data.dimensions.x + 1] ) * data.heightScale;
            double height, dx, dz;
            if( u + v <= 1 )
            {
                height = ha + u * ( hb - ha ) + v * ( hc - ha );
                dx = ( hb - ha ) / data.spacing.x;
                dz = ( hc - ha ) / data.spacing.y;
            }
            else
            {
                height = hd + ( 1 - u ) * ( hc - hd ) + ( 1 - v ) * ( hb - hd );
                dx = ( hd - hc ) / data.spacing.x;
                dz = ( hd - hb ) / data.spacing.y;
            }
            world = toWorld( placement, { local.x, real_Num( height ), local.y } );
            const double nx = -dx / placement.getScale().x, ny = 1.0 / placement.getScale().y,
                         nz = -dz / placement.getScale().z;
            const double length = std::hypot( nx, ny, nz );
            const auto n = placement.getOrientation() * Vector3<real_Num>( real_Num( nx / length ),
                                                                           real_Num( ny / length ),
                                                                           real_Num( nz / length ) );
            normal = toFloat( n );
            if( !normal.isFinite() || !std::isfinite( length ) || length <= 0 )
                throw std::runtime_error( "Terrain ground normal exceeds finite bounds." );
            return true;
        }
        bool writeInputFile( const String &path, const std::string &bytes, String &error )
        {
            std::filesystem::path temporary;
            try
            {
                if( path.empty() || path.find( '\0' ) != String::npos ||
                    bytes.size() > 256ull * 1024 * 1024 )
                    throw std::runtime_error(
                        "Terrain input export requires a valid path and at most 256 MiB." );
                const auto destination = std::filesystem::u8path( path.c_str() );
                temporary = destination;
                temporary += ".tmp-" + std::string( StringUtil::getUUID().c_str() );
                std::ofstream file( temporary, std::ios::binary | std::ios::trunc );
                file.write( bytes.data(), std::streamsize( bytes.size() ) );
                file.flush();
                if( !file )
                    throw std::runtime_error( "Cannot write terrain integration input." );
                file.close();
                if( !file )
                    throw std::runtime_error( "Cannot close terrain integration input." );
#ifdef _WIN32
                if( !MoveFileExW( temporary.c_str(), destination.c_str(),
                                  MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH ) )
                    throw std::runtime_error( "Cannot atomically replace terrain integration input." );
#else
                std::filesystem::rename( temporary, destination );
#endif
                error.clear();
                return true;
            }
            catch( const std::exception &e )
            {
                error = e.what();
                if( !temporary.empty() )
                {
                    std::error_code ignored;
                    std::filesystem::remove( temporary, ignored );
                }
                return false;
            }
        }
    }  // namespace

    bool terrainIntegrationIsNotifying( const TerrainSystem *owner )
    {
        for( auto frame = notificationFrame; frame; frame = frame->previous )
            if( frame->owner == owner ) return true;
        return false;
    }

    void notifyTerrainIntegration( TerrainSystem *owner, render::TerrainSnapshot source,
        const Transform3<real_Num> &placement, u64 serial, bool detached ) noexcept
    {
        if( !owner || terrainIntegrationIsNotifying( owner ) ) return;
        NotificationFrame frame{ owner, notificationFrame };
        notificationFrame = &frame;
        try
        {
            auto message = make_ptr<TerrainIntegrationPublication>();
            message->source = std::move( source ); message->placement = placement; message->detached = detached; message->serial = serial;
            for( auto listener : owner->getObjectListeners() )
                if( listener )
                    try { listener->handleEvent( EventType::Scene, terrainPublished, {}, nullptr, message, nullptr ); }
                    catch( ... ) { /* A dependent listener cannot undo a completed terrain publication. */ }
        }
        catch( ... ) { /* Notification allocation failure must not unwind committed renderer/physics state. */ }
        notificationFrame = frame.previous;
    }

    class TerrainIntegrationBinding::Listener : public IEventListener
    {
    public:
        AtomicWeakPtr<TerrainIntegrationBinding> owner;
        Parameter handleEvent( EventType, hash_type value, const Array<Parameter> &,
            SmartPtr<ISharedObject>, SmartPtr<ISharedObject> object, SmartPtr<IEvent> ) override
        {
            if( value == terrainPublished )
                if( auto binding = owner.load().lock() )
                    if( auto message = dynamic_cast<TerrainIntegrationPublication *>( object.get() ) )
                        binding->published( this, message->source, message->placement, message->serial, message->detached );
            return Parameter();
        }
        WP_CLASS_REGISTER_DECL;
    };
    WP_CLASS_REGISTER_DERIVED( workphone::scene, TerrainIntegrationBinding::Listener, IEventListener );

    struct TerrainIntegrationBinding::State
    {
        mutable std::mutex mutex;
        WeakPtr<TerrainSystem> owner;
        SmartPtr<Listener> listener;
        TerrainIntegrationOptions options;
        render::TerrainSnapshot latest;
        Transform3<real_Num> placement;
        TerrainIntegrationSnapshot snapshot;
        String error;
        u64 serial = 0;
    };
    TerrainIntegrationBinding::TerrainIntegrationBinding() : m_state( std::make_unique<State>() ) {}
    TerrainIntegrationBinding::~TerrainIntegrationBinding() { unbind(); }

    bool TerrainIntegrationBinding::bind( SmartPtr<TerrainSystem> source,
        const TerrainIntegrationOptions &options, String &error )
    {
        try
        {
            if( !source ) { error = "Terrain integration binding requires a source owner."; return false; }
            if( !validateIntegrationOptions( options, error ) ) return false;
            render::TerrainSnapshot active;
            Transform3<real_Num> placement;
            u64 serial;
            bool detached;
            source->getTerrainIntegrationState( active, placement, serial, detached );
            if( detached ) { error = "Terrain integration source has unloaded."; return false; }
            TerrainIntegrationSnapshot candidate;
            if( !buildIntegration( active, placement, options, {}, true, candidate, error ) ) return false;
            auto candidateOptions = options;
            auto listener = make_ptr<Listener>();
            listener->owner = this;
            // Subscribe before installing this identity. Publications during this gap
            // are recovered by the atomic source-state reconciliation below.
            source->addObjectListener( listener );
            SmartPtr<TerrainSystem> previousOwner;
            SmartPtr<Listener> previousListener;
            {
                std::lock_guard<std::mutex> lock( m_state->mutex );
                previousOwner = m_state->owner.lock(); previousListener = m_state->listener;
                if( previousListener ) previousListener->owner = nullptr;
                m_state->owner = source;
                m_state->listener = listener;
                m_state->options = std::move( candidateOptions );
                m_state->latest = active;
                m_state->placement = placement;
                m_state->snapshot = std::move( candidate );
                m_state->serial = serial;
                m_state->error.clear();
            }
            if( previousOwner && previousListener ) previousOwner->removeObjectListener( previousListener );
            source->getTerrainIntegrationState( active, placement, serial, detached );
            published( listener.get(), active, placement, serial, detached );
            error.clear(); return true;
        }
        catch( const std::exception &e ) { error = e.what(); return false; }
    }

    bool TerrainIntegrationBinding::configure( const TerrainIntegrationOptions &options, String &error )
    {
        try
        {
            if( !validateIntegrationOptions( options, error ) ) return false;
            auto candidateOptions = options;
            std::lock_guard<std::mutex> lock( m_state->mutex );
            if( !m_state->listener ) { error = "Terrain integration binding is detached."; return false; }
            TerrainIntegrationSnapshot candidate;
            if( !buildIntegration( m_state->latest, m_state->placement, options, {}, true, candidate, error ) )
            { m_state->error = error; return false; }
            m_state->options = std::move( candidateOptions );
            m_state->snapshot = std::move( candidate );
            m_state->error.clear(); error.clear(); return true;
        }
        catch( const std::exception &e ) { error = e.what(); return false; }
    }

    void TerrainIntegrationBinding::unbind()
    {
        SmartPtr<TerrainSystem> source;
        SmartPtr<Listener> listener;
        {
            std::lock_guard<std::mutex> lock( m_state->mutex );
            source = m_state->owner.lock(); listener = m_state->listener;
            if( listener ) listener->owner = nullptr;
            m_state->owner = nullptr; m_state->listener = nullptr;
            m_state->latest.reset(); m_state->snapshot.reset(); m_state->error.clear();
        }
        if( source && listener ) source->removeObjectListener( listener );
    }

    void TerrainIntegrationBinding::published( Listener *expectedListener, render::TerrainSnapshot source,
        const Transform3<real_Num> &placement, u64 serial, bool detached )
    {
        std::unique_lock<std::mutex> lock( m_state->mutex );
        if( m_state->listener.get() != expectedListener || serial <= m_state->serial ) return;
        m_state->serial = serial;
        if( detached )
        {
            auto owner = m_state->owner.lock();
            auto listener = m_state->listener;
            listener->owner = nullptr;
            m_state->owner = nullptr; m_state->listener = nullptr;
            m_state->latest.reset(); m_state->snapshot.reset(); m_state->error.clear();
            lock.unlock();
            if( owner ) owner->removeObjectListener( listener );
            return;
        }
        m_state->latest = source; m_state->placement = placement;
        if( !source ) { m_state->snapshot.reset(); m_state->error.clear(); return; }
        TerrainIntegrationSnapshot candidate;
        if( buildIntegration( source, placement, m_state->options, m_state->snapshot, false, candidate, m_state->error ) )
            m_state->snapshot = std::move( candidate );
    }

    TerrainIntegrationSnapshot TerrainIntegrationBinding::getSnapshot() const
    { std::lock_guard<std::mutex> lock( m_state->mutex ); return m_state->snapshot; }
    String TerrainIntegrationBinding::getLastError() const
    { std::lock_guard<std::mutex> lock( m_state->mutex ); return m_state->error; }

    bool TerrainGroundPopulation::setSeeds( const Array<TerrainGroundSeed> &seeds, String &error )
    {
        try
        {
            if( seeds.size() > maximumInstances )
                throw std::runtime_error( "Terrain ground population exceeds 65536 instances." );
            std::set<u64> ids;
            for( const auto &seed : seeds )
                if( !ids.insert( seed.id ).second || !std::isfinite( seed.localPosition.x ) ||
                    !std::isfinite( seed.localPosition.y ) || !std::isfinite( seed.heightOffset ) ||
                    !std::isfinite( seed.yaw ) || !seed.scale.isFinite() || seed.scale.x <= 0 ||
                    seed.scale.y <= 0 || seed.scale.z <= 0 || seed.allowedLayers > 15 ||
                    !std::isfinite( seed.minimumLayerWeight ) || seed.minimumLayerWeight < 0 || seed.minimumLayerWeight > 1 )
                    throw std::runtime_error(
                        "Terrain ground seeds require unique IDs and finite positive-scale "
                        "transforms." );
            auto candidate = seeds;
            Array<TerrainGroundPlacement> placements( seeds.size() );
            m_seeds.swap( candidate );
            m_placements.swap( placements );
            m_terrain.reset();
            m_dirty = {};
            m_resampled = 0;
            error.clear();
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    bool TerrainGroundPopulation::setMasks( const TerrainGroundMasks &masks, String &error )
    {
        try
        {
            if( !masks.density.empty() || !masks.exclusions.empty() )
            {
                if( masks.dimensions.x < 2 || masks.dimensions.y < 2 ||
                    masks.dimensions.x > render::terrainMaximumDimension || masks.dimensions.y > render::terrainMaximumDimension ||
                    u64( masks.dimensions.x ) * masks.dimensions.y > render::terrainMaximumSamples ||
                    ( !masks.density.empty() && masks.density.size() != size_t( masks.dimensions.x ) * masks.dimensions.y ) ||
                    ( !masks.exclusions.empty() && masks.exclusions.size() != size_t( masks.dimensions.x - 1 ) * ( masks.dimensions.y - 1 ) ) )
                    throw std::runtime_error( "Terrain masks require bounded matching sample/cell grids." );
                for( auto density : masks.density )
                    if( !std::isfinite( density ) || density < 0 || density > 1 )
                        throw std::runtime_error( "Terrain density mask samples must be finite in [0,1]." );
                for( auto exclusion : masks.exclusions )
                    if( exclusion > 1 ) throw std::runtime_error( "Terrain exclusions must be zero or one." );
            }
            if( !m_masks || m_masks->dimensions != masks.dimensions || m_masks->density != masks.density || m_masks->exclusions != masks.exclusions )
                m_masks = std::make_shared<TerrainGroundMasks>( masks );
            error.clear(); return true;
        }
        catch( const std::exception &e ) { error = e.what(); return false; }
    }

    bool TerrainGroundPopulation::update( render::TerrainSnapshot terrain,
                                          const Transform3<real_Num> &placement, String &error )
    {
        if( !validateSource( terrain, placement, error ) )
            return false;
        try
        {
            if( m_masks && ( !m_masks->density.empty() || !m_masks->exclusions.empty() ) && m_masks->dimensions != terrain->dimensions )
                throw std::runtime_error( "Terrain mask dimensions do not match the published source." );
            if( terrain == m_terrain && placement == m_transform && m_masks == m_sampledMasks )
            {
                m_resampled = 0;
                m_dirty = {};
                error.clear();
                return true;
            }
            const auto &data = *terrain;
            const s32 width = data.dimensions.x - 1, depth = data.dimensions.y - 1;
            const bool full = !m_terrain || placement != m_transform ||
                              data.dimensions != m_terrain->dimensions ||
                              data.spacing != m_terrain->spacing || data.origin != m_terrain->origin ||
                              data.heightScale != m_terrain->heightScale;
            Array<u8> dirty( size_t( width ) * depth, full ? 1 : 0 );
            TerrainDirtyRect region;
            const auto mark = [&]( s32 x, s32 z, u8 reason = 1 ) {
                if( x >= 0 && z >= 0 && x < width && z < depth )
                    dirty[size_t( z ) * width + x] |= reason;
            };
            const bool weighted = std::any_of( m_seeds.begin(), m_seeds.end(), []( const auto &seed ) { return seed.minimumLayerWeight > 0; } );
            if( !full )
            {
                for( s32 z = 0; z < data.dimensions.y; ++z )
                    for( s32 x = 0; x < data.dimensions.x; ++x )
                    {
                        if( data.heights[size_t( z ) * data.dimensions.x + x] !=
                            m_terrain->heights[size_t( z ) * data.dimensions.x + x] ||
                            densityAt( m_masks, size_t( z ) * data.dimensions.x + x ) != densityAt( m_sampledMasks, size_t( z ) * data.dimensions.x + x ) )
                        {
                            mark( x - 1, z - 1 );
                            mark( x, z - 1 );
                            mark( x - 1, z );
                            mark( x, z );
                        }
                        if( weighted && weightsAt( data, size_t( z ) * data.dimensions.x + x ) != weightsAt( *m_terrain, size_t( z ) * data.dimensions.x + x ) )
                        {
                            mark( x-1, z-1, 2 ); mark( x, z-1, 2 );
                            mark( x-1, z, 2 ); mark( x, z, 2 );
                        }
                    }
                for( s32 z = 0; z < depth; ++z )
                    for( s32 x = 0; x < width; ++x )
                        if( render::terrainCellIsHole( data, x, z ) !=
                            render::terrainCellIsHole( *m_terrain, x, z ) ||
                            exclusionAt( m_masks, size_t( z ) * width + x ) != exclusionAt( m_sampledMasks, size_t( z ) * width + x ) )
                            mark( x, z );
            }
            for( s32 z = 0; z < depth; ++z )
                for( s32 x = 0; x < width; ++x )
                    if( dirty[size_t( z ) * width + x] )
                    {
                        if( region.maxX < 0 )
                            region = { x, z, x, z };
                        else
                        {
                            region.minX = std::min( region.minX, x );
                            region.minZ = std::min( region.minZ, z );
                            region.maxX = std::max( region.maxX, x );
                            region.maxZ = std::max( region.maxZ, z );
                        }
                    }
            auto candidates = m_placements;
            u32 count = 0;
            for( size_t i = 0; i < m_seeds.size(); ++i )
            {
                const auto &seed = m_seeds[i];
                s32 x, z;
                const bool inside = localCell( data, seed.localPosition, x, z );
                if( !full && ( !inside || ( !( dirty[size_t( z ) * width + x] & 1 ) &&
                    !( seed.minimumLayerWeight > 0 && ( dirty[size_t( z ) * width + x] & 2 ) ) ) ) )
                    continue;
                ++count;
                auto &output = candidates[i];
                output.id = seed.id;
                output.sampledRevision = data.revision;
                Vector3<real_Num> world;
                output.supported =
                    inside && acceptsSeed( data, seed, m_masks, x, z ) &&
                    groundAtLocal( data, placement, seed.localPosition, world, output.normal );
                if( output.supported )
                {
                    world.y += seed.heightOffset * placement.getScale().y;
                    const Quaternion<real_Num> rotation( real_Num( std::cos( seed.yaw / 2 ) ), 0,
                                                         real_Num( std::sin( seed.yaw / 2 ) ), 0 );
                    output.worldTransform = { world, placement.getOrientation() * rotation,
                                              placement.getScale() * Vector3<real_Num>( seed.scale.x,
                                                                                        seed.scale.y,
                                                                                        seed.scale.z ) };
                    if( !world.isFinite() || !toFloat( world ).isFinite() ||
                        !toFloat( output.worldTransform.getScale() ).isFinite() )
                        throw std::runtime_error(
                            "Terrain ground placement exceeds finite transform bounds." );
                }
            }
            m_placements.swap( candidates );
            m_terrain = terrain;
            m_transform = placement;
            m_sampledMasks = m_masks;
            m_dirty = region;
            m_resampled = count;
            error.clear();
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    const Array<TerrainGroundPlacement> &TerrainGroundPopulation::getPlacements() const
    {
        return m_placements;
    }
    render::TerrainSnapshot TerrainGroundPopulation::getTerrainSnapshot() const
    {
        return m_terrain;
    }
    u32 TerrainGroundPopulation::getLastResampledCount() const
    {
        return m_resampled;
    }
    TerrainDirtyRect TerrainGroundPopulation::getLastDirtyCells() const
    {
        return m_dirty;
    }

    bool buildTerrainNavigationInput( render::TerrainSnapshot terrain,
                                      const Transform3<real_Num> &placement,
                                      TerrainNavigationInput &output, String &error )
    {
        if( !validateSource( terrain, placement, error ) )
            return false;
        try
        {
            render::TerrainMeshData mesh;
            if( !render::buildTerrainMeshData( *terrain, mesh, error ) )
                return false;
            TerrainNavigationInput candidate;
            candidate.source = terrain;
            candidate.placement = placement;
            candidate.positions.swap( mesh.positions );
            candidate.indices.swap( mesh.indices );
            candidate.worldBounds.setNull();
            for( auto &point : candidate.positions )
            {
                point = toFloat( toWorld( placement, { point.x, point.y, point.z } ) );
                if( !point.isFinite() )
                    throw std::runtime_error( "Terrain navigation coordinates are not finite." );
            }
            // Unreferenced samples (including fully holed cells) do not enlarge navigation bounds.
            for( const auto index : candidate.indices )
                candidate.worldBounds.merge( candidate.positions[index] );
            output = std::move( candidate );
            error.clear();
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    bool buildTerrainShorelineInput( render::TerrainSnapshot terrain,
                                     const Transform3<real_Num> &placement, f32 waterLevel,
                                     TerrainShorelineInput &output, String &error, u32 maximumSegments )
    {
        if( !std::isfinite( waterLevel ) || maximumSegments > 1048576 )
        {
            error = "Shoreline requires a finite level and a segment budget of at most 1048576.";
            return false;
        }
        TerrainNavigationInput mesh;
        if( !buildTerrainNavigationInput( terrain, placement, mesh, error ) )
            return false;
        try
        {
            TerrainShorelineInput candidate;
            candidate.source = terrain;
            candidate.waterLevel = waterLevel;
            std::set<std::pair<u32, u32>> emittedEdges;
            const auto append = [&]( const Vector3F &a, const Vector3F &b ) {
                if( ( a - b ).lengthSquared() == 0 )
                    return;
                if( candidate.segments.size() >= maximumSegments )
                    throw std::runtime_error( "Terrain shoreline segment budget exceeded." );
                candidate.segments.push_back( { a, b } );
            };
            for( size_t triangle = 0; triangle < mesh.indices.size(); triangle += 3 )
            {
                Vector3F points[3];
                u32 ids[3];
                double height[3];
                for( u32 i = 0; i < 3; ++i )
                {
                    ids[i] = mesh.indices[triangle + i];
                    points[i] = mesh.positions[ids[i]];
                    height[i] = double( points[i].y ) - waterLevel;
                }
                if( height[0] == 0 && height[1] == 0 && height[2] == 0 )
                    continue;  // coplanar water surface has no contour
                Array<Vector3F> crossings;
                for( u32 edge = 0; edge < 3; ++edge )
                {
                    const u32 next = ( edge + 1 ) % 3;
                    if( height[edge] == 0 && height[next] == 0 )
                    {
                        const auto key = std::minmax( ids[edge], ids[next] );
                        if( emittedEdges.insert( key ).second )
                            append( points[edge], points[next] );
                        crossings.clear();
                        break;
                    }
                    if( height[edge] == 0 )
                        crossings.push_back( points[edge] );
                    else if( ( height[edge] < 0 && height[next] > 0 ) ||
                             ( height[edge] > 0 && height[next] < 0 ) )
                    {
                        const double t = height[edge] / ( height[edge] - height[next] );
                        crossings.emplace_back(
                            f32( points[edge].x + t * ( double( points[next].x ) - points[edge].x ) ),
                            waterLevel,
                            f32( points[edge].z + t * ( double( points[next].z ) - points[edge].z ) ) );
                    }
                }
                if( crossings.size() == 2 )
                    append( crossings[0], crossings[1] );
            }
            output = std::move( candidate );
            error.clear();
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    bool sampleTerrainPathInput( render::TerrainSnapshot terrain, const Transform3<real_Num> &placement,
                                 const Array<Vector3F> &points, TerrainPathInput &output, String &error )
    {
        if( !validateSource( terrain, placement, error ) )
            return false;
        try
        {
            if( points.size() > TerrainGroundPopulation::maximumInstances )
                throw std::runtime_error( "Terrain path input exceeds 65536 anchors." );
            TerrainPathInput candidate;
            candidate.source = terrain;
            candidate.points.reserve( points.size() );
            for( const auto &point : points )
            {
                if( !point.isFinite() )
                    throw std::runtime_error( "Terrain path anchors must be finite." );
                const auto local =
                    ( placement.getOrientation().inverse() *
                      ( Vector3<real_Num>( point.x, point.y, point.z ) - placement.getPosition() ) ) /
                    placement.getScale();
                TerrainPathPoint sampled;
                sampled.position = point;
                sampled.normal = { 0, 1, 0 };
                Vector3<real_Num> ground;
                f32 height = 0;
                sampled.supported = render::sampleTerrainHeight( *terrain, placement,
                                                                 { point.x, point.y, point.z }, height );
                if( sampled.supported )
                {
                    const Vector2F clamped(
                        f32( std::clamp(
                            double( local.x ), double( terrain->origin.x ),
                            double( terrain->origin.x ) +
                                ( terrain->dimensions.x - 1 ) * double( terrain->spacing.x ) ) ),
                        f32( std::clamp(
                            double( local.z ), double( terrain->origin.y ),
                            double( terrain->origin.y ) +
                                ( terrain->dimensions.y - 1 ) * double( terrain->spacing.y ) ) ) );
                    sampled.supported =
                        groundAtLocal( *terrain, placement, clamped, ground, sampled.normal );
                    sampled.position.y = height;
                }
                candidate.points.push_back( sampled );
            }
            output = std::move( candidate );
            error.clear();
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    bool exportTerrainNavigationFile( SmartPtr<TerrainSystem> terrain, const String &path,
                                      String &error )
    {
        if( !terrain )
        {
            error = "No terrain selected.";
            return false;
        }
        auto actor = terrain->getActor();
        TerrainNavigationInput input;
        if( !buildTerrainNavigationInput( terrain->getTerrainSnapshot(),
                                          actor ? actor->getWorldTransform() : Transform3<real_Num>(),
                                          input, error ) )
            return false;
        std::ostringstream out;
        out.imbue( std::locale::classic() );
        out << std::setprecision( std::numeric_limits<f32>::max_digits10 );
        out << "# Workphone terrain navigation input, revision " << input.source->revision << "\n";
        for( const auto &p : input.positions )
            out << "v " << p.x << ' ' << p.y << ' ' << p.z << '\n';
        for( size_t i = 0; i < input.indices.size(); i += 3 )
            out << "f " << input.indices[i] + 1 << ' ' << input.indices[i + 1] + 1 << ' '
                << input.indices[i + 2] + 1 << '\n';
        return writeInputFile( path, out.str(), error );
    }

    bool exportTerrainShorelineFile( SmartPtr<TerrainSystem> terrain, const String &path, f32 waterLevel,
                                     String &error )
    {
        if( !terrain )
        {
            error = "No terrain selected.";
            return false;
        }
        auto actor = terrain->getActor();
        TerrainShorelineInput input;
        if( !buildTerrainShorelineInput( terrain->getTerrainSnapshot(),
                                         actor ? actor->getWorldTransform() : Transform3<real_Num>(),
                                         waterLevel, input, error ) )
            return false;
        std::ostringstream out;
        out.imbue( std::locale::classic() );
        out << std::setprecision( std::numeric_limits<f32>::max_digits10 );
        out << "{\"format\":\"workphone.terrain.shoreline-input\",\"version\":1,\"revision\":\""
            << input.source->revision << "\",\"waterLevel\":" << input.waterLevel << ",\"segments\":[";
        for( size_t i = 0; i < input.segments.size(); ++i )
        {
            if( i )
                out << ',';
            const auto &s = input.segments[i];
            out << "[[" << s.start.x << ',' << s.start.y << ',' << s.start.z << "],[" << s.end.x << ','
                << s.end.y << ',' << s.end.z << "]]";
        }
        out << "]}";
        return writeInputFile( path, out.str(), error );
    }
}  // namespace workphone::scene
