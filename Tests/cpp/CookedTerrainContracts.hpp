#pragma once
#include "ClawTerrainContracts.hpp"
#include "ClawFoliageContracts.hpp"
#include <WPGraphics/Resources/TerrainResource.hpp>
#include <WPGraphics/Resources/GraphicsResourceCompiler.hpp>
#include <Workphone/System/ResourceSystem.hpp>
#include <Workphone/Graphics/Texture.hpp>
#include <Workphone/System/ResourceCompilerRegistry.hpp>
#include <WPSQLite/ResourceCompilationDatabase.hpp>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <thread>

namespace cooked_terrain_contracts
{
    using namespace workphone;
    using namespace workphone::render;
    using namespace workphone::resource;
    namespace fs = std::filesystem;
    inline void require( bool value, const String &error )
    {
        if( !value )
            throw std::runtime_error( error.c_str() );
    }
    inline String path( const fs::path &value )
    {
        return value.generic_u8string().c_str();
    }
    inline void text( const fs::path &file, const String &value )
    {
        std::ofstream out( file, std::ios::binary );
        out.write( value.data(), value.size() );
        require( out.good(), "Terrain fixture file write failed" );
    }
    inline void image( const fs::path &file, const std::array<u8, 4> &rgba )
    {
        std::array<u8, 18 + 16 * 4> bytes{};
        bytes[2] = 2;
        bytes[12] = bytes[14] = 4;
        bytes[16] = 32;
        bytes[17] = 0x28;
        for( size_t i = 18; i < bytes.size(); i += 4 )
        {
            bytes[i] = rgba[2];
            bytes[i + 1] = rgba[1];
            bytes[i + 2] = rgba[0];
            bytes[i + 3] = rgba[3];
        }
        std::ofstream out( file, std::ios::binary );
        out.write( reinterpret_cast<const char *>( bytes.data() ), bytes.size() );
        require( out.good(), "Terrain fixture TGA write failed" );
    }
    // Identical encoded pixels exposed through two legal native SRV formats.
    // This catches both missing decode on UNORM and double decode on SRGB views.
    class ColourTransferTexture : public Texture
    {
    public:
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> view;
        ColourTransferTexture( ID3D11Device *device, bool srgb )
        {
            D3D11_TEXTURE2D_DESC desc{};
            desc.Width = desc.Height = 4;
            desc.MipLevels = desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_B8G8R8A8_TYPELESS;
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_IMMUTABLE;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            std::array<u8, 64> pixels{};
            for( size_t i = 0; i < pixels.size(); i += 4 )
            {
                pixels[i] = pixels[i + 1] = pixels[i + 2] = 128;
                pixels[i + 3] = 255;
            }
            D3D11_SUBRESOURCE_DATA data{ pixels.data(), 16, 64 };
            Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
            require( SUCCEEDED( device->CreateTexture2D( &desc, &data, &texture ) ), "Colour transfer source must allocate" );
            D3D11_SHADER_RESOURCE_VIEW_DESC srv{};
            srv.Format = srgb ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB : DXGI_FORMAT_B8G8R8A8_UNORM;
            srv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srv.Texture2D.MipLevels = 1;
            require( SUCCEEDED( device->CreateShaderResourceView( texture.Get(), &srv, &view ) ), "Colour transfer view must allocate" );
        }
        void getTextureFinal( void **value ) const override { if( value ) *value = view.Get(); }
    };

    inline void colourTransfer( ClawRendererDX11 &renderer )
    {
        auto native = wp_renderer_get_dx11( renderer.getNativeRenderer() );
        auto device = static_cast<ID3D11Device *>( wp_renderer_dx11_get_device( native ) );
        auto encoded = make_ptr<ColourTransferTexture>( device, false );
        auto hardwareDecoded = make_ptr<ColourTransferTexture>( device, true );
        TerrainData data;
        data.dimensions = { 5, 5 };
        data.spacing = { .5f, .5f };
        data.origin = { -1, -1 };
        data.heights.assign( 25, 0 );
        for( auto &layer : data.layers )
        {
            layer.albedo = "mid-grey";
            layer.tint = { .35f, .70f, .90f, 1 };
        }
        renderer.setSceneLighting( ColourF::White, { 0, -1, 0 }, ColourF::White, 0 );
        renderer.setSceneFog( 0, ColourF::Black, 0, 0, 1 );
        auto terrain = make_ptr<ClawTerrain>();
        String error;
        const auto draw = [&]( SmartPtr<ITexture> texture, TerrainProjection projection ) {
            data.appearance.projection = projection;
            require( terrain->applyCookedTerrain( data,
                [texture]( const String &, String & ) { return texture; }, error ), error );
            renderer.clear( ColourF::Black );
            renderer.renderTerrain( terrain );
            return claw_foliage_contracts::pixels( native );
        };
        for( auto projection : { TerrainProjection::Planar, TerrainProjection::Triplanar } )
        {
            const auto software = draw( encoded, projection );
            const auto hardware = draw( hardwareDecoded, projection );
            require( software.size() == 128 * 128 * 4 && hardware.size() == software.size(), "Colour transfer readback extent changed" );
            const size_t centre = ( 64 * 128 + 64 ) * 4;
            for( size_t channel = 0; channel < 3; ++channel )
            {
                const int a = software[centre + channel], b = hardware[centre + channel];
                if( std::abs( a - b ) > 2 )
                    std::printf( "Terrain colour transfer mismatch: projection=%u channel=%zu UNORM=%d SRGB=%d\n",
                                 unsigned( projection ), channel, a, b );
                require( std::abs( a - b ) <= 2,
                         "Mid-grey terrain must shade equivalently through UNORM and hardware-SRGB views with nonwhite tint" );
                const float tint[] = { .35f, .70f, .90f };
                const float linearGrey = std::pow( ( 128.f / 255.f + .055f ) / 1.055f, 2.4f );
                // Terrain tint follows the existing authoring colour convention:
                // its encoded RGB is decoded independently from the texture.
                const float linearTint = std::pow( ( tint[channel] + .055f ) / 1.055f, 2.4f );
                const float radiance = linearGrey * linearTint * .96f;
                const int expected = int( 255.f * std::pow( radiance / ( 1.f + radiance ), 1.f / 2.2f ) + .5f );
                if( std::abs( b - expected ) > 6 )
                    std::printf( "Terrain analytic mismatch: projection=%u channel=%zu actual=%d expected=%d\n", unsigned( projection ), channel, b, expected );
                require( std::abs( b - expected ) <= 6,
                         "Terrain tint must multiply linear albedo before lighting and display encoding" );
            }
        }
        terrain->unload( nullptr );
        std::printf( "Terrain colour transfer: mid-grey UNORM/SRGB, nonwhite tint, planar/triplanar PASS\n" );
    }

    inline void run( ClawRendererDX11 &renderer, const fs::path &parent )
    {
        struct Scope
        {
            ClawRendererDX11 &r;
            ~Scope()
            {
                r.setCamera( nullptr );
                r.setRenderTarget( nullptr );
            }
        } scope{ renderer };
        claw_terrain_contracts::MeshManagerScope meshManager;
        const auto folder = parent / "terrain-package", source = folder / "authoring",
                   compiled = folder / "compiled", package = folder / "package";
        fs::create_directories( source );
        TerrainData authored;
        authored.dimensions = { 5, 5 };
        authored.spacing = { .5f, .5f };
        authored.origin = { -1, -1 };
        authored.heights.assign( 25, 0 );
        authored.layerWeights.resize( 25 );
        const std::array<u8, 4> colours[] = {
            { 255, 0, 0, 255 }, { 0, 255, 0, 255 }, { 0, 0, 255, 255 }, { 255, 255, 0, 255 }
        };
        for( int layer = 0; layer < 4; ++layer )
        {
            String *names[] = { &authored.layers[layer].albedo, &authored.layers[layer].normal,
                                &authored.layers[layer].orm };
            for( int map = 0; map < 3; ++map )
            {
                const auto stem =
                    std::string( "layer" ) + std::to_string( layer ) + "_" + std::to_string( map );
                *names[map] = String( "data://" ) + stem.c_str() + ".texres";
                image( source / ( stem + ".tga" ), map == 0   ? colours[layer]
                                                   : map == 1 ? std::array<u8, 4>{ 128, 128, 255, 255 }
                                                              : std::array<u8, 4>{ 255, 220, 0, 255 } );
                const auto filter = map == 0 ? "colour" : map == 1 ? "normal" : "data";
                text( source / ( stem + ".texres" ), String( "{\"version\":1,\"source\":\"data://" ) +
                                                         stem.c_str() + ".tga\",\"mipFilter\":\"" +
                                                         filter +
                                                         "\",\"atlasColumns\":1,\"alphaCutoff\":0.5}" );
            }
        }
        for( int z = 0; z < 5; ++z )
            for( int x = 0; x < 5; ++x )
            {
                auto &w = authored.layerWeights[z * 5 + x];
                w = { 0, 0, 0, 0 };
                w[( z >= 2 ? 2 : 0 ) + ( x >= 2 ? 1 : 0 )] = 1;
            }
        authored.appearance.projection = TerrainProjection::Triplanar;
        authored.appearance.detailNormal = "data://global_detail.texres";
        authored.appearance.macroStrength = .1f;
        authored.appearance.macroSeed = 0xffffffffu;
        authored.appearance.wetness = .2f;
        authored.surfaceMasks.assign( 25, Vector2F( .5f, 1 ) );
        image( source / "global_detail.tga", { 220, 128, 210, 255 } );
        text( source / "global_detail.texres",
              "{\"version\":1,\"source\":\"data://global_detail.tga\",\"mipFilter\":\"normal\",\"atlasColumns\":1,\"alphaCutoff\":0.5}" );
        text( source / "ground.terrain", serializeTerrainData( authored ) );
        auto registry = std::make_shared<ResourceCompilerRegistry>();
        String error;
        require( registry->registerCompiler( std::make_shared<GraphicsResourceCompiler>(), &error ),
                 error );
        require( registry->registerCompiler( std::make_shared<TerrainResourceCompiler>(), &error ),
                 error );
        ResourceSystem author( registry, std::make_shared<ResourceCompilationDatabase>() );
        ResourceSystemConfig config;
        config.sourceRoot = path( source );
        config.compiledRoot = path( compiled );
        config.target = "terrain-dx11-contract";
        require( author.initialize( config, error ), error );
        const ResourceID id( "data://ground.terrain" );
        CompilationOptions options;
        options.packagedBuild = true;
        const auto report = author.compile( id, options );
        if( !report.succeeded() )
        {
            for( const auto &m : report.messages )
                error += " " + m;
        }
        require( report.succeeded(), "Terrain package compilation failed: " + error );
        require( author.writeRuntimeManifest( path( compiled / "terrain.wprs" ), { id }, error ),
                 error );
        author.shutdown();
        fs::copy( compiled, package, fs::copy_options::recursive );
        // Relocate the only source tree so accidental source fallback cannot pass.
        fs::rename( source, folder / "authoring-offline" );
        ResourceSystem runtime( nullptr, nullptr );
        RuntimeResourceConfig mount;
        mount.compiledRoot = path( package );
        mount.manifestPath = path( package / "terrain.wprs" );
        mount.target = config.target;
        require( runtime.initializeRuntime( mount, error ), error );
        auto loaded = runtime.load( id, error );
        require( loaded && loaded->dependencies.size() == 13,
                 "Source-free terrain must install all twelve layer textures and the detail normal: " + error );
        auto scheduler = WorldStreamingScheduler::shared();
        WorldStreamProducerLimits producerLimits;
        producerLimits.maximumPending = producerLimits.maximumActive = producerLimits.maximumReady = 1;
        producerLimits.maximumReadyBytes = 64ull << 20;
        auto producer = scheduler->createProducer( producerLimits, error );
        require( producer != nullptr, error );
        const auto budgetBefore = scheduler->stats();
        CookedTerrainDecodeOptions decodeOptions;
        decodeOptions.key = 71;
        require( queueCookedTerrainDecode( *producer, loaded, config.target, true, decodeOptions, error ), error );
        const auto acceptedGeneration = producer->stats().generation;
        auto tooSmall = decodeOptions;
        tooSmall.maximumWorkingBytes = 1;
        require( !queueCookedTerrainDecode( *producer, loaded, config.target, true, tooSmall, error ) &&
                     producer->stats().generation == acceptedGeneration,
                 "a decode outside its declared byte budget must preserve the accepted generation" );
        WorldStreamResult decodeResult;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 10 );
        while( !producer->take( decodeResult ) )
        {
            require( std::chrono::steady_clock::now() < deadline, "Shared cooked decoder timed out" );
            std::this_thread::yield();
        }
        require( decodeResult.key == 71 && decodeResult.generation == acceptedGeneration,
                 "shared cooked decoder must return the requested generation" );
        auto decoded = std::static_pointer_cast<const DecodedCookedTerrain>( decodeResult.payload );
        const auto decodeError = decodeResult.error;
        const auto reservedDecodeBytes = decodeResult.bytes;
        decodeResult.payload.reset();
        producer->drain();
        require( decoded && scheduler->stats().used[size_t(StreamMemoryClass::ResultCpu)] >=
                     budgetBefore.used[size_t(StreamMemoryClass::ResultCpu)] + reservedDecodeBytes &&
                     scheduler->stats().used[size_t(StreamMemoryClass::SourceCpu)] >
                     budgetBefore.used[size_t(StreamMemoryClass::SourceCpu)],
                 "typed cooked candidate must retain shared result and immutable source reservations" );
        require( decoded && decoded->data.tiles.size() == 1 && decoded->textures.size() == 13 &&
                     decoded->data.terrain.appearance == authored.appearance &&
                     decoded->data.terrain.surfaceMasks == authored.surfaceMasks,
                 "CPU worker must validate the cooked tile manifest and all texture payloads: " +
                     decodeError );
        require( loaded->header.compilerVersion == TerrainResourceCompiler().versionFor( id.type() ),
                 "terrain compiler must emit versioned tile manifests" );
        const auto number = [&]( size_t offset ) {
            u32 value = 0;
            for( size_t i = 0; i < 4; ++i )
                value |= u32( loaded->payload.at( offset + i ) ) << ( i * 8 );
            return value;
        };
        size_t manifest = 16;
        manifest += 4 + number( manifest );
        manifest += 4 + number( manifest );
        const size_t manifestBytes = 20 + size_t( number( manifest + 16 ) ) * 64;
        auto badBounds = std::make_shared<RuntimeResource>( *loaded );
        badBounds->payload.at( manifest + 40 ) ^= 1;
        badBounds->header.payloadHash =
            hashBytes( badBounds->payload.data(), badBounds->payload.size() );
        require( !decodeCookedTerrainResource( badBounds, config.target, true, error ),
                 "valid-hash forged tile bounds must reject against authoritative source samples" );
        struct LegacyVersion final : IResourceCompiler
        {
            u32 version = 1;
            String name() const override { return "TerrainHistoricalVersionFixture"; }
            Array<CompilerOutput> outputs() const override { return {{ResourceTypeID("terrain"),version}}; }
            CompilationStatus compile(const CompileContext &,std::ostream &,Array<String> &) const override
            { return CompilationStatus::Failure; }
        };
        const auto makeLegacy = [&]( u32 version ) {
            auto legacy = std::make_shared<RuntimeResource>( *loaded );
            legacy->dependencies.clear(); legacy->header.installDependencies.clear();
            legacy->header.installArtifacts.clear();
            for( size_t i = 0; i < loaded->dependencies.size(); ++i )
            {
                const auto &dependency = loaded->dependencies[i];
                if( dependency->header.resourceId.str() != authored.appearance.detailNormal )
                {
                    legacy->dependencies.push_back( dependency );
                    legacy->header.installDependencies.push_back( dependency->header.resourceId );
                    legacy->header.installArtifacts.push_back( loaded->header.installArtifacts.at( i ) );
                }
            }
            auto historicalData = authored;
            historicalData.appearance = {};
            historicalData.surfaceMasks.clear();
            auto json = serializeTerrainData( historicalData );
            json.replace( json.find( "\"version\":3" ), 11, "\"version\":2" );
            legacy->payload.assign( loaded->payload.begin(), loaded->payload.begin() + 8 );
            const auto number = [&]( u64 value, size_t bytes ) {
                for( size_t i = 0; i < bytes; ++i ) legacy->payload.push_back( u8( value >> ( i * 8 ) ) );
            };
            const auto string = [&]( const String &value ) {
                number( value.size(), 4 ); legacy->payload.insert( legacy->payload.end(), value.begin(), value.end() );
            };
            number( version, 4 ); number( 1, 4 ); string( config.target ); string( json );
            if( version == 2 ) legacy->payload.insert( legacy->payload.end(), loaded->payload.begin() + manifest,
                                                       loaded->payload.begin() + manifest + manifestBytes );
            number( legacy->dependencies.size(), 4 );
            for( const auto &dependency : legacy->dependencies )
            {
                string( dependency->header.resourceId.str() ); number( dependency->header.compilerVersion, 8 );
                number( dependency->header.sourceHash, 8 ); number( dependency->header.payloadHash, 8 );
            }
            LegacyVersion historical; historical.version = version;
            legacy->header.compilerVersion = historical.versionFor( id.type() );
            legacy->header.payloadSize = legacy->payload.size();
            legacy->header.payloadHash = hashBytes( legacy->payload.data(), legacy->payload.size() );
            require( legacy->header.isValid(), "synthetic historical terrain must retain matching install artifact pins" );
            return legacy;
        };
        auto legacy = makeLegacy( 1 );
        auto migrated = decodeCookedTerrainResource( legacy, config.target, true, error );
        require( migrated != nullptr, "v1 cooked terrain migration decode failed: " + error );
        auto migratedTiles = decodeCookedTerrainResource( makeLegacy( 2 ), config.target, true, error );
        require( migratedTiles != nullptr, "v2 cooked terrain migration decode failed: " + error );
        require( migrated && migratedTiles && migrated->data.tiles.size() == 1 &&
                     migratedTiles->data.terrain.appearance == TerrainAppearance{} &&
                     migrated->data.terrain.appearance == TerrainAppearance{} &&
                     migratedTiles->data.terrain.surfaceMasks.empty(),
                 "v1/v2 cooked terrain migrates tile metadata and retains inert appearance defaults" );
        const auto beforeUpload = scheduler->stats();
        constexpr auto gpuClass = size_t( StreamMemoryClass::Gpu );
        struct RestoreGpu
        {
            std::shared_ptr<WorldStreamingScheduler> scheduler;
            u64 limit;
            ~RestoreGpu() { String ignored; scheduler->setBudget( StreamMemoryClass::Gpu, limit, ignored ); }
        } restoreGpu{ scheduler, beforeUpload.limits[gpuClass] };
        require( scheduler->setBudget( StreamMemoryClass::Gpu, beforeUpload.used[gpuClass], error ), error );
        require( !stageCookedTerrainMaterial( renderer, decoded, error ) &&
                     scheduler->stats().used[gpuClass] == beforeUpload.used[gpuClass],
                 "cooked texture uploads must reject before allocation when their GPU budget is exhausted" );
        require( scheduler->setBudget( StreamMemoryClass::Gpu, beforeUpload.limits[gpuClass], error ), error );
        u64 cookedGpuBytes = 0;
        for( const auto &texture : decoded->textures )
            for( const auto &level : texture.levels ) cookedGpuBytes += level.bgra.size();
        auto candidate = stageCookedTerrainMaterial( renderer, decoded, error );
        require( candidate != nullptr, error );
        require( scheduler->stats().used[gpuClass] == beforeUpload.used[gpuClass] + cookedGpuBytes,
                 "all cooked mip bytes must be reserved while their GPU owners are alive" );
        auto escapedTexture = candidate->resolver( authored.appearance.detailNormal, error );
        require( escapedTexture != nullptr, error );
        auto terrain = make_ptr<ClawTerrain>();
        require( terrain->applyCookedTerrain( candidate, error ), error );
        auto target = make_ptr<ClawRenderTarget>();
        target->setSize( { 128, 128 } );
        renderer.setRenderTarget( target );
        renderer.setViewport( nullptr );
        auto camera = make_ptr<ClawCamera>();
        camera->setFOVy( 1.04719755f );
        camera->setNearClipDistance( .1f );
        camera->setFarClipDistance( 10 );
        camera->setAspectRatio( 1 );
        wp_camera_set_position( camera->getNativeCamera(), { 0, 3, 0 } );
        wp_camera_look_at( camera->getNativeCamera(), { 0, 0, 0 }, { 0, 0, -1 } );
        renderer.setCamera( camera );
        renderer.disableShadows();
        renderer.setSceneLighting( ColourF::White, { 0, -1, 0 }, ColourF::White, 0 );
        renderer.setSceneFog( 0, ColourF::Black, 0, 0, 1 );
        renderer.clear( ColourF::Black );
        renderer.renderTerrain( terrain );
        require( terrain->getTerrainResidency().usedCookedTileManifest,
                 "render-owner preparation must reuse verified cooked tile metadata" );
        auto native = wp_renderer_get_dx11( renderer.getNativeRenderer() );
        const auto output = claw_foliage_contracts::pixels( native );
        size_t red = 0, green = 0, blue = 0, yellow = 0;
        for( size_t p = 0; p < output.size(); p += 4 )
        {
            const auto r = output[p], g = output[p + 1], b = output[p + 2];
            red += r > g + 35 && r > b + 35;
            green += g > r + 35 && g > b + 35;
            blue += b > r + 35 && b > g + 35;
            yellow += r > 80 && g > 80 && b + 35 < std::min( r, g );
        }
        require( red > 25 && green > 25 && blue > 25 && yellow > 25,
                 "Packaged terrain's four PBR layers must all reach actual DX11 pixels" );
        auto noDetail = authored;
        noDetail.appearance.detailStrength = 0;
        require( terrain->applyTerrainData( noDetail, error ), error );
        renderer.clear( ColourF::Black ); renderer.renderTerrain( terrain );
        const auto withoutDetail = claw_foliage_contracts::pixels( native );
        size_t detailChanges = 0;
        for( size_t p = 0; p < output.size(); p += 4 )
            detailChanges += std::abs( int(output[p])-int(withoutDetail[p]) ) > 2 ||
                             std::abs( int(output[p+1])-int(withoutDetail[p+1]) ) > 2 ||
                             std::abs( int(output[p+2])-int(withoutDetail[p+2]) ) > 2;
        require( detailChanges > 25, "the thirteenth cooked detail-normal dependency must visibly affect terrain" );
        require( terrain->applyCookedTerrain( candidate, error ), error );
        renderer.clear( ColourF::Black ); renderer.renderTerrain( terrain );
        claw_foliage_contracts::equivalent( output, claw_foliage_contracts::pixels( native ),
                                          "restoring cooked appearance must restore identical pixels" );
        const auto revision = terrain->getTerrainRevision();
        auto corrupt = std::make_shared<RuntimeResource>( *loaded );
        corrupt->payload.back() ^= 1;
        require( !stageCookedTerrainMaterial( renderer, corrupt, config.target, true, error ) &&
                     terrain->getTerrainRevision() == revision,
                 "Corrupt cooked update must preserve the complete visible terrain" );
        require( !stageCookedTerrainMaterial( renderer, loaded, "wrong-target", true, error ),
                 "Wrong-target cooked terrain must reject" );
        auto stale = std::make_shared<RuntimeResource>( *loaded );
        auto staleTexture = std::make_shared<RuntimeResource>( *stale->dependencies[0] );
        ++staleTexture->header.sourceHash;
        stale->dependencies[0] = staleTexture;
        require( !stageCookedTerrainMaterial( renderer, stale, config.target, true, error ),
                 "Changed texture generation must reject despite otherwise valid terrain payload" );
        runtime.clearRuntimeCache();
        loaded.reset();
        candidate.reset();
        decoded.reset();
        migrated.reset();
        migratedTiles.reset();
        legacy.reset();
        badBounds.reset();
        corrupt.reset();
        stale.reset();
        staleTexture.reset();
        renderer.clear( ColourF::Black );
        renderer.renderTerrain( terrain );
        claw_foliage_contracts::equivalent(
            output, claw_foliage_contracts::pixels( native ),
            "Terrain must retain GPU layers after caller handles/cache release" );
        std::printf(
            "Source-free cooked terrain: 13 pinned textures; visible red=%zu green=%zu blue=%zu "
            "yellow=%zu; failure/lifetime checks PASS\n",
            red, green, blue, yellow );
        terrain->unload( nullptr );
        require( scheduler->stats().used[gpuClass] == beforeUpload.used[gpuClass] + cookedGpuBytes,
                 "a texture returned by the resolver must retain its upload budget beyond terrain unload" );
        escapedTexture = nullptr;
        require( scheduler->stats().used[size_t(StreamMemoryClass::ResultCpu)] ==
                     budgetBefore.used[size_t(StreamMemoryClass::ResultCpu)] &&
                     scheduler->stats().used[size_t(StreamMemoryClass::SourceCpu)] ==
                     budgetBefore.used[size_t(StreamMemoryClass::SourceCpu)] &&
                     scheduler->stats().used[gpuClass] == beforeUpload.used[gpuClass],
                 "last cooked texture/terrain destruction must release shared decode/source/GPU budgets" );
        colourTransfer( renderer );
        runtime.shutdown();
    }
}  // namespace cooked_terrain_contracts
