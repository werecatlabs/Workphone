#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Directors/ButtonDirector.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, ButtonDirector, UiElementDirector );

    const String ButtonDirector::textSizeStr = String( "textSize" );
    const String ButtonDirector::normalColourStr = String( "normalColour" );
    const String ButtonDirector::highlightedColourStr = String( "highlightedColour" );
    const String ButtonDirector::pressedColourStr = String( "pressedColour" );
    const String ButtonDirector::disabledColourStr = String( "disabledColour" );

    ButtonDirector::ButtonDirector() = default;

    ButtonDirector::~ButtonDirector() = default;

    SmartPtr<Properties> ButtonDirector::getProperties() const
    {
        auto properties = UiElementDirector::getProperties();
        if( !properties )
        {
            return nullptr;
        }

        properties->setProperty( textSizeStr, m_textSize );
        properties->setProperty( normalColourStr, m_normalColour );
        properties->setProperty( highlightedColourStr, m_highlightedColour );
        properties->setProperty( pressedColourStr, m_pressedColour );
        properties->setProperty( disabledColourStr, m_disabledColour );

        return properties;
    }

    void ButtonDirector::setProperties( SmartPtr<Properties> properties )
    {
        UiElementDirector::setProperties( properties );
        if( !properties )
        {
            return;
        }

        properties->getPropertyValue( textSizeStr, m_textSize );
        properties->getPropertyValue( normalColourStr, m_normalColour );
        properties->getPropertyValue( highlightedColourStr, m_highlightedColour );
        properties->getPropertyValue( pressedColourStr, m_pressedColour );
        properties->getPropertyValue( disabledColourStr, m_disabledColour );
    }

    void ButtonDirector::setDisabledColour( const ColourF &disabledColour )
    {
        m_disabledColour = disabledColour;
    }

    ColourF ButtonDirector::getDisabledColour() const
    {
        return m_disabledColour;
    }

    void ButtonDirector::setPressedColour( const ColourF &pressedColour )
    {
        m_pressedColour = pressedColour;
    }

    ColourF ButtonDirector::getPressedColour() const
    {
        return m_pressedColour;
    }

    void ButtonDirector::setHighlightedColour( const ColourF &highlightedColour )
    {
        m_highlightedColour = highlightedColour;
    }

    ColourF ButtonDirector::getHighlightedColour() const
    {
        return m_highlightedColour;
    }

    void ButtonDirector::setNormalColour( const ColourF &normalColour )
    {
        m_normalColour = normalColour;
    }

    ColourF ButtonDirector::getNormalColour() const
    {
        return m_normalColour;
    }

    void ButtonDirector::setTextSize( u32 textSize )
    {
        m_textSize = textSize;
    }

    u32 ButtonDirector::getTextSize() const
    {
        return m_textSize;
    }
}  // namespace workphone::scene
