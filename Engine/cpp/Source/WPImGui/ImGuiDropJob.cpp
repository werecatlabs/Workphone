#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiDropJob.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Interface/UI/IUIDropTarget.hpp>
#include <Workphone/Interface/System/IEvent.hpp>
#include <imgui.h>
#include <utility>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiDropJob, Job );

    ImGuiDropJob::ImGuiDropJob() = default;
    ImGuiDropJob::~ImGuiDropJob() = default;

    void ImGuiDropJob::execute()
    {
        if( !isInterrupted() && m_dropTarget &&
            ( !m_object || m_object->getLoadingState() == LoadingState::Loaded ) )
        {
            try
            {
                m_dropTarget->handleEvent( EventType::UI, IEvent::handleDrop, m_arguments, m_sender,
                                           m_object, nullptr );
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

    void ImGuiDropJob::queue( SmartPtr<IUIDropTarget> dropTarget, Array<Parameter> arguments,
                              SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object )
    {
        if( !dropTarget )
        {
            return;
        }

        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager ? applicationManager->getFactoryManager() : nullptr;
        if( !factoryManager )
        {
            return;
        }

        auto job = factoryManager->make_ptr<ImGuiDropJob>();
        if( !job )
        {
            return;
        }

        // A tree row, its insertion line and the enclosing window can all observe the same
        // release. Deduplicate that gesture, without delaying separate asset drops.
        if( ImGui::GetCurrentContext() && ImGui::IsMouseReleased( ImGuiMouseButton_Left ) )
        {
            if( const auto payload = ImGui::GetDragDropPayload();
                payload && payload->IsDataType( "_TREENODE" ) )
            {
                static thread_local ImGuiContext *lastContext = nullptr;
                static thread_local int lastFrame = -1;
                static thread_local ImGuiID lastSource = 0;
                const auto context = ImGui::GetCurrentContext();
                const auto frame = ImGui::GetFrameCount();
                if( lastContext == context && lastFrame == frame && lastSource == payload->SourceId )
                {
                    return;
                }
                lastContext = context;
                lastFrame = frame;
                lastSource = payload->SourceId;
            }
        }

        job->setDropTarget( dropTarget );
        job->setSender( sender );
        job->setObject( object );
        job->setArguments( std::move( arguments ) );

        auto jobQueue = applicationManager->getJobQueue();
        if( jobQueue && jobQueue->isRunning() )
        {
            jobQueue->addJob( job, TaskId::Application );
        }
        else
        {
            job->execute();
        }
    }
}  // namespace workphone::ui
