#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/UI/ClawUITextEntry.hpp>
#include <Workphone/Workphone.hpp>
#include <WorkphoneCore/workphone.h>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone, ClawUITextEntry, ClawUIElement<IUITextEntry> );

    ClawUITextEntry::ClawUITextEntry() : m_nextCursorFlash( 0 )
    {
        setType( "TextEntry" );
    }

    ClawUITextEntry::~ClawUITextEntry() = default;

    bool ClawUITextEntry::handleEvent( const SmartPtr<IInputEvent> &event )
    {
        return false;
    }

    void ClawUITextEntry::update()
    {
        if( auto applicationManager = core::IApplicationManager::instance() )
        {
            if( auto timer = applicationManager->getTimer() )
            {
                m_nextCursorFlash = timer->getTimeMilliseconds() + 1000;
            }
        }
        ClawUIElement::update();
    }

    void ClawUITextEntry::setText( const String &text )
    {
        m_text = text;
    }

    String ClawUITextEntry::getText() const
    {
        return m_text;
    }

    void ClawUITextEntry::setTextSize( f32 textSize )
    {
        m_textSize = MathF::max( textSize, 0.0f );
    }

    f32 ClawUITextEntry::getTextSize() const
    {
        return m_textSize;
    }

    void ClawUITextEntry::setVerticalAlignment( u8 alignment )
    {
        m_verticalAlignment = alignment;
    }

    u8 ClawUITextEntry::getVerticalAlignment() const
    {
        return m_verticalAlignment;
    }

    void ClawUITextEntry::setHorizontalAlignment( u8 alignment )
    {
        m_horizontalAlignment = alignment;
    }

    u8 ClawUITextEntry::getHorizontalAlignment() const
    {
        return m_horizontalAlignment;
    }

    void ClawUITextEntry::setPlaceholder( const String &placeholder )
    {
        m_placeholder = placeholder;
    }

    String ClawUITextEntry::getPlaceholder() const
    {
        return m_placeholder;
    }

    void ClawUITextEntry::setReadOnly( bool readOnly )
    {
        m_readOnly = readOnly;
    }

    bool ClawUITextEntry::isReadOnly() const
    {
        return m_readOnly;
    }

    void ClawUITextEntry::setSecureEntry( bool secureEntry )
    {
        m_secureEntry = secureEntry;
        if( secureEntry )
        {
            m_inputType = InputType::Password;
        }
    }

    bool ClawUITextEntry::isSecureEntry() const
    {
        return m_secureEntry;
    }

    void ClawUITextEntry::setMultiline( bool multiline )
    {
        m_multiline = multiline;
        if( multiline )
        {
            m_inputType = InputType::Multiline;
        }
    }

    bool ClawUITextEntry::isMultiline() const
    {
        return m_multiline;
    }

    void ClawUITextEntry::setInputType( InputType inputType, const String &textHint )
    {
        m_inputType = inputType;
        m_textHint = textHint;
        m_multiline = inputType == InputType::Multiline;
        m_secureEntry = inputType == InputType::Password;
    }

    IUITextEntry::InputType ClawUITextEntry::getInputType() const
    {
        return m_inputType;
    }

    String ClawUITextEntry::getTextHint() const
    {
        return m_textHint;
    }

    void ClawUITextEntry::setPosition( const Vector2<real_Num> &position )
    {
        ClawUIElement::setPosition( position );
    }

    void ClawUITextEntry::setSize( const Vector2<real_Num> &size )
    {
        ClawUIElement::setSize( size );
    }

    void ClawUITextEntry::draw( struct wp_context *ctx )
    {
        if( !beginWorkphoneWidget( ctx ) )
        {
            return;
        }

        constexpr size_t maxTextLength = 4096;
        char buffer[maxTextLength] = {};
        const auto length = std::min( m_text.size(), maxTextLength - 1 );
        memcpy( buffer, m_text.c_str(), length );

        wp_flags flags = m_multiline ? WORKPHONE_EDIT_BOX : WORKPHONE_EDIT_FIELD;
        if( m_readOnly )
        {
            flags |= WORKPHONE_EDIT_READ_ONLY;
        }

        wp_plugin_filter filter = wp_filter_default;
        if( m_inputType == InputType::Email )
        {
            filter = wp_filter_ascii;
        }

        wp_edit_string_zero_terminated( ctx, flags, buffer, static_cast<wp_s32>( maxTextLength ),
                                        filter );
        if( !m_readOnly && m_text != buffer )
        {
            m_text = buffer;
        }

        drawWorkphoneChildren( ctx );
    }
}  // namespace workphone::ui
