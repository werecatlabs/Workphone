#ifndef WPPhysxShape_h__
#define WPPhysxShape_h__

#include <WPPhysx/WPPhysxPrerequisites.hpp>
#include <WPPhysx/WPPhysxSharedObject.hpp>
#include <Workphone/Interface/Physics/IPhysicsShape3.hpp>
#include <Workphone/Interface/Physics/IPhysicsBody3.hpp>
#include <Workphone/Interface/Physics/IPhysicsMaterial3.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/State/Messages/StateMessage.hpp>
#include <Workphone/State/States/ShapeStateData.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <WPPhysx/WPPhysxManager.hpp>
#include <PxPhysicsAPI.h>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Base implementation of a physics collision shape backed by PhysX.
         *
         * This template implements common functionality for concrete PhysX-backed shape
         * types (sphere/box/mesh/etc). It adapts the engine-level `IPhysicsShape3`
         * behaviour to an underlying `physx::PxShape` and provides utilities for:
         *  - holding the raw `PxShape` pointer,
         *  - associating the shape with a `physx::PxRigidActor`,
         *  - configuring collision filtering (collision type / mask) and trigger flags,
         *  - responding to state changes (state messages / shape state objects),
         *  - serialising a small set of properties (e.g. `isTrigger`).
         *
         * Ownership / lifetime:
         *  - The class stores a raw/atomic pointer to a PhysX `PxShape` in `m_shape`.
         *    PhysX object lifetime rules apply: when the shape is destroyed the engine
         *    must call `release()` on it. `unload()` will detach and release the shape
         *    if it exists.
         *
         * Threading:
         *  - Access to `m_shape` uses an atomic wrapper. Callers must still respect
         *    PhysX threading rules when interacting with the raw `PxShape` pointer.
         *
         * @tparam T Concrete derived shape class (used for registration / factory glue).
         */
        template <class T>
        class PhysxShape : public PhysxSharedObject<T>
        {
        public:
            /** @brief Create an empty PhysxShape. */
            PhysxShape();

            /** @brief Virtual destructor. Derived classes should clean up PhysX objects in unload. */
            ~PhysxShape() override;

            /**
             * @brief Load the shape from an optional data object.
             *
             * Derived classes should create the underlying PhysX shape (call `createShape`)
             * in response to state messages or here as appropriate.
             *
             * @param data Optional data used during load; may be null.
             */
            void load( SmartPtr<ISharedObject> data );

            /**
             * @brief Unload the shape and release any associated PhysX resources.
             *
             * Detaches the `PxShape` from its actor (if set) then calls `release()` on it
             * and clears the internal pointer.
             *
             * @param data Optional data used during unload; may be null.
             */
            void unload( SmartPtr<ISharedObject> data );

            /**
             * @brief Return the internal raw object pointer for generic shared object handling.
             *
             * This writes the raw pointer to the underlying PhysX shape into `ppObject`.
             *
             * @param ppObject Output pointer address to receive the raw shape pointer.
             */
            void _getObject( void **ppObject ) const override;

            /**
             * @brief Get the underlying PhysX shape pointer.
             * @return Pointer to the PhysX `PxShape`, or `nullptr` if none set.
             */
            physx::PxShape *getShape() const;

            /**
             * @brief Set the underlying PhysX shape pointer.
             *
             * This assignment does not take ownership via an additional reference count;
             * the caller / owner is responsible for ensuring proper creation / release
             * of the PhysX object. `unload()` will call `release()` if a shape is present.
             *
             * @param shape Raw `physx::PxShape` pointer to store.
             */
            void setShape( physx::PxShape *shape );

            /**
             * @brief Check whether a PhysX shape is present.
             * @return True if `getShape()` returns a non-null pointer.
             */
            virtual bool hasShapeData() const override;

            /**
             * @brief Create a copy/clone of this physics shape at the engine interface level.
             *
             * Default implementation returns `nullptr`. Derived classes that can be cloned
             * should override this method and return a new `IPhysicsShape3` instance.
             *
             * @return Smart pointer to a cloned `IPhysicsShape3` or `nullptr` if cloning not supported.
             */
            virtual SmartPtr<IPhysicsShape3> clone();

            /**
             * @brief Return child objects owned/used by this shape for object graph traversal.
             *
             * Typical children could include associated materials or body references. Default
             * implementation returns an empty array.
             *
             * @return Array of `ISharedObject` smart pointers representing children.
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @brief Serialise a small set of runtime properties to a `Properties` object.
             *
             * Currently stores the `isTrigger` flag. Derived classes may add additional properties.
             *
             * @return A `Properties` object containing serialised properties, or `nullptr` on error.
             */
            virtual SmartPtr<Properties> getProperties() const;

            /**
             * @brief Restore properties from a `Properties` container.
             *
             * Reads the `isTrigger` flag if present. Derived classes should read their own keys.
             *
             * @param properties Properties container to read from.
             */
            virtual void setProperties( SmartPtr<Properties> properties );

            /**
             * @brief Get the PhysX actor this shape is attached to (if any).
             * @return Pointer to the `physx::PxRigidActor` this shape is attached to, or `nullptr`.
             */
            physx::PxRigidActor *getPxActor() const;

            /**
             * @brief Associate this shape with a PhysX actor.
             *
             * This class does not attach/detach shapes automatically when this is set; `unload`
             * handles detaching if required. Setting the actor simply records the pointer.
             *
             * @param pxActor Raw `physx::PxRigidActor` pointer to associate with the shape.
             */
            void setPxActor( physx::PxRigidActor *pxActor );

            /**
             * @brief Validate the internal state of the shape object.
             *
             * Default implementation returns true; derived classes may perform checks
             * (e.g. ensure `m_shape` is valid).
             *
             * @return True if the object is considered valid.
             */
            bool isValid() const override;

            void setTrigger( bool trigger ) override;

            void setCollisionType( u32 type ) override;

            void setCollisionMask( u32 mask ) override;

            /**
             * @brief Handle an incoming state change message.
             *
             * The default implementation listens for `IPhysicsShape::CREATE_SHAPE_HASH`
             * and will call `createShape()` in response.
             *
             * @param message State message to handle.
             * @return True if the message was handled, otherwise false.
             */
            virtual bool handleStateChanged( const SmartPtr<IStateMessage> &message );

            /**
             * @brief Handle an incoming state object.
             *
             * If `state` contains a `ShapeState` derived data object, the shape's collision
             * mask, collision type and trigger flag will be applied using `setupCollisionMask`.
             *
             * @param state State object containing data (may be a `ShapeState`).
             * @return True if the state was handled, otherwise false.
             */
            virtual bool handleStateChanged( SmartPtr<IState> &state );

            /**
             * @brief Configure collision filter and trigger flags on the provided `PxShape`.
             *
             * Uses the shape's stored collision type/mask and trigger flag properties.
             *
             * @param shape PhysX shape to configure.
             */
            void setupCollisionMask( physx::PxShape *shape );
            /**
             * @brief Configure collision filter and trigger flags on the provided `PxShape`.
             *
             * Sets PhysX shape flags for simulation vs trigger behaviour and writes the
             * provided `collisionType` and `collisionMask` into the shape's `PxFilterData`.
             *
             * @param shape PhysX shape to configure.
             * @param collisionType Bitmask representing the collision category for this shape.
             * @param collisionMask Bitmask representing which categories this shape collides with.
             * @param trigger True to set the shape as a trigger (non-simulation); false otherwise.
             */
            void setupCollisionMask( physx::PxShape *shape, u32 collisionType, u32 collisionMask,
                                     bool trigger );

            WP_CLASS_REGISTER_TEMPLATE_DECL( PhysxShape, T );

        protected:
            /**
             * @brief Internal listener that forwards state messages/changes to the owning shape.
             *
             * Instances of this listener are registered with the engine state system to receive
             * notifications and then call the owning `PhysxShape`'s handlers. It keeps a weak
             * reference to avoid ownership cycles.
             */
            class ShapeStateListener : public IStateListener
            {
            public:
                /** @brief Default constructor. */
                ShapeStateListener();

                /** @brief Destructor. */
                ~ShapeStateListener() override;

                /**
                 * @brief Unload the listener and clear the owner reference.
                 * @param data Optional data, ignored by default.
                 */
                void unload( SmartPtr<ISharedObject> data ) override;

                /**
                 * @brief Handle a state message by forwarding to the owner shape.
                 * @param message State message to forward.
                 * @return True if the owner handled the message, false otherwise.
                 */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

                /**
                 * @brief Handle a full state change by forwarding to the owner shape.
                 * @param state State object to forward.
                 * @return True if the owner handled the state, false otherwise.
                 */
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                /**
                 * @brief Get a strong smart pointer to the owning `PhysxShape`.
                 * @return Smart pointer to the owner, or an empty pointer if the owner no longer exists.
                 */
                SmartPtr<PhysxShape<T>> getOwner() const;

                /**
                 * @brief Set the owning `PhysxShape`.
                 * @param owner Smart pointer to assign as the owner.
                 */
                void setOwner( SmartPtr<PhysxShape<T>> owner );

            protected:
                /** @brief Weak pointer to the owning shape to avoid reference cycles. */
                AtomicWeakPtr<PhysxShape<T>> m_owner;
            };

            /**
             * @brief Create and register any state objects/listeners required by this shape.
             *
             * Default implementation does nothing. Derived classes that need a state object
             * should override and create/register it here.
             */
            virtual void createStateObject();

            /**
             * @brief Create the underlying PhysX shape (called in response to a CREATE_SHAPE message).
             *
             * Default implementation does nothing. Derived classes must override to create concrete
             * shapes.
             */
            virtual void createShape();

            /** @brief Atomic raw pointer to the PhysX shape instance. */
            AtomicRawPtr<physx::PxShape> m_shape;

            /** @brief PhysX actor this shape is (or will be) attached to. Not owned. */
            physx::PxRigidActor *m_pxActor = nullptr;

            /** @brief Whether this shape should behave as a trigger. */
            bool m_isTrigger = false;
        };

        template <class T>
        PhysxShape<T>::PhysxShape() = default;

        template <class T>
        PhysxShape<T>::~PhysxShape() = default;

        template <class T>
        void PhysxShape<T>::load( SmartPtr<ISharedObject> data )
        {
            PhysxSharedObject<T>::load( data );
        }

        template <class T>
        void PhysxShape<T>::unload( SmartPtr<ISharedObject> data )
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto physicsManager = (PhysxManager *)applicationManager->getPhysicsManagerPtr();
            WP_ASSERT( physicsManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            // NOTE: do NOT manually unload or touch the state listener here.
            // The base class unload chain (PhysxSharedObject -> T -> PhysicsShape::unload)
            // calls destroyStateObject(), which correctly unloads the listener, removes it
            // from the state context, and clears the stored weak reference. Doing it here
            // as well causes a double-unload; and if the state manager is torn down out of
            // order, resolving the stale handle here lands on the wrong live object.

            if( auto shape = getShape() )
            {
                if( auto actor = getPxActor() )
                {
                    actor->detachShape( *shape, false );
                }

                shape->release();
                setShape( nullptr );
            }

            PhysxSharedObject<T>::unload( data );
        }

        template <class T>
        void PhysxShape<T>::_getObject( void **ppObject ) const
        {
            *ppObject = getShape();
        }

        template <class T>
        physx::PxShape *PhysxShape<T>::getShape() const
        {
            return m_shape.load();
        }

        template <class T>
        void PhysxShape<T>::setShape( physx::PxShape *shape )
        {
            m_shape = shape;
        }

        template <class T>
        bool PhysxShape<T>::hasShapeData() const
        {
            return getShape() != nullptr;
        }

        template <class T>
        SmartPtr<IPhysicsShape3> PhysxShape<T>::clone()
        {
            return nullptr;
        }

        template <class T>
        Array<SmartPtr<ISharedObject>> PhysxShape<T>::getChildObjects() const
        {
            Array<SmartPtr<ISharedObject>> objects;
            objects.reserve( 10 );

            // objects.push_back( T::m_body );
            //  objects.push_back( T::m_material );
            return objects;
        }

        template <class T>
        SmartPtr<Properties> PhysxShape<T>::getProperties() const
        {
            try
            {
                auto properties = workphone::make_ptr<Properties>();
                properties->setProperty( "isTrigger", m_isTrigger );
                return properties;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        template <class T>
        void PhysxShape<T>::setProperties( SmartPtr<Properties> properties )
        {
            try
            {
                properties->getPropertyValue( "isTrigger", m_isTrigger );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        template <class T>
        physx::PxRigidActor *PhysxShape<T>::getPxActor() const
        {
            return m_pxActor;
        }

        template <class T>
        void PhysxShape<T>::setPxActor( physx::PxRigidActor *pxActor )
        {
            m_pxActor = pxActor;
        }

        template <class T>
        bool PhysxShape<T>::isValid() const
        {
            return true;
        }

        template <class T>
        void PhysxShape<T>::setTrigger( bool trigger )
        {
            T::setTrigger( trigger );
            setupCollisionMask( getShape() );
        }

        template <class T>
        void PhysxShape<T>::setCollisionType( u32 type )
        {
            T::setCollisionType( type );
            setupCollisionMask( getShape() );
        }

        template <class T>
        void PhysxShape<T>::setCollisionMask( u32 mask )
        {
            T::setCollisionMask( mask );
            setupCollisionMask( getShape() );
        }

        template <class T>
        bool PhysxShape<T>::handleStateChanged( const SmartPtr<IStateMessage> &message )
        {
            auto type = message->getType();
            if( type == physics::IPhysicsShape::CREATE_SHAPE_HASH )
            {
                createShape();
                return true;
            }

            return false;
        }

        template <class T>
        bool PhysxShape<T>::handleStateChanged( SmartPtr<IState> &state )
        {
            if( auto stateData = state->getData() )
            {
                if( stateData->isDerived<ShapeStateData>() )
                {
                    auto shapeState = workphone::static_pointer_cast<ShapeStateData>( stateData );

                    if( auto shape = PhysxShape<T>::getShape() )
                    {
                        auto trigger =
                            BitUtil::getFlagValue( shapeState->flags, IPhysicsShape::ShapeFlagTrigger );

                        setupCollisionMask( shape, shapeState->collisionType, shapeState->collisionMask,
                                            trigger );
                    }

                    return true;
                }
            }

            return false;
        }

        template <class T>
        void PhysxShape<T>::setupCollisionMask( physx::PxShape *shape )
        {
            auto collisionType = PhysxShape<T>::getCollisionType();
            auto collisionMask = PhysxShape<T>::getCollisionMask();
            auto trigger = PhysxShape<T>::isTrigger();

            setupCollisionMask( shape, collisionType, collisionMask, trigger );
        }

        template <class T>
        void PhysxShape<T>::setupCollisionMask( physx::PxShape *shape, u32 collisionType,
                                                u32 collisionMask, bool trigger )
        {
            if( shape )
            {
                ScopedLock lock( this, true );

                if( !trigger )
                {
                    shape->setFlag( physx::PxShapeFlag::eSIMULATION_SHAPE, true );
                    shape->setFlag( physx::PxShapeFlag::eTRIGGER_SHAPE, false );
                }
                else
                {
                    shape->setFlag( physx::PxShapeFlag::eSIMULATION_SHAPE, false );
                    shape->setFlag( physx::PxShapeFlag::eTRIGGER_SHAPE, true );
                }

                physx::PxFilterData filterData;
                filterData.word0 = collisionType;
                filterData.word1 = collisionMask;

                shape->setSimulationFilterData( filterData );
                shape->setQueryFilterData( filterData );
            }
        }

        template <class T>
        void PhysxShape<T>::createStateObject()
        {
        }

        template <class T>
        void PhysxShape<T>::createShape()
        {
        }

        template <class T>
        PhysxShape<T>::ShapeStateListener::ShapeStateListener() = default;

        template <class T>
        PhysxShape<T>::ShapeStateListener::~ShapeStateListener() = default;

        template <class T>
        void PhysxShape<T>::ShapeStateListener::unload( SmartPtr<ISharedObject> data )
        {
            m_owner = nullptr;
        }

        template <class T>
        bool PhysxShape<T>::ShapeStateListener::handleStateMessage(
            const SmartPtr<IStateMessage> &message )
        {
            if( auto owner = getOwner() )
            {
                auto shape = (IPhysicsShape *)owner.get();
                return shape->handleStateChanged( message );
            }

            return false;
        }

        template <class T>
        bool PhysxShape<T>::ShapeStateListener::handleStateChanged( SmartPtr<IState> &state )
        {
            if( auto owner = getOwner() )
            {
                auto shape = (IPhysicsShape *)owner.get();
                return shape->handleStateChanged( state );
            }

            return false;
        }

        template <class T>
        SmartPtr<PhysxShape<T>> PhysxShape<T>::ShapeStateListener::getOwner() const
        {
            auto p = m_owner.load();
            return p.lock();
        }

        template <class T>
        void PhysxShape<T>::ShapeStateListener::setOwner( SmartPtr<PhysxShape<T>> owner )
        {
            m_owner = owner;
        }

        WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone, PhysxShape, T, T );

    } // end namespace physics
} // namespace workphone

#endif // WPPhysxShape_h__
