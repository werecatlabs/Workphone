#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/Terrain/FoliageAuthoring.hpp>
#include <Workphone/Scene/Components/Terrain/TerrainIntegration.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <cJSON.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <set>
#include <stdexcept>
#include <limits>
#include <cstring>
#ifdef _WIN32
#    include <windows.h>
#endif

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, FoliageAuthoring, ISharedObject );

    namespace
    {
        using namespace render;
        constexpr double pi = 3.14159265358979323846;

        double random01( u64 id, u64 salt )
        {
            return double( foliageStableHash( id ^ salt ) >> 11 ) / 9007199254740992.0;
        }

        bool inside( Vector2F p, const Array<Vector2F> &polygon )
        {
            bool result = false;
            for( size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++ )
            {
                if( ( polygon[i].y > p.y ) != ( polygon[j].y > p.y ) &&
                    double( p.x ) < ( double( polygon[j].x ) - polygon[i].x ) *
                                            ( double( p.y ) - polygon[i].y ) /
                                            ( double( polygon[j].y ) - polygon[i].y ) +
                                        polygon[i].x )
                {
                    result = !result;
                }
            }
            return result;
        }

        bool excluded( Vector2F p, const Array<FoliageExclusion> &exclusions )
        {
            for( const auto &ex : exclusions )
            {
                if( ex.radius == 0 )
                {
                    if( inside( p, ex.points ) )
                    {
                        return true;
                    }
                    continue;
                }
                for( size_t i = 1; i < ex.points.size(); ++i )
                {
                    const auto a = ex.points[i - 1], b = ex.points[i];
                    const double dx = double( b.x ) - a.x, dz = double( b.y ) - a.y;
                    const double length = dx * dx + dz * dz;
                    const double t = length > 0 ? std::clamp( ( ( double( p.x ) - a.x ) * dx +
                                                                ( double( p.y ) - a.y ) * dz ) /
                                                                  length,
                                                              0.0, 1.0 )
                                                : 0;
                    if( std::hypot( double( p.x ) - a.x - t * dx, double( p.y ) - a.y - t * dz ) <=
                        ex.radius )
                    {
                        return true;
                    }
                }
            }
            return false;
        }

        void require( bool okay, const char *message )
        {
            if( !okay )
            {
                throw std::runtime_error( message );
            }
        }

        void validate( const FoliagePlacementRecipe &r, size_t species )
        {
            require(
                r.version == 1 && std::isfinite( r.pageSize ) && r.pageSize >= 1 && r.pageSize <= 4096 &&
                    std::isfinite( r.candidateSpacing ) && r.candidateSpacing >= .05f &&
                    r.candidateSpacing <= 256,
                "Foliage recipe requires v1, page size 1..4096 and candidate spacing .05..256 metres." );
            require( r.rules.size() <= foliageMaximumSpecies && r.exclusions.size() <= 128,
                     "Foliage rule/exclusion budget exceeded." );
            for( const auto &v : r.rules )
            {
                for( auto n :
                     { v.weight, v.density, v.minimumSpacing, v.minimumHeight, v.maximumHeight,
                       v.minimumSlope, v.maximumSlope, v.minimumScale, v.maximumScale, v.heightOffset,
                       v.minimumLayerWeight, v.minimumMoisture, v.maximumMoisture } )
                {
                    require( std::isfinite( n ), "Foliage rule values must be finite." );
                }
                require( v.species < species && v.weight > 0 && v.density >= 0 && v.density <= 10000 &&
                             v.minimumSpacing >= 0 && v.minimumSpacing <= 4 * r.candidateSpacing &&
                             v.minimumHeight <= v.maximumHeight && v.minimumSlope >= 0 &&
                             v.maximumSlope <= 90 && v.minimumSlope <= v.maximumSlope &&
                             v.minimumScale > 0 && v.maximumScale <= 100 &&
                             v.minimumScale <= v.maximumScale && v.allowedLayers <= 15 &&
                             v.minimumLayerWeight >= 0 && v.minimumLayerWeight <= 1 &&
                             v.minimumMoisture >= 0 && v.maximumMoisture <= 1 &&
                             v.minimumMoisture <= v.maximumMoisture,
                         "Invalid foliage species/range; minimum spacing may not exceed four candidate "
                         "spacings." );
            }
            for( const auto &ex : r.exclusions )
            {
                require( std::isfinite( ex.radius ) && ex.radius >= 0 && ex.radius <= 100000 &&
                             ex.points.size() >= ( ex.radius == 0 ? 3u : 2u ) && ex.points.size() <= 256,
                         "Foliage exclusions require a polygon or a finite polyline corridor (at most "
                         "256 points)." );
                for( auto point : ex.points )
                {
                    require( std::isfinite( point.x ) && std::isfinite( point.y ),
                             "Foliage exclusion coordinates must be finite." );
                }
            }
            const auto count = s64( r.maskDimensions.x ) * r.maskDimensions.y;
            require( ( r.densityMask.empty() && r.biomeMask.empty() &&
                       r.maskDimensions == Vector2I( 0, 0 ) ) ||
                         ( r.maskDimensions.x > 0 && r.maskDimensions.y > 0 && count <= 65536 &&
                           ( r.densityMask.empty() || r.densityMask.size() == count ) &&
                           ( r.biomeMask.empty() || r.biomeMask.size() == count ) ),
                     "Foliage mask dimensions do not match values (maximum 65536 samples)." );
            for( auto v : r.densityMask )
            {
                require( std::isfinite( v ) && v >= 0 && v <= 1,
                         "Foliage density mask must be in [0,1]." );
            }
            for( auto v : r.biomeMask )
            {
                require( v < r.rules.size(), "Foliage biome mask references a missing rule." );
            }
        }

        QuaternionF aligned( Vector3F normal, float yaw, bool align )
        {
            auto rotation = QuaternionF::eulerDegrees( 0, yaw, 0 );
            if( !align )
            {
                return rotation;
            }
            QuaternionF tilt( 1 + normal.y, normal.z, 0, -normal.x );
            tilt.normalise();
            return tilt * rotation;
        }

        struct Contents
        {
            FoliagePlacementRecipe recipe;
            FoliageSnapshot world;
            std::map<u64, f32> offsets;
            u64 manual = 1;
        };

        using Json = std::unique_ptr<cJSON, decltype( &cJSON_Delete )>;

        double number( cJSON *o, const char *name, double fallback )
        {
            auto p = cJSON_GetObjectItemCaseSensitive( o, name );
            if( !p )
            {
                return fallback;
            }
            require( cJSON_IsNumber( p ) && std::isfinite( p->valuedouble ) &&
                         std::abs( p->valuedouble ) <= std::numeric_limits<f32>::max(),
                     "Foliage JSON number is invalid." );
            return p->valuedouble;
        }

        u32 integer( cJSON *o, const char *name, u32 fallback )
        {
            const auto v = number( o, name, fallback );
            require( v >= 0 && v <= 4294967295.0 && std::floor( v ) == v,
                     "Foliage JSON integer is invalid." );
            return u32( v );
        }

        bool boolean( cJSON *o, const char *name, bool fallback )
        {
            auto p = cJSON_GetObjectItemCaseSensitive( o, name );
            if( !p )
            {
                return fallback;
            }
            require( cJSON_IsBool( p ), "Foliage JSON boolean is invalid." );
            return cJSON_IsTrue( p );
        }
    }  // namespace

    struct FoliageAuthoring::State
    {
        mutable std::recursive_mutex mutex;
        Contents current;
        std::unique_ptr<Contents> stroke;
        Array<Contents> undo, redo;
        u64 revision = 0;

        static u64 retainedBytes( const Contents &content )
        {
            u64 bytes = render::foliageSourceBytes( *content.world ) +
                        content.recipe.densityMask.capacity() * sizeof( f32 ) +
                        content.recipe.biomeMask.capacity() * sizeof( u32 ) +
                        content.offsets.size() * 64;
            for( const auto &ex : content.recipe.exclusions )
            {
                bytes += ex.points.capacity() * sizeof( Vector2F );
            }
            return bytes;
        }

        static void remember( Array<Contents> &history, const Contents &content )
        {
            history.push_back( content );
            u64 bytes = 0;
            for( const auto &entry : history )
            {
                bytes += retainedBytes( entry );
            }
            while( history.size() > 1 && ( history.size() > 8 || bytes > 64ull * 1024 * 1024 ) )
            {
                bytes -= retainedBytes( history.front() );
                history.erase( history.begin() );
            }
        }

        void publish( FoliageWorldData world, bool history = true )
        {
            require( render::foliageSourceBytes( world ) <= 56ull * 1024 * 1024,
                     "Foliage authoring world exceeds the 56 MiB bounded undo/source tier." );
            world.revision = ++revision;
            auto snapshot = std::make_shared<const FoliageWorldData>( std::move( world ) );
            if( history && !stroke )
            {
                remember( undo, current );
                redo.clear();
            }
            current.world = std::move( snapshot );
        }
    };

    FoliageAuthoring::FoliageAuthoring() : m_state( std::make_unique<State>() )
    {
        m_state->current.world = std::make_shared<const render::FoliageWorldData>();
    }

    FoliageAuthoring::~FoliageAuthoring() = default;

    render::FoliageSnapshot FoliageAuthoring::getSnapshot() const
    {
        std::lock_guard<std::recursive_mutex> guard( m_state->mutex );
        return m_state->current.world;
    }

    FoliagePlacementRecipe FoliageAuthoring::getRecipe() const
    {
        std::lock_guard<std::recursive_mutex> guard( m_state->mutex );
        return m_state->current.recipe;
    }

    bool FoliageAuthoring::configure( const FoliagePlacementRecipe &recipe, String &error )
    {
        std::lock_guard<std::recursive_mutex> guard( m_state->mutex );
        try
        {
            validate( recipe, getSnapshot()->species.size() );
            auto world = *getSnapshot();
            world.pageSize = recipe.pageSize;
            auto next = recipe;
            m_state->publish( std::move( world ) );
            m_state->current.recipe = std::move( next );
            error.clear();
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    bool FoliageAuthoring::setSpecies( Array<std::shared_ptr<const render::FoliageSpecies>> species,
                                       String &error )
    {
        std::lock_guard<std::recursive_mutex> guard( m_state->mutex );
        try
        {
            auto next = *getSnapshot();
            next.species = std::move( species );
            validate( m_state->current.recipe, next.species.size() );
            if( !render::validateFoliageWorld( next, error ) )
            {
                return false;
            }
            m_state->publish( std::move( next ) );
            error.clear();
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    bool FoliageAuthoring::regenerateAll( render::TerrainSnapshot terrain, String &error,
                                          const std::atomic<bool> *cancel )
    {
        std::lock_guard<std::recursive_mutex> guard( m_state->mutex );
        if( !terrain )
        {
            error = "Foliage generation requires authoritative terrain.";
            return false;
        }
        render::FoliageCellKey low, high;
        const auto end =
            terrain->origin + Vector2F( ( terrain->dimensions.x - 1 ) * terrain->spacing.x,
                                        ( terrain->dimensions.y - 1 ) * terrain->spacing.y );
        if( !render::foliageCellForPosition( { terrain->origin.x, 0, terrain->origin.y },
                                             m_state->current.recipe.pageSize, low, error ) ||
            !render::foliageCellForPosition( { end.x, 0, end.y }, m_state->current.recipe.pageSize, high,
                                             error ) )
        {
            return false;
        }
        if( ( s64( high.x ) - low.x + 1 ) * ( s64( high.z ) - low.z + 1 ) > render::foliageMaximumPages )
        {
            error = "Foliage generation exceeds the 2048-cell authoring budget.";
            return false;
        }
        Array<render::FoliageCellKey> cells;
        for( s64 z = low.z; z <= high.z; ++z )
        {
            for( s64 x = low.x; x <= high.x; ++x )
            {
                cells.push_back( { s32( x ), s32( z ) } );
            }
        }
        return regenerate( terrain, cells, error, cancel );
    }

    bool FoliageAuthoring::regenerate( render::TerrainSnapshot terrain,
                                       const Array<render::FoliageCellKey> &cells, String &error,
                                       const std::atomic<bool> *cancel )
    {
        std::lock_guard<std::recursive_mutex> guard( m_state->mutex );
        try
        {
            require( terrain && render::validateTerrainData( *terrain, error ),
                     "Foliage requires valid authoritative terrain." );
            const auto &recipe = m_state->current.recipe;
            validate( recipe, getSnapshot()->species.size() );
            require( cells.size() <= render::foliageMaximumPages,
                     "Foliage regeneration exceeds 2048 cells." );
            std::set<render::FoliageCellKey> requested( cells.begin(), cells.end() );

            struct Candidate
            {
                render::FoliageInstance instance;
                u32 rule = 0;
                bool eligible = false;
            };

            std::map<std::pair<s32, s32>, Candidate> candidates;
            const double spacing = recipe.candidateSpacing;
            double weight = 0;
            for( const auto &rule : recipe.rules )
            {
                if( rule.enabled )
                {
                    weight += rule.weight;
                }
            }
            const auto check = [&] {
                require( !cancel || !cancel->load(),
                         "Foliage generation cancelled; previous population retained." );
            };
            for( auto cell : requested )
            {
                check();
                const double lx = std::floor( double( cell.x ) * recipe.pageSize / spacing ) - 5;
                const double lz = std::floor( double( cell.z ) * recipe.pageSize / spacing ) - 5;
                const double hx = std::ceil( ( double( cell.x ) + 1 ) * recipe.pageSize / spacing ) + 5;
                const double hz = std::ceil( ( double( cell.z ) + 1 ) * recipe.pageSize / spacing ) + 5;
                require( lx >= -2147483640.0 && lz >= -2147483640.0 && hx <= 2147483640.0 &&
                             hz <= 2147483640.0 && ( hx - lx ) * ( hz - lz ) <= maximumCandidates,
                         "Foliage candidate grid exceeds finite coordinates/work budget; increase "
                         "candidate spacing." );
                for( s64 z = s64( lz ); z < s64( hz ); ++z )
                {
                    for( s64 x = s64( lx ); x < s64( hx ); ++x )
                    {
                        auto key = std::make_pair( s32( x ), s32( z ) );
                        if( candidates.count( key ) )
                        {
                            continue;
                        }
                        require( candidates.size() < maximumCandidates,
                                 "Foliage candidate budget exceeded (262144 including halo)." );
                        Candidate value;
                        const auto id = render::foliageStableHash(
                                            render::foliageCellIdentity( { key.first, key.second } ) ^
                                            ( u64( recipe.seed ) << 32 ) ^ recipe.version ) &
                                        0x7fffffffffffffffULL;
                        value.instance.id = id ? id : 1;
                        value.instance.seed = u32( render::foliageStableHash( id ) );
                        value.instance.position = { f32( ( x + random01( id, 1 ) ) * spacing ), 0,
                                                    f32( ( z + random01( id, 2 ) ) * spacing ) };
                        require( f32( double( value.instance.position.x ) + spacing ) !=
                                         value.instance.position.x &&
                                     f32( double( value.instance.position.z ) + spacing ) !=
                                         value.instance.position.z,
                                 "Foliage candidate spacing is below float coordinate precision at this "
                                 "location." );
                        double choice = random01( id, 3 ) * weight;
                        for( u32 i = 0; i < recipe.rules.size(); ++i )
                        {
                            if( recipe.rules[i].enabled )
                            {
                                value.rule = i;
                                choice -= recipe.rules[i].weight;
                                if( choice < 0 )
                                {
                                    break;
                                }
                            }
                        }
                        if( weight > 0 )
                        {
                            double mask = 1;
                            const auto &p = value.instance.position;
                            const double u =
                                ( double( p.x ) - terrain->origin.x ) /
                                ( ( terrain->dimensions.x - 1 ) * double( terrain->spacing.x ) );
                            const double v =
                                ( double( p.z ) - terrain->origin.y ) /
                                ( ( terrain->dimensions.y - 1 ) * double( terrain->spacing.y ) );
                            if( recipe.maskDimensions.x && u >= 0 && u <= 1 && v >= 0 && v <= 1 )
                            {
                                const auto index = std::min( recipe.maskDimensions.y - 1,
                                                             s32( v * recipe.maskDimensions.y ) ) *
                                                       recipe.maskDimensions.x +
                                                   std::min( recipe.maskDimensions.x - 1,
                                                             s32( u * recipe.maskDimensions.x ) );
                                if( !recipe.densityMask.empty() )
                                {
                                    mask = recipe.densityMask[index];
                                }
                                if( !recipe.biomeMask.empty() )
                                {
                                    value.rule = recipe.biomeMask[index];
                                }
                            }
                            const auto &rule = recipe.rules[value.rule];
                            value.instance.species = rule.species;
                            const float scale =
                                f32( rule.minimumScale +
                                     random01( id, 5 ) * ( rule.maximumScale - rule.minimumScale ) );
                            value.instance.scale = { scale, scale, scale };
                            value.eligible = rule.enabled &&
                                             random01( id, 4 ) < std::min( 1.0, rule.density * spacing *
                                                                                    spacing * mask ) &&
                                             !excluded( { p.x, p.z }, recipe.exclusions );
                        }
                        candidates.emplace( key, std::move( value ) );
                    }
                }
            }
            // Ground all candidates in bounded batches through the terrain integration contract.
            Array<Candidate *> batch;
            const auto sample = [&] {
                if( batch.empty() )
                {
                    return;
                }
                Array<TerrainGroundSeed> seeds;
                for( auto c : batch )
                {
                    const auto &r = recipe.rules[c->rule];
                    TerrainGroundSeed s;
                    s.id = c->instance.id;
                    s.localPosition = { c->instance.position.x, c->instance.position.z };
                    s.heightOffset = r.heightOffset;
                    s.allowedLayers = r.allowedLayers;
                    s.minimumLayerWeight = r.minimumLayerWeight;
                    seeds.push_back( s );
                }
                TerrainGroundPopulation ground;
                require( ground.setSeeds( seeds, error ) &&
                             ground.update( terrain, Transform3<real_Num>(), error ),
                         "Foliage ground sampling failed." );
                for( size_t i = 0; i < batch.size(); ++i )
                {
                    auto &c = *batch[i];
                    const auto &p = ground.getPlacements()[i];
                    const auto &r = recipe.rules[c.rule];
                    if( !p.supported )
                    {
                        c.eligible = false;
                        continue;
                    }
                    const auto position = p.worldTransform.getPosition();
                    c.instance.position.y = f32( position.y );
                    const auto slope =
                        std::acos( std::clamp( double( p.normal.y ), -1.0, 1.0 ) ) * 180 / pi;
                    auto x = std::clamp(
                        s32( ( c.instance.position.x - terrain->origin.x ) / terrain->spacing.x ), 0,
                        terrain->dimensions.x - 1 );
                    auto z = std::clamp(
                        s32( ( c.instance.position.z - terrain->origin.y ) / terrain->spacing.y ), 0,
                        terrain->dimensions.y - 1 );
                    const float moisture =
                        terrain->surfaceMasks.empty()
                            ? 1.f
                            : terrain->surfaceMasks[size_t( z ) * terrain->dimensions.x + x].x;
                    c.eligible = position.y >= r.minimumHeight && position.y <= r.maximumHeight &&
                                 slope >= r.minimumSlope && slope <= r.maximumSlope &&
                                 moisture >= r.minimumMoisture && moisture <= r.maximumMoisture;
                    c.instance.rotation =
                        aligned( p.normal, r.randomYaw ? f32( random01( c.instance.id, 6 ) * 360 ) : 0,
                                 r.alignToNormal );
                }
                batch.clear();
            };
            for( auto &entry : candidates )
            {
                check();
                if( entry.second.eligible )
                {
                    batch.push_back( &entry.second );
                }
                if( batch.size() == 65536 )
                {
                    sample();
                }
            }
            sample();
            auto world = *getSnapshot();
            world.instances.clear();
            std::map<u64, render::FoliageInstance> overrides;
            for( const auto &instance : getSnapshot()->instances )
            {
                render::FoliageCellKey cell;
                if( !render::foliageCellForPosition( instance.position, recipe.pageSize, cell, error ) )
                {
                    throw std::runtime_error( error.c_str() );
                }
                if( instance.flags )
                {
                    overrides.emplace( instance.id, instance );
                }
                else if( !requested.count( cell ) )
                {
                    world.instances.push_back( instance );
                }
            }
            for( const auto &entry : candidates )
            {
                check();
                const auto &c = entry.second;
                if( !c.eligible || overrides.count( c.instance.id ) )
                {
                    continue;
                }
                render::FoliageCellKey cell;
                if( !render::foliageCellForPosition( c.instance.position, recipe.pageSize, cell,
                                                     error ) )
                {
                    throw std::runtime_error( error.c_str() );
                }
                if( !requested.count( cell ) )
                {
                    continue;
                }
                bool keep = true;
                for( s32 z = -5; z <= 5 && keep; ++z )
                {
                    for( s32 x = -5; x <= 5 && keep; ++x )
                    {
                        auto neighbour =
                            candidates.find( { entry.first.first + x, entry.first.second + z } );
                        if( neighbour == candidates.end() || !neighbour->second.eligible ||
                            neighbour->second.instance.id >= c.instance.id )
                        {
                            continue;
                        }
                        const auto &other = neighbour->second;
                        const auto distance = std::max( recipe.rules[c.rule].minimumSpacing,
                                                        recipe.rules[other.rule].minimumSpacing );
                        if( std::hypot( double( c.instance.position.x ) - other.instance.position.x,
                                        double( c.instance.position.z ) - other.instance.position.z ) <
                            distance )
                        {
                            keep = false;
                        }
                    }
                }
                if( keep )
                {
                    world.instances.push_back( c.instance );
                }
                require( world.instances.size() + overrides.size() <= maximumAuthoredInstances,
                         "Foliage population exceeds 65536 authored instances." );
            }
            for( const auto &entry : overrides )
            {
                world.instances.push_back( entry.second );
            }
            std::sort( world.instances.begin(), world.instances.end(),
                       []( const auto &a, const auto &b ) { return a.id < b.id; } );
            if( !render::validateFoliageWorld( world, error ) )
            {
                return false;
            }
            check();
            m_state->publish( std::move( world ) );
            error.clear();
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    render::FoliageSnapshot FoliageAuthoring::getGroundedSnapshot( render::TerrainSnapshot terrain,
                                                                   String &error ) const
    {
        std::lock_guard<std::recursive_mutex> guard( m_state->mutex );
        try
        {
            if( !terrain )
            {
                error = "Foliage requires active terrain.";
                return {};
            }
            const auto source = m_state->current.world;
            auto world = *source;
            Array<TerrainGroundSeed> seeds;
            for( const auto &i : world.instances )
            {
                TerrainGroundSeed seed;
                seed.id = i.id;
                seed.localPosition = { i.position.x, i.position.z };
                for( const auto &rule : m_state->current.recipe.rules )
                {
                    if( rule.species == i.species )
                    {
                        seed.heightOffset = rule.heightOffset;
                        seed.allowedLayers = rule.allowedLayers;
                        seed.minimumLayerWeight = rule.minimumLayerWeight;
                        break;
                    }
                }
                seeds.push_back( seed );
            }
            TerrainGroundPopulation ground;
            if( !ground.setSeeds( seeds, error ) ||
                !ground.update( terrain, Transform3<real_Num>(), error ) )
            {
                return {};
            }
            world.instances.clear();
            for( size_t i = 0; i < seeds.size(); ++i )
            {
                if( ground.getPlacements()[i].supported )
                {
                    auto instance = source->instances[i];
                    bool configured = false, enabled = false;
                    for( const auto &rule : m_state->current.recipe.rules )
                    {
                        if( rule.species == instance.species )
                        {
                            configured = true;
                            enabled = enabled || rule.enabled;
                        }
                    }
                    if( configured && !enabled )
                    {
                        continue;
                    }
                    const auto &placement = ground.getPlacements()[i];
                    // Manual transforms retain their authored vertical offset; procedural placements
                    // follow the latest authoritative ground. Holes hide both without deleting them.
                    instance.position.y = f32( placement.worldTransform.getPosition().y );
                    const auto offset = m_state->current.offsets.find( instance.id );
                    if( offset != m_state->current.offsets.end() )
                    {
                        instance.position.y += offset->second;
                    }
                    world.instances.push_back( instance );
                }
            }
            if( !render::validateFoliageWorld( world, error ) )
            {
                return {};
            }
            error.clear();
            return std::make_shared<const render::FoliageWorldData>( std::move( world ) );
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return {};
        }
    }

    bool FoliageAuthoring::resample( render::TerrainSnapshot terrain, String &error )
    {
        std::lock_guard<std::recursive_mutex> guard( m_state->mutex );
        try
        {
            if( !terrain )
            {
                error = "Foliage resampling requires active terrain.";
                return false;
            }
            const auto source = m_state->current.world;
            auto world = *source;
            Array<TerrainGroundSeed> seeds;
            for( const auto &i : world.instances )
            {
                TerrainGroundSeed s;
                s.id = i.id;
                s.localPosition = { i.position.x, i.position.z };
                seeds.push_back( s );
            }
            TerrainGroundPopulation ground;
            if( !ground.setSeeds( seeds, error ) ||
                !ground.update( terrain, Transform3<real_Num>(), error ) )
            {
                return false;
            }
            bool changed = false;
            for( size_t i = 0; i < world.instances.size(); ++i )
            {
                if( ground.getPlacements()[i].supported )
                {
                    auto height = f32( ground.getPlacements()[i].worldTransform.getPosition().y );
                    for( const auto &rule : m_state->current.recipe.rules )
                    {
                        if( rule.species == world.instances[i].species )
                        {
                            height += rule.heightOffset;
                            break;
                        }
                    }
                    const auto offset = m_state->current.offsets.find( world.instances[i].id );
                    if( offset != m_state->current.offsets.end() )
                    {
                        height += offset->second;
                    }
                    changed = changed || world.instances[i].position.y != height;
                    world.instances[i].position.y = height;
                }
            }
            if( changed )
            {
                m_state->publish( std::move( world ), false );
            }
            error.clear();
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    bool FoliageAuthoring::beginStroke( String &error )
    {
        std::lock_guard<std::recursive_mutex> guard( m_state->mutex );
        if( m_state->stroke )
        {
            error = "A foliage stroke is already active.";
            return false;
        }
        m_state->stroke = std::make_unique<Contents>( m_state->current );
        error.clear();
        return true;
    }

    bool FoliageAuthoring::commitStroke( String &error )
    {
        std::lock_guard<std::recursive_mutex> guard( m_state->mutex );
        if( !m_state->stroke )
        {
            error = "No foliage stroke is active.";
            return false;
        }
        State::remember( m_state->undo, *m_state->stroke );
        m_state->redo.clear();
        m_state->stroke.reset();
        error.clear();
        return true;
    }

    bool FoliageAuthoring::cancelStroke( String &error )
    {
        std::lock_guard<std::recursive_mutex> guard( m_state->mutex );
        if( !m_state->stroke )
        {
            error.clear();
            return true;
        }
        m_state->current = *m_state->stroke;
        m_state->stroke.reset();
        auto world = *getSnapshot();
        m_state->publish( std::move( world ), false );
        error.clear();
        return true;
    }

    bool FoliageAuthoring::isStrokeActive() const
    {
        std::lock_guard<std::recursive_mutex> guard( m_state->mutex );
        return bool( m_state->stroke );
    }

    bool FoliageAuthoring::paint( render::TerrainSnapshot terrain, u32 species, Vector2F centre,
                                  f32 radius, u32 count, u32 seed, String &error )
    {
        std::lock_guard<std::recursive_mutex> guard( m_state->mutex );
        try
        {
            require(
                species < getSnapshot()->species.size() && std::isfinite( centre.x ) &&
                    std::isfinite( centre.y ) && std::isfinite( radius ) && radius > 0 &&
                    radius <= 100000 && count <= 4096,
                "Foliage paint requires a valid species, finite brush and at most 4096 candidates." );
            auto world = *getSnapshot();
            auto serial = m_state->current.manual;
            require( world.instances.size() + count <= maximumAuthoredInstances,
                     "Foliage paint exceeds the 65536 instance budget." );
            Array<TerrainGroundSeed> seeds;
            for( u32 i = 0; i < count; ++i )
            {
                TerrainGroundSeed s;
                s.id = 0x8000000000000000ULL | serial++;
                const auto h = render::foliageStableHash( s.id ^ seed );
                const auto a = random01( h, 1 ) * 2 * pi, r = std::sqrt( random01( h, 2 ) ) * radius;
                s.localPosition = { f32( centre.x + r * std::cos( a ) ),
                                    f32( centre.y + r * std::sin( a ) ) };
                seeds.push_back( s );
            }
            TerrainGroundPopulation ground;
            if( !ground.setSeeds( seeds, error ) ||
                !ground.update( terrain, Transform3<real_Num>(), error ) )
            {
                return false;
            }
            for( size_t i = 0; i < seeds.size(); ++i )
            {
                if( ground.getPlacements()[i].supported )
                {
                    render::FoliageInstance instance;
                    instance.id = seeds[i].id;
                    instance.species = species;
                    instance.flags = render::FoliageManual | render::FoliageLocked;
                    const auto p = ground.getPlacements()[i].worldTransform.getPosition();
                    instance.position = { f32( p.x ), f32( p.y ), f32( p.z ) };
                    instance.rotation = QuaternionF::eulerDegrees(
                        0, f32( random01( instance.id ^ seed, 9 ) * 360 ), 0 );
                    instance.seed = seed;
                    world.instances.push_back( instance );
                }
            }
            if( !render::validateFoliageWorld( world, error ) )
            {
                return false;
            }
            m_state->publish( std::move( world ) );
            m_state->current.manual = serial;
            error.clear();
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    bool FoliageAuthoring::erase( Vector2F centre, f32 radius, String &error )
    {
        std::lock_guard<std::recursive_mutex> guard( m_state->mutex );
        if( !std::isfinite( centre.x ) || !std::isfinite( centre.y ) || !std::isfinite( radius ) ||
            radius <= 0 )
        {
            error = "Foliage erase requires a finite positive brush.";
            return false;
        }
        Array<u64> ids;
        for( const auto &i : getSnapshot()->instances )
        {
            if( std::hypot( double( i.position.x ) - centre.x, double( i.position.z ) - centre.y ) <=
                radius )
            {
                ids.push_back( i.id );
            }
        }
        return setInstanceFlags( ids, render::FoliageRemoved, true, error );
    }

    Array<u64> FoliageAuthoring::select( const Array<Vector2F> &polygon ) const
    {
        std::lock_guard<std::recursive_mutex> guard( m_state->mutex );
        Array<u64> ids;
        if( polygon.size() < 3 || polygon.size() > 256 )
        {
            return ids;
        }
        for( auto p : polygon )
        {
            if( !std::isfinite( p.x ) || !std::isfinite( p.y ) )
            {
                return ids;
            }
        }
        for( const auto &i : getSnapshot()->instances )
        {
            if( !( i.flags & render::FoliageRemoved ) &&
                inside( { i.position.x, i.position.z }, polygon ) )
            {
                ids.push_back( i.id );
            }
        }
        return ids;
    }

    bool FoliageAuthoring::transformSelection( const Array<u64> &ids, Vector3F translation, f32 yaw,
                                               f32 scale, String &error )
    {
        std::lock_guard<std::recursive_mutex> guard( m_state->mutex );
        try
        {
            require(
                translation.isFinite() && std::isfinite( yaw ) && std::isfinite( scale ) && scale > 0,
                "Foliage transform requires finite translation/yaw and positive scale." );
            std::set<u64> selected( ids.begin(), ids.end() );
            auto world = *getSnapshot();
            auto offsets = m_state->current.offsets;
            for( auto &i : world.instances )
            {
                if( selected.count( i.id ) )
                {
                    offsets[i.id] += translation.y;
                    i.position += translation;
                    i.rotation = QuaternionF::eulerDegrees( 0, yaw, 0 ) * i.rotation;
                    i.scale *= scale;
                    i.flags |= render::FoliageManual | render::FoliageLocked;
                }
            }
            if( !render::validateFoliageWorld( world, error ) )
            {
                return false;
            }
            m_state->publish( std::move( world ) );
            m_state->current.offsets = std::move( offsets );
            error.clear();
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    bool FoliageAuthoring::replaceSpecies( const Array<u64> &ids, u32 species, String &error )
    {
        std::lock_guard<std::recursive_mutex> guard( m_state->mutex );
        if( species >= getSnapshot()->species.size() )
        {
            error = "Foliage replacement species is unavailable.";
            return false;
        }
        std::set<u64> selected( ids.begin(), ids.end() );
        auto world = *getSnapshot();
        for( auto &i : world.instances )
        {
            if( selected.count( i.id ) )
            {
                i.species = species;
                i.flags |= render::FoliageManual | render::FoliageLocked;
            }
        }
        m_state->publish( std::move( world ) );
        error.clear();
        return true;
    }

    bool FoliageAuthoring::setInstanceFlags( const Array<u64> &ids, u32 mask, bool enabled,
                                             String &error )
    {
        std::lock_guard<std::recursive_mutex> guard( m_state->mutex );
        if( mask & ~( render::FoliageManual | render::FoliageLocked | render::FoliageRemoved |
                      render::FoliagePromoted ) )
        {
            error = "Unknown foliage instance flag.";
            return false;
        }
        std::set<u64> selected( ids.begin(), ids.end() );
        auto world = *getSnapshot();
        for( auto &i : world.instances )
        {
            if( selected.count( i.id ) )
            {
                i.flags = enabled ? i.flags | mask : i.flags & ~mask;
            }
        }
        m_state->publish( std::move( world ) );
        error.clear();
        return true;
    }

    bool FoliageAuthoring::undo( String &error )
    {
        std::lock_guard<std::recursive_mutex> guard( m_state->mutex );
        if( m_state->stroke || m_state->undo.empty() )
        {
            error = "Foliage undo requires committed history.";
            return false;
        }
        State::remember( m_state->redo, m_state->current );
        m_state->current = m_state->undo.back();
        m_state->undo.pop_back();
        auto world = *getSnapshot();
        m_state->publish( std::move( world ), false );
        error.clear();
        return true;
    }

    bool FoliageAuthoring::redo( String &error )
    {
        std::lock_guard<std::recursive_mutex> guard( m_state->mutex );
        if( m_state->stroke || m_state->redo.empty() )
        {
            error = "Foliage redo requires committed history.";
            return false;
        }
        State::remember( m_state->undo, m_state->current );
        m_state->current = m_state->redo.back();
        m_state->redo.pop_back();
        auto world = *getSnapshot();
        m_state->publish( std::move( world ), false );
        error.clear();
        return true;
    }

    bool FoliageAuthoring::serialize( String &output, String &error ) const
    {
        std::lock_guard<std::recursive_mutex> guard( m_state->mutex );
        try
        {
            String world;
            if( !render::serializeFoliageWorld( *getSnapshot(), world, error ) )
            {
                return false;
            }
            render::detail::FoliageJsonEstimate budget;
            budget.nodes = render::detail::checkFoliageJsonBudget( { world.data(), world.size() } );
            budget.wireBytes = world.size();
            const auto &metadata = m_state->current.recipe;
            budget.add( 64 + m_state->current.offsets.size() * 8 + metadata.rules.size() * 48 +
                        metadata.densityMask.size() + metadata.biomeMask.size() );
            for( const auto &ex : metadata.exclusions )
            {
                budget.add( 8 + ex.points.size() * 3 );
            }
            Json root( cJSON_CreateObject(), cJSON_Delete );
            require( bool( root ), "Cannot allocate foliage JSON." );
            cJSON_AddStringToObject( root.get(), "format", "workphone.foliage-authoring" );
            cJSON_AddNumberToObject( root.get(), "version", 1 );
            cJSON_AddStringToObject( root.get(), "manualSerial",
                                     std::to_string( m_state->current.manual ).c_str() );
            cJSON_AddItemToObject( root.get(), "world", cJSON_Parse( world.c_str() ) );
            auto offsets = cJSON_AddArrayToObject( root.get(), "offsets" );
            for( const auto &offset : m_state->current.offsets )
            {
                auto o = cJSON_CreateObject();
                cJSON_AddStringToObject( o, "id", std::to_string( offset.first ).c_str() );
                cJSON_AddNumberToObject( o, "height", offset.second );
                cJSON_AddItemToArray( offsets, o );
            }
            const auto &r = m_state->current.recipe;
            auto recipe = cJSON_AddObjectToObject( root.get(), "recipe" );
            cJSON_AddNumberToObject( recipe, "version", r.version );
            cJSON_AddNumberToObject( recipe, "seed", r.seed );
            cJSON_AddNumberToObject( recipe, "pageSize", r.pageSize );
            cJSON_AddNumberToObject( recipe, "candidateSpacing", r.candidateSpacing );
            auto rules = cJSON_AddArrayToObject( recipe, "rules" );
            for( const auto &v : r.rules )
            {
                auto o = cJSON_CreateObject();
                cJSON_AddItemToArray( rules, o );
#define FIELD( name ) cJSON_AddNumberToObject( o, #name, v.name )
                FIELD( species );
                FIELD( weight );
                FIELD( density );
                FIELD( minimumSpacing );
                FIELD( minimumHeight );
                FIELD( maximumHeight );
                FIELD( minimumSlope );
                FIELD( maximumSlope );
                FIELD( minimumScale );
                FIELD( maximumScale );
                FIELD( heightOffset );
                FIELD( allowedLayers );
                FIELD( minimumLayerWeight );
                FIELD( minimumMoisture );
                FIELD( maximumMoisture );
#undef FIELD
                cJSON_AddBoolToObject( o, "enabled", v.enabled );
                cJSON_AddBoolToObject( o, "randomYaw", v.randomYaw );
                cJSON_AddBoolToObject( o, "alignToNormal", v.alignToNormal );
            }
            auto exclusions = cJSON_AddArrayToObject( recipe, "exclusions" );
            for( const auto &ex : r.exclusions )
            {
                auto o = cJSON_CreateObject();
                cJSON_AddItemToArray( exclusions, o );
                cJSON_AddNumberToObject( o, "radius", ex.radius );
                auto points = cJSON_AddArrayToObject( o, "points" );
                for( auto p : ex.points )
                {
                    double values[] = { p.x, p.y };
                    cJSON_AddItemToArray( points, cJSON_CreateDoubleArray( values, 2 ) );
                }
            }
            cJSON_AddNumberToObject( recipe, "maskWidth", r.maskDimensions.x );
            cJSON_AddNumberToObject( recipe, "maskHeight", r.maskDimensions.y );
            auto density = cJSON_AddArrayToObject( recipe, "densityMask" );
            for( auto value : r.densityMask )
            {
                cJSON_AddItemToArray( density, cJSON_CreateNumber( value ) );
            }
            auto biome = cJSON_AddArrayToObject( recipe, "biomeMask" );
            for( auto value : r.biomeMask )
            {
                cJSON_AddItemToArray( biome, cJSON_CreateNumber( value ) );
            }
            std::unique_ptr<char, decltype( &cJSON_free )> text( cJSON_PrintUnformatted( root.get() ),
                                                                 cJSON_free );
            require( bool( text ) && std::strlen( text.get() ) <= render::foliageMaximumSourceBytes,
                     "Foliage source exceeds 128 MiB." );
            render::detail::checkFoliageJsonBudget( text.get() );
            output = text.get();
            error.clear();
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    bool FoliageAuthoring::deserialize( const String &json, String &error )
    {
        std::lock_guard<std::recursive_mutex> guard( m_state->mutex );
        try
        {
            require( !isStrokeActive(), "Commit or cancel the foliage stroke before loading." );
            require(
                json.size() <= render::foliageMaximumSourceBytes && json.find( '\0' ) == String::npos,
                "Foliage source is too large or contains embedded nulls." );
            render::detail::checkFoliageJsonBudget( { json.data(), json.size() } );
            Json root( cJSON_ParseWithOpts( json.c_str(), nullptr, true ), cJSON_Delete );
            require( bool( root ), "Invalid foliage JSON." );
            auto format = cJSON_GetObjectItemCaseSensitive( root.get(), "format" );
            require( cJSON_IsString( format ) &&
                         std::string( format->valuestring ) == "workphone.foliage-authoring" &&
                         integer( root.get(), "version", 0 ) == 1,
                     "Unsupported foliage authoring format/version." );
            auto worldJson = cJSON_GetObjectItemCaseSensitive( root.get(), "world" );
            require( cJSON_IsObject( worldJson ), "Foliage world is missing." );
            std::unique_ptr<char, decltype( &cJSON_free )> worldText(
                cJSON_PrintUnformatted( worldJson ), cJSON_free );
            // Retire the nested DOM before the world codec parses its printed copy.
            // Keeping both full trees alive would defeat the shared scratch budget.
            cJSON_DeleteItemFromObjectCaseSensitive( root.get(), "world" );
            render::FoliageWorldData world;
            if( !worldText || !render::deserializeFoliageWorld( worldText.get(), world, error ) )
            {
                return false;
            }
            require( world.instances.size() <= maximumAuthoredInstances,
                     "Authoring source exceeds 65536 instances." );
            FoliagePlacementRecipe r;
            auto recipe = cJSON_GetObjectItemCaseSensitive( root.get(), "recipe" );
            require( cJSON_IsObject( recipe ), "Foliage recipe is missing." );
            r.version = integer( recipe, "version", 0 );
            r.seed = integer( recipe, "seed", 1 );
            r.pageSize = f32( number( recipe, "pageSize", 64 ) );
            r.candidateSpacing = f32( number( recipe, "candidateSpacing", 1 ) );
            auto rules = cJSON_GetObjectItemCaseSensitive( recipe, "rules" );
            require( cJSON_IsArray( rules ) && cJSON_GetArraySize( rules ) <= 64,
                     "Invalid foliage rules." );
            for( auto o = rules->child; o; o = o->next )
            {
                require( cJSON_IsObject( o ), "Invalid foliage rule." );
                FoliagePlacementRule v;
#define FIELD( name ) v.name = f32( number( o, #name, v.name ) )
                FIELD( weight );
                FIELD( density );
                FIELD( minimumSpacing );
                FIELD( minimumHeight );
                FIELD( maximumHeight );
                FIELD( minimumSlope );
                FIELD( maximumSlope );
                FIELD( minimumScale );
                FIELD( maximumScale );
                FIELD( heightOffset );
                FIELD( minimumLayerWeight );
                FIELD( minimumMoisture );
                FIELD( maximumMoisture );
#undef FIELD
                v.species = integer( o, "species", 0 );
                v.allowedLayers = integer( o, "allowedLayers", 15 );
                v.enabled = boolean( o, "enabled", true );
                v.randomYaw = boolean( o, "randomYaw", true );
                v.alignToNormal = boolean( o, "alignToNormal", false );
                r.rules.push_back( v );
            }
            auto exclusions = cJSON_GetObjectItemCaseSensitive( recipe, "exclusions" );
            require( cJSON_IsArray( exclusions ) && cJSON_GetArraySize( exclusions ) <= 128,
                     "Invalid foliage exclusions." );
            for( auto o = exclusions->child; o; o = o->next )
            {
                FoliageExclusion ex;
                ex.radius = f32( number( o, "radius", 0 ) );
                auto points = cJSON_GetObjectItemCaseSensitive( o, "points" );
                require( cJSON_IsArray( points ) && cJSON_GetArraySize( points ) <= 256,
                         "Invalid foliage exclusion points." );
                for( auto p = points->child; p; p = p->next )
                {
                    require( cJSON_IsArray( p ) && cJSON_GetArraySize( p ) == 2,
                             "Invalid foliage exclusion coordinate." );
                    auto x = p->child, z = x->next;
                    require( cJSON_IsNumber( x ) && cJSON_IsNumber( z ) &&
                                 std::isfinite( x->valuedouble ) && std::isfinite( z->valuedouble ) &&
                                 std::abs( x->valuedouble ) <= std::numeric_limits<f32>::max() &&
                                 std::abs( z->valuedouble ) <= std::numeric_limits<f32>::max(),
                             "Invalid foliage exclusion coordinate." );
                    ex.points.push_back( { f32( x->valuedouble ), f32( z->valuedouble ) } );
                }
                r.exclusions.push_back( std::move( ex ) );
            }
            const auto width = integer( recipe, "maskWidth", 0 ),
                       height = integer( recipe, "maskHeight", 0 );
            require( width <= 65536 && height <= 65536 && u64( width ) * height <= 65536,
                     "Foliage masks exceed 65536 samples." );
            r.maskDimensions = { s32( width ), s32( height ) };
            auto density = cJSON_GetObjectItemCaseSensitive( recipe, "densityMask" ),
                 biome = cJSON_GetObjectItemCaseSensitive( recipe, "biomeMask" );
            require( cJSON_IsArray( density ) && cJSON_IsArray( biome ) &&
                         cJSON_GetArraySize( density ) <= 65536 && cJSON_GetArraySize( biome ) <= 65536,
                     "Invalid foliage masks." );
            for( auto v = density->child; v; v = v->next )
            {
                require( cJSON_IsNumber( v ) && std::isfinite( v->valuedouble ) && v->valuedouble >= 0 &&
                             v->valuedouble <= 1,
                         "Invalid density mask." );
                r.densityMask.push_back( f32( v->valuedouble ) );
            }
            for( auto v = biome->child; v; v = v->next )
            {
                require( cJSON_IsNumber( v ) && v->valuedouble >= 0 && v->valuedouble < r.rules.size() &&
                             std::floor( v->valuedouble ) == v->valuedouble,
                         "Invalid biome mask." );
                r.biomeMask.push_back( u32( v->valuedouble ) );
            }
            validate( r, world.species.size() );
            require( world.pageSize == r.pageSize, "Foliage recipe and world page sizes differ." );
            auto serialJson = cJSON_GetObjectItemCaseSensitive( root.get(), "manualSerial" );
            require( cJSON_IsString( serialJson ), "Missing manual serial." );
            std::string serialText( serialJson->valuestring );
            require( !serialText.empty() && serialText.size() <= 19 &&
                         serialText.find_first_not_of( "0123456789" ) == std::string::npos,
                     "Invalid manual serial." );
            const auto serial = std::stoull( serialText );
            require( serial > 0 && serial < 0x7fffffffffffffffULL, "Manual serial out of range." );
            for( const auto &i : world.instances )
            {
                if( i.id >> 63 )
                {
                    require( ( i.id & 0x7fffffffffffffffULL ) < serial,
                             "Manual serial would duplicate existing IDs." );
                }
            }
            std::map<u64, f32> offsets;
            auto offsetValues = cJSON_GetObjectItemCaseSensitive( root.get(), "offsets" );
            require( cJSON_IsArray( offsetValues ) &&
                         cJSON_GetArraySize( offsetValues ) <= maximumAuthoredInstances,
                     "Invalid foliage manual offsets." );
            std::set<u64> existing;
            for( const auto &i : world.instances )
            {
                existing.insert( i.id );
            }
            for( auto o = offsetValues->child; o; o = o->next )
            {
                auto idValue = cJSON_GetObjectItemCaseSensitive( o, "id" );
                require( cJSON_IsString( idValue ), "Invalid offset ID." );
                std::string value( idValue->valuestring );
                require( !value.empty() && value.size() <= 20 &&
                             value.find_first_not_of( "0123456789" ) == std::string::npos,
                         "Invalid offset ID." );
                const auto id = std::stoull( value );
                require( existing.count( id ) &&
                             offsets.emplace( id, f32( number( o, "height", 0 ) ) ).second,
                         "Duplicate or missing foliage offset ID." );
            }
            m_state->publish( std::move( world ) );
            m_state->current.offsets = std::move( offsets );
            m_state->current.recipe = std::move( r );
            m_state->current.manual = serial;
            error.clear();
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    bool FoliageAuthoring::save( const String &path, String &error ) const
    {
        std::lock_guard<std::recursive_mutex> guard( m_state->mutex );
        String data;
        if( !serialize( data, error ) )
        {
            return false;
        }
        std::filesystem::path temporary;
        try
        {
            require( !path.empty() && path.find( '\0' ) == String::npos, "Invalid foliage save path." );
            auto destination = std::filesystem::u8path( path.c_str() );
            temporary = destination;
            temporary += ".tmp-" + std::string( StringUtil::getUUID().c_str() );
            std::ofstream stream( temporary, std::ios::binary | std::ios::trunc );
            stream.write( data.data(), std::streamsize( data.size() ) );
            stream.flush();
            require( bool( stream ), "Cannot write foliage source." );
            stream.close();
            require( bool( stream ), "Cannot close foliage source." );
#ifdef _WIN32
            require( MoveFileExW( temporary.c_str(), destination.c_str(),
                                  MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH ) != 0,
                     "Cannot atomically replace foliage source." );
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

    bool FoliageAuthoring::load( const String &path, String &error )
    {
        std::lock_guard<std::recursive_mutex> guard( m_state->mutex );
        try
        {
            require( !path.empty() && path.find( '\0' ) == String::npos, "Invalid foliage load path." );
            auto file = std::filesystem::u8path( path.c_str() );
            const auto bytes = std::filesystem::file_size( file );
            require( bytes <= render::foliageMaximumSourceBytes, "Foliage source exceeds 128 MiB." );
            std::ifstream stream( file, std::ios::binary );
            std::string text( size_t( bytes ), '\0' );
            stream.read( text.data(), std::streamsize( bytes ) );
            require( bool( stream ), "Cannot read foliage source." );
            return deserialize( text.c_str(), error );
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }
}  // namespace workphone::scene
