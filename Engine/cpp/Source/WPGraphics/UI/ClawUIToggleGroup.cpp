#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/UI/ClawUIToggleGroup.hpp>
#include <WPGraphics/UI/ClawUIToggleButton.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::ui
{
    ClawUIToggleGroup::ClawUIToggleGroup( const String &id ) : m_curToggledBtn( nullptr )
    {
        setName( id );
        setType( "ToggleGroup" );
    }

    ClawUIToggleGroup::~ClawUIToggleGroup() = default;

    void ClawUIToggleGroup::addToggleButton( ClawUIToggleButton *button )
    {
        if( button && std::find( m_toggleButtons.begin(), m_toggleButtons.end(), button ) ==
                          m_toggleButtons.end() )
        {
            m_toggleButtons.push_back( button );
        }
    }

    bool ClawUIToggleGroup::removeToggleButton( ClawUIToggleButton *button )
    {
        auto it = std::find( m_toggleButtons.begin(), m_toggleButtons.end(), button );
        if( it == m_toggleButtons.end() )
        {
            return false;
        }

        m_toggleButtons.erase( it );
        if( m_curToggledBtn == button )
        {
            m_curToggledBtn = nullptr;
        }
        return true;
    }

    void ClawUIToggleGroup::OnSetButtonToggled( ClawUIToggleButton *button )
    {
        if( !button || m_curToggledBtn == button )
        {
            return;
        }

        for( auto toggleButton : m_toggleButtons )
        {
            if( toggleButton && toggleButton != button )
            {
                toggleButton->setToggled( false );
            }
        }

        m_curToggledBtn = button;
        m_curToggledBtn->setToggled( true );
    }

    ClawUIToggleButton *ClawUIToggleGroup::getCurToggledButton() const
    {
        return m_curToggledBtn;
    }

    void ClawUIToggleGroup::draw( struct wp_context *ctx )
    {
        if( ctx && isVisible() && isEnabled() )
        {
            drawWorkphoneChildren( ctx );
        }
    }
}  // namespace workphone::ui
