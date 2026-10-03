#ifndef WP_RigidBody3_h__
#define WP_RigidBody3_h__

/**
 * @file RigidBody3.hpp
 * @brief Templated 3D rigid body implementation that wraps shared state accessors.
 *
 * This file provides the `RigidBody3<T>` template which implements common
 * rigid-body operations by reading and invalidating state objects. The class
 * maintains a local collection of collision shapes and forwards motion and mass
 * related modifications into state objects so a central physics system can
 * observe and apply changes.
 */

#include <Workphone/Physics/PhysicsBody3.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Interface/System/IStateManager.hpp>
#include <Workphone/State/States/RigidBodyState.hpp>
#include <Workphone/State/States/PhysicsBodyMassState.hpp>
#include <Workphone/State/States/PhysicsBodyMotionState.hpp>
#include <Workphone/State/States/BoundingBoxStateData.hpp>

namespace workphone
{
    namespace physics
    {
        /**
         * @brief Templated rigid body base class.
         *
         * RigidBody3 delegates persistent state to a state context (accessible
         * via the base `PhysicsBody3<T>`). Methods either read state data or
         * call `invalidateStateData` to indicate a mutation that should be
         * synchronized by the state manager. The class also keeps an internal
         *, thread-protected list of associated collision shapes.
         *
         * @tparam T Type used by the physics implementation (platform/engine specific).
         */
        template <class T>
        class RigidBody3 : public PhysicsBody3<T>
        {
        public:
            /** @brief Construct a RigidBody3. Defaulted implementation. */
            RigidBody3();

            /** @brief Virtual destructor. Defaulted implementation. */
            ~RigidBody3() override;

            /**
             * @brief Load resources or initialize runtime state.
             * @param data Optional initialization data (shared-object).
             *
             * Implementations may create and register state objects with the
             * global state manager here. Default implementation is empty.
             */
            void load( SmartPtr<ISharedObject> data );

            /**
             * @brief Unload runtime state and detach from the state manager.
             * @param data Optional shutdown data (shared-object).
             *
             * Removes registered state objects and listeners from the
             * application state manager and clears local references.
             */
            void unload( SmartPtr<ISharedObject> data );

            /**
             * @brief Set or clear a rigid-body flag.
             * @param flag The flag to set or clear.
             * @param value True to set the flag, false to clear it.
             *
             * The change is written into the `RigidBodyState` via
             * `invalidateStateData` so the authoritative system can pick it up.
             */
            void setRigidBodyFlag( RigidBodyFlagEnum flag, bool value ) override;

            /**
             * @brief Retrieve the current rigid-body flags.
             * @return The current flags (or zero if state is unavailable).
             */
            RigidBodyFlagEnum getRigidBodyFlags() const override;

            /**
             * @brief Add a collision shape to this rigid body.
             * @param shape Shared pointer to the shape to add.
             *
             * The container is guarded with a ScopedLock on the body instance.
             */
            void addShape( SmartPtr<IPhysicsShape3> shape );

            /**
             * @brief Remove a collision shape from this rigid body.
             * @param shape Shared pointer to the shape to remove.
             * @param wakeOnLostTouch If true the physics system may be notified to wake the body.
             *
             * The container is guarded with a ScopedLock on the body instance.
             */
            void removeShape( SmartPtr<IPhysicsShape3> shape, bool wakeOnLostTouch = true );

            /**
             * @brief Get a copy of the attached collision shapes.
             * @return Array of shared pointers to shapes.
             *
             * Returns a snapshot copy; caller modifications won't affect the internal list.
             */
            Array<SmartPtr<IPhysicsShape3>> getShapes() const;

            /**
             * @brief Get the number of attached shapes.
             * @return Number of shapes currently attached.
             */
            u32 getNumShapes() const;

            /**
             * @brief Set the linear velocity of the body.
             * @param linVel Desired linear velocity in world-space units.
             * @param autowake If true, the underlying physics body may be woken.
             *
             * Writes into `PhysicsBodyMotionState`.
             */
            void setLinearVelocity( const Vector3<real_Num> &linVel, bool autowake = true ) override;

            /**
             * @brief Get the current linear velocity.
             * @return Linear velocity from `PhysicsBodyMotionState` or zero vector if unavailable.
             */
            Vector3<real_Num> getLinearVelocity() const override;

            /**
             * @brief Set the angular velocity of the body.
             * @param angVel Desired angular velocity in world-space units (radians/sec).
             * @param autowake If true, the underlying physics body may be woken.
             *
             * Writes into `PhysicsBodyMotionState`.
             */
            void setAngularVelocity( const Vector3<real_Num> &angVel, bool autowake = true ) override;

            /**
             * @brief Get the current angular velocity.
             * @return Angular velocity from `PhysicsBodyMotionState` or zero vector if unavailable.
             */
            Vector3<real_Num> getAngularVelocity() const override;

            /**
             * @brief Accumulate a force to be applied to the body.
             * @param force Force vector in world-space units.
             *
             * Forces are accumulated into the motion state so they can be
             * applied by the physics update step.
             */
            void addForce( const Vector3<real_Num> &force ) override;

            /**
             * @brief Clear previously accumulated forces.
             * @param mode The force mode to clear (default: Force).
             *
             * Sets the clear-force flag on the motion state so the physics
             * update can reset forces at the next sync point.
             */
            void clearForce( ForceModeEnum mode = ForceModeEnum::Force ) override;

            /**
             * @brief Accumulate a torque to be applied to the body.
             * @param torque Torque vector in body/world space depending on system convention.
             */
            void addTorque( const Vector3<real_Num> &torque ) override;

            /**
             * @brief Clear previously accumulated torques.
             * @param mode The torque mode to clear (default: Force).
             *
             * Sets the clear-torque flag on the motion state so the physics
             * update can reset torques at the next sync point.
             */
            void clearTorque( ForceModeEnum mode = ForceModeEnum::Force ) override;

            /**
             * @brief Get the local axis-aligned bounding box for the body.
             * @return Local-space AABB or an empty-initialized structure if unavailable.
             */
            AABB3<real_Num> getLocalAABB() const override;

            /**
             * @brief Get the world-space axis-aligned bounding box for the body.
             * @return World-space AABB or an empty-initialized structure if unavailable.
             */
            AABB3<real_Num> getWorldAABB() const override;

            /**
             * @brief Set the center-of-mass local transform.
             * @param pose Local-space transform placing the center of mass relative to the body's
             * origin.
             *
             * Writes into `PhysicsBodyMassState`.
             */
            void setCMassLocalPose( const Transform3<real_Num> &pose ) override;

            /**
             * @brief Get the center-of-mass local transform.
             * @return Transform from `PhysicsBodyMassState` or an identity/empty transform if
             * unavailable.
             */
            Transform3<real_Num> getCMassLocalPose() const override;

            /**
             * @brief Set the mass-space inertia tensor.
             * @param m Inertia tensor components along principal axes.
             *
             * Writes into `PhysicsBodyMassState`.
             */
            void setMassSpaceInertiaTensor( const Vector3<real_Num> &m ) override;

            /**
             * @brief Get the mass-space inertia tensor.
             * @return Inertia tensor from `PhysicsBodyMassState` or zero vector if unavailable.
             */
            Vector3<real_Num> getMassSpaceInertiaTensor() const override;

            /**
             * @brief Get the inverse of the mass-space inertia tensor (componentwise).
             * @return Inverse inertia tensor or zero vector if unavailable.
             *
             * Computes safe reciprocal as 1.0 / inertia component; be mindful of zeroes.
             */
            Vector3<real_Num> getMassSpaceInvInertiaTensor() const override;

            WP_CLASS_REGISTER_TEMPLATE_DECL( RigidBody3, T );

        protected:
            /** @brief Collection of shapes attached to this rigid body. Thread-protected by ScopedLock
             * on the body. */
            ConcurrentArray<SmartPtr<IPhysicsShape3>> m_shapes;
        };

        WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone, RigidBody3, T, T );

        template <class T>
        RigidBody3<T>::RigidBody3() = default;

        template <class T>
        RigidBody3<T>::~RigidBody3() = default;

        template <class T>
        void RigidBody3<T>::load( SmartPtr<ISharedObject> data )
        {
            // Intentionally empty: specific implementations may override or
            // rely on the base PhysicsBody3 loading/registration flow.
        }

        template <class T>
        void RigidBody3<T>::unload( SmartPtr<ISharedObject> data )
        {
            // Detach any registered state listener/context from the global state manager
            auto applicationManager = core::IApplicationManager::instance();
            auto stateManager = applicationManager->getStateManager();

            if( auto stateContext = RigidBody3<T>::getStateContext() )
            {
                if( auto stateListener = RigidBody3<T>::getStateListener() )
                {
                    stateContext->removeStateListener( stateListener );
                }

                stateManager->removeStateContext( stateContext );

                stateContext->unload( nullptr );
                RigidBody3<T>::setStateContext( nullptr );
            }

            if( auto stateListener = RigidBody3<T>::getStateListener() )
            {
                stateListener->unload( nullptr );
                RigidBody3<T>::setStateListener( nullptr );
            }
        }

        template <class T>
        void RigidBody3<T>::setRigidBodyFlag( RigidBodyFlagEnum flag, bool value )
        {
            if( auto stateContext = RigidBody3<T>::getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<RigidbodyState>() )
                {
                    state->rigidBodyFlags =
                        BitUtil::setFlagValue( state->rigidBodyFlags, (u32)flag, value );
                }
            }
        }

        template <class T>
        RigidBodyFlagEnum RigidBody3<T>::getRigidBodyFlags() const
        {
            if( auto stateContext = RigidBody3<T>::getStateContext() )
            {
                if( auto state = stateContext->template getStateData<RigidbodyState>() )
                {
                    return (RigidBodyFlagEnum)state->rigidBodyFlags;
                }
            }

            return (RigidBodyFlagEnum)0;
        }

        template <class T>
        void RigidBody3<T>::addShape( SmartPtr<IPhysicsShape3> shape )
        {
            ScopedLock lock( this );
            m_shapes.push_back( shape );
        }

        template <class T>
        void RigidBody3<T>::removeShape( SmartPtr<IPhysicsShape3> shape, bool wakeOnLostTouch )
        {
            ScopedLock lock( this );
            m_shapes.erase( std::remove( m_shapes.begin(), m_shapes.end(), shape ), m_shapes.end() );
        }

        template <class T>
        Array<SmartPtr<IPhysicsShape3>> RigidBody3<T>::getShapes() const
        {
            return m_shapes.snapshot();
        }

        template <class T>
        u32 RigidBody3<T>::getNumShapes() const
        {
            ScopedLock lock( this );
            return (u32)m_shapes.size();
        }

        template <class T>
        void RigidBody3<T>::setLinearVelocity( const Vector3<real_Num> &linVel, bool autowake )
        {
            if( auto stateContext = RigidBody3<T>::getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<PhysicsBodyMotionState>() )
                {
                    state->linearVelocity = linVel;
                }
            }
        }

        template <class T>
        Vector3<real_Num> RigidBody3<T>::getLinearVelocity() const
        {
            if( auto stateContext = RigidBody3<T>::getStateContext() )
            {
                if( auto state = stateContext->template getStateData<PhysicsBodyMotionState>() )
                {
                    return state->linearVelocity;
                }
            }

            return Vector3<real_Num>::zero();
        }

        template <class T>
        void RigidBody3<T>::setAngularVelocity( const Vector3<real_Num> &angVel, bool autowake )
        {
            if( auto stateContext = RigidBody3<T>::getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<PhysicsBodyMotionState>() )
                {
                    state->angularVelocity = angVel;
                }
            }
        }

        template <class T>
        Vector3<real_Num> RigidBody3<T>::getAngularVelocity() const
        {
            if( auto stateContext = RigidBody3<T>::getStateContext() )
            {
                if( auto state = stateContext->template getStateData<PhysicsBodyMotionState>() )
                {
                    return state->angularVelocity;
                }
            }

            return Vector3<real_Num>::zero();
        }

        template <class T>
        void RigidBody3<T>::addForce( const Vector3<real_Num> &force )
        {
            if( auto stateContext = RigidBody3<T>::getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<PhysicsBodyMotionState>() )
                {
                    state->addedForce += force;
                }
            }

            if( auto applicationManager = core::IApplicationManager::instancePtr() )
            {
                if( auto physicsManager = applicationManager->getPhysicsManagerPtr() )
                {
                    physicsManager->queueDebugForce( this->getId(), this->getTransform().getPosition(),
                                                     force );
                }
            }
        }

        template <class T>
        void RigidBody3<T>::clearForce( ForceModeEnum mode )
        {
            if( auto stateContext = RigidBody3<T>::getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<PhysicsBodyMotionState>() )
                {
                    state->addedForce = Vector3<real_Num>::zero();
                    state->flags = BitUtil::setFlagValue(
                        state->flags, (u32)IPhysicsBody3::PhysicsBodyMotionFlagClearForce, true );
                }
            }
        }

        template <class T>
        void RigidBody3<T>::addTorque( const Vector3<real_Num> &torque )
        {
            if( auto stateContext = RigidBody3<T>::getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<PhysicsBodyMotionState>() )
                {
                    state->addedTorque += torque;
                }
            }
        }

        template <class T>
        void RigidBody3<T>::clearTorque( ForceModeEnum mode )
        {
            if( auto stateContext = RigidBody3<T>::getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<PhysicsBodyMotionState>() )
                {
                    state->addedTorque = Vector3<real_Num>::zero();
                    state->flags = BitUtil::setFlagValue(
                        state->flags, (u32)IPhysicsBody3::PhysicsBodyMotionFlagClearTorque, true );
                }
            }
        }

        template <class T>
        AABB3<real_Num> RigidBody3<T>::getLocalAABB() const
        {
            if( auto stateContext = RigidBody3<T>::getStateContext() )
            {
                if( auto state = stateContext->template getStateData<BoundingBoxStateData>() )
                {
                    return state->localAABB;
                }
            }

            return {};
        }

        template <class T>
        AABB3<real_Num> RigidBody3<T>::getWorldAABB() const
        {
            if( auto stateContext = RigidBody3<T>::getStateContext() )
            {
                if( auto state = stateContext->template getStateData<BoundingBoxStateData>() )
                {
                    return state->worldAABB;
                }
            }

            return {};
        }

        template <class T>
        void RigidBody3<T>::setCMassLocalPose( const Transform3<real_Num> &pose )
        {
            if( auto stateContext = RigidBody3<T>::getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<PhysicsBodyMassState>() )
                {
                    state->massSpaceLocalPose = pose;
                }
            }
        }

        template <class T>
        Transform3<real_Num> RigidBody3<T>::getCMassLocalPose() const
        {
            if( auto stateContext = RigidBody3<T>::getStateContext() )
            {
                if( auto state = stateContext->template getStateData<PhysicsBodyMassState>() )
                {
                    return state->massSpaceLocalPose;
                }
            }

            return {};
        }

        template <class T>
        void RigidBody3<T>::setMassSpaceInertiaTensor( const Vector3<real_Num> &m )
        {
            if( auto stateContext = RigidBody3<T>::getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<PhysicsBodyMassState>() )
                {
                    state->inertiaTensor = m;
                }
            }
        }

        template <class T>
        Vector3<real_Num> RigidBody3<T>::getMassSpaceInertiaTensor() const
        {
            if( auto stateContext = RigidBody3<T>::getStateContext() )
            {
                if( auto state = stateContext->template getStateData<PhysicsBodyMassState>() )
                {
                    return state->inertiaTensor;
                }
            }

            return Vector3<real_Num>::zero();
        }

        template <class T>
        Vector3<real_Num> RigidBody3<T>::getMassSpaceInvInertiaTensor() const
        {
            if( auto stateContext = RigidBody3<T>::getStateContext() )
            {
                if( auto state = stateContext->template getStateData<PhysicsBodyMassState>() )
                {
                    return real_Num( 1.0 ) / state->inertiaTensor;
                }
            }

            return Vector3<real_Num>::zero();
        }

    }  // namespace physics
}  // namespace workphone

#endif  // WP_RigidBody3_h__
