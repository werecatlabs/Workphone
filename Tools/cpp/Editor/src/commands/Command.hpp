#ifndef Command_h__
#define Command_h__

#include <EditorPrerequisites.hpp>
#include <Workphone/Interface/System/ICommand.hpp>
#include <Workphone/Thread/RecursiveMutex.hpp>

namespace workphone
{
    namespace editor
    {
        /**
         * @class Command
         * @brief Base class for all commands.
         */
        class Command : public ICommand
        {
        public:
            /**
             * @brief Constructor.
             */
            Command();

            /**
             * @brief Destructor.
             */
            ~Command() override;

            /**
             * @brief Undo the command.
             */
            void undo() override;

            /**
             * @brief Redo the command.
             */
            void redo() override;

            /**
             * @brief Execute the command.
             */
            void execute() override;

            /**
             * @brief Get the state of the command.
             * @return The state of the command.
             */
            State getState() const override;

            /**
             * @brief Set the state of the command.
             * @param state The state of the command.
             */
            void setState( State state ) override;

            /** Used to know if to run on the primary thread.
             * @return bool The value.
             */
            bool isPrimary() const override;

            /** Used to know if to run on the primary thread.
             * @param primary The value to set.
             */
            void setPrimary( bool primary ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /** Mutex for thread safety. */
            mutable RecursiveMutex m_mutex;

            /** The state of the command. */
            State m_commandState = State::Allocated;

            bool m_isPrimary = false;
        };
    }  // namespace editor
}  // namespace workphone

#endif  // Command_h__
