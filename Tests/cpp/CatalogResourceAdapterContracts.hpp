#ifndef WPCatalogResourceAdapterContracts_h__
#define WPCatalogResourceAdapterContracts_h__

#include <Workphone/Database/CatalogResourceAdapter.hpp>
#include <Workphone/System/Resource.hpp>
#include <Workphone/System/ResourceCompilerRegistry.hpp>
#include <Workphone/System/ResourceSystem.hpp>
#include <Workphone/Database/AssetPackagePolicy.hpp>
#include <Workphone/Interface/Database/IParameterizedDatabase.hpp>
#include <WPSQLite/ResourceCompilationDatabase.hpp>
#include <WPSQLite/SQLiteDatabase.hpp>

#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
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

    class ObservedDatabase final : public SQLiteDatabase
    {
    public:
        std::function<void(const String &)> afterQuery;
        SmartPtr<IDatabaseQuery> queryBound(const String &sql,const Array<String> &values) override
        {
            auto result = SQLiteDatabase::queryBound(sql,values);
            if(result && afterQuery) afterQuery(sql);
            return result;
        }
    };

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
        auto dependencyAsset = asset( "dependency.ctest" );
        catalog->addResourceEntry( dependencyAsset );
        const auto dependencyUUID = dependencyAsset->getHandle()->getUUIDAsString();
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
        const auto initialCompilation=adapter.compile(request);
        if(!initialCompilation.succeeded())
            for(const auto &message:initialCompilation.messages) std::cerr << message << '\n';
        require( initialCompilation.succeeded(), "Catalog request must compile its resource" );
        auto loaded = adapter.load( request, error );
        require( loaded && loaded->dependencies.size() == 1,
                 "Catalog load must use ResourceSystem install dependency loading" );
        require( adapter.compile( request ).status == CompilationStatus::UpToDate,
                 "Catalog bridge must reuse existing compilation metadata" );

        RuntimeResourceBinding binding;
        require( adapter.runtimeBinding( request, binding, error ) && binding.uuid == uuid &&
                 binding.resourceId == request.resourceId,
                 "Export bindings must retain the authored catalog UUID and exact resource identity" );
        CompilationOptions packaged;
        packaged.packagedBuild = true;
        require( adapter.compile( request, packaged ).succeeded(), "Catalog package compilation failed" );
        const auto packagePath = root / "uuid-package";
        require( resources->writeRuntimePackage( packagePath.u8string().c_str(), { request.resourceId }, { binding }, error ),
                 error.c_str() );
        ResourceSystem packagedRuntime( nullptr, nullptr );
        RuntimeResourceConfig runtimeConfig;
        runtimeConfig.compiledRoot = packagePath.u8string().c_str();
        runtimeConfig.manifestPath = ( packagePath / "runtime.wprm" ).u8string().c_str();
        runtimeConfig.target = config.target;
        require( packagedRuntime.initializeRuntime( runtimeConfig, error ) && packagedRuntime.loadUUID( uuid, error ),
                 "Catalog UUID must load through a compiler/catalog-free package mount" );
        auto packagedOriginal = packagedRuntime.loadUUID( uuid, error );
        const auto checkedPath = root / "catalog-admitted-package";
        require(adapter.writeRuntimePackage(checkedPath.u8string().c_str(),{request},error),error.c_str());
        ResourceSystem checkedRuntime(nullptr,nullptr);
        auto checkedConfig = runtimeConfig;
        checkedConfig.compiledRoot = checkedPath.u8string().c_str();
        checkedConfig.manifestPath = (checkedPath / "runtime.wprm").u8string().c_str();
        require(checkedRuntime.initializeRuntime(checkedConfig,error),error.c_str());
        auto checkedRoot = checkedRuntime.loadUUID(uuid,error);
        require(checkedRoot && checkedRuntime.loadUUID(dependencyUUID,error) == checkedRoot->dependencies[0],
                "Catalog export must map every transitive install dependency to its durable UUID");
        const auto rejectedPath = root / "rejected-catalog-package";
        AssetPackagePolicy dependencyPolicy;
        require(getAssetPackagePolicy(catalog,dependencyUUID,dependencyPolicy,error) &&
                setAssetPackagePolicy(catalog,dependencyPolicy.entry,AssetPackageUsage::EditorOnly,error),error.c_str());
        require(!adapter.writeRuntimePackage(rejectedPath.u8string().c_str(),{request},error) && !fs::exists(rejectedPath),
                "Catalog export must reject expired root requests");
        require(adapter.resolve(uuid,request,error),error.c_str());
        require(!adapter.writeRuntimePackage(rejectedPath.u8string().c_str(),{request},error) &&
                error.find("Editor-only") != String::npos && error.find("dependency.ctest") != String::npos &&
                !fs::exists(rejectedPath),"Editor-only transitive dependencies must block publication with identity diagnostics");
        require(checkedRuntime.loadUUID(dependencyUUID,error) == checkedRoot->dependencies[0],
                "Policy changes must not mutate an already exported package");
        require(getAssetPackagePolicy(catalog,dependencyUUID,dependencyPolicy,error) &&
                setAssetPackagePolicy(catalog,dependencyPolicy.entry,AssetPackageUsage::Excluded,error) &&
                adapter.resolve(uuid,request,error),error.c_str());
        require(!adapter.writeRuntimePackage(rejectedPath.u8string().c_str(),{request},error) &&
                error.find("excluded") != String::npos,"Explicit excluded dependencies must block packaging");
        require(getAssetPackagePolicy(catalog,dependencyUUID,dependencyPolicy,error) &&
                setAssetPackagePolicy(catalog,dependencyPolicy.entry,AssetPackageUsage::Runtime,error) &&
                adapter.resolve(uuid,request,error),error.c_str());
        auto db = catalog->getDatabase();
        auto bound = dynamic_cast<IParameterizedDatabase *>(db.get());
        require(bound->queryBound("DELETE FROM resources WHERE uuid=?",{dependencyUUID}) != nullptr &&
                adapter.resolve(uuid,request,error),"Remove dependency catalog identity");
        require(!adapter.writeRuntimePackage(rejectedPath.u8string().c_str(),{request},error) &&
                error.find("not a cataloged") != String::npos && !fs::exists(rejectedPath),
                "Uncataloged cooked dependencies must not silently ship by path");
        catalog->addResourceEntry(dependencyAsset);
        require(adapter.resolve(uuid,request,error),error.c_str());
        require(bound->queryBound("UPDATE resources SET type='Unmapped' WHERE uuid=?",{dependencyUUID}) != nullptr &&
                adapter.resolve(uuid,request,error),"Change dependency catalog type");
        require(!adapter.writeRuntimePackage(rejectedPath.u8string().c_str(),{request},error) && !fs::exists(rejectedPath),
                "Every package member requires a compatible explicit type mapping");
        require(bound->queryBound("UPDATE resources SET type=? WHERE uuid=?",{snapshot.type,dependencyUUID}) != nullptr &&
                adapter.resolve(uuid,request,error),"Restore dependency catalog type");
        require(bound->queryBound("UPDATE wp_asset_package_schema SET version=2",{}) != nullptr &&
                adapter.resolve(uuid,request,error),"Seed future package policy version");
        require(!adapter.writeRuntimePackage(rejectedPath.u8string().c_str(),{request},error) && !fs::exists(rejectedPath),
                "Unsupported shipping metadata must prevent catalog export");
        require(bound->queryBound("UPDATE wp_asset_package_schema SET version=1",{}) != nullptr &&
                adapter.resolve(uuid,request,error),"Restore package policy version");
        require(adapter.writeRuntimePackage(rejectedPath.u8string().c_str(),{request},error),error.c_str());
        // Observe a real policy read inside the production admission, then try
        // independent and same-connection work while publication is still pending.
        auto observed = make_ptr<ObservedDatabase>();
        db->close();
        observed->loadFromFile(catalogPath);
        catalog->setDatabase(observed);
        require(adapter.resolve(uuid,request,error),error.c_str());
        auto independent = core::IApplicationManager::instance()->getFactoryManager()->make_object<IDatabase>();
        independent->loadFromFile(catalogPath);
        auto independentBound = dynamic_cast<IParameterizedDatabase *>(independent.get());
        require(independentBound->queryBound("PRAGMA busy_timeout=0",{}) != nullptr,"Configure nonblocking writer probe");
        const auto fencedPath = root / "fenced-catalog-package";
        bool observedAdmission = false;
        std::future<bool> sameConnection;
        observed->afterQuery = [&](const String &statement) {
            if(observedAdmission || statement.find("SELECT usage FROM wp_asset_package_usage") != 0) return;
            observedAdmission = true;
            require(!independentBound->queryBound("UPDATE wp_asset_package_usage SET usage='excluded' WHERE uuid=?",{dependencyUUID}),
                    "Independent SQLite writers must be excluded while package admission is active");
            std::promise<void> started;
            auto ready = started.get_future();
            sameConnection = std::async(std::launch::async,[&,signal=std::move(started)]() mutable {
                signal.set_value();
                auto result = observed->queryBound("SELECT uuid FROM resources",{});
                return result != nullptr && fs::exists(fencedPath / "runtime.wprm");
            });
            ready.get();
            require(sameConnection.wait_for(std::chrono::milliseconds(30)) == std::future_status::timeout,
                    "Same-connection work must wait through package publication");
        };
        require(adapter.writeRuntimePackage(fencedPath.u8string().c_str(),{request},error),error.c_str());
        require(observedAdmission && sameConnection.valid() && sameConnection.get(),
                "Connection admission must remain held until the complete package is visible");
        observed->afterQuery = {};
        require(independentBound->queryBound("UPDATE wp_asset_package_usage SET usage='runtime' WHERE uuid=?",{dependencyUUID}) != nullptr,
                "Independent writers must resume after package publication");
        independent->close();
        require(adapter.resolve(uuid,request,error),error.c_str());

        auto readOnly = make_ptr<AssetDatabaseManager>();
        require(readOnly->setProjectRoot(config.sourceRoot) && readOnly->openReadOnly(catalogPath,error),error.c_str());
        CatalogResourceAdapter readOnlyAdapter(readOnly,resources,config.sourceRoot,mappings);
        CatalogResourceAdapter::Request readOnlyRequest;
        require(readOnlyAdapter.resolve(uuid,readOnlyRequest,error) &&
                !readOnlyAdapter.writeRuntimePackage((root / "readonly-package").u8string().c_str(),{readOnlyRequest},error),
                "Catalog export must reject read-only mutation admission explicitly");
        readOnly->unload(nullptr);
        write(other / "root.ctest","other service payload\n");
        auto otherResources = std::make_shared<ResourceSystem>(registry,std::make_shared<ResourceCompilationDatabase>());
        auto otherConfig = config;
        otherConfig.sourceRoot = other.u8string().c_str();
        otherConfig.compiledRoot = (root / "other-compiled").u8string().c_str();
        require(otherResources->initialize(otherConfig,error) && otherResources->compile(request.resourceId,packaged).succeeded(),
                "Build alternate actual source root");
        CatalogResourceAdapter wrongService(catalog,otherResources,config.sourceRoot,mappings);
        require(!wrongService.writeRuntimePackage((root / "wrong-source-package").u8string().c_str(),{request},error) &&
                error.find("source root") != String::npos,"Catalog export must verify the actual ResourceSystem source root");
        otherResources->shutdown();
        checkedRuntime.shutdown();
        // Return to the original authoring mode for the existing contracts below.
        require( adapter.compile( request ).succeeded(), "Catalog Editor-mode selection failed" );

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
        require( !adapter.runtimeBinding( request, binding, error ) && binding.uuid.empty() &&
                 !binding.resourceId.isValid(), "Stale requests must not return exportable UUID bindings" );
        require( packagedRuntime.loadUUID( uuid, error ) == packagedOriginal &&
                 packagedOriginal->header.resourceId == ResourceID( "data://root.ctest" ),
                 "Authoring rename must not change an already exported UUID package generation" );
        require( adapter.resolve( uuid, rejected, error ) &&
                     rejected.resourceId == ResourceID( "data://renamed.ctest" ) &&
                     rejected.entry.uuid == uuid,
                 "Rename must preserve durable UUID and explicitly change ResourceID" );
        request = rejected;
        require( adapter.compile( request ).succeeded() && adapter.load( request, error ),
                 "Renamed catalog source must compile and load through its new ID" );
        require( adapter.runtimeBinding( request, binding, error ) && binding.uuid == uuid &&
                 binding.resourceId == ResourceID( "data://renamed.ctest" ),
                 "New export bindings must preserve UUID across source renames" );
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

        packagedRuntime.shutdown();
        resources->shutdown();
        catalog->unload( nullptr );
    }
}  // namespace catalog_adapter_contracts

inline void catalogResourceAdapterContracts( const std::filesystem::path &folder )
{
    catalog_adapter_contracts::run( folder );
}

#endif  // WPCatalogResourceAdapterContracts_h__
