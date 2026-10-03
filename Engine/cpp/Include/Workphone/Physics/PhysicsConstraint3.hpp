#ifndef PhysicsConstraint3_h__
#define PhysicsConstraint3_h__

#include <Workphone/Physics/PhysicsConstraint.hpp>
#include <Workphone/Interface/Physics/IPhysicsConstraint3.hpp>
#include <Workphone/Interface/Physics/IPhysicsBody3.hpp>
#include <Workphone/State/States/ConstraintStateData.hpp>
#include <Workphone/Core/Properties.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Base template implementation for 3D physics constraints.
         *
         * PhysicsConstraint3 provides the common implementation used by concrete
         * 3D constraint types. It stores and exposes the two constrained bodies,
         * local actor poses, constraint flags and projection / break force parameters
         * via the shared state (ConstraintStateData) accessed through the
         * PhysicsConstraint<T> state context.
         *
         * Template parameter T is the concrete derived constraint implementation
         * (CRTP-like registration macros are used below).
         */
        template <class T>
        class PhysicsConstraint3 : public PhysicsConstraint<T>
        {
        public:
            /** Default constructor. */
            PhysicsConstraint3();

            /** Virtual destructor. */
            ~PhysicsConstraint3() override;

            /**
             * @brief Load object data from a serialized representation.
             * @param data Shared object containing serialized data to load.
             *
             * Default implementation forwards to PhysicsConstraint<T>::load.
             * Derived classes may override to load additional fields.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload or clear runtime resources.
             * @param data Shared object used for unloading context.
             *
             * Default implementation forwards to PhysicsConstraint<T>::unload.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Get the first physics body (actor A) attached to this constraint.
             * @return SmartPtr to IPhysicsBody3 or nullptr if not set.
             *
             * The returned body pointer is read from the constraint state. This method
             * does not modify the state.
             */
            virtual SmartPtr<IPhysicsBody3> getBodyA() const;

            /**
             * @brief Set the first physics body (actor A) for this constraint.
             * @param bodyA SmartPtr to the body to attach.
             *
             * The body is written into the ConstraintStateData via an invalidate
             * call on the state context so that changes are propagated to the runtime.
             */
            virtual void setBodyA( SmartPtr<IPhysicsBody3> bodyA );

            /**
             * @brief Get the second physics body (actor B) attached to this constraint.
             * @return SmartPtr to IPhysicsBody3 or nullptr if not set.
             *
             * The returned body pointer is read from the constraint state. This method
             * does not modify the state.
             */
            virtual SmartPtr<IPhysicsBody3> getBodyB() const;

            /**
             * @brief Set the second physics body (actor B) for this constraint.
             * @param bodyB SmartPtr to the body to attach.
             *
             * The body is written into the ConstraintStateData via an invalidate
             * call on the state context so that changes are propagated to the runtime.
             */
            virtual void setBodyB( SmartPtr<IPhysicsBody3> bodyB );

            /**
             * @brief Set the local transform (pose) for the specified actor index.
             * @param actor Index identifying which actor's local pose to set.
             * @param localPose Local transform relative to the body used by the constraint.
             *
             * Writes the given transform into the ConstraintStateData's pose array.
             */
            virtual void setLocalPose( JointActorIndexEnum actor,
                                       const Transform3<real_Num> &localPose );

            /**
             * @brief Get the local transform (pose) for the specified actor index.
             * @param actor Index identifying which actor's local pose to return.
             * @return Transform3<real_Num> The stored local pose. Returns default-initialized
             *         Transform3 if no state is available.
             */
            virtual Transform3<real_Num> getLocalPose( JointActorIndexEnum actor ) const;

            /**
             * @brief Set or clear a constraint flag.
             * @param flag The flag to modify.
             * @param value true to set the flag, false to clear it.
             *
             * The flag bits are stored in ConstraintStateData::flags and modified
             * through an invalidate call so that the runtime sees the update.
             */
            virtual void setConstraintFlag( ConstraintFlagEnum flag, bool value );

            /**
             * @brief Retrieve the currently stored constraint flags bitmask.
             * @return ConstraintFlagEnum Bitmask representing active flags.
             *
             * If no state is available the method returns zero.
             */
            virtual ConstraintFlagEnum getConstraintFlags() const;

            /**
             * @brief Set the break force thresholds for the constraint.
             * @param force Linear break force threshold.
             * @param torque Angular break torque threshold.
             *
             * Break thresholds are written into the constraint state so the physics
             * backend can use them to determine when the constraint should break.
             */
            virtual void setBreakForce( real_Num force, real_Num torque );

            /**
             * @brief Get the currently configured break force thresholds.
             * @param force Output parameter set to the stored linear force threshold.
             * @param torque Output parameter set to the stored angular torque threshold.
             *
             * If no state is available the output parameters are left unchanged.
             */
            virtual void getBreakForce( real_Num &force, real_Num &torque ) const;

            /**
             * @brief Set the linear projection tolerance used by the constraint.
             * @param tolerance Linear tolerance value.
             *
             * Projection tolerances are used to correct drift between constrained bodies.
             */
            virtual void setProjectionLinearTolerance( real_Num tolerance );

            /**
             * @brief Get the configured linear projection tolerance.
             * @return real_Num The linear projection tolerance. Returns zero if state missing.
             */
            virtual real_Num getProjectionLinearTolerance() const;

            /**
             * @brief Set the angular projection tolerance used by the constraint.
             * @param tolerance Angular tolerance value.
             *
             * Projection tolerances are used to correct rotational drift between constrained bodies.
             */
            virtual void setProjectionAngularTolerance( real_Num tolerance );

            /**
             * @brief Get the configured angular projection tolerance.
             * @return real_Num The angular projection tolerance. Returns zero if state missing.
             */
            virtual real_Num getProjectionAngularTolerance() const;

            /** @copydoc Physics3SharedObject<T>::getChildObjects */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /** @copydoc Physics3SharedObject<T>::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc Physics3SharedObject<T>::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Get opaque user pointer associated with this constraint.
             * @return void* The stored user data pointer (may be nullptr).
             *
             * The pointer is not managed by this class; clients are responsible for its lifetime.
             */
            void *getUserData() const override;

            /**
             * @brief Set opaque user pointer associated with this constraint.
             * @param userData Pointer to user data. The class does not take ownership.
             */
            void setUserData( void *userData ) override;

            WP_CLASS_REGISTER_TEMPLATE_DECL( PhysicsConstraint3, T );

        protected:
            /** Opaque user data pointer (not owned). */
            void *m_userData = nullptr;
        };

        WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone::physics, PhysicsConstraint3, T, T );

        template <class T>
        PhysicsConstraint3<T>::PhysicsConstraint3()
        {
        }

        template <class T>
        PhysicsConstraint3<T>::~PhysicsConstraint3()
        {
        }

        template <class T>
        void PhysicsConstraint3<T>::load( SmartPtr<ISharedObject> data )
        {
            PhysicsConstraint<T>::load( data );
        }

        template <class T>
        void PhysicsConstraint3<T>::unload( SmartPtr<ISharedObject> data )
        {
            PhysicsConstraint<T>::unload( data );
        }

        template <class T>
        SmartPtr<IPhysicsBody3> PhysicsConstraint3<T>::getBodyA() const
        {
            if( auto stateContext = PhysicsConstraint<T>::getStateContext() )
            {
                if( auto state = stateContext->template getStateData<ConstraintStateData>() )
                {
                    auto p = state->bodyA.load();
                    return p.lock();
                }
            }

            return nullptr;
        }

        template <class T>
        void PhysicsConstraint3<T>::setBodyA( SmartPtr<IPhysicsBody3> bodyA )
        {
            if( auto stateContext = PhysicsConstraint<T>::getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<ConstraintStateData>() )
                {
                    state->bodyA = bodyA;
                }
            }
        }

        template <class T>
        SmartPtr<IPhysicsBody3> PhysicsConstraint3<T>::getBodyB() const
        {
            if( auto stateContext = PhysicsConstraint<T>::getStateContext() )
            {
                if( auto state = stateContext->template getStateData<ConstraintStateData>() )
                {
                    auto p = state->bodyB.load();
                    return p.lock();
                }
            }

            return nullptr;
        }

        template <class T>
        void PhysicsConstraint3<T>::setBodyB( SmartPtr<IPhysicsBody3> bodyB )
        {
            if( auto stateContext = PhysicsConstraint<T>::getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<ConstraintStateData>() )
                {
                    state->bodyB = bodyB;
                }
            }
        }

        template <class T>
        void PhysicsConstraint3<T>::setLocalPose( JointActorIndexEnum actor,
                                                  const Transform3<real_Num> &localPose )
        {
            if( auto stateContext = PhysicsConstraint<T>::getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<ConstraintStateData>() )
                {
                    state->poses[(u32)actor] = localPose;
                }
            }
        }

        template <class T>
        Transform3<real_Num> PhysicsConstraint3<T>::getLocalPose( JointActorIndexEnum actor ) const
        {
            if( auto stateContext = PhysicsConstraint<T>::getStateContext() )
            {
                if( auto state = stateContext->template getStateData<ConstraintStateData>() )
                {
                    return state->poses[(u32)actor];
                }
            }

            return {};
        }

        template <class T>
        void PhysicsConstraint3<T>::setConstraintFlag( ConstraintFlagEnum flag, bool value )
        {
            if( auto stateContext = PhysicsConstraint<T>::getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<ConstraintStateData>() )
                {
                    if( value )
                        state->flags |= (u32)flag;
                    else
                        state->flags &= ~(u32)flag;
                }
            }
        }

        template <class T>
        ConstraintFlagEnum PhysicsConstraint3<T>::getConstraintFlags() const
        {
            if( auto stateContext = PhysicsConstraint<T>::getStateContext() )
            {
                if( auto state = stateContext->template getStateData<ConstraintStateData>() )
                {
                    return static_cast<ConstraintFlagEnum>( state->flags );
                }
            }

            return (ConstraintFlagEnum)0;
        }

        template <class T>
        void PhysicsConstraint3<T>::setBreakForce( real_Num force, real_Num torque )
        {
            if( auto stateContext = PhysicsConstraint<T>::getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<ConstraintStateData>() )
                {
                    state->force = force;
                    state->torque = torque;
                }
            }
        }

        template <class T>
        void PhysicsConstraint3<T>::getBreakForce( real_Num &force, real_Num &torque ) const
        {
            if( auto stateContext = PhysicsConstraint<T>::getStateContext() )
            {
                if( auto state = stateContext->template getStateData<ConstraintStateData>() )
                {
                    force = state->force;
                    torque = state->torque;
                }
            }
        }

        template <class T>
        void PhysicsConstraint3<T>::setProjectionLinearTolerance( real_Num tolerance )
        {
            if( auto stateContext = PhysicsConstraint<T>::getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<ConstraintStateData>() )
                {
                    state->linearTolerance = tolerance;
                }
            }
        }

        template <class T>
        real_Num PhysicsConstraint3<T>::getProjectionLinearTolerance() const
        {
            if( auto stateContext = PhysicsConstraint<T>::getStateContext() )
            {
                if( auto state = stateContext->template getStateData<ConstraintStateData>() )
                {
                    return state->linearTolerance;
                }
            }

            return (real_Num)0;
        }

        template <class T>
        void PhysicsConstraint3<T>::setProjectionAngularTolerance( real_Num tolerance )
        {
            if( auto stateContext = PhysicsConstraint<T>::getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<ConstraintStateData>() )
                {
                    state->angularTolerance = tolerance;
                }
            }
        }

        template <class T>
        real_Num PhysicsConstraint3<T>::getProjectionAngularTolerance() const
        {
            if( auto stateContext = PhysicsConstraint<T>::getStateContext() )
            {
                if( auto state = stateContext->template getStateData<ConstraintStateData>() )
                {
                    return state->angularTolerance;
                }
            }

            return (real_Num)0;
        }

        template <class T>
        Array<SmartPtr<ISharedObject>> PhysicsConstraint3<T>::getChildObjects() const
        {
            auto objects = PhysicsConstraint<T>::getChildObjects();

            auto bodyA = PhysicsConstraint3<T>::getBodyA();
            auto bodyB = PhysicsConstraint3<T>::getBodyB();

            objects.emplace_back( bodyA );
            objects.emplace_back( bodyB );

            return objects;
        }

        template <class T>
        SmartPtr<Properties> PhysicsConstraint3<T>::getProperties() const
        {
            auto properties = PhysicsConstraint<T>::getProperties();

            auto bodyA = PhysicsConstraint3<T>::getBodyA();
            auto bodyB = PhysicsConstraint3<T>::getBodyB();

            properties->setPropertyAsType( "bodyA", bodyA );
            properties->setPropertyAsType( "bodyB", bodyB );

            return properties;
        }

        template <class T>
        void PhysicsConstraint3<T>::setProperties( SmartPtr<Properties> properties )
        {
            PhysicsConstraint<T>::setProperties( properties );

            auto bodyA = SmartPtr<IPhysicsBody3>();
            auto bodyB = SmartPtr<IPhysicsBody3>();

            properties->getPropertyAsType( "bodyA", bodyA );
            properties->getPropertyAsType( "bodyB", bodyB );
        }

        template <class T>
        void *PhysicsConstraint3<T>::getUserData() const
        {
            return m_userData;
        }

        template <class T>
        void PhysicsConstraint3<T>::setUserData( void *userData )
        {
            m_userData = userData;
        }

    }  // namespace physics
}  // namespace workphone

#endif  // PhysicsConstraint3_h__
