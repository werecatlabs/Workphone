#include <Workphone/Core/Handle.hpp>
#include <Workphone/Database/AssetDatabaseManager.hpp>
#include <Workphone/Database/AssetPackagePolicy.hpp>
#include <Workphone/Database/CatalogResourceAdapter.hpp>
#include <Workphone/Graphics/Material.hpp>
#include <Workphone/Graphics/Texture.hpp>
#include <Workphone/Graphics/TerrainData.hpp>
#include <Workphone/Graphics/FoliageData.hpp>
#include <Workphone/Interface/Database/IDatabase.hpp>
#include <Workphone/IO/FileSystem.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/FactoryManager.hpp>
#include <Workphone/System/ResourceCompilerRegistry.hpp>
#include <Workphone/System/ResourceSystem.hpp>
#include <Workphone/System/Resource.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <WPGraphics/Resources/GraphicsResourceCompiler.hpp>
#include <WPGraphics/Resources/TerrainResource.hpp>
#include <WPGraphics/Resources/FoliageResource.hpp>
#include <WPSQLite/ResourceCompilationDatabase.hpp>
#include <WPSQLite/WPSQLite.hpp>
#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

// External tools register their authored file types using the engine's existing
// Resource/IResource metadata. The production compilers still own every format.
namespace workphone
{
    class CppSdkTerrainAsset : public Resource<IResource> { public: WP_CLASS_REGISTER_DECL; };
    class CppSdkFoliageWorldAsset : public Resource<IResource> { public: WP_CLASS_REGISTER_DECL; };
    class CppSdkFoliageSpeciesAsset : public Resource<IResource> { public: WP_CLASS_REGISTER_DECL; };
    class CppSdkFoliagePageAsset : public Resource<IResource> { public: WP_CLASS_REGISTER_DECL; };
    WP_CLASS_REGISTER_DERIVED( workphone, CppSdkTerrainAsset, Resource<IResource> );
    WP_CLASS_REGISTER_DERIVED( workphone, CppSdkFoliageWorldAsset, Resource<IResource> );
    WP_CLASS_REGISTER_DERIVED( workphone, CppSdkFoliageSpeciesAsset, Resource<IResource> );
    WP_CLASS_REGISTER_DERIVED( workphone, CppSdkFoliagePageAsset, Resource<IResource> );
}

namespace
{
    namespace fs = std::filesystem;
    using namespace workphone;
    using namespace workphone::resource;

    constexpr const char *materialUUID = "2a6d7957-45ab-4bbb-9d96-91f7a9c1a001";
    constexpr const char *textureUUID = "2a6d7957-45ab-4bbb-9d96-91f7a9c1a002";
    constexpr const char *terrainUUID = "2a6d7957-45ab-4bbb-9d96-91f7a9c1a003";
    constexpr const char *foliageUUID = "2a6d7957-45ab-4bbb-9d96-91f7a9c1a004";
    constexpr const char *speciesUUID = "2a6d7957-45ab-4bbb-9d96-91f7a9c1a005";
    constexpr const char *pageUUID = "2a6d7957-45ab-4bbb-9d96-91f7a9c1a006";

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

    std::string foldedPath( const fs::path &path )
    {
        auto result = fs::weakly_canonical( path ).generic_u8string();
        std::transform( result.begin(), result.end(), result.begin(),
                        []( unsigned char c ) { return static_cast<char>( std::tolower( c ) ); } );
        if( result.back() != '/' )
            result += '/';
        return result;
    }

    void writeText( const fs::path &path, const char *contents )
    {
        std::ofstream stream( path, std::ios::binary | std::ios::trunc );
        stream << contents;
        require( stream.good(), "Cannot write generated sample descriptor" );
    }

    void writeImage( const fs::path &path )
    {
        // Original uniform 4x4 BGRA image, top-down uncompressed TGA. Every mip
        // remains the same colour, making the installed reader checks exact.
        std::array<unsigned char, 18 + 4 * 4 * 4> bytes{};
        bytes[2] = 2;
        bytes[12] = bytes[14] = 4;
        bytes[16] = 32;
        bytes[17] = 0x28;
        for( size_t pixel = 18; pixel < bytes.size(); pixel += 4 )
        {
            bytes[pixel] = 32;
            bytes[pixel + 1] = 96;
            bytes[pixel + 2] = 224;
            bytes[pixel + 3] = 255;
        }
        std::ofstream stream( path, std::ios::binary | std::ios::trunc );
        stream.write( reinterpret_cast<const char *>( bytes.data() ), bytes.size() );
        require( stream.good(), "Cannot write generated sample image" );
    }

    void writeTerrain( const fs::path &path )
    {
        render::TerrainData terrain;
        terrain.dimensions = { 3, 3 };
        terrain.spacing = { 2, 4 };
        terrain.origin = { -2, -4 };
        terrain.heightScale = .5f;
        terrain.heights = { 0, 1, 2, 2, 3, 4, 4, 5, 6 };
        terrain.holes = { 0, 0, 0, 1 };
        terrain.layers[0].albedo = "data://albedo.texres";
        terrain.layers[0].uvScale = { .5f, .25f };
        terrain.layers[0].tint = { .75f, .5f, .25f, 1 };
        terrain.layers[0].roughness = .625f;
        terrain.layers[0].metallic = .125f;
        terrain.appearance.wetness = .25f;
        terrain.surfaceMasks.assign( 9, Vector2F( .5f, 1 ) );
        String error;
        require( render::validateTerrainData( terrain, error ), error );
        const auto descriptor = render::serializeTerrainData( terrain );
        require( !descriptor.empty(), "Sample terrain serialization failed" );
        writeText( path, descriptor.c_str() );
    }

    void writeFoliage( const fs::path &source )
    {
        auto species = std::make_shared<render::FoliageSpecies>();
        species->id = "installed-sdk-bush";
        species->kind = render::FoliageKind::Bush;
        species->boundsMinimum = { -.5f, 0, -.25f };
        species->boundsMaximum = { .5f, 1, .25f };
        species->materials.resize( 1 );
        species->materials[0].albedo = "data://albedo.texres";
        species->materials[0].roughness = .625f;
        species->materials[0].metalness = .125f;
        for( auto &lod : species->lods )
        {
            lod.positions = { { -.5f, 0, 0 }, { .5f, 0, 0 }, { 0, 1, 0 } };
            lod.normals = { { 0, 0, 1 }, { 0, 0, 1 }, { 0, 0, 1 } };
            lod.uvs = { { 0, 1 }, { 1, 1 }, { .5f, 0 } };
            lod.indices = { 0, 1, 2 };
            lod.sections = { { 0, 3, 0 } };
        }
        auto world = std::make_shared<render::FoliageWorldData>();
        world->pageSize = 16;
        world->species.push_back( species );
        for( u32 index = 0; index < 2; ++index )
        {
            render::FoliageInstance instance;
            instance.id = 21 + index;
            instance.position = Vector3F( f32( index + 1 ), 0, f32( index + 1 ) );
            instance.seed = 101 + index;
            instance.flags = render::FoliageManual;
            world->instances.push_back( instance );
        }
        String error;
        require( render::writeFoliageSources( world, pathText( source ),
                     ResourceID( "data://forest.folworld" ), error ), error );
    }

    struct AuthoringRuntime
    {
        TypeManager types;
        SmartPtr<core::ApplicationManager> application;
        SmartPtr<FactoryManager> factories;
        SmartPtr<FileSystem> fileSystem;
        SmartPtr<SQLitePlugin> sqlite;
        SmartPtr<AssetDatabaseManager> catalog;
        std::shared_ptr<ResourceSystem> resources;

        AuthoringRuntime()
        {
            types.load();
            TypeManager::setInstance( &types );
        }

        void initialize()
        {
            application = make_ptr<core::ApplicationManager>();
            core::IApplicationManager::setInstance( application );
            fileSystem = make_ptr<FileSystem>();
            application->setFileSystem( fileSystem );
            factories = make_ptr<FactoryManager>();
            application->setFactoryManager( factories );
            factories->load( nullptr );
            sqlite = make_ptr<SQLitePlugin>();
            sqlite->load( nullptr );
        }

        ~AuthoringRuntime()
        {
            if( resources )
                resources->shutdown();
            resources.reset();
            if( catalog )
                catalog->unload( nullptr );
            catalog = nullptr;
            if( sqlite )
                sqlite->unload( nullptr );
            sqlite = nullptr;
            if( factories )
                factories->unload( nullptr );
            if( application )
                application->setFactoryManager( nullptr );
            factories = nullptr;
            if( application )
                application->setFileSystem( nullptr );
            fileSystem = nullptr;
            core::IApplicationManager::setInstance( nullptr );
            application = nullptr;
            types.unload();
            TypeManager::setInstance( nullptr );
        }
    };

    template <class T>
    SmartPtr<T> catalogAsset( const char *path, const char *uuid )
    {
        auto result = make_ptr<T>();
        result->getHandle()->setUUID( uuid );
        result->setFilePath( path );
        return result;
    }

    void setRuntimePolicy( SmartPtr<AssetDatabaseManager> catalog, const char *uuid )
    {
        AssetDatabaseManager::EntrySnapshot entry;
        require( catalog->tryGetEntry( uuid, entry ), "Sample catalog identity was not inserted" );
        String error;
        require( setAssetPackagePolicy( catalog, entry, AssetPackageUsage::Runtime, error ),
                 error );
    }

    void cookSample( const fs::path &output, const fs::path &work )
    {
        require( !fs::exists( output ), "Content output must be a new directory" );
        require( !fs::exists( work ) || ( fs::is_directory( work ) && fs::is_empty( work ) ),
                 "Authoring work directory must be new or empty" );
        const auto outputText = foldedPath( output );
        const auto workText = foldedPath( work );
        require( outputText.rfind( workText, 0 ) != 0 && workText.rfind( outputText, 0 ) != 0,
                 "Content and authoring directories must not overlap" );
        const auto source = work / "source";
        fs::create_directories( source );
        writeImage( source / "albedo.tga" );
        writeText( source / "albedo.texres",
                   R"({"version":1,"source":"data://albedo.tga","mipFilter":"colour","atlasColumns":1,"alphaCutoff":0.5})" );
        writeText( source / "surface.matres",
                   R"({"version":1,"baseColour":[0.75,0.5,0.25,1],"metalness":0.125,"roughness":0.625,"baseColourTexture":"data://albedo.texres"})" );
        writeTerrain( source / "ground.terrain" );
        writeFoliage( source );

        AuthoringRuntime runtime;
        runtime.initialize();
        runtime.catalog = make_ptr<AssetDatabaseManager>();
        require( runtime.catalog->setProjectRoot( pathText( source ) ),
                 "Sample catalog rejected its explicit source root" );
        runtime.catalog->loadFromFile( pathText( work / "catalog.db" ) );
        const auto database = runtime.catalog->getDatabase();
        require( database && database->isLoaded(), "Sample requires the real SQLite catalog" );
        const auto material = catalogAsset<render::Material>( "surface.matres", materialUUID );
        const auto texture = catalogAsset<render::Texture>( "albedo.texres", textureUUID );
        const auto terrain = catalogAsset<CppSdkTerrainAsset>( "ground.terrain", terrainUUID );
        const auto foliage = catalogAsset<CppSdkFoliageWorldAsset>( "forest.folworld", foliageUUID );
        Array<SmartPtr<ISharedObject>> assets = { material, texture, terrain, foliage };
        unsigned speciesFiles = 0, pageFiles = 0;
        for( const auto &entry : fs::recursive_directory_iterator( source ) )
        {
            if( !entry.is_regular_file() )
                continue;
            const auto relative = pathText( fs::relative( entry.path(), source ) );
            if( entry.path().extension() == ".folspec" )
            {
                ++speciesFiles;
                assets.push_back( catalogAsset<CppSdkFoliageSpeciesAsset>( relative.c_str(), speciesUUID ) );
            }
            else if( entry.path().extension() == ".folpage" )
            {
                ++pageFiles;
                assets.push_back( catalogAsset<CppSdkFoliagePageAsset>( relative.c_str(), pageUUID ) );
            }
        }
        require( speciesFiles == 1 && pageFiles == 1, "Sample foliage must export one species and one page" );
        runtime.catalog->addResourceEntries( assets );
        for( const auto *uuid : { materialUUID, textureUUID, terrainUUID, foliageUUID, speciesUUID, pageUUID } )
            setRuntimePolicy( runtime.catalog, uuid );

        String error;
        auto registry = std::make_shared<ResourceCompilerRegistry>();
        require( registry->registerCompiler( std::make_shared<render::GraphicsResourceCompiler>(),
                                             &error ), error );
        require( registry->registerCompiler( std::make_shared<render::TerrainResourceCompiler>(), &error ), error );
        require( registry->registerCompiler( std::make_shared<render::FoliageResourceCompiler>(), &error ), error );
        runtime.resources = std::make_shared<ResourceSystem>(
            registry, std::make_shared<ResourceCompilationDatabase>() );
        ResourceSystemConfig config;
        config.sourceRoot = pathText( source );
        config.compiledRoot = pathText( work / "compiled" );
        config.target = "installed-cpp-sample";
        require( runtime.resources->initialize( config, error ), error );
        Array<CatalogResourceAdapter::TypeMapping> mappings;
        for( const auto *uuid : { materialUUID, textureUUID, terrainUUID, foliageUUID, speciesUUID, pageUUID } )
        {
            AssetDatabaseManager::EntrySnapshot entry;
            require( runtime.catalog->tryGetEntry( uuid, entry ), "Sample catalog lost an admitted type" );
            mappings.push_back( { entry.type, ResourceID( entry.path ).type() } );
        }
        CatalogResourceAdapter adapter( runtime.catalog, runtime.resources, config.sourceRoot, mappings );
        CatalogResourceAdapter::Request root;
        require( adapter.resolve( materialUUID, root, error ), error );
        require( root.resourceId == ResourceID( "data://surface.matres" ),
                 "Catalog sample identity changed" );
        CompilationOptions options;
        options.packagedBuild = true;
        Array<CatalogResourceAdapter::Request> roots = { root };
        for( const auto *uuid : { terrainUUID, foliageUUID } )
        {
            CatalogResourceAdapter::Request extra;
            require( adapter.resolve( uuid, extra, error ), error );
            roots.push_back( extra );
        }
        for( const auto &request : roots )
        {
            const auto compilation = adapter.compile( request, options );
            if( !compilation.succeeded() )
                for( const auto &message : compilation.messages )
                    std::cerr << message << '\n';
            require( compilation.succeeded(), "Sample package compilation failed" );
        }
        Array<RuntimePackagePreviewEntry> entries;
        require( adapter.previewRuntimePackage( roots, entries, error ), error );
        require( entries.size() == 6, "Sample package must contain all six typed closure resources" );
        require( adapter.writeRuntimePackage( pathText( output ), roots, error ), error );
        size_t files = 0;
        for( const auto &entry : fs::recursive_directory_iterator( output ) )
            if( entry.is_regular_file() )
            {
                ++files;
                const auto extension = entry.path().extension();
                require( extension == ".wprm" || extension == ".wprs",
                         "Authoring input leaked into runtime content" );
            }
        require( files == 7 && fs::is_regular_file( output / "runtime.wprm" ),
                 "Sample package must contain one manifest and six cooked containers" );
        std::cout << "materialUUID=" << materialUUID << '\n'
                  << "textureUUID=" << textureUUID << '\n'
                  << "terrainUUID=" << terrainUUID << '\n'
                  << "foliageUUID=" << foliageUUID << '\n'
                  << "speciesUUID=" << speciesUUID << '\n'
                  << "pageUUID=" << pageUUID << '\n'
                  << "packageResources=6\npackageFiles=7\ncatalogAdmission=passed\n"
                  << "Cooked C++ sample: material/texture/terrain/foliage species/page/world exported\n";
    }
}

int wmain( int argc, wchar_t **argv )
{
    try
    {
        require( argc == 3, "Usage: cpp-sdk-cook-sample <new-content-directory> <private-authoring-work-directory>" );
        cookSample( fs::absolute( fs::path( argv[1] ) ), fs::absolute( fs::path( argv[2] ) ) );
        return 0;
    }
    catch( const std::exception &exception )
    {
        std::cerr << "Cooked C++ sample failed: " << exception.what() << '\n';
        return 1;
    }
}
