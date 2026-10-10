#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/ResourceSystem.hpp>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cwctype>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <sstream>

#if defined WP_PLATFORM_WIN32
#    include <process.h>
#else
#    include <unistd.h>
#endif

namespace workphone::resource
{
    namespace
    {
        namespace fs = std::filesystem;
        std::atomic<u64> payloadTemporaryCounter{ 0 };
        constexpr u64 maximumManifestBytes = 16ull * 1024ull * 1024ull;
        constexpr u32 maximumManifestResources = 65536;
        constexpr u32 maximumManifestDependencies = 4096;
        constexpr u32 maximumManifestDepth = 256;
        const ResourceID manifestResourceId( "data://runtime.wprm" );

        struct RuntimeManifest
        {
            String target;
            Array<ResourceID> roots;
            std::map<String, CompiledResourceHeader> resources;
            std::map<String, String> artifacts;
        };

        String encodedArtifactKey( const String &value )
        {
            static const char digits[] = "0123456789abcdef";
            String encoded;
            for( unsigned char byte : value )
            {
                if((byte>='a' && byte<='z') || (byte>='0' && byte<='9') || byte=='_' || byte=='-')
                    encoded += static_cast<char>(byte);
                else
                {
                    encoded += '~';
                    encoded += digits[byte>>4];
                    encoded += digits[byte&15];
                }
            }
            String result;
            for(size_t offset=0; offset<encoded.size(); offset+=80)
            {
                if(offset) result += "/";
                // Avoid Windows device names, trailing dots, and case-folding aliases.
                result += "r" + encoded.substr(offset,80);
            }
            return result;
        }

        String artifactNamespace( const String &target )
        {
            return "artifacts/v2/" + encodedArtifactKey(target) + "/";
        }

        void writeManifestNumber( std::ostream &stream, u64 value )
        {
            for( size_t index = 0; index < sizeof( value ); ++index )
            {
                stream.put( static_cast<char>( value & 0xffu ) );
                value >>= 8;
            }
        }

        bool readManifestNumber( std::istream &stream, u64 &value )
        {
            value = 0;
            for( size_t index = 0; index < sizeof( value ); ++index )
            {
                const auto byte = stream.get();
                if( byte == std::char_traits<char>::eof() )
                    return false;
                value |= static_cast<u64>( static_cast<unsigned char>( byte ) ) << ( index * 8 );
            }
            return true;
        }

        bool writeManifestString( std::ostream &stream, const String &value )
        {
            if( value.empty() || value.size() > 4096 )
                return false;
            writeManifestNumber( stream, value.size() );
            stream.write( value.data(), static_cast<std::streamsize>( value.size() ) );
            return stream.good();
        }

        bool readManifestString( std::istream &stream, String &value )
        {
            u64 size = 0;
            if( !readManifestNumber( stream, size ) || size == 0 || size > 4096 )
                return false;
            std::string bytes( static_cast<size_t>( size ), '\0' );
            stream.read( &bytes[0], static_cast<std::streamsize>( size ) );
            if( !stream || bytes.find( '\0' ) != std::string::npos )
                return false;
            value = bytes.c_str();
            return true;
        }

        bool sameRuntimeHeader( const CompiledResourceHeader &left,
                                const CompiledResourceHeader &right )
        {
            return left.resourceId == right.resourceId && left.resourceType == right.resourceType &&
                   left.compilerVersion == right.compilerVersion && left.sourceHash == right.sourceHash &&
                   left.payloadHash == right.payloadHash && left.payloadSize == right.payloadSize &&
                   left.installDependencies == right.installDependencies;
        }

        bool readManifest( const String &path, RuntimeManifest &manifest, String &error )
        {
            RuntimeResource container;
            if( !CompiledResourceIO::read( path, container, error, maximumManifestBytes ) )
                return false;
            if( container.header.resourceId != manifestResourceId ||
                container.header.compilerVersion != 1 || !container.header.installDependencies.empty() )
            {
                error = "Unsupported runtime manifest container";
                return false;
            }
            std::string bytes( container.payload.begin(), container.payload.end() );
            std::istringstream input( bytes, std::ios::binary );
            u64 version = 0;
            u64 rootCount = 0;
            u64 resourceCount = 0;
            auto malformed = [&]() {
                error = "Runtime manifest is malformed or exceeds its format limits";
                return false;
            };
            if( !readManifestNumber( input, version ) || (version != 1 && version != 2) ||
                !readManifestString( input, manifest.target ) ||
                !readManifestNumber( input, rootCount ) || rootCount == 0 ||
                rootCount > maximumManifestResources )
                return malformed();
            std::set<String> rootIds;
            for( u64 index = 0; index < rootCount; ++index )
            {
                String id;
                if( !readManifestString( input, id ) || !ResourceID::isValidString( id ) ||
                    !rootIds.insert( id ).second )
                    return malformed();
                manifest.roots.emplace_back( id );
            }
            if( !readManifestNumber( input, resourceCount ) || resourceCount == 0 ||
                resourceCount > maximumManifestResources )
                return malformed();
            for( u64 index = 0; index < resourceCount; ++index )
            {
                CompiledResourceHeader header;
                String id;
                u64 dependencyCount = 0;
                if( !readManifestString( input, id ) || !header.resourceId.set( id ) ||
                    !readManifestNumber( input, header.compilerVersion ) ||
                    !readManifestNumber( input, header.sourceHash ) ||
                    !readManifestNumber( input, header.payloadHash ) ||
                    !readManifestNumber( input, header.payloadSize ) ||
                    !readManifestNumber( input, dependencyCount ) ||
                    dependencyCount > maximumManifestDependencies )
                    return malformed();
                header.resourceType = header.resourceId.type();
                String artifact=header.resourceId.compiledRelativePath();
                if( version==2 )
                {
                    ResourceID pathId;
                    if( !readManifestString(input,artifact) ||
                        artifact.find(':')!=String::npos || artifact.find('\\')!=String::npos ||
                        !pathId.set("data://"+artifact) || pathId.sourceRelativePath()!=artifact ||
                        artifact.find(artifactNamespace(manifest.target))!=0 )
                        return malformed();
                }
                manifest.artifacts[header.resourceId.str()]=artifact;
                std::set<String> uniqueDependencies;
                for( u64 dependencyIndex = 0; dependencyIndex < dependencyCount; ++dependencyIndex )
                {
                    String dependency;
                    if( !readManifestString( input, dependency ) ||
                        !ResourceID::isValidString( dependency ) ||
                        !uniqueDependencies.insert( ResourceID( dependency ).str() ).second )
                        return malformed();
                    header.installDependencies.emplace_back( dependency );
                }
                if( !header.isValid() ||
                    !manifest.resources.emplace( header.resourceId.str(), std::move( header ) ).second )
                    return malformed();
            }
            if( input.peek() != std::char_traits<char>::eof() )
                return malformed();

            std::set<String> visiting;
            std::set<String> visited;
            std::map<String, u32> subtreeDepths;
            std::function<bool( const ResourceID &, u32 )> visit = [&]( const ResourceID &id, u32 depth ) {
                if( depth > maximumManifestDepth || visiting.count( id.str() ) )
                    return false;
                if( visited.count( id.str() ) )
                    return depth + subtreeDepths[id.str()] - 1 <= maximumManifestDepth;
                const auto entry = manifest.resources.find( id.str() );
                if( entry == manifest.resources.end() )
                    return false;
                visiting.insert( id.str() );
                u32 subtreeDepth = 1;
                for( const auto &dependency : entry->second.installDependencies )
                {
                    if( !visit( dependency, depth + 1 ) )
                        return false;
                    subtreeDepth = std::max( subtreeDepth, subtreeDepths[dependency.str()] + 1 );
                }
                visiting.erase( id.str() );
                visited.insert( id.str() );
                subtreeDepths[id.str()] = subtreeDepth;
                return true;
            };
            for( const auto &root : manifest.roots )
                if( !visit( root, 1 ) )
                    return malformed();
            if( visited.size() != manifest.resources.size() )
                return malformed();
            return true;
        }

        String pathString( const fs::path &path )
        {
            return path.u8string().c_str();
        }

        String normalizeLogicalDataPath( const String &input, String &error )
        {
            String path = input;
            std::replace( path.begin(), path.end(), '\\', '/' );
            if( path.size() >= 7 )
            {
                String scheme = path.substr( 0, 7 );
                std::transform( scheme.begin(), scheme.end(), scheme.begin(), []( unsigned char c ) {
                    return static_cast<char>( std::tolower( c ) );
                } );
                if( scheme == "data://" )
                    path = path.substr( 7 );
            }
            if( path.empty() || path.front() == '/' || path.back() == '/' ||
                path.find( "://" ) != String::npos || path.find( ':' ) != String::npos )
            {
                error = "Data dependency paths must be relative data:// paths";
                return {};
            }

            size_t start = 0;
            while( start <= path.size() )
            {
                const size_t separator = path.find( '/', start );
                const String segment =
                    path.substr( start, separator == String::npos ? String::npos : separator - start );
                if( segment.empty() || segment == "." || segment == ".." )
                {
                    error = "Data dependency paths cannot contain empty, '.' or '..' segments";
                    return {};
                }
                if( separator == String::npos )
                    break;
                start = separator + 1;
            }
            return String( "data://" ) + path;
        }

        bool pathComponentEquals( const fs::path &left, const fs::path &right )
        {
#if defined WP_PLATFORM_WIN32
            auto a = left.wstring();
            auto b = right.wstring();
            std::transform( a.begin(), a.end(), a.begin(), ::towlower );
            std::transform( b.begin(), b.end(), b.begin(), ::towlower );
            return a == b;
#else
            return left == right;
#endif
        }

        bool isUnderRoot( const fs::path &root, const fs::path &candidate )
        {
            auto rootIterator = root.begin();
            auto candidateIterator = candidate.begin();
            for( ; rootIterator != root.end(); ++rootIterator, ++candidateIterator )
            {
                if( candidateIterator == candidate.end() ||
                    !pathComponentEquals( *rootIterator, *candidateIterator ) )
                    return false;
            }
            return true;
        }

        bool resolveUnderRoot( const fs::path &root, const String &logicalPath, bool mustExist,
                               fs::path &resolved, String &error )
        {
            String normalized = normalizeLogicalDataPath( logicalPath, error );
            if( normalized.empty() )
                return false;

            std::error_code filesystemError;
            fs::path candidate = root / fs::u8path( normalized.substr( 7 ).c_str() );
            candidate = mustExist ? fs::canonical( candidate, filesystemError )
                                  : fs::weakly_canonical( candidate, filesystemError );
            if( filesystemError )
            {
                error = String( "Failed to resolve data path '" ) + normalized +
                        "': " + filesystemError.message().c_str();
                return false;
            }
            if( !isUnderRoot( root, candidate ) )
            {
                error = String( "Data path escapes the configured root: " ) + normalized;
                return false;
            }
            if( mustExist && !fs::is_regular_file( candidate, filesystemError ) )
            {
                error = String( "Data dependency is not a regular file: " ) + normalized;
                return false;
            }
            resolved = candidate;
            return true;
        }

        CompilationReport failureReport( const ResourceID &resourceId, const String &message )
        {
            CompilationReport report;
            report.resourceId = resourceId;
            report.status = CompilationStatus::Failure;
            report.messages.push_back( message );
            return report;
        }

        void appendUnique( Array<String> &messages, const String &message )
        {
            if( !message.empty() &&
                std::find( messages.begin(), messages.end(), message ) == messages.end() )
                messages.push_back( message );
        }

        String makePayloadTemporaryPath( const String &outputPath )
        {
            const auto processId =
#if defined WP_PLATFORM_WIN32
                static_cast<u64>( _getpid() );
#else
                static_cast<u64>( getpid() );
#endif
            return outputPath + ".payload.tmp." + std::to_string( processId ).c_str() + "." +
                   std::to_string( ++payloadTemporaryCounter ).c_str();
        }

        struct TemporaryFile
        {
            String path;
            ~TemporaryFile()
            {
                if( !path.empty() )
                {
                    std::error_code ignored;
                    fs::remove( fs::u8path( path.c_str() ), ignored );
                }
            }
        };
    }  // namespace

    bool CompilationReport::succeeded() const
    {
        return status == CompilationStatus::Success ||
               status == CompilationStatus::SuccessWithWarnings || status == CompilationStatus::UpToDate;
    }

    class ResourceSystem::Impl
    {
    public:
        Impl( std::shared_ptr<IResourceCompilerRegistry> registryValue,
              std::shared_ptr<IResourceCompilationDatabase> databaseValue ) :
            registry( std::move( registryValue ) ),
            database( std::move( databaseValue ) )
        {
        }

        void invalidateBuildEvidence( const ResourceID &changed )
        {
            std::set<String> visited{ changed.str() };
            Array<String> pending{ changed.str() };
            for( size_t index = 0; index < pending.size(); ++index )
            {
                verifiedBuilds.erase( pending[index] );
                Array<String> dependents;
                if( !database->getDependents( pending[index], dependents ) )
                {
                    // A diagnostic-query failure must never leave stale publication evidence.
                    verifiedBuilds.clear();
                    return;
                }
                for( const auto &dependent : dependents )
                    if( visited.insert( dependent ).second )
                        pending.push_back( dependent );
            }
        }

        CompilationReport compileNode( const ResourceID &resourceId, const CompilationOptions &options,
                                       std::map<String, CompilationReport> &completed,
                                       Array<String> &stack )
        {
            const auto completedIterator = completed.find( resourceId.str() );
            if( completedIterator != completed.end() )
                return completedIterator->second;

            if( std::find( stack.begin(), stack.end(), resourceId.str() ) != stack.end() )
            {
                String cycle;
                for( const auto &entry : stack )
                    cycle += entry + " -> ";
                cycle += resourceId.str();
                auto report =
                    failureReport( resourceId, String( "Resource dependency cycle: " ) + cycle );
                completed[resourceId.str()] = report;
                return report;
            }

            stack.push_back( resourceId.str() );
            struct StackGuard
            {
                Array<String> &stack;
                ~StackGuard()
                {
                    stack.pop_back();
                }
            } stackGuard{ stack };

            auto fail = [&]( const String &message ) {
                auto report = failureReport( resourceId, message );
                completed[resourceId.str()] = report;
                return report;
            };

            const auto compiler = registry->getCompiler( resourceId.type() );
            if( !compiler )
                return fail( String( "No compiler registered for resource type '" ) +
                             resourceId.type().str() + "'" );

            const u64 compilerVersion = compiler->versionFor( resourceId.type() );
            if( compilerVersion == 0 )
                return fail( "The registered compiler did not declare this output type" );

            fs::path sourcePath;
            String error;
            if( !resolveUnderRoot( sourceRoot, String( "data://" ) + resourceId.sourceRelativePath(),
                                   true, sourcePath, error ) )
                return fail( error );

            fs::path outputPath;
            if( !resolveUnderRoot( compiledRoot, String( "data://.staging/" ) +
                                   std::to_string(resourceId.pathHash()).c_str() + ".wprs",
                                   false, outputPath, error ) )
                return fail( error );

            CompileContext context;
            context.resourceId = resourceId;
            context.sourcePath = pathString( sourcePath );
            context.outputPath = pathString( outputPath );
            context.sourceRoot = pathString( sourceRoot );
            context.compiledRoot = pathString( compiledRoot );
            context.target = config.target;
            context.packagedBuild = options.packagedBuild;
            if( resourceId.str().size()>1024 || config.target.size()>128 )
                return fail("Resource identity or target exceeds artifact namespace limits");

            u64 sourceFileHash=0, sourceFileSize=0;
            if(!CompiledResourceIO::hashFile(context.sourcePath,sourceFileHash,sourceFileSize,error))
                return fail(error);

            DependencySet dependencies;
            try
            {
                if( !compiler->getDependencies( context, dependencies, error ) )
                    return fail( error.empty() ? "Compiler failed to enumerate dependencies" : error );
            }
            catch( const std::exception &exception )
            {
                return fail( String( "Compiler dependency scan threw an exception: " ) +
                             exception.what() );
            }
            catch( ... )
            {
                return fail( "Compiler dependency scan threw an unknown exception" );
            }

            std::map<String, bool> canonicalDependencies;
            std::map<String, u64> dependencyHashes;
            std::map<String, ResourceID> installDependencies;

            for( const auto &dependency : dependencies.compileDependencies )
            {
                String canonicalPath;
                u64 dependencyHash = 0;
                if( dependency.isResource )
                {
                    ResourceID dependencyId;
                    if( !dependencyId.set( dependency.path, &error ) )
                        return fail( String( "Invalid resource dependency: " ) + error );
                    if( dependencyId == resourceId )
                        return fail( "A resource cannot depend on itself" );

                    const auto childReport = compileNode( dependencyId, options, completed, stack );
                    if( !childReport.succeeded() )
                    {
                        auto report =
                            fail( String( "Failed to compile dependency " ) + dependencyId.str() );
                        report.messages.insert( report.messages.end(), childReport.messages.begin(),
                                                childReport.messages.end() );
                        completed[resourceId.str()] = report;
                        return report;
                    }
                    canonicalPath = dependencyId.str();
                    dependencyHash = combineHash( childReport.sourceHash, childReport.outputHash );
                    context.dependencyArtifacts[dependencyId.str()]=childReport.outputPath;
                }
                else
                {
                    canonicalPath = normalizeLogicalDataPath( dependency.path, error );
                    if( canonicalPath.empty() )
                        return fail( error );
                    fs::path dependencyPath;
                    if( !resolveUnderRoot( sourceRoot, canonicalPath, true, dependencyPath, error ) )
                        return fail( error );
                    u64 dependencySize = 0;
                    if( !CompiledResourceIO::hashFile( pathString( dependencyPath ), dependencyHash,
                                                       dependencySize, error ) )
                        return fail( error );
                    dependencyHash = combineHash( dependencyHash, dependencySize );
                }

                const auto existing = canonicalDependencies.find( canonicalPath );
                if( existing != canonicalDependencies.end() &&
                    existing->second != dependency.isResource )
                    return fail( String( "Dependency has conflicting resource/data classifications: " ) +
                                 canonicalPath );
                canonicalDependencies[canonicalPath] = dependency.isResource;
                dependencyHashes[canonicalPath] = dependencyHash;
            }

            for( const auto &dependencyId : dependencies.installDependencies )
            {
                if( !dependencyId.isValid() )
                    return fail( "Compiler returned an invalid install dependency" );
                if( dependencyId == resourceId )
                    return fail( "A resource cannot install-depend on itself" );

                const auto childReport = compileNode( dependencyId, options, completed, stack );
                if( !childReport.succeeded() )
                {
                    auto report =
                        fail( String( "Failed to compile install dependency " ) + dependencyId.str() );
                    report.messages.insert( report.messages.end(), childReport.messages.begin(),
                                            childReport.messages.end() );
                    completed[resourceId.str()] = report;
                    return report;
                }
                installDependencies[dependencyId.str()] = dependencyId;
                context.dependencyArtifacts[dependencyId.str()]=childReport.outputPath;
                canonicalDependencies[dependencyId.str()] = true;
                dependencyHashes[dependencyId.str()] =
                    combineHash( childReport.sourceHash, childReport.outputHash );
            }

            u64 sourceHash = hashString( resourceId.str() );
            sourceHash = combineHash( sourceHash, compilerVersion );
            sourceHash = hashString( config.target, sourceHash );
            sourceHash = combineHash( sourceHash, options.packagedBuild ? 1u : 0u );
            sourceHash = combineHash( sourceHash, sourceFileHash );
            sourceHash = combineHash( sourceHash, sourceFileSize );
            for( const auto &dependency : dependencyHashes )
            {
                sourceHash = hashString( dependency.first, sourceHash );
                sourceHash = combineHash( sourceHash, dependency.second );
            }

            CompiledResourceRecord previousRecord;
            u64 scannedHash=0,scannedSize=0;
            if(!CompiledResourceIO::hashFile(context.sourcePath,scannedHash,scannedSize,error) ||
               scannedHash!=sourceFileHash || scannedSize!=sourceFileSize)
                return fail("Source changed during dependency scanning");
            if( !database->getRecord( resourceId.str(), previousRecord ) )
                return fail( String( "Failed to query compilation database: " ) +
                             database->getLastError() );

            if( !options.force && previousRecord.isValid() &&
                previousRecord.compilerVersion == compilerVersion &&
                previousRecord.sourceHash == sourceHash &&
                previousRecord.outputPath.find(artifactNamespace(config.target))==0 )
            {
                if(!resolveUnderRoot(compiledRoot,"data://"+previousRecord.outputPath,true,outputPath,error))
                    error.clear();
                else
                {
                context.outputPath=pathString(outputPath);
                CompiledResourceHeader existingHeader;
                if( fs::is_regular_file( outputPath ) &&
                    CompiledResourceIO::validate( context.outputPath, existingHeader, error ) &&
                    existingHeader.resourceId == resourceId &&
                    existingHeader.compilerVersion == compilerVersion &&
                    existingHeader.sourceHash == sourceHash &&
                    existingHeader.payloadHash == previousRecord.outputHash )
                {
                    CompilationReport report;
                    report.status = CompilationStatus::UpToDate;
                    report.resourceId = resourceId;
                    report.outputPath = context.outputPath;
                    report.sourceHash = sourceHash;
                    report.outputHash = previousRecord.outputHash;
                    completed[resourceId.str()] = report;
                    return report;
                }
                error.clear();
                }
            }

            std::error_code filesystemError;
            if(!resolveUnderRoot(compiledRoot,"data://.staging/" +
                String(std::to_string(resourceId.pathHash()).c_str()) + ".wprs",false,outputPath,error)) return fail(error);
            context.outputPath=pathString(outputPath);
            fs::create_directories( outputPath.parent_path(), filesystemError );
            if( filesystemError )
                return fail( String( "Failed to create compiled resource directory: " ) +
                             filesystemError.message().c_str() );

            TemporaryFile payloadFile{ makePayloadTemporaryPath( context.outputPath ) };
            std::ofstream payload( fs::u8path( payloadFile.path.c_str() ), std::ios::binary | std::ios::trunc );
            if( !payload )
                return fail( "Failed to create temporary compiled payload" );

            CompilationStatus status = CompilationStatus::Failure;
            Array<String> compilerMessages;
            const auto compileStart = std::chrono::steady_clock::now();
            try
            {
                status = compiler->compile( context, payload, compilerMessages );
            }
            catch( const std::exception &exception )
            {
                appendUnique( compilerMessages,
                              String( "Compiler threw an exception: " ) + exception.what() );
                status = CompilationStatus::Failure;
            }
            catch( ... )
            {
                appendUnique( compilerMessages, "Compiler threw an unknown exception" );
                status = CompilationStatus::Failure;
            }
            payload.flush();
            const bool payloadWriteSucceeded = payload.good();
            payload.close();

            if( status != CompilationStatus::Success &&
                status != CompilationStatus::SuccessWithWarnings )
            {
                auto report = fail( "Resource compiler reported failure" );
                report.messages.insert( report.messages.end(), compilerMessages.begin(),
                                        compilerMessages.end() );
                completed[resourceId.str()] = report;
                return report;
            }
            if( !payloadWriteSucceeded )
                return fail( "Resource compiler failed while writing its payload" );

            u64 checkedHash=0, checkedSize=0;
            if(!CompiledResourceIO::hashFile(context.sourcePath,checkedHash,checkedSize,error) ||
               checkedHash!=sourceFileHash || checkedSize!=sourceFileSize ||
               compiler->versionFor(resourceId.type())!=compilerVersion)
                return fail("Source or compiler changed during compilation; artifact was not committed");
            for(const auto &dependency:canonicalDependencies)
            {
                if(dependency.second)
                {
                    CompiledResourceHeader pinnedHeader;
                    if(!CompiledResourceIO::validate(context.dependencyArtifacts.at(dependency.first),pinnedHeader,error) ||
                       pinnedHeader.resourceId.str()!=dependency.first ||
                       combineHash(pinnedHeader.sourceHash,pinnedHeader.payloadHash)!=dependencyHashes.at(dependency.first))
                        return fail("Compiled dependency changed during compilation");
                }
                else
                {
                    fs::path checkedPath;
                    if(!resolveUnderRoot(sourceRoot,dependency.first,true,checkedPath,error) ||
                       !CompiledResourceIO::hashFile(pathString(checkedPath),checkedHash,checkedSize,error) ||
                       combineHash(checkedHash,checkedSize)!=dependencyHashes.at(dependency.first))
                        return fail("Raw dependency changed during compilation");
                }
            }

            u64 payloadHash = 0;
            u64 payloadSize = 0;
            if( !CompiledResourceIO::hashFile( payloadFile.path, payloadHash, payloadSize, error ) )
                return fail( error );

            CompiledResourceHeader header;
            header.resourceId = resourceId;
            header.resourceType = resourceId.type();
            header.compilerVersion = compilerVersion;
            header.sourceHash = sourceHash;
            header.payloadHash = payloadHash;
            header.payloadSize = payloadSize;
            for( const auto &dependency : installDependencies )
                header.installDependencies.push_back( dependency.second );

            String relativeArtifact=artifactNamespace(config.target) +
                (options.packagedBuild?"package/":"editor/") + encodedArtifactKey(resourceId.str()) + "/" +
                std::to_string(sourceHash).c_str() + "-" + std::to_string(payloadHash).c_str() + ".wprs";
            if(!resolveUnderRoot(compiledRoot,"data://"+relativeArtifact,false,outputPath,error)) return fail(error);
            CompiledResourceHeader existing;
            if(fs::exists(outputPath) && !CompiledResourceIO::validate(pathString(outputPath),existing,error))
            {
                // Never overwrite a damaged generation that an existing package may pin.
                const auto base=relativeArtifact.substr(0,relativeArtifact.size()-5);
                do
                {
                    relativeArtifact=base + "-repair-" +
                        std::to_string(payloadTemporaryCounter.fetch_add(1)).c_str() + ".wprs";
                    if(!resolveUnderRoot(compiledRoot,"data://"+relativeArtifact,false,outputPath,error)) return fail(error);
                } while(fs::exists(outputPath));
            }
            context.outputPath=pathString(outputPath);
            fs::create_directories(outputPath.parent_path(),filesystemError);
            if(filesystemError) return fail(filesystemError.message().c_str());
            if(fs::exists(outputPath))
            {
                if(!CompiledResourceIO::validate(context.outputPath,existing,error) || !sameRuntimeHeader(existing,header))
                    return fail("Artifact ownership/hash collision; existing generation retained");
            }
            else
            {
                TemporaryFile stagedContainer{payloadFile.path + ".container"};
                if(!CompiledResourceIO::writeAtomic(stagedContainer.path,header,payloadFile.path,error)) return fail(error);
                // Exclusive publication prevents another process's artifact from being replaced.
                fs::create_hard_link(fs::u8path(stagedContainer.path.c_str()),outputPath,filesystemError);
                if(filesystemError && (!fs::exists(outputPath) ||
                   !CompiledResourceIO::validate(context.outputPath,existing,error) || !sameRuntimeHeader(existing,header)))
                    return fail("Exclusive artifact publication failed; existing files retained");
            }

            CompiledResourceRecord newRecord;
            newRecord.resourceId = resourceId.str();
            newRecord.resourceType = resourceId.type().str();
            newRecord.outputPath = relativeArtifact;
            newRecord.compilerVersion = compilerVersion;
            newRecord.sourceHash = sourceHash;
            newRecord.outputHash = payloadHash;

            Array<CompileDependencyRecord> databaseDependencies;
            databaseDependencies.reserve( canonicalDependencies.size() );
            for( const auto &dependency : canonicalDependencies )
                databaseDependencies.push_back( { dependency.first, dependency.second } );
            if( !database->commitCompilation( newRecord, databaseDependencies ) )
                return fail(
                    String( "Artifact staged but compilation index commit failed; previous generation retained: " ) +
                    database->getLastError() );

            {
                std::unique_lock<std::shared_mutex> cacheLock( cacheMutex );
                // A cached parent owns its prior install dependency snapshot. Invalidating
                // only the child would let the next parent load silently reuse that snapshot.
                runtimeCache.clear();
            }
            invalidateBuildEvidence( resourceId );

            CompilationReport report;
            report.status = status;
            report.resourceId = resourceId;
            report.outputPath = context.outputPath;
            report.sourceHash = sourceHash;
            report.outputHash = payloadHash;
            report.messages = std::move( compilerMessages );
            report.elapsedMilliseconds =
                std::chrono::duration<f64, std::milli>( std::chrono::steady_clock::now() - compileStart )
                    .count();
            completed[resourceId.str()] = report;
            return report;
        }

        struct LoadRequest
        {
            std::set<String> stack;
            std::map<String, std::shared_ptr<const RuntimeResource>> completed;
            std::map<String, u32> subtreeDepth;
            u64 payloadBytes = 0;
        };

        std::shared_ptr<const RuntimeResource> loadNode( const ResourceID &resourceId,
                                                        LoadRequest &request, String &error )
        {
            if( request.stack.size() >= runtimeConfig.maxDependencyDepth )
            {
                error = "Runtime install dependency depth exceeds the configured limit";
                return nullptr;
            }
            if( request.stack.count( resourceId.str() ) )
            {
                error = String( "Runtime install dependency cycle at " ) + resourceId.str();
                return nullptr;
            }
            const auto complete = request.completed.find( resourceId.str() );
            if( complete != request.completed.end() )
            {
                if( request.stack.size() + request.subtreeDepth[resourceId.str()] >
                    runtimeConfig.maxDependencyDepth )
                {
                    error = "Runtime install dependency depth exceeds the configured limit";
                    return nullptr;
                }
                return complete->second;
            }
            if( request.completed.size() + request.stack.size() >= runtimeConfig.maxClosureResources )
            {
                error = "Runtime install closure resource count exceeds the configured limit";
                return nullptr;
            }
            const auto pinned = runtimeManifest.resources.find( resourceId.str() );
            if( runtimeOnly && pinned == runtimeManifest.resources.end() )
            {
                error = String( "Resource is not present in the runtime manifest: " ) + resourceId.str();
                return nullptr;
            }

            std::shared_ptr<const RuntimeResource> cached;
            {
                std::shared_lock<std::shared_mutex> lock( cacheMutex );
                const auto iterator = runtimeCache.find( resourceId.str() );
                if( iterator != runtimeCache.end() )
                    cached = iterator->second.lock();
            }
            request.stack.insert( resourceId.str() );
            struct StackGuard
            {
                std::set<String> &stack;
                String id;
                ~StackGuard()
                {
                    stack.erase( id );
                }
            } stackGuard{ request.stack, resourceId.str() };

            std::shared_ptr<RuntimeResource> loaded;
            if( !cached )
            {
                fs::path outputPath;
                String artifact;
                if(runtimeOnly)
                    artifact=runtimeManifest.artifacts.at(resourceId.str());
                else
                {
                    CompiledResourceRecord record;
                    if(!database->getRecord(resourceId.str(),record) || !record.isValid() ||
                        record.outputPath.find(artifactNamespace(config.target))!=0)
                    {
                        error="No committed artifact for this resource and target";
                        return nullptr;
                    }
                    artifact=record.outputPath;
                }
                if( !resolveUnderRoot( compiledRoot, String( "data://" ) + artifact,
                                       true, outputPath, error ) )
                    return nullptr;
                loaded = std::make_shared<RuntimeResource>();
                const auto remaining = runtimeConfig.maxClosurePayloadBytes - request.payloadBytes;
                if( !CompiledResourceIO::read( pathString( outputPath ), *loaded, error,
                                               std::min( runtimeConfig.maxPayloadBytes, remaining ) ) )
                    return nullptr;
            }
            const auto &header = cached ? cached->header : loaded->header;
            if( header.resourceId != resourceId || header.resourceType != resourceId.type() )
            {
                error = String( "Compiled resource identity mismatch for " ) + resourceId.str();
                return nullptr;
            }
            if( runtimeOnly && !sameRuntimeHeader( header, pinned->second ) )
            {
                error = String( "Compiled resource does not match the pinned runtime manifest: " ) +
                        resourceId.str();
                return nullptr;
            }
            if( header.payloadSize > runtimeConfig.maxPayloadBytes ||
                header.payloadSize > runtimeConfig.maxClosurePayloadBytes - request.payloadBytes )
            {
                error = "Runtime install closure payload exceeds the configured limit";
                return nullptr;
            }
            request.payloadBytes += header.payloadSize;

            if( loaded )
                loaded->dependencies.reserve( header.installDependencies.size() );
            u32 subtreeDepth = 1;
            for( const auto &dependencyId : header.installDependencies )
            {
                auto dependency = loadNode( dependencyId, request, error );
                if( !dependency )
                    return nullptr;
                subtreeDepth = std::max( subtreeDepth, request.subtreeDepth[dependencyId.str()] + 1 );
                if( loaded )
                    loaded->dependencies.push_back( std::move( dependency ) );
            }
            std::shared_ptr<const RuntimeResource> result = cached ? cached : loaded;
            request.completed.emplace( resourceId.str(), result );
            request.subtreeDepth[resourceId.str()] = subtreeDepth;
            return result;
        }

        std::shared_ptr<IResourceCompilerRegistry> registry;
        std::shared_ptr<IResourceCompilationDatabase> database;
        ResourceSystemConfig config;
        RuntimeResourceConfig runtimeConfig;
        RuntimeManifest runtimeManifest;
        std::map<String, CompilationReport> verifiedBuilds;
        fs::path sourceRoot;
        fs::path compiledRoot;
        mutable std::mutex stateMutex;
        mutable std::shared_mutex operationMutex;
        mutable std::shared_mutex cacheMutex;
        mutable std::mutex runtimeLoadMutex;
        std::map<String, std::weak_ptr<const RuntimeResource>> runtimeCache;
        bool initialized = false;
        bool runtimeOnly = false;
    };

    ResourceSystem::ResourceSystem( std::shared_ptr<IResourceCompilerRegistry> registry,
                                    std::shared_ptr<IResourceCompilationDatabase> database ) :
        m_impl( new Impl( std::move( registry ), std::move( database ) ) )
    {
    }

    ResourceSystem::~ResourceSystem()
    {
        shutdown();
        delete m_impl;
        m_impl = nullptr;
    }

    bool ResourceSystem::initialize( const ResourceSystemConfig &config, String &error )
    {
        error.clear();
        std::unique_lock<std::shared_mutex> operationLock( m_impl->operationMutex );
        std::lock_guard<std::mutex> stateLock( m_impl->stateMutex );
        if( m_impl->initialized )
        {
            error = "Resource system is already initialized";
            return false;
        }
        if( !m_impl->registry || !m_impl->database || config.sourceRoot.empty() ||
            config.compiledRoot.empty() || config.target.empty() || config.target.size()>128 || config.maxRuntimePayloadBytes == 0 )
        {
            error = "Resource system configuration is incomplete";
            return false;
        }

        std::error_code filesystemError;
        fs::path source = fs::canonical( fs::u8path( config.sourceRoot.c_str() ), filesystemError );
        if( filesystemError || !fs::is_directory( source, filesystemError ) )
        {
            error =
                String( "Resource source root is not an accessible directory: " ) + config.sourceRoot;
            return false;
        }

        fs::path compiled = fs::absolute( fs::u8path( config.compiledRoot.c_str() ), filesystemError );
        if( filesystemError )
        {
            error = String( "Failed to resolve compiled resource root: " ) +
                    filesystemError.message().c_str();
            return false;
        }
        fs::create_directories( compiled, filesystemError );
        if( filesystemError )
        {
            error = String( "Failed to create compiled resource root: " ) +
                    filesystemError.message().c_str();
            return false;
        }
        compiled = fs::canonical( compiled, filesystemError );
        if( filesystemError )
        {
            error = String( "Failed to canonicalize compiled resource root: " ) +
                    filesystemError.message().c_str();
            return false;
        }

        String databasePath = config.databasePath;
        if( databasePath.empty() )
            databasePath = pathString( compiled / ".resource_compilation.db" );
        if( !m_impl->database->connect( databasePath ) )
        {
            error = String( "Failed to open resource compilation database: " ) +
                    m_impl->database->getLastError();
            return false;
        }

        m_impl->config = config;
        m_impl->config.databasePath = databasePath;
        m_impl->sourceRoot = std::move( source );
        m_impl->compiledRoot = std::move( compiled );
        m_impl->runtimeConfig = RuntimeResourceConfig();
        m_impl->runtimeConfig.maxPayloadBytes = config.maxRuntimePayloadBytes;
        m_impl->runtimeConfig.maxClosurePayloadBytes = config.maxRuntimePayloadBytes;
        m_impl->runtimeManifest = RuntimeManifest();
        m_impl->verifiedBuilds.clear();
        m_impl->runtimeOnly = false;
        m_impl->initialized = true;
        return true;
    }

    bool ResourceSystem::initializeRuntime( const RuntimeResourceConfig &config, String &error )
    {
        error.clear();
        std::unique_lock<std::shared_mutex> operationLock( m_impl->operationMutex );
        std::lock_guard<std::mutex> stateLock( m_impl->stateMutex );
        if( m_impl->initialized )
        {
            error = "Resource system is already initialized";
            return false;
        }
        if( config.compiledRoot.empty() || config.manifestPath.empty() || config.target.empty() || config.target.size()>128 ||
            config.maxPayloadBytes == 0 || config.maxClosurePayloadBytes == 0 ||
            config.maxClosureResources == 0 || config.maxClosureResources > maximumManifestResources ||
            config.maxDependencyDepth == 0 || config.maxDependencyDepth > maximumManifestDepth )
        {
            error = "Runtime resource configuration is incomplete or exceeds its limits";
            return false;
        }
        std::error_code filesystemError;
        auto compiled = fs::canonical( fs::u8path( config.compiledRoot.c_str() ), filesystemError );
        if( filesystemError || !fs::is_directory( compiled, filesystemError ) )
        {
            error = "Runtime compiled root must be an existing accessible directory";
            return false;
        }
        RuntimeManifest manifest;
        if( !readManifest( config.manifestPath, manifest, error ) )
            return false;
        if( manifest.target != config.target )
        {
            error = "Runtime manifest target does not match the requested target";
            return false;
        }
        m_impl->runtimeConfig = config;
        m_impl->runtimeManifest = std::move( manifest );
        m_impl->compiledRoot = std::move( compiled );
        m_impl->sourceRoot.clear();
        m_impl->runtimeOnly = true;
        m_impl->initialized = true;
        return true;
    }

    bool ResourceSystem::writeRuntimeManifest( const String &path, const Array<ResourceID> &roots,
                                               String &error )
    {
        error.clear();
        std::unique_lock<std::shared_mutex> operationLock( m_impl->operationMutex );
        if( !m_impl->initialized || m_impl->runtimeOnly )
        {
            error = "Runtime manifests can only be exported by an initialized authoring resource system";
            return false;
        }
        if( path.empty() || roots.empty() || roots.size() > maximumManifestResources )
        {
            error = "A runtime manifest requires a destination and at least one valid root";
            return false;
        }
        std::error_code filesystemError;
        const auto destination = fs::weakly_canonical( fs::u8path( path.c_str() ), filesystemError );
        if( filesystemError )
        {
            error = "Failed to resolve the runtime manifest destination";
            return false;
        }
        if(isUnderRoot(m_impl->compiledRoot / "artifacts",destination) ||
           isUnderRoot(m_impl->compiledRoot / ".staging",destination))
        {
            error="Runtime manifests cannot replace files in reserved artifact or staging storage";
            return false;
        }
        RuntimeManifest manifest;
        manifest.target = m_impl->config.target;
        std::set<String> visiting;
        std::map<String, u32> subtreeDepths;
        std::function<bool( const ResourceID & )> visit = [&]( const ResourceID &id ) {
            if( !id.isValid() || visiting.count( id.str() ) || visiting.size() >= maximumManifestDepth )
            {
                error = "Invalid or cyclic runtime manifest dependency graph";
                return false;
            }
            if( manifest.resources.count( id.str() ) )
            {
                if( visiting.size() + subtreeDepths[id.str()] > maximumManifestDepth )
                {
                    error = "Runtime manifest dependency depth exceeds its format limit";
                    return false;
                }
                return true;
            }
            if( manifest.resources.size() + visiting.size() >= maximumManifestResources )
            {
                error = "Runtime manifest resource count exceeds its format limit";
                return false;
            }
            fs::path output;
            CompiledResourceRecord record;
            if(!m_impl->database->getRecord(id.str(),record) || !record.isValid() ||
               record.outputPath.find(artifactNamespace(m_impl->config.target))!=0)
            {
                error="No committed artifact for runtime manifest resource";
                return false;
            }
            if( !resolveUnderRoot( m_impl->compiledRoot, String( "data://" ) + record.outputPath,
                                   true, output, error ) )
                return false;
            if( pathComponentEquals( destination, output ) )
            {
                error = "A runtime manifest cannot replace one of its resource containers";
                return false;
            }
            CompiledResourceHeader header;
            if( !CompiledResourceIO::validate( pathString( output ), header, error ) )
                return false;
            const auto verified = m_impl->verifiedBuilds.find( id.str() );
            if( !m_impl->database->getRecord( id.str(), record ) || !record.isValid() ||
                header.resourceId != id || header.compilerVersion != record.compilerVersion ||
                header.sourceHash != record.sourceHash || header.payloadHash != record.outputHash ||
                verified == m_impl->verifiedBuilds.end() ||
                verified->second.sourceHash != header.sourceHash ||
                verified->second.outputHash != header.payloadHash )
            {
                error = String( "Runtime manifest resource must be compiled or checked up to date "
                                "for this target in the current authoring session: " ) + id.str();
                return false;
            }
            visiting.insert( id.str() );
            u32 subtreeDepth = 1;
            for( const auto &dependency : header.installDependencies )
            {
                if( !visit( dependency ) )
                    return false;
                subtreeDepth = std::max( subtreeDepth, subtreeDepths[dependency.str()] + 1 );
            }
            visiting.erase( id.str() );
            manifest.artifacts[id.str()]=record.outputPath;
            manifest.resources.emplace( id.str(), std::move( header ) );
            subtreeDepths[id.str()] = subtreeDepth;
            return true;
        };
        std::map<String, ResourceID> sortedRoots;
        for( const auto &root : roots )
        {
            if( !visit( root ) )
                return false;
            sortedRoots[root.str()] = root;
        }

        std::ostringstream encoded( std::ios::binary );
        writeManifestNumber( encoded, 2 );
        bool stringsValid = writeManifestString( encoded, manifest.target );
        writeManifestNumber( encoded, sortedRoots.size() );
        for( const auto &root : sortedRoots )
            stringsValid = writeManifestString( encoded, root.first ) && stringsValid;
        writeManifestNumber( encoded, manifest.resources.size() );
        for( const auto &entry : manifest.resources )
        {
            const auto &header = entry.second;
            stringsValid = writeManifestString( encoded, entry.first ) && stringsValid;
            writeManifestNumber( encoded, header.compilerVersion );
            writeManifestNumber( encoded, header.sourceHash );
            writeManifestNumber( encoded, header.payloadHash );
            writeManifestNumber( encoded, header.payloadSize );
            writeManifestNumber( encoded, header.installDependencies.size() );
            stringsValid=writeManifestString(encoded,manifest.artifacts.at(entry.first)) && stringsValid;
            for( const auto &dependency : header.installDependencies )
                stringsValid = writeManifestString( encoded, dependency.str() ) && stringsValid;
            if( encoded.tellp() > static_cast<std::streamoff>( maximumManifestBytes ) )
                break;
        }
        const auto payload = encoded.str();
        if( !stringsValid || !encoded || payload.size() > maximumManifestBytes )
        {
            error = "Runtime manifest exceeds its format limits";
            return false;
        }
        fs::create_directories( destination.parent_path(), filesystemError );
        if( filesystemError )
        {
            error = "Failed to create runtime manifest destination directory";
            return false;
        }
        TemporaryFile temporary{ makePayloadTemporaryPath( pathString( destination ) ) };
        std::ofstream output( fs::u8path( temporary.path.c_str() ), std::ios::binary | std::ios::trunc );
        output.write( payload.data(), static_cast<std::streamsize>( payload.size() ) );
        output.flush();
        const bool written = output.good();
        output.close();
        if( !written || output.fail() )
        {
            error = "Failed to write runtime manifest staging payload";
            return false;
        }
        CompiledResourceHeader header;
        header.resourceId = manifestResourceId;
        header.resourceType = manifestResourceId.type();
        header.compilerVersion = 1;
        header.sourceHash = hashBytes( payload.data(), payload.size() );
        header.payloadHash = header.sourceHash;
        header.payloadSize = payload.size();
        return CompiledResourceIO::writeAtomic( pathString( destination ), header, temporary.path, error );
    }

    void ResourceSystem::shutdown()
    {
        if( !m_impl )
            return;
        std::unique_lock<std::shared_mutex> operationLock( m_impl->operationMutex );
        std::lock_guard<std::mutex> stateLock( m_impl->stateMutex );
        if( !m_impl->initialized )
            return;
        {
            std::unique_lock<std::shared_mutex> cacheLock( m_impl->cacheMutex );
            m_impl->runtimeCache.clear();
        }
        if( !m_impl->runtimeOnly )
            m_impl->database->disconnect();
        m_impl->runtimeManifest = RuntimeManifest();
        m_impl->verifiedBuilds.clear();
        m_impl->sourceRoot.clear();
        m_impl->compiledRoot.clear();
        m_impl->initialized = false;
        m_impl->runtimeOnly = false;
    }

    bool ResourceSystem::isInitialized() const
    {
        std::lock_guard<std::mutex> lock( m_impl->stateMutex );
        return m_impl->initialized;
    }

    CompilationReport ResourceSystem::compile( const ResourceID &resourceId,
                                               const CompilationOptions &options )
    {
        if( !resourceId.isValid() )
            return failureReport( resourceId, "Cannot compile an invalid resource ID" );
        std::unique_lock<std::shared_mutex> operationLock( m_impl->operationMutex );
        {
            std::lock_guard<std::mutex> stateLock( m_impl->stateMutex );
            if( !m_impl->initialized )
                return failureReport( resourceId, "Resource system is not initialized" );
            if( m_impl->runtimeOnly )
                return failureReport( resourceId, "Cooking is unavailable in a read-only runtime mount" );
        }
        std::map<String, CompilationReport> completed;
        Array<String> stack;
        auto report = m_impl->compileNode( resourceId, options, completed, stack );
        for( const auto &entry : completed )
            if( entry.second.succeeded() )
                m_impl->verifiedBuilds[entry.first] = entry.second;
        return report;
    }

    Array<CompilationReport> ResourceSystem::compileAffected( const String &changedPath,
                                                              const CompilationOptions &options )
    {
        Array<CompilationReport> reports;
        std::unique_lock<std::shared_mutex> operationLock( m_impl->operationMutex );
        {
            std::lock_guard<std::mutex> stateLock( m_impl->stateMutex );
            if( !m_impl->initialized )
            {
                reports.push_back( failureReport( ResourceID(), "Resource system is not initialized" ) );
                return reports;
            }
            if( m_impl->runtimeOnly )
            {
                reports.push_back( failureReport( ResourceID(), "Cooking is unavailable in a read-only runtime mount" ) );
                return reports;
            }
        }

        String error;
        const String canonicalPath = normalizeLogicalDataPath( changedPath, error );
        if( canonicalPath.empty() )
        {
            reports.push_back( failureReport( ResourceID(), error ) );
            return reports;
        }

        std::set<String> affected;
        Array<String> pending{ canonicalPath };
        for( size_t index = 0; index < pending.size(); ++index )
        {
            Array<String> direct;
            if( !m_impl->database->getDependents( pending[index], direct ) )
            {
                reports.push_back(
                    failureReport( ResourceID(), String( "Failed to query affected resources: " ) +
                                                     m_impl->database->getLastError() ) );
                return reports;
            }
            for( const auto &id : direct )
            {
                if( affected.insert( id ).second )
                    pending.push_back( id );
            }
        }

        ResourceID changedResource( canonicalPath );
        if( changedResource.isValid() && m_impl->registry->hasCompiler( changedResource.type() ) )
            affected.insert( changedResource.str() );

        std::map<String, CompilationReport> completed;
        Array<String> stack;
        for( const auto &id : affected )
            reports.push_back( m_impl->compileNode( ResourceID( id ), options, completed, stack ) );
        for( const auto &entry : completed )
            if( entry.second.succeeded() )
                m_impl->verifiedBuilds[entry.first] = entry.second;
        return reports;
    }

    std::shared_ptr<const RuntimeResource> ResourceSystem::load( const ResourceID &resourceId,
                                                                 String &error )
    {
        error.clear();
        if( !resourceId.isValid() )
        {
            error = "Cannot load an invalid resource ID";
            return nullptr;
        }
        std::shared_lock<std::shared_mutex> operationLock( m_impl->operationMutex );
        {
            std::lock_guard<std::mutex> stateLock( m_impl->stateMutex );
            if( !m_impl->initialized )
            {
                error = "Resource system is not initialized";
                return nullptr;
            }
        }
        // Synchronous requests share one retained identity even when separate roots
        // are loaded concurrently. Async scheduling and residency budgets remain separate work.
        std::lock_guard<std::mutex> loadLock( m_impl->runtimeLoadMutex );
        Impl::LoadRequest request;
        auto loaded = m_impl->loadNode( resourceId, request, error );
        if( !loaded )
            return nullptr;
        std::unique_lock<std::shared_mutex> cacheLock( m_impl->cacheMutex );
        for( const auto &entry : request.completed )
            m_impl->runtimeCache[entry.first] = entry.second;
        return loaded;
    }

    void ResourceSystem::unload( const ResourceID &resourceId )
    {
        (void)resourceId;
        std::unique_lock<std::shared_mutex> operationLock( m_impl->operationMutex );
        std::unique_lock<std::shared_mutex> lock( m_impl->cacheMutex );
        m_impl->runtimeCache.clear();
    }

    void ResourceSystem::clearRuntimeCache()
    {
        std::unique_lock<std::shared_mutex> operationLock( m_impl->operationMutex );
        std::unique_lock<std::shared_mutex> lock( m_impl->cacheMutex );
        m_impl->runtimeCache.clear();
    }

    std::shared_ptr<IResourceCompilerRegistry> ResourceSystem::registry() const
    {
        return m_impl->registry;
    }
}  // namespace workphone::resource
