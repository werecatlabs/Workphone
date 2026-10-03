#ifndef RigidbodyListener_h__
#define RigidbodyListener_h__

#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Math/Transform3.hpp>
#include <Workphone/Memory/AtomicRawPtr.hpp>

namespace workphone
{
    namespace scene
    {
        /**
         * @brief Event listener that bridges physics body updates to a `Rigidbody` component.
         *
         * The `RigidbodyListener` listens for events (typically from the physics system)
         * and applies relevant updates to the associated `Rigidbody`. It can also receive
         * transform updates directly through `handleTransform`.
         *
         * Ownership:
         * - The listener holds a non-owning raw pointer to a `Rigidbody` (`m_owner`).
         *   The caller is responsible for ensuring the `Rigidbody` remains valid while
         *   this listener is registered or in use.
         *
         * @see Rigidbody
         */
        class WPCore_API RigidbodyListener : public IEventListener
        {
        public:
            /**
             * @brief Default constructor.
             *
             * Initializes the listener with no owner (`m_owner == nullptr`).
             */
            RigidbodyListener();

            /**
             * @brief Destructor.
             *
             * Does not delete or attempt to release the `Rigidbody` pointed to by
             * `m_owner` (non-owning pointer).
             */
            ~RigidbodyListener() override;

            /**
             * @brief Handle an incoming event.
             *
             * This override receives events dispatched from the event system. The
             * implementation should inspect `eventType`, `eventValue` and `arguments`
             * and perform appropriate actions on the listener's owner `Rigidbody`.
             *
             * Parameters correspond to the generic event interface:
             * @param eventType   Type/category of the event.
             * @param eventValue  Event specific hashed value (identifier).
             * @param arguments   List of event-specific parameters.
             * @param sender      The object that sent the event (may be null).
             * @param object      Optional object associated with the event (may be null).
             * @param event       The original event object (may be null).
             *
             * @return A `Parameter` describing any return value from handling the event.
             */
            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

            /**
             * @brief Called when a physics body transform has been updated.
             *
             * This function is invoked to propagate a transform from the physics engine
             * (`body`) into the engine's `Rigidbody` component by applying the supplied
             * transform `t`. Implementations should update the `Rigidbody`'s transform
             * (and any associated scene node) as required.
             *
             * @param body Pointer to the physics body whose transform changed (may be null).
             * @param t    The new transform from the physics simulation.
             */
            void handleTransform( physics::IPhysicsBody3 *body, const Transform3<real_Num> &t );

            /**
             * @brief Get the associated `Rigidbody` owner.
             *
             * Returns the non-owning pointer stored in `m_owner`.
             *
             * @return Pointer to the `Rigidbody` or nullptr if none has been set.
             */
            Rigidbody *getOwner() const;

            /**
             * @brief Set the associated `Rigidbody` owner.
             *
             * The listener stores the pointer but does not assume ownership. The caller
             * is responsible for ensuring the lifetime of `owner` covers the listener's use.
             *
             * @param owner Non-owning pointer to a `Rigidbody`.
             */
            void setOwner( Rigidbody *owner );

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Non-owning pointer to the `Rigidbody` this listener updates.
             *
             * Defaults to `nullptr`. This pointer must remain valid for the duration
             * that the listener is registered/used.
             */
            AtomicRawPtr<Rigidbody> m_owner;
        };

        inline auto RigidbodyListener::getOwner() const -> Rigidbody *
        {
            return m_owner.load();
        }

    }  // namespace scene
}  // namespace workphone

#endif  // RigidbodyListener_h__
