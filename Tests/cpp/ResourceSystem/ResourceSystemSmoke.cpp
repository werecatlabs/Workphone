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

        auto database = std::make_shared<ResourceCompilationDatabase>();
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
        require( fs::is_regular_file( compiled / "dependency.txtres" ),
                 "Resource dependency was not compiled" );
        require( fs::is_regular_file( compiled / "root.txtres" ), "Root resource was not compiled" );

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
        std::fstream corrupt( compiled / "root.txtres",
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
