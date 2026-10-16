#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiDragJob.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Interface/UI/IUIDragSource.hpp>
#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone::ui
{

    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiDragJob, Job );

    void ImGuiDragJob::execute()
    {
        if( isInterrupted() )
        {
            return;
        }

        auto element = m_element.lock();
        if( !m_dragSource || !element || element->getLoadingState() != LoadingState::Loaded )
        {
            return;
        }

        auto args = Array<Parameter>();
        args.resize( m_root ? 1 : 3 );
        args[0].object = element;

        try
        {
            auto result = m_dragSource->handleEvent( EventType::UI, IEvent::handleDrag, args,
                                                     element, element, nullptr );
            m_payload = result.str;
        }
        catch( const std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
        catch( ... )
        {
            WP_LOG_ERROR( "ImGuiDragJob::execute: drag source threw an unknown exception." );
        }
    }

    ImGuiDragJob::~ImGuiDragJob()
    {
    }

    ImGuiDragJob::ImGuiDragJob()
    {
    }

    void ImGuiDragJob::setDragSource( SmartPtr<IUIDragSource> dragSource )
    {
        m_dragSource = dragSource;
    }

    void ImGuiDragJob::setElement( SmartPtr<IUIElement> element )
    {
        m_element = element;
    }

    void ImGuiDragJob::setRoot( bool root )
    {
        m_root = root;
    }

    const String &ImGuiDragJob::getPayload() const
    {
        return m_payload;
    }

}  // namespace workphone::ui
