#ifndef CommandManagerStandard_H
#define CommandManagerStandard_H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/System/ICommandManager.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{

    /** A single threaded implementation of the command manager.
    Runs the commands from the primary thread. */
    class WPCore_API CommandManager : public ICommandManager
    {
    public:
        /** Constructor. */
        CommandManager();

        /** Destructor. */
        ~CommandManager() override;

        /** @copydoc ICommandManager::addCommand */
        void addCommand( SmartPtr<ICommand> command ) override;

        /** @copydoc ICommandManager::removeCommand */
        void removeCommand( SmartPtr<ICommand> command ) override;

        /** @copydoc ICommandManager::hasCommand */
        bool hasCommand( SmartPtr<ICommand> command ) override;

        /** @copydoc ICommandManager::isCommandQueued */
        bool isCommandQueued( SmartPtr<ICommand> command ) override;

        /** @copydoc ICommandManager::getNextCommand */
        SmartPtr<ICommand> getNextCommand() override;

        /** @copydoc ICommandManager::getPreviousCommand */
        SmartPtr<ICommand> getPreviousCommand() override;

        /** @copydoc ICommandManager::clearAll */
        void clearAll() override;

    private:
        /** Number of applied commands; [0,size] separates undo and redo history. */
        s32 m_currentCommand;

        /** The index of the last undo command. */
        s32 m_lastUndoCommand;

        /** The index of the last redo command. */
        s32 m_lastRedoCommand;

        /** The number of stored commands. */
        u32 m_numStoredCommands;

        /** The array of stored commands. */
        Array<SmartPtr<ICommand>> m_commands;

        /** The last command executed. */
        CommandType m_lastCommand = CommandType::COMMAND_MANAGER_NONE;
    };
}  // namespace workphone

#endif
