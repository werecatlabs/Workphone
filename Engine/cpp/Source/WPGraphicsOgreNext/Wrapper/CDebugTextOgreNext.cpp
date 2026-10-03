#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CDebugTextOgreNext.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, CDebugTextOgreNext, IDebugText );

    CDebugTextOgreNext::CDebugTextOgreNext() = default;

    CDebugTextOgreNext::~CDebugTextOgreNext()
    {
        unload( nullptr );
    }

    void CDebugTextOgreNext::load( SmartPtr<ISharedObject> data )
    {
    }

    void CDebugTextOgreNext::unload( SmartPtr<ISharedObject> data )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        auto overlayManager = graphicsSystem->getOverlayManager();

        if( m_textElement )
        {
            overlayManager->removeElement( m_textElement );
            m_textElement = nullptr;
        }
    }

    auto CDebugTextOgreNext::getText() const -> String
    {
        return m_text;
    }

    void CDebugTextOgreNext::setText( const String &text )
    {
        m_text = text;
    }

    auto CDebugTextOgreNext::getTextElement() const -> SmartPtr<IOverlayElementText>
    {
        return m_textElement;
    }

    void CDebugTextOgreNext::setTextElement( SmartPtr<IOverlayElementText> textElement )
    {
        m_textElement = textElement;
    }
}  // namespace workphone::render
