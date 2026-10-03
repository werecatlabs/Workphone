#ifndef WPPhysxRigidBody3_h__
#define WPPhysxRigidBody3_h__

#include <WPPhysx/WPPhysxBody3.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Interface/Physics/IPhysicsManager.hpp>
#include <Workphone/Interface/Physics/IPhysicsShape3.hpp>
#include <Workphone/Interface/System/IStateManager.hpp>
#include <Workphone/State/Messages/StateMessageObject.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <PxRigidBody.h>
#include <WPPhysx/PhysxUtil.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Template base for PhysX rigid body wrappers.
         *
         * This class provides common functionality for rigid bodies backed by PhysX
         * and is intended to be used as a CRTP base where `T` implements higher-level
         * behaviour (for example static vs dynamic bodies).
         *
         * The class manages a PhysX `PxRigidActor` pointer and offers methods to
         * attach/detach shapes, control velocity, and apply forces/torques.
         *
         * Threading and lifetime:
         * - Many operations assume they are executed on the physics task/thread or
         *   are forwarded there via the engine's state messaging system.
         * - The class uses an atomic raw pointer to safely store the underlying
         *   `PxRigidActor` instance.
         *
         * @tparam T The derived class type (CRTP). Derived class typically stores
         *           a container of shapes in `T::m_shapes`.
         */
        template <class T>
        class PhysxRigidBody3 : public PhysxBody3<T>
        {
        public:
            /** Constructor. Creates an empty wrapper; no PhysX actor is attached. */
            PhysxRigidBody3();

            /**
             * @brief Destructor.
             *
             * Calls `unload(nullptr)` to detach state listeners and clean up engine
             * state objects. It does not release the PhysX actor pointer here �
             * actor lifetime is managed elsewhere.
             */
            ~PhysxRigidBody3() override;

            /**
             * @brief Unload and detach engine state objects associated with this body.
             *
             * This removes any registered state listeners and state context objects
             * from the engine's state manager, and unloads them. The method is safe
             * to call multiple times.
             *
             * @param data Optional shared object data (unused).
             */
            void unload( SmartPtr<ISharedObject> data );

            /**
             * @brief Attach a collision shape to this rigid body.
             *
             * The supplied shape will be associated with this body and, if required,
             * loaded and attached to the underlying PhysX actor. If the body is
             * not yet loaded this call ensures it is loaded first.
             *
             * The method will set the shape's actor pointer to this object, call
             * `shape->load()` if necessary, obtain the raw `PxShape` and attach it
             * to the `PxRigidActor`.
             *
             * @param shape Smart pointer to the shape to attach. If null, the call is a no-op.
             *
             * @note If called from a non-physics thread and the engine requires it,
             *       this operation may be queued to run on the physics task instead.
             */
            void addShape( SmartPtr<IPhysicsShape3> shape ) override;

            /**
             * @brief Detach a collision shape from this rigid body.
             *
             * Detaches the shape's `PxShape` from the underlying `PxRigidActor`,
             * unloads the shape and clears its actor pointer. If the caller is not on
             * the physics thread the detach may be posted as a state message instead.
             *
             * @param shape Smart pointer to the shape to remove. If null, the call is a no-op.
             * @param wakeOnLostTouch When true, touching objects that lost contact due
             *                        to this removal may be woken (passed to PhysX detach).
             */
            void removeShape( SmartPtr<IPhysicsShape3> shape, bool wakeOnLostTouch = true ) override;

            void setCollisionType( u32 type ) override;

            void setCollisionMask( u32 mask ) override;

            /**
             * @brief Set the linear velocity of the rigid body.
             *
             * Implementations should forward the value to the underlying PhysX actor
             * if it is dynamic. If the actor is kinematic or static this call is a no-op.
             *
             * @param linVel Linear velocity vector in world space.
             * @param autowake If true, waking behaviour is enabled when applied to a dynamic actor.
             */
            void setLinearVelocity( const Vector3<physics_Num> &linVel, bool autowake = true ) override;

            /**
             * @brief Get the current linear velocity of the rigid body.
             *
             * Returns the last known linear velocity. If the underlying actor is not available
             * or the class cannot query PhysX, a zero vector is returned.
             *
             * @return Current linear velocity in world space.
             */
            Vector3<physics_Num> getLinearVelocity() const override;

            /**
             * @brief Set the angular velocity of the rigid body.
             *
             * Implementations should forward the value to the underlying PhysX actor
             * if it is dynamic.
             *
             * @param angVel Angular velocity vector in world space (radians per second).
             * @param autowake If true, waking behaviour is enabled when applied to a dynamic actor.
             */
            void setAngularVelocity( const Vector3<physics_Num> &angVel, bool autowake = true ) override;

            /**
             * @brief Get the current angular velocity of the rigid body.
             *
             * Returns the last known angular velocity. If the underlying actor is not available
             * or the class cannot query PhysX, a zero vector is returned.
             *
             * @return Current angular velocity in world space.
             */
            Vector3<physics_Num> getAngularVelocity() const override;

            /**
             * @brief Apply a force to the rigid body.
             *
             * Forces should be applied according to the engine's force mode semantics
             * (e.g., impulse vs continuous force). Derived types should map this call
             * to the appropriate PhysX calls on `PxRigidBody`.
             *
             * @param force Force vector in world space.
             */
            void addForce( const Vector3<physics_Num> &force ) override;

            /**
             * @brief Clear accumulated forces on the body.
             *
             * @param mode Mode defining which forces to clear (Force vs Acceleration/Impulse).
             */
            void clearForce( ForceModeEnum mode = ForceModeEnum::Force ) override;

            /**
             * @brief Apply a torque to the rigid body.
             *
             * @param torque Torque vector in world space.
             */
            void addTorque( const Vector3<physics_Num> &torque ) override;

            /**
             * @brief Clear accumulated torques on the body.
             *
             * @param mode Mode defining which torques to clear.
             */
            void clearTorque( ForceModeEnum mode = ForceModeEnum::Force ) override;

            /**
             * @brief Retrieve the underlying raw object pointer.
             *
             * This returns the internal actor pointer as a raw void* to match
             * the interface expected elsewhere in the engine.
             *
             * @param object Out parameter updated with the raw actor pointer (may be null).
             */
            void _getObject( void **object ) const;

            /**
             * @brief Get the underlying PhysX actor pointer.
             *
             * @return Pointer to the underlying `physx::PxRigidActor`, or nullptr if none set.
             */
            physx::PxRigidActor *getActor() const;

            /**
             * @brief Set the underlying PhysX actor pointer.
             *
             * The class stores the pointer atomically. Ownership semantics remain
             * the responsibility of the caller/owner of the PhysX objects.
             *
             * @param actor Pointer to a `physx::PxRigidActor` (may be nullptr).
             */
            void setActor( physx::PxRigidActor *actor );

            WP_CLASS_REGISTER_TEMPLATE_DECL( PhysxRigidBody3, T );

        protected:
            /**
             * @brief Atomic raw pointer to the underlying PhysX rigid actor.
             *
             * Stored atomically to allow safe reads from multiple threads when only
             * the pointer value is accessed. The pointer refers to a PhysX-managed
             * object; this class does not own (release) the actor by default.
             */
            AtomicRawPtr<physx::PxRigidActor> m_actor;
        };

        WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone, PhysxRigidBody3, T, T );

        template <class T>
        PhysxRigidBody3<T>::PhysxRigidBody3()
        {
        }

        template <class T>
        PhysxRigidBody3<T>::~PhysxRigidBody3()
        {
            unload( nullptr );
        }

        template <class T>
        void PhysxRigidBody3<T>::unload( SmartPtr<ISharedObject> data )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto stateManager = applicationManager->getStateManager();

            if( auto stateContext = PhysxRigidBody3<T>::getStateContext() )
            {
                if( auto stateListener = PhysxRigidBody3<T>::getStateListener() )
                {
                    stateContext->removeStateListener( stateListener );
                }

                if( stateManager )
                {
                    stateManager->removeStateContext( stateContext );
                }

                stateContext->unload( nullptr );
                PhysxRigidBody3<T>::setStateContext( nullptr );
            }

            if( auto stateListener = PhysxRigidBody3<T>::getStateListener() )
            {
                stateListener->unload( nullptr );
                PhysxRigidBody3<T>::setStateListener( nullptr );
            }
        }

        template <class T>
        void PhysxRigidBody3<T>::addShape( SmartPtr<IPhysicsShape3> shape )
        {
            try
            {
                if( !this->isLoaded() )
                {
                    this->load( nullptr );
                }

                if( shape )
                {
                    ScopedLock lock( this, true );

                    auto pThis = PhysxRigidBody3<T>::template getSharedFromThis<T>();
                    shape->setActor( pThis );
                    shape->setCollisionType( PhysxRigidBody3<T>::getCollisionType() );
                    shape->setCollisionMask( PhysxRigidBody3<T>::getCollisionMask() );

                    if( !shape->isLoaded() )
                    {
                        shape->load( nullptr );
                    }

                    shape->setCollisionType( PhysxRigidBody3<T>::getCollisionType() );
                    shape->setCollisionMask( PhysxRigidBody3<T>::getCollisionMask() );

                    physx::PxShape *pShape = nullptr;
                    shape->_getObject( (void **)&pShape );

                    // WP_ASSERT( pShape );

                    if( pShape )
                    {
                        if( auto pxActor = PhysxRigidBody3<T>::getActor() )
                        {
                            WP_ASSERT( pShape->getActor() == nullptr );
                            pxActor->attachShape( *pShape );
                        }
                    }

                    T::m_shapes.push_back( shape );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        template <class T>
        void PhysxRigidBody3<T>::removeShape( SmartPtr<IPhysicsShape3> shape, bool wakeOnLostTouch )
        {
            try
            {
                if( shape )
                {
                    ScopedLock lock( this, true );

                    physx::PxShape *pShape = nullptr;
                    shape->_getObject( (void **)&pShape );

                    if( pShape )
                    {
                        if( auto pxActor = PhysxRigidBody3<T>::getActor() )
                        {
                            pxActor->detachShape( *pShape );
                        }
                    }

                    shape->unload( nullptr );
                    shape->setActor( nullptr );

                    auto it = std::find( T::m_shapes.begin(), T::m_shapes.end(), shape );
                    if( it != T::m_shapes.end() )
                    {
                        T::m_shapes.erase( it );
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        template <class T>
        void PhysxRigidBody3<T>::setCollisionType( u32 type )
        {
            T::setCollisionType( type );

            for( auto &shape : T::m_shapes.snapshot() )
            {
                if( shape )
                {
                    shape->setCollisionType( type );
                    shape->setCollisionMask( PhysxRigidBody3<T>::getCollisionMask() );
                }
            }
        }

        template <class T>
        void PhysxRigidBody3<T>::setCollisionMask( u32 mask )
        {
            T::setCollisionMask( mask );

            for( auto &shape : T::m_shapes.snapshot() )
            {
                if( shape )
                {
                    shape->setCollisionType( PhysxRigidBody3<T>::getCollisionType() );
                    shape->setCollisionMask( mask );
                }
            }
        }

        template <class T>
        void PhysxRigidBody3<T>::setLinearVelocity( const Vector3<physics_Num> &linVel, bool autowake )
        {
        }

        template <class T>
        Vector3<physics_Num> PhysxRigidBody3<T>::getLinearVelocity() const
        {
            return Vector3<physics_Num>::zero();
        }

        template <class T>
        void PhysxRigidBody3<T>::setAngularVelocity( const Vector3<physics_Num> &angVel, bool autowake )
        {
        }

        template <class T>
        Vector3<physics_Num> PhysxRigidBody3<T>::getAngularVelocity() const
        {
            return Vector3<physics_Num>::zero();
        }

        template <class T>
        void PhysxRigidBody3<T>::addForce( const Vector3<physics_Num> &force )
        {
        }

        template <class T>
        void PhysxRigidBody3<T>::clearForce( ForceModeEnum mode )
        {
        }

        template <class T>
        void PhysxRigidBody3<T>::addTorque( const Vector3<physics_Num> &torque )
        {
        }

        template <class T>
        void PhysxRigidBody3<T>::clearTorque( ForceModeEnum mode )
        {
        }

        template <class T>
        void PhysxRigidBody3<T>::_getObject( void **object ) const
        {
            auto p = PhysxRigidBody3<T>::getActor();
            *object = p;
        }

        template <class T>
        physx::PxRigidActor *PhysxRigidBody3<T>::getActor() const
        {
            auto p = m_actor.load();
            return p;
        }

        template <class T>
        void PhysxRigidBody3<T>::setActor( physx::PxRigidActor *actor )
        {
            m_actor = actor;
        }

    } // namespace physics
} // namespace workphone

#endif // WPPhysxRigidBody3_h__
