#ifndef CTextureOgreStateListener_h__
#define CTextureOgreStateListener_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Graphics/Texture.hpp>
#include <OgreTexture.h>

namespace workphone
{
    namespace render
    {

        /**
         * @class CTextureOgreStateListener
         * @brief State listener for Ogre-backed texture wrapper objects.
         *
         * This class implements the engine's IStateListener interface and is
         * intended to be attached to a `CTextureOgre` instance. It receives
         * state messages and state-changed notifications from the engine's
         * state system and performs appropriate handling such as forwarding
         * notifications to the owning `CTextureOgre` or performing cleanup
         * actions when an unload is requested.
         *
         * The listener stores a weak reference to its owner to avoid strong
         * ownership cycles. The owner may be set or queried via `setOwner`
         * / `getOwner`.
         *
         * @see CTextureOgre
         */
        class CTextureOgreStateListener : public IStateListener
        {
        public:
            /**
             * @brief Construct a new state listener.
             *
             * Initializes internal state. Does not assume ownership of any
             * `CTextureOgre` until `setOwner` is called.
             */
            CTextureOgreStateListener();

            /**
             * @brief Virtual destructor.
             *
             * Ensures derived cleanup runs correctly when the listener is
             * destroyed. Does not forcibly unload the owner; any required
             * owner cleanup should be initiated by the owner or through the
             * `unload` callback.
             */
            ~CTextureOgreStateListener() override;

            /**
             * @brief Invoked when the listener should release runtime resources.
             *
             * The engine calls `unload` to notify listeners they should release
             * any runtime resources or detach from external systems. Implementations
             * may forward the request to the owner texture or perform listener-local
             * cleanup. `data` may contain additional unload parameters.
             *
             * @param data Optional shared object containing unload parameters.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Handle an incoming state message.
             *
             * Called by the state system when a specific state message is sent.
             * Implementations should inspect `message` and act accordingly.
             *
             * @param message The state message received.
             * @return true if the message was handled and should not be propagated further; false otherwise.
             */
            bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

            /**
             * @brief Handle a state-changed notification.
             *
             * Called when an observed `IState` instance has changed. The listener
             * can react to changes (for example, reload or update GPU resources).
             *
             * @param state The state object that changed.
             * @return true if the change was handled; false to allow further processing.
             */
            bool handleStateChanged( SmartPtr<IState> &state ) override;

            /**
             * @brief Get the owner `CTextureOgre`.
             *
             * Returns a strong pointer to the owning texture if it is still alive.
             * The internal storage is a weak pointer to avoid reference cycles,
             * so the returned `SmartPtr` may be null if the owner has been destroyed.
             *
             * @return SmartPtr<CTextureOgre> Strong pointer to the owner or null.
             */
            SmartPtr<CTextureOgre> getOwner() const;

            /**
             * @brief Set the owner `CTextureOgre`.
             *
             * Assigns the owning texture for this listener. The listener will keep
             * a weak reference to the owner to avoid creating ownership cycles.
             *
             * @param owner Smart pointer to the texture that owns this listener.
             */
            void setOwner( SmartPtr<CTextureOgre> owner );

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Weak reference to the owning `CTextureOgre`.
             *
             * Stored as an `AtomicWeakPtr` so the listener can safely observe the
             * owner without preventing its destruction. Before using the owner,
             * callers should obtain a strong `SmartPtr` via `getOwner()` and
             * verify it is non-null.
             */
            AtomicWeakPtr<CTextureOgre> m_owner;
        };

    }  // namespace render
}  // namespace workphone

#endif  // CTextureOgreStateListener_h__
