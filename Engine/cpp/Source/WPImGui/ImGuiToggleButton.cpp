#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiToggleButton.hpp>
#include <WPImGui/ImGuiUtil.hpp>
#include <Workphone/Workphone.hpp>
#include <imgui.h>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiToggleButton, ImGuiElement<IUIToggle> );

    ImGuiToggleButton::ImGuiToggleButton()
    {
    }

    ImGuiToggleButton::~ImGuiToggleButton()
    {
    }

    void ImGuiToggleButton::update()
    {
        auto sameLine = getSameLine();
        ImGui::SameLine( sameLine );

        auto name = getName();
        //if( ImGui::Button( name.c_str() ) )
        //{
        //    m_isToggled = !m_isToggled;
        //}

        //ImGui::Checkbox( name.c_str(), &m_isToggled );

        auto scale = getScale();

        if( ImGuiUtil::ToggleButton( name.c_str(), &m_isToggled, scale ) )
        {
            if( auto parent = getParent() )
            {
                if( parent->isDerived<IUIToolbar>() )
                {
                    auto toolbar = workphone::static_pointer_cast<IUIToolbar>( parent );
                    WP_ASSERT( toolbar );

                    auto listeners = toolbar->getObjectListeners();
                    for( auto listener : listeners )
                    {
                        auto args = Array<Parameter>();
                        args.resize( 1 );
                        args.push_back( Parameter( m_isToggled ) );

                        listener->handleEvent( EventType::UI, IEvent::handleSelection, args, toolbar,
                                               this, nullptr );
                    }
                }
                else
                {
                    auto listeners = getObjectListeners();
                    for( auto listener : listeners )
                    {
                        if( listener )
                        {
                            auto args = Array<Parameter>();
                            args.resize( 1 );
                            args.push_back( Parameter( m_isToggled ) );

                            listener->handleEvent( EventType::UI, IEvent::handleSelection, args, this,
                                                   this, nullptr );
                        }
                    }
                }
            }
        }
    }

    void ImGuiToggleButton::setToggled( bool toggled )
    {
        m_isToggled = toggled;
    }

    bool ImGuiToggleButton::isToggled() const
    {
        return m_isToggled;
    }

    IUIToggle::ToggleType ImGuiToggleButton::getToggleType() const
    {
        return m_toggleType;
    }

    void ImGuiToggleButton::setToggleType( ToggleType toggleType )
    {
        m_toggleType = toggleType;
    }

    IUIToggle::ToggleState ImGuiToggleButton::getToggleState() const
    {
        return m_toggleState;
    }

    void ImGuiToggleButton::setToggleState( ToggleState toggleState )
    {
        m_toggleState = toggleState;
    }

    bool ImGuiToggleButton::getShowLabel() const
    {
        return m_showLabel;
    }

    void ImGuiToggleButton::setShowLabel( bool showLabel )
    {
        m_showLabel = showLabel;
    }

    String ImGuiToggleButton::getLabel() const
    {
        return m_label;
    }

    void ImGuiToggleButton::setLabel( const String &label )
    {
        m_label = label;
    }

    void ImGuiToggleButton::setTextSize( f32 textSize )
    {
        m_textSize = textSize;
    }

    f32 ImGuiToggleButton::getTextSize() const
    {
        return m_textSize;
    }
}  // namespace workphone::ui
