#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Directors/TextDirector.hpp>
#include <Workphone/Interface/Database/IResourceDatabase.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, TextDirector, Director );

    // Definitions of static const key strings
    const String TextDirector::VerticalAlignmentStr = "VerticalAlignment";
    const String TextDirector::HorizontalAlignmentStr = "HorizontalAlignment";
    const String TextDirector::TextSizeStr = "TextSize";
    const String TextDirector::SaveButtonStr = "SaveButton";
    const String TextDirector::ImportButtonStr = "ImportButton";
    const String TextDirector::SaveStr = "Save";
    const String TextDirector::ImportStr = "Import";

    TextDirector::TextDirector() = default;

    TextDirector::~TextDirector() = default;

    auto TextDirector::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = UiElementDirector::getProperties();
        if( !properties )
        {
            return nullptr;
        }

        properties->setProperty( VerticalAlignmentStr, m_verticalAlignment );
        properties->setProperty( HorizontalAlignmentStr, m_horizontalAlignment );
        properties->setProperty( TextSizeStr, m_textSize );

        properties->setButtonPressed( SaveButtonStr );
        properties->setButtonPressed( ImportButtonStr );

        return properties;
    }

    void TextDirector::setProperties( SmartPtr<Properties> properties )
    {
        UiElementDirector::setProperties( properties );
        if( !properties )
        {
            return;
        }

        properties->getPropertyValue( VerticalAlignmentStr, m_verticalAlignment );
        properties->getPropertyValue( HorizontalAlignmentStr, m_horizontalAlignment );
        properties->getPropertyValue( TextSizeStr, m_textSize );

        if( properties->isButtonPressed( SaveButtonStr ) || properties->isButtonPressed( SaveStr ) )
        {
            save();
        }

        if( properties->isButtonPressed( ImportButtonStr ) || properties->isButtonPressed( ImportStr ) )
        {
            import();
        }
    }

    void TextDirector::setTextSize( u32 textSize )
    {
        m_textSize = textSize;
    }

    u32 TextDirector::getTextSize() const
    {
        return m_textSize;
    }

    void TextDirector::setHorizontalAlignment( u32 horizontalAlignment )
    {
        m_horizontalAlignment = horizontalAlignment;
    }

    u32 TextDirector::getHorizontalAlignment() const
    {
        return m_horizontalAlignment;
    }

    void TextDirector::setVerticalAlignment( u32 verticalAlignment )
    {
        m_verticalAlignment = verticalAlignment;
    }

    u32 TextDirector::getVerticalAlignment() const
    {
        return m_verticalAlignment;
    }

}  // namespace workphone::scene
