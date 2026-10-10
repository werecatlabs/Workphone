#ifndef CommandManagerMT_H
#define CommandManagerMT_H

#include <Workphone/Interface/System/ICommandManager.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Thread/RecursiveMutex.hpp>
#include <utility>

namespace workphone
{

    /**
     * @brief Thread-safe, multi-threaded implementation of ICommandManager.
     *
     * CommandManagerMT stores a sequence of commands and provides operations for
     * adding, removing, querying and navigating commands (next/previous). All
     * public operations are guarded by an internal recursive mutex to allow safe
     * usage from multiple threads. External callers may also acquire the same
     * lock using `lock()`, `try_lock()` and `unlock()` for batching multiple
     * operations atomically.
     *
     * @note This class assumes ownership semantics are supplied by `SmartPtr`.
     */
    class WPCore_API CommandManagerMT : public ICommandManager
    {
    public:
        /**
         * @brief Construct a new CommandManagerMT.
         *
         * Initializes internal indices and containers used to manage the command
         * history.
         */
        CommandManagerMT();

        /**
         * @brief Destroy the CommandManagerMT.
         *
         * Ensures any resources are released. Commands held in the internal array
         * are released via their SmartPtr destructors.
         */
        ~CommandManagerMT() override;

        /**
         * @brief Load state or configuration into the command manager.
         *
         * @param data Optional shared object providing initialization data.
         *
         * @copydetails ICommandManager::load
         */
        void load( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Unload or clear state associated with the command manager.
         *
         * @param data Optional shared object provided during unload.
         *
         * @copydetails ICommandManager::unload
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Add a command to the manager.
         *
         * The command is appended to the internal command history. If callers
         * need to add multiple commands atomically, acquire the manager lock
         * first with `lock()`.
         *
         * @param command A smart pointer to the ICommand to add.
         *
         * @copydetails ICommandManager::addCommand
         */
        void addCommand( SmartPtr<ICommand> command ) override;

        /**
         * @brief Remove a specific command from the manager.
         *
         * If the command is present multiple times, behavior depends on the
         * internal implementation (typically removes the first matching entry).
         *
         * @param command The command to remove.
         *
         * @copydetails ICommandManager::removeCommand
         */
        void removeCommand( SmartPtr<ICommand> command ) override;

        /**
         * @brief Check whether the specified command exists in the manager.
         *
         * @param command The command to search for.
         * @return true if the command exists, false otherwise.
         *
         * @copydetails ICommandManager::hasCommand
         */
        bool hasCommand( SmartPtr<ICommand> command ) override;

        /**
         * @brief Determine whether the specified command is currently queued.
         *
         * A queued command is one that is present in the pending list of commands
         * managed by this object.
         *
         * @param command The command to check.
         * @return true if the command is queued, false otherwise.
         *
         * @copydetails ICommandManager::isCommandQueued
         */
        bool isCommandQueued( SmartPtr<ICommand> command ) override;

        /**
         * @brief Retrieve the next command in the sequence relative to the
         * current index.
         *
         * Advances the internal current index when a next command is successfully
         * retrieved.
         *
         * @return SmartPtr<ICommand> The next command or a null SmartPtr if none.
         *
         * @copydetails ICommandManager::getNextCommand
         */
        SmartPtr<ICommand> getNextCommand() override;

        /**
         * @brief Retrieve the previous command in the sequence relative to the
         * current index.
         *
         * Moves the internal current index backwards when a previous command is
         * successfully retrieved.
         *
         * @return SmartPtr<ICommand> The previous command or a null SmartPtr if none.
         *
         * @copydetails ICommandManager::getPreviousCommand
         */
        SmartPtr<ICommand> getPreviousCommand() override;

        /**
         * @brief Remove all stored commands and reset internal indices.
         *
         * After calling this method the command history will be empty and the
         * current/undo/redo indices reset.
         *
         * @copydetails ICommandManager::clearAll
         */
        void clearAll() override;

        /**
         * @brief Acquire the internal recursive mutex.
         *
         * Use this when batching multiple operations to avoid repetitive lock/unlock
         * overhead and to ensure atomicity across several calls.
         */
        void lock() override;

        /**
         * @brief Attempt to acquire the internal recursive mutex without blocking.
         *
         * @return true if the lock was acquired, false otherwise.
         */
        bool try_lock() override;

        /**
         * @brief Release the internal recursive mutex.
         *
         * This should be called to match a prior `lock()` or successful `try_lock()`.
         */
        void unlock() override;

    private:
        void invalidatePending( SmartPtr<ICommand> command );

        /** Internal recursive mutex used to ensure thread safety for all operations. */
        RecursiveMutex m_mutex;

        /** Number of applied commands; [0,size] separates undo and redo history. */
        s32 m_currentCommand;

        /** Index of the last undoable command. */
        s32 m_lastUndoCommand;

        /** Index of the last redoable command. */
        s32 m_lastRedoCommand;

        /** Maximum retained history length. */
        u32 m_numStoredCommands;

        /** Container holding the sequence of commands managed by this object. */
        Array<SmartPtr<ICommand>> m_commands;

        /** Queued job identities; removing an entry cancels its future execution. */
        Array<std::pair<SmartPtr<ICommand>, SmartPtr<IJob>>> m_pendingJobs;

        /** Started work finishes outside the manager lock; unload is deferred for it. */
        Array<SmartPtr<ICommand>> m_executingCommands;
        Array<SmartPtr<ICommand>> m_deferredUnloads;

        /** The type of the last command executed. Defaults to none. */
        CommandType m_lastCommand = CommandType::COMMAND_MANAGER_NONE;
    };
}  // namespace workphone

#endif
