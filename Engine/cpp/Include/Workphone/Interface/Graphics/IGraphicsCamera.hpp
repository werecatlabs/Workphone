#ifndef _IGraphicsCamera_H
#define _IGraphicsCamera_H

#include <Workphone/Interface/Graphics/IFrustum.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <Workphone/Math/Ray3.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @class IGraphicsCamera
         * @brief A camera for the graphics system.
         */
        class WPCore_API IGraphicsCamera : public IFrustum
        {
        public:
            enum class CompositeBlendMode : u8
            {
                Replace,
                Alpha,
                Add,
                Multiply,
                Screen
            };

            struct PostProcessSettings
            {
                bool enabled = false;
                bool fxaa = false;
                bool bloom = false;
                f32 exposure = 0.0f;
                f32 gamma = 2.2f;
                f32 contrast = 1.0f;
                f32 saturation = 1.0f;
                f32 bloomIntensity = 0.0f;
                f32 bloomThreshold = 1.0f;
                f32 vignette = 0.0f;
                String workspace;
            };

            struct CompositeLayer
            {
                SmartPtr<ITexture> texture;
                CompositeBlendMode blendMode = CompositeBlendMode::Alpha;
                f32 opacity = 1.0f;
                bool enabled = true;
            };
            /** Camera flag to indicate rendering of UI elements. */
            static const u32 CameraFlagRenderUI;

            /** Camera flag to indicate automatic aspect ratio adjustment. */
            static const u32 CameraFlagAutoAspectRatio;

            IGraphicsCamera();

            /**
             * @brief Destructor.
             */
            ~IGraphicsCamera() override;

            /**
             * @brief Gets the direction the camera is facing.
             * @return The direction the camera is facing.
             */
            virtual Vector3<real_Num> getDirection() const = 0;

            /**
             * @brief Gets the direction from the relative screen coordinates.
             * @param screenPosition The relative screen coordinates.
             * @return The direction from the relative screen coordinates.
             */
            virtual Vector3<real_Num> getDirection( const Vector2<real_Num> &screenPosition ) const = 0;

            /**
             * @brief Gets the direction from the relative screen coordinates and returns a world
             * position.
             * @param screenPosition The relative screen coordinates.
             * @param worldPosition The world position.
             * @return The direction from the relative screen coordinates.
             */
            virtual Vector3<real_Num> getDirection( const Vector2<real_Num> &screenPosition,
                                                    Vector3<real_Num> &worldPosition ) const = 0;

            /**
             * @brief Gets the up vector of the camera.
             * @return The up vector of the camera.
             */
            virtual Vector3<real_Num> getUp() const = 0;

            /**
             * @brief Gets the right vector of the camera.
             * @return The right vector of the camera.
             */
            virtual Vector3<real_Num> getRight() const = 0;

            /**
             * @brief Sets the level-of-detail bias factor for this camera.
             * @param factor The level-of-detail bias factor.
             */
            virtual void setLodBias( f32 factor = 1.0 ) = 0;

            /**
             * @brief Returns the level-of-detail bias factor currently applied to this camera.
             * @return The level-of-detail bias factor currently applied to this camera.
             */
            virtual f32 getLodBias() const = 0;

            /**
             * @brief Gets a world space ray as cast from the camera through a viewport position.
             * @param screenx The x position at which the ray should intersect the viewport, in
             * normalized screen coordinates [0,1].
             * @param screeny The y position at which the ray should intersect the viewport, in
             * normalized screen coordinates [0,1].
             * @return The world space ray as cast from the camera through the viewport position.
             */
            virtual Ray3<real_Num> getRay( f32 screenx, f32 screeny ) const = 0;

            /**
             * @brief Gets the screen position from the given world position.
             * @param position The world position.
             * @return The screen position from the given world position.
             */
            virtual Vector2<real_Num> getScreenPosition( const Vector3<real_Num> &position ) = 0;

            /**
             * @brief Sets the viewing window inside of the viewport.
             * @param left The left edge of the window.
             * @param top The top edge of the window.
             * @param right The right edge of the window.
             * @param bottom The bottom edge of the window.
             */
            virtual void setWindow( f32 left, f32 top, f32 right, f32 bottom ) = 0;

            /**
             * @brief Gets the vertical field of view angle in degrees.
             * @return The vertical field of view angle in degrees.
             */
            virtual f32 getFOVy() const = 0;

            /**
             * @brief Sets the vertical field of view angle in degrees.
             * @param fov The vertical field of view angle in degrees.
             */
            virtual void setFOVy( f32 fov ) = 0;

            /**
             * @brief Sets whether the aspect ratio should be automatically calculated by a viewport that
             * owns this frustum whenever the frustum is resized.
             * @param autoratio Whether the aspect ratio should be automatically calculated.
             */
            virtual void setAutoAspectRatio( bool autoratio ) = 0;

            /**
             * @brief Gets whether the aspect ratio should be automatically calculated by a viewport that
             * owns this frustum whenever the frustum is resized.
             * @return Whether the aspect ratio should be automatically calculated.
             */
            virtual bool getAutoAspectRatio() const = 0;

            /**
             * @brief Sets the distance to the near clipping plane of the camera's frustum.
             * @param nearDist The distance to the near clipping plane of the camera's frustum.
             */
            void setNearClipDistance( f32 nearDist ) override = 0;

            /**
             * @brief Gets the distance to the near clipping plane of the camera's frustum.
             * @return The distance to the near clipping plane of the camera's frustum.
             */
            f32 getNearClipDistance() const override = 0;

            /**
             * @brief Sets the distance to the far clipping plane of the camera's frustum.
             * @param farDist The distance to the far clipping plane of the camera's frustum.
             */
            void setFarClipDistance( f32 farDist ) override = 0;

            /**
             * @brief Gets the distance to the far clipping plane of the camera's frustum.
             * @return The distance to the far clipping plane of the camera's frustum.
             */
            f32 getFarClipDistance() const override = 0;

            /**
             * @brief Sets the aspect ratio of the camera's frustum.
             * @param ratio The aspect ratio of the camera's frustum.
             */
            void setAspectRatio( f32 ratio ) override = 0;

            /**
             * @brief Gets the aspect ratio of the camera's frustum.
             * @return The aspect ratio of the camera's frustum.
             */
            f32 getAspectRatio() const override = 0;

            /**
             * @brief Gets the viewport that this camera is associated with.
             * @return The viewport that this camera is associated with.
             */
            virtual SmartPtr<IViewport> getViewport() const = 0;

            /**
             * @brief Sets the viewport that this camera is associated with.
             * @param viewport The viewport that this camera is associated with.
             */
            virtual void setViewport( SmartPtr<IViewport> viewport ) = 0;

            /**
             * @brief Gets the view matrix of the camera.
             * @return The view matrix of the camera.
             */
            virtual Matrix4<real_Num> getViewMatrix() const = 0;

            /**
             * @brief Gets the projection matrix of the camera.
             * @return The projection matrix of the camera.
             */
            virtual Matrix4<real_Num> getProjectionMatrix() const = 0;

            /**
             * @brief Gets a pointer to the view matrix of the camera in a format suitable for passing to
             * rendering functions.
             * @return A pointer to the view matrix of the camera in a format suitable for passing to
             * rendering functions.
             */
            virtual void *getRenderViewMatrix() const = 0;

            /**
             * @brief Gets a pointer to the projection matrix of the camera in a format suitable for
             * passing to rendering functions.
             * @return A pointer to the projection matrix of the camera in a format suitable for passing
             * to rendering functions.
             */
            virtual void *getRenderProjectionMatrix() const = 0;

            /**
             * @brief Gets the texture that the camera is rendering to.
             * @return The texture that the camera is rendering to.
             */
            virtual SmartPtr<ITexture> getTargetTexture() const = 0;

            /**
             * @brief Sets the texture that the camera should render to.
             * @param targetTexture The texture that the camera should render to.
             */
            virtual void setTargetTexture( SmartPtr<ITexture> targetTexture ) = 0;

            /** Controls the post-processing workspace and its standard effect parameters. */
            virtual PostProcessSettings getPostProcessSettings() const = 0;
            virtual void setPostProcessSettings( const PostProcessSettings &settings ) = 0;

            /** Additional render targets made available to the camera compositor, in layer order. */
            virtual Array<CompositeLayer> getCompositeLayers() const = 0;
            virtual void setCompositeLayers( const Array<CompositeLayer> &layers ) = 0;

            /**
             * @brief Sets whether UI objects should be rendered by the camera.
             * @param enabled Whether UI objects should be rendered by the camera.
             */
            virtual void setRenderUI( bool enabled ) = 0;

            /**
             * @brief Gets whether UI objects should be rendered by the camera.
             * @return Whether UI objects should be rendered by the camera.
             */
            virtual bool getRenderUI() const = 0;

            /**
             * @brief Gets the screen width of the camera.
             * @return The screen width of the camera.
             */
            virtual s32 getScreenWidth() const = 0;

            /**
             * @brief Sets the screen width of the camera.
             * @param width The screen width of the camera.
             */
            virtual void setScreenWidth( s32 width ) = 0;

            /**
             * @brief Gets the screen height of the camera.
             * @return The screen height of the camera.
             */
            virtual s32 getScreenHeight() const = 0;

            /**
             * @brief Sets the screen height of the camera.
             * @param height The screen height of the camera.
             */
            virtual void setScreenHeight( s32 height ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace render
}  // namespace workphone

#endif
