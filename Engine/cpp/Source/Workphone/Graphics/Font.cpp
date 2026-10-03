#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/Font.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>
#include <Workphone/Core/Properties.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, Font, IFont );

    Font::Font() = default;

    Font::Font( u32 poolTypeId ) : ResourceGraphics<IFont>( poolTypeId )
    {
    }

    Font::~Font() = default;

    void Font::load( SmartPtr<ISharedObject> data )
    {
        // Set the loading state to Loading
        this->setLoadingState( LoadingState::Loading );
        // TODO: Add font loading logic here (e.g., load font from font_source)
        // For now, just set the state to Loaded
        this->setLoadingState( LoadingState::Loaded );
    }

    void Font::unload( SmartPtr<ISharedObject> data )
    {
        const auto &state = this->getLoadingState();
        if( state != LoadingState::Unloaded )
        {
            this->setLoadingState( LoadingState::Unloading );
            ResourceGraphics<IFont>::unload( data );
            this->setLoadingState( LoadingState::Unloaded );
        }
    }

    SmartPtr<Properties> Font::getProperties() const
    {
        auto properties = ResourceGraphics<IFont>::getProperties();
        properties->setProperty( IFont::fontTypeStr, m_fontType );
        properties->setProperty( IFont::fontSourceStr, m_fontSource );
        properties->setProperty( IFont::fontSizeStr, m_fontSize );
        properties->setProperty( IFont::fontResolutionStr, m_fontResolution );
        return properties;
    }

    void Font::setProperties( SmartPtr<Properties> properties )
    {
        ResourceGraphics<IFont>::setProperties( properties );
        properties->getPropertyValue( IFont::fontTypeStr, m_fontType );
        properties->getPropertyValue( IFont::fontSourceStr, m_fontSource );
        properties->getPropertyValue( IFont::fontSizeStr, m_fontSize );
        properties->getPropertyValue( IFont::fontResolutionStr, m_fontResolution );
    }

    String Font::getFontType() const
    {
        return m_fontType;
    }

    void Font::setFontType( const String &type )
    {
        m_fontType = type;
    }

    String Font::getFontSource() const
    {
        return m_fontSource;
    }

    void Font::setFontSource( const String &source )
    {
        m_fontSource = source;
    }

    u32 Font::getFontSize() const
    {
        return m_fontSize;
    }

    void Font::setFontSize( u32 size )
    {
        m_fontSize = size;
    }

    u32 Font::getFontResolution() const
    {
        return m_fontResolution;
    }

    void Font::setFontResolution( u32 resolution )
    {
        m_fontResolution = resolution;
    }

}  // namespace workphone::render
