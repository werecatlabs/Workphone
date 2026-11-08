#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/ClawDebug.hpp>
#include <Workphone/Graphics/DebugLine.hpp>
#include <Workphone/Interface/Graphics/IRenderer3.hpp>
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
        m_lines.clear();

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

    SmartPtr<IDebugLine> ClawDebug::drawLine( hash_type id, const Vector3<real_Num> &start,
                                              const Vector3<real_Num> &end, u32 colour )
    {
        if( !start.isFinite() || !end.isFinite() )
        {
            return nullptr;
        }

        std::lock_guard<RecursiveMutex> lock( m_mutex );
        auto &entry = m_lines[id];
        if( !entry.line )
        {
            entry.line = workphone::make_ptr<DebugLine>();
            // Keep a line visible across render frames until the next physics update.
            entry.line->setMaxLifeTime( 0.15 );
        }
        entry.refreshed = std::chrono::steady_clock::now();
        entry.line->setPosition( start );
        entry.line->setVector( end - start );
        entry.line->setColour( colour );
        entry.line->setLifeTime( 0.0 );
        entry.line->setVisible( true );
        return entry.line;
    }

    void ClawDebug::renderLines( IRenderer3 &renderer )
    {
        struct Line
        {
            Vector3<real_Num> start;
            Vector3<real_Num> end;
            u32 colour;
        };

        Array<Line> lines;
        {
            std::lock_guard<RecursiveMutex> lock( m_mutex );
            const auto now = std::chrono::steady_clock::now();
            lines.reserve( m_lines.size() );
            for( auto it = m_lines.begin(); it != m_lines.end(); )
            {
                auto &line = it->second.line;
                const auto age = std::chrono::duration<f64>( now - it->second.refreshed ).count();
                line->setLifeTime( age );
                if( const auto lifetime = line->getMaxLifeTime(); lifetime > 0.0 && age >= lifetime )
                {
                    line->setVisible( false );
                    it = m_lines.erase( it );
                    continue;
                }
                if( line->isVisible() )
                {
                    const auto start = line->getPosition();
                    lines.push_back( { start, start + line->getVector(), line->getColour() } );
                }
                ++it;
            }
        }

        constexpr f32 byteToFloat = 1.0f / 255.0f;
        for( const auto &line : lines )
        {
            const auto colour = line.colour;
            renderer.drawLine( line.start, line.end,
                ColourF( ( ( colour >> 24 ) & 0xffu ) * byteToFloat,
                         ( ( colour >> 16 ) & 0xffu ) * byteToFloat,
                         ( ( colour >> 8 ) & 0xffu ) * byteToFloat,
                         ( colour & 0xffu ) * byteToFloat ) );
        }
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
