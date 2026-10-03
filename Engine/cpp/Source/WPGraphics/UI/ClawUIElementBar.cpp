#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/UI/ClawUIElementBar.hpp>
#include <WPGraphics/UI/ClawUIWorkphoneContext.hpp>
#include <Workphone/Workphone.hpp>
#include <WorkphoneCore/workphone.h>

namespace workphone::ui
{
    ClawUIElementBar::ClawUIElementBar()
    {
        m_type = "Bar";
    }

    ClawUIElementBar::~ClawUIElementBar() = default;

    void ClawUIElementBar::setMaterialName( const String &materialName )
    {
        m_materialName = materialName;
    }

    void ClawUIElementBar::setPosition( const Vector2F &position )
    {
        ClawUIElement::setPosition( position );
    }

    void ClawUIElementBar::setPoints( f32 points )
    {
        m_targetPoints = MathF::max( points, 0.0f );
    }

    void ClawUIElementBar::setMaxPoints( f32 maxPoints )
    {
        m_maxPoints = MathF::max( maxPoints, 0.0f );
    }

    void ClawUIElementBar::update()
    {
        m_prevPoints = m_curPoints;
        m_curPoints = m_targetPoints;
        ClawUIElement::update();
    }

    u8 ClawUIElementBar::getBarOrientation() const
    {
        return m_barOrientation;
    }

    void ClawUIElementBar::setBarOrientation( u8 barOrientation )
    {
        m_barOrientation = barOrientation;
    }

    void ClawUIElementBar::draw( struct wp_context *ctx )
    {
        if( !beginWorkphoneWidget( ctx ) )
        {
            return;
        }

        const auto ratio =
            m_maxPoints > 0.0f ? MathF::clamp( m_targetPoints / m_maxPoints, 0.0f, 1.0f ) : 0.0f;
        const auto bounds = getWorkphoneBounds();
        if( auto canvas = wp_window_get_canvas( ctx ) )
        {
            auto background = getColour();
            background.a *= 0.25f;
            wp_fill_rect( canvas, bounds, 0.0f, ClawUIWorkphoneContext::toWorkphoneColor( background ) );

            auto fill = bounds;
            if( m_barOrientation == static_cast<u8>( BarOrientation::BO_VERTICAL ) )
            {
                fill.y += fill.h * ( 1.0f - ratio );
                fill.h *= ratio;
            }
            else
            {
                fill.w *= ratio;
            }
            wp_fill_rect( canvas, fill, 0.0f, ClawUIWorkphoneContext::toWorkphoneColor( getColour() ) );
        }
        drawWorkphoneChildren( ctx );
    }
}  // namespace workphone::ui
