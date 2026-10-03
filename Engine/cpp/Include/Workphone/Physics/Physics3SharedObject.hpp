#ifndef WP_Physics3SharedObject_h__
#define WP_Physics3SharedObject_h__

#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Physics/IPhysicsManager.hpp>
#include <Workphone/Interface/System/IStateManager.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Interface/System/IState.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Base mix-in for physics-related shared objects.
         *
         * This template class augments any class `T` with common physics-subsystem
         * state management and synchronization helpers:
         *  - holds an (atomic) weak reference to an ::IStateContext for thread-safe
         *    lifetime checks and safe access,
         *  - holds an atomic strong reference to an ::IStateListener,
         *  - provides helpers to enqueue state messages to the physics state task,
         *  - forwards lock/try_lock/unlock to the global physics manager.
         *
         * Template parameter T is expected to provide a static `getLoadingState()`
         * that returns a value comparable to `LoadingState::Loaded`.
         *
         * The class is intended to be used as a CRTP-like mixin derived from the
         * concrete shared object implementation type.
         *
         * @tparam T Underlying shared object class being extended.
         */
        template <typename T>
        class Physics3SharedObject : public T
        {
        public:
            /** @name Lifecycle
             *  Construction / destruction.
             */
            ///@{
            Physics3SharedObject();
            ~Physics3SharedObject();
            ///@}

            /**
             * @brief Called when the shared object is unloaded.
             *
             * This performs cleanup of any attached state context and state listener.
             * Implementations should call the base `unload` when overriding.
             *
             * @param data Optional unload data; currently ignored by the base.
             */
            void unload( SmartPtr<ISharedObject> data );

            /**
             * @brief Query whether it is safe to operate on this object from the current thread.
             *
             * This implementation returns true when:
             *  - the underlying `T` reports its loading state as `LoadingState::Loaded`, and
             *  - the currently executing task equals the physics manager's physics task.
             *
             * Use this to guard operations that must run on the physics thread.
             *
             * @return true if current thread/task is the physics task and the object is loaded.
             */
            bool isThreadSafe() const;

            /**
             * @brief Enqueue a message to the associated state context on the physics state task.
             *
             * If no state context is attached, the message is silently ignored.
             *
             * @param message Message to deliver to the state context.
             */
            void addMessage( SmartPtr<IStateMessage> message );

            /**
             * @brief Get the state context associated with this object.
             *
             * Returns a strong SmartPtr by locking the internally-stored weak pointer.
             * Caller should check the returned pointer before use.
             *
             * @return SmartPtr to the current IStateContext or nullptr if none.
             */
            SmartPtr<IStateContext> getStateContext() const;

            /**
             * @brief Attach or replace the state context for this object.
             *
             * The provided context is stored into an atomic weak pointer so the
             * object does not keep it alive beyond intended scope. Use `getStateContext`
             * to obtain a strong reference before use.
             *
             * @param stateContext New state context (may be nullptr to clear).
             */
            void setStateContext( SmartPtr<IStateContext> stateContext );

            /**
             * @brief Get the state listener associated with this object.
             *
             * The listener is stored as an atomic strong pointer.
             *
             * @return SmartPtr to the IStateListener or nullptr if none.
             */
            SmartPtr<IStateListener> getStateListener() const;

            /**
             * @brief Set the state listener for this object.
             *
             * The listener will be unloaded and cleared by `destroyStateContext`
             * when the object is unloaded.
             *
             * @param stateListener Listener to attach (may be nullptr to clear).
             */
            void setStateListener( SmartPtr<IStateListener> stateListener );

            /**
             * @name Physics manager synchronization helpers
             *
             * These forward to the global physics manager to ensure callers can
             * safely lock the physics subsystem when performing multi-step updates.
             */
            ///@{
            void lock();
            bool try_lock();
            void unlock();
            ///@}

            WP_CLASS_REGISTER_TEMPLATE_DECL( Physics3SharedObject, T );

        protected:
            /**
             * @brief Cleanup helper that tears down attached state context and listener.
             *
             * Steps performed:
             *  - If a state context is attached:
             *      - remove the attached state listener (if any) from the context,
             *      - call `unload(nullptr)` on the context,
             *      - remove the context from the global state manager,
             *      - clear the stored weak pointer to the context.
             *  - If a state listener is attached:
             *      - call `unload(nullptr)` on the listener,
             *      - clear the stored listener pointer.
             *
             * This method is safe to call on any thread but assumes the global
             * state manager and state context lifetime semantics are respected by
             * the caller.
             */
            void destroyStateContext();

            /** Weak reference to the state context associated with this object.
             *
             * Stored as an atomic weak pointer so the shared object can observe
             * the context without keeping it alive indefinitely.
             */
            AtomicWeakPtr<IStateContext> m_stateContext;

            /** Strong atomic reference to a state listener attached to the context.
             *
             * If set, the listener will be removed from the context and unloaded
             * when `destroyStateContext` runs.
             */
            AtomicSmartPtr<IStateListener> m_stateListener;
        };

        template <typename T>
        Physics3SharedObject<T>::Physics3SharedObject() = default;

        template <typename T>
        Physics3SharedObject<T>::~Physics3SharedObject() = default;

        template <typename T>
        void Physics3SharedObject<T>::unload( SmartPtr<ISharedObject> data )
        {
            destroyStateContext();
        }

        template <typename T>
        bool Physics3SharedObject<T>::isThreadSafe() const
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto physicsManager = applicationManager->getPhysicsManager();
            auto physicsTask = physicsManager->getPhysicsTask();

            auto task = Thread::getCurrentTask();

            const auto &loadingState = T::getLoadingState();

            return loadingState == LoadingState::Loaded && task == physicsTask;
        }

        template <typename T>
        void Physics3SharedObject<T>::addMessage( SmartPtr<IStateMessage> message )
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto physicsManager = applicationManager->getPhysicsManager();

            if( auto stateContext = Physics3SharedObject<T>::getStateContext() )
            {
                const auto stateTask = physicsManager->getStateTask();
                stateContext->addMessage( stateTask, message );
            }
        }

        template <typename T>
        SmartPtr<IStateContext> Physics3SharedObject<T>::getStateContext() const
        {
            auto p = m_stateContext.load();
            return p.lock();
        }

        template <typename T>
        void Physics3SharedObject<T>::setStateContext( SmartPtr<IStateContext> stateContext )
        {
            m_stateContext = stateContext;
        }

        template <typename T>
        SmartPtr<IStateListener> Physics3SharedObject<T>::getStateListener() const
        {
            return m_stateListener;
        }

        template <typename T>
        void Physics3SharedObject<T>::setStateListener( SmartPtr<IStateListener> stateListener )
        {
            m_stateListener = stateListener;
        }

        template <typename T>
        void Physics3SharedObject<T>::lock()
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto physicsManager = applicationManager->getPhysicsManager();
            physicsManager->lock();
        }

        template <typename T>
        bool Physics3SharedObject<T>::try_lock()
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto physicsManager = applicationManager->getPhysicsManager();
            return physicsManager->try_lock();
        }

        template <typename T>
        void Physics3SharedObject<T>::unlock()
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto physicsManager = applicationManager->getPhysicsManager();
            physicsManager->unlock();
        }

        template <typename T>
        void Physics3SharedObject<T>::destroyStateContext()
        {
            auto applicationManager = core::IApplicationManager::instance();

            if( auto stateManager = applicationManager->getStateManager() )
            {
                if( auto stateContext = getStateContext() )
                {
                    if( auto stateListener = Physics3SharedObject<T>::getStateListener() )
                    {
                        stateContext->removeStateListener( stateListener );
                    }

                    stateContext->unload( nullptr );

                    if( stateManager )
                    {
                        stateManager->removeStateContext( stateContext );
                    }

                    setStateContext( nullptr );
                }

                if( auto stateListener = Physics3SharedObject<T>::getStateListener() )
                {
                    stateListener->unload( nullptr );
                    Physics3SharedObject<T>::setStateListener( nullptr );
                }
            }
        }

        WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone::physics, Physics3SharedObject, T, T );

    }  // namespace physics
}  // namespace workphone

#endif  // WP_Physics3SharedObject_h__
