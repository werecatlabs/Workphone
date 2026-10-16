#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiDropJob.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Interface/UI/IUIDropTarget.hpp>
#include <Workphone/Interface/System/IEvent.hpp>
#include <utility>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiDropJob, Job );

    ImGuiDropJob::ImGuiDropJob() = default;
    ImGuiDropJob::~ImGuiDropJob() = default;

    void ImGuiDropJob::execute()
    {
        if( !isInterrupted() && m_dropTarget )
        {
            try
            {
                m_dropTarget->handleEvent( EventType::UI, IEvent::handleDrop, m_arguments,
                                           m_sender, m_object, nullptr );
            }
            catch( const std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
            catch( ... )
            {
                WP_LOG_ERROR( "ImGuiDropJob::execute: drop target threw an unknown exception." );
            }
        }
    }

    void ImGuiDropJob::setDropTarget( SmartPtr<IUIDropTarget> dropTarget )
    {
        m_dropTarget = dropTarget;
    }

    void ImGuiDropJob::setSender( SmartPtr<ISharedObject> sender )
    {
        m_sender = sender;
    }

    void ImGuiDropJob::setObject( SmartPtr<ISharedObject> object )
    {
        m_object = object;
    }

    void ImGuiDropJob::setArguments( Array<Parameter> arguments )
    {
        m_arguments = std::move( arguments );
    }
}  // namespace workphone::ui
