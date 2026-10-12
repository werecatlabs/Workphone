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
                                      const Array<CompiledResourceHeader> &headers, u64 version = 1,
                                      const std::map<String,String> &artifacts = {},
                                      const Array<RuntimeResourceBinding> &bindings = {} )
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
            if(version==2 || version==3) string(artifacts.at(header.resourceId.str()));
            for( const auto &dependency : header.installDependencies )
                string( dependency.str() );
        }
        if( version >= 3 )
        {
            number( bindings.size() );
            for( const auto &binding : bindings )
            {
                string( binding.uuid );
                string( binding.resourceId.str() );
            }
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
        const String rootUUID = "a1234567-89ab-cdef-0123-456789abcdef";
        const String upperRootUUID = "A1234567-89AB-CDEF-0123-456789ABCDEF";
        const String leafUUID = "b1234567-89ab-cdef-0123-456789abcdef";
        const Array<RuntimeResourceBinding> bindings = { { rootUUID, rootId }, { leafUUID, leafId } };
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
        const auto exportedPackage = fixture.root / "exported-package";
        require( !authoring.writeRuntimePackage( pathText( exportedPackage ), { rootId }, bindings, error ) &&
                 !fs::exists( exportedPackage ), "Editor-mode resources must not be exported as a shipping package" );
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
                 oldLeaf->payload.back() == '\n',
                 "Child recook did not clear the cache or retain the old snapshot" );
        require( std::string( oldLeaf->payload.begin(), oldLeaf->payload.end() ) == "leaf-v1\n" &&
                 std::string( newRoot->dependencies[0]->dependencies[0]->payload.begin(),
                              newRoot->dependencies[0]->dependencies[0]->payload.end() ) == "leaf-v1\n",
                 "Independently recooked child changed the older parent's dependency snapshot" );
        auto directLeaf = authoring.load(leafId,error);
        require(directLeaf && directLeaf != newRoot->dependencies[0]->dependencies[0] &&
                std::string(directLeaf->payload.begin(),directLeaf->payload.end()) == "leaf-v2\n",
                "Direct child load must select the latest committed generation beside the old parent");
        require(authoring.load(rootId,error) == newRoot,
                "Generation cache must retain the parent without aliasing its child's latest generation");
        authoring.clearRuntimeCache();
        authoring.shutdown();
        require(authoring.initialize(authoringConfig,error),error);
        auto restartedRoot = authoring.load(rootId,error);
        require(restartedRoot &&
                std::string(restartedRoot->dependencies[0]->dependencies[0]->payload.begin(),
                            restartedRoot->dependencies[0]->dependencies[0]->payload.end()) == "leaf-v1\n",
                "Persistent dependency pins must survive a service restart and independent child recook");
        const auto oldLeafPath=cooked / fs::u8path(restartedRoot->dependencies[0]->header.installArtifacts[0].path.c_str());
        const auto oldLeafBytes=read(oldLeafPath);
        authoring.clearRuntimeCache();
        fs::remove(oldLeafPath);
        require(!authoring.load(rootId,error),"Missing pinned child must fail instead of falling back to the latest index entry");
        write(oldLeafPath,oldLeafBytes);
        require(authoring.load(rootId,error)!=nullptr,"Missing dependency failure must not poison the generation cache");
        require( authoring.compile( rootId ).succeeded(), "Runtime closure recook failed" );
        auto recookedRoot = authoring.load(rootId,error);
        require(recookedRoot && std::string(recookedRoot->dependencies[0]->dependencies[0]->payload.begin(),
                                           recookedRoot->dependencies[0]->dependencies[0]->payload.end()) == "leaf-v2\n",
                "Recooking the parent must advance its complete pinned dependency closure");
        const auto manifest = cooked / "runtime.wprm";
        require( authoring.writeRuntimeManifest( pathText( manifest ), { rootId }, error ), error );
        const auto manifestBytes = read( manifest );
        const auto uuidManifest = cooked / "runtime-uuid.wprm";
        require( authoring.writeRuntimeManifest( pathText( uuidManifest ), { rootId }, bindings, error ), error );
        const auto uuidManifestBytes = read( uuidManifest );
        require( authoring.writeRuntimeManifest( pathText( uuidManifest ), { rootId },
                     { { leafUUID, leafId }, { upperRootUUID, rootId } }, error ) &&
                 read( uuidManifest ) == uuidManifestBytes,
                 "UUID manifest must be deterministic across input order and UUID case" );
        for( const auto &invalid : Array<Array<RuntimeResourceBinding>>{
                 { { rootUUID, rootId }, { upperRootUUID, leafId } },
                 { { rootUUID, rootId }, { leafUUID, rootId } },
                 { { rootUUID, ResourceID( "data://missing.rtres" ) } },
                 { { "00000000-0000-0000-0000-000000000000", rootId } },
                 { { "not-a-uuid", rootId } },
                 { { rootUUID, ResourceID() } } } )
        {
            require( !authoring.writeRuntimeManifest( pathText( uuidManifest ), { rootId }, invalid, error ) &&
                     !error.empty() && read( uuidManifest ) == uuidManifestBytes,
                     "Invalid UUID export must preserve the previous manifest" );
        }
        ResourceID resolvedUUID = rootId;
        require( !authoring.resolveRuntimeUUID( rootUUID, resolvedUUID, error ) &&
                 !resolvedUUID.isValid() && !authoring.loadUUID( rootUUID, error ),
                 "Authoring must not infer UUID bindings from paths" );
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
        const auto rootArtifact=fs::u8path(authoring.compile(rootId).outputPath.c_str());
        const auto rootBytes = read( rootArtifact );
        require( !authoring.writeRuntimeManifest( pathText( rootArtifact ), { rootId }, error ) &&
                 read( rootArtifact ) == rootBytes,
                 "Manifest export overwrote a resource payload" );
        authoring.shutdown();
        require( authoring.initialize( authoringConfig, error ), error );
        require( !authoring.writeRuntimeManifest( pathText( manifest ), { rootId }, error ),
                 "Export trusted target provenance from an old unqualified compilation database" );
        require( authoring.compile( rootId ).status == CompilationStatus::UpToDate,
                 "Restart lost persistent compilation metadata" );
        require( authoring.writeRuntimeManifest( pathText( manifest ), { rootId }, error ), error );
        std::map<std::string,fs::path> artifactPaths;
        for(const auto &entry:descriptors)
        {
            CompiledResourceRecord record;
            require(database->getRecord(("data://"+entry.first).c_str(),record) && record.isValid(),"Committed artifact path");
            artifactPaths[entry.first]=fs::u8path(record.outputPath.c_str());
        }
        CompilationOptions packageOptions;
        packageOptions.packagedBuild = true;
        const auto packageBuild = authoring.compile( rootId, packageOptions );
        require( packageBuild.succeeded(), "Runtime fixture package-mode compilation failed" );
        write( cooked / "unrelated-private-file", "do not package" );
        require( authoring.writeRuntimePackage( pathText( exportedPackage ), { rootId }, bindings, error ), error );
        require( !fs::exists( exportedPackage / ".resource_compilation.db" ) &&
                 !fs::exists( exportedPackage / "unrelated-private-file" ) &&
                 !fs::exists( exportedPackage / ".resource-writers" ),
                 "Runtime package must contain only its manifest and install closure" );
        unsigned packagedFiles = 0;
        for( const auto &entry : fs::recursive_directory_iterator( exportedPackage ) )
            if( entry.is_regular_file() ) ++packagedFiles;
        require( packagedFiles == descriptors.size() + 1, "Runtime package exported an incomplete or excess closure" );
        const auto packageBeforeRetry = snapshot( exportedPackage );
        require( !authoring.writeRuntimePackage( pathText( exportedPackage ), { rootId }, bindings, error ) &&
                 snapshot( exportedPackage ) == packageBeforeRetry,
                 "Runtime package retry must preserve an existing destination byte-for-byte" );
        fs::create_directory( fixture.root / "occupied-empty" );
        require( !authoring.writeRuntimePackage( pathText( fixture.root / "occupied-empty" ), { rootId }, bindings, error ),
                 "Runtime package must not overwrite an existing empty directory" );
        require( !authoring.writeRuntimePackage( pathText( source / "package" ), { rootId }, bindings, error ) &&
                 !authoring.writeRuntimePackage( pathText( cooked / "package" ), { rootId }, bindings, error ) &&
                 !authoring.writeRuntimePackage( pathText( fixture.root ), { rootId }, bindings, error ),
                 "Runtime package must reject source/cooked storage overlap" );
        const auto failedPackage = fixture.root / "failed-package";
        require( !authoring.writeRuntimePackage( pathText( failedPackage ), { rootId }, { { rootUUID, independentId } }, error ) &&
                 !fs::exists( failedPackage ), "Invalid UUID package must not publish a partial directory" );
        const auto packageRootPath = fs::u8path( packageBuild.outputPath.c_str() );
        const auto packageRootBytes = read( packageRootPath );
        write( packageRootPath, "damaged" );
        require( !authoring.writeRuntimePackage( pathText( failedPackage ), { rootId }, bindings, error ) &&
                 !fs::exists( failedPackage ), "Corrupt package input must not publish a destination" );
        write( packageRootPath, packageRootBytes );
        for( const auto &entry : fs::directory_iterator( fixture.root ) )
            require( entry.path().filename().u8string().find( ".wp-package-" ) != 0,
                     "Completed or failed export leaked its staging directory" );
        authoring.shutdown();

        fs::create_directories( relocated );
        u64 totalPayload = 0;
        for( const auto &entry : descriptors )
        {
            const auto relative=artifactPaths.at(entry.first);
            fs::create_directories((relocated / relative).parent_path());
            fs::copy_file( cooked / relative, relocated / relative );
            RuntimeResource payload;
            require( CompiledResourceIO::read( pathText( relocated / relative ), payload, error ), error );
            totalPayload += payload.header.payloadSize;
        }
        fs::copy_file( manifest, relocated / "runtime.wprm" );
        fs::copy_file( uuidManifest, relocated / "runtime-uuid.wprm" );
        fs::remove_all( source );
        fs::remove_all( cooked );
        const auto packageBefore = snapshot( relocated );
        for( const auto &entry : fs::recursive_directory_iterator( relocated ) )
            fs::permissions( entry.path(), fs::perms::owner_read, fs::perm_options::replace );

        RuntimeResourceConfig runtimeConfig;
        runtimeConfig.compiledRoot = pathText( relocated );
        runtimeConfig.manifestPath = pathText( relocated / "runtime.wprm" );
        runtimeConfig.target = authoringConfig.target;
        runtimeConfig.maxClosurePayloadBytes = totalPayload;
        runtimeConfig.maxClosureResources = 5;
        runtimeConfig.maxDependencyDepth = 5;
        ResourceSystem runtime( nullptr, nullptr );
        auto exportedConfig = runtimeConfig;
        exportedConfig.compiledRoot = pathText( exportedPackage );
        exportedConfig.manifestPath = pathText( exportedPackage / "runtime.wprm" );
        const auto exportedBeforeLoad = snapshot( exportedPackage );
        for( const auto &entry : fs::recursive_directory_iterator( exportedPackage ) )
            if( entry.is_regular_file() )
                fs::permissions( entry.path(), fs::perms::owner_read, fs::perm_options::replace );
        require( runtime.initializeRuntime( exportedConfig, error ) && runtime.loadUUID( rootUUID, error ) != nullptr,
                 "Exported runtime package must load by durable UUID with source/cooked storage absent" );
        runtime.shutdown();
        require( snapshot( exportedPackage ) == exportedBeforeLoad, "Exported runtime package mount changed files" );
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

        auto uuidConfig = runtimeConfig;
        uuidConfig.manifestPath = pathText( relocated / "runtime-uuid.wprm" );
        require( runtime.initializeRuntime( uuidConfig, error ), error );
        error = "old diagnostic";
        require( runtime.resolveRuntimeUUID( upperRootUUID, resolvedUUID, error ) &&
                 resolvedUUID == rootId && error.empty(),
                 "Runtime UUID lookup must normalize case and clear previous diagnostics" );
        auto uuidRoot = runtime.loadUUID( rootUUID, error );
        require( uuidRoot && uuidRoot == runtime.load( rootId, error ) &&
                 runtime.loadUUID( leafUUID, error ) == uuidRoot->dependencies[0]->dependencies[0],
                 "UUID and logical loads must share retained closure identities" );
        for( const auto &missing : Array<String>{ "c1234567-89ab-cdef-0123-456789abcdef",
                "data://root.rtres", "00000000-0000-0000-0000-000000000000", "not-a-uuid" } )
        {
            resolvedUUID = rootId;
            require( !runtime.resolveRuntimeUUID( missing, resolvedUUID, error ) && !resolvedUUID.isValid() &&
                     !runtime.loadUUID( missing, error ) && !error.empty(),
                     "Invalid or absent UUID must fail without path fallback or partial output" );
        }
        auto uuidFirst = std::async( std::launch::async, [&]() { String diagnostic; return runtime.loadUUID( rootUUID, diagnostic ); } );
        auto uuidSecond = std::async( std::launch::async, [&]() { String diagnostic; return runtime.load( rootId, diagnostic ); } );
        require( uuidFirst.get() == uuidRoot && uuidSecond.get() == uuidRoot,
                 "Concurrent UUID and path loads must share identity" );
        runtime.shutdown();
        require( !runtime.loadUUID( rootUUID, error ) && !uuidRoot->payload.empty(),
                 "UUID load after shutdown must fail while retained resources remain valid" );
        require( snapshot( relocated ) == packageBefore, "UUID lookup/load wrote into the package" );

        auto limited = runtimeConfig;
        limited.manifestPath = uuidConfig.manifestPath;
        limited.maxClosurePayloadBytes = totalPayload - 1;
        require( runtime.initializeRuntime( limited, error ), error );
        auto cachedLeaf = runtime.load( leafId, error );
        require( cachedLeaf != nullptr && !runtime.loadUUID( rootUUID, error ),
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

        for( const auto &entry : fs::recursive_directory_iterator( relocated ) )
            fs::permissions( entry.path(), fs::perms::owner_all, fs::perm_options::add );
        const auto parentPath=relocated / artifactPaths.at("root.rtres");
        const auto goodParentBytes=read(parentPath);
        RuntimeResource alteredParent;
        require(CompiledResourceIO::read(pathText(parentPath),alteredParent,error),error);
        const auto parentPayload=fixture.root / "parent.payload";
        write(parentPayload,std::string(alteredParent.payload.begin(),alteredParent.payload.end()));
        alteredParent.header.installArtifacts[0].path=artifactPaths.at("right.rtres").generic_u8string().c_str();
        require(CompiledResourceIO::writeAtomic(pathText(parentPath),alteredParent.header,pathText(parentPayload),error),error);
        require(runtime.initializeRuntime(runtimeConfig,error),error);
        require(!runtime.load(rootId,error),"Runtime accepted parent pins inconsistent with manifest artifact paths");
        runtime.shutdown();
        write(parentPath,goodParentBytes);
        require(CompiledResourceIO::read(pathText(parentPath),alteredParent,error),error);
        ++alteredParent.header.installArtifacts[0].payloadHash;
        require(CompiledResourceIO::writeAtomic(pathText(parentPath),alteredParent.header,pathText(parentPayload),error),error);
        require(runtime.initializeRuntime(runtimeConfig,error),error);
        require(!runtime.load(rootId,error),"Runtime accepted a dependency fingerprint inconsistent with its parent pin");
        runtime.shutdown();
        write(parentPath,goodParentBytes);
        const auto leafPath = relocated / artifactPaths.at("leaf.rtres");
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
        const std::map<String, String> uuidPaths = {
            { rootId.str(), artifactPaths.at("root.rtres").generic_u8string().c_str() }
        };
        for( const auto &invalid : Array<Array<RuntimeResourceBinding>>{
                 { { rootUUID, rootId }, { upperRootUUID, rootId } },
                 { { rootUUID, rootId }, { leafUUID, rootId } },
                 { { rootUUID, leafId } },
                 { { "00000000-0000-0000-0000-000000000000", rootId } },
                 { { "a1234567_89ab-cdef-0123-456789abcdef", rootId } } } )
        {
            writeManifestFixture( relocated / "runtime.wprm", runtimeConfig.target,
                                  { graphRoot }, 3, uuidPaths, invalid );
            require( !runtime.initializeRuntime( runtimeConfig, error ) && !runtime.isInitialized(),
                     "Malformed UUID manifest must fail before mount publication" );
        }
        writeManifestFixture( relocated / "runtime.wprm", runtimeConfig.target,
                              { graphRoot }, 3, uuidPaths, {} );
        require( runtime.initializeRuntime( runtimeConfig, error ), "V3 empty UUID map must be valid" );
        require( !runtime.resolveRuntimeUUID( rootUUID, resolvedUUID, error ) && !resolvedUUID.isValid(),
                 "Empty map must not retain bindings from an earlier mount" );
        runtime.shutdown();
        writeManifestFixture( relocated / "runtime.wprm", runtimeConfig.target, { graphRoot, graphRoot } );
        require( !runtime.initializeRuntime( runtimeConfig, error ), "Runtime accepted duplicate manifest identities" );
        writeManifestFixture( relocated / "runtime.wprm", runtimeConfig.target, { graphRoot }, 4 );
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
        const auto legacy=fixture.root / "legacy-package";
        fs::create_directories(legacy);
        Array<CompiledResourceHeader> legacyHeaders;
        std::map<String,String> pinnedPaths;
        for(const auto &entry:descriptors)
        {
            RuntimeResource resource;
            require(CompiledResourceIO::read(pathText(relocated / artifactPaths.at(entry.first)),resource,error),error);
            auto header = resource.header;
            header.formatVersion = 1;
            header.installArtifacts.clear();
            const auto payload = fixture.root / "legacy.payload";
            write(payload,std::string(resource.payload.begin(),resource.payload.end()));
            require(CompiledResourceIO::writeAtomic(pathText(legacy / entry.first),header,pathText(payload),error),error);
            pinnedPaths[header.resourceId.str()]=pathText(artifactPaths.at(entry.first));
            legacyHeaders.push_back(header);
        }
        auto legacyConfig=runtimeConfig;
        legacyConfig.compiledRoot=pathText(legacy);
        legacyConfig.manifestPath=pathText(legacy / "runtime.wprm");
        writeManifestFixture(legacy / "runtime.wprm",runtimeConfig.target,legacyHeaders);
        require(runtime.initializeRuntime(legacyConfig,error) && runtime.load(rootId,error)!=nullptr,
                "Version-one packages must remain loadable from their legacy layout");
        runtime.shutdown();
        pinnedPaths[rootId.str()]="../escape.wprs";
        writeManifestFixture(legacy / "runtime.wprm",runtimeConfig.target,legacyHeaders,2,pinnedPaths);
        require(!runtime.initializeRuntime(legacyConfig,error),"Version-two manifest accepted a traversal artifact path");
        require( snapshot( relocated ) == packageBefore, "Runtime fixture failed to restore package bytes" );
    }
}

#endif
