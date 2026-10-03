#ifndef CCamera_h__
#define CCamera_h__

#include <Workphone/Graphics/GraphicsObject.hpp>
#include <Workphone/Interface/Graphics/IGraphicsCamera.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @class Camera
         * @brief Represents a camera in the scene, providing view and projection transformations,
         * visibility checks, and render target management.
         *
         * The Camera class encapsulates the properties and behaviors of a camera, including its
         * orientation, field of view, aspect ratio, clipping planes, and associated render targets. It
         * provides methods to convert between screen and world coordinates, generate rays, and determine
         * object visibility within the camera's frustum. The camera can be linked to a viewport and
         * supports rendering to textures.
         */
        class WPCore_API GraphicsCamera : public GraphicsObject<IGraphicsCamera>
        {
        public:
            /**
             * @brief Default constructor.
             */
            GraphicsCamera();

            /**
             * @brief Destructor.
             */
            ~GraphicsCamera() override;

            /** @brief Loads the database manager with the specified data
             * @param data The shared object containing initialization data
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @brief Unloads the database manager and releases resources
             * @param data The shared object containing cleanup data
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Gets the camera's forward direction vector.
             * @return The direction vector.
             */
            Vector3<real_Num> getDirection() const override;

            /**
             * @brief Gets the direction vector from the camera through a given screen position.
             * @param screenPosition The 2D screen position.
             * @return The direction vector in world space.
             */
            Vector3<real_Num> getDirection( const Vector2<real_Num> &screenPosition ) const override;

            /**
             * @brief Gets the direction vector and world position from a given screen position.
             * @param screenPosition The 2D screen position.
             * @param worldPosition Output parameter for the world position.
             * @return The direction vector in world space.
             */
            Vector3<real_Num> getDirection( const Vector2<real_Num> &screenPosition,
                                            Vector3<real_Num> &worldPosition ) const override;

            /**
             * @brief Gets the camera's up vector.
             * @return The up vector.
             */
            Vector3<real_Num> getUp() const override;

            /**
             * @brief Gets the camera's right vector.
             * @return The right vector.
             */
            Vector3<real_Num> getRight() const override;

            /**
             * @brief Sets the LOD (Level of Detail) bias factor.
             * @param factor The LOD bias factor (default is 1.0).
             */
            void setLodBias( f32 factor = 1.0 ) override;

            /**
             * @brief Gets the LOD (Level of Detail) bias factor.
             * @return The LOD bias factor.
             */
            f32 getLodBias() const override;

            /**
             * @brief Generates a ray from the camera through the given screen coordinates.
             * @param screenx The X coordinate on the screen (normalized or pixel, depending on
             * implementation).
             * @param screeny The Y coordinate on the screen.
             * @return The generated ray in world space.
             */
            Ray3<real_Num> getRay( f32 screenx, f32 screeny ) const override;

            /**
             * @brief Converts a world position to a screen position.
             * @param position The world position.
             * @return The corresponding 2D screen position.
             */
            Vector2<real_Num> getScreenPosition( const Vector3<real_Num> &position ) override;

            /**
             * @brief Sets the camera's view window.
             * @param left The left coordinate.
             * @param top The top coordinate.
             * @param right The right coordinate.
             * @param bottom The bottom coordinate.
             */
            void setWindow( f32 left, f32 top, f32 right, f32 bottom ) override;

            /**
             * @brief Gets the vertical field of view (FOV) in radians or degrees (implementation
             * dependent).
             * @return The vertical FOV.
             */
            f32 getFOVy() const override;

            /**
             * @brief Sets the vertical field of view (FOV).
             * @param fov The vertical FOV.
             */
            void setFOVy( f32 fov ) override;

            /**
             * @brief Enables or disables automatic aspect ratio adjustment based on the viewport.
             * @param autoratio True to enable, false to disable.
             */
            void setAutoAspectRatio( bool autoratio ) override;

            /**
             * @brief Checks if automatic aspect ratio adjustment is enabled.
             * @return True if enabled, false otherwise.
             */
            bool getAutoAspectRatio() const override;

            /**
             * @brief Sets the near clipping distance.
             * @param nearDist The near clip distance.
             */
            void setNearClipDistance( f32 nearDist ) override;

            /**
             * @brief Gets the near clipping distance.
             * @return The near clip distance.
             */
            f32 getNearClipDistance() const override;

            /**
             * @brief Sets the far clipping distance.
             * @param farDist The far clip distance.
             */
            void setFarClipDistance( f32 farDist ) override;

            /**
             * @brief Gets the far clipping distance.
             * @return The far clip distance.
             */
            f32 getFarClipDistance() const override;

            /**
             * @brief Sets the aspect ratio (width/height) of the camera.
             * @param ratio The aspect ratio.
             */
            void setAspectRatio( f32 ratio ) override;

            /**
             * @brief Gets the aspect ratio (width/height) of the camera.
             * @return The aspect ratio.
             */
            f32 getAspectRatio() const override;

            /**
             * @brief Gets the viewport associated with this camera.
             * @return A smart pointer to the viewport.
             */
            SmartPtr<IViewport> getViewport() const override;

            /**
             * @brief Sets the viewport for this camera.
             * @param viewport A smart pointer to the viewport.
             */
            void setViewport( SmartPtr<IViewport> viewport ) override;

            /**
             * @brief Gets the camera's view matrix.
             * @return The view matrix.
             */
            Matrix4<real_Num> getViewMatrix() const override;

            /**
             * @brief Gets the camera's projection matrix.
             * @return The projection matrix.
             */
            Matrix4<real_Num> getProjectionMatrix() const override;

            /**
             * @brief Gets the raw render view matrix pointer (API-specific).
             * @return Pointer to the render view matrix.
             */
            void *getRenderViewMatrix() const override;

            /**
             * @brief Gets the raw render projection matrix pointer (API-specific).
             * @return Pointer to the render projection matrix.
             */
            void *getRenderProjectionMatrix() const override;

            /**
             * @brief Gets the target texture for rendering.
             * @return A smart pointer to the target texture.
             */
            SmartPtr<ITexture> getTargetTexture() const override;

            /**
             * @brief Sets the target texture for rendering.
             * @param targetTexture A smart pointer to the target texture.
             */
            void setTargetTexture( SmartPtr<ITexture> targetTexture ) override;

            PostProcessSettings getPostProcessSettings() const override;
            void setPostProcessSettings( const PostProcessSettings &settings ) override;

            Array<CompositeLayer> getCompositeLayers() const override;
            void setCompositeLayers( const Array<CompositeLayer> &layers ) override;

            /**
             * @brief Enables or disables UI rendering for this camera.
             * @param enabled True to enable, false to disable.
             */
            void setRenderUI( bool enabled ) override;

            /**
             * @brief Checks if UI rendering is enabled for this camera.
             * @return True if enabled, false otherwise.
             */
            bool getRenderUI() const override;

            /**
             * @brief Checks if an axis-aligned bounding box is visible in the camera's frustum.
             * @param bound The bounding box.
             * @return True if visible, false otherwise.
             */
            bool isObjectVisible( const AABB3<real_Num> &bound ) const override;

            /**
             * @brief Checks if a sphere is visible in the camera's frustum.
             * @param bound The bounding sphere.
             * @return True if visible, false otherwise.
             */
            bool isObjectVisible( const Sphere3<real_Num> &bound ) const override;

            /**
             * @brief Checks if a point is visible in the camera's frustum.
             * @param vert The point in world space.
             * @return True if visible, false otherwise.
             */
            bool isObjectVisible( const Vector3<real_Num> &vert ) const override;

            /**
             * @brief Gets the child objects of the camera.
             * @return An array of smart pointers to child objects.
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @brief Gets the screen width in pixels.
             * @return The screen width.
             */
            s32 getScreenWidth() const override;

            /**
             * @brief Sets the screen width in pixels.
             * @param width The screen width.
             */
            void setScreenWidth( s32 width ) override;

            /**
             * @brief Gets the screen height in pixels.
             * @return The screen height.
             */
            s32 getScreenHeight() const override;

            /**
             * @brief Sets the screen height in pixels.
             * @param height The screen height.
             */
            void setScreenHeight( s32 height ) override;

            WP_CLASS_REGISTER_DECL;

        private:
            bool m_renderUI = true;
        };
    }  // namespace render
}  // namespace workphone

#endif  // CCamera_h__
