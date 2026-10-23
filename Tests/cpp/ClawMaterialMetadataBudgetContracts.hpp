#pragma once

#include <WPGraphics/ClawMaterial.hpp>
#include <WPGraphics/ClawMaterialRenderCache.hpp>
#include <WPGraphics/ClawMaterialDrawRecords.hpp>
#include <WPGraphics/ClawMesh.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <vector>

namespace claw_material_metadata_budget_contracts
{
    using namespace workphone;
    using namespace workphone::render;
    inline void require( bool value, const char *message )
    { if( !value ) throw std::runtime_error( message ); }

    class Mesh final : public ClawMesh
    {
    public:
        SmartPtr<IMaterial> surface;
        SmartPtr<IMaterial> getMaterial( s32 = -1 ) const override { return surface; }
    };

    inline void run()
    {
        auto scheduler = WorldStreamingScheduler::shared();
        const auto source = size_t( StreamMemoryClass::SourceCpu );
        const auto baseline = scheduler->stats();
        struct Restore
        {
            std::shared_ptr<WorldStreamingScheduler> scheduler;
            u64 limit;
            ~Restore() { String error; scheduler->setBudget( StreamMemoryClass::SourceCpu, limit, error ); }
        } restore{ scheduler, baseline.limits[source] };
        String error;
        const auto limit = [&]( u64 bytes ) {
            require( scheduler->setBudget( StreamMemoryClass::SourceCpu, bytes, error ), "metadata test capacity applies" );
        };

        // Alignment padding, allocation-local ownership and physical retirement
        // also apply to allocator rebinds used by shared_ptr control blocks.
        limit( baseline.used[source] );
        bool denied = false;
        try { allocateSourceBudgetStorage( 257, 256 ); } catch( const std::bad_alloc & ) { denied = true; }
        require( denied && scheduler->stats().used[source] == baseline.used[source],
                 "metadata allocation denies before storage and leaks no reservation" );
        limit( baseline.limits[source] );
        auto aligned = allocateSourceBudgetStorage( 257, 256 );
        require( reinterpret_cast<std::uintptr_t>( aligned ) % 256 == 0 &&
                     scheduler->stats().used[source] == baseline.used[source] + sourceBudgetStorageBytes( 257, 256 ),
                 "metadata admission includes its exact header and alignment padding" );
        freeSourceBudgetStorage( aligned, 256 );
        {
            auto owner = std::allocate_shared<u64>( SourceBudgetAllocator<u64>{}, 7 );
            std::weak_ptr<u64> weak = owner;
            const auto allocated = scheduler->stats().used[source];
            owner.reset();
            require( weak.expired() && scheduler->stats().used[source] == allocated,
                     "coallocated control block storage remains charged while weak ownership retains it" );
            weak.reset();
        }
        require( scheduler->stats().used[source] == baseline.used[source], "final metadata allocation retirement returns all bytes" );

        {
            // Test-owned states bypass engine snapshot allocation so admission
            // below isolates persistent record pages, buckets, nodes and store.
            std::vector<MaterialRenderState::Ptr> states;
            for( u32 i = 0; i < 65; ++i ) states.push_back( std::make_shared<MaterialRenderState>() );
            ClawMaterialRenderCache cache;
            limit( baseline.used[source] ); denied = false;
            try { cache.resolve( states[0] ); } catch( const std::bad_alloc & ) { denied = true; }
            require( denied && cache.getResidentCount() == 0 && scheduler->stats().used[source] == baseline.used[source],
                     "cold record-store admission rejects without publishing a handle" );
            limit( baseline.limits[source] );
            const auto first = cache.resolve( states[0] );
            require( cache.lookup( first ) && scheduler->stats().used[source] > baseline.used[source],
                     "record store retries and retains charged metadata" );
            const auto resident = scheduler->stats();
            limit( resident.used[source] ); denied = false;
            try { cache.resolve( states[1] ); } catch( const std::bad_alloc & ) { denied = true; }
            require( denied && cache.lookup( first ) && cache.getResidentCount() == 1 &&
                         scheduler->stats().used[source] == resident.used[source],
                     "denied identity-node insertion preserves old records and releases candidate allocations" );
            limit( baseline.limits[source] );
            for( u32 i = 1; i < 64; ++i ) cache.resolve( states[i] );
            const auto fullPage = scheduler->stats();
            limit( fullPage.used[source] ); denied = false;
            try { cache.resolve( states[64] ); } catch( const std::bad_alloc & ) { denied = true; }
            require( denied && cache.getResidentCount() == 64 && cache.lookup( first ) &&
                         scheduler->stats().used[source] == fullPage.used[source],
                     "denied page growth preserves the resident city material table" );
            limit( baseline.limits[source] );
            require( cache.lookup( cache.resolve( states[64] ) ) && cache.getResidentCount() == 65,
                     "record-page growth retries after capacity returns" );
            auto lease = cache.retain();
            auto copy = cache;
            const auto shared = scheduler->stats().used[source];
            cache.clear(); copy.clear();
            require( lease.get( first ) && scheduler->stats().used[source] == shared,
                     "cache aliases and published leases share one metadata charge through retirement" );
            lease = {};
        }
        require( scheduler->stats().used[source] == baseline.used[source], "final cache lease retirement frees pages, buckets and nodes" );

        {
            auto material = make_ptr<ClawMaterial>();
            material->load( nullptr );
            require( material->isLoaded(), "metadata fixture loads its real material graph" );
            Mesh mesh; mesh.load( nullptr ); mesh.surface = material;
            const auto assets = scheduler->stats().used[source];
            {
                ClawMaterialRenderCache cache;
                ClawMaterialDrawRecords records;
                records.capture( &mesh ); records.seal( &cache );
                const auto binding = *records.find( &mesh );
                require( records.get( binding, 0 ) != nullptr, "draw metadata fixture publishes a retained record" );
                for( u64 headroom : { 0ull, 64ull, 128ull, 256ull, 512ull, 1024ull, 4096ull, 16384ull } )
                {
                    ClawMaterialDrawRecords candidate;
                    const auto before = scheduler->stats();
                    limit( before.used[source] + headroom ); denied = false;
                    try { candidate.capture( &mesh ); } catch( const std::bad_alloc & ) { denied = true; }
                    require( records.get( binding, 0 ) && records.valid( binding, &mesh, 1 ),
                             "candidate allocation denial preserves the previously sealed draw publication" );
                    if( denied ) require( candidate.getMeshCount() == 0 && candidate.getBindingCount() == 0,
                                          "failed capture exposes no partial mesh binding" );
                    limit( baseline.limits[source] );
                    candidate.capture( &mesh ); candidate.seal( &cache );
                    const auto retried = candidate.find( &mesh );
                    require( retried && candidate.valid( *retried, &mesh, 1 ) && candidate.get( *retried, 0 ),
                             "partially grown draw storage retries coherently" );
                }
                const auto warm = scheduler->stats();
                records.clear();
                require( scheduler->stats().used[source] == warm.used[source],
                         "cleared reusable publication capacity remains charged while physically retained" );
                limit( warm.used[source] );
                records.capture( &mesh ); records.seal( &cache );
                require( scheduler->stats().used[source] == warm.used[source] &&
                             scheduler->stats().rejectedReservations[source] == warm.rejectedReservations[source],
                         "warm metadata reuse makes no new admission request even at the limit" );
                limit( baseline.limits[source] );
            }
            require( scheduler->stats().used[source] == assets,
                     "draw and cache teardown return all metadata storage while assets remain live" );
        }
        require( scheduler->stats().used[source] == baseline.used[source], "metadata fixture returns the complete SourceCpu baseline" );
        std::puts( "Material metadata admission, cache leases, failed-growth rollback and warm reuse passed." );
    }
}
