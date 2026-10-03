#ifndef _COverlay_H
#define _COverlay_H

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Interface/Graphics/IOverlay.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Graphics/SharedGraphicsObject.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>

namespace workphone
{
    namespace render
    {

        /** @brief OgreNext implementation of the IOverlay interface. */
        class COverlayOgreNext : public SharedGraphicsObject<IOverlay>
        {
        public:
            /** @brief Listener used to forward state events to the owning overlay. */
            class OverlayStateListener : public IStateListener
            {
            public:
                /** @brief Constructor. */
                OverlayStateListener();

                /** @brief Destructor. */
                ~OverlayStateListener() override;

                /** @copydoc ISharedObject::unload */
                void unload( SmartPtr<ISharedObject> data ) override;

                /** @copydoc IStateListener::handleStateMessage */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;
                /** @copydoc IStateListener::handleStateChanged */
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                /** @brief Gets the owning overlay instance. */
                COverlayOgreNext *getOwner() const;

                /** @brief Sets the owning overlay instance. */
                void setOwner( COverlayOgreNext *owner );

                WP_CLASS_REGISTER_DECL;

            protected:
                /// Non-owning pointer to the parent overlay.
                COverlayOgreNext *m_owner = nullptr;
            };

            /** @brief Constructor. */
            COverlayOgreNext();

            /** @brief Destructor. */
            ~COverlayOgreNext() override;

            /** @copydoc ISharedObject::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc ISharedObject::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @brief Updates the overlay each frame. */
            void update() override;

            /** @brief Adds an overlay element to this overlay. */
            void addElement( SmartPtr<IOverlayElement> element ) override;

            /** @brief Removes an overlay element from this overlay. */
            bool removeElement( SmartPtr<IOverlayElement> element ) override;

            /** @brief Adds a scene node to this overlay. */
            void addSceneNode( SmartPtr<IGraphicsSceneNode> sceneNode ) override;

            /** @brief Removes a scene node from this overlay. */
            bool removeSceneNode( SmartPtr<IGraphicsSceneNode> sceneNode ) override;

            /** @brief Sets whether the overlay is visible. */
            void setVisible( bool isVisible ) override;

            /** @brief Returns whether the overlay is visible. */
            bool isVisible() const override;

            /** @brief Sets the overlay z-order. */
            void setZOrder( u32 zorder ) override;

            /** @brief Returns the overlay z-order. */
            u32 getZOrder() const override;

            /** @brief Applies z-order changes to the native overlay object. */
            void updateZOrder();

            /** @brief Gets the underlying native object pointer. */
            void _getObject( void **ppObject ) const override;

            /** @brief Returns the overlay elements attached to this overlay. */
            Array<SmartPtr<IOverlayElement>> getElements() const override;

            /** @brief Returns the absolute resolution used by this overlay. */
            Vector2I getAbsoluteResolution() const override;

            /** @brief Sets the absolute resolution used by this overlay. */
            void setAbsoluteResolution( const Vector2I &absoluteResolution ) override;

            /** @copydoc ISharedObject::isValid */
            bool isValid() const override;

            /** @copydoc IComponent::getChildObjects */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /** @copydoc IComponent::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc IComponent::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /// The Ogre overlay object.
            Ogre::v1::Overlay *m_overlay = nullptr;

            /// The root Ogre overlay container.
            Ogre::v1::OverlayContainer *m_container = nullptr;

            /// Thread-safe collection of overlay elements.
            ConcurrentArray<SmartPtr<IOverlayElement>> m_elements;
        };
    }  // end namespace render
}  // namespace workphone

#endif
