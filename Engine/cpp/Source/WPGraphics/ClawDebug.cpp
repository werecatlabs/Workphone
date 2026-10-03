#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/ClawDebug.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::render
{
    ClawDebug::~ClawDebug()
    {
        clear();
    }

    void ClawDebug::unload( SmartPtr<ISharedObject> data )
    {
        clear();
        Debug::unload( data );
    }

    void ClawDebug::clear()
    {
        std::lock_guard<RecursiveMutex> lock( m_mutex );

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto renderUI = applicationManager ? applicationManager->getRenderUI() : nullptr;
        for( auto &[id, element] : m_textElements )
        {
            if( !element )
            {
                continue;
            }

            if( renderUI )
            {
                renderUI->removeElement( element );
            }
            else if( element->isLoaded() )
            {
                element->unload( nullptr );
            }
        }
        m_textElements.clear();

        Debug::clear();
    }

    void ClawDebug::drawText( hash_type id, const Vector2<real_Num> &position, const String &text,
                              u32 color )
    {
        std::lock_guard<RecursiveMutex> lock( m_mutex );

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto renderUI = applicationManager ? applicationManager->getRenderUI() : nullptr;
        if( !renderUI )
        {
            WP_LOG_WARNING( "ClawDebug::drawText: render UI is not available." );
            return;
        }

        auto &element = m_textElements[id];
        if( !element )
        {
            element = renderUI->addElementByType<ui::IUIText>();
            if( !element )
            {
                WP_LOG_ERROR( "ClawDebug::drawText: failed to create a text element." );
                m_textElements.erase( id );
                return;
            }

            element->setSize( Vector2F( 0.45f, 0.06f ) );
            element->setHorizontalAlignment( 0 );
            element->setVerticalAlignment( 0 );
        }

        element->setPosition( Vector2F( position.X(), position.Y() ) );
        element->setText( text );
        element->setVisible( !text.empty() );

        // Debug callers historically pass zero for the default colour.
        ColourF colour = ColourF::White;
        if( color != 0 )
        {
            constexpr f32 byteToFloat = 1.0f / 255.0f;
            colour = ColourF( static_cast<f32>( ( color >> 24 ) & 0xffu ) * byteToFloat,
                              static_cast<f32>( ( color >> 16 ) & 0xffu ) * byteToFloat,
                              static_cast<f32>( ( color >> 8 ) & 0xffu ) * byteToFloat,
                              static_cast<f32>( color & 0xffu ) * byteToFloat );
        }
        element->setColour( colour );
    }
}  // namespace workphone::render
