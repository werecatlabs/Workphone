#ifndef ClawHammerCamera_h__
#define ClawHammerCamera_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <WorkphoneGraphics/workphone_graphics_camera.h>
#include <Workphone/Graphics/GraphicsCamera.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @class ClawCamera
         * @brief Implementation of a graphics camera specifically for the Claw graphics engine.
         *
         * This class wraps a native wp_camera object and provides an interface consistent
         * with the Workphone GraphicsCamera system, allowing it to be integrated into
         * the scene graph and managed as a graphics object.
         */
        class WPGraphics_API ClawCamera : public GraphicsCamera
        {
        public:
            /**
             * @brief Constructs a new ClawCamera instance.
             * Initializes the underlying native wp_camera.
             */
            ClawCamera();

            /**
             * @brief Destroys the ClawCamera instance.
             * Ensures the native wp_camera is properly cleaned up.
             */
            ~ClawCamera() override;

            WP_CLASS_REGISTER_DECL;

            /** @name Camera Properties
             *  Methods for managing the camera's view and projection attributes.
             *  @{
             */
            f32 getFOVy() const override;
            void setFOVy( f32 fov ) override;
            f32 getNearClipDistance() const override;
            void setNearClipDistance( f32 nearDist ) override;
            f32 getFarClipDistance() const override;
            void setFarClipDistance( f32 farDist ) override;
            f32 getAspectRatio() const override;
            void setAspectRatio( f32 ratio ) override;
            /** @} */

            /**
             * @brief Computes the view matrix based on the current camera position and orientation.
             * @return A Matrix4 representing the view transformation.
             */
            Matrix4<real_Num> getViewMatrix() const override;

            /**
             * @brief Computes the projection matrix based on FOV, aspect ratio, and clip planes.
             * @return A Matrix4 representing the projection transformation.
             */
            Matrix4<real_Num> getProjectionMatrix() const override;

            /**
             * @brief Attaches the camera to a parent scene node.
             * @param parent The scene node to attach to.
             */
            void attachToParent( SmartPtr<IGraphicsSceneNode> parent ) override;

            /**
             * @brief Detaches the camera from its parent scene node.
             * @param parent The parent node to detach from.
             */
            void detachFromParent( SmartPtr<IGraphicsSceneNode> parent ) override;

            /**
             * @brief Retrieves the underlying native Claw camera object.
             * @return A pointer to the native wp_camera.
             */
            wp_camera *getNativeCamera() const;

            /**
             * @brief Internal method to retrieve the base object pointer.
             * @param ppObject Pointer to receive the object address.
             */
            void _getObject( void **ppObject ) const override;

            /**
             * @brief Creates a deep copy of the camera.
             * @param name Optional name for the cloned camera; defaults to empty string.
             * @return A SmartPtr to the cloned IGraphicsObject.
             */
            SmartPtr<IGraphicsObject> clone(
                const String &name = StringUtil::EmptyString ) const override;

        protected:
            /** @brief The underlying native camera implementation. */
            wp_camera *m_camera;
        };
    }  // namespace render
}  // namespace workphone

#endif  // ClawHammerCamera_h__
