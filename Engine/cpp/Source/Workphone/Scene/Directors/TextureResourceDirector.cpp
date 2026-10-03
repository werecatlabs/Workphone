#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Directors/TextureResourceDirector.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, TextureResourceDirector, ResourceDirector );

    TextureResourceDirector::TextureResourceDirector() = default;

    TextureResourceDirector::~TextureResourceDirector() = default;

    auto TextureResourceDirector::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = ResourceDirector::getProperties();

        auto textureTypes = Array<String>();
        textureTypes.reserve( 24 );

        textureTypes.emplace_back( "Texture" );
        textureTypes.emplace_back( "Normal Map" );
        textureTypes.emplace_back( "Sprite" );
        textureTypes.emplace_back( "UI" );

        auto textureSizes = Array<String>();
        textureSizes.reserve( 12 );

        textureSizes.emplace_back( "32" );
        textureSizes.emplace_back( "64" );
        textureSizes.emplace_back( "128" );
        textureSizes.emplace_back( "256" );
        textureSizes.emplace_back( "512" );
        textureSizes.emplace_back( "1024" );
        textureSizes.emplace_back( "2048" );
        textureSizes.emplace_back( "4096" );
        textureSizes.emplace_back( "8192" );

        properties->setPropertyAsEnum( "type", m_textureType, textureTypes );
        properties->setPropertyAsEnum( "size", m_textureSize, textureSizes );
        properties->setProperty( "useTiling", m_useTiling );

        properties->setProperty( "borderLeft", m_borderLeft );
        properties->setProperty( "borderRight", m_borderRight );
        properties->setProperty( "borderTop", m_borderTop );
        properties->setProperty( "borderBottom", m_borderBottom );

        properties->setButtonPressed( "Save" );
        properties->setButtonPressed( "Import" );

        return properties;
    }

    void TextureResourceDirector::setProperties( SmartPtr<Properties> properties )
    {
        ResourceDirector::setProperties( properties );

        properties->getPropertyValue( "type", m_textureType );
        properties->getPropertyValue( "size", m_textureSize );
        properties->getPropertyValue( "useTiling", m_useTiling );

        properties->getPropertyValue( "borderLeft", m_borderLeft );
        properties->getPropertyValue( "borderRight", m_borderRight );
        properties->getPropertyValue( "borderTop", m_borderTop );
        properties->getPropertyValue( "borderBottom", m_borderBottom );

        if( properties->isButtonPressed( "Save" ) )
        {
            save();
        }

        if( properties->isButtonPressed( "Import" ) )
        {
            import();
        }
    }

    void TextureResourceDirector::setUseTiling( bool useTiling )
    {
        m_useTiling = useTiling;
    }

    bool TextureResourceDirector::getUseTiling() const
    {
        return m_useTiling;
    }

    void TextureResourceDirector::setBorderBottom( s32 borderBottom )
    {
        m_borderBottom = borderBottom;
    }

    s32 TextureResourceDirector::getBorderBottom() const
    {
        return m_borderBottom;
    }

    void TextureResourceDirector::setBorderTop( s32 borderTop )
    {
        m_borderTop = borderTop;
    }

    s32 TextureResourceDirector::getBorderTop() const
    {
        return m_borderTop;
    }

    void TextureResourceDirector::setBorderRight( s32 borderRight )
    {
        m_borderRight = borderRight;
    }

    s32 TextureResourceDirector::getBorderRight() const
    {
        return m_borderRight;
    }

    void TextureResourceDirector::setBorderLeft( s32 borderLeft )
    {
        m_borderLeft = borderLeft;
    }

    s32 TextureResourceDirector::getBorderLeft() const
    {
        return m_borderLeft;
    }

}  // namespace workphone::scene
