#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiButton.hpp>
#include <Workphone/Workphone.hpp>
#include <imgui.h>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiButton, ImGuiElement<IUIButton> );

    ImGuiButton::ImGuiButton() = default;

    ImGuiButton::~ImGuiButton() = default;

    void ImGuiButton::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loaded );
    }

    void ImGuiButton::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloaded );
    }

    void ImGuiButton::update()
    {
        auto label = m_label.load();
        if( StringUtil::isNullOrEmpty( label ) )
        {
            label = "Untitled";
        }

        auto enabled = isEnabled();
        ImGui::BeginDisabled( !enabled );
        auto pressed = ImGui::Button( label.c_str() );
        ImGui::EndDisabled();

        if( pressed && enabled )
        {
            auto applicationManager = core::IApplicationManager::instancePtr();

            if( auto parent = getParent() )
            {
                if( parent->isDerived<IUIToolbar>() )
                {
                    auto toolbar = workphone::static_pointer_cast<IUIToolbar>( parent );
                    if( toolbar )
                    {
                        auto args = Array<Parameter>();
                        applicationManager->triggerEvent( EventType::UI, IEvent::handleSelection,
                                                          args, toolbar, this, nullptr, false,
                                                          Thread::Application_Flag );
                    }
                }
                else
                {
                    auto args = Array<Parameter>();
                    applicationManager->triggerEvent( EventType::UI, IEvent::handleSelection, args,
                                                      this, nullptr, nullptr, false,
                                                      Thread::Application_Flag );
                }
            }
        }
    }

    String ImGuiButton::getLabel() const
    {
        auto str = m_label.load();
        return str.c_str();
    }

    void ImGuiButton::setLabel( const String &label )
    {
        m_label = label.c_str();
    }

    void ImGuiButton::setTextSize( f32 textSize )
    {
        m_textSize = textSize;
    }

    f32 ImGuiButton::getTextSize() const
    {
        return m_textSize;
    }
}  // namespace workphone::ui
