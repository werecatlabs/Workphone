#pragma once

#include "ClawFoliageWorldContracts.hpp"
#include "ClawFoliageMatchedAcceptance.hpp"
#include "DX11TestEvidence.hpp"
#include <Workphone/Scene/Components/Terrain/FoliagePlantGenerator.hpp>
#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <dxgi1_4.h>

namespace claw_foliage_acceptance
{
    using namespace claw_foliage_contracts;
    using Clock = std::chrono::steady_clock;
    constexpr u32 width = 1920, height = 1080;
    constexpr u32 cells = 8, treesPerCell = 32, grassPerCell = 144;
    constexpr f32 cellWidth = 64;
    constexpr u64 seed = 0x464f4c31;

    inline FoliageSnapshot forest()
    {
        auto data = std::make_shared<FoliageWorldData>();
        data->pageSize = cellWidth;
        data->revision = seed;
        String error;
        for( u32 kind = 0; kind < 3; ++kind )
        {
            scene::FoliagePlantRecipe recipe;
            const bool grass = kind == 2;
            recipe.speciesId = grass ? "acceptance-grass" : kind ? "acceptance-pine-b" : "acceptance-pine-a";
            recipe.seed = u32( seed ) + kind;
            recipe.kind = grass ? FoliageKind::Grass : FoliageKind::Tree;
            recipe.height = grass ? .65f : 7.f + kind;
            recipe.radius = grass ? .45f : 2.f;
            recipe.trunkRadius = grass ? .02f : .17f;
            recipe.branches = grass ? 8 : 16;
            recipe.bark.tint = { .35f, .20f, .11f, 1.f };
            recipe.leaves.tint = grass ? ColourF( .50f, .75f, .22f, 1.f ) : ColourF( .30f, .64f, .28f, 1.f );
            recipe.leaves.albedo = "acceptance-leaf";
            recipe.wind.amplitude = grass ? .11f : .27f;
            recipe.wind.flutter = .03f;
            recipe.wind.bendEnd = recipe.height;
            auto species = std::make_shared<FoliageSpecies>();
            require( scene::generateFoliagePlant( recipe, *species, error ), error.c_str() );
            species->maximumDistance = grass ? 128.f : 480.f;
            species->shadowDistance = grass ? 52.f : 160.f;
            if( grass ) species->densityFractions = { 1.f, .65f, .4f };
            data->species.push_back( std::move( species ) );
        }
        u64 id = 1;
        for( s32 z = -s32( cells ) / 2; z < s32( cells ) / 2; ++z )
            for( s32 x = -s32( cells ) / 2; x < s32( cells ) / 2; ++x )
                for( u32 plant = 0; plant < treesPerCell + grassPerCell; ++plant, ++id )
                {
                    const auto random = [&]( u64 lane ) {
                        return f32( foliageStableHash( seed + id * 7 + lane ) >> 40 ) / 16777216.f;
                    };
                    FoliageInstance instance;
                    instance.id = id;
                    instance.seed = u32( foliageStableHash( seed ^ id ) );
                    instance.species = plant >= treesPerCell ? 2 : plant % 2;
                    instance.position = { ( x + random( 0 ) ) * cellWidth, 0.f,
                                          ( z + random( 1 ) ) * cellWidth };
                    instance.position.y = 2.f * std::sin( instance.position.x * .018f ) *
                                          std::cos( instance.position.z * .016f );
                    const auto scale = .65f + random( 2 ) * .65f;
                    instance.scale = { scale, scale, scale };
                    const auto halfAngle = random( 3 ) * 3.14159265f;
                    instance.rotation = QuaternionF( std::cos( halfAngle ), 0, std::sin( halfAngle ), 0 );
                    data->instances.push_back( instance );
                }
        require( validateFoliageWorld( *data, error ), error.c_str() );
        return data;
    }

    class Camera final : public ClawCamera
    {
    public:
        Matrix4<real_Num> getViewMatrix() const override { return Matrix4<real_Num>( m_view.ptr() ); }
        Matrix4<real_Num> getProjectionMatrix() const override { return Matrix4<real_Num>( m_projection.ptr() ); }

        ClawFoliageView configure( u64 key, const Vector3F &position, const Vector3F &look )
        {
            const auto forward = ( look - position ).normaliseCopy();
            const auto right = forward.crossProduct( { 0, 1, 0 } ).normaliseCopy();
            const auto up = right.crossProduct( forward );
            const Vector3F rows[] = { right, up, -forward };
            m_view = Matrix4F::identity();
            for( int row = 0; row < 3; ++row )
            {
                for( int col = 0; col < 3; ++col ) m_view[row][col] = rows[row][col];
                m_view[row][3] = -rows[row].dotProduct( position );
            }
            setNearClipDistance( .25f );
            setFarClipDistance( 480.f );
            m_projection.makePerspective( 1.04719755f, f32( width ) / height, .25f, 480.f );
            ClawFoliageView view;
            view.key = key;
            view.worldToClip = m_projection * m_view;
            view.position = position;
            view.projectionScale = m_projection[1][1] * .5f;
            return view;
        }

    private:
        Matrix4F m_view, m_projection;
    };

    inline SmartPtr<ClawTexture> leafTexture( wp_renderer_dx11 *native )
    {
        auto texture = make_ptr<ClawTexture>();
        TextureMipLevel level;
        level.width = level.height = 32;
        level.bgra.resize( level.width * level.height * 4 );
        for( u32 y = 0; y < level.height; ++y )
            for( u32 x = 0; x < level.width; ++x )
            {
                const f32 t = ( y + .5f ) / level.height;
                const bool covered = std::abs( f32( x ) - 15.5f ) < 4.f + 13.f * t;
                auto *pixel = level.bgra.data() + ( y * level.width + x ) * 4;
                pixel[0] = 140; pixel[1] = 212; pixel[2] = 165; pixel[3] = covered ? 255 : 0;
            }
        TextureMipSettings settings;
        settings.filter = TextureMipFilter::Cutout;
        String error;
        const auto levels = generateTextureMips( level.bgra.data(), level.width, level.height, settings );
        const Array<TextureMipLevel> mips( levels.begin(), levels.end() );
        require( texture->uploadCookedMips( mips, settings, native, error ), error.c_str() );
        return texture;
    }

    class Measurement final
    {
    public:
        explicit Measurement( wp_renderer_dx11 *native ) :
            m_device( static_cast<ID3D11Device *>( wp_renderer_dx11_get_device( native ) ) ),
            m_context( static_cast<ID3D11DeviceContext *>( wp_renderer_dx11_get_context( native ) ) )
        {
            m_clock = makeQuery( D3D11_QUERY_TIMESTAMP_DISJOINT );
            m_start = makeQuery( D3D11_QUERY_TIMESTAMP );
            m_finish = makeQuery( D3D11_QUERY_TIMESTAMP );
            m_pipeline = makeQuery( D3D11_QUERY_PIPELINE_STATISTICS );
        }

        void begin()
        {
            m_context->Begin( m_clock.Get() );
            m_context->End( m_start.Get() );
        }

        void beginCoverage() { m_context->Begin( m_pipeline.Get() ); }
        void endCoverage() { m_context->End( m_pipeline.Get() ); }

        double finish()
        {
            m_context->End( m_finish.Get() );
            m_context->End( m_clock.Get() );
            m_context->Flush();
            const auto clock = read<D3D11_QUERY_DATA_TIMESTAMP_DISJOINT>( m_clock.Get() );
            const auto start = read<UINT64>( m_start.Get() );
            const auto end = read<UINT64>( m_finish.Get() );
            return clock.Disjoint || !clock.Frequency || end < start ? -1.0 :
                   1000.0 * double( end - start ) / double( clock.Frequency );
        }

        u64 pixelShaderInvocations() { return read<D3D11_QUERY_DATA_PIPELINE_STATISTICS>( m_pipeline.Get() ).PSInvocations; }

    private:
        ComPtr<ID3D11Query> makeQuery( D3D11_QUERY kind )
        {
            ComPtr<ID3D11Query> query;
            const D3D11_QUERY_DESC desc{ kind, 0 };
            require( SUCCEEDED( m_device->CreateQuery( &desc, query.GetAddressOf() ) ),
                     "DX11 foliage acceptance queries must be available" );
            return query;
        }

        template <class T> T read( ID3D11Query *query )
        {
            T value{};
            const auto deadline = Clock::now() + std::chrono::seconds( 20 );
            HRESULT status;
            while( ( status = m_context->GetData( query, &value, sizeof( value ),
                                                 D3D11_ASYNC_GETDATA_DONOTFLUSH ) ) == S_FALSE &&
                   Clock::now() < deadline )
                std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
            require( status == S_OK, "DX11 foliage acceptance query timed out or failed" );
            return value;
        }

        ID3D11Device *m_device;
        ID3D11DeviceContext *m_context;
        ComPtr<ID3D11Query> m_clock, m_start, m_finish, m_pipeline;
    };

    inline double milliseconds( Clock::duration duration )
    {
        return std::chrono::duration<double, std::milli>( duration ).count();
    }

    inline ComPtr<IDXGIAdapter1> adapter( wp_renderer_dx11 *native )
    {
        auto *device = static_cast<ID3D11Device *>( wp_renderer_dx11_get_device( native ) );
        ComPtr<IDXGIDevice> dxgi;
        ComPtr<IDXGIAdapter> base;
        ComPtr<IDXGIAdapter1> result;
        require( SUCCEEDED( device->QueryInterface( IID_PPV_ARGS( dxgi.GetAddressOf() ) ) ) &&
                 SUCCEEDED( dxgi->GetAdapter( base.GetAddressOf() ) ) && SUCCEEDED( base.As( &result ) ),
                 "DX11 fixture must identify the adapter kind" );
        return result;
    }

    inline void waitForPages( ClawFoliageWorld &world, ClawRendererDX11 &renderer,
                             const Array<ClawFoliageView> &views, f32 seconds, bool expectPages = true )
    {
        String error;
        const auto deadline = Clock::now() + std::chrono::seconds( 15 );
        do
        {
            require( world.prepare( renderer, views, seconds, error ), error.c_str() );
            const auto status = world.diagnostics();
            require( !status.failedPages, "forest page load failed" );
            if( status.currentGenerationReady && !status.pendingPages &&
                ( status.residentPages != 0 ) == expectPages ) return;
            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
        } while( Clock::now() < deadline );
        require( false, "forest residency did not settle before the deadline" );
    }

    inline ClawFoliageDiagnostics renderPass( ClawFoliageWorld &world, ClawRendererDX11 &renderer,
                                             wp_renderer_dx11 *native, const ClawFoliageView &view )
    {
        String error;
        const auto before = statistics( native );
        require( world.render( renderer, view, error ), error.c_str() );
        const auto after = statistics( native );
        const auto d = world.diagnostics();
        require( d.viewKey == view.key && d.actualDraws == d.predictedDraws &&
                 d.actualDraws == after.draws - before.draws &&
                 d.uploadBytes == after.instance_upload_bytes - before.instance_upload_bytes,
                 "forest pass counters must match actual native draws and uploads" );
        return d;
    }

    inline size_t capture( wp_renderer_dx11 *native, const std::filesystem::path &path = {} )
    {
        const auto viewport = wp_renderer_dx11_get_viewport( native );
        require( viewport.x == 0 && viewport.y == 0 && viewport.width == s32( width ) &&
                 viewport.height == s32( height ), "forest viewport must cover the entire target" );
        auto image = pixels( native );
        require( image.size() == size_t( width ) * height * 4, "forest capture must match the locked target" );
        size_t covered = 0;
        for( size_t p = 0; p < image.size(); p += 4 ) covered += image[p] || image[p + 1] || image[p + 2];
        if( !path.empty() )
        {
            BITMAPFILEHEADER file{};
            BITMAPINFOHEADER info{};
            file.bfType = 0x4d42;
            file.bfOffBits = sizeof( file ) + sizeof( info );
            file.bfSize = file.bfOffBits + DWORD( image.size() );
            info.biSize = sizeof( info );
            info.biWidth = width;
            info.biHeight = -LONG( height );
            info.biPlanes = 1;
            info.biBitCount = 32;
            for( size_t p = 0; p < image.size(); p += 4 ) std::swap( image[p], image[p + 2] );
            std::ofstream out( path, std::ios::binary );
            out.write( reinterpret_cast<const char *>( &file ), sizeof( file ) );
            out.write( reinterpret_cast<const char *>( &info ), sizeof( info ) );
            out.write( reinterpret_cast<const char *>( image.data() ), image.size() );
            out.close();
            require( bool( out ), "forest evidence image must save" );
        }
        return covered;
    }

    inline void flight( ClawFoliageWorld &world, ClawRendererDX11 &renderer,
                        SmartPtr<Camera> camera, const SmartPtr<ClawRenderTarget> &target,
                        u32 frames, u32 soakSeconds, const std::filesystem::path &captureDirectory )
    {
        auto *native = wp_renderer_get_dx11( renderer.getNativeRenderer() );
        Measurement measure( native );
        ComPtr<IDXGIAdapter3> memory;
        adapter( native ).As( &memory );
        renderer.setRenderTarget( target );
        renderer.setViewport( nullptr );
        renderer.setCamera( camera );
        renderer.setSceneLighting( ColourF::White, { -.25f, -1.f, .35f }, ColourF::White, .8f );
        String error;
        const auto start = Clock::now();
        const auto deadline = start + std::chrono::seconds( soakSeconds );
        const auto scheduler = WorldStreamingScheduler::shared();
        u64 maxCpu = 0, maxGpu = 0, maxUpload = 0, maxLocalVideoMemory = 0;
        u64 frame = 0;
        u32 observedPasses = 0, observedLods = 0;
        for( ; frame < frames || Clock::now() < deadline; ++frame )
        {
            // Cross the same terrain at grazing height; periodically jump to the other
            // end and reverse direction to exercise eviction and independent view history.
            const auto step = f32( frame % 32 ) / 31.f;
            const f32 direction = ( frame / 32 ) % 2 ? -1.f : 1.f;
            const Vector3F eye{ -165.f * direction + 180.f * direction * step, 3.5f, -190.f * direction };
            const Vector3F look{ eye.x + 20.f * direction, .8f, eye.z + 95.f * direction };
            const auto colour = camera->configure( 1, eye, look );
            auto shadow = colour;
            shadow.key = 2;
            shadow.shadow = true;
            auto reflectedEye = eye, reflectedLook = look;
            reflectedEye.y = -eye.y;
            reflectedLook.y = -look.y;
            const auto reflection = camera->configure( 3, reflectedEye, reflectedLook );
            const Array<ClawFoliageView> views{ colour, shadow, reflection };
            const auto seconds = f32( frame % 65536 ) / 60.f;
            const auto settling = Clock::now();
            if( frame && frame % 32 == 0 )
            {
                auto remote = colour;
                remote.position = { 8192.f, 32.f, 8192.f };
                waitForPages( world, renderer, { remote }, seconds, false );
            }
            if( frame == 0 || frame % 32 == 0 ) waitForPages( world, renderer, views, seconds );
            const auto cpuStart = Clock::now();
            const auto settleMs = frame % 32 == 0 ? milliseconds( cpuStart - settling ) : 0.0;
            require( world.prepare( renderer, views, seconds, error ), error.c_str() );
            const auto prepared = Clock::now();
            measure.begin();
            camera->configure( 1, eye, look );
            require( renderer.beginShadowMap( 1024 ), "forest shadow pass must allocate" );
            shadow.worldToClip = renderer.foliageViewProjection( true );
            const auto s = renderPass( world, renderer, native, shadow );
            renderer.endShadowMap();

            renderer.clear( ColourF::Black );
            measure.beginCoverage();
            const auto c = renderPass( world, renderer, native, colour );
            measure.endCoverage();
            const bool captureCoverage = frame == 0 || frame == frames / 2;

            camera->configure( 3, reflectedEye, reflectedLook );
            renderer.clear( ColourF::Black );
            const auto r = renderPass( world, renderer, native, reflection );
            const auto submitted = Clock::now();
            const auto gpu = measure.finish();
            const auto psInvocations = measure.pixelShaderInvocations();
            size_t covered = 0;
            if( captureCoverage )
            {
                const auto path = [&]( const char *view ) {
                    return captureDirectory.empty() ? std::filesystem::path{} :
                        captureDirectory / ( std::string( view ) + "-" + std::to_string( frame ) + ".bmp" );
                };
                require( capture( native, path( "reflection" ) ) > 100,
                         "the independently projected reflected view must cover real pixels" );
                camera->configure( 1, eye, look );
                renderer.clear( ColourF::Black );
                renderPass( world, renderer, native, colour );
                covered = capture( native, path( "colour" ) );
            }
            observedPasses |= ( c.actualDraws ? 1u : 0u ) | ( s.actualDraws ? 2u : 0u ) |
                              ( r.actualDraws ? 4u : 0u );
            for( u32 lod = 0; lod < c.lodInstances.size(); ++lod )
                if( c.lodInstances[lod] ) observedLods |= 1u << lod;
            require( !c.failedPages && !s.failedPages && !r.failedPages, "forest page failed during flight" );
            if( captureCoverage ) require( covered > 100 && psInvocations, "grazing-angle forest must cover real pixels" );
            const auto budget = scheduler->stats();
            const auto gpuClass = size_t( StreamMemoryClass::Gpu );
            maxGpu = std::max( maxGpu, budget.used[gpuClass] );
            maxCpu = std::max( maxCpu, budget.used[size_t( StreamMemoryClass::SourceCpu )] +
                budget.used[size_t( StreamMemoryClass::PendingCpu )] + budget.used[size_t( StreamMemoryClass::WorkingCpu )] );
            maxUpload = std::max( maxUpload, c.uploadBytes + s.uploadBytes + r.uploadBytes );
            DXGI_QUERY_VIDEO_MEMORY_INFO local{};
            if( memory && SUCCEEDED( memory->QueryVideoMemoryInfo( 0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &local ) ) )
                maxLocalVideoMemory = std::max( maxLocalVideoMemory, u64( local.CurrentUsage ) );
            if( frame < frames || frame % 100 == 0 )
            {
                const auto splits = []( const ClawFoliageDiagnostics &d ) {
                    return d.speciesSplits + d.lodSplits + d.sectionSplits + d.capacitySplits + d.policySplits;
                };
                std::printf( "FOLIAGE_FRAME,%llu,%.6f,%.6f,%.6f,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%u,%llu,%llu,%.6f\n",
                    static_cast<unsigned long long>( frame ), milliseconds( prepared - cpuStart ),
                    milliseconds( submitted - prepared ), gpu,
                    static_cast<unsigned long long>( c.actualDraws ), static_cast<unsigned long long>( s.actualDraws ),
                    static_cast<unsigned long long>( r.actualDraws ),
                    static_cast<unsigned long long>( c.submittedTriangles + s.submittedTriangles + r.submittedTriangles ),
                    static_cast<unsigned long long>( c.uploadBytes + s.uploadBytes + r.uploadBytes ),
                    static_cast<unsigned long long>( psInvocations ), static_cast<unsigned long long>( covered ),
                    c.residentPages, static_cast<unsigned long long>( c.residentInstances ),
                    static_cast<unsigned long long>( splits( c ) + splits( s ) + splits( r ) ), settleMs );
                std::fflush( stdout );
            }
        }
        require( observedPasses == 7, "forest must submit all three passes" );
        require( observedLods == 7, "the automatic forest view must submit all three LODs" );
        std::printf( "FOLIAGE_FLIGHT,%llu,%.3f,%llu,%llu,%llu,%llu\n", static_cast<unsigned long long>( frame ),
            milliseconds( Clock::now() - start ) / 1000.0, static_cast<unsigned long long>( maxCpu ),
            static_cast<unsigned long long>( maxGpu ), static_cast<unsigned long long>( maxUpload ),
            static_cast<unsigned long long>( maxLocalVideoMemory ) );
        renderer.disableShadows();
    }

    inline void grassFlight( ClawRendererDX11 &renderer, const FoliageSnapshot &forest,
                             SmartPtr<Camera> camera, const SmartPtr<ClawTexture> &texture,
                             const std::filesystem::path &captureDirectory )
    {
        auto grass = std::make_shared<FoliageWorldData>();
        grass->pageSize = forest->pageSize;
        grass->revision = forest->revision;
        grass->species = { forest->species[2] };
        for( auto instance : forest->instances )
        {
            if( instance.species != 2 ) continue;
            instance.species = 0;
            grass->instances.push_back( instance );
        }
        auto *native = wp_renderer_get_dx11( renderer.getNativeRenderer() );
        Measurement measure( native );
        ClawFoliageWorld world;
        String error;
        require( world.setSource( grass, [&]( const String &, String & ) -> SmartPtr<ITexture> { return texture; }, error ),
                 error.c_str() );
        constexpr u32 samples = 24;
        for( u32 frame = 0; frame < samples; ++frame )
        {
            const auto &anchor = grass->instances[frame * 313 % grass->instances.size()].position;
            const auto view = camera->configure( 4, anchor + Vector3F( -3.f, .9f, -8.f ),
                                                anchor + Vector3F( 0.f, .3f, 32.f ) );
            waitForPages( world, renderer, { view }, f32( frame ) / 60.f );
            measure.begin();
            renderer.clear( ColourF::Black );
            measure.beginCoverage();
            const auto draws = renderPass( world, renderer, native, view ).actualDraws;
            measure.endCoverage();
            const auto gpu = measure.finish();
            const auto invocations = measure.pixelShaderInvocations();
            const auto output = captureDirectory.empty() || frame != 0 ? std::filesystem::path{} :
                                captureDirectory / "grass-0.bmp";
            const auto covered = frame == 0 || frame == samples - 1 ? capture( native, output ) : 0;
            std::printf( "FOLIAGE_GRASS,%u,%.6f,%llu,%llu,%llu\n", frame, gpu,
                static_cast<unsigned long long>( draws ), static_cast<unsigned long long>( invocations ),
                static_cast<unsigned long long>( covered ) );
            require( draws && invocations && ( ( frame != 0 && frame != samples - 1 ) || covered > 32 ),
                     "grazing grass must submit real draws and cover visible pixels" );
        }
    }

    inline void run( ClawRendererDX11 &renderer, const SmartPtr<IGraphicsWindow> &window,
                     u32 frames, u32 soakSeconds, const std::filesystem::path &captureDirectory )
    {
        claw_foliage_scene_contracts::Fixture services;
        auto *native = wp_renderer_get_dx11( renderer.getNativeRenderer() );
        DXGI_ADAPTER_DESC1 description{};
        require( SUCCEEDED( adapter( native )->GetDesc1( &description ) ), "DX11 adapter flags must be available" );
        // Some WARP versions report no DXGI_ADAPTER_FLAG_SOFTWARE on the D3D11
        // device's adapter. Also reject the Microsoft Basic Render Driver ID.
        const bool basicRenderDriver = description.VendorId == 0x1414 && description.DeviceId == 0x008c;
        const bool hardware = !basicRenderDriver &&
            !( description.Flags & ( DXGI_ADAPTER_FLAG_SOFTWARE | DXGI_ADAPTER_FLAG_REMOTE ) );
        std::printf( "FOLIAGE_ADAPTER,%s\n", hardware ? "physical" : "software-or-remote" );
        require( hardware || soakSeconds <= 60, "the release-duration foliage soak requires a physical DX11 adapter" );
        const auto data = forest();
        std::printf( "FOLIAGE_FIXTURE,1,%llu,%u,%u,%u,%u,%u,%u\n", static_cast<unsigned long long>( seed ),
                     width, height, cells * cells, cells * cells * treesPerCell, cells * cells * grassPerCell, frames );
        auto texture = leafTexture( native );
        auto target = make_ptr<ClawRenderTarget>();
        target->setSize( { s32( width ), s32( height ) } );
        auto camera = make_ptr<Camera>();
        renderer.setRenderTarget( target );
        renderer.setViewport( nullptr );
        claw_foliage_matched_acceptance::run( renderer, data, camera, texture );
        ClawFoliageWorld world;
        String error;
        require( world.setSource( data, [&]( const String &, String & ) -> SmartPtr<ITexture> { return texture; }, error ),
                 error.c_str() );
        grassFlight( renderer, data, camera, texture, captureDirectory );
        flight( world, renderer, camera, target, frames, soakSeconds, captureDirectory );

        // Force the actual native owner to be torn down, then reuse the same world
        // and retained source. No software simulation of device replacement.
        renderer.setRenderTarget( nullptr );
        renderer.setCamera( nullptr );
        renderer.unload( nullptr );
        renderer.load( window );
        require( renderer.isLoaded(), "foliage recovery must rebuild the native renderer" );
        native = wp_renderer_get_dx11( renderer.getNativeRenderer() );
        require( recordDX11TestDevice( static_cast<ID3D11Device *>( wp_renderer_dx11_get_device( native ) ) ),
                 "recovered renderer must identify its device" );
        target = make_ptr<ClawRenderTarget>();
        target->setSize( { s32( width ), s32( height ) } );
        renderer.setRenderTarget( target );
        renderer.setViewport( nullptr );
        renderer.setCamera( camera );
        const auto view = camera->configure( 1, { -165.f, 3.5f, -190.f }, { -145.f, .8f, -95.f } );
        waitForPages( world, renderer, { view }, 0 );
        renderer.clear( ColourF::Black );
        require( renderPass( world, renderer, native, view ).actualDraws && capture( native ) > 100,
                 "world must render retained foliage on the new native device" );
        std::puts( "FOLIAGE_RECOVERY,passed" );
        renderer.setRenderTarget( nullptr );
        renderer.setCamera( nullptr );
    }
}
