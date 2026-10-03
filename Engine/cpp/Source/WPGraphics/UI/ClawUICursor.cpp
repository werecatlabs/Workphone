#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/UI/ClawUICursor.hpp>
#include <WPGraphics/UI/ClawUIWorkphoneContext.hpp>
#include <Workphone/Workphone.hpp>
#include <WorkphoneCore/workphone.h>

namespace workphone::ui
{
    ClawUICursor::ClawUICursor()
    {
        setType( "Cursor" );
        setSize( Vector2F( 0.02f, 0.03f ) );
    }

    ClawUICursor::~ClawUICursor() = default;

    void ClawUICursor::initialise()
    {
        setName( "ClawUICursor" );
    }

    bool ClawUICursor::handleEvent( const SmartPtr<IInputEvent> &event )
    {
        if( event && event->getEventType() == IInputEvent::EventType::Mouse )
        {
            if( auto mouseState = event->getMouseState() )
            {
                auto applicationManager = core::IApplicationManager::instance();
                if( applicationManager )
                {
                    if( auto window = applicationManager->getWindow() )
                    {
                        const auto windowSize = window->getSize();
                        const auto mousePosition = mouseState->getAbsolutePosition();
                        if( windowSize.x > 0 && windowSize.y > 0 )
                        {
                            setPosition( Vector2F( mousePosition.X() / windowSize.x,
                                                   mousePosition.Y() / windowSize.y ) );
                        }
                    }
                }
            }
        }

        return false;
    }

    void ClawUICursor::setMaterialName( const String &materialName )
    {
        m_materialName = materialName;
    }

    void ClawUICursor::setSize( const Vector2F &size )
    {
        ClawUIElement::setSize( size );
    }

    void ClawUICursor::draw( struct wp_context *ctx )
    {
        if( !beginWorkphoneWidget( ctx ) )
        {
            return;
        }

        auto canvas = wp_window_get_canvas( ctx );
        if( !canvas )
        {
            return;
        }

        const auto bounds = getWorkphoneBounds();
        const auto colour = ClawUIWorkphoneContext::toWorkphoneColor( getColour() );
        wp_fill_triangle( canvas, bounds.x, bounds.y, bounds.x, bounds.y + bounds.h, bounds.x + bounds.w,
                          bounds.y + bounds.h * 0.65f, colour );
    }
}  // namespace workphone::ui
