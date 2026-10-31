#ifndef IResourceCompiler_h__
#define IResourceCompiler_h__

#include <Workphone/System/ResourceID.hpp>
#include <Workphone/System/ResourceBuildProfile.hpp>
#include <Workphone/Core/Array.hpp>

#include <ostream>
#include <map>
#include <functional>
#include <memory>

namespace workphone::resource
{
    enum class CompilationStatus
    {
        Failure = -1,
        UpToDate = 0,
        Success = 1,
        SuccessWithWarnings = 2
    };

    struct WPCore_API CompilerOutput
    {
        ResourceTypeID type;
        u64 version = 0;
    };

    struct WPCore_API CompileDependency
    {
        String path;
        bool isResource = false;
        bool optional = false;

        static CompileDependency resource( const ResourceID &resourceId );
        static CompileDependency data( const String &dataPath );
    };

    struct WPCore_API DependencySet
    {
        Array<CompileDependency> compileDependencies;
        Array<ResourceID> installDependencies;
    };

    struct WPCore_API CompileContext
    {
        ResourceID resourceId;
        String sourcePath;
        String outputPath;
        String sourceRoot;
        String compiledRoot;
        String target;
        bool packagedBuild = false;
        /** Pinned absolute artifacts of successfully compiled dependencies. */
        std::map<String, String> dependencyArtifacts;
        /** During dependency scanning, capture auxiliary data:// inputs before
         * reading them. Optional absent inputs return an empty snapshot path and
         * their absence is checked before publication. Empty for standalone tools.
         * Compile receives a snapshot sourcePath/sourceRoot and no callback.
         */
        std::function<bool( const String &, bool, String &, String & )> snapshotInput;
        ResourceBuildProfile buildProfile;
        /** Detached canonical authored UUID -> exact ResourceID mapping captured
         * for this build. Compilers may not infer paths or consult a catalog. */
        std::shared_ptr<const std::map<String,ResourceID>> assetBindings;
    };

    /** Base interface implemented by format-specific resource compilers. */
    class WPCore_API IResourceCompiler
    {
    public:
        virtual ~IResourceCompiler() = default;

        virtual String name() const = 0;
        virtual Array<CompilerOutput> outputs() const = 0;

        virtual bool getDependencies( const CompileContext &context, DependencySet &dependencies,
                                      String &error ) const;

        /** Write only the compiled payload; the resource system owns the file header. */
        virtual CompilationStatus compile( const CompileContext &context, std::ostream &output,
                                           Array<String> &messages ) const = 0;

        u64 versionFor( const ResourceTypeID &type ) const;
    };
}  // namespace workphone::resource

#endif  // IResourceCompiler_h__
