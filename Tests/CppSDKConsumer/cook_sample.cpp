#include <Workphone/Core/Handle.hpp>
#include <Workphone/Database/AssetDatabaseManager.hpp>
#include <Workphone/Database/AssetPackagePolicy.hpp>
#include <Workphone/Database/CatalogResourceAdapter.hpp>
#include <Workphone/Graphics/Material.hpp>
#include <Workphone/Graphics/Texture.hpp>
#include <Workphone/Interface/Database/IDatabase.hpp>
#include <Workphone/IO/FileSystem.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/FactoryManager.hpp>
#include <Workphone/System/ResourceCompilerRegistry.hpp>
#include <Workphone/System/ResourceSystem.hpp>
#include <WPGraphics/Resources/GraphicsResourceCompiler.hpp>
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

namespace
{
    namespace fs = std::filesystem;
    using namespace workphone;
    using namespace workphone::resource;

    constexpr const char *materialUUID = "2a6d7957-45ab-4bbb-9d96-91f7a9c1a001";
    constexpr const char *textureUUID = "2a6d7957-45ab-4bbb-9d96-91f7a9c1a002";

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
        runtime.catalog->addResourceEntries( { material, texture } );
        setRuntimePolicy( runtime.catalog, materialUUID );
        setRuntimePolicy( runtime.catalog, textureUUID );

        String error;
        auto registry = std::make_shared<ResourceCompilerRegistry>();
        require( registry->registerCompiler( std::make_shared<render::GraphicsResourceCompiler>(),
                                             &error ), error );
        runtime.resources = std::make_shared<ResourceSystem>(
            registry, std::make_shared<ResourceCompilationDatabase>() );
        ResourceSystemConfig config;
        config.sourceRoot = pathText( source );
        config.compiledRoot = pathText( work / "compiled" );
        config.target = "installed-cpp-sample";
        require( runtime.resources->initialize( config, error ), error );
        CatalogResourceAdapter adapter( runtime.catalog, runtime.resources, config.sourceRoot,
            { { "Material", ResourceTypeID( "matres" ) }, { "Texture", ResourceTypeID( "texres" ) } } );
        CatalogResourceAdapter::Request root;
        require( adapter.resolve( materialUUID, root, error ), error );
        require( root.resourceId == ResourceID( "data://surface.matres" ),
                 "Catalog sample identity changed" );
        CompilationOptions options;
        options.packagedBuild = true;
        const auto compilation = adapter.compile( root, options );
        if( !compilation.succeeded() )
            for( const auto &message : compilation.messages )
                std::cerr << message << '\n';
        require( compilation.succeeded(), "Sample package compilation failed" );
        Array<RuntimePackagePreviewEntry> entries;
        require( adapter.previewRuntimePackage( { root }, entries, error ), error );
        require( entries.size() == 2, "Sample package must contain exactly material and texture" );
        require( adapter.writeRuntimePackage( pathText( output ), { root }, error ), error );
        size_t files = 0;
        for( const auto &entry : fs::recursive_directory_iterator( output ) )
            if( entry.is_regular_file() )
            {
                ++files;
                const auto extension = entry.path().extension();
                require( extension == ".wprm" || extension == ".wprs",
                         "Authoring input leaked into runtime content" );
            }
        require( files == 3 && fs::is_regular_file( output / "runtime.wprm" ),
                 "Sample package must contain one manifest and two cooked containers" );
        std::cout << "materialUUID=" << materialUUID << '\n'
                  << "textureUUID=" << textureUUID << '\n'
                  << "packageResources=2\npackageFiles=3\ncatalogAdmission=passed\n"
                  << "Cooked C++ sample: original material and three-mip texture exported\n";
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
