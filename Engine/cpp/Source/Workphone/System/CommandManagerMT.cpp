#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/CommandManagerMT.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Interface/System/ICommand.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/System/IJobQueue.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/Interface/System/ITask.hpp>
#include <Workphone/System/Job.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Memory/WeakPtr.hpp>
#include <algorithm>

namespace workphone
{
    class HistoryCommandJob final : public Job
    {
    public:
        std::function<void( IJob * )> run;
        void execute() override
        {
            if( run )
                run( this );
        }
        WP_CLASS_REGISTER_DECL;
    };
    WP_CLASS_REGISTER_DERIVED( workphone, HistoryCommandJob, Job );

    CommandManagerMT::CommandManagerMT() :
        m_currentCommand( 0 ),
        m_lastUndoCommand( -1 ),
        m_lastRedoCommand( -1 ),
        m_numStoredCommands( 100 )
    {
        setName( "CommandManagerMT" );
        m_commands.reserve( m_numStoredCommands );
    }
    CommandManagerMT::~CommandManagerMT() = default;

    void CommandManagerMT::load( SmartPtr<ISharedObject> )
    {
        ScopedLock lock( this );
        setLoadingState( LoadingState::Loaded );
    }
    void CommandManagerMT::unload( SmartPtr<ISharedObject> )
    {
        ScopedLock lock( this );
        for( auto command : m_commands )
            if( command )
            {
                if( std::find( m_executingCommands.begin(), m_executingCommands.end(), command ) !=
                    m_executingCommands.end() )
                    m_deferredUnloads.push_back( command );
                else
                    command->unload( nullptr );
            }
        clearAll();
        setLoadingState( LoadingState::Unloaded );
    }

    void CommandManagerMT::addCommand( SmartPtr<ICommand> command )
    {
        ScopedLock lock( this );
        if( !command )
            return;
        // Cursor is the number of applied commands. A new branch discards all redo work.
        for( size_t i = m_currentCommand; i < m_commands.size(); ++i )
            invalidatePending( m_commands[i] );
        m_commands.erase( m_commands.begin() + m_currentCommand, m_commands.end() );
        while( m_commands.size() >= m_numStoredCommands )
        {
            // History eviction must not cancel a legitimate queued operation.
            m_commands.erase( m_commands.begin() );
        }
        m_commands.push_back( command );
        m_currentCommand = static_cast<s32>( m_commands.size() );
        m_lastCommand = CommandType::COMMAND_MANAGER_ADD;
        auto application = core::IApplicationManager::instancePtr();
        const auto notifyAdded = [&] {
            if( application )
            {
                Array<Parameter> args;
                args.emplace_back( m_currentCommand );
                application->triggerEvent( EventType::Application, IEvent::addCommand, args, this,
                                           nullptr, nullptr );
            }
        };
        // Already executed owner-thread edits only need history registration.
        if( command->getState() == ICommand::State::Finished )
        {
            notifyAdded();
            return;
        }
        auto tasks = application ? application->getTaskManager() : nullptr;
        auto primary = tasks ? tasks->getTask( TaskId::Primary ) : nullptr;
        auto queue = application ? application->getJobQueue() : nullptr;
        if( ( command->isPrimary() && primary ) || ( !command->isPrimary() && ( queue || primary ) ) )
        {
            auto job = make_ptr<HistoryCommandJob>();
            const WeakPtr<CommandManagerMT> owner( this );
            job->run = [owner, command]( IJob *identity ) mutable {
                auto manager = owner.lock();
                if( !manager )
                    return;
                {
                    ScopedLock guard( manager.get() );
                    const auto present = std::find_if(
                        manager->m_pendingJobs.begin(), manager->m_pendingJobs.end(),
                        [identity]( const auto &entry ) { return entry.second.get() == identity; } );
                    if( present == manager->m_pendingJobs.end() )
                        return;
                    manager->m_executingCommands.push_back( command );
                    command->setState( ICommand::State::Executing );
                }
                // Commands can wait for other engine tasks; never hold the history lock here.
                std::exception_ptr failure;
                try
                {
                    command->execute();
                }
                catch( ... )
                {
                    failure = std::current_exception();
                }
                bool deferredUnload = false;
                {
                    ScopedLock guard( manager.get() );
                    command->setState( ICommand::State::Finished );
                    auto &executing = manager->m_executingCommands;
                    executing.erase( std::remove( executing.begin(), executing.end(), command ),
                                     executing.end() );
                    // A clear followed by re-add of this object must not retire the replacement job.
                    auto &pending = manager->m_pendingJobs;
                    pending.erase( std::remove_if( pending.begin(), pending.end(),
                                                   [identity]( const auto &entry ) {
                                                       return entry.second.get() == identity;
                                                   } ),
                                   pending.end() );
                    auto &retired = manager->m_deferredUnloads;
                    deferredUnload =
                        std::find( retired.begin(), retired.end(), command ) != retired.end();
                    retired.erase( std::remove( retired.begin(), retired.end(), command ),
                                   retired.end() );
                    if( failure )
                        manager->removeCommand( command );
                }
                if( deferredUnload )
                    command->unload( nullptr );
                if( failure )
                    std::rethrow_exception( failure );
            };
            m_pendingJobs.emplace_back( command, job );
            command->setState( ICommand::State::Queued );
            try
            {
                if( !command->isPrimary() && queue )
                    queue->addJob( job );
                else
                    primary->addJob( job );
            }
            catch( ... )
            {
                removeCommand( command );
                throw;
            }
        }
        else
            command->execute();
        // Event handlers may request undo; pending identity/state must already be installed.
        notifyAdded();
    }
    void CommandManagerMT::removeCommand( SmartPtr<ICommand> command )
    {
        ScopedLock lock( this );
        invalidatePending( command );
        for( size_t i = 0; i < m_commands.size(); )
        {
            if( m_commands[i] != command )
            {
                ++i;
                continue;
            }
            if( i < static_cast<size_t>( m_currentCommand ) )
                --m_currentCommand;
            m_commands.erase( m_commands.begin() + i );
        }
    }
    bool CommandManagerMT::hasCommand( SmartPtr<ICommand> command )
    {
        ScopedLock lock( this );
        return std::find( m_commands.begin(), m_commands.end(), command ) != m_commands.end();
    }
    bool CommandManagerMT::isCommandQueued( SmartPtr<ICommand> command )
    {
        ScopedLock lock( this );
        if( std::any_of( m_pendingJobs.begin(), m_pendingJobs.end(),
                         [&command]( const auto &entry ) { return entry.first == command; } ) )
            return true;
        const auto found = std::find( m_commands.begin(), m_commands.end(), command );
        return found != m_commands.end() && found - m_commands.begin() >= m_currentCommand;
    }
    SmartPtr<ICommand> CommandManagerMT::getNextCommand()
    {
        ScopedLock lock( this );
        if( !m_pendingJobs.empty() || !m_executingCommands.empty() ||
            m_currentCommand >= static_cast<s32>( m_commands.size() ) )
            return nullptr;
        auto command = m_commands[m_currentCommand++];
        m_lastCommand = CommandType::COMMAND_MANAGER_REDO;
        if( auto application = core::IApplicationManager::instancePtr() )
        {
            Array<Parameter> args;
            args.emplace_back( static_cast<s32>( m_commands.size() ) - m_currentCommand );
            args.emplace_back( m_currentCommand );
            application->triggerEvent( EventType::Application, IEvent::getNextCommand, args, this,
                                       nullptr, nullptr );
        }
        return command;
    }
    SmartPtr<ICommand> CommandManagerMT::getPreviousCommand()
    {
        ScopedLock lock( this );
        if( !m_pendingJobs.empty() || !m_executingCommands.empty() || m_currentCommand <= 0 )
            return nullptr;
        auto command = m_commands[--m_currentCommand];
        m_lastCommand = CommandType::COMMAND_MANAGER_UNDO;
        if( auto application = core::IApplicationManager::instancePtr() )
        {
            Array<Parameter> args;
            args.emplace_back( static_cast<s32>( m_commands.size() ) - m_currentCommand );
            args.emplace_back( m_currentCommand );
            application->triggerEvent( EventType::Application, IEvent::getPreviousCommand, args, this,
                                       nullptr, nullptr );
        }
        return command;
    }
    void CommandManagerMT::clearAll()
    {
        ScopedLock lock( this );
        m_pendingJobs.clear();
        m_commands.clear();
        m_currentCommand = 0;
        m_lastUndoCommand = m_lastRedoCommand = -1;
        m_lastCommand = CommandType::COMMAND_MANAGER_NONE;
    }
    void CommandManagerMT::invalidatePending( SmartPtr<ICommand> command )
    {
        m_pendingJobs.erase(
            std::remove_if( m_pendingJobs.begin(), m_pendingJobs.end(),
                            [&command]( const auto &entry ) { return entry.first == command; } ),
            m_pendingJobs.end() );
    }
    void CommandManagerMT::lock()
    {
        m_mutex.lock();
    }
    bool CommandManagerMT::try_lock()
    {
        return m_mutex.try_lock();
    }
    void CommandManagerMT::unlock()
    {
        m_mutex.unlock();
    }
}  // namespace workphone
