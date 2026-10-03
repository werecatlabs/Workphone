#ifndef FBCPhysicsBody3_h__
#define FBCPhysicsBody3_h__

#include <Workphone/Interface/Physics/IPhysicsBody3.hpp>
#include <Workphone/Interface/Physics/IPhysicsScene3.hpp>
#include <Workphone/Interface/Physics/IPhysicsShape3.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/State/States/PhysicsBodyState.hpp>
#include <Workphone/State/States/RigidbodyState.hpp>
#include <Workphone/Physics/Physics3SharedObject.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/UnorderedMap.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Base template implementation for a 3D physics body.
         *
         * PhysicsBody3<T> is a light-weight, shared implementation of the
         * IPhysicsBody3 interface that stores common state (scene ownership,
         * user data, flags, mass, transform, collision filtering, etc.) via
         * an engine state context (PhysicsBodyState). Concrete backends (T)
         * should derive from this template to provide engine-specific
         * integration (e.g. creating native physics actors, applying forces).
         *
         * The class interacts with a state context to read or invalidate the
         * current state data. Many setters call invalidateStateData so that
         * the backend can pick up changes and apply them to the native
         * physics representation.
         *
         * @tparam T Backend implementation type that derives from this template.
         */
        template <class T>
        class PhysicsBody3 : public Physics3SharedObject<T>
        {
        public:
            /**
             * @brief Default constructor.
             *
             * Initializes an empty physics body. Does not create any native
             * physics object; backend-specific setup happens in derived types.
             */
            PhysicsBody3();

            /**
             * @brief Virtual destructor.
             *
             * Derived backends should handle native resource cleanup. This
             * destructor ensures correct polymorphic destruction.
             */
            ~PhysicsBody3() override;

            /**
             * @brief Get the scene this body belongs to.
             * @return Smart pointer to the associated IPhysicsScene3 or nullptr
             *         if the body is not attached to a scene.
             */
            SmartPtr<IPhysicsScene3> getScene() const override;

            /**
             * @brief Attach the body to a physics scene.
             *
             * Attaching a body to a scene does not guarantee that a native
             * actor is created immediately — backends decide when to create or
             * move native actors into the scene.
             *
             * @param scene Smart pointer to the scene to associate with the body.
             */
            void setScene( SmartPtr<IPhysicsScene3> scene ) override;

            /**
             * @brief Set the world transform for this body.
             *
             * The transform is stored in the shared state (PhysicsBodyState).
             * Backends should observe changes and update the native actor's
             * transform as required.
             *
             * @param transform New transform to apply to the body.
             */
            void setTransform( const Transform3<real_Num> &transform ) override;

            /**
             * @brief Get the world transform stored in the shared state.
             *
             * If the state context or state data is not available, returns a
             * default-initialized transform.
             *
             * @return Current transform of the body.
             */
            Transform3<real_Num> getTransform() const override;

            /**
             * @brief Set or clear an actor-level flag.
             *
             * Flags control behaviour such as whether the body is kinematic,
             * enabled, etc. Flags are stored in the PhysicsBodyState and the
             * backend should apply them to the native actor when detected.
             *
             * @param flag The actor flag to modify.
             * @param value True to set the flag, false to clear it.
             */
            void setActorFlag( ActorFlagEnum flag, bool value ) override;

            /**
             * @brief Get the combined actor flags for this body.
             * @return Current actor flags as an ActorFlagEnum bitset.
             */
            ActorFlagEnum getActorFlags() const override;

            /**
             * @brief Get the body's mass from the shared state.
             * @return Mass value or 0.0 if state data is unavailable.
             */
            real_Num getMass() const override;

            /**
             * @brief Set the body's mass.
             *
             * The value is stored in state so that the physics backend can
             * recompute mass/inertia when necessary.
             *
             * @param mass New mass for the body.
             */
            void setMass( real_Num mass ) override;

            /**
             * @brief Set the collision type/category bits for this body.
             *
             * Collision type is used by the engine's collision filtering to
             * determine which groups an object belongs to.
             *
             * @param type Bitmask representing the collision category (u32).
             */
            void setCollisionType( u32 type ) override;

            /**
             * @brief Get the collision type/category for this body.
             * @return Collision type bitmask or 0 if state is unavailable.
             */
            u32 getCollisionType() const override;

            /**
             * @brief Set the collision mask for this body.
             *
             * The mask determines which collision types this body will collide
             * with (bitwise AND with other bodies' types).
             *
             * @param mask Collision mask bitmask.
             */
            void setCollisionMask( u32 mask ) override;

            /**
             * @brief Get the collision mask for this body.
             * @return Collision mask bitmask or 0 if state is unavailable.
             */
            u32 getCollisionMask() const override;

            /**
             * @brief Enable or disable the body.
             *
             * Disabling a body typically prevents simulation and/or collision
             * processing. The flag is stored in shared state so the backend
             * can respond appropriately.
             *
             * @param enabled True to enable, false to disable.
             */
            void setEnabled( bool enabled ) override;

            /**
             * @brief Check whether the body is enabled.
             * @return True if enabled, false otherwise.
             */
            bool isEnabled() const override;

            /**
             * @brief Get the generic user data pointer associated with this body.
             * @return Void pointer previously set with setUserData or nullptr.
             * @note The class does not manage the memory of user data.
             */
            void *getUserData() const override;

            /**
             * @brief Set a generic user data pointer for this body.
             *
             * The pointer is stored verbatim. Ownership semantics are the caller's
             * responsibility.
             *
             * @param userData Pointer to associate with the body.
             */
            void setUserData( void *userData ) override;

            /**
             * @brief Retrieve user data by a numeric id.
             *
             * This map-based user data storage allows multiple entries to be
             * associated with a single body (indexed by id).
             *
             * @param id Identifier for the user data entry.
             * @return Stored pointer or nullptr if not found.
             */
            void *getUserDataById( u32 id ) const override;

            /**
             * @brief Store user data associated with the given id.
             *
             * Overwrites any existing entry for the id.
             *
             * @param id Identifier to store the data under.
             * @param userData Pointer to store.
             */
            void setUserDataById( u32 id, void *userData ) override;

            /**
             * @brief Query whether the body is in kinematic mode.
             *
             * Kinematic bodies are typically moved by the user (setTransform)
             * and are not affected by forces from the physics simulation.
             *
             * @return True if kinematic, false otherwise.
             */
            bool getKinematicMode() const override;

            /**
             * @brief Set or clear the kinematic mode for this body.
             *
             * The backend should update the native actor to reflect the change.
             *
             * @param kinematicMode True to make the body kinematic.
             */
            void setKinematicMode( bool kinematicMode ) override;

            /**
             * @brief Clone the physics body.
             *
             * Default implementation returns nullptr. Backends that support
             * cloning should override and provide a deep copy including shapes,
             * mass properties and relevant state.
             *
             * @return Smart pointer to a cloned IPhysicsBody3 or nullptr.
             */
            SmartPtr<IPhysicsBody3> clone() override;

            /**
             * @brief Wake up the body (take it out of a sleeping state).
             *
             * Default implementation is a no-op. Backends should override to
             * call their native wake/sleep APIs.
             */
            void wakeUp() override;

            /**
             * @brief Get engine-specific properties attached to the body.
             *
             * Properties provide an extensible key/value store for engine or
             * gameplay metadata. Default returns nullptr.
             *
             * @return SmartPtr to a Properties object or nullptr.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Set engine-specific properties on the body.
             *
             * Default implementation does nothing; backends can override to
             * persist or propagate properties to native objects.
             *
             * @param properties Properties object to associate with the body.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            WP_CLASS_REGISTER_TEMPLATE_DECL( PhysicsBody3, T );

        protected:
            /**
             * @brief The physics scene this body is attached to.
             *
             * AtomicSmartPtr is used because scene pointer may be read/written
             * from multiple threads (main thread and physics worker threads).
             */
            AtomicSmartPtr<IPhysicsScene3> m_scene;

            /**
             * @brief Generic user data pointer for the body.
             *
             * Not owned by this class; lifecycle is caller-managed.
             */
            void *m_userData = nullptr;

            /**
             * @brief Map of user data pointers indexed by id.
             *
             * Allows multiple pieces of user data to be attached to a body.
             */
            UnorderedMap<u32, void *> m_userDataMap;
        };

        template <class T>
        PhysicsBody3<T>::PhysicsBody3()
        {
        }

        template <class T>
        PhysicsBody3<T>::~PhysicsBody3()
        {
        }

        template <class T>
        SmartPtr<IPhysicsScene3> PhysicsBody3<T>::getScene() const
        {
            return m_scene;
        }

        template <class T>
        void PhysicsBody3<T>::setScene( SmartPtr<IPhysicsScene3> scene )
        {
            m_scene = scene;
        }

        template <class T>
        void PhysicsBody3<T>::setTransform( const Transform3<real_Num> &transform )
        {
            if( auto stateContext = PhysicsBody3<T>::getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<PhysicsBodyState>() )
                {
                    state->transform = transform;
                }
            }
        }

        template <class T>
        Transform3<real_Num> PhysicsBody3<T>::getTransform() const
        {
            if( auto stateContext = PhysicsBody3<T>::getStateContext() )
            {
                if( auto state = stateContext->template getStateData<PhysicsBodyState>() )
                {
                    WP_ASSERT( state->transform.isSane() );
                    return state->transform;
                }
            }

            return {};
        }

        template <class T>
        void PhysicsBody3<T>::setActorFlag( ActorFlagEnum flag, bool value )
        {
            if( auto stateContext = PhysicsBody3<T>::getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<PhysicsBodyState>() )
                {
                    state->actorFlags = BitUtil::setFlagValue( state->actorFlags, (u32)flag, value );
                }
            }
        }

        template <class T>
        ActorFlagEnum PhysicsBody3<T>::getActorFlags() const
        {
            if( auto stateContext = PhysicsBody3<T>::getStateContext() )
            {
                if( auto state = stateContext->template getStateData<PhysicsBodyState>() )
                {
                    return static_cast<ActorFlagEnum>( state->actorFlags );
                }
            }

            return static_cast<ActorFlagEnum>( 0 );
        }

        template <class T>
        real_Num PhysicsBody3<T>::getMass() const
        {
            if( auto stateContext = PhysicsBody3<T>::getStateContext() )
            {
                if( auto rigidbodyState = stateContext->template getStateData<PhysicsBodyState>() )
                {
                    return rigidbodyState->mass;
                }
            }

            return static_cast<real_Num>( 0.0 );
        }

        template <class T>
        void PhysicsBody3<T>::setMass( real_Num mass )
        {
            if( auto stateContext = PhysicsBody3<T>::getStateContext() )
            {
                if( auto rigidbodyState =
                        stateContext->template invalidateStateData<PhysicsBodyState>() )
                {
                    rigidbodyState->mass = mass;
                }
            }
        }

        template <class T>
        void PhysicsBody3<T>::setCollisionType( u32 type )
        {
            if( auto stateContext = PhysicsBody3<T>::getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<PhysicsBodyState>() )
                {
                    state->collisionType = type;
                }
            }
        }

        template <class T>
        u32 PhysicsBody3<T>::getCollisionType() const
        {
            if( auto stateContext = PhysicsBody3<T>::getStateContext() )
            {
                if( auto state = stateContext->template getStateData<PhysicsBodyState>() )
                {
                    return state->collisionType;
                }
            }

            return 0;
        }

        template <class T>
        void PhysicsBody3<T>::setCollisionMask( u32 mask )
        {
            if( auto stateContext = PhysicsBody3<T>::getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<PhysicsBodyState>() )
                {
                    state->collisionMask = mask;
                }
            }
        }

        template <class T>
        u32 PhysicsBody3<T>::getCollisionMask() const
        {
            if( auto stateContext = PhysicsBody3<T>::getStateContext() )
            {
                if( auto state = stateContext->template getStateData<PhysicsBodyState>() )
                {
                    return state->collisionMask;
                }
            }

            return 0;
        }

        template <class T>
        void PhysicsBody3<T>::setEnabled( bool enabled )
        {
            if( auto stateContext = PhysicsBody3<T>::getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<PhysicsBodyState>() )
                {
                    state->flags = BitUtil::setFlagValue(
                        state->flags, IPhysicsBody3::PhysicsBodyFlagEnabled, enabled );
                }
            }
        }

        template <class T>
        bool PhysicsBody3<T>::isEnabled() const
        {
            if( auto stateContext = PhysicsBody3<T>::getStateContext() )
            {
                if( auto state = stateContext->template getStateData<PhysicsBodyState>() )
                {
                    return BitUtil::getFlagValue( state->flags, IPhysicsBody3::PhysicsBodyFlagEnabled );
                }
            }

            return false;
        }

        template <class T>
        void *PhysicsBody3<T>::getUserData() const
        {
            return m_userData;
        }

        template <class T>
        void PhysicsBody3<T>::setUserData( void *userData )
        {
            m_userData = userData;
        }

        template <class T>
        void *PhysicsBody3<T>::getUserDataById( u32 id ) const
        {
            auto it = m_userDataMap.find( id );
            if( it != m_userDataMap.end() )
            {
                return it->second;
            }

            return nullptr;
        }

        template <class T>
        void PhysicsBody3<T>::setUserDataById( u32 id, void *userData )
        {
            m_userDataMap[id] = userData;
        }

        template <class T>
        bool PhysicsBody3<T>::getKinematicMode() const
        {
            if( auto stateContext = PhysicsBody3<T>::getStateContext() )
            {
                if( auto state = stateContext->template getStateData<PhysicsBodyState>() )
                {
                    return BitUtil::getFlagValue( state->flags,
                                                  IPhysicsBody3::PhysicsBodyFlagKinematic );
                }
            }

            return false;
        }

        template <class T>
        void PhysicsBody3<T>::setKinematicMode( bool kinematicMode )
        {
            if( auto stateContext = PhysicsBody3<T>::getStateContext() )
            {
                if( auto state = stateContext->template invalidateStateData<PhysicsBodyState>() )
                {
                    state->flags = BitUtil::setFlagValue(
                        state->flags, IPhysicsBody3::PhysicsBodyFlagKinematic, kinematicMode );
                }
            }
        }

        template <class T>
        SmartPtr<IPhysicsBody3> PhysicsBody3<T>::clone()
        {
            return nullptr;
        }

        template <class T>
        void PhysicsBody3<T>::wakeUp()
        {
        }

        template <class T>
        SmartPtr<Properties> PhysicsBody3<T>::getProperties() const
        {
            return nullptr;
        }

        template <class T>
        void PhysicsBody3<T>::setProperties( SmartPtr<Properties> properties )
        {
        }

        WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone, PhysicsBody3, T, T );
    }  // namespace physics
}  // namespace workphone

#endif  // FBCPhysicsBody3_h__
