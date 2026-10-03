#ifndef _ICOMMAND_H
#define _ICOMMAND_H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /**
     * @brief Interface for a command, providing support for the undo/redo functionality.
     * Inherits from ISharedObject.
     */
    class WPCore_API ICommand : public ISharedObject
    {
    public:
        enum class State
        {
            Allocated,
            Queued,
            Executing,
            Finished,

            Count
        };

        /**
         * @brief Destroy the ICommand object.
         */
        ~ICommand() override;

        /**
         * @brief Undo the previously executed command.
         */
        virtual void undo() = 0;

        /**
         * @brief Redo the previously undone command.
         */
        virtual void redo() = 0;

        /**
         * @brief Execute the command.
         */
        virtual void execute() = 0;

        /**
         * @brief Get the state of the command.
         * @return State The state of the command.
         */
        virtual State getState() const = 0;

        /**
         * @brief Set the state of the command.
         * @param state The state of the command.
         */
        virtual void setState( State state ) = 0;

        /** Used to know if to run on the primary thread.
         * @return bool The value.
         */
        virtual bool isPrimary() const = 0;

        /** Used to know if to run on the primary thread.
         * @param primary The value to set.
         */
        virtual void setPrimary( bool primary ) = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif
