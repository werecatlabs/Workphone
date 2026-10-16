#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiListenerJob.hpp>
#include <Workphone/Workphone.hpp>
#include <utility>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiListenerJob, Job );

    ImGuiListenerJob::ImGuiListenerJob() = default;
    ImGuiListenerJob::~ImGuiListenerJob() = default;

    void ImGuiListenerJob::execute()
    {
        if( m_object && m_object->getLoadingState() != LoadingState::Loaded )
        {
            return;
        }

        for( auto &listener : m_listeners )
        {
            if( isInterrupted() )
            {
                return;
            }

            if( listener )
            {
                try
                {
                    listener->handleEvent( m_eventType, m_eventValue, m_arguments, m_sender,
                                           m_object, nullptr );
                }
                catch( const std::exception &e )
                {
                    WP_LOG_EXCEPTION( e );
                }
                catch( ... )
                {
                    WP_LOG_ERROR( "ImGuiListenerJob::execute: listener threw an unknown exception." );
                }
            }
        }
    }

    void ImGuiListenerJob::queue( Array<SmartPtr<IEventListener>> listeners,
                                  EventType eventType, hash_type eventValue,
                                  Array<Parameter> arguments, SmartPtr<ISharedObject> sender,
                                  SmartPtr<ISharedObject> object )
    {
        if( listeners.empty() )
        {
            return;
        }

        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager ? applicationManager->getFactoryManager() : nullptr;
        if( !factoryManager )
        {
            return;
        }

        auto job = factoryManager->make_ptr<ImGuiListenerJob>();
        if( !job )
        {
            return;
        }

        job->m_listeners = std::move( listeners );
        job->m_eventType = eventType;
        job->m_eventValue = eventValue;
        job->m_arguments = std::move( arguments );
        job->m_sender = sender;
        job->m_object = object;

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
