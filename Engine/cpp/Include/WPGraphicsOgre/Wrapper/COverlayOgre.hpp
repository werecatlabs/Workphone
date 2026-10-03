#ifndef _COverlay_H
#define _COverlay_H

/**
 * @file COverlayOgre.hpp
 * @brief Ogre implementation of the Overlay interface.
 *
 * Provides an Ogre-specific wrapper for overlay functionality used by the
 * engine UI and render systems. Public API mirrors the `Overlay` interface
 * while exposing access to the underlying Ogre objects via `_getObject`.
 */

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Graphics/Overlay.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Ogre implementation of the `Overlay` component.
         *
         * This class implements the engine `Overlay` interface using Ogre's
         * `Ogre::Overlay` and `Ogre::OverlayContainer`. The class acts as the
         * owner/manager of overlay elements and scene-node-attached UI.
         *
         * @remarks
         * - Raw `Ogre::` pointers are non-owning handles to objects managed by
         *   the Ogre resource/system. Lifetime and destruction are handled by
         *   the graphics system; this wrapper holds references only while valid.
         */
        class COverlayOgre : public Overlay
        {
        public:
            // String constants for commonly used property/element names
            static const String panelStr;
            static const String visibleStr;
            static const String nameStr;
            static const String zorderStr;
            static const String originStr;
            static const String zOrderStr;
            static const String scrollXStr;
            static const String scrollYStr;
            static const String scaleXStr;
            static const String scaleYStr;

            /**
             * @brief Listener that forwards state messages/changes to the overlay.
             *
             * Small helper implementing `IStateListener` to receive engine
             * state events and notify the owning `COverlayOgre` instance.
             *
             * @remarks The listener stores a non-owning pointer to its owner.
             *         Owner must ensure listener is detached before destruction.
             */
            class OverlayStateListener : public IStateListener
            {
            public:
                /**
                 * @brief Create a listener with no owner.
                 */
                OverlayStateListener();

                /**
                 * @brief Virtual destructor.
                 */
                ~OverlayStateListener() override;

                /**
                 * @brief Handle a state message forwarded by the system.
                 *
                 * @param message Smart pointer to the state message.
                 * @return true if the message was handled and should not be
                 *         further propagated; false otherwise.
                 */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

                /**
                 * @brief Notify about a state change.
                 *
                 * @param state Smart pointer to the new state.
                 * @return true if the change was handled.
                 */
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                /**
                 * @brief Get the owning `COverlayOgre` instance.
                 * @return Pointer to the owner, or nullptr if none set.
                 */
                COverlayOgre *getOwner() const;

                /**
                 * @brief Set the owning `COverlayOgre` instance.
                 * @param owner Non-owning pointer to the owner.
                 */
                void setOwner( COverlayOgre *owner );

            protected:
                /// Non-owning pointer to the overlay using this listener.
                COverlayOgre *m_owner = nullptr;
            };

            /**
             * @brief Construct an Ogre overlay wrapper.
             *
             * Initializes internal state. Does not create underlying Ogre
             * objects until `load` is called.
             */
            COverlayOgre();

            /**
             * @brief Destructor.
             *
             * Ensure any acquired Ogre resources are released or detached prior
             * to destruction. Does not assume ownership of Ogre-managed objects.
             */
            ~COverlayOgre() override;

            /**
             * @copydoc ISharedObject::load
             *
             * @remarks Expected to create/attach Ogre overlay resources using
             *          data provided in `data`.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc ISharedObject::unload
             *
             * @remarks Detaches and cleans up any references to Ogre overlay
             *          objects. Underlying Ogre resources are released by the
             *          graphics system.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Add an overlay element to this overlay.
             * @param element Element to add (smart pointer).
             */
            void addElement( SmartPtr<IOverlayElement> element ) override;

            /**
             * @brief Remove an overlay element from this overlay.
             * @param element Element to remove (smart pointer).
             * @return true if the element was found and removed.
             */
            bool removeElement( SmartPtr<IOverlayElement> element ) override;

            /**
             * @brief Attach a scene node (UI/3D) to this overlay.
             * @param sceneNode Scene node to attach.
             */
            void addSceneNode( SmartPtr<IGraphicsSceneNode> sceneNode ) override;

            /**
             * @brief Remove an attached scene node from this overlay.
             * @param sceneNode Scene node to remove.
             * @return true if the scene node was attached and removed.
             */
            bool removeSceneNode( SmartPtr<IGraphicsSceneNode> sceneNode ) override;

            /**
             * @brief Set overlay visibility.
             * @param visible true to show the overlay; false to hide it.
             */
            void setVisible( bool visible ) override;

            /**
             * @brief Query overlay visibility.
             * @return true if overlay is visible.
             */
            bool isVisible() const override;

            /**
             * @brief Set the overlay Z-order (render priority).
             * @param zorder Z-order value; higher values render on top.
             */
            void setZOrder( u32 zorder ) override;

            /**
             * @brief Get the overlay Z-order.
             * @return Current Z-order value.
             */
            u32 getZOrder() const override;

            /**
             * @brief Apply any pending Z-order changes to underlying Ogre objects.
             *
             * Call when Z-order has changed and needs to be propagated.
             */
            void updateZOrder() override;

            /**
             * @brief Retrieve an opaque pointer to the underlying Ogre object.
             *
             * @param ppObject Output pointer location. Will receive a pointer to
             *                 the internal Ogre overlay object (type depends on
             *                 implementation). Pointer is non-owning.
             */
            void _getObject( void **ppObject ) const override;

            /**
             * @brief Get the overlay elements currently managed by this overlay.
             * @return Array of smart pointers to overlay elements.
             */
            Array<SmartPtr<IOverlayElement>> getElements() const override;

            /**
             * @brief Get the absolute resolution used by the overlay.
             * @return Resolution as `Vector2I` (width, height).
             */
            Vector2I getAbsoluteResolution() const override;

            /**
             * @brief Set the absolute resolution for overlay layout calculations.
             * @param absoluteResolution Resolution as `Vector2I` (width, height).
             */
            void setAbsoluteResolution( const Vector2I &absoluteResolution ) override;

            /**
             * @copydoc ISharedObject::isValid
             *
             * @return true if the overlay and its resources are in a usable state.
             */
            bool isValid() const override;

            /**
             * @copydoc IComponent::getChildObjects
             *
             * @return Child shared objects (elements, nodes) owned/referenced by this overlay.
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @copydoc IComponent::getProperties
             *
             * @return Property container describing this overlay (may be null).
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @copydoc IComponent::setProperties
             *
             * @param properties Property container used to configure this overlay.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Non-owning pointer to the underlying Ogre overlay.
             *
             * Managed by the Ogre graphics system. Valid only while the overlay
             * is loaded and the graphics system owns it.
             */
            Ogre::Overlay *m_overlay = nullptr;

            /**
             * @brief Non-owning pointer to the root container for this overlay.
             *
             * Used to add/remove UI elements in Ogre. Lifetime is managed by Ogre.
             */
            Ogre::OverlayContainer *m_container = nullptr;

            /**
             * @brief Internal runtime state for the overlay.
             *
             * Smart pointer for ownership semantics within the engine.
             */
            SmartPtr<OverlayState> m_state;
        };
    }  // end namespace render
}  // namespace workphone

#endif
