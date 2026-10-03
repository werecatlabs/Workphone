#ifndef WPPhysxConstraint_h__
#define WPPhysxConstraint_h__

#include <WPPhysx/WPPhysxPrerequisites.hpp>
#include <WPPhysx/WPPhysxSharedObject.hpp>
#include <WPPhysx/WPPhysxRigidDynamic.hpp>
#include <Workphone/Interface/Physics/IPhysicsConstraint3.hpp>
#include <Workphone/Interface/Physics/IPhysicsBody3.hpp>
#include <Workphone/Interface/Physics/IRigidDynamic3.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/State/States/State.hpp>
#include <Workphone/State/States/ConstraintStateData.hpp>
#include <PxJoint.h>

namespace workphone
{
    namespace physics
    {
        /**
         * @brief Base wrapper for PhysX joint constraints.
         *
         * This template class adapts a generic constraint type `T` into the
         * engine's shared-object and state-listener system. It stores a
         * pointer to a PhysX `PxJoint` and updates the joint's actor
         * references whenever the corresponding constraint state changes.
         *
         * @tparam T The derived constraint type.
         */
        template <class T>
        class PhysxConstraint : public PhysxSharedObject<T>
        {
        public:
            /**
             * @brief Construct a new PhysxConstraint.
             */
            PhysxConstraint();
            /**
             * @brief Destroy the PhysxConstraint.
             */
            ~PhysxConstraint() override;

            /**
             * @brief Handle a state message from the engine's state system.
             *
             * Override this in derived classes to respond to state messages
             * (such as events) associated with the constraint.
             *
             * @param message The state message to handle.
             */
            virtual void handleStateChanged( const SmartPtr<IStateMessage> &message );

            /**
             * @brief Handle a full state change for the constraint.
             *
             * The default implementation checks for `ConstraintStateData`
             * and updates the underlying PxJoint actors accordingly.
             *
             * @param state The new state object.
             */
            virtual void handleStateChanged( SmartPtr<IState> &state );

            /**
             * @brief Get the underlying PhysX joint.
             *
             * @return physx::PxJoint* Raw pointer to the PxJoint (may be nullptr).
             */
            physx::PxJoint *getJoint() const;

            /**
             * @brief Set the underlying PhysX joint.
             *
             * @param joint Raw pointer to a PhysX joint to use for this constraint.
             */
            void setJoint( physx::PxJoint *joint );

            WP_CLASS_REGISTER_TEMPLATE_DECL( PhysxConstraint, T );

        protected:
            /**
             * @brief State listener that forwards state notifications to the owner constraint.
             *
             * The listener holds a weak reference to the owning `PhysxConstraint`
             * to avoid reference cycles between the state system and the constraint.
             */
            class StateListener : public IStateListener
            {
            public:
                /**
                 * @brief Construct a new StateListener.
                 */
                StateListener();

                /**
                 * @brief Destroy the StateListener.
                 */
                ~StateListener();

                /**
                 * @brief Called when a state message is emitted.
                 *
                 * Forwards the message to the owner constraint if available.
                 *
                 * @param message The state message.
                 * @return true if the message was handled, false otherwise.
                 */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message );

                /**
                 * @brief Called when a state object has changed.
                 *
                 * Forwards the state change to the owner constraint if available.
                 *
                 * @param state The changed state object.
                 * @return true if the change was handled, false otherwise.
                 */
                bool handleStateChanged( SmartPtr<IState> &state );

                /**
                 * @brief Get a shared pointer to the owner constraint.
                 *
                 * Returns a strong pointer by promoting the internal weak pointer.
                 */
                SmartPtr<PhysxConstraint<T>> getOwner() const;

                /**
                 * @brief Set the owner constraint for this listener.
                 *
                 * @param owner Shared pointer to the owner constraint.
                 */
                void setOwner( SmartPtr<PhysxConstraint<T>> owner );

            private:
                AtomicWeakPtr<PhysxConstraint<T>> m_owner;
            };

            /**
             * @brief Raw pointer to the PhysX joint used by this constraint.
             *
             * This pointer is owned/managed externally (PhysX). It may be
             * nullptr when the constraint has not been initialized.
             */
            physx::PxJoint *m_joint = nullptr;
        };

        WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone::physics, PhysxConstraint, T, T );

        template <class T>
        PhysxConstraint<T>::PhysxConstraint() = default;

        template <class T>
        PhysxConstraint<T>::~PhysxConstraint() = default;

        template <class T>
        auto PhysxConstraint<T>::getJoint() const -> physx::PxJoint *
        {
            return m_joint;
        }

        template <class T>
        void PhysxConstraint<T>::setJoint( physx::PxJoint *joint )
        {
            m_joint = joint;
        }

        template <class T>
        void PhysxConstraint<T>::handleStateChanged( const SmartPtr<IStateMessage> &message )
        {
        }

        template <class T>
        void PhysxConstraint<T>::handleStateChanged( SmartPtr<IState> &state )
        {
            using namespace physx;

            auto stateData = state->getData();
            if( stateData->isDerived<ConstraintStateData>() )
            {
                auto constraintStateData =
                    workphone::static_pointer_cast<ConstraintStateData>( stateData );

                RawPtr<PxRigidActor> pxActor0;
                RawPtr<PxRigidActor> pxActor1;

                auto pActor0 = constraintStateData->bodyA.load();
                auto pActor1 = constraintStateData->bodyB.load();

                auto actor0 = pActor0.lock();
                auto actor1 = pActor1.lock();

                if( actor0 )
                {
                    if( actor0->isDerived<IRigidDynamic3>() )
                    {
                        auto pActor0 = workphone::static_pointer_cast<PhysxRigidDynamic>( actor0 );
                        pxActor0 = pActor0->getActor();
                    }
                }

                if( actor1 )
                {
                    if( actor1->isDerived<IRigidDynamic3>() )
                    {
                        auto pActor1 = workphone::static_pointer_cast<PhysxRigidDynamic>( actor1 );
                        pxActor1 = pActor1->getActor();
                    }
                }

                auto constraint = getJoint();
                if( constraint )
                {
                    constraint->setActors( pxActor0, pxActor1 );
                }
            }
        }

        template <class T>
        PhysxConstraint<T>::StateListener::StateListener()
        {
        }

        template <class T>
        PhysxConstraint<T>::StateListener::~StateListener()
        {
        }

        template <class T>
        bool PhysxConstraint<T>::StateListener::handleStateMessage(
            const SmartPtr<IStateMessage> &message )
        {
            if( auto owner = getOwner() )
            {
                // owner->handleStateMessage( message );
            }

            return false;
        }

        template <class T>
        bool PhysxConstraint<T>::StateListener::handleStateChanged( SmartPtr<IState> &state )
        {
            if( auto owner = getOwner() )
            {
                // owner->handleStateMessage( state );
            }

            return false;
        }

        template <class T>
        SmartPtr<PhysxConstraint<T>> PhysxConstraint<T>::StateListener::getOwner() const
        {
            auto p = m_owner.load();
            return p.lock();
        }

        template <class T>
        void PhysxConstraint<T>::StateListener::setOwner( SmartPtr<PhysxConstraint<T>> owner )
        {
            m_owner = owner;
        }

    } // end namespace physics
} // namespace workphone

#endif // WPPhysxConstraint_h__
