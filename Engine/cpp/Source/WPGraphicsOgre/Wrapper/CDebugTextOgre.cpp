#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CDebugTextOgre.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, CDebugTextOgre, IDebugText );

    CDebugTextOgre::CDebugTextOgre() = default;

    CDebugTextOgre::~CDebugTextOgre()
    {
        unload( nullptr );
    }

    void CDebugTextOgre::load( SmartPtr<ISharedObject> data )
    {
    }

    void CDebugTextOgre::unload( SmartPtr<ISharedObject> data )
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

    auto CDebugTextOgre::getText() const -> String
    {
        return m_text;
    }

    void CDebugTextOgre::setText( const String &text )
    {
        m_text = text;
    }

    auto CDebugTextOgre::getTextElement() const -> SmartPtr<IOverlayElementText>
    {
        return m_textElement;
    }

    void CDebugTextOgre::setTextElement( SmartPtr<IOverlayElementText> textElement )
    {
        m_textElement = textElement;
    }
}  // namespace workphone::render
