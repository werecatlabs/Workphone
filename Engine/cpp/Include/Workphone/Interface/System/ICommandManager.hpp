#ifndef ICommandManager_H
#define ICommandManager_H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /** Interface for a command manager class.
     */
    class WPCore_API ICommandManager : public ISharedObject
    {
    public:
        enum class CommandType
        {
            COMMAND_MANAGER_NONE,
            COMMAND_MANAGER_UNDO,
            COMMAND_MANAGER_REDO,
            COMMAND_MANAGER_ADD,
            Count
        };

        /** Virtual destructor. */
        ~ICommandManager() override;

        /** Adds a command to the command history.
        @param command The command instance.
        */
        virtual void addCommand( SmartPtr<ICommand> command ) = 0;

        /** Removes a command from the command history. */
        virtual void removeCommand( SmartPtr<ICommand> command ) = 0;

        /** Checks if the command is in the command history. */
        virtual bool hasCommand( SmartPtr<ICommand> command ) = 0;

        /** Checks if the command is queued in the command history. */
        virtual bool isCommandQueued( SmartPtr<ICommand> command ) = 0;

        /** Retrieves the next command returns null=0 if
        there is not a next command.
        */
        virtual SmartPtr<ICommand> getNextCommand() = 0;

        /** Retrieves the previous command returns null=0 if
        there is not a next command.
        */
        virtual SmartPtr<ICommand> getPreviousCommand() = 0;

        /** Clears all the commands. */
        virtual void clearAll() = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif
