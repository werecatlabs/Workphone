#ifndef __FBThreadTypes_H__
#define __FBThreadTypes_H__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/WorkphoneEnums.hpp>
#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Core/FixedArray.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Thread/SpinRWMutex.hpp>

namespace workphone
{

    /**
     * @brief The id of the current task that is being processed.
     */
    extern WP_THREAD_LOCAL_STORAGE s32 CURRENT_TASK_ID;

    /**
     * @brief An engine id of the thread that is currently executing.
     */
    extern WP_THREAD_LOCAL_STORAGE u32 CURRENT_THREAD_ID;

    /**
     * @brief Flags associated with the current executing thread.
     */
    extern WP_THREAD_LOCAL_STORAGE u32 CURRENT_THREAD_FLAGS;

    /**
     * @brief The id of the current update event.
     */
    extern WP_THREAD_LOCAL_STORAGE u32 CURRENT_UPDATE_EVENT;

    /**
     * @brief A class to provide system thread functions.
     */
    class WPCore_API Thread
    {
    public:
        /**
         * @brief Thread IDs used within the Workphone API.
         */
        enum class ThreadId
        {
            Primary = -1,
            WorkerThread = 0,
            WorkerThreadMax = WorkerThread + 50,

            Count
        };

        /**
         * @brief Different states during the update process.
         */
        enum class UpdateState
        {
            PreUpdate,
            Update,
            PostUpdate,
            Transform,

            Count
        };

        static const u32 Primary_Flag;
        static const u32 Ai_Flag;
        static const u32 Animation_Flag;
        static const u32 Application_Flag;
        static const u32 Collision_Flag;
        static const u32 Controls_Flag;
        static const u32 Dynamics_Flag;
        static const u32 GarbageCollect_Flag;
        static const u32 Input_Flag;
        static const u32 Physics_Flag;
        static const u32 None_Flag;
        static const u32 Render_Flag;
        static const u32 Sound_Flag;

        /**
         * @brief Get the current task being executed by the thread.
         * @return The current task.
         */
        static TaskId getCurrentTask();

        /**
         * @brief Set the current task for the thread.
         * @param task The task to set.
         */
        static void setCurrentTask( TaskId task );

        /**
         * @brief Get the current update state.
         * @return The current update state.
         */
        static u32 getTaskFlags();

        /**
         * @brief Set the current update state.
         * @param taskFlags The update state to set.
         */
        static void setTaskFlags( u32 taskFlags );

        /**
         * @brief Get the current update event.
         * @return The current update event.
         */
        static u32 getTaskFlags( TaskId task );

        /**
         * @brief Set the current update event.
         * @param task The task to set the update event for.
         * @param taskFlags The update event to set.
         */
        static void setTaskFlags( TaskId task, u32 taskFlags );

        /**
         * @brief Get the flag for a specific task.
         * @param flag The flag to get.
         * @return The flag value.
         */
        static bool getTaskFlag( u32 flag );

        /**
         * @brief Set the flag for a specific task.
         * @param flag The flag to set.
         * @param value The value to set the flag to.
         */
        static void setTaskFlag( u32 flag, bool value );

        /**
         * @brief Get the ID of the current executing thread.
         * @return The thread ID.
         */
        static ThreadId getCurrentThreadId();

        /**
         * @brief Set the ID of the current executing thread.
         * @param threadId The thread ID to set.
         */
        static void setCurrentThreadId( ThreadId threadId );

        /**
         * @brief Get the number of hardware threads available on the system.
         * @return The number of hardware threads.
         */
        static u32 hardware_concurrency();

        /**
         * @brief Get the number of physical threads available on the system.
         * @return The number of physical threads.
         */
        static u32 physical_concurrency();

        /**
         * @brief Make the current thread sleep for the specified duration.
         * @param seconds The duration to sleep in seconds.
         */
        static void sleep( time_interval seconds );

        /**
         * @brief Yield the current thread's execution to allow other threads to run.
         */
        static void yield();

        /**
         * @brief Atomically exchange the value of a pointer.
         * @param target The target pointer to exchange.
         * @param value The value to set.
         */
        static void interlockedExchangePointer( void **target, void *value );

        /**
         * @brief Atomically exchange the value of a variable.
         * @param target The target variable to exchange.
         * @param value The value to set.
         */
        static void interlockedExchange( volatile long *target, long value );

        /**
         * @brief Increment the value of a variable atomically.
         * @param value The variable to increment.
         * @return The new value of the variable after incrementing.
         */
        static long interlockedIncrement( volatile long *value );

        /**
         * @brief Decrement the value of a variable atomically.
         * @param value The variable to decrement.
         * @return The new value of the variable after decrementing.
         */
        static long interlockedDecrement( volatile long *value );

        /**
         * @brief Atomically compare the value of a variable with an expected value, and if they are
         * equal, replace the value.
         * @param a The target variable.
         * @param b The expected value.
         * @param c The new value to set if the comparison is successful.
         * @return The original value of the variable before the comparison.
         */
        static long interlockedCompareExchange( volatile long *a, long b, long c );

        /**
         * @brief Get the name associated with a specific task ID.
         * @param id The task ID.
         * @return The name of the task.
         */
        static String getTaskName( TaskId id );

    private:
        /** The task flags for each task. */
        static FixedArray<u32, static_cast<u32>( TaskId::Count )> m_taskFlags;

        /** The mutex to protect the task flags. */
        static SpinRWMutex m_taskFlagsMutex;
    };
}  // namespace workphone

#endif
