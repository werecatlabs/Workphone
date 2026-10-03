#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Interface/Resource/IResourceCompiler.hpp>

namespace workphone::resource
{
    CompileDependency CompileDependency::resource( const ResourceID &resourceId )
    {
        CompileDependency dependency;
        dependency.path = resourceId.str();
        dependency.isResource = true;
        return dependency;
    }

    CompileDependency CompileDependency::data( const String &dataPath )
    {
        CompileDependency dependency;
        dependency.path = dataPath;
        dependency.isResource = false;
        return dependency;
    }

    bool IResourceCompiler::getDependencies( const CompileContext &, DependencySet &, String & ) const
    {
        return true;
    }

    u64 IResourceCompiler::versionFor( const ResourceTypeID &type ) const
    {
        // Include the compiled container version so format changes invalidate all outputs.
        constexpr u64 containerVersion = 1;
        for( const auto &output : outputs() )
        {
            if( output.type == type && output.version != 0 )
            {
                u64 version = combineHash( 14695981039346656037ull, containerVersion );
                version = combineHash( version, output.version );
                return combineHash( version, type.value() );
            }
        }
        return 0;
    }
}  // namespace workphone::resource
