#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiLabelSliderPair.hpp>
#include <imgui.h>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiLabelSliderPair, ImGuiElement<IUILabelSliderPair> );

    ImGuiLabelSliderPair::ImGuiLabelSliderPair() = default;

    ImGuiLabelSliderPair::~ImGuiLabelSliderPair() = default;

    void ImGuiLabelSliderPair::update()
    {
        ImGui::PushID( this );
        ImGui::Text( "%s", m_label.c_str() );
        ImGui::SameLine();
        if( ImGui::SliderFloat( "", &m_value, m_minValue, m_maxValue ) )
        {
            if( auto parent = getParent() )
            {
                u32 count = 0;
                auto listeners = FixedArray<SmartPtr<IEventListener>, 32>();
                getObjectListenersList( listeners.data(), &count, static_cast<u32>( listeners.size() ) );

                for( u32 i = 0; i < count; ++i )
                {
                    auto &listener = listeners[i];
                    auto args = Array<Parameter>();
                    args.reserve( 1 );

                    args.push_back( Parameter( m_value ) );
                    listener->handleEvent( EventType::UI, IEvent::handleValueChanged, args, this,
                                           this, nullptr );
                }
            }
        }

        ImGui::PopID();
    }

    void ImGuiLabelSliderPair::setMaxValue( f32 maxValue )
    {
        m_maxValue = maxValue;
    }

    f32 ImGuiLabelSliderPair::getMaxValue() const
    {
        return m_maxValue;
    }

    void ImGuiLabelSliderPair::setMinValue( f32 minValue )
    {
        m_minValue = minValue;
    }

    f32 ImGuiLabelSliderPair::getMinValue() const
    {
        return m_minValue;
    }

    void ImGuiLabelSliderPair::setValue( f32 value )
    {
        m_value = value;
    }

    f32 ImGuiLabelSliderPair::getValue() const
    {
        return m_value;
    }

    void ImGuiLabelSliderPair::setLabel( const String &label )
    {
        m_label = label;
    }

    String ImGuiLabelSliderPair::getLabel() const
    {
        return m_label;
    }
}  // namespace workphone::ui
