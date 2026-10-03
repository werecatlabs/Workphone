#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/DebugText.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Interface/Graphics/IOverlayElementText.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, DebugText, IDebugText );

    DebugText::DebugText() : m_text(), m_textElement( nullptr )
    {
    }

    DebugText::~DebugText() = default;

    void DebugText::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            ISharedObject::load( data );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void DebugText::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( m_textElement )
            {
                m_textElement->unload( data );
                m_textElement = nullptr;
            }

            ISharedObject::unload( data );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    String DebugText::getText() const
    {
        return m_text;
    }

    void DebugText::setText( const String &text )
    {
        m_text = text;

        if( m_textElement )
        {
            //m_textElement->setText( text );
        }
    }

    SmartPtr<IOverlayElementText> DebugText::getTextElement() const
    {
        return m_textElement;
    }

    void DebugText::setTextElement( SmartPtr<IOverlayElementText> textElement )
    {
        m_textElement = textElement;

        if( m_textElement && !m_text.empty() )
        {
            //m_textElement->setText( m_text );
        }
    }

}  // namespace workphone::render
