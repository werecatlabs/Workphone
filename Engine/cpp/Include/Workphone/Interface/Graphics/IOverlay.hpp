#ifndef _IOverlay_H
#define _IOverlay_H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @class IOverlay
         * @brief Abstract interface representing a UI overlay container.
         *
         * Implementations manage a collection of overlay elements
         * and optional scene nodes that can be used to attach 3D objects or
         * transforms. Overlays expose operations for element lifecycle,
         * visibility, z-order management, and resolution-dependent layout.
         */
        class WPCore_API IOverlay : public ISharedObject
        {
        public:
            /// State message identifier used to attach an object to the overlay.
            static const hash_type STATE_MESSAGE_ATTACH_OBJECT;

            /// State message identifier used to detach an object from the overlay.
            static const hash_type STATE_MESSAGE_DETACH_OBJECT;

            /// State message identifier used to detach all objects from the overlay.
            static const hash_type STATE_MESSAGE_DETACH_ALL_OBJECTS;

            /**
             * @brief Virtual destructor.
             */
            ~IOverlay() override;

            /**
             * @brief Add an overlay element to this overlay.
             * @param element Smart pointer to the element to add.
             */
            virtual void addElement( SmartPtr<IOverlayElement> element ) = 0;

            /**
             * @brief Remove an overlay element from this overlay.
             * @param element Smart pointer to the element to remove.
             * @return true if the element was found and removed; false otherwise.
             */
            virtual bool removeElement( SmartPtr<IOverlayElement> element ) = 0;

            /**
             * @brief Get all overlay elements currently contained by this overlay.
             * @return Snapshot of the overlay elements currently registered with this overlay.
             */
            virtual Array<SmartPtr<IOverlayElement>> getElements() const = 0;

            /**
             * @brief Associate a scene node with this overlay.
             * @param sceneNode Smart pointer to the scene node to add.
             */
            virtual void addSceneNode( SmartPtr<IGraphicsSceneNode> sceneNode ) = 0;

            /**
             * @brief Remove an associated scene node from this overlay.
             * @param sceneNode Smart pointer to the node to remove.
             * @return true if the node was removed; false otherwise.
             */
            virtual bool removeSceneNode( SmartPtr<IGraphicsSceneNode> sceneNode ) = 0;

            /**
             * @brief Set whether this overlay is visible.
             * @param visible True to show the overlay; false to hide it.
             */
            virtual void setVisible( bool visible ) = 0;

            /**
             * @brief Query overlay visibility.
             * @return true if visible; false otherwise.
             */
            virtual bool isVisible() const = 0;

            /**
             * @brief Set the Z-order for this overlay.
             * @param zorder Z-order value; larger values typically render on top.
             */
            virtual void setZOrder( u32 zorder ) = 0;

            /**
             * @brief Get current Z-order value.
             * @return Current z-order.
             */
            virtual u32 getZOrder() const = 0;

            /**
             * @brief Propagate or recalculate overlay and element z-ordering.
             */
            virtual void updateZOrder() = 0;

            /**
             * @brief Get the absolute layout resolution used by this overlay.
             * @return Vector2I Resolution in pixels (width, height).
             */
            virtual Vector2I getAbsoluteResolution() const = 0;

            /**
             * @brief Set the absolute layout resolution for this overlay.
             * @param absoluteResolution Resolution in pixels (width, height).
             */
            virtual void setAbsoluteResolution( const Vector2I &absoluteResolution ) = 0;

            /**
             * @brief Retrieve a pointer to the underlying backend object.
             * @param ppObject Output pointer that receives the backend object pointer.
             *
             * The pointer type depends on the active renderer. Ownership is not
             * transferred to the caller.
             */
            virtual void _getObject( void **ppObject ) const = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif
