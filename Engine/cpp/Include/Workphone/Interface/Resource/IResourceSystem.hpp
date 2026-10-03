#ifndef IResourceSystem_h__
#define IResourceSystem_h__

#include <Workphone/Interface/Resource/IResourceCompilerRegistry.hpp>
#include <Workphone/System/CompiledResource.hpp>

#include <memory>

namespace workphone::resource
{
    struct WPCore_API ResourceSystemConfig
    {
        String sourceRoot;
        String compiledRoot;
        String databasePath;
        String target = "pc";
        u64 maxRuntimePayloadBytes = 1024ull * 1024ull * 1024ull;
    };

    struct WPCore_API CompilationOptions
    {
        bool force = false;
        bool packagedBuild = false;
    };

    struct WPCore_API CompilationReport
    {
        CompilationStatus status = CompilationStatus::Failure;
        ResourceID resourceId;
        String outputPath;
        u64 sourceHash = 0;
        u64 outputHash = 0;
        f64 elapsedMilliseconds = 0.0;
        Array<String> messages;

        bool succeeded() const;
    };

    /** Abstract dependency-aware resource build and runtime service. */
    class WPCore_API IResourceSystem
    {
    public:
        virtual ~IResourceSystem() = default;

        virtual bool initialize( const ResourceSystemConfig &config, String &error ) = 0;
        virtual void shutdown() = 0;
        virtual bool isInitialized() const = 0;

        virtual CompilationReport compile( const ResourceID &resourceId,
                                           const CompilationOptions &options = {} ) = 0;

        virtual Array<CompilationReport> compileAffected( const String &changedPath,
                                                          const CompilationOptions &options = {} ) = 0;

        virtual std::shared_ptr<const RuntimeResource> load( const ResourceID &resourceId,
                                                             String &error ) = 0;
        virtual void unload( const ResourceID &resourceId ) = 0;
        virtual void clearRuntimeCache() = 0;

        virtual std::shared_ptr<IResourceCompilerRegistry> registry() const = 0;
    };
}  // namespace workphone::resource

#endif  // IResourceSystem_h__
