#include "UnitTests.hpp"

#include <Workphone/System/ResourceSystem.hpp>
#include <Workphone/System/ResourceCompilerRegistry.hpp>
#include <WPSQLite/ResourceCompilationDatabase.hpp>
#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>

namespace
{
    namespace fs = std::filesystem;
    using namespace workphone;
    using namespace workphone::resource;

    class TextResourceCompiler final : public IResourceCompiler
    {
    public:
        String name() const override
        {
            return "TextResourceCompiler";
        }

        Array<CompilerOutput> outputs() const override
        {
            return { { ResourceTypeID( "txtres" ), 7 } };
        }

        bool getDependencies( const CompileContext &context, DependencySet &dependencies,
                              String &error ) const override
        {
            std::ifstream source( context.sourcePath.c_str() );
            if( !source )
            {
                error = "Failed to read text resource descriptor";
                return false;
            }

            std::string line;
            while( std::getline( source, line ) )
            {
                if( line.rfind( "resource=", 0 ) == 0 )
                {
                    ResourceID id( line.substr( 9 ).c_str() );
                    if( !id.isValid() )
                    {
                        error = "Invalid resource dependency in descriptor";
                        return false;
                    }
                    dependencies.compileDependencies.push_back( CompileDependency::resource( id ) );
                }
                else if( line.rfind( "data=", 0 ) == 0 )
                {
                    dependencies.compileDependencies.push_back(
                        CompileDependency::data( line.substr( 5 ).c_str() ) );
                }
                else if( line.rfind( "install=", 0 ) == 0 )
                {
                    ResourceID id( line.substr( 8 ).c_str() );
                    if( !id.isValid() )
                    {
                        error = "Invalid install dependency in descriptor";
                        return false;
                    }
                    dependencies.installDependencies.push_back( id );
                }
            }
            return true;
        }

        CompilationStatus compile( const CompileContext &context, std::ostream &output,
                                   Array<String> & ) const override
        {
            std::ifstream source( context.sourcePath.c_str(), std::ios::binary );
            if( !source )
                return CompilationStatus::Failure;

            std::string contents( ( std::istreambuf_iterator<char>( source ) ),
                                  std::istreambuf_iterator<char>() );
            std::transform( contents.begin(), contents.end(), contents.begin(),
                            []( unsigned char c ) { return static_cast<char>( std::toupper( c ) ); } );
            output.write( contents.data(), static_cast<std::streamsize>( contents.size() ) );
            return output ? CompilationStatus::Success : CompilationStatus::Failure;
        }
    };

    struct ResourceSystemFixture
    {
        ResourceSystemFixture()
        {
            const auto unique = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            root = fs::temp_directory_path() /
                   ( std::string( "lioncat_resource_system_" ) + std::to_string( unique ) );
            source = root / "source";
            compiled = root / "compiled";
            fs::create_directories( source / "data" );
            fs::create_directories( compiled );

            write( source / "dependency.txtres",
                   "data=data://data/shared.txt\n"
                   "child payload\n" );
            write( source / "data" / "shared.txt", "shared-v1\n" );
            write( source / "root.txtres",
                   "resource=data://dependency.txtres\n"
                   "install=data://dependency.txtres\n"
                   "root payload\n" );

            registry = std::make_shared<ResourceCompilerRegistry>();
            String registryError;
            BOOST_REQUIRE(
                registry->registerCompiler( std::make_shared<TextResourceCompiler>(), &registryError ) );

            auto database = std::make_shared<ResourceCompilationDatabase>();
            system = std::make_unique<ResourceSystem>( registry, database );
            ResourceSystemConfig config;
            config.sourceRoot = source.u8string().c_str();
            config.compiledRoot = compiled.u8string().c_str();
            config.target = "tests";
            String error;
            BOOST_REQUIRE_MESSAGE( system->initialize( config, error ), error.c_str() );
        }

        ~ResourceSystemFixture()
        {
            system.reset();
            std::error_code ignored;
            fs::remove_all( root, ignored );
        }

        static void write( const fs::path &path, const std::string &contents )
        {
            std::ofstream stream( path, std::ios::binary | std::ios::trunc );
            stream.write( contents.data(), static_cast<std::streamsize>( contents.size() ) );
            BOOST_REQUIRE( stream.good() );
        }

        fs::path root;
        fs::path source;
        fs::path compiled;
        std::shared_ptr<IResourceCompilerRegistry> registry;
        std::unique_ptr<ResourceSystem> system;
    };
}  // namespace

BOOST_AUTO_TEST_CASE( production_resource_ids_are_canonical_and_safe )
{
    using namespace workphone::resource;

    ResourceID id( "DATA://characters\\hero.mesh:lod0.mesh" );
    BOOST_REQUIRE( id.isValid() );
    BOOST_CHECK_EQUAL( id.str(), "data://characters/hero.mesh:lod0.mesh" );
    BOOST_CHECK_EQUAL( id.type().str(), "mesh" );
    BOOST_CHECK_EQUAL( id.sourceRelativePath(), "characters/hero.mesh" );
    BOOST_CHECK_EQUAL( id.compiledRelativePath(), "characters/hero_lod0.mesh" );
    BOOST_CHECK_EQUAL( id.parent().str(), "data://characters/hero.mesh" );

    BOOST_CHECK( !ResourceID( "data://../secrets.mesh" ).isValid() );
    BOOST_CHECK( !ResourceID( "C:/absolute.mesh" ).isValid() );
    BOOST_CHECK( !ResourceID( "data://missing_extension" ).isValid() );
    BOOST_CHECK( !ResourceID( "data://bad.extensiontoolong" ).isValid() );
}

BOOST_FIXTURE_TEST_CASE( production_resource_system_compiles_caches_and_loads, ResourceSystemFixture )
{
    ResourceID rootId( "data://root.txtres" );
    auto first = system->compile( rootId );
    BOOST_REQUIRE( first.succeeded() );
    BOOST_CHECK( first.status == CompilationStatus::Success );
    BOOST_CHECK( fs::is_regular_file( fs::u8path(first.outputPath.c_str()) ) );
    BOOST_CHECK( fs::is_regular_file( fs::u8path(system->compile(ResourceID("data://dependency.txtres")).outputPath.c_str()) ) );

    String loadError;
    auto loaded = system->load( rootId, loadError );
    BOOST_REQUIRE_MESSAGE( loaded, loadError.c_str() );
    BOOST_CHECK_EQUAL( loaded->header.resourceId.str(), rootId.str() );
    BOOST_CHECK_EQUAL( loaded->dependencies.size(), 1u );
    BOOST_CHECK( !loaded->payload.empty() );

    auto second = system->compile( rootId );
    BOOST_REQUIRE( second.succeeded() );
    BOOST_CHECK( second.status == CompilationStatus::UpToDate );
    BOOST_CHECK_EQUAL( second.sourceHash, first.sourceHash );
    BOOST_CHECK_EQUAL( second.outputHash, first.outputHash );
}

BOOST_FIXTURE_TEST_CASE( production_resource_system_rebuilds_transitive_changes, ResourceSystemFixture )
{
    ResourceID rootId( "data://root.txtres" );
    auto initial = system->compile( rootId );
    BOOST_REQUIRE( initial.succeeded() );

    write( source / "data" / "shared.txt", "shared-v2\n" );
    const auto affected = system->compileAffected( "data://data/shared.txt" );
    BOOST_REQUIRE_EQUAL( affected.size(), 2u );
    const auto rootRebuild =
        std::find_if( affected.begin(), affected.end(),
                      [&]( const CompilationReport &report ) { return report.resourceId == rootId; } );
    BOOST_REQUIRE( rootRebuild != affected.end() );
    BOOST_REQUIRE( rootRebuild->succeeded() );
    BOOST_CHECK( rootRebuild->status == CompilationStatus::Success );
    BOOST_CHECK_NE( rootRebuild->sourceHash, initial.sourceHash );
}

BOOST_FIXTURE_TEST_CASE( production_resource_system_rejects_corrupt_payloads, ResourceSystemFixture )
{
    ResourceID rootId( "data://root.txtres" );
    const auto cooked=system->compile(rootId);
    BOOST_REQUIRE( cooked.succeeded() );

    auto outputPath = fs::u8path(cooked.outputPath.c_str());
    std::fstream output( outputPath, std::ios::binary | std::ios::in | std::ios::out );
    BOOST_REQUIRE( output );
    output.seekp( -1, std::ios::end );
    char byte = 0;
    output.write( &byte, 1 );
    output.close();

    system->unload( rootId );
    String error;
    BOOST_CHECK( !system->load( rootId, error ) );
    BOOST_CHECK( !error.empty() );

    const auto repaired = system->compile( rootId );
    BOOST_REQUIRE( repaired.succeeded() );
    BOOST_CHECK( repaired.status == CompilationStatus::Success );
    error.clear();
    BOOST_CHECK( system->load( rootId, error ) );
}

BOOST_AUTO_TEST_CASE( production_resource_registry_rejects_type_collisions )
{
    using namespace workphone;
    using namespace workphone::resource;

    ResourceCompilerRegistry registry;
    String error;
    BOOST_REQUIRE( registry.registerCompiler( std::make_shared<TextResourceCompiler>(), &error ) );
    error.clear();
    BOOST_CHECK( !registry.registerCompiler( std::make_shared<TextResourceCompiler>(), &error ) );
    BOOST_CHECK( !error.empty() );
}
