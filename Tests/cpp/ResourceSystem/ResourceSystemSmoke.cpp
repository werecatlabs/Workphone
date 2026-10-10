#include "../ResourceRuntimeContracts.hpp"
#include <Workphone/System/ResourceSystem.hpp>
#include <Workphone/System/ResourceCompilerRegistry.hpp>
#include <WPSQLite/ResourceCompilationDatabase.hpp>

#include <algorithm>
#include <chrono>
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

    class FaultDatabase final : public IResourceCompilationDatabase
    {
    public:
        ResourceCompilationDatabase inner;
        bool rejectCommit=false;
        bool connect(const String &path) override { return inner.connect(path); }
        void disconnect() override { inner.disconnect(); }
        bool isConnected() const override { return inner.isConnected(); }
        String getLastError() const override { return rejectCommit?"Injected index commit failure":inner.getLastError(); }
        bool reset() override { return inner.reset(); }
        bool getRecord(const String &id,CompiledResourceRecord &record) const override { return inner.getRecord(id,record); }
        bool commitCompilation(const CompiledResourceRecord &record,const Array<CompileDependencyRecord> &edges) override
        { return !rejectCommit && inner.commitCompilation(record,edges); }
        bool removeRecord(const String &id) override { return inner.removeRecord(id); }
        bool getDependencies(const String &id,Array<CompileDependencyRecord> &edges) const override
        { return inner.getDependencies(id,edges); }
        bool getDependents(const String &path,Array<String> &ids) const override { return inner.getDependents(path,ids); }
    };

    void require( bool condition, const std::string &message )
    {
        if( !condition )
            throw std::runtime_error( message );
    }

    void writeFile( const fs::path &path, const std::string &contents )
    {
        std::ofstream stream( path, std::ios::binary | std::ios::trunc );
        stream.write( contents.data(), static_cast<std::streamsize>( contents.size() ) );
        require( stream.good(), "Failed to write test input" );
    }

    std::string reportMessage( const CompilationReport &report )
    {
        std::string message = "status=" + std::to_string( static_cast<int>( report.status ) );
        for( const auto &entry : report.messages )
            message += std::string( "; " ) + entry.c_str();
        return message;
    }

    class TextCompiler final : public IResourceCompiler
    {
    public:
        String name() const override
        {
            return "SmokeTextCompiler";
        }

        Array<CompilerOutput> outputs() const override
        {
            return { { ResourceTypeID( "txtres" ), 11 } };
        }

        bool getDependencies( const CompileContext &context, DependencySet &dependencies,
                              String &error ) const override
        {
            std::ifstream input( context.sourcePath.c_str() );
            if( !input )
            {
                error = "Unable to scan descriptor";
                return false;
            }

            std::string line;
            while( std::getline( input, line ) )
            {
                if( line.rfind( "resource=", 0 ) == 0 )
                {
                    ResourceID id( line.substr( 9 ).c_str() );
                    if( !id.isValid() )
                    {
                        error = "Invalid resource dependency";
                        return false;
                    }
                    dependencies.compileDependencies.push_back( CompileDependency::resource( id ) );
                }
                else if( line.rfind( "install=", 0 ) == 0 )
                {
                    ResourceID id( line.substr( 8 ).c_str() );
                    if( !id.isValid() )
                    {
                        error = "Invalid install dependency";
                        return false;
                    }
                    dependencies.installDependencies.push_back( id );
                }
                else if( line.rfind( "data=", 0 ) == 0 )
                {
                    dependencies.compileDependencies.push_back(
                        CompileDependency::data( line.substr( 5 ).c_str() ) );
                }
            }
            return true;
        }

        CompilationStatus compile( const CompileContext &context, std::ostream &output,
                                   Array<String> & ) const override
        {
            std::ifstream input( context.sourcePath.c_str(), std::ios::binary );
            if( !input )
                return CompilationStatus::Failure;
            output << input.rdbuf();
            input.close();
            if(context.resourceId.str()=="data://mutating.txtres")
                writeFile(fs::u8path(context.sourcePath.c_str()),"changed-during-compile\n");
            return output ? CompilationStatus::Success : CompilationStatus::Failure;
        }
    };
}  // namespace

int main()
{
    using namespace workphone;
    using namespace workphone::resource;

    fs::path root;
    try
    {
        const ResourceID canonicalId( "DATA://models\\car.mesh:lod0.mesh" );
        require( canonicalId.compiledRelativePath() == "models/car_lod0.mesh",
                 std::string( "Resource ID canonicalization failed: " ) + canonicalId.str().c_str() +
                     " => " + canonicalId.compiledRelativePath().c_str() );
        require( !ResourceID( "data://../escape.mesh" ).isValid(),
                 "Traversal resource ID was accepted" );

        const auto unique = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        root = fs::temp_directory_path() /
               ( std::string( "lioncat_wpresource_smoke_" ) + std::to_string( unique ) );
        const fs::path source = root / "source";
        const fs::path compiled = root / "compiled";
        fs::create_directories( source / "data" );
        fs::create_directories( compiled );

        writeFile( source / "dependency.txtres",
                   "data=data://data/shared.txt\n"
                   "dependency payload\n" );
        writeFile( source / "data" / "shared.txt", "shared-v1\n" );
        writeFile( source / "root.txtres",
                   "resource=data://dependency.txtres\n"
                   "install=data://dependency.txtres\n"
                   "root payload\n" );

        std::shared_ptr<IResourceCompilerRegistry> registry =
            std::make_shared<ResourceCompilerRegistry>();
        String error;
        require( registry->registerCompiler( std::make_shared<TextCompiler>(), &error ), error.c_str() );
        require( !registry->registerCompiler( std::make_shared<TextCompiler>(), &error ),
                 "Duplicate compiler registration was accepted" );

        auto database = std::make_shared<FaultDatabase>();
        ResourceSystem system( registry, database );
        ResourceSystemConfig config;
        config.sourceRoot = source.u8string().c_str();
        config.compiledRoot = compiled.u8string().c_str();
        config.target = "smoke";
        error.clear();
        require( system.initialize( config, error ), error.c_str() );

        const ResourceID rootId( "data://root.txtres" );
        const auto first = system.compile( rootId );
        require( first.status == CompilationStatus::Success,
                 std::string( "Initial compilation failed: " ) + reportMessage( first ) );
        require( fs::is_regular_file( fs::u8path(system.compile(ResourceID("data://dependency.txtres")).outputPath.c_str()) ),
                 "Resource dependency was not compiled" );
        require( fs::is_regular_file( fs::u8path(first.outputPath.c_str()) ), "Root resource was not compiled" );

        const auto second = system.compile( rootId );
        require( second.status == CompilationStatus::UpToDate,
                 "Unchanged resource was not detected as up to date" );

        error.clear();
        auto loaded = system.load( rootId, error );
        require( loaded != nullptr, error.c_str() );
        require( loaded->dependencies.size() == 1,
                 "Install dependency was not loaded with the root resource" );

        writeFile( source / "data" / "shared.txt", "shared-v2\n" );
        const auto affected = system.compileAffected( "data://data/shared.txt" );
        require( affected.size() == 2, "Transitive affected-resource set was incomplete" );
        const auto rootRebuild = std::find_if(
            affected.begin(), affected.end(),
            [&]( const CompilationReport &report ) { return report.resourceId == rootId; } );
        require( rootRebuild != affected.end() && rootRebuild->status == CompilationStatus::Success,
                 "Changed raw dependency did not rebuild its transitive dependent" );
        require( rootRebuild->sourceHash != first.sourceHash,
                 "Changed transitive dependency did not alter the root source hash" );

        system.unload( rootId );
        std::fstream corrupt( fs::u8path(rootRebuild->outputPath.c_str()),
                              std::ios::binary | std::ios::in | std::ios::out );
        require( static_cast<bool>( corrupt ), "Failed to open output for corruption test" );
        corrupt.seekp( -1, std::ios::end );
        const char badByte = 0;
        corrupt.write( &badByte, 1 );
        corrupt.close();
        error.clear();
        require( !system.load( rootId, error ), "Corrupt payload was accepted" );

        const auto repaired = system.compile( rootId );
        require( repaired.status == CompilationStatus::Success,
                 "Corrupt compiled output was not automatically rebuilt" );
        error.clear();
        require( system.load( rootId, error ) != nullptr, error.c_str() );

        writeFile( source / "cycle_a.txtres", "resource=data://cycle_b.txtres\n" );
        writeFile( source / "cycle_b.txtres", "resource=data://cycle_a.txtres\n" );
        const auto cycle = system.compile( ResourceID( "data://cycle_a.txtres" ) );
        require( cycle.status == CompilationStatus::Failure, "Resource dependency cycle was accepted" );

        writeFile( source / "quote's.txtres", "quoted resource payload\n" );
        require( system.compile( ResourceID( "data://quote's.txtres" ) ).succeeded(),
                 "Prepared database statements did not handle a quoted resource ID" );

        fs::create_directories(source / "models");
        writeFile(source / "models" / "car.mesh","subasset payload\n");
        writeFile(source / "models" / "car_lod0.txtres","independent payload\n");
        const auto subasset=system.compile(ResourceID("data://models/car.mesh:lod0.txtres"));
        const auto alias=system.compile(ResourceID("data://models/car_lod0.txtres"));
        require(subasset.succeeded(),"Subasset: "+reportMessage(subasset));
        require(alias.succeeded(),"Alias: "+reportMessage(alias));
        require(subasset.succeeded() && alias.succeeded() && subasset.outputPath!=alias.outputPath &&
                fs::is_regular_file(fs::u8path(subasset.outputPath.c_str())),"Flattened subasset aliases must retain independent artifacts");
        writeFile(source / "case.txtres","case fixture\n");
        writeFile(source / "CASE.txtres","case fixture\n");
        const auto lower=system.compile(ResourceID("data://case.txtres"));
        const auto upper=system.compile(ResourceID("data://CASE.txtres"));
        require(lower.succeeded() && upper.succeeded() &&
                !fs::equivalent(fs::u8path(lower.outputPath.c_str()),fs::u8path(upper.outputPath.c_str())),
                "Case-distinct logical IDs must not alias on Windows artifact storage");
        writeFile(source / "generation.txtres","last-good\n");
        const ResourceID generationId("data://generation.txtres");
        const auto good=system.compile(generationId);
        require(good.succeeded(),"Generation baseline");
        const auto pinnedManifest=compiled / "pinned.wprm";
        require(system.writeRuntimeManifest(pinnedManifest.u8string().c_str(),{generationId},error),error.c_str());
        writeFile(source / "generation.txtres","replacement\n");
        database->rejectCommit=true;
        require(!system.compile(generationId).succeeded(),"Injected database failure must fail build");
        database->rejectCommit=false;
        system.clearRuntimeCache();
        auto preserved=system.load(generationId,error);
        require(preserved && std::string(preserved->payload.begin(),preserved->payload.end())=="last-good\n",
                "Index commit failure must retain last-good disk generation after cache eviction");
        const auto replacement=system.compile(generationId);
        require(replacement.succeeded() && replacement.outputPath!=good.outputPath &&
                fs::is_regular_file(fs::u8path(good.outputPath.c_str())),"Successful replacement must not overwrite prior generation");
        CompilationOptions packaged;
        packaged.packagedBuild=true;
        const auto packageVariant=system.compile(generationId,packaged);
        require(packageVariant.succeeded() && packageVariant.outputPath!=replacement.outputPath,
                "Packaged and Editor variants must coexist on disk");
        auto targetDatabase=std::make_shared<ResourceCompilationDatabase>();
        ResourceSystem otherTarget(registry,targetDatabase);
        auto otherConfig=config;
        otherConfig.target="another-target";
        require(otherTarget.initialize(otherConfig,error),error.c_str());
        const auto targetVariant=otherTarget.compile(generationId);
        require(targetVariant.succeeded() && targetVariant.outputPath!=replacement.outputPath,
                "Targets sharing a cooked root must retain independent artifacts");
        otherTarget.shutdown();
        ResourceSystem pinnedRuntime(nullptr,nullptr);
        RuntimeResourceConfig runtimeConfig;
        runtimeConfig.compiledRoot=config.compiledRoot;
        runtimeConfig.manifestPath=pinnedManifest.u8string().c_str();
        runtimeConfig.target=config.target;
        require(pinnedRuntime.initializeRuntime(runtimeConfig,error),error.c_str());
        auto pinned=pinnedRuntime.load(generationId,error);
        require(pinned && std::string(pinned->payload.begin(),pinned->payload.end())=="last-good\n",
                "An existing manifest must retain its exact generation across builds, modes and targets");
        pinnedRuntime.shutdown();
        writeFile(source / "mutating.txtres","original\n");
        require(!system.compile(ResourceID("data://mutating.txtres")).succeeded(),
                "Compiler-time source changes must reject publication");
        CompiledResourceRecord uncommitted;
        require(database->getRecord("data://mutating.txtres",uncommitted) && !uncommitted.isValid(),
                "Changed-input compilation must not create a committed index record");

        system.shutdown();
        error.clear();
        require( system.initialize( config, error ), error.c_str() );
        require( system.compile( rootId ).status == CompilationStatus::UpToDate,
                 "Persistent compilation metadata was not reused after restart" );

        system.shutdown();
        std::error_code ignored;
        fs::remove_all( root, ignored );
        resource_runtime_contracts::run();
        std::cout << "WPResource authoring and cooked-only runtime contracts passed" << std::endl;
        return 0;
    }
    catch( const std::exception &exception )
    {
        std::error_code ignored;
        if( !root.empty() )
            fs::remove_all( root, ignored );
        std::cerr << "WPResource smoke test failure: " << exception.what() << std::endl;
        return 1;
    }
}
