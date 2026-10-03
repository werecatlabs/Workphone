#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/ResourceCompilerRegistry.hpp>

#include <mutex>
#include <set>
#include <shared_mutex>

namespace workphone::resource
{
    class ResourceCompilerRegistry::Impl
    {
    public:
        mutable std::shared_mutex mutex;
        std::map<String, std::shared_ptr<IResourceCompiler>> compilersByName;
        std::map<String, std::shared_ptr<IResourceCompiler>> compilersByType;
    };

    ResourceCompilerRegistry::ResourceCompilerRegistry() : m_impl( std::make_shared<Impl>() )
    {
    }

    ResourceCompilerRegistry::~ResourceCompilerRegistry() = default;

    bool ResourceCompilerRegistry::registerCompiler( std::shared_ptr<IResourceCompiler> compiler,
                                                     String *error )
    {
        if( !compiler )
        {
            if( error )
                *error = "Cannot register a null resource compiler";
            return false;
        }

        const String compilerName = compiler->name();
        const auto compilerOutputs = compiler->outputs();
        if( compilerName.empty() || compilerOutputs.empty() )
        {
            if( error )
                *error = "A resource compiler must have a name and at least one output";
            return false;
        }

        std::set<String> outputTypes;
        for( const auto &output : compilerOutputs )
        {
            if( !output.type.isValid() || output.version == 0 ||
                !outputTypes.insert( output.type.str() ).second )
            {
                if( error )
                    *error = "Compiler outputs must contain unique valid types and non-zero versions";
                return false;
            }
        }

        std::unique_lock<std::shared_mutex> lock( m_impl->mutex );
        if( m_impl->compilersByName.find( compilerName ) != m_impl->compilersByName.end() )
        {
            if( error )
                *error =
                    String( "A resource compiler named '" ) + compilerName + "' is already registered";
            return false;
        }
        for( const auto &output : compilerOutputs )
        {
            if( m_impl->compilersByType.find( output.type.str() ) != m_impl->compilersByType.end() )
            {
                if( error )
                    *error = String( "A compiler is already registered for resource type '" ) +
                             output.type.str() + "'";
                return false;
            }
        }

        m_impl->compilersByName.emplace( compilerName, compiler );
        for( const auto &output : compilerOutputs )
        {
            m_impl->compilersByType.emplace( output.type.str(), compiler );
        }
        return true;
    }

    bool ResourceCompilerRegistry::unregisterCompiler( const String &compilerName )
    {
        std::unique_lock<std::shared_mutex> lock( m_impl->mutex );
        const auto iterator = m_impl->compilersByName.find( compilerName );
        if( iterator == m_impl->compilersByName.end() )
            return false;

        const auto compiler = iterator->second;
        for( auto typeIterator = m_impl->compilersByType.begin();
             typeIterator != m_impl->compilersByType.end(); )
        {
            if( typeIterator->second == compiler )
                typeIterator = m_impl->compilersByType.erase( typeIterator );
            else
                ++typeIterator;
        }
        m_impl->compilersByName.erase( iterator );
        return true;
    }

    void ResourceCompilerRegistry::clear()
    {
        std::unique_lock<std::shared_mutex> lock( m_impl->mutex );
        m_impl->compilersByType.clear();
        m_impl->compilersByName.clear();
    }

    std::shared_ptr<const IResourceCompiler> ResourceCompilerRegistry::getCompiler(
        const ResourceTypeID &type ) const
    {
        std::shared_lock<std::shared_mutex> lock( m_impl->mutex );
        const auto iterator = m_impl->compilersByType.find( type.str() );
        return iterator == m_impl->compilersByType.end() ? nullptr : iterator->second;
    }

    Array<std::shared_ptr<const IResourceCompiler>> ResourceCompilerRegistry::getCompilers() const
    {
        std::shared_lock<std::shared_mutex> lock( m_impl->mutex );
        Array<std::shared_ptr<const IResourceCompiler>> result;
        result.reserve( m_impl->compilersByName.size() );
        for( const auto &entry : m_impl->compilersByName )
            result.push_back( entry.second );
        return result;
    }

    bool ResourceCompilerRegistry::hasCompiler( const ResourceTypeID &type ) const
    {
        return getCompiler( type ) != nullptr;
    }
}  // namespace workphone::resource
