#pragma once
#include "MaterialRenderCacheReference.hpp"
#include <WPGraphics/ClawMaterialRenderCache.hpp>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace material_cache_benchmark
{
    using namespace workphone;
    using render::ClawMaterialRenderCache;
    using Reference = render::material_cache_reference::MaterialRenderCacheReference;

    inline void require( bool condition, const char *message )
    {
        if( !condition ) throw std::runtime_error( message );
    }

    inline std::shared_ptr<MaterialRenderState> makeState( size_t seed )
    {
        auto state = std::make_shared<MaterialRenderState>();
        const auto v = f32( seed % 97 ) / 97.0f;
        state->diffuseColour = ColourF( v, 0.4f, 1.0f - v, 0.8f );
        state->emissiveColour = ColourF( v, v * 2.0f, v * 3.0f, 1.0f );
        state->flags = seed % 2 ? render::emissionEnabledFlag | render::cutoutFlag : 0u;
        state->emissionIntensity = 2.0f;
        state->metalness = v;
        state->roughness = 1.0f - v;
        state->uvOffsetX = v;
        state->uvRotation = f32( seed % 360 );
        state->uvProjection = u32( seed % 9 );
        state->uvTriplanarScale = v;
        state->metallicSource = u32( seed % 4 );
        state->roughnessSource = u32( seed % 6 );
        state->aoSource = u32( seed % 2 );
        state->opacitySource = u32( seed % 3 );
        state->blendMode = u32( seed % 5 );
        state->materialType = seed % 3 ? MaterialType::Standard : MaterialType::UI;
        state->uvWrapU = u32( seed % 5 );
        state->uvWrapV = u32( ( seed + 1 ) % 5 );
        state->uvFilter = u32( seed % 4 );
        state->uvAniso = v * 20.0f;
        return state;
    }

    inline size_t bucket( const MaterialRenderState::Ptr &state )
    {
        auto hash = std::hash<const MaterialRenderState *>{}( state.get() ) >> 4u;
        hash ^= hash >> 11u;
        return hash % ClawMaterialRenderCache::SetCount;
    }

    // Choose identical bucket occupancy for both implementations. Retain rejected
    // candidates until selection finishes so allocator reuse cannot stall a bucket.
    inline std::vector<MaterialRenderState::Ptr> makeStates( size_t perBucket )
    {
        std::array<std::vector<MaterialRenderState::Ptr>, ClawMaterialRenderCache::SetCount> buckets;
        std::vector<MaterialRenderState::Ptr> pool;
        size_t selected = 0;
        while( selected < perBucket * buckets.size() && pool.size() < 100000 )
        {
            const MaterialRenderState::Ptr state = makeState( pool.size() );
            pool.push_back( state );
            auto &target = buckets[bucket( state )];
            if( target.size() < perBucket )
            {
                target.push_back( state );
                ++selected;
            }
        }
        require( selected == perBucket * buckets.size(), "benchmark bucket selection must complete" );
        std::vector<MaterialRenderState::Ptr> result;
        for( size_t way = 0; way < perBucket; ++way )
            for( const auto &items : buckets ) result.push_back( items[way] );
        // Values depend on workload order, not allocator addresses or the number
        // of rejected candidates, so the reported checksum is reproducible.
        for( size_t i = 0; i < result.size(); ++i )
        {
            auto values = makeState( i );
            *std::const_pointer_cast<MaterialRenderState>( result[i] ) = *values;
        }
        return result;
    }

    inline void contracts()
    {
        ClawMaterialRenderCache cache;
        Reference reference;
        require( sizeof( cache ) < sizeof( reference ) * 3 / 4,
                 "compact cache must use less than three quarters of the former fixed storage" );
        const auto states = makeStates( 8 );
        for( int pass = 0; pass < 3; ++pass )
            for( const auto &state : states )
            {
                const auto &a = reference.get( state );
                const auto &b = cache.get( state );
                const auto before = a.compose(), after = b.compose();
                require( std::memcmp( &before, &after, sizeof( before ) ) == 0 &&
                         a.textureSlots == b.textureSlots && a.wrapU == b.wrapU && a.wrapV == b.wrapV &&
                         a.filter == b.filter && a.anisotropy == b.anisotropy,
                         "compact preparation must preserve every native constant and binding" );
            }
        require( cache.getPreparationCount() == states.size() &&
                 cache.getPreparationCount() <= reference.getPreparationCount(),
                 "persistent records avoid re-preparation beyond the front-cache capacity" );
        // Include the static null/default record and cache copy/move after a hit.
        const auto defaults = cache.get( {} ).compose();
        const auto oldDefaults = reference.get( {} ).compose();
        require( std::memcmp( &defaults, &oldDefaults, sizeof( defaults ) ) == 0,
                 "null preparation preserves the former defaults" );
        auto copy = cache;
        auto moved = std::move( copy );
        const auto preparations = moved.getPreparationCount();
        moved.get( states.back() );
        require( moved.getPreparationCount() == preparations,
                 "last-use indices remain valid when a cache is copied or moved" );
    }

    // Read all submitted bytes out of line. The timed path must materialize the
    // complete draw block; a checksum of only roughness could eliminate packing.
#if defined( _MSC_VER )
    __declspec( noinline )
#else
    __attribute__( ( noinline ) )
#endif
    inline u64 consume( const wp_material_dx11 &draw, const std::array<u32, 6> &slots, u32 sampler )
    {
        std::array<u32, sizeof( draw ) / sizeof( u32 )> words;
        std::memcpy( words.data(), &draw, sizeof( draw ) );
        u64 sum = sampler;
        for( const auto word : words ) sum += word;
        for( const auto slot : slots ) sum += slot;
        return sum;
    }

    struct Measurement { double ms; u64 checksum; u64 preparations; };

    template <class Cache>
    Measurement measure( Cache &cache, const std::vector<MaterialRenderState::Ptr> &states,
                         size_t groupSize, size_t draws )
    {
        cache.clear();
        for( const auto &state : states ) cache.get( state );
        u64 checksum = 0;
        const auto start = std::chrono::steady_clock::now();
        for( size_t drawIndex = 0; drawIndex < draws; ++drawIndex )
        {
            const auto &prepared = cache.get( states[( drawIndex / groupSize ) % states.size()] );
            auto draw = prepared.compose();
            draw.light_color = { 1, 0.5f, 0.25f, f32( drawIndex % 4 ) };
            draw.camera_position = { 1, 2, 3, 0.3f };
            draw.fog_params = { 1, 0.01f, 1, 100 };
            draw.map_flags = { 1, 1, 0, 1 };
            draw.extra_map_flags = { 1, 0, 0, 0 };
            checksum += consume( draw, prepared.textureSlots,
                                 prepared.wrapU + prepared.wrapV + prepared.filter + prepared.anisotropy );
        }
        const auto elapsed = std::chrono::steady_clock::now() - start;
        return { std::chrono::duration<double, std::milli>( elapsed ).count(), checksum,
                 cache.getPreparationCount() };
    }

    inline void compare( const char *name, const std::vector<MaterialRenderState::Ptr> &states,
                         size_t groupSize )
    {
        constexpr size_t draws = 1000000, samples = 7;
        Reference reference;
        ClawMaterialRenderCache cache;
        std::array<double, samples> oldTimes{}, newTimes{};
        Measurement before{}, after{};
        for( size_t sample = 0; sample < samples; ++sample )
        {
            // Alternate execution order to limit a consistent thermal/order bias.
            if( sample % 2 )
            {
                after = measure( cache, states, groupSize, draws );
                before = measure( reference, states, groupSize, draws );
            }
            else
            {
                before = measure( reference, states, groupSize, draws );
                after = measure( cache, states, groupSize, draws );
            }
            require( before.checksum == after.checksum && after.preparations == states.size(),
                     "persistent cache benchmark requires equal output and one preparation per live state" );
            oldTimes[sample] = before.ms;
            newTimes[sample] = after.ms;
        }
        std::sort( oldTimes.begin(), oldTimes.end() );
        std::sort( newTimes.begin(), newTimes.end() );
        std::cout << "workload=" << name << " states=" << states.size() << " draws=" << draws
                  << " grouped_draws=" << groupSize << " samples=" << samples
                  << " former_median_ms=" << oldTimes[samples / 2]
                  << " compact_median_ms=" << newTimes[samples / 2]
                  << " former_preparations=" << before.preparations
                  << " persistent_preparations=" << after.preparations
                  << " persistent_accounted_bytes=" << cache.getStorageBytes()
                  << " checksum=" << after.checksum << '\n';
    }

    inline void run()
    {
        contracts();
        std::cout << "former_cache_bytes=" << sizeof( Reference )
                  << " persistent_cache_control_bytes=" << sizeof( ClawMaterialRenderCache )
                  << " former_prepared_bytes=" << sizeof( render::material_cache_reference::PreparedMaterialReference )
                  << " compact_prepared_bytes=" << sizeof( render::ClawPreparedMaterialDX11 ) << '\n';
        compare( "grouped", makeStates( 1 ), 64 );
        const auto fourWays = makeStates( 4 );
        std::vector<MaterialRenderState::Ptr> colliding;
        for( size_t way = 0; way < 4; ++way ) colliding.push_back( fourWays[way * ClawMaterialRenderCache::SetCount] );
        compare( "four_way_hits", colliding, 1 );
        compare( "capacity_misses", makeStates( 8 ), 1 );
    }
}
