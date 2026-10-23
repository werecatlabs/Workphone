#pragma once

#include <WPGraphics/ClawMaterial.hpp>
#include <WPGraphics/ClawMaterialRenderCache.hpp>
#include <WPGraphics/ClawMaterialDrawRecords.hpp>
#include <WPGraphics/ClawMesh.hpp>
#include <WPGraphics/ClawTexture.hpp>
#include <WPGraphics/ClawMaterialTechnique.hpp>
#include <WPGraphics/ClawMaterialPass.hpp>
#include <WPGraphics/ClawTextureManager.hpp>
#include <Workphone/Mesh/Mesh.hpp>
#include <Workphone/Graphics/MaterialManager.hpp>
#include <Workphone/Graphics/TextureManager.hpp>
#include <Workphone/Graphics/MaterialTexture.hpp>
#include <Workphone/Graphics/MaterialPass.hpp>
#include <Workphone/Mesh/MeshManager.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/System/FactoryTemplate.hpp>
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

    class MaterialInventory final : public MaterialManager
    {
    public:
        void grow() { m_materials.reserve(32); }
        void clearRetained() { m_materials.clear(); }
    };
    class TextureInventory final : public ClawTextureManager
    {
    public:
        void grow() { m_textures.reserve(32); m_renderTargets.reserve(16); }
    };
    class MeshInventory final : public MeshManager
    {
    public:
        SmartPtr<IMesh> mesh = make_ptr<workphone::Mesh>();
        void grow()
        {
            addMesh(mesh);
            addMeshResource(nullptr);
            removeMeshResource(nullptr); // retain inventory capacity, no invalid resource to unload
        }
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
            MaterialInventory materials;
            TextureInventory textures;
            MeshInventory meshes;
            for( const auto &grow : { std::function<void()>([&]{ materials.grow(); }),
                    std::function<void()>([&]{ textures.grow(); }),
                    std::function<void()>([&]{ meshes.grow(); }) } )
            {
                const auto before = scheduler->stats().used[source];
                limit(before); denied = false;
                try { grow(); } catch(const std::bad_alloc &) { denied = true; }
                require(denied && scheduler->stats().used[source] == before,
                    "manager inventory admission denies before growth");
                limit(baseline.limits[source]); grow();
                require(scheduler->stats().used[source] > before,
                    "material texture render-target and mesh inventories charge retained capacity");
            }
            const auto retained = scheduler->stats().used[source];
            materials.clearRetained();
            require(scheduler->stats().used[source] == retained, "empty reusable inventory stays charged");
            materials.unload(nullptr); textures.unload(nullptr); meshes.unload(nullptr);
        }
        require(scheduler->stats().used[source] == baseline.used[source], "inventory destruction returns physical capacity");
        {
            limit(baseline.used[source]); denied = false;
            try { auto node = make_ptr<MaterialTexture>(); } catch(const std::bad_alloc &) { denied = true; }
            require(denied && scheduler->stats().used[source] == baseline.used[source],
                "direct authoring node allocation participates before construction");
            limit(baseline.limits[source]);
            FactoryTemplate<MaterialTexture> factory;
            const auto before = scheduler->stats().used[source];
            limit(before + sourceBudgetStorageBytes(sizeof(MaterialTexture), alignof(MaterialTexture)));
            denied = false;
            try { factory.createObject(); } catch(const std::bad_alloc &) { denied = true; }
            require(denied && factory.getInstances().empty() && scheduler->stats().used[source] == before,
                "denied factory inventory publication destroys and frees its candidate object");
            limit(baseline.limits[source]);
            auto object = factory.createObject();
            require(object && factory.getInstances().size() == 1, "factory admission retries");
            factory.freeObject(object);
            factory.unload(nullptr);
        }
        require(scheduler->stats().used[source] == baseline.used[source], "factory teardown returns storage");
        {
            Pool<MaterialTexture> pool(2);
            limit(baseline.used[source]); denied = false;
            try { pool.allocate_object(); } catch(const std::bad_alloc &) { denied = true; }
            require(denied && scheduler->stats().used[source] == baseline.used[source],
                "asset pool denies backing storage without publishing a free slot");
            limit(baseline.limits[source]);
            MaterialTexture *slots[4];
            for(auto &slot : slots) slot = pool.allocate_object();
            const auto occupied = scheduler->stats().used[source];
            limit(occupied);
            for(auto slot : slots) pool.free_object(slot);
            require(scheduler->stats().used[source] == occupied,
                "pool return at the cap never allocates and retained unused slots remain charged");
            for(auto &slot : slots) slot = pool.allocate_object();
            require(scheduler->stats().used[source] == occupied, "warm pool reuse requires no new admission");
            for(auto slot : slots) pool.free_object(slot);
            limit(baseline.limits[source]);
        }
        require(scheduler->stats().used[source] == baseline.used[source], "pool teardown returns backing and free-list storage");
        {
            Material material;
            const String name("AnAuthoredShaderWithALongNameThatRequiresHeapStorage");
            const String path("Textures/City/VeryLongAuthoredTextureReferenceForBudgetCoverage.dds");
            limit(baseline.used[source]); denied = false;
            try { material.setShaderTexture(name, path); } catch(const std::bad_alloc &) { denied = true; }
            require(denied && material.getShaderTexture(name).empty() && scheduler->stats().used[source] == baseline.used[source],
                "cold shader authoring failure publishes no partial optional block");
            limit(baseline.limits[source]); material.setShaderTexture(name, path);
            const auto retained = scheduler->stats().used[source];
            limit(retained); denied = false;
            try { material.setShaderTexture(name, path + "/replacement"); } catch(const std::bad_alloc &) { denied = true; }
            require(denied && material.getShaderTexture(name) == path && scheduler->stats().used[source] == retained,
                "authoring path replacement admits old and new strings before publication");
            limit(baseline.limits[source]);
            MaterialTexture unit;
            unit.setScale({2,3,4});
            require(scheduler->stats().used[source] > retained, "optional texture facade metadata is charged");
            unit.setScale(Vector3<real_Num>::unit());
            require(scheduler->stats().used[source] == retained, "default facade restoration frees its optional storage");
        }
        require(scheduler->stats().used[source] == baseline.used[source], "authoring teardown returns storage");
        {
            Material material;
            Array<SmartPtr<IMaterialTechnique>> techniques(3);
            limit(baseline.used[source]); denied = false;
            try { material.setTechniques(techniques); } catch(const std::bad_alloc &) { denied = true; }
            require(denied && material.getNumTechniques() == 0, "denied authoring graph replacement preserves the previous graph");
            limit(baseline.limits[source]); material.setTechniques(techniques);
            require(material.getNumTechniques() == 3 && scheduler->stats().used[source] > baseline.used[source],
                "technique graph collections retry with admitted capacity");
            material.setTechniques({});
            require(scheduler->stats().used[source] == baseline.used[source], "empty graph replacement retires its actual old buffer");
            Array<SmartPtr<ITexture>> faces(6);
            material.setCubicTextures(faces);
            auto snapshot = material.getCubicTextureSnapshot();
            const auto retained = scheduler->stats().used[source];
            limit(retained); denied = false;
            try { material.setCubicTextures(Array<SmartPtr<ITexture>>(4)); } catch(const std::bad_alloc &) { denied = true; }
            require(denied && material.getCubicTextureSnapshot() == snapshot && scheduler->stats().used[source] == retained,
                "denied cube authoring replacement preserves the published immutable face list");
            material.setCubicTextures({});
            require(scheduler->stats().used[source] == retained, "retained cube snapshot keeps its source capacity after owner clear");
            snapshot.reset();
            require(scheduler->stats().used[source] == baseline.used[source], "last cube snapshot owner releases its object and buffer");
            limit(baseline.limits[source]);
        }
        {
            auto sourceTechnique = make_ptr<ClawMaterialTechnique>();
            auto targetTechnique = make_ptr<ClawMaterialTechnique>();
            auto pass = make_ptr<ClawMaterialPass>();
            sourceTechnique->addPass(pass);
            const auto before = scheduler->stats().used[source];
            limit(before); denied = false;
            try { targetTechnique->addPass(pass); } catch(const std::bad_alloc &) { denied = true; }
            require(denied && targetTechnique->getNumPasses() == 0 && pass->getParent() == sourceTechnique &&
                scheduler->stats().used[source] == before,
                "denied pass inventory growth preserves a caller-owned pass parent");
            limit(baseline.limits[source]);
        }
        require(scheduler->stats().used[source] == baseline.used[source], "denied graph adoption teardown returns storage");
        {
            auto state = make_ptr<MaterialPassStateData>();
            const auto snapshot = state->getRenderState();
            const auto before = scheduler->stats().used[source];
            limit(before); denied = false;
            try { state->setVertexShaderName("CityBudgetShader"); } catch(const std::bad_alloc &) { denied = true; }
            require(denied && state->getVertexShaderName().empty() && state->getStringAuthoringStorageBytes() == 0,
                "denied packed pass text preserves the original authoring fields");
            denied = false;
            try { state->setFloatSetting(MaterialFloatSetting::heightScale, 0.3f); } catch(const std::bad_alloc &) { denied = true; }
            require(denied && state->getNumericAuthoringStorageBytes() == 0 &&
                state->getRenderState() == snapshot && scheduler->stats().used[source] == before,
                "denied packed pass numeric metadata preserves the published render version");
            limit(baseline.limits[source]);
            state->setVertexShaderName("CityBudgetShader");
            state->setFloatSetting(MaterialFloatSetting::heightScale, 0.3f);
            require(state->getRenderState() == snapshot && scheduler->stats().used[source] > before,
                "cold pass metadata retry charges its buffers without rebuilding rendered values");
        }
        require(scheduler->stats().used[source] == baseline.used[source], "pass authoring payload and state teardown returns storage");
        {
            Array<MaterialShaderParameterInfo> info(1);
            info[0].m_name = "AUniqueLongShaderParameterNameForSharedBudgetCoverage";
            info[0].m_type = MaterialShaderParameterType::Scalar;
            limit(baseline.used[source]); denied = false;
            try { MaterialShaderParameterLayout::Create(info); } catch(const std::bad_alloc &) { denied = true; }
            require(denied && scheduler->stats().used[source] == baseline.used[source], "schema admission denies before cloning buffers");
            limit(baseline.limits[source]);
            MaterialShaderParametersInstance values;
            values.Reset(info);
            const auto handle = values.GetParameter(0);
            values.SetScalar(handle, 1.0f);
            auto copy = values;
            const auto shared = scheduler->stats().used[source];
            const auto revision = copy.GetRevision();
            limit(shared); denied = false;
            try { copy.SetScalar(handle, 2.0f); } catch(const std::bad_alloc &) { denied = true; }
            require(denied && copy.GetScalar(handle) == 1.0f && values.GetScalar(handle) == 1.0f &&
                copy.GetRevision() == revision && scheduler->stats().used[source] == shared,
                "denied shader copy-on-write preserves both owners and revision");
            limit(baseline.limits[source]); copy.SetScalar(handle, 2.0f);
            require(values.GetScalar(handle) == 1.0f && copy.GetScalar(handle) == 2.0f &&
                scheduler->stats().used[source] > shared, "shader copy-on-write retry charges independent values once");
        }
        require(scheduler->stats().used[source] == baseline.used[source], "shader schema and value teardown returns storage");
        {
            ClawTexture texture;
            Array<f32> overallocated(4, 1.0f);
            overallocated.reserve(16384);
            limit(baseline.used[source] + 8192);
            require(!texture.copyLinearFloatData(overallocated, {1,1}, error) &&
                    scheduler->stats().used[source] == baseline.used[source],
                "retained float texture admission includes copied capacity rather than only logical pixels");
            auto settings = make_ptr<Properties>();
            settings->setProperty("mipmapFilter", 1);
            limit(baseline.used[source]); denied = false;
            try { texture.setProperties(settings); } catch(const std::bad_alloc &) { denied = true; }
            require(denied && scheduler->stats().used[source] == baseline.used[source],
                "texture cache node and bucket admission denies without retained storage");
            limit(baseline.limits[source]); texture.setProperties(settings);
            require(scheduler->stats().used[source] > baseline.used[source], "empty texture cache metadata is charged without a pixel payload");
            texture.unload(nullptr);
            require(scheduler->stats().used[source] > baseline.used[source], "retained texture cache metadata stays charged after unload");
        }
        require(scheduler->stats().used[source] == baseline.used[source], "final texture cache erasure frees nodes and retained bucket capacity");

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
