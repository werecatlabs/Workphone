#ifndef WP_RESOURCE_RUNTIME_CONTRACTS_HPP
#define WP_RESOURCE_RUNTIME_CONTRACTS_HPP

#include <Workphone/System/ResourceSystem.hpp>
#include <Workphone/System/ResourceCompilerRegistry.hpp>
#include <WPSQLite/ResourceCompilationDatabase.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <map>
#include <sstream>
#include <stdexcept>

namespace resource_runtime_contracts
{
    namespace fs = std::filesystem;
    using namespace workphone;
    using namespace workphone::resource;

    inline void require( bool condition, const String &message )
    {
        if( !condition )
            throw std::runtime_error( message.c_str() );
    }

    inline String pathText( const fs::path &path )
    {
        return path.u8string().c_str();
    }

    inline void write( const fs::path &path, const std::string &bytes )
    {
        std::ofstream output( path, std::ios::binary | std::ios::trunc );
        output.write( bytes.data(), static_cast<std::streamsize>( bytes.size() ) );
        require( output.good(), "Runtime fixture write failed" );
    }

    inline std::string read( const fs::path &path )
    {
        std::ifstream input( path, std::ios::binary );
        require( static_cast<bool>( input ), "Runtime fixture read failed" );
        return std::string( std::istreambuf_iterator<char>( input ), std::istreambuf_iterator<char>() );
    }

    class Compiler final : public IResourceCompiler
    {
    public:
        String name() const override { return "RuntimeManifestContractCompiler"; }
        Array<CompilerOutput> outputs() const override { return { { ResourceTypeID( "rtres" ), 7 } }; }
        bool getDependencies( const CompileContext &context, DependencySet &dependencies,
                              String &error ) const override
        {
            std::ifstream input( fs::u8path(context.sourcePath.c_str()) );
            if( !input )
            {
                error = "Missing runtime fixture descriptor";
                return false;
            }
            std::string line;
            while( std::getline( input, line ) )
            {
                if( line.rfind( "install=", 0 ) == 0 )
                    dependencies.installDependencies.emplace_back( line.substr( 8 ).c_str() );
            }
            return true;
        }
        CompilationStatus compile( const CompileContext &context, std::ostream &output,
                                   Array<String> & ) const override
        {
            std::ifstream input( fs::u8path(context.sourcePath.c_str()), std::ios::binary );
            output << input.rdbuf();
            return input && output ? CompilationStatus::Success : CompilationStatus::Failure;
        }
    };

    struct Fixture
    {
        fs::path root;
        Fixture()
        {
            root = fs::temp_directory_path() /
                   fs::u8path( std::string(u8"workphone_runtime_contract_资产_é_") + std::to_string(
                       std::chrono::high_resolution_clock::now().time_since_epoch().count() ) );
            fs::create_directories( root / "source" );
        }
        ~Fixture()
        {
            std::error_code ignored;
            for( fs::recursive_directory_iterator item( root, ignored ), end; item != end;
                 item.increment( ignored ) )
            {
                fs::permissions( item->path(), fs::perms::owner_all, fs::perm_options::add, ignored );
                if( ignored )
                    break;
            }
            fs::remove_all( root, ignored );
        }
    };

    inline std::map<String, std::pair<u64, u64>> snapshot( const fs::path &root )
    {
        std::map<String, std::pair<u64, u64>> result;
        String error;
        for( const auto &entry : fs::recursive_directory_iterator( root ) )
        {
            u64 hash = 0;
            u64 size = 0;
            if( entry.is_regular_file() )
                require( CompiledResourceIO::hashFile( pathText( entry.path() ), hash, size, error ), error );
            result[pathText( fs::relative( entry.path(), root ) )] = { hash, size };
        }
        return result;
    }

    // Write an otherwise valid container around intentionally invalid graph metadata.
    // This reaches graph/schema validation rather than only the outer payload hash check.
    inline void writeManifestFixture( const fs::path &path, const String &target,
                                      const Array<CompiledResourceHeader> &headers, u64 version = 1 )
    {
        std::ostringstream bytes( std::ios::binary );
        auto number = [&]( u64 value ) {
            for( size_t index = 0; index < sizeof( value ); ++index )
                bytes.put( static_cast<char>( ( value >> ( index * 8 ) ) & 0xffu ) );
        };
        auto string = [&]( const String &value ) {
            number( value.size() );
            bytes.write( value.data(), static_cast<std::streamsize>( value.size() ) );
        };
        number( version );
        string( target );
        number( 1 );
        string( "data://root.rtres" );
        number( headers.size() );
        for( const auto &header : headers )
        {
            string( header.resourceId.str() );
            number( header.compilerVersion );
            number( header.sourceHash );
            number( header.payloadHash );
            number( header.payloadSize );
            number( header.installDependencies.size() );
            for( const auto &dependency : header.installDependencies )
                string( dependency.str() );
        }
        const auto payloadPath = path.parent_path() / "manifest-test.payload";
        write( payloadPath, bytes.str() );
        CompiledResourceHeader container;
        container.resourceId = ResourceID( "data://runtime.wprm" );
        container.resourceType = container.resourceId.type();
        container.compilerVersion = 1;
        container.sourceHash = 1;
        String error;
        require( CompiledResourceIO::hashFile( pathText( payloadPath ), container.payloadHash,
                                              container.payloadSize, error ), error );
        require( CompiledResourceIO::writeAtomic( pathText( path ), container, pathText( payloadPath ), error ), error );
        fs::remove( payloadPath );
    }

    inline void run()
    {
        Fixture fixture;
        const auto source = fixture.root / "source";
        const auto cooked = fixture.root / "cooked";
        const auto relocated = fixture.root / "relocated";
        const ResourceID rootId( "data://root.rtres" );
        const ResourceID leafId( "data://leaf.rtres" );
        const std::map<std::string, std::string> descriptors = {
            { "root.rtres", "install=data://left.rtres\ninstall=data://right.rtres\nroot\n" },
            { "left.rtres", "install=data://leaf.rtres\nleft\n" },
            { "right.rtres", "install=data://link.rtres\nright\n" },
            { "link.rtres", "install=data://left.rtres\nlink\n" },
            { "leaf.rtres", "leaf-v1\n" }
        };
        for( const auto &entry : descriptors )
            write( source / entry.first, entry.second );
        auto registry = std::make_shared<ResourceCompilerRegistry>();
        auto database = std::make_shared<ResourceCompilationDatabase>();
        String error;
        require( registry->registerCompiler( std::make_shared<Compiler>(), &error ), error );
        ResourceSystem authoring( registry, database );
        ResourceSystemConfig authoringConfig;
        authoringConfig.sourceRoot = pathText( source );
        authoringConfig.compiledRoot = pathText( cooked );
        authoringConfig.target = "runtime-contract";
        require( authoring.initialize( authoringConfig, error ), error );
        require( authoring.compile( rootId ).succeeded(), "Runtime fixture cook failed" );
        auto oldRoot = authoring.load( rootId, error );
        require( oldRoot != nullptr, error );
        auto oldLeaf = oldRoot->dependencies[0]->dependencies[0];
        write( source / "leaf.rtres", "leaf-v2\n" );
        require( authoring.compile( leafId ).succeeded(), "Runtime leaf recook failed" );
        require( !authoring.writeRuntimeManifest( pathText( cooked / "incoherent.wprm" ), { rootId }, error ),
                 "Manifest accepted an unchecked parent after an independent child recook" );
        auto newRoot = authoring.load( rootId, error );
        require( newRoot && newRoot != oldRoot &&
                 newRoot->dependencies[0]->dependencies[0] != oldLeaf &&
                 oldLeaf->payload.back() == '\n',
                 "Child recook did not invalidate cached parent or retain the old snapshot" );
        require( std::string( oldLeaf->payload.begin(), oldLeaf->payload.end() ) == "leaf-v1\n" &&
                 std::string( newRoot->dependencies[0]->dependencies[0]->payload.begin(),
                              newRoot->dependencies[0]->dependencies[0]->payload.end() ) == "leaf-v2\n",
                 "Recook changed a held resource or reloaded stale child bytes" );
        require( authoring.compile( rootId ).succeeded(), "Runtime closure recook failed" );
        const auto manifest = cooked / "runtime.wprm";
        require( authoring.writeRuntimeManifest( pathText( manifest ), { rootId }, error ), error );
        const auto manifestBytes = read( manifest );
        write( source / "independent.rtres", "independent\n" );
        const ResourceID independentId( "data://independent.rtres" );
        require( authoring.compile( independentId ).succeeded(), "Independent root cook failed" );
        require( authoring.writeRuntimeManifest( pathText( cooked / "multi-root.wprm" ),
                                                 { rootId, independentId }, error ),
                 "Independent root compilation invalidated unrelated manifest evidence" );
        require( authoring.writeRuntimeManifest( pathText( manifest ), { rootId, rootId }, error ), error );
        require( read( manifest ) == manifestBytes, "Runtime manifest export is not deterministic" );
        require( !authoring.writeRuntimeManifest( pathText( manifest ), { ResourceID( "data://missing.rtres" ) }, error ),
                 "Missing runtime manifest root was accepted" );
        require( read( manifest ) == manifestBytes, "Failed manifest export replaced the last-good manifest" );
        const auto rootBytes = read( cooked / "root.rtres" );
        require( !authoring.writeRuntimeManifest( pathText( cooked / "root.rtres" ), { rootId }, error ) &&
                 read( cooked / "root.rtres" ) == rootBytes,
                 "Manifest export overwrote a resource payload" );
        authoring.shutdown();
        require( authoring.initialize( authoringConfig, error ), error );
        require( !authoring.writeRuntimeManifest( pathText( manifest ), { rootId }, error ),
                 "Export trusted target provenance from an old unqualified compilation database" );
        require( authoring.compile( rootId ).status == CompilationStatus::UpToDate,
                 "Restart lost persistent compilation metadata" );
        require( authoring.writeRuntimeManifest( pathText( manifest ), { rootId }, error ), error );
        authoring.shutdown();

        fs::create_directories( relocated );
        u64 totalPayload = 0;
        for( const auto &entry : descriptors )
        {
            fs::copy_file( cooked / entry.first, relocated / entry.first );
            RuntimeResource payload;
            require( CompiledResourceIO::read( pathText( relocated / entry.first ), payload, error ), error );
            totalPayload += payload.header.payloadSize;
        }
        fs::copy_file( manifest, relocated / "runtime.wprm" );
        fs::remove_all( source );
        fs::remove_all( cooked );
        const auto packageBefore = snapshot( relocated );
        for( const auto &entry : fs::directory_iterator( relocated ) )
            fs::permissions( entry.path(), fs::perms::owner_read, fs::perm_options::replace );

        RuntimeResourceConfig runtimeConfig;
        runtimeConfig.compiledRoot = pathText( relocated );
        runtimeConfig.manifestPath = pathText( relocated / "runtime.wprm" );
        runtimeConfig.target = authoringConfig.target;
        runtimeConfig.maxClosurePayloadBytes = totalPayload;
        runtimeConfig.maxClosureResources = 5;
        runtimeConfig.maxDependencyDepth = 5;
        ResourceSystem runtime( nullptr, nullptr );
        auto invalidConfig = runtimeConfig;
        invalidConfig.target = "different-target";
        require( !runtime.initializeRuntime( invalidConfig, error ) && !runtime.isInitialized(),
                 "Runtime accepted a mismatched target" );
        invalidConfig = runtimeConfig;
        invalidConfig.compiledRoot = pathText( fixture.root / "missing-mount" );
        require( !runtime.initializeRuntime( invalidConfig, error ) &&
                 !fs::exists( fixture.root / "missing-mount" ),
                 "Runtime initialization created a missing mount" );
        require( runtime.initializeRuntime( runtimeConfig, error ), error );
        require( runtime.registry() == nullptr && !runtime.compile( rootId ).succeeded() &&
                 !runtime.compileAffected( leafId.str() ).front().succeeded() &&
                 !runtime.writeRuntimeManifest( pathText( relocated / "forbidden.wprm" ), { rootId }, error ),
                 "Read-only runtime attempted authoring work" );
        error = "old diagnostic";
        auto loaded = runtime.load( rootId, error );
        require( loaded != nullptr && error.empty(), error );
        const auto leaf = loaded->dependencies[0]->dependencies[0];
        require( loaded->dependencies[1]->dependencies[0]->dependencies[0] == loaded->dependencies[0],
                 "Diamond runtime dependencies did not share identity" );
        require( runtime.load( leafId, error ) == leaf, "Retained runtime resources were not reused" );
        require( !runtime.load( ResourceID( "data://unlisted.rtres" ), error ),
                 "Runtime accepted a resource absent from its manifest" );
        auto first = std::async( std::launch::async, [&]() { String diagnostic; return runtime.load( rootId, diagnostic ); } );
        auto second = std::async( std::launch::async, [&]() { String diagnostic; return runtime.load( rootId, diagnostic ); } );
        require( first.get() == loaded && second.get() == loaded, "Concurrent runtime load changed retained identity" );
        runtime.shutdown();
        require( !runtime.load( rootId, error ) && !loaded->payload.empty() && !leaf->payload.empty(),
                 "Shutdown invalidated held runtime snapshots or allowed another load" );
        require( snapshot( relocated ) == packageBefore, "Read-only runtime changed package files" );
        require( runtime.initializeRuntime( runtimeConfig, error ), error );
        require( runtime.load( rootId, error ) != nullptr, error );
        runtime.shutdown();

        auto limited = runtimeConfig;
        limited.maxClosurePayloadBytes = totalPayload - 1;
        require( runtime.initializeRuntime( limited, error ), error );
        auto cachedLeaf = runtime.load( leafId, error );
        require( cachedLeaf != nullptr && !runtime.load( rootId, error ),
                 "Aggregate runtime budget ignored a cached dependency" );
        runtime.shutdown();
        limited = runtimeConfig;
        limited.maxClosureResources = 4;
        require( runtime.initializeRuntime( limited, error ), error );
        require( !runtime.load( rootId, error ), "Runtime accepted an over-budget install resource count" );
        runtime.shutdown();
        limited = runtimeConfig;
        limited.maxDependencyDepth = 4;
        require( runtime.initializeRuntime( limited, error ), error );
        require( !runtime.load( rootId, error ), "Runtime depth limit missed the longer diamond path" );
        runtime.shutdown();

        for( const auto &entry : fs::directory_iterator( relocated ) )
            fs::permissions( entry.path(), fs::perms::owner_all, fs::perm_options::add );
        const auto leafPath = relocated / "leaf.rtres";
        const auto goodLeafBytes = read( leafPath );
        auto corruptLeafBytes = goodLeafBytes;
        corruptLeafBytes.back() ^= 1;
        write( leafPath, corruptLeafBytes );
        require( runtime.initializeRuntime( runtimeConfig, error ), error );
        require( !runtime.load( rootId, error ), "Runtime accepted a corrupt install dependency" );
        write( leafPath, goodLeafBytes );
        require( runtime.load( rootId, error ) != nullptr, "Failed closure load poisoned the runtime cache" );
        auto retained = runtime.load( rootId, error );
        runtime.unload( leafId );
        fs::remove( leafPath );
        require( !runtime.load( rootId, error ) && !retained->dependencies[0]->dependencies[0]->payload.empty(),
                 "Unloading a child reused a stale cached parent or invalidated held data" );
        write( leafPath, goodLeafBytes );
        runtime.shutdown();

        RuntimeResource replacement;
        require( CompiledResourceIO::read( pathText( leafPath ), replacement, error ), error );
        const auto replacementPayload = fixture.root / "replacement.payload";
        write( replacementPayload, "leaf-v3\n" );
        require( CompiledResourceIO::hashFile( pathText( replacementPayload ), replacement.header.payloadHash,
                                              replacement.header.payloadSize, error ), error );
        require( CompiledResourceIO::writeAtomic( pathText( leafPath ), replacement.header,
                                                 pathText( replacementPayload ), error ), error );
        require( runtime.initializeRuntime( runtimeConfig, error ), error );
        require( !runtime.load( rootId, error ), "Runtime accepted valid bytes from an unpinned generation" );
        runtime.shutdown();
        write( leafPath, goodLeafBytes );

        auto damagedManifest = manifestBytes;
        damagedManifest.back() ^= 1;
        write( relocated / "runtime.wprm", damagedManifest );
        require( !runtime.initializeRuntime( runtimeConfig, error ) && !runtime.isInitialized(),
                 "Runtime accepted a corrupt manifest" );
        write( relocated / "runtime.wprm", manifestBytes );
        require( runtime.initializeRuntime( runtimeConfig, error ), error );
        require( runtime.load( rootId, error ) != nullptr, error );
        runtime.shutdown();

        CompiledResourceHeader graphRoot;
        graphRoot.resourceId = rootId;
        graphRoot.resourceType = rootId.type();
        graphRoot.compilerVersion = 1;
        graphRoot.sourceHash = 1;
        graphRoot.payloadHash = 1;
        graphRoot.installDependencies = { leafId };
        CompiledResourceHeader graphLeaf = graphRoot;
        graphLeaf.resourceId = leafId;
        graphLeaf.installDependencies = { rootId };
        writeManifestFixture( relocated / "runtime.wprm", runtimeConfig.target, { graphRoot, graphLeaf } );
        require( !runtime.initializeRuntime( runtimeConfig, error ), "Runtime accepted a cyclic manifest" );
        writeManifestFixture( relocated / "runtime.wprm", runtimeConfig.target, { graphRoot } );
        require( !runtime.initializeRuntime( runtimeConfig, error ), "Runtime accepted an incomplete manifest closure" );
        graphRoot.installDependencies.clear();
        writeManifestFixture( relocated / "runtime.wprm", runtimeConfig.target, { graphRoot, graphRoot } );
        require( !runtime.initializeRuntime( runtimeConfig, error ), "Runtime accepted duplicate manifest identities" );
        writeManifestFixture( relocated / "runtime.wprm", runtimeConfig.target, { graphRoot }, 2 );
        require( !runtime.initializeRuntime( runtimeConfig, error ), "Runtime accepted a newer manifest schema" );
        // The shared subtree is visited first along the short branch. Memoization
        // must still count its depth when the long branch reaches it afterwards.
        graphRoot.installDependencies = { leafId, ResourceID( "data://chain0.rtres" ) };
        graphLeaf.installDependencies = { ResourceID( "data://tail.rtres" ) };
        CompiledResourceHeader tail = graphLeaf;
        tail.resourceId = ResourceID( "data://tail.rtres" );
        tail.installDependencies.clear();
        Array<CompiledResourceHeader> deepGraph{ graphRoot, graphLeaf, tail };
        for( unsigned index = 0; index < 254; ++index )
        {
            auto node = tail;
            node.resourceId = ResourceID( ( "data://chain" + std::to_string( index ) + ".rtres" ).c_str() );
            node.installDependencies = { index == 253 ? leafId : ResourceID(
                ( "data://chain" + std::to_string( index + 1 ) + ".rtres" ).c_str() ) };
            deepGraph.push_back( node );
        }
        writeManifestFixture( relocated / "runtime.wprm", runtimeConfig.target, deepGraph );
        require( !runtime.initializeRuntime( runtimeConfig, error ),
                 "Runtime manifest accepted a shared-DAG path deeper than its format limit" );
        deepGraph.pop_back();
        deepGraph.back().installDependencies = { leafId };
        writeManifestFixture( relocated / "runtime.wprm", runtimeConfig.target, deepGraph );
        require( runtime.initializeRuntime( runtimeConfig, error ),
                 "Runtime manifest rejected the exact supported shared-DAG depth boundary" );
        runtime.shutdown();
        write( relocated / "runtime.wprm", manifestBytes );
        require( snapshot( relocated ) == packageBefore, "Runtime fixture failed to restore package bytes" );
    }
}

#endif
