#ifndef WPPhysxBody3_h__
#define WPPhysxBody3_h__

#include <WPPhysx/WPPhysxPrerequisites.hpp>
#include <WPPhysx/WPPhysxSharedObject.hpp>
#include <Workphone/Physics/PhysicsBody3.hpp>
#include <Workphone/Interface/Physics/IPhysicsBody3.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/State/States/PhysicsBodyState.hpp>
#include <Workphone/State/States/RigidbodyState.hpp>

namespace workphone
{
    namespace physics
    {
        /**
         * @brief A thin adapter that connects a physics body implementation to the
         *        Workphone physics interfaces.
         *
         * This class template wraps a concrete physics body implementation `T`
         * and exposes the `IPhysicsBody3` behavior to the rest of the engine.
         * It also listens for state changes (via `handleStateChanged`) and
         * updates the underlying PhysX scene accordingly (for example adding
         * or removing the actor when the enabled flag changes).
         *
         * @tparam T The concrete base type that implements physics body behavior.
         */
        template <class T>
        class PhysxBody3 : public T
        {
        public:
            /**
             * @brief Construct a PhysxBody3 instance.
             *
             * The constructor does minimal initialization. Any heavy setup is
             * expected to be performed by the concrete implementation `T` or
             * by factory code that creates and configures the object.
             */
            PhysxBody3();

            /**
             * @brief Destructor.
             *
             * Virtual to ensure proper cleanup of derived types.
             */
            ~PhysxBody3() override;

            /**
             * @brief Create a clone of this physics body.
             *
             * Implementations should return a new `IPhysicsBody3` instance that
             * duplicates the state of this object. The default implementation
             * returns `nullptr` which indicates cloning is unsupported.
             *
             * @return SmartPtr<IPhysicsBody3> A smart pointer to the cloned body,
             *         or `nullptr` when cloning is not supported.
             */
            SmartPtr<IPhysicsBody3> clone() override;

            /**
             * @brief Handle changes to an associated state object.
             *
             * This method is intended to be invoked when the state system
             * reports that a `IState` object has changed. The default
             * implementation inspects the state data for `PhysicsBodyState`
             * and will add/remove this body from the PhysX scene when the
             * enabled flag changes.
             *
             * @param state The state object that changed.
             */
            virtual void handleStateChanged( const SmartPtr<IState> &state );

            WP_CLASS_REGISTER_TEMPLATE_DECL( PhysxBody3, T );

        protected:
        };

        template <class T>
        PhysxBody3<T>::PhysxBody3()
        {
        }

        template <class T>
        PhysxBody3<T>::~PhysxBody3()
        {
        }

        template <class T>
        SmartPtr<IPhysicsBody3> PhysxBody3<T>::clone()
        {
            return nullptr;
        }

        template <class T>
        void PhysxBody3<T>::handleStateChanged( const SmartPtr<IState> &state )
        {
            auto stateData = state->getData();
            if( stateData->isDerived<PhysicsBodyState>() )
            {
                auto stateData = state->getData();
                auto rigidbodyState = workphone::static_pointer_cast<PhysicsBodyState>( stateData );
                if( rigidbodyState )
                {
                    if( auto scene = PhysxBody3<T>::getScene() )
                    {
                        auto enabled = BitUtil::getFlagValue( rigidbodyState->flags,
                                                              IPhysicsBody3::PhysicsBodyFlagEnabled );
                        if( enabled )
                        {
                            scene->addActor( this );
                        }
                        else
                        {
                            scene->removeActor( this );
                        }
                    }
                }
            }
        }

        WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone, PhysxBody3, T, T );
    } // namespace physics
} // namespace workphone

#endif // WPPhysxBody3_h__
