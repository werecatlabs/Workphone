#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/CommandManager.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Interface/System/ICommand.hpp>
#include <algorithm>

namespace workphone
{
    CommandManager::CommandManager() :
        m_currentCommand( 0 ),
        m_lastUndoCommand( -1 ),
        m_lastRedoCommand( -1 ),
        m_numStoredCommands( 100 )
    {
        setName( "CommandManager" );
        m_commands.reserve( m_numStoredCommands );
    }
    CommandManager::~CommandManager() = default;

    void CommandManager::addCommand( SmartPtr<ICommand> command )
    {
        if( !command )
            return;
        // Cursor is the number of applied commands. A new branch discards all redo work.
        m_commands.erase( m_commands.begin() + m_currentCommand, m_commands.end() );
        while( m_commands.size() >= m_numStoredCommands )
            m_commands.erase( m_commands.begin() );
        m_commands.push_back( command );
        m_currentCommand = static_cast<s32>( m_commands.size() );
        m_lastCommand = CommandType::COMMAND_MANAGER_ADD;
        auto application = core::IApplicationManager::instancePtr();
        if( application )
        {
            Array<Parameter> args;
            args.emplace_back( m_currentCommand );
            application->triggerEvent( EventType::Application, IEvent::addCommand, args, this, nullptr,
                                       nullptr );
        }
        if( command->getState() != ICommand::State::Finished )
            command->execute();
    }
    void CommandManager::removeCommand( SmartPtr<ICommand> command )
    {
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
    bool CommandManager::hasCommand( SmartPtr<ICommand> command )
    {
        return std::find( m_commands.begin(), m_commands.end(), command ) != m_commands.end();
    }
    bool CommandManager::isCommandQueued( SmartPtr<ICommand> command )
    {
        const auto found = std::find( m_commands.begin(), m_commands.end(), command );
        return found != m_commands.end() && found - m_commands.begin() >= m_currentCommand;
    }
    SmartPtr<ICommand> CommandManager::getNextCommand()
    {
        if( m_currentCommand >= static_cast<s32>( m_commands.size() ) )
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
    SmartPtr<ICommand> CommandManager::getPreviousCommand()
    {
        if( m_currentCommand <= 0 )
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
    void CommandManager::clearAll()
    {
        m_commands.clear();
        m_currentCommand = 0;
        m_lastUndoCommand = m_lastRedoCommand = -1;
        m_lastCommand = CommandType::COMMAND_MANAGER_NONE;
    }
}  // namespace workphone
