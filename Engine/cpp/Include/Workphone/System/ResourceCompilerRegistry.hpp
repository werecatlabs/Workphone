#ifndef WPResourceCompilerRegistry_h__
#define WPResourceCompilerRegistry_h__

#include <Workphone/Interface/Resource/IResourceCompilerRegistry.hpp>

#include <map>
#include <memory>

namespace workphone::resource
{
    /** Thread-safe registry that enforces one compiler per resource type. */
    class WPCore_API ResourceCompilerRegistry final : public IResourceCompilerRegistry
    {
    public:
        ResourceCompilerRegistry();
        ~ResourceCompilerRegistry() override;

        ResourceCompilerRegistry( const ResourceCompilerRegistry & ) = delete;
        ResourceCompilerRegistry &operator=( const ResourceCompilerRegistry & ) = delete;

        bool registerCompiler( std::shared_ptr<IResourceCompiler> compiler,
                               String *error = nullptr ) override;
        bool unregisterCompiler( const String &compilerName ) override;
        void clear() override;

        std::shared_ptr<const IResourceCompiler> getCompiler(
            const ResourceTypeID &type ) const override;
        Array<std::shared_ptr<const IResourceCompiler>> getCompilers() const override;
        bool hasCompiler( const ResourceTypeID &type ) const override;

    private:
        class Impl;
        std::shared_ptr<Impl> m_impl;
    };
}  // namespace workphone::resource

#endif  // WPResourceCompilerRegistry_h__
