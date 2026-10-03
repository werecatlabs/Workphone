#ifndef IResourceCompiler_h__
#define IResourceCompiler_h__

#include <Workphone/System/ResourceID.hpp>
#include <Workphone/Core/Array.hpp>

#include <ostream>

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
