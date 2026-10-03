#ifndef __WP_TaskManager_h__
#define __WP_TaskManager_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/Interface/System/IFSMManager.hpp>
#include <Workphone/Interface/System/IFSM.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Atomics/AtomicValue.hpp>
#include <Workphone/Core/FixedArray.hpp>
#include <Workphone/System/Task.hpp>

namespace workphone
{

    /** Default task manager implementation.  */
    class WPCore_API TaskManager : public ITaskManager
    {
    public:
        /* Used to lock all the tasks. */
        class Lock : public ISharedObject
        {
        public:
            Lock();
            explicit Lock( SmartPtr<ITaskManager> taskManager );
            ~Lock() override;

            SmartPtr<ITaskManager> m_taskManager;
        };

        /** Constructor */
        TaskManager();

        /** Destructor */
        ~TaskManager() override;

        /** @copydoc ISharedObject::load */
        void load( SmartPtr<ISharedObject> data ) override;

        /** @copydoc ISharedObject::reload */
        void reload( SmartPtr<ISharedObject> data ) override;

        /** @copydoc ISharedObject::unload */
        void unload( SmartPtr<ISharedObject> data ) override;

        /** @copydoc ISharedObject::update */
        void update() override;

        void addJobAllTasks( SmartPtr<IJob> job ) override;

        /** @copydoc ITaskManager::clearEventJobs */
        void clearEventJobs() override;

        Array<SmartPtr<ITask>> getTasks() const override;

        void wait() override;
        void stop() override;
        void reset() override;

        void shutdown() override;

        TaskLock lockTask( TaskId taskId ) override;

        u32 getNumTasks() const override;

        SmartPtr<ITask> getTask( TaskId taskId ) const override;
        ITask *getTaskPtr( TaskId taskId ) const override;

        void setState( State state ) override;
        State getState() const override;

        /** @copydoc ITaskManager::setFlags */
        void setFlags( u32 id, u32 flag, bool value );

        /** @copydoc ITaskManager::getFlags */
        bool getFlags( u32 id, u32 flag ) const;

        atomic_u32 *getFlagsPtr( u32 id ) const;

        /** @copydoc ITask::setState */
        void setState( u32 id, ITask::State state );

        /** @copydoc ITask::getState */
        ITask::State getState( u32 id ) const;

        /** @copydoc TaskManager::getTaskId */
        TaskId getTaskId( u32 id ) const;

        /** @copydoc TaskManager::setTaskId */
        void setTaskId( u32 id, TaskId task );

        atomic_s32 *getAffinityPtr( u32 id );

        u32 getAffinity( u32 id ) const;

        void setAffinity( u32 id, u32 affinity );

        atomic_s32 *getStoppedPtr( u32 id );

        atomic_u32 getTaskFlagsPtr( u32 id );

        u32 getTaskFlags( u32 id ) const;

        void setTaskFlags( u32 id, u32 taskFlags );

        atomic_u32 *getThreadTaskFlagsPtr( u32 id );

        atomic_f64 *getTargetFPSPtr( u32 id );

        f64 getTargetFPS( u32 id ) const;

        void setTargetFPS( u32 id, f64 targetfps );

        atomic_f64 *getAutoFPSPtr( u32 id );

        atomic_f64 *getNextUpdateTimePtr( u32 id );

        f64 getNextUpdateTime( u32 id ) const;

        void setNextUpdateTime( u32 id, f64 nextUpdateTime );

        atomic_u32 *getTickCountPtr( u32 id );

        TaskId *getTaskIdsPtr( u32 id );

        TaskId getTaskIds( u32 id ) const;

        void setTaskIds( u32 id, TaskId taskId );

        ITask::State *getTaskStatesPtr( u32 id );

        ITask::State getTaskState( u32 id ) const;

        void setTaskState( u32 id, ITask::State state );

        bool isValid() const override;

        SmartPtr<IFSMManager> getFSMManager() const;
        void setFSMManager( SmartPtr<IFSMManager> fsmManager );

        IFSM *getFsmPtr() const;

        SmartPtr<IFSM> getFsm() const;

        void setFsm( SmartPtr<IFSM> fsm );

        FSMReturnType handleEvent( u32 state, FSMEvent eventType );

        WP_CLASS_REGISTER_DECL;

    private:
        void calculateTaskAffinity();

        AtomicSmartPtr<IFSM> m_fsm;
        AtomicSmartPtr<IFSMManager> m_fsmManager;

        AtomicValue<State> m_state = State::None;

        constexpr static u32 maxTasks = static_cast<u32>( TaskId::Count );

        FixedArray<SmartPtr<Task>, maxTasks> m_tasks;
        FixedArray<atomic_s32, maxTasks> m_affinity;
        FixedArray<atomic_s32, maxTasks> m_stopped;
        FixedArray<atomic_u32, maxTasks> m_taskFlags;
        FixedArray<atomic_u32, maxTasks> m_threadTaskFlags;
        FixedArray<atomic_f64, maxTasks> m_targetfps;
        FixedArray<atomic_f64, maxTasks> m_autoFPS;
        FixedArray<atomic_f64, maxTasks> m_nextUpdateTimes;
        FixedArray<atomic_u32, maxTasks> m_tickCount;
        FixedArray<TaskId, maxTasks> m_taskIds;
        FixedArray<ITask::State, maxTasks> m_states;
        FixedArray<atomic_s32, maxTasks> m_threadHint;

        atomic_u32 m_idCount;

        static u32 m_idExt;
    };

    inline IFSM *TaskManager::getFsmPtr() const
    {
        return m_fsm.get();
    }

}  // namespace workphone

#endif  // __WP_TaskManager_h__
