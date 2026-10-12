#include <Workphone/Workphone.hpp>
#include <Workphone/Graphics/ProceduralSurfaceSourceExport.hpp>
#include <Workphone/Interface/Procedural/TextureForgeTypes.hpp>
#if WP_COOKED_PROCEDURAL
#include <WPProcedural/WPProcedural.hpp>
#include <Workphone/Interface/Procedural/ITextureForge.hpp>
#endif
#include <WPGraphics/Resources/ClawMaterialResource.hpp>
#include <WPGraphics/Resources/GraphicsResourceCompiler.hpp>
#include <WPGraphics/Resources/GraphicsResourceFormats.hpp>
#include <WPGraphics/ClawMesh.hpp>
#include <WPGraphics/ClawRenderTarget.hpp>
#include <Workphone/Core/LogManagerDefault.hpp>
#include <Workphone/System/Resource.hpp>
#include <Workphone/System/ResourceCompilerRegistry.hpp>
#include <Workphone/System/ResourceSystem.hpp>
#include <Workphone/System/StateManager.hpp>
#include <Workphone/System/TimerMT.hpp>
#include <WPSQLite/WPSQLite.hpp>
#include <WPSQLite/ResourceCompilationDatabase.hpp>
#include <workphone_graphics_mesh.h>
#include "ClawTextureMipContracts.hpp"
#include "DX11TestEvidence.hpp"
#include "GraphicsResourceCompilerContracts.hpp"
#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace workphone
{
    class CookedMaterialTestAsset : public Resource<IResource>
    {
    public:
        WP_CLASS_REGISTER_DECL;
    };
    WP_CLASS_REGISTER_DERIVED( workphone, CookedMaterialTestAsset, Resource<IResource> );
    class CookedTextureTestAsset : public Resource<IResource>
    {
    public:
        WP_CLASS_REGISTER_DECL;
    };
    WP_CLASS_REGISTER_DERIVED( workphone, CookedTextureTestAsset, Resource<IResource> );
}  // namespace workphone

namespace
{
    using namespace workphone;
    using namespace workphone::render;
    using namespace workphone::resource;
    using Microsoft::WRL::ComPtr;
    namespace fs = std::filesystem;

    void require( bool condition, const char *message )
    {
        if( !condition )
            throw std::runtime_error( message );
    }

    void require( bool condition, const String &message )
    {
        if( !condition )
            throw std::runtime_error( message.c_str() );
    }

    String pathText( const fs::path &path )
    {
        return path.generic_u8string().c_str();
    }

    void writeText( const fs::path &path, const char *text )
    {
        std::ofstream stream( path, std::ios::binary | std::ios::trunc );
        stream << text;
        require( stream.good(), "Generated descriptor must be writable" );
    }

    const char *textureDescriptor =
        R"({"version":1,"source":"data://albedo.tga","mipFilter":"colour","atlasColumns":1,"alphaCutoff":0.5})";
    const char *materialDescriptor =
        R"({"version":1,"baseColour":[1,1,1,1],"metalness":0,"roughness":1,"baseColourTexture":"data://albedo.texres"})";

    // Generated uncompressed top-down BGRA TGA; no external image/media fixtures.
    // Asymmetric intensities make both orientation and the filtered mip data observable.
    void writeImage( const fs::path &path, bool green )
    {
        std::array<unsigned char, 18 + 4 * 4 * 4> bytes{};
        bytes[2] = 2;
        bytes[12] = bytes[14] = 4;
        bytes[16] = 32;
        bytes[17] = 0x28;
        for( unsigned y = 0; y < 4; ++y )
            for( unsigned x = 0; x < 4; ++x )
            {
                const auto offset = 18 + ( y * 4 + x ) * 4;
                bytes[offset + ( green ? 1 : 2 )] = static_cast<unsigned char>( 128 + x * 17 + y * 23 );
                bytes[offset + 3] = 255;
            }
        std::ofstream stream( path, std::ios::binary | std::ios::trunc );
        stream.write( reinterpret_cast<const char *>( bytes.data() ), bytes.size() );
        require( stream.good(), "Generated TGA must be writable" );
    }

    template <class T>
    SmartPtr<T> asset( const String &path )
    {
        auto value = make_ptr<T>();
        value->getHandle()->setUUID( StringUtil::getUUID() );
        value->setFilePath( path );
        return value;
    }

    struct Fixture
    {
        claw_texture_mip_contracts::Fixture graphics;
        SmartPtr<TimerMT> timer;
        SmartPtr<StateManager> states;
        SmartPtr<SQLitePlugin> sqlite;
        SmartPtr<AssetDatabaseManager> catalog;
        std::shared_ptr<ResourceSystem> resources;
        SmartPtr<CookedMaterialTestAsset> materialAsset;
        SmartPtr<CookedTextureTestAsset> textureAsset;
        Array<CatalogResourceAdapter::TypeMapping> mappings;
        fs::path temporary;
        fs::path folder;
        fs::path source;
        String databasePath;
        String uuid;

        void initialize()
        {
            graphics.initialize();
            auto factory = graphics.application->getFactoryManager();
            factory->load( nullptr );
            graphics.application->setLogManager( make_ptr<LogManagerDefault>() );
            graphics.application->setFileSystem( make_ptr<FileSystem>() );
            timer = make_ptr<TimerMT>();
            timer->load( nullptr );
            graphics.application->setTimer( timer );
            require( timer->isLoaded(), "Cooked material fixture requires a loaded state timer" );
            states = make_ptr<StateManager>();
            graphics.application->setStateManager( states );
            states->load( nullptr );
            require( states->isLoaded(), "Cooked material fixture requires the real StateManager" );
            sqlite = make_ptr<SQLitePlugin>();
            sqlite->load( nullptr );
            temporary = fs::temp_directory_path();
            folder = temporary /
                     ( std::string( "workphone_cooked_graphics_" ) + StringUtil::getUUID().c_str() );
            source = folder / "source";
            fs::create_directories( source );
            runGraphicsResourceCompilerContracts( folder );
            writeImage( source / "albedo.tga", false );
            writeText( source / "albedo.texres", textureDescriptor );
            writeText( source / "surface.matres", materialDescriptor );
            catalog = make_ptr<AssetDatabaseManager>();
            require( catalog->setProjectRoot( pathText( source ) ), "Catalog source root must bind" );
            databasePath = pathText( folder / "catalog.db" );
            catalog->loadFromFile( databasePath );
            auto database = catalog->getDatabase();
            require( database && database->isLoaded(), "GPU catalog fixture requires real SQLite" );
            materialAsset = asset<CookedMaterialTestAsset>( "surface.matres" );
            textureAsset = asset<CookedTextureTestAsset>( "albedo.texres" );
            catalog->addResourceEntry( materialAsset );
            catalog->addResourceEntry( textureAsset );
            uuid = materialAsset->getHandle()->getUUIDAsString();
            AssetDatabaseManager::EntrySnapshot material, texture;
            require( catalog->tryGetEntry( uuid, material ) &&
                         catalog->tryGetEntry( textureAsset->getHandle()->getUUIDAsString(), texture ),
                     "Material and texture descriptors must have durable catalog identities" );
            mappings = { { material.type, ResourceTypeID( "matres" ) },
                         { texture.type, ResourceTypeID( "texres" ) } };
            auto registry = std::make_shared<ResourceCompilerRegistry>();
            String error;
            require( registry->registerCompiler( std::make_shared<GraphicsResourceCompiler>(), &error ),
                     "Graphics resource compiler must register" );
            resources = std::make_shared<ResourceSystem>(
                registry, std::make_shared<ResourceCompilationDatabase>() );
            ResourceSystemConfig config;
            config.sourceRoot = pathText( source );
            config.compiledRoot = pathText( folder / "compiled" );
            config.target = "cooked-dx11-contract";
            require( resources->initialize( config, error ), error );
            auto *native = wp_renderer_get_dx11( graphics.renderer->getNativeRenderer() );
            require( native && recordDX11TestDevice( static_cast<ID3D11Device *>(
                                   wp_renderer_dx11_get_device( native ) ) ),
                     "Cooked graphics contract must record its actual DX11 device" );
        }

        ~Fixture()
        {
            materialAsset = nullptr;
            textureAsset = nullptr;
            if( resources )
                resources->shutdown();
            resources.reset();
            if( catalog )
                catalog->unload( nullptr );
            catalog = nullptr;
            if( sqlite )
                sqlite->unload( nullptr );
            sqlite = nullptr;
            if( states )
                states->unload( nullptr );
            if( graphics.application )
            {
                graphics.application->setStateManager( nullptr );
                graphics.application->setFileSystem( nullptr );
                graphics.application->setTimer( nullptr );
            }
            states = nullptr;
            if( timer )
                timer->unload( nullptr );
            timer = nullptr;
            if( !folder.empty() && folder.parent_path() == temporary &&
                folder.filename().string().find( "workphone_cooked_graphics_" ) == 0 )
            {
                std::error_code error;
                fs::remove_all( folder, error );
            }
        }
    };

    class Triangle : public ClawMesh
    {
    public:
        Triangle()
        {
            m_mesh = wp_graphics_mesh_create();
            require( m_mesh != nullptr, "Generated draw mesh must allocate" );
            const wp_graphics_mesh_vertex_pnt vertices[] = {
                { { -.9f, -.9f, .5f }, { 0, 0, -1 }, { 0, 1 } },
                { { .9f, -.9f, .5f }, { 0, 0, -1 }, { 1, 1 } },
                { { 0, .9f, .5f }, { 0, 0, -1 }, { .5f, 0 } }
            };
            const wp_u16 indices[] = { 0, 1, 2 };
            wp_graphics_mesh_set_vertices( m_mesh, WORKPHONE_VERTEX_FORMAT_PNT, vertices, 3 );
            wp_graphics_mesh_set_indices_u16( m_mesh, indices, 3 );
        }
    };

    // Deterministically models another compiler replacing a valid root generation
    // between this consumer's successful compile report and its runtime load.
    class ReplacedGenerationSystem : public IResourceSystem
    {
    public:
        enum class Replacement
        {
            None,
            SourceHash,
            PayloadHash
        };

        explicit ReplacedGenerationSystem( std::shared_ptr<IResourceSystem> delegate ) :
            m_delegate( std::move( delegate ) )
        {
        }

        Replacement replacement = Replacement::None;
        bool compilationSucceeded = false;
        bool validReplacementLoaded = false;

        bool initialize( const ResourceSystemConfig &config, String &error ) override
        {
            return m_delegate->initialize( config, error );
        }
        void shutdown() override
        {
            m_delegate->shutdown();
        }
        bool isInitialized() const override
        {
            return m_delegate->isInitialized();
        }
        CompilationReport compile( const ResourceID &id, const CompilationOptions &options ) override
        {
            const auto report = m_delegate->compile( id, options );
            compilationSucceeded = report.succeeded();
            return report;
        }
        Array<CompilationReport> compileAffected( const String &path,
                                                  const CompilationOptions &options ) override
        {
            return m_delegate->compileAffected( path, options );
        }
        std::shared_ptr<const RuntimeResource> load( const ResourceID &id, String &error ) override
        {
            auto loaded = m_delegate->load( id, error );
            if( !loaded || replacement == Replacement::None )
                return loaded;
            auto changed = std::make_shared<RuntimeResource>( *loaded );
            if( replacement == Replacement::SourceHash )
                ++changed->header.sourceHash;
            else
            {
                CookedMaterialData material;
                require( decodeCookedMaterial( *changed, material, error ), error );
                material.roughness = .75f;
                std::ostringstream output( std::ios::binary );
                require( writeCookedMaterial( material, output, error ), error );
                const auto bytes = output.str();
                changed->payload.resize( bytes.size() );
                std::memcpy( changed->payload.data(), bytes.data(), bytes.size() );
                changed->header.payloadSize = changed->payload.size();
                changed->header.payloadHash =
                    hashBytes( changed->payload.data(), changed->payload.size() );
            }
            CookedMaterialData decoded;
            validReplacementLoaded = decodeCookedMaterial( *changed, decoded, error );
            require( validReplacementLoaded, error );
            return changed;
        }
        void unload( const ResourceID &id ) override
        {
            m_delegate->unload( id );
        }
        void clearRuntimeCache() override
        {
            m_delegate->clearRuntimeCache();
        }
        std::shared_ptr<IResourceCompilerRegistry> registry() const override
        {
            return m_delegate->registry();
        }

    private:
        std::shared_ptr<IResourceSystem> m_delegate;
    };

    void requireGpuMips( const std::shared_ptr<const ClawMaterialResource::Candidate> &candidate,
                         bool green, const procedural::TextureBuffer *generated = nullptr )
    {
        require( candidate && candidate->resource() && candidate->resource()->dependencies.size() == 1,
                 "Published material must retain its installed cooked texture dependency" );
        CookedTextureData cooked;
        String error;
        require( decodeCookedTexture( *candidate->resource()->dependencies[0], cooked, error ), error );
        if( generated )
        {
            require( cooked.levels.size() == 7 && cooked.levels[0].width == generated->width &&
                         cooked.levels[0].height == generated->height && cooked.levels.back().width == 1,
                     "Generated asphalt must retain its full 64x64 semantic mip chain" );
            const auto &base = cooked.levels[0].bgra;
            require( base.size() == generated->pixels.size(), "Generated asphalt byte extent changed" );
            for( size_t i = 0; i < base.size(); i += 4 )
                require( base[i] == generated->pixels[i + 2] &&
                             base[i + 1] == generated->pixels[i + 1] &&
                             base[i + 2] == generated->pixels[i] && base[i + 3] == generated->pixels[i + 3],
                         "Generated asphalt source pixels changed during cook" );
        }
        else
        {
            require( cooked.levels.size() == 3 && cooked.levels[0].width == 4 &&
                         cooked.levels[1].width == 2 && cooked.levels[2].width == 1,
                     "Offline compiler must produce the complete 4x4 colour mip chain" );
            const auto &base = cooked.levels[0].bgra;
            require( base[( green ? 1 : 2 )] == 128 && base[( 3 * 4 + 3 ) * 4 + ( green ? 1 : 2 )] == 248,
                     "Generated TGA orientation and BGRA channel identity must survive cooking" );
        }
        auto texture = candidate->texture();
        auto *view = claw_texture_mip_contracts::view( *texture );
        const auto desc = claw_texture_mip_contracts::description( view );
        require( desc.Format == DXGI_FORMAT_B8G8R8A8_UNORM && desc.MipLevels == cooked.levels.size(),
                 "GPU texture must retain the exact cooked BGRA format and mip count" );
        ComPtr<ID3D11Resource> nativeResource;
        view->GetResource( nativeResource.GetAddressOf() );
        ComPtr<ID3D11Texture2D> nativeTexture;
        require( SUCCEEDED( nativeResource.As( &nativeTexture ) ), "Cooked GPU texture must be 2D" );
        ComPtr<ID3D11Device> device;
        nativeTexture->GetDevice( device.GetAddressOf() );
        ComPtr<ID3D11DeviceContext> context;
        device->GetImmediateContext( context.GetAddressOf() );
        for( unsigned mip = 0; mip < cooked.levels.size(); ++mip )
        {
            const auto &level = cooked.levels[mip];
            auto stagingDesc = desc;
            stagingDesc.Width = level.width;
            stagingDesc.Height = level.height;
            stagingDesc.MipLevels = stagingDesc.ArraySize = 1;
            stagingDesc.Usage = D3D11_USAGE_STAGING;
            stagingDesc.BindFlags = stagingDesc.MiscFlags = 0;
            stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            ComPtr<ID3D11Texture2D> staging;
            require(
                SUCCEEDED( device->CreateTexture2D( &stagingDesc, nullptr, staging.GetAddressOf() ) ),
                "Cooked mip readback must allocate" );
            context->CopySubresourceRegion( staging.Get(), 0, 0, 0, 0, nativeTexture.Get(), mip,
                                            nullptr );
            D3D11_MAPPED_SUBRESOURCE mapped{};
            require( SUCCEEDED( context->Map( staging.Get(), 0, D3D11_MAP_READ, 0, &mapped ) ),
                     "Cooked mip readback must map" );
            bool equal = true;
            for( unsigned y = 0; y < level.height; ++y )
                equal = equal &&
                        std::memcmp(
                            static_cast<const unsigned char *>( mapped.pData ) + y * mapped.RowPitch,
                            level.bgra.data() + y * level.width * 4, level.width * 4 ) == 0;
            context->Unmap( staging.Get(), 0 );
            require( equal, "Every GPU mip byte must match the immutable cooked texture payload" );
        }
    }

    void requireDraw( Fixture &fixture,
                      const std::shared_ptr<const ClawMaterialResource::Candidate> &candidate,
                      bool green, bool asphalt = false )
    {
        require( candidate != nullptr, "A visible cooked material must exist" );
        auto target = make_ptr<ClawRenderTarget>();
        target->setSize( { 32, 32 } );
        auto renderer = fixture.graphics.renderer;
        renderer->setRenderTarget( target );
        renderer->setViewport( nullptr );
        renderer->setCamera( nullptr );
        renderer->setSceneLighting( ColourF::White, Vector3F( 0, -1, 0 ), ColourF::White, 0 );
        Triangle mesh;
        mesh.setMaterial( candidate->material() );
        renderer->clear( ColourF::Black );
        renderer->renderMesh( &mesh, Matrix4F::identity() );
        auto *native = wp_renderer_get_dx11( renderer->getNativeRenderer() );
        auto *device = static_cast<ID3D11Device *>( wp_renderer_dx11_get_device( native ) );
        auto *context = static_cast<ID3D11DeviceContext *>( wp_renderer_dx11_get_context( native ) );
        ComPtr<ID3D11RenderTargetView> rtv;
        context->OMGetRenderTargets( 1, rtv.GetAddressOf(), nullptr );
        require( rtv != nullptr, "Cooked draw must bind its render target" );
        ComPtr<ID3D11Resource> resource;
        rtv->GetResource( resource.GetAddressOf() );
        ComPtr<ID3D11Texture2D> rendered;
        require( SUCCEEDED( resource.As( &rendered ) ), "Cooked draw target must be 2D" );
        D3D11_TEXTURE2D_DESC desc{};
        rendered->GetDesc( &desc );
        const auto format = desc.Format;
        desc.Usage = D3D11_USAGE_STAGING;
        desc.BindFlags = desc.MiscFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        ComPtr<ID3D11Texture2D> staging;
        require( SUCCEEDED( device->CreateTexture2D( &desc, nullptr, staging.GetAddressOf() ) ),
                 "Cooked draw readback must allocate" );
        context->CopyResource( staging.Get(), rendered.Get() );
        D3D11_MAPPED_SUBRESOURCE mapped{};
        require( SUCCEEDED( context->Map( staging.Get(), 0, D3D11_MAP_READ, 0, &mapped ) ),
                 "Cooked draw readback must map" );
        const auto *pixel =
            static_cast<const unsigned char *>( mapped.pData ) + 16 * mapped.RowPitch + 16 * 4;
        const unsigned red = pixel[format == DXGI_FORMAT_B8G8R8A8_UNORM ? 2 : 0];
        const unsigned actualGreen = pixel[1];
        const unsigned blue = pixel[format == DXGI_FORMAT_B8G8R8A8_UNORM ? 0 : 2];
        const unsigned alpha = pixel[3];
        context->Unmap( staging.Get(), 0 );
        renderer->setRenderTarget( nullptr );
        const bool expectedColour = asphalt
            ? red > 5 && red < 200 && actualGreen > 5 && blue > 5 && alpha > 200 &&
                  std::abs( int( red ) - int( actualGreen ) ) < 25 &&
                  std::abs( int( red ) - int( blue ) ) < 25
            : green ? actualGreen > 60 && actualGreen > red + 40 && actualGreen > blue + 40
                  : red > 60 && red > actualGreen + 40 && red > blue + 40;
        if( !expectedColour )
        {
            auto material = candidate->material();
            const auto diffuse = material->getDiffuse();
            const auto specular = material->getSpecular();
            std::cerr << "Cooked draw diagnostic: expected=" << ( green ? "green" : "red" )
                      << " centre RGBA=" << red << ',' << actualGreen << ',' << blue << ',' << alpha
                      << " format=" << format << " diffuse=" << diffuse.r << ',' << diffuse.g << ','
                      << diffuse.b << ',' << diffuse.a << " specular=" << specular.r << ',' << specular.g
                      << ',' << specular.b << " metalness=" << material->getMetalness()
                      << " roughness=" << material->getRoughness() << " cull=" << material->getCullMode()
                      << " depth=" << material->getDepthTest() << '\n';
        }
        require( expectedColour,
                 green ? "Catalog-cooked green texture must visibly shade the production mesh draw"
                       : "Catalog-cooked red texture must visibly shade the production mesh draw" );
    }

    void generatedAsphaltContracts( Fixture &fixture )
    {
#if WP_COOKED_PROCEDURAL
        auto plugin = make_ptr<procedural::WPProcedural>();
        plugin->load( nullptr );
        auto forge = fixture.graphics.application->getFactoryManager()->
            createObjectFromType<procedural::ITextureForge>( "ITextureForge" );
        require( forge != nullptr, "Generated asphalt contract requires the real texture forge" );
        forge->setSeed( 731 );
        procedural::SurfaceBakeParams params;
        params.size = 64;
        auto surface = forge->bakeSurface( procedural::SurfaceTag::Asphalt, params );
        // Match the racing surface's restrained aggregate calibration.
        for( size_t i = 0; i < surface.albedo.pixels.size(); i += 4 )
        {
            for( unsigned channel = 0; channel < 3; ++channel )
                surface.albedo.pixels[i + channel] = u8( 60 + surface.albedo.pixels[i + channel] / 6 );
            for( unsigned channel = 0; channel < 2; ++channel )
                surface.normal.pixels[i + channel] =
                    u8( 128 + ( int( surface.normal.pixels[i + channel] ) - 128 ) / 10 );
        }
        forge = nullptr;
        plugin->unload( nullptr );
        plugin = nullptr;
        const auto source = fixture.folder / "generated-asphalt-source";
        std::string exportError;
        require( procedural::texture_export::exportAsphaltSource( surface, source, 731, exportError ),
                 exportError.c_str() );
        auto catalog = make_ptr<AssetDatabaseManager>();
        require( catalog->setProjectRoot( pathText( source ) ), "Generated catalog source root must bind" );
        catalog->loadFromFile( pathText( fixture.folder / "generated-asphalt.db" ) );
        auto material = asset<CookedMaterialTestAsset>( "asphalt.matres" );
        catalog->addResourceEntry( material );
        Array<SmartPtr<CookedTextureTestAsset>> textures;
        for( const char *name : { "albedo", "normal", "orm", "ao", "roughness", "metalness" } )
        {
            auto texture = asset<CookedTextureTestAsset>( String( "asphalt-" ) + name + ".texres" );
            catalog->addResourceEntry( texture );
            textures.push_back( texture );
        }
        auto resources = std::make_shared<ResourceSystem>( fixture.resources->registry(),
            std::make_shared<ResourceCompilationDatabase>() );
        ResourceSystemConfig config;
        config.sourceRoot = pathText( source );
        config.compiledRoot = pathText( fixture.folder / "generated-asphalt-compiled" );
        config.target = "cooked-dx11-contract";
        String error;
        require( resources->initialize( config, error ), error );
        for( const auto &texture : textures )
        {
            const ResourceID id( String( "data://" ) + texture->getFilePath() );
            require( resources->compile( id, {} ).succeeded(), "Generated semantic map must cook" );
            auto loaded = resources->load( id, error );
            CookedTextureData decoded;
            require( loaded && decodeCookedTexture( *loaded, decoded, error ) && decoded.levels.size() == 7,
                     "Generated normal/ORM/scalar source maps must decode with complete semantic mips" );
        }
        {
            ClawMaterialResource publisher( catalog, resources, config.sourceRoot, fixture.mappings,
                                            fixture.graphics.renderer, config.target );
            const auto uuid = material->getHandle()->getUUIDAsString();
            require( publisher.reload( uuid, error ), error );
            auto visible = publisher.current();
            requireGpuMips( visible, false, &surface.albedo );
            requireDraw( fixture, visible, false, true );
            std::ifstream originalStream( source / "asphalt.matres", std::ios::binary );
            const std::string originalMaterial( std::istreambuf_iterator<char>( originalStream ), {} );
            originalStream.close();
            writeText( source / "asphalt.matres", "{\"version\":1," );
            require( !publisher.reload( uuid, error, true ) && publisher.current() == visible,
                     "Failed generated-material rebake must retain the last visible GPU bundle" );
            requireDraw( fixture, publisher.current(), false, true );
            writeText( source / "asphalt.matres", originalMaterial.c_str() );
            Array<ResourceID> packagedIds{ ResourceID( "data://asphalt.matres" ) };
            for( const auto &texture : textures )
                packagedIds.push_back( ResourceID( String( "data://" ) + texture->getFilePath() ) );
            CompilationOptions packageOptions;
            packageOptions.packagedBuild = true;
            for( const auto &id : packagedIds )
                require( resources->compile( id, packageOptions ).succeeded(),
                         "Generated material package closure must compile" );
            const auto manifest = fixture.folder / "generated-asphalt.wprm";
            require( resources->writeRuntimeManifest( pathText( manifest ), packagedIds, error ), error );
            const auto hidden = fixture.folder / "generated-asphalt-source-hidden";
            fs::rename( source, hidden );
            ResourceSystem packaged( nullptr, nullptr );
            RuntimeResourceConfig runtime;
            runtime.compiledRoot = config.compiledRoot;
            runtime.manifestPath = pathText( manifest );
            runtime.target = config.target;
            require( packaged.initializeRuntime( runtime, error ), error );
            for( const auto &id : packagedIds )
                require( packaged.load( id, error ) != nullptr,
                         "Generated material and semantic maps must load without sources or compilers" );
            // Retained cooked data must reupload and draw while every source is unavailable.
            visible->texture()->unload( nullptr );
            visible->texture()->load( nullptr );
            requireGpuMips( visible, false, &surface.albedo );
            requireDraw( fixture, visible, false, true );
            packaged.shutdown();
            fs::rename( hidden, source );
            require( publisher.unload( error ), error );
        }
        resources->shutdown();
        catalog->unload( nullptr );
        std::cout << "PASS: real forge asphalt source export, six semantic map cooks/decodes, exact "
                     "GPU albedo mip bytes, base-colour-only draw, last-good rebake retention and source-free closure/reupload\n";
#else
        std::cout << "SKIP: generated asphalt material contract requires WPProcedural target\n";
#endif
    }

    void contracts( Fixture &fixture )
    {
        String error;
        const String sourceRoot = pathText( fixture.source );
        ClawMaterialResource publisher( fixture.catalog, fixture.resources, sourceRoot, fixture.mappings,
                                        fixture.graphics.renderer, "cooked-dx11-contract" );
        auto red = publisher.stage( fixture.uuid, error );
        require( red != nullptr, error );
        auto redMaterial = red->material();
        require( redMaterial->getSpecular() == ColourF( .04f, .04f, .04f, 1 ),
                 "Cooked metallic/roughness materials must use the fixed dielectric F0 baseline" );
        require( !publisher.current(), "Staging the first GPU bundle must not make it visible" );
        requireGpuMips( red, false );
        require( publisher.publish( red, error ), error );
        requireDraw( fixture, publisher.current(), false );

        for(const char *profile : {"windows-dx11-dds-bgra8-v1","windows-dx11-dds-bgra8-srgb-v1",
                                  "windows-dx11-dds-bc1-v1","windows-dx11-dds-bc1-srgb-v1",
                                  "windows-dx11-dds-bc3-v1","windows-dx11-dds-bc3-srgb-v1"})
        {
            auto nativeResources=std::make_shared<ResourceSystem>(fixture.resources->registry(),
                std::make_shared<ResourceCompilationDatabase>());
            ResourceSystemConfig nativeConfig;
            nativeConfig.sourceRoot=sourceRoot;nativeConfig.compiledRoot=pathText(fixture.folder/"compiled");
            nativeConfig.target=profile;
            require(nativeResources->initialize(nativeConfig,error),error);
            ClawMaterialResource nativePublisher(fixture.catalog,nativeResources,sourceRoot,fixture.mappings,
                                                 fixture.graphics.renderer,profile,true);
            auto native=nativePublisher.stage(fixture.uuid,error);
            require(native!=nullptr,error);
            require(nativePublisher.publish(native,error),error);
            void *rawView=nullptr;native->texture()->getTextureFinal(&rawView);
            require(rawView!=nullptr,"DDS profile must install a native texture view");
            D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc{};
            static_cast<ID3D11ShaderResourceView *>(rawView)->GetDesc(&viewDesc);
            require(viewDesc.Format==static_cast<DXGI_FORMAT>(ddsFormatForTarget(profile)) &&
                    viewDesc.Texture2D.MipLevels==3,"DDS upload must preserve native GPU format and mip tails");
            CookedTextureData encoded;
            require(decodeCookedTexture(*native->resource()->dependencies.at(0),encoded,error),error);
            const auto layout=dds::inspect(encoded.ddsBytes);
            auto properties=native->texture()->getProperties();
            require(properties->getProperty("cookedProfile")==profile &&
                    properties->getPropertyObject("cookedProfile").isReadOnly() &&
                    properties->getPropertyObject("cookedResidentBytes").isReadOnly(),
                    "Editor texture inspection must expose immutable cooked profile and memory data");
            wp_texture_encoded_mip_dx11 invalidMip{encoded.ddsBytes.data()+layout.mips[0].offset,4,4,0};
            auto renderer=wp_renderer_get_dx11(fixture.graphics.renderer->getNativeRenderer());
            require(!wp_renderer_dx11_create_texture_encoded_mips_native(renderer,&invalidMip,1,layout.format) &&
                    !wp_renderer_dx11_create_texture_encoded_mips_native(renderer,&invalidMip,1,49),
                    "Native upload boundary must reject incorrect byte counts and unsupported formats");
            ComPtr<ID3D11Resource> gpuResource;
            static_cast<ID3D11ShaderResourceView *>(rawView)->GetResource(gpuResource.GetAddressOf());
            ComPtr<ID3D11Texture2D> gpuTexture;
            require(SUCCEEDED(gpuResource.As(&gpuTexture)),"Native DDS must be a 2D resource");
            D3D11_TEXTURE2D_DESC stagingDesc{};gpuTexture->GetDesc(&stagingDesc);
            stagingDesc.Usage=D3D11_USAGE_STAGING;stagingDesc.BindFlags=stagingDesc.MiscFlags=0;
            stagingDesc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
            ComPtr<ID3D11Device> device;gpuTexture->GetDevice(device.GetAddressOf());
            ComPtr<ID3D11DeviceContext> context;device->GetImmediateContext(context.GetAddressOf());
            ComPtr<ID3D11Texture2D> staging;
            require(SUCCEEDED(device->CreateTexture2D(&stagingDesc,nullptr,staging.GetAddressOf())),
                    "DDS readback texture must allocate");
            context->CopyResource(staging.Get(),gpuTexture.Get());
            for(unsigned level=0;level<layout.mips.size();++level)
            {
                const auto &mip=layout.mips[level];
                D3D11_MAPPED_SUBRESOURCE mapped{};
                require(SUCCEEDED(context->Map(staging.Get(),level,D3D11_MAP_READ,0,&mapped)),
                        "Every DDS mip must support readback");
                bool exact=true;
                const auto rows=dds::blockBytes(layout.format)?(mip.height+3)/4:mip.height;
                for(unsigned row=0;row<rows;++row)
                    exact=exact && mapped.RowPitch>=mip.rowPitch &&
                        std::memcmp(static_cast<const u8 *>(mapped.pData)+row*mapped.RowPitch,
                                    encoded.ddsBytes.data()+mip.offset+row*mip.rowPitch,mip.rowPitch)==0;
                context->Unmap(staging.Get(),level);
                require(exact,"DDS GPU subresources must preserve exact encoded bytes and row pitches");
            }
            if(viewDesc.Format==DXGI_FORMAT_B8G8R8A8_UNORM || viewDesc.Format==DXGI_FORMAT_BC1_UNORM ||
               viewDesc.Format==DXGI_FORMAT_BC3_UNORM) requireDraw(fixture,nativePublisher.current(),false);
            fs::rename(fixture.source/"albedo.tga",fixture.source/"albedo.hidden");
            const auto manifest=fixture.folder/(std::string(profile)+".wprm");
            require(nativeResources->writeRuntimeManifest(pathText(manifest),{ResourceID("data://surface.matres")},error),error);
            ResourceSystem sourceFree(nullptr,nullptr);
            RuntimeResourceConfig runtimeConfig;
            runtimeConfig.compiledRoot=nativeConfig.compiledRoot;runtimeConfig.manifestPath=pathText(manifest);
            runtimeConfig.target=profile;
            require(sourceFree.initializeRuntime(runtimeConfig,error),error);
            auto packaged=sourceFree.load(ResourceID("data://surface.matres"),error);
            CookedMaterialData materialData;
            require(packaged && decodeCookedMaterial(*packaged,materialData,error) && materialData.packagedBuild,
                    "DDS package closure must load without source images or compiler services");
            sourceFree.shutdown();
            native->texture()->unload(nullptr);native->texture()->load(nullptr);
            rawView=nullptr;native->texture()->getTextureFinal(&rawView);
            fs::rename(fixture.source/"albedo.hidden",fixture.source/"albedo.tga");
            require(rawView!=nullptr,"Retained DDS must reupload without the source image");
            // Invalid replacement must leave the installed DDS view unchanged.
            CookedTextureData broken;
            require(!uploadCookedDds(native->texture().get(),broken,
                    wp_renderer_get_dx11(fixture.graphics.renderer->getNativeRenderer()),error),
                    "Invalid DDS replacement must fail");
            void *preserved=nullptr;native->texture()->getTextureFinal(&preserved);
            require(preserved==rawView,"Failed DDS upload must retain the previous native view");
            nativeResources->shutdown();
        }
        std::cout << "PASS: six Windows DDS profiles, native formats, compressed draw and source-free retained reupload\n";

        writeImage( fixture.source / "albedo.tga", true );
        auto green = publisher.stage( fixture.uuid, error, true );
        require( green && publisher.current() == red,
                 "Staging a replacement must retain the previous visible material" );
        requireGpuMips( green, true );
        requireDraw( fixture, publisher.current(), false );
        require( publisher.publish( green, error ), error );
        requireDraw( fixture, publisher.current(), true );
        requireGpuMips( red, false );

        writeText( fixture.source / "surface.matres", "{\"version\":1," );
        require( !publisher.reload( fixture.uuid, error, true ) && !error.empty() &&
                     publisher.current() == green,
                 "Malformed source must preserve the last visible GPU material" );
        requireDraw( fixture, publisher.current(), true );
        writeText( fixture.source / "surface.matres", materialDescriptor );

        auto older = publisher.stage( fixture.uuid, error, true );
        auto newer = publisher.stage( fixture.uuid, error, true );
        require( older && newer && !publisher.publish( older, error ) && !error.empty() &&
                     publisher.current() == green,
                 "Newer reload tickets must reject older work at the same catalog generation" );
        require( publisher.publish( newer, error ), error );
        auto visible = publisher.current();
        auto superseded = publisher.stage( fixture.uuid, error );
        writeText( fixture.source / "albedo.texres", "{\"version\":999}" );
        require( superseded && !publisher.stage( fixture.uuid, error, true ) &&
                     !publisher.publish( superseded, error ) && publisher.current() == visible,
                 "Even a failed newer reload must supersede earlier staged work" );
        writeText( fixture.source / "albedo.texres", textureDescriptor );
        requireDraw( fixture, publisher.current(), true );

        {
            auto racing = std::make_shared<ReplacedGenerationSystem>( fixture.resources );
            ClawMaterialResource guarded( fixture.catalog, racing, sourceRoot, fixture.mappings,
                                          fixture.graphics.renderer, "cooked-dx11-contract" );
            require( guarded.reload( fixture.uuid, error, true ), error );
            const auto previous = guarded.current();
            for( auto replacement : { ReplacedGenerationSystem::Replacement::SourceHash,
                                      ReplacedGenerationSystem::Replacement::PayloadHash } )
            {
                racing->replacement = replacement;
                racing->compilationSucceeded = false;
                racing->validReplacementLoaded = false;
                require( !guarded.stage( fixture.uuid, error, true ) && !error.empty() &&
                             racing->compilationSucceeded && racing->validReplacementLoaded &&
                             guarded.current() == previous,
                         "A valid generation loaded after compile must match that compile report" );
                requireDraw( fixture, guarded.current(), true );
            }
        }

        {
            ClawMaterialResource other( fixture.catalog, fixture.resources, sourceRoot, fixture.mappings,
                                        fixture.graphics.renderer, "cooked-dx11-contract" );
            auto foreign = other.stage( fixture.uuid, error, true );
            require( foreign && !publisher.publish( foreign, error ) && !error.empty() &&
                         publisher.current() == visible && !other.current(),
                     "Candidates must only publish into their originating owner" );
        }

        auto renamed = publisher.stage( fixture.uuid, error );
        require( renamed != nullptr, error );
        writeText( fixture.source / "renamed.matres", materialDescriptor );
        fixture.materialAsset->setFilePath( "renamed.matres" );
        fixture.catalog->updateResourceEntry( fixture.materialAsset );
        require( !publisher.publish( renamed, error ) && publisher.current() == visible,
                 "Catalog rename must reject a staged result without replacing the visible asset" );
        requireDraw( fixture, publisher.current(), true );
        require( publisher.reload( fixture.uuid, error ), error );
        require(
            publisher.current()->resource()->header.resourceId == ResourceID( "data://renamed.matres" ),
            "Durable UUID must publish its renamed cooked ResourceID" );
        visible = publisher.current();

        auto deleted = publisher.stage( fixture.uuid, error );
        require( deleted != nullptr, error );
        fixture.catalog->removeResourceEntry( fixture.materialAsset );
        require( !fixture.catalog->hasResourceById( fixture.uuid ) &&
                     !publisher.publish( deleted, error ) && publisher.current() == visible,
                 "Catalog deletion must invalidate staged publication and preserve the visible asset" );
        fixture.catalog->addResourceEntry( fixture.materialAsset );
        require(
            fixture.catalog->hasResourceById( fixture.uuid ) && !publisher.publish( deleted, error ),
            "Reinserting the same UUID must not revive its old generation" );
        requireDraw( fixture, publisher.current(), true );

        auto closed = publisher.stage( fixture.uuid, error );
        require( closed != nullptr, error );
        fixture.catalog->unload( nullptr );
        require( !publisher.publish( closed, error ) && publisher.current() == visible,
                 "Catalog unload must reject publication while retaining the last visible asset" );
        requireDraw( fixture, publisher.current(), true );
        fixture.catalog->loadFromFile( fixture.databasePath );
        require( !publisher.publish( closed, error ),
                 "Catalog reopen must not revive staged generations" );
        require( publisher.reload( fixture.uuid, error ), error );
        visible = publisher.current();

        auto ownerCandidate = publisher.stage( fixture.uuid, error );
        require( ownerCandidate != nullptr, error );
        bool threadRejected = false;
        std::thread worker( [&] {
            String threadError;
            const bool stageRejected =
                !publisher.stage( fixture.uuid, threadError ) && !threadError.empty();
            const bool publishRejected =
                !publisher.publish( ownerCandidate, threadError ) && !threadError.empty();
            const bool unloadRejected = !publisher.unload( threadError ) && !threadError.empty();
            threadRejected = stageRejected && publishRejected && unloadRejected;
        } );
        worker.join();
        require( threadRejected && publisher.current() == visible,
                 "Worker-thread stage/publish/unload must reject without changing render ownership" );
        requireDraw( fixture, publisher.current(), true );

        auto unloaded = publisher.stage( fixture.uuid, error );
        require( unloaded && publisher.unload( error ) && !publisher.current() &&
                     !publisher.publish( unloaded, error ),
                 "Consumer unload must clear visibility and invalidate staged tickets" );
        require( publisher.reload( fixture.uuid, error ), error );
        visible = publisher.current();
        auto oldDevice = publisher.stage( fixture.uuid, error );
        require( oldDevice != nullptr, error );
        auto retainedTexture = visible->texture();
        ComPtr<ID3D11ShaderResourceView> retainedView;
        retainedView = claw_texture_mip_contracts::view( *retainedTexture );
        ComPtr<ID3D11Device> retainedDevice;
        retainedView->GetDevice( retainedDevice.GetAddressOf() );
        fixture.graphics.renderer->unload( nullptr );
        require(
            !publisher.publish( oldDevice, error ) && !publisher.current(),
            "Device loss must reject staged GPU publication and hide incompatible visible handles" );
        require( claw_texture_mip_contracts::greenPixel( retainedView.Get() ) == 128,
                 "A retained old-device bundle must preserve its original GPU texture" );
        fixture.graphics.renderer->load( fixture.graphics.window );
        require( fixture.graphics.renderer->isLoaded(), "Contract device must recreate" );
        require( !publisher.publish( oldDevice, error ) && !publisher.current(),
                 "Recreating the renderer must not accept resources pinned to the previous device" );
        auto *recreated = wp_renderer_get_dx11( fixture.graphics.renderer->getNativeRenderer() );
        require( wp_renderer_dx11_get_device( recreated ) != retainedDevice.Get() &&
                     claw_texture_mip_contracts::greenPixel( retainedView.Get() ) == 128,
                 "Device recreation must preserve the retained bundle on its distinct original device" );
        require( publisher.unload( error ), error );
        {
            ClawMaterialResource restored( fixture.catalog, fixture.resources, sourceRoot,
                                           fixture.mappings, fixture.graphics.renderer,
                                           "cooked-dx11-contract" );
            require( restored.reload( fixture.uuid, error ), error );
            requireGpuMips( restored.current(), true );
            requireDraw( fixture, restored.current(), true );
            require( restored.unload( error ), error );
        }
        std::cout << "PASS: generated catalog material/texture compilers, exact cooked mip uploads, "
                     "DX11 red-to-green draw, failed replacement, generation/ticket/owner rejection, "
                     "rename/delete/reopen, thread ownership and device recreation\n";
    }
}  // namespace

#include "CookedTerrainContracts.hpp"
#include "CookedFoliageContracts.hpp"

int main()
{
    try
    {
        Fixture fixture;
        fixture.initialize();
        generatedAsphaltContracts( fixture );
        contracts( fixture );
        cooked_terrain_contracts::run( *fixture.graphics.renderer, fixture.folder );
        cooked_foliage_contracts::run( *fixture.graphics.renderer, fixture.folder );
        return 0;
    }
    catch( const std::exception &error )
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
