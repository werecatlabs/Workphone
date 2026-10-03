#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Directors/MaterialResourceDirector.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, MaterialResourceDirector, ResourceDirector );

    const String MaterialResourceDirector::textureTypeStr = String( "textureType" );
    const String MaterialResourceDirector::textureSizeStr = String( "textureSize" );

    MaterialResourceDirector::MaterialResourceDirector() = default;

    MaterialResourceDirector::~MaterialResourceDirector() = default;

    auto MaterialResourceDirector::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = ResourceDirector::getProperties();
        if( !properties )
        {
            return nullptr;
        }

        properties->setProperty( textureTypeStr, getTextureType() );
        properties->setProperty( textureSizeStr, getTextureSize() );

        return properties;
    }

    void MaterialResourceDirector::setProperties( SmartPtr<Properties> properties )
    {
        ResourceDirector::setProperties( properties );

        if( !properties )
        {
            return;
        }

        auto textureType = getTextureType();
        auto textureSize = getTextureSize();
        properties->getPropertyValue( textureTypeStr, textureType );
        properties->getPropertyValue( textureSizeStr, textureSize );
        setTextureType( textureType );
        setTextureSize( textureSize );
    }

    String MaterialResourceDirector::getTextureType() const
    {
        return m_textureType;
    }

    void MaterialResourceDirector::setTextureType( const String &textureType )
    {
        m_textureType = textureType;
    }

    s32 MaterialResourceDirector::getTextureSize() const
    {
        return m_textureSize;
    }

    void MaterialResourceDirector::setTextureSize( s32 textureSize )
    {
        m_textureSize = textureSize;
    }
}  // namespace workphone::scene
