#ifndef BaseState_h__
#define BaseState_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/System/IState.hpp>
#include <Workphone/Atomics/AtomicFloat.hpp>
#include <Workphone/Memory/AtomicSmartPtr.hpp>
#include <Workphone/Memory/AtomicWeakPtr.hpp>

namespace workphone
{

    /** Base class for state objects. */
    class WPCore_API State : public IState
    {
    public:
        static const String timeStr;

        /** Constructor. */
        State();

        State( u32 poolTypeId );

        /** Destructor. */
        ~State() override;

        /** @copydoc IState::load */
        void unload( SmartPtr<ISharedObject> data ) override;

        /** @copydoc IState::getTime */
        time_interval getTime() const override;

        /** @copydoc IState::setTime */
        void setTime( time_interval time ) override;

        /** @copydoc IState::isDirty */
        bool isDirty() const override;

        /** @copydoc IState::setDirty */
        void setDirty( bool dirty ) override;

        /** @copydoc State::getOwner */
        SmartPtr<ISharedObject> getOwner() const override;

        /** @copydoc IState::setOwner */
        void setOwner( SmartPtr<ISharedObject> owner ) override;

        /** @copydoc IState::getStateContext */
        SmartPtr<IStateContext> getStateContext() const override;

        /** @copydoc IState::setStateContext */
        void setStateContext( SmartPtr<IStateContext> stateContext ) override;

        /** @copydoc IState::getProperties */
        SmartPtr<Properties> getProperties() const override;

        /** @copydoc IState::setProperties */
        void setProperties( SmartPtr<Properties> properties ) override;

        /** @copydoc IState::clone */
        SmartPtr<IState> clone() const override;

        /** @copydoc IState::assign */
        void assign( SmartPtr<IState> state ) override;

        /** @copydoc IState::getData */
        SmartPtr<ISharedObject> getData() const override;

        /** @copydoc IState::setData */
        void setData( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Atomically increment the send counter.
         *
         * Indicates the message has been queued or dispatched one additional
         * time.
         */
        void addSendCount() override;

        /**
         * @brief Atomically decrement the send counter.
         *
         * Indicates one fewer active/queued send. Behavior when the counter
         * is already zero is implementation-defined.
         */
        void removeSendCount() override;

        /**
         * @brief Retrieve the current send counter value.
         *
         * @return Number of times this message has been sent/queued.
         */
        u32 getSendCount() const override;

        /**
         * @brief Explicitly set the send counter value.
         *
         * @param sendCount New value for the send counter.
         */
        void setSendCount( u32 sendCount ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /** Makes a clone of the state object. */
        void makeClone( SmartPtr<State> state ) const;

        /// The state mutex.
        mutable SpinRWMutex m_mutex;

        /// The update time of the state.
        atomic_f64 m_updateTime = 0.0;

        /// The dirty time of the state.
        atomic_f64 m_dirtyTime = 0.0;

        /**
         * @brief Atomic counter tracking how many times this message has been
         * queued or dispatched.
         */
        atomic_u32 m_sendCount = 0;
    };

}  // namespace workphone

#endif  // BaseState_h__
