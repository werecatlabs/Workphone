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
#include <map>
#include <mutex>
#include <set>
#include <shared_mutex>

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
                    fs::remove( path.c_str(), ignored );
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
            if( !resolveUnderRoot( compiledRoot, String( "data://" ) + resourceId.compiledRelativePath(),
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
                canonicalDependencies[dependencyId.str()] = true;
                dependencyHashes[dependencyId.str()] =
                    combineHash( childReport.sourceHash, childReport.outputHash );
            }

            u64 sourceFileHash = 0;
            u64 sourceFileSize = 0;
            if( !CompiledResourceIO::hashFile( context.sourcePath, sourceFileHash, sourceFileSize,
                                               error ) )
                return fail( error );

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
            if( !database->getRecord( resourceId.str(), previousRecord ) )
                return fail( String( "Failed to query compilation database: " ) +
                             database->getLastError() );

            if( !options.force && previousRecord.isValid() &&
                previousRecord.compilerVersion == compilerVersion &&
                previousRecord.sourceHash == sourceHash &&
                previousRecord.outputPath == resourceId.compiledRelativePath() )
            {
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

            std::error_code filesystemError;
            fs::create_directories( outputPath.parent_path(), filesystemError );
            if( filesystemError )
                return fail( String( "Failed to create compiled resource directory: " ) +
                             filesystemError.message().c_str() );

            TemporaryFile payloadFile{ makePayloadTemporaryPath( context.outputPath ) };
            std::ofstream payload( payloadFile.path.c_str(), std::ios::binary | std::ios::trunc );
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

            if( !CompiledResourceIO::writeAtomic( context.outputPath, header, payloadFile.path, error ) )
                return fail( error );

            CompiledResourceRecord newRecord;
            newRecord.resourceId = resourceId.str();
            newRecord.resourceType = resourceId.type().str();
            newRecord.outputPath = resourceId.compiledRelativePath();
            newRecord.compilerVersion = compilerVersion;
            newRecord.sourceHash = sourceHash;
            newRecord.outputHash = payloadHash;

            Array<CompileDependencyRecord> databaseDependencies;
            databaseDependencies.reserve( canonicalDependencies.size() );
            for( const auto &dependency : canonicalDependencies )
                databaseDependencies.push_back( { dependency.first, dependency.second } );
            if( !database->commitCompilation( newRecord, databaseDependencies ) )
                return fail(
                    String( "Compiled output was written but the compilation database could not "
                            "be updated: " ) +
                    database->getLastError() );

            {
                std::unique_lock<std::shared_mutex> cacheLock( cacheMutex );
                runtimeCache.erase( resourceId.str() );
            }

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

        std::shared_ptr<const RuntimeResource> loadNode( const ResourceID &resourceId,
                                                         std::set<String> &stack, String &error )
        {
            {
                std::shared_lock<std::shared_mutex> lock( cacheMutex );
                const auto iterator = runtimeCache.find( resourceId.str() );
                if( iterator != runtimeCache.end() )
                {
                    if( auto existing = iterator->second.lock() )
                        return existing;
                }
            }

            if( !stack.insert( resourceId.str() ).second )
            {
                error = String( "Runtime install dependency cycle at " ) + resourceId.str();
                return nullptr;
            }
            struct StackGuard
            {
                std::set<String> &stack;
                String id;
                ~StackGuard()
                {
                    stack.erase( id );
                }
            } stackGuard{ stack, resourceId.str() };

            fs::path outputPath;
            if( !resolveUnderRoot( compiledRoot, String( "data://" ) + resourceId.compiledRelativePath(),
                                   true, outputPath, error ) )
                return nullptr;

            auto loaded = std::make_shared<RuntimeResource>();
            if( !CompiledResourceIO::read( pathString( outputPath ), *loaded, error,
                                           config.maxRuntimePayloadBytes ) )
                return nullptr;
            if( loaded->header.resourceId != resourceId ||
                loaded->header.resourceType != resourceId.type() )
            {
                error = String( "Compiled resource identity mismatch for " ) + resourceId.str();
                return nullptr;
            }

            loaded->dependencies.reserve( loaded->header.installDependencies.size() );
            for( const auto &dependencyId : loaded->header.installDependencies )
            {
                auto dependency = loadNode( dependencyId, stack, error );
                if( !dependency )
                    return nullptr;
                loaded->dependencies.push_back( std::move( dependency ) );
            }

            std::unique_lock<std::shared_mutex> lock( cacheMutex );
            const auto iterator = runtimeCache.find( resourceId.str() );
            if( iterator != runtimeCache.end() )
            {
                if( auto existing = iterator->second.lock() )
                    return existing;
            }
            runtimeCache[resourceId.str()] = loaded;
            return loaded;
        }

        std::shared_ptr<IResourceCompilerRegistry> registry;
        std::shared_ptr<IResourceCompilationDatabase> database;
        ResourceSystemConfig config;
        fs::path sourceRoot;
        fs::path compiledRoot;
        mutable std::mutex stateMutex;
        mutable std::shared_mutex operationMutex;
        mutable std::shared_mutex cacheMutex;
        std::map<String, std::weak_ptr<const RuntimeResource>> runtimeCache;
        bool initialized = false;
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
        std::unique_lock<std::shared_mutex> operationLock( m_impl->operationMutex );
        std::lock_guard<std::mutex> stateLock( m_impl->stateMutex );
        if( m_impl->initialized )
        {
            error = "Resource system is already initialized";
            return false;
        }
        if( !m_impl->registry || !m_impl->database || config.sourceRoot.empty() ||
            config.compiledRoot.empty() || config.target.empty() || config.maxRuntimePayloadBytes == 0 )
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
        m_impl->initialized = true;
        return true;
    }

    void ResourceSystem::shutdown()
    {
        if( !m_impl )
            return;
        std::unique_lock<std::shared_mutex> operationLock( m_impl->operationMutex );
        std::lock_guard<std::mutex> stateLock( m_impl->stateMutex );
        if( !m_impl->initialized )
            return;
        clearRuntimeCache();
        m_impl->database->disconnect();
        m_impl->sourceRoot.clear();
        m_impl->compiledRoot.clear();
        m_impl->initialized = false;
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
        }
        std::map<String, CompilationReport> completed;
        Array<String> stack;
        return m_impl->compileNode( resourceId, options, completed, stack );
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
        return reports;
    }

    std::shared_ptr<const RuntimeResource> ResourceSystem::load( const ResourceID &resourceId,
                                                                 String &error )
    {
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
        std::set<String> stack;
        return m_impl->loadNode( resourceId, stack, error );
    }

    void ResourceSystem::unload( const ResourceID &resourceId )
    {
        std::unique_lock<std::shared_mutex> lock( m_impl->cacheMutex );
        m_impl->runtimeCache.erase( resourceId.str() );
    }

    void ResourceSystem::clearRuntimeCache()
    {
        std::unique_lock<std::shared_mutex> lock( m_impl->cacheMutex );
        m_impl->runtimeCache.clear();
    }

    std::shared_ptr<IResourceCompilerRegistry> ResourceSystem::registry() const
    {
        return m_impl->registry;
    }
}  // namespace workphone::resource
