#ifndef __CJob_h__
#define __CJob_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/System/IJob.hpp>
#include <Workphone/Memory/CoroutineData.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Atomics/AtomicValue.hpp>

namespace workphone
{

    /** Base class for a job class. */
    class WPCore_API Job : public IJob
    {
    public:
        /** Constructor. */
        Job();

        /** Destructor. */
        ~Job() override;

        /** @copydoc IJob::getState */
        State getState() const override;

        /** @copydoc IJob::setState */
        void setState( State state ) override;

        /** @copydoc IJob::getProgress */
        u32 getProgress() const override;

        /** @copydoc IJob::setProgress */
        void setProgress( u32 progress ) override;

        /** @copydoc IJob::getPriority */
        s32 getPriority() const override;

        /** @copydoc IJob::setPriority */
        void setPriority( s32 priority ) override;

        /** @copydoc IJob::isPrimary */
        bool isPrimary() const override;

        /** @copydoc IJob::setPrimary */
        void setPrimary( bool primary ) override;

        /** @copydoc IJob::isFinished */
        bool isFinished() const override;

        /** @copydoc IJob::setInterrupted */
        void setInterrupted( bool interrupted ) override;

        /** @copydoc IJob::isInterrupted */
        bool isInterrupted() const override;

        /** @copydoc IJob::stop */
        void stop() override;

        /** @copydoc IJob::wait */
        bool wait() override;

        /** @copydoc IJob::wait */
        bool wait( f64 maxWaitTime ) override;

        /** @copydoc IJob::getAffinity */
        s32 getAffinity() const override;

        /** @copydoc IJob::setAffinity */
        void setAffinity( s32 affinity ) override;

        /** @copydoc IJob::execute */
        void execute() override;

        /** @copydoc IJob::coroutine_execute */
        void coroutine_execute() override;

        /** @copydoc IJob::coroutine_execute_step */
        void coroutine_execute_step( SmartPtr<ICoroutineData> &yield ) override;

        /** @copydoc IJob::isCoroutine */
        bool isCoroutine() const override;

        /** @copydoc IJob::setCoroutine */
        void setCoroutine( bool coroutine ) override;

        /** @copydoc IJob::setCallbackFunction */
        void setCallbackFunction( std::function<void( int )> callbackFunction ) override;

        Parameter handleEvent( EventType eventType, hash_type eventValue,
                               const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                               SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        // The job affinity.
        atomic_u32 m_affinity = 0;

        // The job state.
        AtomicValue<State> m_state = State::Ready;

        // The job progress.
        atomic_u32 m_progress = 0;

        // The job priority.
        atomic_u32 m_priority = 0;

        // To know if the job is primary.
        atomic_bool m_isPrimary = false;

        // To know if the job is coroutine.
        atomic_bool m_isCoroutine = false;

        // To know if the job has been interrupted.
        atomic_bool m_interrupted = false;

        // Object for coroutine data.
        SmartPtr<ICoroutineData> m_yieldObject;

        // Callback function.
        std::function<void( int )> m_callbackFunction;
    };
}  // namespace workphone

#endif  // JobBase_h__
