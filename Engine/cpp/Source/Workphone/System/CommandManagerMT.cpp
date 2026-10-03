#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/CommandManagerMT.hpp>
#include <Workphone/Core/Parameter.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/System/ICommand.hpp>
#include <Workphone/Interface/System/IEvent.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/System/IJobQueue.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/Interface/System/ITask.hpp>
#include <Workphone/Jobs/RunCommandJob.hpp>
#include <algorithm>

namespace workphone
{
    CommandManagerMT::CommandManagerMT() :
        m_currentCommand( 0 ),
        m_lastUndoCommand( -1 ),
        m_lastRedoCommand( -1 ),
        m_numStoredCommands( 100 )
    {
    }

    CommandManagerMT::~CommandManagerMT()
    {
        m_commands.clear();
    }

    void CommandManagerMT::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );

        static const auto name = String( "CommandManagerMT" );
        setName( name );

        m_commands.reserve( m_numStoredCommands );
        m_lastCommand = CommandType::COMMAND_MANAGER_NONE;

        setLoadingState( LoadingState::Loaded );
    }

    void CommandManagerMT::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );

        for( auto &command : m_commands )
        {
            if( command )
            {
                command->unload( nullptr );
            }
        }

        m_commands.clear();

        setLoadingState( LoadingState::Unloaded );
    }

    void CommandManagerMT::addCommand( SmartPtr<ICommand> command )
    {
        ScopedLock lock( this );

        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();

        auto taskManager = applicationManager->getTaskManager();
        auto jobQueue = applicationManager->getJobQueue();

        auto primaryTask = taskManager->getTask( TaskId::Primary );

        auto index = m_currentCommand;
        auto count = ( static_cast<s32>( m_commands.size() ) - 1 ) - m_currentCommand;
        auto used = static_cast<s32>( m_commands.size() );

        // check if we need to remove first element
        if( index == 0 && m_lastCommand == CommandType::COMMAND_MANAGER_UNDO && m_lastUndoCommand == 0 )
        {
            m_commands.clear();
            m_commands.reserve( m_numStoredCommands );
        }

        if( !( index >= used || index < 0 || count < 1 || index + count > used ) &&
            m_commands.size() > 0 )
        {
            auto startIndex = m_currentCommand + 1;
            auto endIndex = ( static_cast<s32>( m_commands.size() ) - 1 ) - m_currentCommand;

            auto startIt = m_commands.begin();
            std::advance( startIt, startIndex );

            auto endIt = m_commands.begin();
            std::advance( endIt, endIndex );

            m_commands.erase( startIt, endIt );
        }

        if( m_commands.size() < m_numStoredCommands )
        {
            m_commands.push_back( command );
        }
        else
        {
            if( m_commands.empty() )
            {
                auto startIt = m_commands.begin();
                m_commands.erase( startIt );
            }

            m_commands.push_back( command );
        }

        m_currentCommand = static_cast<s32>( m_commands.size() - 1 );

        auto numCommands = static_cast<s32>( m_commands.size() );

        auto args = Array<Parameter>();
        args.emplace_back( numCommands );
        applicationManager->triggerEvent( EventType::Application, IEvent::addCommand, args, this,
                                          nullptr, nullptr );

        m_lastCommand = CommandType::COMMAND_MANAGER_ADD;

        if( command->isPrimary() )
        {
            if( primaryTask )
            {
                auto job = factoryManager->make_ptr<RunCommandJob>();
                job->setCommand( command );
                primaryTask->addJob( job );
            }
        }
        else if( jobQueue )
        {
            auto job = factoryManager->make_ptr<RunCommandJob>();
            job->setCommand( command );

            jobQueue->addJob( job );
        }
        else if( primaryTask )
        {
            auto job = factoryManager->make_ptr<RunCommandJob>();
            job->setCommand( command );

            primaryTask->addJob( job );
        }
        else
        {
            command->execute();
        }
    }

    void CommandManagerMT::removeCommand( SmartPtr<ICommand> command )
    {
        ScopedLock lock( this );

        m_commands.erase( std::remove( m_commands.begin(), m_commands.end(), command ),
                          m_commands.end() );
    }

    auto CommandManagerMT::hasCommand( SmartPtr<ICommand> command ) -> bool
    {
        ScopedLock lock( this );

        auto it = std::find( m_commands.begin(), m_commands.end(), command );
        if( it != m_commands.end() )
        {
            return true;
        }

        return false;
    }

    auto CommandManagerMT::isCommandQueued( SmartPtr<ICommand> command ) -> bool
    {
        ScopedLock lock( this );

        auto it = std::find( m_commands.begin(), m_commands.end(), command );
        if( it != m_commands.end() )
        {
            if( it - m_commands.begin() > m_currentCommand )
            {
                return true;
            }
        }

        return false;
    }

    auto CommandManagerMT::getNextCommand() -> SmartPtr<ICommand>
    {
        ScopedLock lock( this );

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        SmartPtr<ICommand> pCommand;
        s32 command = -1;

        // in case of an Add operation we don't redo
        if( m_lastCommand == CommandType::COMMAND_MANAGER_ADD )
        {
            return pCommand;
        }

        // read first the command then modify the index
        if( m_lastCommand != CommandType::COMMAND_MANAGER_UNDO || m_lastUndoCommand == 0 )
        {
            pCommand = m_commands[m_currentCommand];
            command = m_currentCommand;
        }

        ++m_currentCommand;

        auto nextCommandIdx = static_cast<s32>( m_commands.size() ) - m_currentCommand;
        auto prevCommandIdx = static_cast<s32>( m_commands.size() ) -
                              ( static_cast<s32>( m_commands.size() ) - m_currentCommand );

        auto args = Array<Parameter>();
        args.emplace_back( nextCommandIdx );
        args.emplace_back( prevCommandIdx );
        applicationManager->triggerEvent( EventType::Application, IEvent::getNextCommand, args, this,
                                          nullptr, nullptr );

        if( m_currentCommand > static_cast<s32>( m_commands.size() ) - 1 )
        {
            m_currentCommand = static_cast<s32>( m_commands.size() ) - 1;

            if( ( m_lastCommand == CommandType::COMMAND_MANAGER_REDO ||
                  m_lastCommand == CommandType::COMMAND_MANAGER_ADD ) &&
                m_lastRedoCommand == m_currentCommand )
            {
                return nullptr;
            }
        }

        // read the command after modify the index
        if( m_lastCommand == CommandType::COMMAND_MANAGER_UNDO && m_lastUndoCommand != 0 )
        {
            if( m_currentCommand < m_commands.size() )
            {
                pCommand = m_commands[m_currentCommand];
                command = m_currentCommand;
            }

            if( m_currentCommand < static_cast<s32>( m_commands.size() ) - 1 )
            {
                m_currentCommand++;
            }
        }

        m_lastCommand = CommandType::COMMAND_MANAGER_REDO;

        m_lastRedoCommand = command;

        return pCommand;
    }

    auto CommandManagerMT::getPreviousCommand() -> SmartPtr<ICommand>
    {
        ScopedLock lock( this );

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        SmartPtr<ICommand> pCommand;
        s32 command = -1;

        // read first the command then modify the index
        if( m_lastCommand != CommandType::COMMAND_MANAGER_REDO ||
            m_lastRedoCommand == static_cast<s32>( m_commands.size() ) - 1 )
        {
            if( m_currentCommand < m_commands.size() )
            {
                pCommand = m_commands[m_currentCommand];
                command = m_currentCommand;
            }
        }

        auto nextCommandIdx = static_cast<s32>( m_commands.size() ) - m_currentCommand;
        auto prevCommandIdx = static_cast<s32>( m_commands.size() ) -
                              ( static_cast<s32>( m_commands.size() ) - m_currentCommand );

        auto args = Array<Parameter>();
        args.emplace_back( nextCommandIdx );
        args.emplace_back( prevCommandIdx );
        applicationManager->triggerEvent( EventType::Application, IEvent::getPreviousCommand, args, this,
                                          nullptr, nullptr );

        --m_currentCommand;

        if( m_currentCommand < 0 )
        {
            m_currentCommand = 0;
            if( m_lastCommand == CommandType::COMMAND_MANAGER_UNDO &&
                m_lastUndoCommand == m_currentCommand )
            {
                return pCommand;
            }
        }

        // read the command after modify the index
        if( m_lastCommand == CommandType::COMMAND_MANAGER_REDO &&
            m_lastRedoCommand != static_cast<s32>( m_commands.size() ) - 1 )
        {
            if( m_currentCommand < m_commands.size() )
            {
                pCommand = m_commands[m_currentCommand];
                command = m_currentCommand;
            }
        }

        m_lastCommand = CommandType::COMMAND_MANAGER_UNDO;

        m_lastUndoCommand = command;

        return pCommand;
    }

    void CommandManagerMT::clearAll()
    {
        ScopedLock lock( this );
        m_commands.clear();
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
