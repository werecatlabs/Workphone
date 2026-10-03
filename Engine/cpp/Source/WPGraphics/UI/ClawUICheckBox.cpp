#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/UI/ClawUICheckBox.hpp>
#include <Workphone/Workphone.hpp>
#include <WorkphoneCore/workphone.h>
#include <WorkphoneCore/workphone_toggle.h>

namespace workphone::ui
{
    ClawUICheckBox::ClawUICheckBox()
    {
        setType( "Checkbox" );
    }

    ClawUICheckBox::~ClawUICheckBox() = default;

    void ClawUICheckBox::setValue( bool value )
    {
        m_value = value;
    }

    bool ClawUICheckBox::getValue() const
    {
        return m_value;
    }

    void ClawUICheckBox::draw( struct wp_context *ctx )
    {
        if( !beginWorkphoneWidget( ctx ) )
        {
            return;
        }

        wp_bool active = m_value ? wp_true : wp_false;
        auto label = getLabel();
        if( label.empty() )
        {
            label = "Checkbox";
        }

        const auto *workphoneLabel = reinterpret_cast<const wp_c8 *>( label.c_str() );
        if( wp_checkbox_label_align( ctx, workphoneLabel, &active, WORKPHONE_WIDGET_LEFT,
                                     WORKPHONE_TEXT_LEFT ) )
        {
            m_value = active != wp_false;
        }
        drawWorkphoneChildren( ctx );
    }
}  // namespace workphone::ui
