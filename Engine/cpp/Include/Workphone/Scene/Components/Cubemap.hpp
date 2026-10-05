#ifndef Cubemap_h__
#define Cubemap_h__

#include <Workphone/Scene/Components/Component.hpp>

/**
 * @file Cubemap.hpp
 * @brief Scene component that represents a cubemap / reflection probe.
 *
 * The Cubemap component holds authoring and runtime configuration for
 * reflection probes. It is intentionally renderer-agnostic: capture and
 * GPU-side resources are owned by a render::IGraphicsCubemap implementation,
 * while this class provides the scene/editor-facing data and logic.
 */

namespace workphone
{
    namespace scene
    {
        /**
         * @brief Cubemap / reflection-probe component.
         *
         * Stores authoring data needed by editors, serializers and the render
         * system to manage baked, custom and realtime cubemaps. Back-end capture
         * operations are delegated to render::IGraphicsCubemap; this class
         * exposes a stable scene API for configuration and lifetime management.
         *
         * @note Instances are expected to be owned and updated by the scene
         * system; runtime enabling and distance-based activation controls are
         * provided by this component.
         */
        class WPCore_API Cubemap : public Component
        {
        public:
            /**
             * @brief Source of the cubemap texture.
             *
             * Baked: precomputed and stored on disk.
             * Custom: user-supplied cubemap asset.
             * Realtime: generated at runtime by the renderer.
             */
            enum class SourceType : s32
            {
                Baked = 0,
                Custom = 1,
                Realtime = 2
            };

            /**
             * @brief When and how a realtime cubemap should be refreshed.
             */
            enum class RefreshMode : s32
            {
                OnAwake = 0,
                EveryFrame = 1,
                ViaScripting = 2,
                Interval = 3
            };

            /**
             * @brief Cubemap projection type used for sampling and influence.
             *
             * Infinite: probe treated as coming from infinitely far away (directional).
             * Box: local box-projected cubemap suitable for localized reflections.
             */
            enum class ProjectionMode : s32
            {
                Infinite = 0,
                Box = 1
            };

            /**
             * @brief Shape used to determine the probe's area of influence.
             */
            enum class InfluenceShape : s32
            {
                Sphere = 0,
                Box = 1
            };

            /**
             * @brief How the scene should be cleared when rendering the cubemap.
             */
            enum class ClearMode : s32
            {
                Skybox = 0,
                SolidColor = 1,
                Transparent = 2
            };

            /**
             * @brief Bitmask specifying which cubemap faces should be captured.
             */
            enum FaceMask : u32
            {
                FacePositiveX = 1u << 0u,
                FaceNegativeX = 1u << 1u,
                FacePositiveY = 1u << 2u,
                FaceNegativeY = 1u << 3u,
                FacePositiveZ = 1u << 4u,
                FaceNegativeZ = 1u << 5u,
                FaceAll = FacePositiveX | FaceNegativeX | FacePositiveY | FaceNegativeY | FacePositiveZ |
                          FaceNegativeZ
            };

            /** Constructor / Destructor */
            Cubemap();
            ~Cubemap() override;

            /**
             * @name Lifecycle
             * Methods called by the scene/serialization system.
             */
            //@{
            void load( SmartPtr<ISharedObject> data ) override;
            void reload( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;
            void update() override;
            void updateMaterials() override;
            //@}

            /** Validate the component state for runtime use. */
            bool isValid() const override;

            /** GPU/render resource accessor. The render cubemap is renderer-owned. */
            SmartPtr<render::IGraphicsCubemap> getRenderCubemap() const;
            void setRenderCubemap( SmartPtr<render::IGraphicsCubemap> renderCubemap );

            /** Enable/disable this probe at runtime. */
            bool isEnabled() const override;
            void setEnabled( bool enabled ) override;

            /** Probe naming for editor/serialization. */
            String getProbeName() const;
            void setProbeName( const String &probeName );

            SourceType getSourceType() const;
            void setSourceType( SourceType sourceType );
            void setSourceType( s32 sourceType );

            RefreshMode getRefreshMode() const;
            void setRefreshMode( RefreshMode refreshMode );
            void setRefreshMode( s32 refreshMode );

            ProjectionMode getProjectionMode() const;
            void setProjectionMode( ProjectionMode projectionMode );
            void setProjectionMode( s32 projectionMode );

            InfluenceShape getInfluenceShape() const;
            void setInfluenceShape( InfluenceShape influenceShape );
            void setInfluenceShape( s32 influenceShape );

            ClearMode getClearMode() const;
            void setClearMode( ClearMode clearMode );
            void setClearMode( s32 clearMode );

            String getCubemapPath() const;
            void setCubemapPath( const String &cubemapPath );

            s32 getResolution() const;
            void setResolution( s32 resolution );

            f32 getIntensity() const;
            void setIntensity( f32 intensity );

            f32 getBlendDistance() const;
            void setBlendDistance( f32 blendDistance );

            f32 getImportance() const;
            void setImportance( f32 importance );

            f32 getNearClipDistance() const;
            void setNearClipDistance( f32 nearClipDistance );

            f32 getFarClipDistance() const;
            void setFarClipDistance( f32 farClipDistance );

            f32 getShadowDistance() const;
            void setShadowDistance( f32 shadowDistance );

            u32 getCullingMask() const;
            void setCullingMask( u32 cullingMask );

            u32 getFaceMask() const;
            void setFaceMask( u32 faceMask );

            bool getCaptureStaticObjects() const;
            void setCaptureStaticObjects( bool captureStaticObjects );

            bool getCaptureDynamicObjects() const;
            void setCaptureDynamicObjects( bool captureDynamicObjects );

            bool getUseHDR() const;
            void setUseHDR( bool useHDR );

            bool getGenerateMipmaps() const;
            void setGenerateMipmaps( bool generateMipmaps );

            bool getUseTimeSlicing() const;
            void setUseTimeSlicing( bool useTimeSlicing );

            bool getHighQualityFiltering() const;
            void setHighQualityFiltering( bool highQualityFiltering );

            bool getAutoEnableByDistance() const;
            void setAutoEnableByDistance( bool autoEnableByDistance );

            f32 getCameraDistance() const;
            void setCameraDistance( f32 cameraDistance );

            f32 getEnableDistanceThreshold() const;
            void setEnableDistanceThreshold( f32 distanceThreshold );

            // Backwards-compatible misspelled API kept so existing code does not break.
            f32 getEnableDistanceTheshold() const;
            void setEnableDistanceTheshold( f32 distanceTheshold );

            f32 getSphereRadius() const;
            void setSphereRadius( f32 sphereRadius );

            f32 getBoxExtentX() const;
            f32 getBoxExtentY() const;
            f32 getBoxExtentZ() const;
            void setBoxExtents( f32 x, f32 y, f32 z );

            f32 getCaptureOffsetX() const;
            f32 getCaptureOffsetY() const;
            f32 getCaptureOffsetZ() const;
            void setCaptureOffset( f32 x, f32 y, f32 z );

            f32 getClearColourR() const;
            f32 getClearColourG() const;
            f32 getClearColourB() const;
            f32 getClearColourA() const;
            void setClearColour( f32 r, f32 g, f32 b, f32 a );

            f32 getUpdateInterval() const;
            void setUpdateInterval( f32 updateInterval );

            bool isRuntimeActive() const;
            bool isDirty() const;
            bool isCaptureRequested() const;
            u32 getRefreshCount() const;
            double getLastRefreshRequestTime() const;

            void markDirty();
            void requestCapture();
            void clearCaptureRequest();
            bool consumeCaptureRequest();
            void resetRuntimeState();

            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            WP_CLASS_REGISTER_DECL;

        protected:
            bool shouldBeActive() const;
            bool shouldRequestCapture( double now ) const;
            void applyRenderState();
            void syncMaterialCubemap();
            void clearMaterialCubemap();

            SmartPtr<render::IGraphicsCubemap> m_renderCubemap;
            SmartPtr<render::IMaterial> m_appliedMaterial;
            SmartPtr<render::ITexture> m_appliedTexture;

            FixedString<256> m_probeName = "Reflection Probe";
            FixedString<256> m_cubemapPath;

            SourceType m_sourceType = SourceType::Realtime;
            RefreshMode m_refreshMode = RefreshMode::OnAwake;
            ProjectionMode m_projectionMode = ProjectionMode::Box;
            InfluenceShape m_influenceShape = InfluenceShape::Sphere;
            ClearMode m_clearMode = ClearMode::Skybox;

            bool m_enabled = true;
            bool m_runtimeActive = false;
            bool m_autoEnableByDistance = true;
            bool m_captureStaticObjects = true;
            bool m_captureDynamicObjects = true;
            bool m_useHDR = true;
            bool m_generateMipmaps = true;
            bool m_useTimeSlicing = true;
            bool m_highQualityFiltering = false;
            bool m_dirty = true;
            bool m_renderStateDirty = true;
            bool m_captureRequested = false;
            bool m_hasCaptured = false;

            s32 m_resolution = 128;
            u32 m_cullingMask = 0xFFFFFFFFu;
            u32 m_faceMask = FaceAll;
            u32 m_refreshCount = 0u;

            f32 m_cameraDistance = 0.0f;
            f32 m_distanceThreshold = 50.0f;
            f32 m_intensity = 1.0f;
            f32 m_blendDistance = 1.0f;
            f32 m_importance = 1.0f;
            f32 m_nearClipDistance = 0.1f;
            f32 m_farClipDistance = 1000.0f;
            f32 m_shadowDistance = 100.0f;
            f32 m_sphereRadius = 5.0f;
            f32 m_boxExtentX = 5.0f;
            f32 m_boxExtentY = 5.0f;
            f32 m_boxExtentZ = 5.0f;
            f32 m_captureOffsetX = 0.0f;
            f32 m_captureOffsetY = 0.0f;
            f32 m_captureOffsetZ = 0.0f;
            f32 m_clearColourR = 0.0f;
            f32 m_clearColourG = 0.0f;
            f32 m_clearColourB = 0.0f;
            f32 m_clearColourA = 1.0f;
            f32 m_updateInterval = 1.0f;

            double m_lastRefreshRequestTime = 0.0;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // Cubemap_h__
