#ifndef PhysicsShape_h__
#define PhysicsShape_h__

#include <Workphone/Interface/Physics/IPhysicsShape.hpp>
#include <Workphone/State/States/ShapeStateData.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Interface/System/IStateManager.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Core/BitUtil.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Template mixin that provides common physics shape state management.
         *
         * PhysicsShape<T> augments a derived type T with state-backed properties
         * such as enabled/trigger flags, collision type/mask and lifecycle hooks
         * for creating/destroying a state object and listener.
         *
         * The implementation stores weak atomic references to an IStateContext and
         * an IStateListener. All property accessors operate via the associated
         * ShapeState within the state context when present.
         *
         * @tparam T Base type to extend. T is expected to implement IPhysicsShape
         *           or a compatible interface.
         */
        template <class T>
        class PhysicsShape : public T
        {
        public:
            /**
             * @brief Default constructor.
             */
            PhysicsShape();

            /**
             * @brief Virtual destructor.
             */
            ~PhysicsShape() override;

            /**
             * @brief Load shape-specific data.
             *
             * Implementations may parse provided shared data and create internal
             * resources. Default implementation is empty.
             *
             * @param data Optional shared object containing initialization data.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload and release resources associated with this shape.
             *
             * Default implementation destroys any state object created by
             * `createStateObject`.
             *
             * @param data Optional shared object containing unload hints.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Query whether the underlying shape representation is valid.
             *
             * Returns true when the derived type has a valid native/physics
             * representation (e.g., an allocated physics actor/shape). Default
             * implementation returns false.
             *
             * @return true if the shape is valid; false otherwise.
             */
            bool isValid() const override;

            /**
             * @brief Query whether the shape is attached to its parent (scene / body).
             *
             * Returns true when the shape is currently attached to a physics
             * parent or scene. Default implementation returns false.
             *
             * @return true if attached; false otherwise.
             */
            bool isAttached() const override;

            /**
             * @brief Enable or disable this shape.
             *
             * This sets the ShapeFlagEnabled flag within the associated ShapeState
             * if a state context is present.
             *
             * @param enabled True to enable the shape; false to disable.
             */
            void setEnabled( bool enabled ) override;

            /**
             * @brief Returns whether this shape is enabled.
             *
             * Reads the ShapeFlagEnabled flag from the ShapeState when available.
             *
             * @return true if enabled; false otherwise.
             */
            bool isEnabled() const override;

            /**
             * @brief Returns whether this shape is a trigger.
             *
             * A trigger shape does not generate physical responses but reports
             * overlap events. Checks ShapeFlagTrigger in ShapeState.
             *
             * @return true if this shape is configured as a trigger; false otherwise.
             */
            bool isTrigger() const override;

            /**
             * @brief Configure this shape as a trigger or a collision shape.
             *
             * Updates the ShapeFlagTrigger flag in the ShapeState.
             *
             * @param trigger True to mark as trigger; false to mark as collision shape.
             */
            void setTrigger( bool trigger ) override;

            /**
             * @brief Sets the collision type for this shape.
             *
             * Collision type represents the category/group this shape belongs to.
             * Updated in the shape's ShapeState when a state context exists.
             *
             * @param mask Collision type bitmask.
             */
            void setCollisionType( u32 mask ) override;

            /**
             * @brief Returns the collision type bitmask for this shape.
             *
             * Reads the collisionType field from the ShapeState if present.
             *
             * @return Collision type bitmask or 0 if unavailable.
             */
            u32 getCollisionType() const override;

            /**
             * @brief Sets the collision mask for this shape.
             *
             * The collision mask determines which collision types this shape will
             * interact with. Stored in ShapeState when available.
             *
             * @param mask Collision mask bitmask.
             */
            void setCollisionMask( u32 mask ) override;

            /**
             * @brief Returns the collision mask for this shape.
             *
             * Reads the collisionMask field from ShapeState if present.
             *
             * @return Collision mask bitmask or 0 if unavailable.
             */
            u32 getCollisionMask() const override;

            /**
             * @brief Returns the current state context associated with this shape.
             *
             * The state context provides access to the ShapeState instance used
             * by the engine to coordinate state across systems.
             *
             * @return Shared pointer to IStateContext or null if none set.
             */
            SmartPtr<IStateContext> getStateContext() const;

            /**
             * @brief Assign the state context to use for this shape.
             *
             * The provided context will be stored as an atomic weak pointer so
             * that the shape does not own the context lifetime.
             *
             * @param stateContext Shared pointer to the state context.
             */
            void setStateContext( SmartPtr<IStateContext> stateContext );

            /**
             * @brief Returns the state listener attached to this shape, if any.
             *
             * State listeners receive notifications when state changes occur.
             *
             * @return Shared pointer to IStateListener or null if none set.
             */
            SmartPtr<IStateListener> getStateListener() const;

            /**
             * @brief Set the state listener for this shape.
             *
             * The listener will be stored as an atomic weak pointer.
             *
             * @param stateListener Shared pointer to an IStateListener.
             */
            void setStateListener( SmartPtr<IStateListener> stateListener );

            WP_CLASS_REGISTER_TEMPLATE_DECL( PhysicsShape, T );

        protected:
            /**
             * @brief Create and register the ShapeState / state listener for this shape.
             *
             * Derived classes should override to allocate and register a state
             * object and attach a listener. The default implementation does nothing.
             */
            virtual void createStateObject();

            /**
             * @brief Remove and cleanup the ShapeState and associated listener.
             *
             * The default implementation will:
             *  - unload the state listener (if present),
             *  - remove the listener from the state context,
             *  - remove the state object from the global state manager,
             *  - clear stored weak references.
             *
             * This is called from `unload`.
             */
            virtual void destroyStateObject();

            /**< The state context associated with this shape. */
            AtomicWeakPtr<IStateContext> m_stateContext;

            /**< The state listener associated with this shape. */
            AtomicWeakPtr<IStateListener> m_stateListener;
        };

        WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone::physics, PhysicsShape, T, T );

        template <class T>
        PhysicsShape<T>::PhysicsShape() = default;

        template <class T>
        PhysicsShape<T>::~PhysicsShape() = default;

        template <class T>
        void PhysicsShape<T>::load( SmartPtr<ISharedObject> data )
        {
        }

        template <class T>
        void PhysicsShape<T>::unload( SmartPtr<ISharedObject> data )
        {
            destroyStateObject();
        }

        template <class T>
        bool PhysicsShape<T>::isValid() const
        {
            return false;
        }

        template <class T>
        bool PhysicsShape<T>::isAttached() const
        {
            return false;
        }

        template <class T>
        void PhysicsShape<T>::setEnabled( bool enabled )
        {
            if( auto stateContext = PhysicsShape<T>::getStateContext() )
            {
                if( auto stateData = stateContext->template invalidateStateData<ShapeStateData>() )
                {
                    stateData->flags = BitUtil::setFlagValue( stateData->flags,
                                                              IPhysicsShape::ShapeFlagEnabled, enabled );
                }
            }
        }

        template <class T>
        bool PhysicsShape<T>::isEnabled() const
        {
            if( auto stateContext = PhysicsShape<T>::getStateContext() )
            {
                if( auto stateData = stateContext->template getStateData<ShapeStateData>() )
                {
                    return BitUtil::getFlagValue( stateData->flags, IPhysicsShape::ShapeFlagEnabled );
                }
            }

            return false;
        }

        template <class T>
        bool PhysicsShape<T>::isTrigger() const
        {
            if( auto stateContext = PhysicsShape<T>::getStateContext() )
            {
                if( auto stateData = stateContext->template getStateData<ShapeStateData>() )
                {
                    return BitUtil::getFlagValue( stateData->flags, IPhysicsShape::ShapeFlagTrigger );
                }
            }

            return false;
        }

        template <class T>
        void PhysicsShape<T>::setTrigger( bool trigger )
        {
            if( auto stateContext = PhysicsShape<T>::getStateContext() )
            {
                if( auto stateData = stateContext->template invalidateStateData<ShapeStateData>() )
                {
                    stateData->flags = BitUtil::setFlagValue( stateData->flags,
                                                              IPhysicsShape::ShapeFlagTrigger, trigger );
                }
            }
        }

        template <class T>
        void PhysicsShape<T>::setCollisionType( u32 type )
        {
            if( auto stateContext = PhysicsShape<T>::getStateContext() )
            {
                if( auto stateData = stateContext->template invalidateStateData<ShapeStateData>() )
                {
                    stateData->collisionType = type;
                }
            }
        }

        template <class T>
        auto PhysicsShape<T>::getCollisionType() const -> u32
        {
            if( auto stateContext = PhysicsShape<T>::getStateContext() )
            {
                if( auto stateData = stateContext->template getStateData<ShapeStateData>() )
                {
                    return stateData->collisionType;
                }
            }

            return 0;
        }

        template <class T>
        void PhysicsShape<T>::setCollisionMask( u32 mask )
        {
            if( auto stateContext = PhysicsShape<T>::getStateContext() )
            {
                if( auto stateData = stateContext->template invalidateStateData<ShapeStateData>() )
                {
                    stateData->collisionMask = mask;
                }
            }
        }

        template <class T>
        auto PhysicsShape<T>::getCollisionMask() const -> u32
        {
            if( auto stateContext = PhysicsShape<T>::getStateContext() )
            {
                if( auto stateData = stateContext->template getStateData<ShapeStateData>() )
                {
                    return stateData->collisionMask;
                }
            }

            return 0;
        }

        template <class T>
        SmartPtr<IStateContext> PhysicsShape<T>::getStateContext() const
        {
            auto p = m_stateContext.load();
            return p.lock();
        }

        template <class T>
        void PhysicsShape<T>::setStateContext( SmartPtr<IStateContext> stateContext )
        {
            m_stateContext = stateContext;
        }

        template <class T>
        SmartPtr<IStateListener> PhysicsShape<T>::getStateListener() const
        {
            auto p = m_stateListener.load();
            return p.lock();
        }

        template <class T>
        void PhysicsShape<T>::setStateListener( SmartPtr<IStateListener> stateListener )
        {
            m_stateListener = stateListener;
        }

        template <class T>
        void PhysicsShape<T>::createStateObject()
        {
        }

        template <class T>
        void PhysicsShape<T>::destroyStateObject()
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto stateManager = applicationManager->getStateManagerPtr();

            if( auto stateContext = PhysicsShape<T>::getStateContext() )
            {
                if( auto stateListener = PhysicsShape<T>::getStateListener() )
                {
                    stateListener->unload( nullptr );
                    stateContext->removeStateListener( stateListener );
                    PhysicsShape<T>::setStateListener( nullptr );
                }

                if( stateManager )
                {
                    stateManager->removeStateContext( stateContext );
                }

                PhysicsShape<T>::setStateContext( nullptr );
            }
        }

    }  // namespace physics
}  // namespace workphone

#endif  // PhysicsShape_h__
