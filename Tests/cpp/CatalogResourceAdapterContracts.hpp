#ifndef WPCatalogResourceAdapterContracts_h__
#define WPCatalogResourceAdapterContracts_h__

#include <Workphone/Database/CatalogResourceAdapter.hpp>
#include <Workphone/System/Resource.hpp>
#include <Workphone/System/ResourceCompilerRegistry.hpp>
#include <Workphone/System/ResourceSystem.hpp>
#include <WPSQLite/ResourceCompilationDatabase.hpp>

#include <filesystem>
#include <fstream>
#include <functional>
#include <stdexcept>

namespace workphone
{
    class CatalogAdapterTestResource : public Resource<IResource>
    {
    public:
        WP_CLASS_REGISTER_DECL;
    };
    WP_CLASS_REGISTER_DERIVED( workphone, CatalogAdapterTestResource, Resource<IResource> );

    class CatalogAdapterTestSceneEntry : public ISharedObject
    {
    public:
        WP_CLASS_REGISTER_DECL;
    };
    WP_CLASS_REGISTER_DERIVED( workphone, CatalogAdapterTestSceneEntry, ISharedObject );
}  // namespace workphone

namespace catalog_adapter_contracts
{
    using namespace workphone;
    using namespace workphone::resource;
    namespace fs = std::filesystem;

    inline void require( bool condition, const char *message )
    {
        if( !condition )
            throw std::runtime_error( message );
    }

    inline void write( const fs::path &path, const std::string &contents )
    {
        std::ofstream stream( path, std::ios::binary | std::ios::trunc );
        stream.write( contents.data(), static_cast<std::streamsize>( contents.size() ) );
        require( stream.good(), "Bridge fixture source must be writable" );
    }

    inline SmartPtr<CatalogAdapterTestResource> asset( const String &path )
    {
        auto result = make_ptr<CatalogAdapterTestResource>();
        result->getHandle()->setUUID( StringUtil::getUUID() );
        result->setFilePath( path );
        return result;
    }

    class Compiler final : public IResourceCompiler
    {
    public:
        std::function<void()> duringCompile;
        u64 version = 1;
        bool fail = false;

        String name() const override
        {
            return "CatalogBridgeContractCompiler";
        }
        Array<CompilerOutput> outputs() const override
        {
            return { { ResourceTypeID( "ctest" ), version } };
        }
        bool getDependencies( const CompileContext &context, DependencySet &dependencies,
                              String &error ) const override
        {
            std::ifstream stream( context.sourcePath.c_str() );
            if( !stream )
            {
                error = "Cannot read bridge fixture descriptor";
                return false;
            }
            std::string line;
            while( std::getline( stream, line ) )
            {
                if( line.rfind( "install=", 0 ) == 0 )
                {
                    ResourceID dependency( line.substr( 8 ).c_str() );
                    if( !dependency.isValid() )
                    {
                        error = "Invalid bridge fixture dependency";
                        return false;
                    }
                    dependencies.compileDependencies.push_back(
                        CompileDependency::resource( dependency ) );
                    dependencies.installDependencies.push_back( dependency );
                }
            }
            return true;
        }
        CompilationStatus compile( const CompileContext &context, std::ostream &output,
                                   Array<String> &messages ) const override
        {
            if( fail )
            {
                messages.push_back( "Injected bridge compiler failure" );
                return CompilationStatus::Failure;
            }
            std::ifstream stream( context.sourcePath.c_str(), std::ios::binary );
            if( !stream )
                return CompilationStatus::Failure;
            output << stream.rdbuf();
            if( duringCompile )
                duringCompile();
            return output ? CompilationStatus::Success : CompilationStatus::Failure;
        }
    };

    inline void run( const fs::path &folder )
    {
        const auto root = folder / "bridge";
        const auto source = root / "source";
        const auto other = root / "other";
        fs::create_directories( source );
        fs::create_directories( other );
        write( source / "root.ctest", "install=data://dependency.ctest\noriginal payload\n" );
        write( source / "dependency.ctest", "dependency payload\n" );

        auto catalog = make_ptr<AssetDatabaseManager>();
        require( catalog->setProjectRoot( source.u8string().c_str() ),
                 "Bridge catalog must accept explicit source root" );
        const String catalogPath = ( root / "catalog.db" ).u8string().c_str();
        catalog->loadFromFile( catalogPath );
        auto sourceAsset = asset( "root.ctest" );
        catalog->addResourceEntry( sourceAsset );
        const String uuid = sourceAsset->getHandle()->getUUIDAsString();
        auto scene = make_ptr<CatalogAdapterTestSceneEntry>();
        scene->getHandle()->setUUID( StringUtil::getUUID() );
        catalog->addResourceEntry( scene );
        AssetDatabaseManager::EntrySnapshot snapshot;
        require( catalog->tryGetEntry( uuid, snapshot ), "Bridge fixture catalog row must exist" );

        auto registry = std::make_shared<ResourceCompilerRegistry>();
        auto compiler = std::make_shared<Compiler>();
        String error;
        require( registry->registerCompiler( compiler, &error ), error.c_str() );
        auto compilationDatabase = std::make_shared<ResourceCompilationDatabase>();
        auto resources = std::make_shared<ResourceSystem>( registry, compilationDatabase );
        ResourceSystemConfig config;
        config.sourceRoot = source.u8string().c_str();
        config.compiledRoot = ( root / "compiled" ).u8string().c_str();
        config.target = "catalog-contract";
        require( resources->initialize( config, error ), error.c_str() );
        const Array<CatalogResourceAdapter::TypeMapping> mappings = { { snapshot.type,
                                                                        ResourceTypeID( "ctest" ) } };
        CatalogResourceAdapter adapter( catalog, resources, config.sourceRoot, mappings );
        CatalogResourceAdapter::Request request;
        require( adapter.resolve( uuid, request, error ), error.c_str() );
        require( request.resourceId == ResourceID( "data://root.ctest" ),
                 "Catalog UUID must resolve to its exact canonical ResourceID" );
        require( adapter.compile( request ).succeeded(), "Catalog request must compile its resource" );
        auto loaded = adapter.load( request, error );
        require( loaded && loaded->dependencies.size() == 1,
                 "Catalog load must use ResourceSystem install dependency loading" );
        require( adapter.compile( request ).status == CompilationStatus::UpToDate,
                 "Catalog bridge must reuse existing compilation metadata" );

        {
            struct RestoreWorkingDirectory
            {
                fs::path path = fs::current_path();
                ~RestoreWorkingDirectory()
                {
                    std::error_code ignored;
                    fs::current_path( path, ignored );
                }
            } restore;
            fs::current_path( root );
            CatalogResourceAdapter relativeRoot( catalog, resources, "source", mappings );
            fs::current_path( other );
            CatalogResourceAdapter::Request relativeRequest;
            require(
                relativeRoot.resolve( uuid, relativeRequest, error ) &&
                    relativeRequest.resourceId == request.resourceId &&
                    relativeRoot.compile( relativeRequest ).succeeded() &&
                    relativeRoot.load( relativeRequest, error ),
                "Relative source binding must be captured once and survive working directory changes" );
        }

        CatalogResourceAdapter invalidRoot( catalog, resources, String( "\xc0\xaf" ), mappings );
        CatalogResourceAdapter::Request invalidRequest;
        require( !invalidRoot.resolve( uuid, invalidRequest, error ) && !error.empty(),
                 "Invalid UTF-8 root binding must reject without throwing during resolution" );

        CatalogResourceAdapter wrongRoot( catalog, resources, other.u8string().c_str(), mappings );
        CatalogResourceAdapter::Request rejected;
        require( !wrongRoot.resolve( uuid, rejected, error ) && !error.empty(),
                 "Different source and catalog roots must reject with diagnostics" );
        CatalogResourceAdapter wrongType( catalog, resources, config.sourceRoot,
                                          { { snapshot.type, ResourceTypeID( "texres" ) } } );
        require( !wrongType.resolve( uuid, rejected, error ),
                 "Type mismatch must reject instead of inventing a descriptor extension" );
        CatalogResourceAdapter unmapped( catalog, resources, config.sourceRoot, {} );
        require( !unmapped.resolve( uuid, rejected, error ),
                 "Unmapped catalog types must reject explicitly" );
        CatalogResourceAdapter duplicates( catalog, resources, config.sourceRoot,
                                           { mappings.front(), mappings.front() } );
        require( !duplicates.resolve( uuid, rejected, error ),
                 "Ambiguous catalog type mappings must reject" );
        require( !adapter.resolve( scene->getHandle()->getUUIDAsString(), rejected, error ),
                 "Scene entries cannot become file compilation requests" );
        require( !adapter.resolve( "missing-uuid", rejected, error ) && !rejected.resourceId.isValid(),
                 "Missing UUID must clear the request and report failure" );

        const auto originalHash = loaded->header.payloadHash;
        compiler->fail = true;
        CompilationOptions force;
        force.force = true;
        require( !adapter.compile( request, force ).succeeded(),
                 "Compiler failure must propagate through the catalog adapter" );
        resources->clearRuntimeCache();
        auto retained = adapter.load( request, error );
        require( retained && retained->header.payloadHash == originalHash,
                 "Failed compiler must leave the previously cooked resource loadable" );
        compiler->fail = false;

        compiler->version = 2;
        require( !adapter.load( request, error ) && !error.empty(),
                 "Catalog adapter must reject a stale compiler version" );
        require( adapter.compile( request ).succeeded(), "Version change must rebuild the resource" );
        require( adapter.load( request, error ) != nullptr, "Rebuilt version must load" );
        require( registry->unregisterCompiler( compiler->name() ), "Fixture compiler must unregister" );
        require( !adapter.resolve( uuid, rejected, error ), "Missing compiler must reject resolution" );
        require( registry->registerCompiler( compiler, &error ), error.c_str() );

        write( source / "renamed.ctest", "renamed payload\n" );
        compiler->duringCompile = [&] {
            sourceAsset->setFilePath( "renamed.ctest" );
            catalog->updateResourceEntry( sourceAsset );
        };
        require( !adapter.compile( request, force ).succeeded(),
                 "Deterministic catalog mutation during compile must reject the stale result" );
        compiler->duringCompile = {};
        require( !adapter.isCurrent( request, error ) && !adapter.load( request, error ),
                 "Renamed UUID must invalidate old compile and load requests" );
        require( adapter.resolve( uuid, rejected, error ) &&
                     rejected.resourceId == ResourceID( "data://renamed.ctest" ) &&
                     rejected.entry.uuid == uuid,
                 "Rename must preserve durable UUID and explicitly change ResourceID" );
        request = rejected;
        require( adapter.compile( request ).succeeded() && adapter.load( request, error ),
                 "Renamed catalog source must compile and load through its new ID" );
        catalog->unload( nullptr );
        require( !adapter.load( request, error ), "Unloaded catalog must reject old load requests" );
        catalog->loadFromFile( catalogPath );
        require( !adapter.isCurrent( request, error ), "Reopen must not revive an old generation" );
        require( adapter.resolve( uuid, rejected, error ) && rejected.resourceId == request.resourceId,
                 "Renamed UUID and source mapping must survive catalog reopen" );
        require( adapter.compile( rejected ).status == CompilationStatus::UpToDate,
                 "Reopened catalog must reuse the existing compilation index" );
#ifdef _WIN32
        write( source / "Upper.CTEST", "uppercase extension payload\n" );
        auto uppercase = asset( "Upper.CTEST" );
        catalog->addResourceEntry( uppercase );
        require( adapter.resolve( uppercase->getHandle()->getUUIDAsString(), request, error ) &&
                     request.resourceId == ResourceID( "data://Upper.ctest" ),
                 "Windows extension case folding must preserve the same physical source identity" );
        require( adapter.compile( request ).succeeded() && adapter.load( request, error ),
                 "Existing uppercase-extension source must compile and load through ResourceSystem" );
#endif

        resources->shutdown();
        catalog->unload( nullptr );
    }
}  // namespace catalog_adapter_contracts

inline void catalogResourceAdapterContracts( const std::filesystem::path &folder )
{
    catalog_adapter_contracts::run( folder );
}

#endif  // WPCatalogResourceAdapterContracts_h__
