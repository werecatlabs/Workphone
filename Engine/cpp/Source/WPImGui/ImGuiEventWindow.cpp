#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiEventWindow.hpp>
#include <Workphone/Workphone.hpp>
#include <imgui.h>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiEventWindow, ImGuiWindowT<IUIEventWindow> );

    ImGuiEventWindow::ImGuiEventWindow() = default;

    ImGuiEventWindow::~ImGuiEventWindow() = default;

    Array<SmartPtr<IEvent>> ImGuiEventWindow::getEvents() const
    {
        return m_events;
    }

    void ImGuiEventWindow::setEvents( const Array<SmartPtr<IEvent>> &events )
    {
        m_events = events;
    }

    void ImGuiEventWindow::update()
    {
        auto eventName = String( "Event" );
        ImGui::Text( "%s", eventName.c_str() );

        for( auto e : m_events )
        {
            auto componentEvent = workphone::dynamic_pointer_cast<scene::IComponentEvent>( e );
            auto listeners = componentEvent->getListeners();
            for( auto listener : listeners )
            {
                constexpr int size = 1024;
                char buf[size];

                auto component = listener->getComponent();
                StringUtil::toBuffer( "", buf, size );

                auto objectLabel = String( "Object" );
                if( ImGui::InputText( objectLabel.c_str(), buf, IM_ARRAYSIZE( buf ) ) )
                {
                    //listener->setComponent(  );
                }

                auto objectFunctionLabel = String( "Function" );

                auto functionName = listener->getFunction();
                StringUtil::toBuffer( objectFunctionLabel, buf, size );

                if( ImGui::InputText( objectFunctionLabel.c_str(), buf, IM_ARRAYSIZE( buf ) ) )
                {
                    listener->setFunction( functionName );
                }
            }
        }
    }
}  // namespace workphone::ui
