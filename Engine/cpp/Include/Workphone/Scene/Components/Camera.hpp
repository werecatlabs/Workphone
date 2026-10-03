#ifndef CameraComponent_h__
#define CameraComponent_h__

#include <Workphone/Scene/Components/Component.hpp>
#include <Workphone/Math/Ray3.hpp>
#include <Workphone/Core/ColourF.hpp>
#include <Workphone/Interface/Graphics/IGraphicsCamera.hpp>

namespace workphone
{
    namespace scene
    {
        class RenderTexture;

        /**
         * @brief Camera component
         *
         * The camera component represents a viewpoint inside the scene and is
         * responsible for rendering the scene from that viewpoint. It manages
         * an underlying render camera, an optional scene node, a viewport and
         * render target textures used for game rendering and editor previews.
         *
         * Responsibilities:
         * - Provide accessors to the render `IGraphicsCamera`, `IGraphicsSceneNode`, `IViewport`
         *   and render target textures used when drawing the scene.
         * - Control activation state and z-order used to order camera rendering.
         * - Support rendering to an off-screen texture (editor or target texture)
         *   as well as on-screen viewports.
         * - Provide utility queries such as frustum tests and screen-to-world
         *   ray generation.
         *
         * The component integrates with the component lifecycle (load/unload)
         * and listens to transform changes to keep the render camera/node in
         * sync with the actor transform.
         */
        class WPCore_API Camera : public Component
        {
        public:
            /** Property name for the camera z-order used when serializing. */
            static const String s_zOrderStr;

            /** Property name for the camera active state used when serializing. */
            static const String s_isActiveStr;

            /** Property name for the camera shadow enable flag used when serializing. */
            static const String s_enableShadowsStr;

            /** Property name for filtering scene objects by visibility flags. */
            static const String s_visibilityMaskStr;

            /** Property name for resetting camera settings. */
            static const String s_resetStr;

            /** Property name for triggering update of active state. */
            static const String s_updateActiveStateStr;

            /** Property name for the viewport background colour. */
            static const String s_viewportBackgroundColourStr;

            /** Property name for enabling scene rendering in the viewport. */
            static const String s_enableSceneRenderStr;

            /** Property name for enabling UI rendering in the viewport. */
            static const String s_enableUIStr;

            /** Property name for clearing the viewport every frame. */
            static const String s_clearEveryFrameStr;

            /** Property name for enabling overlays in the viewport. */
            static const String s_overlaysEnabledStr;

            /** Property name for auto-updating the viewport. */
            static const String s_autoUpdatedStr;

            /** Property name for the camera vertical field of view (degrees). */
            static const String s_fovStr;

            /** Property name for the near clip distance. */
            static const String s_nearClipStr;

            /** Property name for the far clip distance. */
            static const String s_farClipStr;

            /** Property name for the orthographic projection window width. */
            static const String s_orthoWidthStr;

            /** Property name for the orthographic projection window height. */
            static const String s_orthoHeightStr;

            static const String s_outputRenderTextureStr;
            static const String s_postProcessEnabledStr;
            static const String s_postProcessWorkspaceStr;
            static const String s_fxaaStr;
            static const String s_bloomStr;
            static const String s_exposureStr;
            static const String s_gammaStr;
            static const String s_contrastStr;
            static const String s_saturationStr;
            static const String s_bloomIntensityStr;
            static const String s_bloomThresholdStr;
            static const String s_vignetteStr;
            static const String s_compositeLayerPrefix;

            /**
             * @brief Construct a new Camera component
             *
             * Initializes internal members and prepares the component to be
             * attached to an actor. Does not create render objects until
             * load() is called.
             */
            Camera();

            /**
             * @brief Destroy the Camera component
             *
             * Cleans up any resources owned by the component. Actual
             * deallocation of render resources is performed in unload().
             */
            ~Camera() override;

            /**
             * @brief Load the camera component
             *
             * Called when the component is activated or the actor is loaded.
             * The optional `data` parameter can contain serialized settings
             * (camera parameters, target texture, viewport configuration, ...).
             * This method should allocate or acquire any render objects required
             * for rendering.
             *
             * @param data Optional serialized data for initialization.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload the camera component
             *
             * Called when the component is removed or the actor is unloaded.
             * Should release or detach render objects and clean up resources.
             *
             * @param data Optional data provided during unload.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Notify component that its actor transform changed
             *
             * Updates the internal render camera and scene node to match the
             * actor's current transform. This version queries the actor for
             * its transform.
             */
            void updateTransform() override;

            /**
             * @brief Update transform from provided Transform3
             *
             * This overload is used to update the camera when the transform is
             * available directly to avoid extra lookups.
             *
             * @param transform The new transform to apply to the camera/node.
             */
            void updateTransform( const Transform3<real_Num> &transform ) override;

            /**
             * @brief Retrieve serializable properties for this component
             *
             * Returns a `Properties` object containing configuration such as
             * z-order, active state and shadow settings that can be used by
             * editors or saved to disk.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Apply properties to this component
             *
             * Loads configuration values from a `Properties` object and
             * updates internal state accordingly.
             *
             * @param properties The properties to apply.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Get objects owned by this component
             *
             * Returns any child shared objects (textures, viewports, cameras)
             * that should be considered part of the component for lifetime
             * management or serialization.
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @brief Update rendering order metadata
             *
             * Invoked when scene ordering or actor ordering changes. This
             * method recalculates the internal z-order used when deciding
             * rendering precedence between multiple cameras.
             */
            void updateOrder() override;

            /**
             * @brief Get the off-screen render target texture
             *
             * This texture is used when the camera renders to an off-screen
             * target (for example, rendering to texture for post-processing
             * or UI previews).
             *
             * @return SmartPtr<render::ITexture> The current target texture or
             * null if rendering to the default framebuffer.
             */
            SmartPtr<render::ITexture> getTargetTexture() const;

            /**
             * @brief Set the off-screen render target texture
             *
             * Attaches a texture to be used as the render target for this
             * camera. Passing null will revert rendering to the default
             * output (window/backbuffer).
             *
             * @param targetTexture The texture to render into.
             */
            void setTargetTexture( SmartPtr<render::ITexture> targetTexture );

            /**
             * @brief Access the underlying render camera interface
             *
             * The `IGraphicsCamera` encapsulates projection (perspective/orthographic)
             * and view parameters used by the renderer. Callers may query or
             * modify the camera directly but must ensure changes remain in
             * sync with the component's transform.
             *
             * @return SmartPtr<render::IGraphicsCamera> The render camera instance.
             */
            SmartPtr<render::IGraphicsCamera> getCamera() const;

            /**
             * @brief Set the underlying render camera implementation
             *
             * Replaces the internal `IGraphicsCamera` instance used for rendering.
             * The component takes a reference to the passed camera.
             *
             * @param camera The camera to use for rendering.
             */
            void setCamera( SmartPtr<render::IGraphicsCamera> camera );

            /**
             * @brief Get the scene node used to position the camera
             *
             * The `IGraphicsSceneNode` (when present) is kept synchronized with the
             * actor transform and is used by the renderer to position the
             * camera in world space.
             *
             * @return SmartPtr<render::IGraphicsSceneNode> The scene node or null.
             */
            SmartPtr<render::IGraphicsSceneNode> getNode() const;

            /**
             * @brief Set the scene node for the camera
             *
             * Attaches a scene node that will be used to represent the
             * camera in the scene graph. The component will keep the node's
             * transform updated to follow the actor.
             *
             * @param node The scene node to attach.
             */
            void setNode( SmartPtr<render::IGraphicsSceneNode> node );

            /**
             * @brief Test whether this camera is currently active
             *
             * Active cameras participate in rendering. Inactive cameras are
             * ignored by the renderer and editor preview logic.
             *
             * @return bool True if active.
             */
            bool isActive() const;

            /**
             * @brief Enable or disable this camera
             *
             * Changing the active state will update rendering lists and may
             * trigger side-effects such as activating or deactivating an
             * associated viewport.
             *
             * @param active True to activate the camera, false to deactivate.
             */
            void setActive( bool active );

            /**
             * @brief Compute z-order relative to another actor
             *
             * Used to determine rendering priority when multiple cameras are
             * present. The exact comparison may take actor ordering into
             * account.
             *
             * @param other The other actor to compare against.
             * @return s32 Negative if this camera should render before the
             * other, positive otherwise.
             */
            s32 getZOrder( SmartPtr<IGameActor> other );

            /**
             * @brief Get numeric z-order for ordering cameras
             *
             * Higher z-order values typically render on top of lower ones.
             *
             * @return u32 The z-order value.
             */
            u32 getZOrder() const;

            /**
             * @brief Set the z-order used to sort this camera for rendering
             *
             * @param zOrder The z-order value to set.
             */
            void setZOrder( u32 zOrder );

            /**
             * @brief Create a world-space ray from a screen position
             *
             * Converts a normalized or pixel screen position into a ray in
             * world space that starts at the camera and points through the
             * viewport at `screenPosition`. Useful for picking and raycasts.
             *
             * @param screenPosition Screen coordinates (normalized or
             * viewport-relative depending on renderer conventions).
             * @return Ray3<real_Num> The resulting world-space ray.
             */
            Ray3<real_Num> getCameraToViewportRay( const Vector2<real_Num> &screenPosition );

            /**
             * @brief Test whether an axis-aligned bounding box intersects the
             * camera frustum
             *
             * Performs a conservative frustum-box test to determine if `box`
             * is potentially visible to this camera.
             *
             * @param box The axis-aligned bounding box in world space.
             * @return true if the box intersects or is inside the frustum.
             */
            bool isInFrustum( const AABB3<real_Num> &box ) const;

            /**
             * @brief Get the render target wrapper used by this camera
             *
             * The `IRenderTarget` groups textures and buffers used when
             * rendering off-screen. This accessor returns the target currently
             * associated with the camera's viewport (if any).
             *
             * @return SmartPtr<render::IRenderTarget> The current render
             * target or null.
             */
            SmartPtr<render::IRenderTarget> getRenderTarget() const;

            /**
             * @brief Get the viewport used for rendering
             *
             * The viewport defines the portion of the render target that this
             * camera draws into, and contains a reference to the `IGraphicsCamera`.
             *
             * @return SmartPtr<render::IViewport> The viewport instance.
             */
            SmartPtr<render::IViewport> getViewport() const;

            /**
             * @brief Set the viewport for this camera
             *
             * Attaches a viewport object which the renderer will use when
             * composing the final image.
             *
             * @param viewport The viewport to use for rendering.
             */
            void setViewport( SmartPtr<render::IViewport> viewport );

            /**
             * @brief Query whether shadow rendering is enabled for this camera
             *
             * Some cameras (e.g., editor previews) may disable expensive
             * features such as shadows. This flag controls that behavior.
             *
             * @return bool True if shadows are enabled.
             */
            bool getEnableShadows() const;

            /**
             * @brief Enable or disable shadow rendering for this camera
             *
             * @param enableShadows True to enable shadows, false to disable.
             */
            void setEnableShadows( bool enableShadows );

            /**
             * @brief Update internal smoothing/interpolation state for the
             * camera transform
             *
             * Used to enable or disable smoothing of camera motion for
             * cinematic or editor effects.
             */
            void updateSmoothTransformState() override;

            /** Set the orthographic projection window height. */
            void setOrthoWindowHeight( f32 height );

            /** Get the orthographic projection window height. */
            f32 getOrthoWindowHeight() const;

            /** Set the orthographic projection window width. */
            void setOrthoWindowWidth( f32 width );

            /** Get the orthographic projection window width. */
            f32 getOrthoWindowWidth() const;

            /** Get the vertical field of view in degrees. */
            f32 getFOV() const;

            /** Set the vertical field of view in degrees. */
            void setFOV( f32 fov );

            /** Get the near clip distance. */
            f32 getNearClipDistance() const;

            /** Set the near clip distance. */
            void setNearClipDistance( f32 nearDist );

            /** Get the far clip distance. */
            f32 getFarClipDistance() const;

            /** Set the far clip distance. */
            void setFarClipDistance( f32 farDist );

            /** Get the background colour used to clear the viewport each frame. */
            ColourF getViewportBackgroundColour() const;

            /** Set the background colour used to clear the viewport each frame. */
            void setViewportBackgroundColour( const ColourF &colour );

            /** Query whether scene geometry is rendered through this camera's viewport. */
            bool getEnableSceneRender() const;

            /** Enable or disable scene geometry rendering through this camera's viewport. */
            void setEnableSceneRender( bool enable );

            /** Query whether UI elements are rendered through this camera's viewport. */
            bool getEnableUI() const;

            /** Enable or disable UI rendering through this camera's viewport. */
            void setEnableUI( bool enable );

            /** Query whether the viewport is cleared before each frame. */
            bool getClearEveryFrame() const;

            /** Enable or disable clearing the viewport before each frame. */
            void setClearEveryFrame( bool clear );

            /** Query whether overlay objects are rendered in this viewport. */
            bool getOverlaysEnabled() const;

            /** Enable or disable overlay rendering in this viewport. */
            void setOverlaysEnabled( bool enabled );

            /** Query whether the viewport updates automatically each frame. */
            bool getAutoUpdated() const;

            /** Enable or disable automatic per-frame viewport updates. */
            void setAutoUpdated( bool autoUpdated );

            SmartPtr<RenderTexture> getOutputRenderTexture() const;
            void setOutputRenderTexture( SmartPtr<RenderTexture> renderTexture );

            render::IGraphicsCamera::PostProcessSettings getPostProcessSettings() const;
            void setPostProcessSettings( const render::IGraphicsCamera::PostProcessSettings &settings );

            Array<render::IGraphicsCamera::CompositeLayer> getCompositeLayers() const;
            void setCompositeRenderTexture( u32 index, SmartPtr<RenderTexture> renderTexture,
                                            render::IGraphicsCamera::CompositeBlendMode blendMode,
                                            f32 opacity, bool enabled = true );

            void updateStatic() override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Internal helper to update activation-related state
             *
             * Called whenever the camera's active flag changes to perform
             * bookkeeping such as enabling viewports or changing renderer
             * lists.
             *
             * @param active New active state.
             */
            void updateActiveState( bool active );

            /**
             * @brief Create and configure a viewport for this camera
             *
             * This method encapsulates the logic to create a new viewport and
             * attach the camera and render target to it when necessary.
             */
            void createViewport();

            /**
             * @brief Destroy the viewport associated with this camera
             */
            void createRenderCamera();

            /**
             * @brief Destroy the render camera associated with this component
             *
             * Cleans up the internal `IGraphicsCamera` instance and any related
             * resources. Called during unload or when replacing the camera.
             */
            void destroyRenderCamera();

            /**
             * @brief React to component flag changes
             *
             * Called when component flags change; used to update internal
             * state that depends on component flags.
             */
            void updateFlags( u32 flags, u32 oldFlags ) override;

            /**
             * @brief Handle finite-state machine events for this component
             *
             * Part of the component's internal FSM. Processes state events
             * specific to camera behavior.
             */
            FSMReturnType handleComponentEvent( u32 state, FSMEvent eventType ) override;

            /**
             * @brief Handle generic engine events
             *
             * Receives engine-level events (input, editor notifications,
             * resource updates) and performs camera-specific responses.
             */
            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

            /// Editor preview texture (rendered when showing camera in editor)
            SmartPtr<render::ITexture> m_targetTexture;

            /// Viewport used by this camera for on-screen or off-screen rendering
            SmartPtr<render::IViewport> m_viewport;

            /// Underlying render camera providing projection/view parameters
            SmartPtr<render::IGraphicsCamera> m_camera;

            /// Scene node representing the camera's position/rotation in world
            SmartPtr<render::IGraphicsSceneNode> m_node;

            /** Background colour used when clearing the viewport. */
            ColourF m_viewportBackgroundColour = ColourF::Blue * 0.5f;

            /** The component's z order used to sort cameras for rendering. */
            u32 m_zOrder = 0;

            /** Scene visibility flags rendered by this camera. */
            u32 m_visibilityMask = std::numeric_limits<u32>::max();

            /** Vertical field of view in degrees. */
            f32 m_fov = 45.0f;

            /** Near clipping plane distance. */
            f32 m_nearClip = 0.1f;

            /** Far clipping plane distance. */
            f32 m_farClip = 1000.0f;

            /** Orthographic projection window width. */
            f32 m_orthoWidth = 0.0f;

            /** Orthographic projection window height. */
            f32 m_orthoHeight = 0.0f;

            /** Whether the camera is currently active (participates in rendering). */
            bool m_isActive = false;

            /** Whether shadows should be enabled when rendering with this camera. */
            bool m_enableShadows = false;

            /** Whether scene geometry is rendered through this camera's viewport. */
            bool m_enableSceneRender = true;

            /** Whether UI elements are rendered through this camera's viewport. */
            bool m_enableUI = true;

            /** Whether the viewport is cleared before each frame. */
            bool m_clearEveryFrame = true;

            /** Whether overlay objects (e.g. HUD) are rendered in this viewport. */
            bool m_overlaysEnabled = true;

            /** Whether the viewport updates automatically each frame. */
            bool m_autoUpdated = true;

            SmartPtr<RenderTexture> m_outputRenderTexture;
            Array<SmartPtr<RenderTexture>> m_compositeRenderTextures;

            Array<render::IGraphicsCamera::CompositeBlendMode> m_compositeBlendModes;
            Array<f32> m_compositeOpacities;
            Array<bool> m_compositeEnabled;

            render::IGraphicsCamera::PostProcessSettings m_postProcessSettings;

            /// Name suffix used to generate unique camera names
            static u32 m_nameExt;

            /// z-order suffix used to generate unique z-order values
            static u32 m_zorderExt;

            /// Viewport suffix used to generate unique viewport identifiers.
            static u32 m_vpExt;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // CameraComponent_h__
