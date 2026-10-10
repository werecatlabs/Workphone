#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Script/ScriptAsset.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, ScriptAsset, Resource<IResource> );

    void ScriptAsset::loadFromFile( const String &path )
    {
        setFilePath( path );
        setLoadingState( LoadingState::Loaded );
    }

    void ScriptAsset::unload( SmartPtr<ISharedObject> data )
    {
        Resource<IResource>::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    SmartPtr<Properties> ScriptAsset::getProperties() const
    {
        auto properties = Resource<IResource>::getProperties();
        properties->setProperty( "asset_type", "script" );
        properties->setProperty( "language", "lua" );
        properties->setProperty( "sourcePath", getFilePath() );
        return properties;
    }
}
