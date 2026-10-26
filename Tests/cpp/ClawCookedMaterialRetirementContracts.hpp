#pragma once

#include <WPGraphics/ClawMaterialDrawRecords.hpp>
#include <WPGraphics/ClawMaterialRenderCache.hpp>
#include <Workphone/Graphics/MaterialPublicationScope.hpp>
#include <Workphone/System/StateContext.hpp>
#include <chrono>

// Included after the existing cooked-resource Fixture and generated-image helpers.
namespace claw_cooked_material_retirement_contracts
{
    using namespace workphone;
    using namespace workphone::render;
    using namespace workphone::resource;
    using Microsoft::WRL::ComPtr;

    inline void run( Fixture &fixture, u32 cycles = 32 )
    {
        require( cycles > 0 && cycles <= 10000, "Cooked retirement cycle count is bounded" );
        const auto sourceFolder = fixture.folder / "retirement-source";
        const auto compiledFolder = fixture.folder / "retirement-compiled";
        fs::create_directories( sourceFolder );
        writeImage( sourceFolder / "red.tga", false );
        writeImage( sourceFolder / "green.tga", true );
        writeText( sourceFolder / "red.texres",
            R"({"version":1,"source":"data://red.tga","mipFilter":"colour","atlasColumns":1,"alphaCutoff":0.5})" );
        writeText( sourceFolder / "green.texres",
            R"({"version":1,"source":"data://green.tga","mipFilter":"colour","atlasColumns":1,"alphaCutoff":0.5})" );
        writeText( sourceFolder / "red.matres",
            R"({"version":1,"baseColour":[1,1,1,1],"metalness":0,"roughness":1,"baseColourTexture":"data://red.texres"})" );
        writeText( sourceFolder / "green.matres",
            R"({"version":1,"baseColour":[1,1,1,1],"metalness":0,"roughness":1,"baseColourTexture":"data://green.texres"})" );

        ResourceSystem resources( fixture.resources->registry(),
                                  std::make_shared<ResourceCompilationDatabase>() );
        ResourceSystemConfig config;
        config.sourceRoot = pathText( sourceFolder );
        config.compiledRoot = pathText( compiledFolder );
        config.target = "cooked-material-retirement";
        String error;
        require( resources.initialize( config, error ), error );
        CompilationOptions options;
        options.packagedBuild = true;
        const ResourceID redId( "data://red.matres" ), greenId( "data://green.matres" );
        require( resources.compile( redId, options ).succeeded() &&
                 resources.compile( greenId, options ).succeeded(),
                 "Retirement fixture cooks two real material/texture dependency closures" );

        auto renderer = fixture.graphics.renderer;
        auto target = make_ptr<ClawRenderTarget>();
        target->setSize( { 32, 32 } );
        renderer->setRenderTarget( target );
        renderer->setViewport( nullptr );
        renderer->setCamera( nullptr );
        renderer->setSceneLighting( ColourF::White, Vector3F( 0, -1, 0 ), ColourF::White, 0 );
        struct ResetTarget
        {
            SmartPtr<ClawRendererDX11> renderer;
            ~ResetTarget() { renderer->setRenderTarget( nullptr ); }
        } resetTarget{ renderer };
        Triangle mesh;
        const auto native = wp_renderer_get_dx11( renderer->getNativeRenderer() );
        const auto device = static_cast<ID3D11Device *>( wp_renderer_dx11_get_device( native ) );
        const auto context = static_cast<ID3D11DeviceContext *>( wp_renderer_dx11_get_context( native ) );
        const auto scheduler = WorldStreamingScheduler::shared();
        constexpr auto source = size_t( StreamMemoryClass::SourceCpu );
        constexpr auto gpu = size_t( StreamMemoryClass::Gpu );
        constexpr auto working = size_t( StreamMemoryClass::WorkingCpu );
        constexpr u64 textureBytes = ( 4 * 4 + 2 * 2 + 1 ) * 4;
        const auto printInventory = []( const char *phase, u32 cycle ) {
            auto types = TypeManager::instance();
            u64 baseBlocks = types->baseObjectDataPool.getSize();
            u64 sharedBlocks = types->sharedObjectDataPool.getSize();
            for( const auto &pool : types->baseObjectDataPools ) baseBlocks += pool.getSize();
            for( const auto &pool : types->sharedObjectDataPools ) sharedBlocks += pool.getSize();
            std::fprintf( stderr, "Cooked material retirement inventory: phase=%s cycle=%u base_pool_blocks=%llu shared_pool_blocks=%llu state_context_instances=%u material_instances=%u texture_instances=%u\n",
                phase, cycle, static_cast<unsigned long long>( baseBlocks ), static_cast<unsigned long long>( sharedBlocks ),
                types->getNumInstances( StateContext::typeInfo() ), types->getNumInstances( ClawMaterial::typeInfo() ),
                types->getNumInstances( ClawTexture::typeInfo() ) );
#if WP_USE_WP_STRING && WP_USE_STANDARD_STRING_ALLOCATOR
            const auto strings = String::character_pool_stats();
            std::fprintf( stderr, "Cooked material retirement character inventory: phase=%s cycle=%u committed=%llu slabs=%llu live=%llu free=%llu\n",
                phase, cycle, static_cast<unsigned long long>( strings.committedBytes ), static_cast<unsigned long long>( strings.slabCount ),
                static_cast<unsigned long long>( strings.liveBlocks ), static_cast<unsigned long long>( strings.freeBlocks ) );
#endif
        };
        std::array<u64, 10> sourceStages{};
        // The stable target and mesh are fixture owners, not per-generation
        // texture cost. Admit their real native storage before comparing the
        // exact two-texture GPU deltas, including during the disclosed warm-up.
        mesh.load( nullptr );
        require( mesh.isLoaded(), "Cooked retirement admits its fixed native mesh" );
        renderer->clear( ColourF::Black );
        context->ClearState();
        wp_renderer_dx11_invalidate_state( native );
        claw_texture_mip_contracts::completeGpuWork( context );

        const auto draw = [&]( const ClawMaterialDrawRecords &records, bool green ) {
            renderer->setRenderTarget( target );
            renderer->setViewport( nullptr );
            renderer->clear( ColourF::Black );
            const auto binding = records.find( &mesh );
            require( binding && renderer->renderMeshRecords( &mesh, Matrix4F::identity(), records, *binding ),
                     "Retained cooked material records submit through the real renderer" );
            ComPtr<ID3D11RenderTargetView> targetView;
            context->OMGetRenderTargets( 1, targetView.GetAddressOf(), nullptr );
            require( targetView != nullptr, "Retained cooked draw binds its target" );
            ComPtr<ID3D11Resource> renderedResource;
            targetView->GetResource( renderedResource.GetAddressOf() );
            ComPtr<ID3D11Texture2D> rendered;
            require( SUCCEEDED( renderedResource.As( &rendered ) ), "Retirement draw has a 2D target" );
            D3D11_TEXTURE2D_DESC description{};
            rendered->GetDesc( &description );
            const auto format = description.Format;
            description.Usage = D3D11_USAGE_STAGING;
            description.BindFlags = description.MiscFlags = 0;
            description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            ComPtr<ID3D11Texture2D> staging;
            require( SUCCEEDED( device->CreateTexture2D( &description, nullptr, staging.GetAddressOf() ) ),
                     "Retirement draw readback allocates" );
            context->CopyResource( staging.Get(), rendered.Get() );
            D3D11_MAPPED_SUBRESOURCE mapped{};
            require( SUCCEEDED( context->Map( staging.Get(), 0, D3D11_MAP_READ, 0, &mapped ) ),
                     "Retirement draw readback maps" );
            const auto pixel = static_cast<const u8 *>( mapped.pData ) + 16 * mapped.RowPitch + 16 * 4;
            const auto red = pixel[format == DXGI_FORMAT_B8G8R8A8_UNORM ? 2 : 0];
            const auto actualGreen = pixel[1];
            const auto blue = pixel[format == DXGI_FORMAT_B8G8R8A8_UNORM ? 0 : 2];
            context->Unmap( staging.Get(), 0 );
            require( green ? actualGreen > 60 && actualGreen > red + 40 && actualGreen > blue + 40
                           : red > 60 && red > actualGreen + 40 && red > blue + 40,
                     "Retained old/new records preserve their own red/green cooked GPU pixels" );
        };

        const auto cycle = [&] {
            const auto before = scheduler->stats();
            sourceStages[0] = before.used[source];
            ClawMaterialRenderCache cache;
            ClawMaterialDrawRecords oldRecords, newRecords;
            std::weak_ptr<const ClawMaterialResource::Candidate> oldCandidate, newCandidate;
            std::weak_ptr<const RuntimeResource> oldPayload, newPayload;
            std::weak_ptr<const MaterialRenderState> oldState, newState;
            ComPtr<ID3D11ShaderResourceView> retainedView;
            ComPtr<ID3D11Resource> retainedResource;
            {
                ClawMaterialResource publisher( nullptr, nullptr, "", {}, renderer, config.target, true );
                auto redResource = resources.load( redId, error );
                sourceStages[1] = scheduler->stats().used[source];
                require( redResource != nullptr, error );
                oldPayload = redResource;
                auto red = publisher.stageRuntime( redResource, error );
                sourceStages[2] = scheduler->stats().used[source];
                require( red && publisher.publish( red, error ), error );
                oldCandidate = red;
                oldState = red->material()->getRenderState();
                mesh.setMaterial( red->material() );
                oldRecords.capture( &mesh );
                oldRecords.seal( &cache );
                retainedView = static_cast<ID3D11ShaderResourceView *>( red->texture()->getNativeTextureView( native ) );
                require( retainedView != nullptr, "Cooked retirement retains a real texture SRV" );
                retainedView->GetResource( retainedResource.GetAddressOf() );

                auto greenResource = resources.load( greenId, error );
                sourceStages[3] = scheduler->stats().used[source];
                require( greenResource != nullptr, error );
                newPayload = greenResource;
                auto green = publisher.stageRuntime( greenResource, error );
                sourceStages[4] = scheduler->stats().used[source];
                require( green && publisher.publish( green, error ) && publisher.current() == green, error );
                newCandidate = green;
                newState = green->material()->getRenderState();
                mesh.setMaterial( green->material() );
                newRecords.capture( &mesh );
                newRecords.seal( &cache );
                draw( newRecords, true );
                sourceStages[5] = scheduler->stats().used[source];
                require( scheduler->stats().used[gpu] == before.used[gpu] + 2 * textureBytes,
                         "Two live cooked generations admit exactly both complete GPU mip chains" );
                require( publisher.unload( error ) && !publisher.current(), error );
                mesh.setMaterial( nullptr );
                resources.unload( redId );
            }
            sourceStages[6] = scheduler->stats().used[source];
            require( oldCandidate.expired() && newCandidate.expired() &&
                     oldPayload.expired() && newPayload.expired() &&
                     !oldState.expired() && !newState.expired(),
                     "Resource cache and publisher unload while captured material generations remain owned" );
            draw( oldRecords, false );
            sourceStages[7] = scheduler->stats().used[source];
            draw( newRecords, true );
            draw( oldRecords, false );
            // The native renderer intentionally retains bindings between meshes.
            // Remove those real context references and fence submitted work before
            // attributing ownership to the records or explicit external COM pins.
            context->ClearState();
            wp_renderer_dx11_invalidate_state( native );
            newRecords.clear();
            oldRecords.clear();
            sourceStages[8] = scheduler->stats().used[source];
            require( oldState.expired() && newState.expired(),
                     "Clearing the final captured records retires both immutable material versions" );
            claw_texture_mip_contracts::completeGpuWork( context );
            require( scheduler->stats().used[gpu] == before.used[gpu] + textureBytes,
                     "Only the deliberately retained old SRV/resource keeps its physical GPU mip lease" );
            retainedView.Reset();
            require( scheduler->stats().used[gpu] == before.used[gpu] + textureBytes,
                     "The external native resource retains the mip reservation after the final SRV release" );
            retainedResource.Reset();
            claw_texture_mip_contracts::completeGpuWork( context );
            require( scheduler->stats().used[gpu] == before.used[gpu],
                     "The last completed native resource release retires the exact physical GPU reservation" );
            sourceStages[9] = scheduler->stats().used[source];
        };

        // Disclose one complete peak-shape warm-up. It includes the native mesh,
        // material/type metadata, resource-cache keys and all four retained draws;
        // every requested cycle below is still measured and checked.
        const auto cold = scheduler->stats();
        cycle();
        const auto baseline = scheduler->stats();
        const auto warmStages = sourceStages;
        printInventory( "warm", 0 );
        const auto contextCount = fixture.states->getStateContexts().size();
        const auto queueBytes = MaterialPublicationScope::retainedStorageBytes();
        std::fprintf( stderr, "Cooked material retirement warm-up: SourceCpu=%llu->%llu Gpu=%llu->%llu WorkingCpu=%llu->%llu\n",
            static_cast<unsigned long long>( cold.used[source] ), static_cast<unsigned long long>( baseline.used[source] ),
            static_cast<unsigned long long>( cold.used[gpu] ), static_cast<unsigned long long>( baseline.used[gpu] ),
            static_cast<unsigned long long>( cold.used[working] ), static_cast<unsigned long long>( baseline.used[working] ) );
        const auto started = std::chrono::steady_clock::now();
        for( u32 index = 0; index < cycles; ++index )
        {
            cycle();
            const auto after = scheduler->stats();
            if( ( index + 1 ) % 16 == 0 ) printInventory( "measured", index + 1 );
            if( after.used[source] != baseline.used[source] || after.used[gpu] != baseline.used[gpu] ||
                after.used[working] != baseline.used[working] ||
                fixture.states->getStateContexts().size() != contextCount || MaterialPublicationScope::pendingEntryCount() ||
                MaterialPublicationScope::retainedStorageBytes() != queueBytes )
            {
                printInventory( "failed", index + 1 );
                std::fprintf( stderr, "Cooked material retirement Source stages: before red_load red_stage green_load green_stage new_draw owner_unload old_draw record_clear native_release\n" );
                for( size_t stage = 0; stage < sourceStages.size(); ++stage )
                    std::fprintf( stderr, "Cooked material retirement Source stage: stage=%zu warm=%llu current=%llu\n", stage,
                        static_cast<unsigned long long>( warmStages[stage] ), static_cast<unsigned long long>( sourceStages[stage] ) );
                std::fprintf( stderr, "Cooked material retirement boundary: cycle=%u SourceCpu=%llu->%llu Gpu=%llu->%llu WorkingCpu=%llu->%llu contexts=%llu->%llu\n",
                    index + 1, static_cast<unsigned long long>( baseline.used[source] ), static_cast<unsigned long long>( after.used[source] ),
                    static_cast<unsigned long long>( baseline.used[gpu] ), static_cast<unsigned long long>( after.used[gpu] ),
                    static_cast<unsigned long long>( baseline.used[working] ), static_cast<unsigned long long>( after.used[working] ),
                    static_cast<unsigned long long>( contextCount ), static_cast<unsigned long long>( fixture.states->getStateContexts().size() ) );
                require( false, "Every measured cooked load/publish/draw/unload cycle returns exact warmed CPU/GPU ownership" );
            }
        }
        resources.shutdown();
        std::fprintf( stderr, "Cooked material retirement passed: cycles=%u loaded_materials=%llu texture_uploads=%llu retained_record_draws=%llu SourceCpu=%llu Gpu=%llu WorkingCpu=%llu elapsed_seconds=%.6f\n",
            cycles, static_cast<unsigned long long>( cycles ) * 2, static_cast<unsigned long long>( cycles ) * 2,
            static_cast<unsigned long long>( cycles ) * 4, static_cast<unsigned long long>( baseline.used[source] ),
            static_cast<unsigned long long>( baseline.used[gpu] ), static_cast<unsigned long long>( baseline.used[working] ),
            std::chrono::duration<double>( std::chrono::steady_clock::now() - started ).count() );
    }
}
