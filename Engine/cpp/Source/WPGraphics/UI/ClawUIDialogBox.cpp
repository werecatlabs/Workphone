#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/UI/ClawUIDialogBox.hpp>
#include <Workphone/Workphone.hpp>
#include <WorkphoneCore/workphone.h>
#include <WorkphoneCore/workphone_group.h>

namespace workphone::ui
{
    ClawUIDialogBox::ClawUIDialogBox()
    {
        setType( "DialogBox" );
    }

    ClawUIDialogBox::~ClawUIDialogBox() = default;

    void ClawUIDialogBox::initialise( SmartPtr<IUIElement> &parent, const TiXmlNode *pNode )
    {
        setParent( parent );
    }

    bool ClawUIDialogBox::handleEvent( const SmartPtr<IInputEvent> &event )
    {
        return isEnabled() && ClawUIElement::handleEvent( event );
    }

    void ClawUIDialogBox::setScrollSpeed( s32 scrollSpeed )
    {
        m_scroll_speed = static_cast<u32>( MathI::max( scrollSpeed, 0 ) );
    }

    String ClawUIDialogBox::getString()
    {
        return m_Text[0];
    }

    void ClawUIDialogBox::OnScroll()
    {
        if( m_bPauseDialog || m_bIsFinished )
        {
            return;
        }

        if( m_Text[0].size() < m_Text[1].size() )
        {
            m_Text[0] = m_Text[1].substr( 0, m_Text[0].size() + 1 );
        }
        else
        {
            m_bIsFinished = true;
        }
    }

    void ClawUIDialogBox::OnClose()
    {
        setVisible( false );
        setEnabled( false );
    }

    void ClawUIDialogBox::draw( struct wp_context *ctx )
    {
        if( !beginWorkphoneWidget( ctx ) )
        {
            return;
        }

        auto id = getName();
        if( id.empty() )
        {
            id = "Dialog";
        }

        const auto *workphoneId = reinterpret_cast<const wp_c8 *>( id.c_str() );
        if( wp_group_begin( ctx, workphoneId, WORKPHONE_WINDOW_BORDER | WORKPHONE_WINDOW_NO_SCROLLBAR ) )
        {
            const auto bounds = getWorkphoneBounds();
            wp_layout_row_dynamic( ctx, MathF::max( bounds.h - 32.0f, 1.0f ), 1 );
            wp_label_wrap( ctx, reinterpret_cast<const wp_c8 *>( m_Text[0].c_str() ) );
            wp_layout_row_dynamic( ctx, 24.0f, 1 );
            if( wp_button_label( ctx, "Close" ) )
            {
                OnClose();
            }
            wp_group_end( ctx );
        }
    }
}  // namespace workphone::ui
