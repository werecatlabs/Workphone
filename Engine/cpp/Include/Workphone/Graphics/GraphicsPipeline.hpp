#ifndef GraphicsPipeline_h__
#define GraphicsPipeline_h__

#include <Workphone/Interface/Graphics/IGraphicsPipeline.hpp>
#include <Workphone/Graphics/SharedGraphicsObject.hpp>
#include <Workphone/Math/Frustum3.hpp>
#include <Workphone/Thread/RecursiveMutex.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Exception.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @class GraphicsPipeline
         * @brief Base implementation of IGraphicsPipeline providing common graphics pipeline functionality.
         *
         * This class provides:
         * - Scene management with object and light tracking
         * - Frustum culling for visible objects and lights
         * - Raycasting for object picking
         * - Quality level management with per-effect settings
         * - Post-processing effect toggles
         * - Debug visualization modes
         * - GBuffer access for advanced rendering techniques
         *
         * Thread Safety:
         * - Methods are thread-safe with internal mutex protection
         * - Objects and lights can be added/removed from any thread
         * - Frame operations (beginFrame, endFrame, render) should be called from render thread
         */
        class WPCore_API GraphicsPipeline : public IGraphicsPipeline
        {
        public:
            /** @brief Default constructor. */
            GraphicsPipeline();

            /** @brief Virtual destructor. */
            virtual ~GraphicsPipeline() override;
            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;

            WP_CLASS_REGISTER_DECL;

            // ========================================================================
            // LIFECYCLE
            // ========================================================================

            /**
             * @brief Initialize the graphics pipeline with the specified dimensions.
             * @param width Frame width in pixels.
             * @param height Frame height in pixels.
             * @return true if initialization succeeded, false otherwise.
             */
            bool initialize( s32 width, s32 height ) override;

            /**
             * @brief Shutdown the graphics pipeline and release all resources.
             */
            void shutdown() override;

            // ========================================================================
            // SCENE
            // ========================================================================

            /**
             * @brief Set the associated graphics scene.
             * @param scene Pointer to the graphics scene, or nullptr to clear.
             */
            void setScene( IGraphicsScene *scene ) override;

            /**
             * @brief Get the associated graphics scene.
             * @return Pointer to the graphics scene, or nullptr if none set.
             */
            IGraphicsScene *getScene() const override;

            // ========================================================================
            // FRAME
            // ========================================================================

            /**
             * @brief Begin a new frame.
             *
             * This should be called at the start of each rendered frame to update
             * temporal effects and prepare for rendering.
             */
            void beginFrame() override;

            /**
             * @brief End the current frame.
             *
             * This should be called at the end of each rendered frame to finalize
             * any temporal accumulation and present the results.
             */
            void endFrame() override;

            /**
             * @brief Execute the main rendering pass.
             *
             * Renders the scene using the current camera, view/projection matrices,
             * and all enabled effects.
             */
            void render() override;

            // ========================================================================
            // CAMERA & MATRICES
            // ========================================================================

            /**
             * @brief Set the primary camera for rendering.
             * @param camera Pointer to the camera, or nullptr to clear.
             */
            void setCamera( IGraphicsCamera *camera ) override;

            /**
             * @brief Get the primary camera.
             * @return Pointer to the camera, or nullptr if none set.
             */
            IGraphicsCamera *getCamera() const override;

            /**
             * @brief Set the view matrix directly (bypasses camera).
             * @param view The 4x4 view matrix.
             */
            void setViewMatrix( const Matrix4F &view ) override;

            /**
             * @brief Set the projection matrix directly (bypasses camera).
             * @param projection The 4x4 projection matrix.
             */
            void setProjectionMatrix( const Matrix4F &projection ) override;

            /**
             * @brief Get the current view matrix.
             * @return Reference to the view matrix.
             */
            const Matrix4F &getViewMatrix() const override;

            /**
             * @brief Get the current projection matrix.
             * @return Reference to the projection matrix.
             */
            const Matrix4F &getProjectionMatrix() const override;

            // ========================================================================
            // CULLING
            // ========================================================================

            /**
             * @brief Get objects that passed frustum culling.
             * @return Array of pointers to culled scene nodes.
             */
            const Array<IGraphicsSceneNode *> &getCulledObjects() const override;

            /**
             * @brief Get lights that passed frustum culling.
             * @return Array of pointers to culled lights.
             */
            const Array<IGraphicsLight *> &getCulledLights() const override;

            /**
             * @brief Get objects that are visible (passed culling and effect-specific tests).
             * @return Array of pointers to visible scene nodes.
             */
            const Array<IGraphicsSceneNode *> &getVisibleNodes() const override;

            // ========================================================================
            // RAYCASTING
            // ========================================================================

            /**
             * @brief Perform a raycast to find the closest hit.
             * @param ray The ray to cast.
             * @param outHit Output parameter for the hit object.
             * @param outPoint Output parameter for the hit world position.
             * @param outNormal Output parameter for the surface normal at the hit.
             * @return true if a hit was found, false otherwise.
             */
            bool raycastClosest( const Ray3F &ray, IGraphicsSceneNode *&outHit, Vector3F &outPoint,
                                 Vector3F &outNormal ) const override;

            /**
             * @brief Perform a raycast to find all hits.
             * @param ray The ray to cast.
             * @param outHits Output array of all hit objects.
             */
            void raycastAll( const Ray3F &ray, Array<IGraphicsSceneNode *> &outHits ) const override;

            // ========================================================================
            // CULLING PARAMETERS
            // ========================================================================

            /**
             * @brief Set the view frustum clipping planes.
             * @param nearClip Distance to the near clipping plane.
             * @param farClip Distance to the far clipping plane.
             */
            void setCullingParams( f32 nearClip, f32 farClip ) override;

            /**
             * @brief Set the field of view angle.
             * @param fov Field of view in degrees.
             */
            void setFOV( f32 fov ) override;

            /**
             * @brief Set the aspect ratio.
             * @param aspect Width/height ratio.
             */
            void setAspectRatio( f32 aspect ) override;

            // ========================================================================
            // OBJECT MANAGEMENT
            // ========================================================================

            /**
             * @brief Add an object to the pipeline.
             * @param object Pointer to the scene node to add.
             */
            void addObject( IGraphicsSceneNode *object ) override;

            /**
             * @brief Remove an object from the pipeline.
             * @param object Pointer to the scene node to remove.
             */
            void removeObject( IGraphicsSceneNode *object ) override;

            /**
             * @brief Get all registered objects.
             * @return Array of pointers to all scene nodes.
             */
            const Array<IGraphicsSceneNode *> &getObjects() const override;

            // ========================================================================
            // LIGHT MANAGEMENT
            // ========================================================================

            /**
             * @brief Add a light to the pipeline.
             * @param light Pointer to the light to add.
             */
            void addLight( IGraphicsLight *light ) override;

            /**
             * @brief Remove a light from the pipeline.
             * @param light Pointer to the light to remove.
             */
            void removeLight( IGraphicsLight *light ) override;

            /**
             * @brief Get all registered lights.
             * @return Array of pointers to all lights.
             */
            const Array<IGraphicsLight *> &getLights() const override;

            // ========================================================================
            // QUALITY SETTINGS
            // ========================================================================

            /**
             * @brief Set the overall quality level.
             * @param level The quality level to apply.
             */
            void setQualityLevel( QualityLevel level ) override;

            /**
             * @brief Get the current quality level.
             * @return The current quality level.
             */
            QualityLevel getQualityLevel() const override;

            // TAA Settings
            void setTaaSettings( const TaaSettings &settings ) override;
            void setTaoaSettings( const TaoaSettings &settings ) override;
            void setSsrSettings( const SsrSettings &settings ) override;
            void setContactShadowSettings( const ContactShadowSettings &settings ) override;
            void setDofSettings( const DofSettings &settings ) override;
            void setHdrSettings( const HdrSettings &settings ) override;
            void setCsmSettings( const CsmSettings &settings ) override;
            void setMotionBlurSettings( const MotionBlurSettings &settings ) override;

            // ========================================================================
            // QUALITY SETTINGS GETTERS
            // ========================================================================

            const TaaSettings &getTaaSettings() const override;
            const TaoaSettings &getTaoaSettings() const override;
            const SsrSettings &getSsrSettings() const override;
            const ContactShadowSettings &getContactShadowSettings() const override;
            const DofSettings &getDofSettings() const override;
            const HdrSettings &getHdrSettings() const override;
            const CsmSettings &getCsmSettings() const override;
            const MotionBlurSettings &getMotionBlurSettings() const override;

            // ========================================================================
            // EFFECT TOGGLES
            // ========================================================================

            void enableTAA( bool enabled ) override;
            void enableGTAO( bool enabled ) override;
            void enableSSR( bool enabled ) override;
            void enableContactShadows( bool enabled ) override;
            void enableDOF( bool enabled ) override;
            void enableBloom( bool enabled ) override;
            void enableExposure( bool enabled ) override;
            void enableMotionBlur( bool enabled ) override;
            void enableCSM( bool enabled ) override;

            // ========================================================================
            // EFFECT TOGGLES QUERY
            // ========================================================================

            bool isTAAEnabled() const override;
            bool isGTAOEnabled() const override;
            bool isSSREnabled() const override;
            bool isContactShadowsEnabled() const override;
            bool isDOFEnabled() const override;
            bool isBloomEnabled() const override;
            bool isExposureEnabled() const override;
            bool isMotionBlurEnabled() const override;
            bool isCSMEnabled() const override;

            // ========================================================================
            // OUTPUT BUFFERS
            // ========================================================================

            /**
             * @brief Get the main output buffer.
             * @param outWidth Optional output for buffer width.
             * @param outHeight Optional output for buffer height.
             * @return Pointer to the output buffer data (RGBA32F format).
             */
            const f32 *getOutputBuffer( s32 *outWidth = nullptr,
                                        s32 *outHeight = nullptr ) const override;

            /**
             * @brief Get the GBuffer normal texture.
             * @return Pointer to the normal buffer data (RGB10A2 format), or nullptr if not available.
             */
            const f32 *getGBufferNormal() const override;

            /**
             * @brief Get the GBuffer depth texture.
             * @return Pointer to the depth buffer data (R32F format), or nullptr if not available.
             */
            const f32 *getGBufferDepth() const override;

            /**
             * @brief Get the GBuffer velocity texture.
             * @return Pointer to the velocity buffer data (RG16F format), or nullptr if not available.
             */
            const f32 *getGBufferVelocity() const override;

            // ========================================================================
            // DEBUG
            // ========================================================================

            /**
             * @brief Set the debug visualization mode.
             * @param view The debug view to enable.
             */
            void setDebugView( DebugView view ) override;

            /**
             * @brief Get the current debug visualization mode.
             * @return The current debug view.
             */
            DebugView getDebugView() const override;

            // ========================================================================
            // STATE
            // ========================================================================

            /**
             * @brief Check if the pipeline is initialized.
             * @return true if initialized, false otherwise.
             */
            bool isInitialized() const override;

            /**
             * @brief Get the frame buffer width.
             * @return Width in pixels.
             */
            s32 getWidth() const override;

            /**
             * @brief Get the frame buffer height.
             * @return Height in pixels.
             */
            s32 getHeight() const override;

        private:
            // ========================================================================
            // PRIVATE HELPERS
            // ========================================================================

            /**
             * @brief Perform frustum culling on all registered objects and lights.
             */
            void performFrustumCulling();

            /**
             * @brief Update the view frustum from current matrices.
             */
            void updateViewFrustum();

            /**
             * @brief Check if an object is within the view frustum.
             * @param object The scene node to test.
             * @return true if the object is potentially visible.
             */
            bool isInFrustum( IGraphicsSceneNode *object ) const;

            /**
             * @brief Check if a light is within the view frustum.
             * @param light The light to test.
             * @return true if the light affects the frustum.
             */
            bool isInFrustum( IGraphicsLight *light ) const;

            /**
             * @brief Apply quality level presets based on the current setting.
             */
            void applyQualityPresets();

            /**
             * @brief Validate object pointer and return mutable reference.
             * @param object Pointer to validate.
             * @return Mutable reference to the object.
             */
            static IGraphicsSceneNode *validateObject( IGraphicsSceneNode *object );

            /**
             * @brief Validate light pointer and return mutable reference.
             * @param light Pointer to validate.
             * @return Mutable reference to the light.
             */
            static IGraphicsLight *validateLight( IGraphicsLight *light );

            // ========================================================================
            // MEMBER VARIABLES
            // ========================================================================

            mutable RecursiveMutex m_mutex;  ///< Thread safety mutex.

            // Initialization state
            bool m_initialized = false;  ///< True if pipeline is initialized.
            s32 m_width = 0;             ///< Frame buffer width.
            s32 m_height = 0;            ///< Frame buffer height.

            // Scene reference
            IGraphicsScene *m_scene = nullptr;  ///< Associated graphics scene (non-owning).

            // Camera and matrices
            IGraphicsCamera *m_camera = nullptr;           ///< Primary rendering camera (non-owning).
            Matrix4F m_viewMatrix = Matrix4F::identity();  ///< Current view matrix.
            Matrix4F m_projectionMatrix = Matrix4F::identity();  ///< Current projection matrix.

            // Culling parameters
            f32 m_nearClip = 0.1f;        ///< Near clipping plane distance.
            f32 m_farClip = 1000.0f;      ///< Far clipping plane distance.
            f32 m_fov = 60.0f;            ///< Field of view in degrees.
            f32 m_aspect = 16.0f / 9.0f;  ///< Aspect ratio (width/height).

            // View frustum for culling
            mutable Frustum3<real_Num> m_frustum;  ///< Current view frustum.

            // Scene objects
            Array<IGraphicsSceneNode *> m_objects;                ///< All registered scene nodes.
            mutable Array<IGraphicsSceneNode *> m_culledObjects;  ///< Objects that passed culling.
            mutable Array<IGraphicsSceneNode *> m_visibleNodes;   ///< Objects confirmed visible.

            // Lights
            Array<IGraphicsLight *> m_lights;                ///< All registered lights.
            mutable Array<IGraphicsLight *> m_culledLights;  ///< Lights that passed culling.

            // Quality settings
            QualityLevel m_qualityLevel = QualityLevel::High;  ///< Current quality level.

            // Effect settings
            TaaSettings m_taaSettings;                      ///< Temporal Anti-Aliasing settings.
            TaoaSettings m_taoaSettings;                    ///< Temporal Ambient Occlusion settings.
            SsrSettings m_ssrSettings;                      ///< Screen Space Reflections settings.
            ContactShadowSettings m_contactShadowSettings;  ///< Contact shadow settings.
            DofSettings m_dofSettings;                      ///< Depth of Field settings.
            HdrSettings m_hdrSettings;                      ///< HDR and bloom settings.
            CsmSettings m_csmSettings;                      ///< Cascaded Shadow Maps settings.
            MotionBlurSettings m_motionBlurSettings;        ///< Motion blur settings.

            // Effect enable/disable flags
            bool m_taaEnabled = true;             ///< TAA enabled.
            bool m_gtaoEnabled = true;            ///< GTAO enabled.
            bool m_ssrEnabled = false;            ///< SSR enabled.
            bool m_contactShadowsEnabled = true;  ///< Contact shadows enabled.
            bool m_dofEnabled = false;            ///< DOF enabled.
            bool m_bloomEnabled = true;           ///< Bloom enabled.
            bool m_exposureEnabled = true;        ///< Exposure/HDR enabled.
            bool m_motionBlurEnabled = false;     ///< Motion blur enabled.
            bool m_csmEnabled = true;             ///< CSM shadows enabled.

            // Output buffers (simulated for base class)
            mutable Array<f32> m_outputBuffer;     ///< Main output buffer.
            mutable Array<f32> m_gBufferNormal;    ///< GBuffer normals.
            mutable Array<f32> m_gBufferDepth;     ///< GBuffer depth.
            mutable Array<f32> m_gBufferVelocity;  ///< GBuffer velocity.

            // Debug
            DebugView m_debugView = DebugView::None;  ///< Current debug visualization mode.

            // Frame timing
            u64 m_frameCount = 0;   ///< Total frames rendered.
            f64 m_deltaTime = 0.0;  ///< Time between frames in seconds.
            f64 m_totalTime = 0.0;  ///< Total elapsed time in seconds.
        };
    }  // namespace render
}  // namespace workphone

#endif  // GraphicsPipeline_h__
