#ifndef __CViewportOgre_H
#define __CViewportOgre_H

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Graphics/Viewport.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @class CViewportOgre
         * @brief Ogre3D-based implementation of the engine Viewport interface.
         *
         * This class wraps an `Ogre::Viewport` and integrates it with the engine's
         * shared-object and state systems. It is responsible for lifecycle
         * operations (load/reload/unload), updating the viewport each frame,
         * and providing access to the underlying Ogre viewport pointer.
         *
         * Thread-safety:
         * - The underlying `Ogre::Viewport` pointer is stored in an
         *   `AtomicRawPtr` to allow safe concurrent reads/writes where appropriate.
         *
         * Usage notes:
         * - `load`, `reload` and `unload` are implemented as part of the
         *   `ISharedObject` contract (see copydoc tags on those methods).
         */
        class CViewportOgre : public Viewport
        {
        public:
            /**
             * @class ViewportStateListener
             * @brief Listen to state messages and state changes relevant to the viewport.
             *
             * This nested helper implements `IStateListener` and forwards relevant
             * state messages/changes to its owner `CViewportOgre`. The listener holds
             * a raw pointer to the owner; lifetime of the listener must not exceed
             * the owner's lifetime.
             */
            class ViewportStateListener : public IStateListener
            {
            public:
                /** Default constructor. */
                ViewportStateListener();

                /** Virtual destructor. */
                ~ViewportStateListener() override;

                /**
                 * Handle an incoming state message.
                 *
                 * @param message Smart pointer to the state message.
                 * @return true if the message was handled and no further processing is required.
                 */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

                /**
                 * Handle a state object that changed.
                 *
                 * @param state Smart pointer to the state that changed.
                 * @return true if the change was handled.
                 */
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                /**
                 * Get the current owner of this listener.
                 *
                 * @return Pointer to the owning `CViewportOgre`, or nullptr if none set.
                 */
                CViewportOgre *getOwner() const;

                /**
                 * Assign an owner to this listener.
                 *
                 * @param owner Pointer to the `CViewportOgre` that owns this listener.
                 */
                void setOwner( CViewportOgre *owner );

            protected:
                /** Raw pointer to the owning viewport. Not owned here. */
                CViewportOgre *m_owner = nullptr;
            };

            /** Default constructor. */
            CViewportOgre();

            /** Default destructor. Releases any held Ogre resources. */
            ~CViewportOgre() override;

            /** @copydoc ISharedObject::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc ISharedObject::reload */
            void reload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc ISharedObject::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * Update the viewport. Typically called each frame to apply any pending
             * changes and ensure the viewport renders correctly.
             *
             * Implementations should avoid heavy work here; it is intended for light
             * per-frame updates.
             */
            void update() override;

            /**
             * Retrieve the internal raw object pointer for interop with external systems.
             *
             * @param ppObject Output pointer that will receive the raw pointer to the underlying object.
             *                 The pointer is written as a `void*`.
             */
            void _getObject( void **ppObject ) const override;

            /**
             * Get the underlying Ogre viewport pointer.
             *
             * @note The returned pointer is owned by Ogre; callers must not delete it.
             *
             * @return Pointer to the `Ogre::Viewport`, or nullptr if none assigned.
             */
            Ogre::Viewport *getViewport() const;

            /**
             * Assign an `Ogre::Viewport` to this wrapper.
             *
             * The viewport pointer is stored atomically. The wrapper does not assume ownership
             * of the pointer in the sense of deleting it; ownership is managed by Ogre.
             *
             * @param viewport Pointer to an `Ogre::Viewport` instance (may be nullptr).
             */
            void setViewport( Ogre::Viewport *viewport );

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * Remove the associated viewport from its render target.
             *
             * Subclasses or the implementation use this to detach the viewport cleanly
             * from Ogre render targets during unload/destruction.
             */
            void removeViewportFromRT() override;

            /** Atomic container holding the raw Ogre viewport pointer used by this wrapper. */
            AtomicRawPtr<Ogre::Viewport> m_viewport;

            /**
             * Name of a background texture used by the viewport (if any).
             *
             * This can be empty when no background texture is used.
             */
            String m_backgroundTextureName;

            /** Whether this viewport is currently active. */
            bool m_active = true;

            /**
             * Extension value used to adjust or track z-order for external systems.
             *
             * Implementation-specific; static to keep a single shared value for all viewports.
             */
            static u32 m_zOrderExt;
        };
    }  // end namespace render
}  // namespace workphone

#endif
