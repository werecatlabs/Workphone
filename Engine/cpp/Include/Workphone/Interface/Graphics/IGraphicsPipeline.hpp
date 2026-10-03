#ifndef IGraphicsPipeline_h__
#define IGraphicsPipeline_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Math/AABB3.hpp>
#include <Workphone/Math/Ray3.hpp>

namespace workphone
{
    namespace render
    {

        // ============================================================================
        // QUALITY LEVELS
        // ============================================================================

        enum class QualityLevel : s32
        {
            Low = 0,
            Medium = 1,
            High = 2,
            Ultra = 3,
            Cinematic = 4
        };

        // ============================================================================
        // QUALITY SETTINGS
        // ============================================================================

        struct TaaSettings
        {
            f32 m_feedback = 0.92f;
            f32 m_clipGamma = 1.25f;
            f32 m_motionScale = 1.0f;
            s32 m_quality = 2;
            s32 m_haltonSamples = 16;
            f32 m_dynamicFeedbackCap = 0.55f;
            f32 m_fastMotionThreshold = 24.0f;
            f32 m_fastMotionMin = 0.72f;
            f32 m_clipMin = 0.82f;
        };

        struct TaoaSettings
        {
            f32 m_radius = 0.9f;
            f32 m_intensity = 1.35f;
            f32 m_thickness = 0.4f;
            s32 m_quality = 2;
            s32 m_slices = 3;
            s32 m_steps = 8;
            f32 m_temporalFeedback = 0.92f;
            s32 m_blurRadius = 3;
        };

        struct SsrSettings
        {
            s32 m_steps = 64;
            f32 m_thickness = 0.1f;
            f32 m_roughnessScale = 0.95f;
            f32 m_maxDistance = 100.0f;
            f32 m_fadeScale = 0.1f;
            f32 m_temporalStrength = 0.5f;
        };

        struct ContactShadowSettings
        {
            s32 m_samples = 16;
            f32 m_length = 0.3f;
            f32 m_thickness = 0.025f;
            f32 m_bias = 0.01f;
            f32 m_normalBias = 0.05f;
            f32 m_intensity = 1.0f;
            s32 m_noiseType = 0;
        };

        struct DofSettings
        {
            f32 m_focalLength = 50.0f;
            f32 m_aperture = 2.8f;
            f32 m_focusDistance = 10.0f;
            f32 m_nearBlur = 1.0f;
            f32 m_farBlur = 1.0f;
            s32 m_bokehShape = 6;
            f32 m_bokehRotation = 0.0f;
            f32 m_bokehIntensity = 1.0f;
            s32 m_sampleCount = 32;
        };

        struct HdrSettings
        {
            s32 m_bloomLevels = 6;
            f32 m_bloomThreshold = 1.0f;
            f32 m_bloomKnee = 0.6f;
            f32 m_bloomIntensity = 0.8f;
            f32 m_skyWeight = 0.15f;
            f32 m_farDistance = 400.0f;
            f32 m_exposureSpeedUp = 3.2f;
            f32 m_exposureSpeedDown = 1.4f;
            f32 m_minEV = -4.0f;
            f32 m_maxEV = 16.0f;
            bool m_enableAgxToneMapping = true;
            f32 m_lutStrength = 1.0f;
        };

        struct CsmSettings
        {
            s32 m_cascadeCount = 4;
            s32 m_shadowMapSize = 2048;
            f32 m_splitLambda = 0.95f;
            f32 m_bias = 0.0001f;
            f32 m_normalBias = 0.02f;
            f32 m_fadeDistance = 50.0f;
            f32 m_fadeTransition = 10.0f;
            bool m_enablePCF = true;
            s32 m_pcfKernelSize = 3;
            f32 m_softness = 1.0f;
        };

        struct MotionBlurSettings
        {
            s32 m_samples = 16;
            f32 m_intensity = 1.0f;
            bool m_cameraBlur = true;
            bool m_objectBlur = true;
            f32 m_velocityScale = 1.0f;
        };

        // ============================================================================
        // BASE GRAPHICS PIPELINE
        // Abstract base class for all graphics pipelines
        // ============================================================================

        class IGraphicsPipeline : public ISharedObject
        {
        public:
            // ============================================================================
            // DEBUG
            // ============================================================================

            enum class DebugView : s32
            {
                None = 0,
                Normals,
                Depth,
                Velocity,
                AO,
                SSR,
                ContactShadows,
                Bloom,
                Exposure,
                TAAHistory
            };

            IGraphicsPipeline();

            ~IGraphicsPipeline();

            // ========================================================================
            // LIFECYCLE
            // ========================================================================

            virtual bool initialize( s32 width, s32 height ) = 0;
            virtual void shutdown() = 0;

            // ========================================================================
            // SCENE
            // ========================================================================

            virtual void setScene( IGraphicsScene *scene ) = 0;
            virtual IGraphicsScene *getScene() const = 0;

            // ========================================================================
            // FRAME
            // ========================================================================

            virtual void beginFrame() = 0;
            virtual void endFrame() = 0;
            virtual void render() = 0;

            // ========================================================================
            // CAMERA
            // ========================================================================

            virtual void setCamera( IGraphicsCamera *camera ) = 0;
            virtual IGraphicsCamera *getCamera() const = 0;
            virtual void setViewMatrix( const Matrix4F &view ) = 0;
            virtual void setProjectionMatrix( const Matrix4F &projection ) = 0;
            virtual const Matrix4F &getViewMatrix() const = 0;
            virtual const Matrix4F &getProjectionMatrix() const = 0;

            // ========================================================================
            // CULLING
            // ========================================================================

            virtual const Array<IGraphicsSceneNode *> &getCulledObjects() const = 0;
            virtual const Array<IGraphicsLight *> &getCulledLights() const = 0;
            virtual const Array<IGraphicsSceneNode *> &getVisibleNodes() const = 0;

            // ========================================================================
            // RAYCASTING
            // ========================================================================

            virtual bool raycastClosest( const Ray3F &ray, IGraphicsSceneNode *&outHit,
                                         Vector3F &outPoint, Vector3F &outNormal ) const = 0;
            virtual void raycastAll( const Ray3F &ray, Array<IGraphicsSceneNode *> &outHits ) const = 0;

            // ========================================================================
            // CULLING PARAMS
            // ========================================================================

            virtual void setCullingParams( f32 nearClip, f32 farClip ) = 0;
            virtual void setFOV( f32 fov ) = 0;
            virtual void setAspectRatio( f32 aspect ) = 0;

            // ========================================================================
            // OBJECT MANAGEMENT
            // ========================================================================

            virtual void addObject( IGraphicsSceneNode *object ) = 0;
            virtual void removeObject( IGraphicsSceneNode *object ) = 0;
            virtual const Array<IGraphicsSceneNode *> &getObjects() const = 0;

            // ============================================================================
            // LIGHT MANAGEMENT
            // ============================================================================

            virtual void addLight( IGraphicsLight *light ) = 0;
            virtual void removeLight( IGraphicsLight *light ) = 0;
            virtual const Array<IGraphicsLight *> &getLights() const = 0;

            // ============================================================================
            // QUALITY SETTINGS
            // ============================================================================

            virtual void setQualityLevel( QualityLevel level ) = 0;
            virtual QualityLevel getQualityLevel() const = 0;

            virtual void setTaaSettings( const TaaSettings &settings ) = 0;
            virtual void setTaoaSettings( const TaoaSettings &settings ) = 0;
            virtual void setSsrSettings( const SsrSettings &settings ) = 0;
            virtual void setContactShadowSettings( const ContactShadowSettings &settings ) = 0;
            virtual void setDofSettings( const DofSettings &settings ) = 0;
            virtual void setHdrSettings( const HdrSettings &settings ) = 0;
            virtual void setCsmSettings( const CsmSettings &settings ) = 0;
            virtual void setMotionBlurSettings( const MotionBlurSettings &settings ) = 0;

            virtual const TaaSettings &getTaaSettings() const = 0;
            virtual const TaoaSettings &getTaoaSettings() const = 0;
            virtual const SsrSettings &getSsrSettings() const = 0;
            virtual const ContactShadowSettings &getContactShadowSettings() const = 0;
            virtual const DofSettings &getDofSettings() const = 0;
            virtual const HdrSettings &getHdrSettings() const = 0;
            virtual const CsmSettings &getCsmSettings() const = 0;
            virtual const MotionBlurSettings &getMotionBlurSettings() const = 0;

            // ============================================================================
            // EFFECT TOGGLES
            // ============================================================================

            virtual void enableTAA( bool enabled ) = 0;
            virtual void enableGTAO( bool enabled ) = 0;
            virtual void enableSSR( bool enabled ) = 0;
            virtual void enableContactShadows( bool enabled ) = 0;
            virtual void enableDOF( bool enabled ) = 0;
            virtual void enableBloom( bool enabled ) = 0;
            virtual void enableExposure( bool enabled ) = 0;
            virtual void enableMotionBlur( bool enabled ) = 0;
            virtual void enableCSM( bool enabled ) = 0;

            virtual bool isTAAEnabled() const = 0;
            virtual bool isGTAOEnabled() const = 0;
            virtual bool isSSREnabled() const = 0;
            virtual bool isContactShadowsEnabled() const = 0;
            virtual bool isDOFEnabled() const = 0;
            virtual bool isBloomEnabled() const = 0;
            virtual bool isExposureEnabled() const = 0;
            virtual bool isMotionBlurEnabled() const = 0;
            virtual bool isCSMEnabled() const = 0;

            // ============================================================================
            // OUTPUT
            // ============================================================================

            virtual const f32 *getOutputBuffer( s32 *outWidth = nullptr,
                                                s32 *outHeight = nullptr ) const = 0;
            virtual const f32 *getGBufferNormal() const = 0;
            virtual const f32 *getGBufferDepth() const = 0;
            virtual const f32 *getGBufferVelocity() const = 0;

            virtual void setDebugView( DebugView view ) = 0;
            virtual DebugView getDebugView() const = 0;

            //virtual const Statistics &getStatistics() const = 0;

            // ============================================================================
            // STATE
            // ============================================================================

            virtual bool isInitialized() const = 0;
            virtual s32 getWidth() const = 0;
            virtual s32 getHeight() const = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace render
}  // namespace workphone

#endif  // IGraphicsPipeline_h__
