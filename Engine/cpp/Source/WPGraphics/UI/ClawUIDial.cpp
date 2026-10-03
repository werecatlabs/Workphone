#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/UI/ClawUIDial.hpp>
#include <WPGraphics/UI/ClawUIWorkphoneContext.hpp>
#include <Workphone/Workphone.hpp>
#include <WorkphoneCore/workphone.h>

namespace workphone::ui
{
    ClawUIDial::ClawUIDial() :
        m_fStartAngle( -WORKPHONE_HALF_PI_F ),
        m_fEndAngle( WORKPHONE_HALF_PI_F ),
        m_fNeedleAngle( -WORKPHONE_HALF_PI_F ),
        m_bIsVisible( true )
    {
        setType( "Dial" );
    }

    ClawUIDial::~ClawUIDial() = default;

    void ClawUIDial::initialise()
    {
    }

    void ClawUIDial::setNeedlePosition( f32 position )
    {
        position = MathF::clamp( position, 0.0f, 1.0f );
        m_fNeedleAngle = m_fStartAngle + ( ( m_fEndAngle - m_fStartAngle ) * position );
    }

    void ClawUIDial::draw( struct wp_context *ctx )
    {
        if( !beginWorkphoneWidget( ctx ) )
        {
            return;
        }

        const auto bounds = getWorkphoneBounds();
        auto canvas = wp_window_get_canvas( ctx );
        if( !canvas )
        {
            return;
        }

        const auto colour = ClawUIWorkphoneContext::toWorkphoneColor( getColour() );
        const auto radius = MathF::min( bounds.w, bounds.h ) * 0.45f;
        const auto cx = bounds.x + bounds.w * 0.5f;
        const auto cy = bounds.y + bounds.h * 0.5f;
        wp_stroke_circle( canvas, bounds, 2.0f, colour );
        wp_stroke_line( canvas, cx, cy, cx + wp_cosf( m_fNeedleAngle ) * radius,
                        cy + wp_sinf( m_fNeedleAngle ) * radius, 2.0f, colour );
        drawWorkphoneChildren( ctx );
    }
}  // namespace workphone::ui
