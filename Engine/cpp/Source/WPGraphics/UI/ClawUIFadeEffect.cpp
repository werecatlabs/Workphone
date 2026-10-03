#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/UI/ClawUIFadeEffect.hpp>
#include <WPGraphics/UI/ClawUIWorkphoneContext.hpp>
#include <Workphone/Workphone.hpp>
#include <WorkphoneCore/workphone.h>

namespace workphone::ui
{
    ClawUIFadeEffect::ClawUIFadeEffect()
    {
        m_type = "FadeEffect";
    }

    ClawUIFadeEffect::~ClawUIFadeEffect() = default;

    void ClawUIFadeEffect::update()
    {
        OnUpdateState( static_cast<u8>( m_fadeState ) );
        ClawUIElement::update();
    }

    void ClawUIFadeEffect::setMaterialName( const String &materialName )
    {
        m_materialName = materialName;
    }

    void ClawUIFadeEffect::setPosition( const Vector2F &position )
    {
        ClawUIElement::setPosition( position );
    }

    void ClawUIFadeEffect::setSize( const Vector2F &size )
    {
        ClawUIElement::setSize( size );
    }

    void ClawUIFadeEffect::OnEnterState( u8 state )
    {
        m_fadeState = static_cast<FadeState>( MathI::clamp( state, 0, FS_COUNT - 1 ) );
        switch( m_fadeState )
        {
        case FS_FADEIN:
            OnEnterFadeInState();
            break;
        case FS_FADEOUT:
            OnEnterFadeOutState();
            break;
        default:
            OnEnterIdleState();
            break;
        }
    }

    void ClawUIFadeEffect::OnUpdateState( u8 state )
    {
        switch( static_cast<FadeState>( state ) )
        {
        case FS_FADEIN:
            OnUpdateFadeInState();
            break;
        case FS_FADEOUT:
            OnUpdateFadeOutState();
            break;
        default:
            OnUpdateIdleState();
            break;
        }
    }

    void ClawUIFadeEffect::OnLeaveState( u8 state )
    {
        switch( static_cast<FadeState>( state ) )
        {
        case FS_FADEIN:
            OnLeaveFadeInState();
            break;
        case FS_FADEOUT:
            OnLeaveFadeOutState();
            break;
        default:
            OnLeaveIdleState();
            break;
        }
    }

    void ClawUIFadeEffect::OnEnterIdleState()
    {
    }

    void ClawUIFadeEffect::OnEnterFadeInState()
    {
        m_alpha = 1.0f;
    }

    void ClawUIFadeEffect::OnEnterFadeOutState()
    {
        m_alpha = 0.0f;
    }

    void ClawUIFadeEffect::OnUpdateIdleState()
    {
    }

    void ClawUIFadeEffect::OnUpdateFadeInState()
    {
        if( auto applicationManager = core::IApplicationManager::instance() )
        {
            if( auto timer = applicationManager->getTimer() )
            {
                m_alpha = MathF::max( 0.0f, m_alpha - 0.5f * static_cast<f32>( timer->getDeltaTime() ) );
                if( m_alpha <= 0.0f )
                {
                    m_fadeState = FS_IDLE;
                }
            }
        }
    }

    void ClawUIFadeEffect::OnUpdateFadeOutState()
    {
        if( auto applicationManager = core::IApplicationManager::instance() )
        {
            if( auto timer = applicationManager->getTimer() )
            {
                m_alpha = MathF::min( 1.0f, m_alpha + 0.5f * static_cast<f32>( timer->getDeltaTime() ) );
                if( m_alpha >= 1.0f )
                {
                    m_fadeState = FS_IDLE;
                }
            }
        }
    }

    void ClawUIFadeEffect::OnLeaveIdleState()
    {
        handleEvent( "EndLeaveIdle" );
    }

    void ClawUIFadeEffect::OnLeaveFadeInState()
    {
        handleEvent( "EndFadeIn" );
    }

    void ClawUIFadeEffect::OnLeaveFadeOutState()
    {
        handleEvent( "EndFadeOut" );
    }

    u8 ClawUIFadeEffect::GetStateFromName( const String &stateName ) const
    {
        if( stateName == "FadeIn" )
        {
            return FS_FADEIN;
        }
        if( stateName == "FadeOut" )
        {
            return FS_FADEOUT;
        }
        return FS_IDLE;
    }

    String ClawUIFadeEffect::GetStateNameFromId( u8 stateId ) const
    {
        switch( static_cast<FadeState>( stateId ) )
        {
        case FS_FADEIN:
            return "FadeIn";
        case FS_FADEOUT:
            return "FadeOut";
        default:
            return "Idle";
        }
    }

    SmartPtr<IFSM> &ClawUIFadeEffect::getFSM()
    {
        return m_fsm;
    }

    void ClawUIFadeEffect::handleEvent( const String &eventType )
    {
        if( eventType == "FadeIn" )
        {
            OnEnterState( FS_FADEIN );
        }
        else if( eventType == "FadeOut" )
        {
            OnEnterState( FS_FADEOUT );
        }
    }

    void ClawUIFadeEffect::draw( struct wp_context *ctx )
    {
        if( !beginWorkphoneWidget( ctx ) )
        {
            return;
        }

        auto colour = getColour();
        colour.a *= MathF::clamp( m_alpha, 0.0f, 1.0f );
        if( auto canvas = wp_window_get_canvas( ctx ) )
        {
            wp_fill_rect( canvas, getWorkphoneBounds(), 0.0f,
                          ClawUIWorkphoneContext::toWorkphoneColor( colour ) );
        }
        drawWorkphoneChildren( ctx );
    }
}  // namespace workphone::ui
