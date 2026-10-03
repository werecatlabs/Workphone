#ifndef SkyboxPanoramaGenerator_h__
#define SkyboxPanoramaGenerator_h__

#include <Workphone/Scene/Components/Component.hpp>

#include <array>

namespace workphone
{
    namespace scene
    {
        class Camera;
        class Skybox;

        /**
         * @brief Builds and navigates a grid of image-based skybox panorama stations.
         *
         * Each station uses six cubemap face images. The generated file convention is:
         *
         *     <directory>/<prefix>_c<column>_r<row>_<face><extension>
         *
         * where face is front, back, left, right, up or down. Put this component and a
         * Skybox component on the same actor. A camera can then jump between stations while
         * retaining its current orientation, producing a Street View style experience.
         */
        class WPCore_API SkyboxPanorama : public Component
        {
        public:
            enum class TransitionEasing : s32
            {
                Linear = 0,
                SmoothStep = 1,
                EaseIn = 2,
                EaseOut = 3,
                EaseInOut = 4
            };

            enum class PanoramaBlendMode : s32
            {
                TimedSwitch = 0,
                CrossFade = 1
            };

            struct PanoramaNode
            {
                u32 index = 0;
                u32 column = 0;
                u32 row = 0;
                Vector3<real_Num> position = Vector3<real_Num>::zero();
                std::array<String, 6> textureNames;
            };

            static const String panoramaDirectoryStr;
            static const String filePrefixStr;
            static const String fileExtensionStr;
            static const String cameraActorNameStr;
            static const String gridColumnsStr;
            static const String gridRowsStr;
            static const String spacingXStr;
            static const String spacingZStr;
            static const String eyeHeightStr;
            static const String originOffsetStr;
            static const String centreGridStr;
            static const String snapCameraStr;
            static const String followCameraStr;
            static const String switchDistanceStr;
            static const String maximumLinkDistanceStr;
            static const String directionToleranceStr;
            static const String activePanoramaStr;
            static const String panoramaCountStr;
            static const String transitionEnabledStr;
            static const String transitionDurationStr;
            static const String transitionEasingStr;
            static const String interpolateCameraStr;
            static const String transitionArcHeightStr;
            static const String transitionFovOffsetStr;
            static const String panoramaBlendModeStr;
            static const String imageSwitchPointStr;
            static const String crossFadeStartStr;
            static const String crossFadeEndStr;
            static const String crossFadeDistanceOffsetStr;
            static const String allowTransitionInterruptionStr;
            static const String isTransitioningStr;
            static const String transitionProgressStr;
            static const String targetPanoramaStr;

            SkyboxPanorama();
            ~SkyboxPanorama() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;
            void update() override;

            SmartPtr<Properties> getProperties() const override;
            void setProperties( SmartPtr<Properties> properties ) override;

            /** Rebuild all panorama stations from the current authoring properties. */
            void rebuildPanoramas();

            u32 getPanoramaCount() const;
            const PanoramaNode *getPanorama( u32 index ) const;
            const Array<PanoramaNode> &getPanoramas() const;

            s32 getActivePanorama() const;

            /** Activate a station and optionally place the camera at that station. */
            bool moveToPanorama( u32 index, bool snapCamera = true );

            /** Activate the station nearest to a world-space point. */
            bool moveToNearestPanorama( const Vector3<real_Num> &worldPosition, bool snapCamera = true );

            /**
             * @brief Move to the best connected station in a world-space direction.
             *
             * Candidates are restricted by Maximum Link Distance and Direction Tolerance.
             */
            bool moveInDirection( const Vector3<real_Num> &worldDirection );

            bool moveForward();
            bool moveBackward();
            bool moveLeft();
            bool moveRight();

            /**
             * @brief Advance an active transition by a caller-supplied time step.
             *
             * update() calls this automatically with the application delta time. The public
             * method is also useful for deterministic playback, tests, and custom update loops.
             */
            void advanceTransition( real_Num deltaTime );

            bool isTransitioning() const;
            real_Num getTransitionProgress() const;
            s32 getTargetPanorama() const;
            void finishTransition();
            void cancelTransition();

            SmartPtr<IGameActor> getCameraActor() const;
            void setCameraActor( SmartPtr<IGameActor> cameraActor );

            String getPanoramaDirectory() const;
            void setPanoramaDirectory( const String &directory );

            String getFilePrefix() const;
            void setFilePrefix( const String &prefix );

            String getFileExtension() const;
            void setFileExtension( const String &extension );

            u32 getGridColumns() const;
            void setGridColumns( u32 columns );

            u32 getGridRows() const;
            void setGridRows( u32 rows );

            real_Num getSpacingX() const;
            void setSpacingX( real_Num spacing );

            real_Num getSpacingZ() const;
            void setSpacingZ( real_Num spacing );

            real_Num getEyeHeight() const;
            void setEyeHeight( real_Num eyeHeight );

            real_Num getSwitchDistance() const;
            void setSwitchDistance( real_Num distance );

            real_Num getMaximumLinkDistance() const;
            void setMaximumLinkDistance( real_Num distance );

            bool getSnapCamera() const;
            void setSnapCamera( bool snapCamera );

            bool getFollowCamera() const;
            void setFollowCamera( bool followCamera );

            bool getTransitionEnabled() const;
            void setTransitionEnabled( bool enabled );

            real_Num getTransitionDuration() const;
            void setTransitionDuration( real_Num duration );

            TransitionEasing getTransitionEasing() const;
            void setTransitionEasing( TransitionEasing easing );

            bool getInterpolateCamera() const;
            void setInterpolateCamera( bool interpolateCamera );

            real_Num getTransitionArcHeight() const;
            void setTransitionArcHeight( real_Num height );

            real_Num getTransitionFovOffset() const;
            void setTransitionFovOffset( real_Num offset );

            PanoramaBlendMode getPanoramaBlendMode() const;
            void setPanoramaBlendMode( PanoramaBlendMode mode );

            real_Num getImageSwitchPoint() const;
            void setImageSwitchPoint( real_Num switchPoint );

            real_Num getCrossFadeStart() const;
            void setCrossFadeStart( real_Num start );

            real_Num getCrossFadeEnd() const;
            void setCrossFadeEnd( real_Num end );

            real_Num getCrossFadeDistanceOffset() const;
            void setCrossFadeDistanceOffset( real_Num offset );

            bool getAllowTransitionInterruption() const;
            void setAllowTransitionInterruption( bool allow );

            WP_CLASS_REGISTER_DECL;

        protected:
            String makeTextureName( u32 column, u32 row, const String &face ) const;
            Vector3<real_Num> getGridOrigin() const;
            real_Num getEffectiveSwitchDistance() const;
            real_Num getEffectiveMaximumLinkDistance() const;
            s32 findNearestPanorama( const Vector3<real_Num> &worldPosition ) const;
            void resolveRuntimeObjects();
            void resolveSkyboxes();
            bool beginTransition( u32 index, bool moveCamera );
            bool activatePanorama( u32 index, bool snapCamera );
            void applyPanoramaTextures( const PanoramaNode &node, SmartPtr<Skybox> skybox = nullptr );
            real_Num applyTransitionEasing( real_Num progress ) const;
            real_Num getCrossFadeWeight( real_Num progress ) const;
            bool prepareCrossFade( const PanoramaNode &target );
            void applyCrossFade( real_Num weight );
            void resetCrossFade();
            SmartPtr<Camera> getCameraComponent() const;
            void markGridDirty();

            Array<PanoramaNode> m_panoramas;
            SmartPtr<IGameActor> m_cameraActor;
            SmartPtr<Skybox> m_visibleSkybox;
            SmartPtr<Skybox> m_hiddenSkybox;

            FixedString<256> m_panoramaDirectory = "Panoramas";
            FixedString<128> m_filePrefix = "panorama";
            FixedString<32> m_fileExtension = ".jpg";
            FixedString<128> m_cameraActorName = "Camera";

            Vector3<real_Num> m_originOffset = Vector3<real_Num>::zero();

            u32 m_gridColumns = 5;
            u32 m_gridRows = 5;
            s32 m_activePanorama = 0;
            s32 m_targetPanorama = -1;

            real_Num m_spacingX = static_cast<real_Num>( 10.0 );
            real_Num m_spacingZ = static_cast<real_Num>( 10.0 );
            real_Num m_eyeHeight = static_cast<real_Num>( 1.7 );
            real_Num m_switchDistance = static_cast<real_Num>( 0.0 );
            real_Num m_maximumLinkDistance = static_cast<real_Num>( 0.0 );
            real_Num m_directionToleranceDegrees = static_cast<real_Num>( 50.0 );
            real_Num m_transitionDuration = static_cast<real_Num>( 0.65 );
            real_Num m_transitionElapsed = static_cast<real_Num>( 0.0 );
            real_Num m_transitionProgress = static_cast<real_Num>( 0.0 );
            real_Num m_transitionArcHeight = static_cast<real_Num>( 0.0 );
            real_Num m_transitionFovOffset = static_cast<real_Num>( -5.0 );
            real_Num m_imageSwitchPoint = static_cast<real_Num>( 0.5 );
            real_Num m_crossFadeStart = static_cast<real_Num>( 0.1 );
            real_Num m_crossFadeEnd = static_cast<real_Num>( 0.9 );
            real_Num m_crossFadeDistanceOffset = static_cast<real_Num>( 1.0 );
            real_Num m_transitionStartFov = static_cast<real_Num>( 0.0 );

            Vector3<real_Num> m_transitionStartPosition = Vector3<real_Num>::zero();
            Vector3<real_Num> m_transitionEndPosition = Vector3<real_Num>::zero();

            TransitionEasing m_transitionEasing = TransitionEasing::SmoothStep;
            PanoramaBlendMode m_panoramaBlendMode = PanoramaBlendMode::CrossFade;

            bool m_centreGrid = true;
            bool m_snapCamera = true;
            bool m_followCamera = false;
            bool m_transitionEnabled = true;
            bool m_interpolateCamera = true;
            bool m_allowTransitionInterruption = true;
            bool m_isTransitioning = false;
            bool m_transitionMovesCamera = false;
            bool m_imageSwitched = false;
            bool m_crossFadeActive = false;
            bool m_gridDirty = true;
            bool m_warnedMissingSkybox = false;
            bool m_warnedMissingBlendSkybox = false;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // SkyboxPanoramaGenerator_h__
