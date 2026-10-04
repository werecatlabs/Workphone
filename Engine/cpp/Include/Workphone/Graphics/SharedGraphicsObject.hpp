#ifndef CSharedGraphicsObject_h__
#define CSharedGraphicsObject_h__

#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Thread/Thread.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/System/IStateManager.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Interface/System/IState.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>
#include <Workphone/Interface/System/IThreadPool.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @brief Template base for graphics objects that are shared between threads/tasks.
         *
         * SharedGraphicsObject<T> augments the underlying type T with support for a state
         * context and a state listener. It provides helpers for safely interacting with the
         * graphics system (locking) and for delivering state messages to the object's
         * state context on the graphics/state thread.
         *
         * @tparam T The concrete base type which typically implements ISharedObject or a
         *           graphics resource interface.
         */
        template <typename T>
        class SharedGraphicsObject : public T
        {
        public:
            /** Default constructor. */
            SharedGraphicsObject();

            template <typename U = T, std::enable_if_t<std::is_constructible<U, u32>::value, int> = 0>
            SharedGraphicsObject( u32 poolTypeId ) : T( poolTypeId )
            {
            }

            /** Default virtual destructor. */
            ~SharedGraphicsObject() override;

            /**
             * @brief Load object data.
             *
             * Forwards to the base type's load implementation. Override in specializations
             * if additional load behaviour is required.
             *
             * @param data Optional shared data passed to the loader.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload object data and destroy associated state context.
             *
             * This calls destroyStateContext() to remove and unload any state-related
             * objects before forwarding to the base type's unload implementation.
             *
             * @param data Optional shared data passed to the unloader.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Returns true when it is safe to access this object from the current task.
             *
             * Implementation checks that the object is loaded and that the current thread/task
             * flags include the render task flag. Uses Thread::getTaskFlags() and compares
             * against Thread::Render_Flag.
             *
             * @return true if current context is the graphics/render task and this object is loaded.
             */
            bool isThreadSafe() const override;

            /**
             * @brief Queue a state message to this object's state context on the graphics/state task.
             *
             * If a state context exists this will look up the graphics system's state task and
             * add the message to the context so it will be processed on the appropriate thread.
             *
             * @param message The message to enqueue; ignored if no state context is present.
             */
            void addMessage( SmartPtr<IStateMessage> message );

            /**
             * @brief Get the raw pointer to the attached IStateContext.
             * Useful when C-style pointer access is required. The returned pointer is not
             * accompanied by ownership guarantees; prefer getStateContext() for ownership.
             * @return Raw IStateContext pointer or nullptr if none set.
             */
            virtual IStateContext *getStateContextPtr() const;

            /**
             * @brief Get the SmartPtr to the attached IStateContext.
             * Returns the internal smart pointer that owns the state context.
             * @return SmartPtr<IStateContext> reference (may be nullptr).
             */
            virtual SmartPtr<IStateContext> getStateContext() const;

            /**
             * @brief Attach a state context to this object.
             * The provided SmartPtr will be stored and later unloaded/removed by
             * destroyStateContext() during object unload.
             * @param stateContext Smart pointer to the state context to attach.
             */
            virtual void setStateContext( SmartPtr<IStateContext> stateContext );

            /**
             * @brief Get the state listener attached to this object.
             * @return SmartPtr<IStateListener> (may be nullptr).
             */
            virtual SmartPtr<IStateListener> getStateListener() const;

            /**
             * @brief Attach a state listener to this object.
             *
             * The listener will be removed from the state context and unloaded by
             * destroyStateContext() when the object is being torn down.
             *
             * @param stateListener Smart pointer to the listener to attach.
             */
            virtual void setStateListener( SmartPtr<IStateListener> stateListener );

            /**
             * @brief Retrieve properties for this shared object.
             *
             * This implementation delegates to ISharedObject::getProperties().
             *
             * @return SmartPtr<Properties> containing the object's properties.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Set properties for this shared object.
             *
             * This implementation delegates to ISharedObject::setProperties().
             *
             * @param properties New properties to assign to the object.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Acquire the graphics system lock.
             *
             * Uses the global application manager to obtain the graphics system and call lock().
             * Asserts if the application manager or graphics system are not available.
             */
            void lock() override;

            /**
             * @brief Try to acquire the graphics system lock without blocking.
             *
             * Returns immediately with whether the lock was obtained.
             *
             * @return true if the lock was acquired, false otherwise.
             */
            bool try_lock() override;

            /**
             * @brief Release the graphics system lock.
             *
             * Calls unlock() on the graphics system obtained from the application manager.
             */
            void unlock() override;

            WP_CLASS_REGISTER_TEMPLATE_DECL( SharedGraphicsObject, T );

        protected:
            /**
             * @brief Remove and unload the attached state context and listener.
             *
             * This method:
             *  - Removes the state listener from the state context (if present).
             *  - Calls unload(nullptr) on the state context and the listener to allow them
             *    to release resources on their own thread.
             *  - Removes the state context from the global state manager.
             *  - Clears the stored smart pointers.
             *
             * It is safe to call multiple times; checks for nullptrs are performed.
             */
            void destroyStateContext();

            /** The state context associated with this graphics/shared object. */
            AtomicSmartPtr<IStateContext> m_stateContext;

            /** The state listener registered with the state context for this object. */
            AtomicSmartPtr<IStateListener> m_stateListener;
        };

        template <typename T>
        SharedGraphicsObject<T>::SharedGraphicsObject() = default;

        template <typename T>
        SharedGraphicsObject<T>::~SharedGraphicsObject()
        {
            destroyStateContext();
        }

        template <typename T>
        void SharedGraphicsObject<T>::load( SmartPtr<ISharedObject> data )
        {
            T::load( data );
        }

        template <typename T>
        void SharedGraphicsObject<T>::unload( SmartPtr<ISharedObject> data )
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto stateManager = applicationManager->getStateManager();
            if( stateManager )
            {
                if( auto stateContext = getStateContext() )
                {
                    if( auto stateListener = getStateListener() )
                    {
                        stateContext->removeStateListener( stateListener );
                    }

                    const auto ownsContext = stateContext->getOwnerPtr() == this;
                    if( ownsContext )
                    {
                        stateContext->setOwner( nullptr );
                    }
                    else
                    {
                        stateContext->removeStatesById( this->getId() );
                    }

                    if( ownsContext ||
                        ( stateContext->getOwnerPtr() == nullptr &&
                          stateContext->getStates().empty() &&
                          stateContext->getStateListeners().empty() ) )
                    {
                        stateManager->removeStateContext( stateContext );
                    }
                }

                m_stateContext = nullptr;
                m_stateListener = nullptr;
            }

            T::unload( data );
        }

        template <typename T>
        bool SharedGraphicsObject<T>::isThreadSafe() const
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto threadPool = applicationManager->getThreadPoolPtr();

            if( threadPool && threadPool->getNumThreads() > 0 )
            {
                auto taskFlags = Thread::getTaskFlags();
                return SharedGraphicsObject<T>::isLoaded() &&
                       ( ( taskFlags & Thread::Render_Flag ) != 0 );
            }

            return true;
        }

        template <typename T>
        void SharedGraphicsObject<T>::addMessage( SmartPtr<IStateMessage> message )
        {
            if( message )
            {
                message->setSender( this );

                auto applicationManager = core::IApplicationManager::instancePtr();
                auto graphicsSystem = applicationManager->getGraphicsSystemPtr();

                if( auto stateContext = SharedGraphicsObject<T>::getStateContextPtr() )
                {
                    const auto stateTask = graphicsSystem->getStateTask();
                    stateContext->addMessage( stateTask, message );
                }
            }
        }

        template <typename T>
        IStateContext *SharedGraphicsObject<T>::getStateContextPtr() const
        {
            return m_stateContext.get();
        }

        template <typename T>
        SmartPtr<IStateContext> SharedGraphicsObject<T>::getStateContext() const
        {
            return m_stateContext;
        }

        template <typename T>
        void SharedGraphicsObject<T>::setStateContext( SmartPtr<IStateContext> stateContext )
        {
            m_stateContext = stateContext;
        }

        template <typename T>
        SmartPtr<IStateListener> SharedGraphicsObject<T>::getStateListener() const
        {
            return m_stateListener;
        }

        template <typename T>
        void SharedGraphicsObject<T>::setStateListener( SmartPtr<IStateListener> stateListener )
        {
            m_stateListener = stateListener;
        }

        template <typename T>
        void SharedGraphicsObject<T>::destroyStateContext()
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            if( applicationManager )
            {
                if( auto stateManager = applicationManager->getStateManagerPtr() )
                {
                    if( auto stateContext = getStateContextPtr() )
                    {
                        if( auto stateListener = getStateListener() )
                        {
                            stateContext->removeStateListener( stateListener );
                        }

                        auto states = stateContext->getStates();
                        for( auto &state : states )
                        {
                            if( state && state->getOwnerPtr() == this )
                            {
                                state->unload( nullptr );
                                stateContext->removeState( state );
                            }
                        }

                        const auto ownsContext = stateContext->getOwnerPtr() == this;
                        if( ownsContext )
                        {
                            stateContext->setOwner( nullptr );
                        }

                        if( ownsContext ||
                            ( stateContext->getOwnerPtr() == nullptr &&
                              stateContext->getStates().empty() &&
                              stateContext->getStateListeners().empty() ) )
                        {
                            stateManager->removeStateContext( stateContext );
                        }
                    }
                }
            }

            m_stateContext = nullptr;
            m_stateListener = nullptr;
        }

        template <typename T>
        SmartPtr<Properties> SharedGraphicsObject<T>::getProperties() const
        {
            return ISharedObject::getProperties();
        }

        template <typename T>
        void SharedGraphicsObject<T>::setProperties( SmartPtr<Properties> properties )
        {
            ISharedObject::setProperties( properties );
        }

        template <typename T>
        void SharedGraphicsObject<T>::lock()
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            if( graphicsSystem )
            {
                graphicsSystem->lock();
            }
        }

        template <typename T>
        bool SharedGraphicsObject<T>::try_lock()
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            if( graphicsSystem )
            {
                return graphicsSystem->try_lock();
            }

            return false;
        }

        template <typename T>
        void SharedGraphicsObject<T>::unlock()
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            if( graphicsSystem )
            {
                graphicsSystem->unlock();
            }
        }

        WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone::render, SharedGraphicsObject, T, T );

    }  // namespace render
}  // namespace workphone

#endif  // CSharedGraphicsObject_h__
