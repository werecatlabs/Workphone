#ifndef Overlay_h__
#define Overlay_h__

#include <Workphone/Interface/Graphics/IOverlay.hpp>
#include <Workphone/Graphics/SharedGraphicsObject.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @class Overlay
         * @brief Manages a collection of overlay elements and associated scene nodes.
         *
         * Overlay implements IOverlay and provides a container
         * for UI/overlay elements. It exposes operations to add and remove elements and
         * scene nodes, control visibility and z-ordering, and query or set the
         * absolute resolution used when laying out overlay elements.
         *
         * The class wraps a backend graphics object via SharedGraphicsObject<IOverlay>.
         * Use @c _getObject to obtain the raw backend pointer for integration with
         * graphics-specific APIs.
         */
        class WPCore_API Overlay : public SharedGraphicsObject<IOverlay>
        {
        public:
            /**
             * @brief Construct a new Overlay.
             *
             * The constructor initializes internal containers and prepares the
             * underlying graphics object (if any) via SharedGraphicsObject.
             */
            Overlay();

            /**
             * @brief Destroy the Overlay.
             *
             * The destructor releases owned overlay elements and cleans up any
             * references to the backend graphics object.
             */
            ~Overlay() override;

            /**
             * @brief Add an overlay element to this overlay.
             * @param element Smart pointer to an object implementing IOverlayElement.
             *
             * The element is stored in the overlay's internal element collection.
             * Ownership is held using SmartPtr semantics.
             */
            void addElement( SmartPtr<IOverlayElement> element ) override;

            /**
             * @brief Remove an overlay element from this overlay.
             * @param element Smart pointer to the element to remove.
             * @return true if the element was found and removed; false otherwise.
             */
            bool removeElement( SmartPtr<IOverlayElement> element ) override;

            /**
             * @brief Retrieve all overlay elements contained in this overlay.
             * @return Snapshot of the current overlay elements.
             *
             * The returned array is a snapshot of the internal collection; modifying
             * the returned array does not affect the overlay's internal storage.
             */
            Array<SmartPtr<IOverlayElement>> getElements() const override;

            /**
             * @brief Add a scene node to be associated with this overlay.
             * @param sceneNode Smart pointer to an IGraphicsSceneNode to attach.
             *
             * Scene nodes can be used by platform-specific overlay implementations
             * to render 3D-attached UI or to drive element transforms.
             */
            void addSceneNode( SmartPtr<IGraphicsSceneNode> sceneNode ) override;

            /**
             * @brief Remove an associated scene node.
             * @param sceneNode Smart pointer to the scene node to remove.
             * @return true if the node was found and removed; false otherwise.
             */
            bool removeSceneNode( SmartPtr<IGraphicsSceneNode> sceneNode ) override;

            /**
             * @brief Set overlay visibility.
             * @param visible true to make the overlay visible; false to hide it.
             */
            void setVisible( bool visible ) override;

            /**
             * @brief Query whether the overlay is visible.
             * @return true if visible; false otherwise.
             */
            bool isVisible() const override;

            /**
             * @brief Set the z-order for this overlay.
             * @param zorder Z-order value; higher values are drawn on top.
             *
             * The exact interpretation of z-order depends on the rendering backend.
             */
            void setZOrder( u32 zorder ) override;

            /**
             * @brief Get the current z-order value.
             * @return Current z-order.
             */
            u32 getZOrder() const override;

            /**
             * @brief Recalculate or propagate z-order state to the backend.
             *
             * Call this when element or overlay ordering changes and the backend
             * needs to be updated to reflect the new ordering.
             */
            void updateZOrder() override;

            /**
             * @brief Get the absolute resolution used for layout.
             * @return Absolute resolution (width, height) in pixels.
             *
             * This resolution is used by overlay elements to compute positions and
             * sizes in screen-space.
             */
            Vector2I getAbsoluteResolution() const override;

            /**
             * @brief Set the absolute resolution used for layout.
             * @param absoluteResolution Resolution to use (width, height) in pixels.
             *
             * Changing the resolution may require elements to be relaid out.
             */
            void setAbsoluteResolution( const Vector2I &absoluteResolution ) override;

            /**
             * @brief Retrieve the underlying backend graphics object pointer.
             * @param ppObject Output parameter that receives the raw backend overlay pointer.
             *
             * The pointer type depends on the active renderer. Ownership is not
             * transferred to the caller.
             */
            void _getObject( void **ppObject ) const override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Internal storage of overlay elements.
             *
             * Elements are stored as SmartPtr<IOverlayElement> instances and exposed
             * through getElements() as snapshots.
             */
            Array<SmartPtr<IOverlayElement>> m_elements;
        };

    }  // namespace render
}  // namespace workphone

#endif  // Overlay_h__
