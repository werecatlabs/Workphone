#ifndef WPResourceSystem_h__
#define WPResourceSystem_h__

#include <Workphone/Interface/Database/IResourceCompilationDatabase.hpp>
#include <Workphone/Interface/Resource/IResourceSystem.hpp>

namespace workphone::resource
{
    /**
     * Dependency-aware build and runtime resource service.
     *
     * Compilation is serialized per ResourceSystem instance, while registry,
     * metadata queries, and runtime cache reads are thread-safe. Compilers can be
     * registered dynamically before or after initialization.
     */
    class WPCore_API ResourceSystem final : public IResourceSystem
    {
    public:
        ResourceSystem( std::shared_ptr<IResourceCompilerRegistry> registry,
                        std::shared_ptr<IResourceCompilationDatabase> database );
        ~ResourceSystem() override;

        ResourceSystem( const ResourceSystem & ) = delete;
        ResourceSystem &operator=( const ResourceSystem & ) = delete;

        bool initialize( const ResourceSystemConfig &config, String &error ) override;
        void shutdown() override;
        bool isInitialized() const override;

        CompilationReport compile( const ResourceID &resourceId,
                                   const CompilationOptions &options = {} ) override;

        /** Recompile all transitive dependents of a changed data:// path. */
        Array<CompilationReport> compileAffected( const String &changedPath,
                                                  const CompilationOptions &options = {} ) override;

        std::shared_ptr<const RuntimeResource> load( const ResourceID &resourceId,
                                                     String &error ) override;
        void unload( const ResourceID &resourceId ) override;
        void clearRuntimeCache() override;

        std::shared_ptr<IResourceCompilerRegistry> registry() const override;

    private:
        class Impl;
        Impl *m_impl = nullptr;
    };
}  // namespace workphone::resource

#endif  // WPResourceSystem_h__
