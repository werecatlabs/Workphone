#ifndef ThreadPoolStandard_h__
#define ThreadPoolStandard_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/System/IThreadPool.hpp>
#include <Workphone/Interface/System/IWorkerThread.hpp>
#include <Workphone/System/FSMListener.hpp>
#include <Workphone/Core/FixedArray.hpp>
#include <Workphone/Thread/RecursiveMutex.hpp>

namespace workphone
{

    /** Standard thread pool implementation. */
    class WPCore_API ThreadPool : public IThreadPool
    {
    public:
        /** Constructor */
        ThreadPool();

        /** Destructor */
        ~ThreadPool() override;

        /** @copydoc IThreadPool::load */
        void load( SmartPtr<ISharedObject> data ) override;

        /** @copydoc IThreadPool::unload */
        void unload( SmartPtr<ISharedObject> data ) override;

        /** @copydoc ThreadPool::addWorkerThread */
        SmartPtr<IWorkerThread> addWorkerThread() override;

        /** @copydoc ThreadPool::getThread */
        SmartPtr<IWorkerThread> getThread( u32 index ) override;

        /** @copydoc ThreadPool::getNumThreads */
        u32 getNumThreads() const override;

        /** @copydoc ThreadPool::setNumThreads */
        void setNumThreads( u32 numThreads ) override;

        /** @copydoc ThreadPool::getState */
        State getState() const override;

        /** @copydoc ThreadPool::setState */
        void setState( State state ) override;

        /** @copydoc ThreadPool::stop */
        void stop() override;

        /** @copydoc ThreadPool::isValid */
        bool isValid() const override;

        WP_CLASS_REGISTER_DECL;

    private:
        class ThreadPoolFSMListener : public FSMListener
        {
        public:
            ThreadPoolFSMListener();
            ~ThreadPoolFSMListener() override;

            FSMReturnType handleEvent( u32 state, FSMEvent eventType ) override;

            SmartPtr<ThreadPool> getOwner() const;
            void setOwner( SmartPtr<ThreadPool> owner );

        private:
            AtomicWeakPtr<ThreadPool> m_owner;
        };

        SmartPtr<IFSM> getFSM() const;
        void setFSM( SmartPtr<IFSM> fsm );

        SmartPtr<IFSMListener> getFSMListener() const;
        void setFSMListener( SmartPtr<IFSMListener> listener );

        FSMReturnType handleEvent( u32 state, FSMEvent eventType );

        AtomicSmartPtr<IFSM> m_fsm;
        AtomicSmartPtr<IFSMListener> m_fsmListener;

        atomic_u32 m_numThreads = 0;

        Array<SmartPtr<IWorkerThread>> m_workerThreads;
        Array<IWorkerThread::State> m_states;
        Array<std::thread *> m_threads;
        Array<f64> m_targetFPS;
        Array<Thread::ThreadId> m_threadId;
        Array<u32> m_queueLengthMilliseconds;
        Array<u32> m_reserveFlags;

        mutable RecursiveMutex m_mutex;
    };
}  // namespace workphone

#endif  // ThreadPoolStandard_h__
