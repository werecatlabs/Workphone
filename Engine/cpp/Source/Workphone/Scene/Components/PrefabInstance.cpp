#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/PrefabInstance.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, PrefabInstance, Component );

    void PrefabInstance::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        Component::load( data );
        setLoadingState( LoadingState::Loaded );
    }
    void PrefabInstance::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );
        Component::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }
    SmartPtr<Properties> PrefabInstance::getProperties() const
    {
        auto result = Component::getProperties();
        result->setProperty( "sourcePath", m_sourcePath );
        result->setProperty( "sourceAssetUuid", m_sourceAssetUuid );
        if( m_identities )
            for( auto entry : m_identities->getChildren() )
            {
                auto copy = make_ptr<Properties>();
                copy->setName( "identity" );
                for( auto property : entry->getPropertiesAsArray() ) copy->setProperty( property );
                result->addChild( copy );
            }
        return result;
    }
    void PrefabInstance::setProperties( SmartPtr<Properties> properties )
    {
        Component::setProperties( properties );
        if( !properties ) return;
        properties->getPropertyValue( "sourcePath", m_sourcePath );
        properties->getPropertyValue( "sourceAssetUuid", m_sourceAssetUuid );
        if( properties->hasProperty( "sourcePath" ) )
        {
            m_identities = make_ptr<Properties>();
            for( auto entry : properties->getChildrenByName( "identity" ) )
            {
                auto copy = make_ptr<Properties>();
                copy->setName( "identity" );
                for( auto property : entry->getPropertiesAsArray() ) copy->setProperty( property );
                m_identities->addChild( copy );
            }
        }
    }
    String PrefabInstance::getSourcePath() const { return m_sourcePath; }
    String PrefabInstance::getSourceAssetUuid() const { return m_sourceAssetUuid; }
    SmartPtr<Properties> PrefabInstance::getIdentityMap() const
    {
        auto result = make_ptr<Properties>();
        for( auto entry : getProperties()->getChildrenByName( "identity" ) ) result->addChild( entry );
        return result;
    }
}
