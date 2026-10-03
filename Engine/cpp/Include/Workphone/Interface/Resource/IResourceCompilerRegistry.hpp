#ifndef IResourceCompilerRegistry_h__
#define IResourceCompilerRegistry_h__

#include <Workphone/Interface/Resource/IResourceCompiler.hpp>

#include <memory>

namespace workphone::resource
{
    /** Abstract registry for locating compilers by resource type. */
    class WPCore_API IResourceCompilerRegistry
    {
    public:
        virtual ~IResourceCompilerRegistry() = default;

        virtual bool registerCompiler( std::shared_ptr<IResourceCompiler> compiler,
                                       String *error = nullptr ) = 0;
        virtual bool unregisterCompiler( const String &compilerName ) = 0;
        virtual void clear() = 0;

        virtual std::shared_ptr<const IResourceCompiler> getCompiler(
            const ResourceTypeID &type ) const = 0;
        virtual Array<std::shared_ptr<const IResourceCompiler>> getCompilers() const = 0;
        virtual bool hasCompiler( const ResourceTypeID &type ) const = 0;
    };
}  // namespace workphone::resource

#endif  // IResourceCompilerRegistry_h__
