#ifndef SampleVehicleAdvanced_h__
#define SampleVehicleAdvanced_h__

#include <Workphone/Application.hpp>
#include "Workphone/Core/Parameter.hpp"
#include "Workphone/Interface/System/IEventListener.hpp"
#include "ProceduralScene.h"
#include "VehicleAudio.h"
#include "VehicleVisualEffects.h"
#include "ReviewEvidence.h"
#include <atomic>
#include <array>
#include <limits>

namespace workphone
{
    namespace advanced { class ProductionSceneProfile; }

    class SampleVehicleAdvanced : public core::Application
    {
    public:
        SampleVehicleAdvanced();
        ~SampleVehicleAdvanced() override;

        void load( SmartPtr<ISharedObject> data ) override;
        void unload( SmartPtr<ISharedObject> data ) override;
        void update() override;

        void setGenerationOptions( u32 seed, procedural::VehicleAppearanceQuality quality )
        {
            m_seed = seed;
            m_quality = quality;
        }
        void setCapture( const std::string &path, const std::string &view )
        {
            m_capturePath = path;
            m_captureView = view;
        }
        bool capturePassed() const
        {
            return m_capturePassed;
        }
        void setReviewOptions( bool hud, s32 forcedLOD, s32 forcedVehicleLOD, f64 benchmarkSeconds, bool orbit, u32 width, u32 height )
        {
            m_hudEnabled = hud; m_forcedLOD = forcedLOD; m_benchmarkSeconds = benchmarkSeconds;
            m_forcedVehicleLOD = forcedVehicleLOD;
            m_orbitCamera = orbit; m_reviewWidth = width; m_reviewHeight = height;
        }
        void reset();
        void setFoliageWorldEnabled( bool enabled ) { m_foliageWorldEnabled = enabled; }
        void setCityOptions( bool enabled, s32 blocks, s32 route, bool naturalRoads = false )
        { m_openCity = enabled; m_cityBlocks = blocks; m_cityRoute = route; m_cityNaturalRoads = naturalRoads; }
        void setRoadAuthoringShowcase(bool enabled) { m_roadAuthoringShowcase=enabled; }
        void setCityTerrainRoads(bool enabled) { m_cityTerrainRoads=enabled; }
        void setMaterialExportDirectory( const std::string &path ) { m_materialExportDirectory = path; }
        void setEvidenceOptions( const std::string &path, const std::string &replay,
                                 std::vector<advanced::ReviewControl> controls )
        {
            m_evidencePath = path;
            m_replayPath = replay;
            m_replayControls = std::move( controls );
        }
        void setSmokeTest( bool enabled );
        void setTrackSmokeTest( bool enabled )
        {
            m_trackSmokeTest = enabled;
        }
        bool smokeTestPassed() const;
        void setAudioOptions( bool enabled, bool smokeTest )
        {
            m_audioEnabled = enabled;
            m_audioSmokeTest = smokeTest;
        }
        void setEffectsOptions( bool enabled, bool smokeTest, const std::string &capture )
        {
            m_effectsEnabled = enabled;
            m_effectsSmokeTest = smokeTest;
            m_effectsCapture = capture;
        }
        void setCollisionSmokeTest( bool enabled ) { m_collisionSmokeTest = enabled; }
        // The command-line harness owns this observer beyond application teardown.
        void setSceneProfile( advanced::ProductionSceneProfile *profile ) { m_sceneProfile = profile; }

    protected:
        class InputListener : public IEventListener
        {
        public:
            InputListener();
            ~InputListener() override;

            void unload( SmartPtr<ISharedObject> data ) override;

            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

            bool inputEvent( SmartPtr<IInputEvent> event );

            void setPriority( s32 priority ) override;

            s32 getPriority() const override;

            SmartPtr<SampleVehicleAdvanced> getOwner() const;

            void setOwner( SmartPtr<SampleVehicleAdvanced> owner );

        protected:
            WeakPtr<SampleVehicleAdvanced> m_owner;
            s32 m_priority = 1000;
        };

        void createPlugins() override;
        bool createSoundManager() override;

        void createScene() override;

        void updateRenderCamera();
        void updateControls();
        void updateWheelVisuals();
        void performReset();
        void updateDebugText();
        void updateSmokeTest();
        void updateVehicleAudio();
        void updateVehicleEffects();
        bool m_effectsEnabled = true, m_effectsSmokeTest = false;
        f64 m_effectsTestTime = 0;
        u32 m_effectsTestPhase = 0;
        size_t m_effectsTestEmitted = 0;
        size_t m_effectsTestParticles = 0, m_effectsTestDecals = 0;
        bool m_effectsTestGrounded = false;
        std::string m_effectsCapture;
        bool m_audioEnabled = true, m_audioSmokeTest = false;
        f64 m_audioElapsed = 0, m_audioSmokeTime = 0;
        u32 m_audioSmokePhase = 0;

        std::string m_capturePath, m_captureView;
        std::string m_evidencePath, m_replayPath;
        std::string m_materialExportDirectory;
        advanced::ProductionSceneProfile *m_sceneProfile = nullptr;
        std::vector<advanced::ReviewControl> m_replayControls;
        size_t m_replayTick = 0;
        bool m_capturePassed = false, m_captureAttempted = false, m_physicsConfigured = false;
        bool m_hudEnabled = true, m_orbitCamera = false, m_benchmarkStarted = false;
        bool m_foliageWorldEnabled = false;
        bool m_openCity = false;
        bool m_cityNaturalRoads = false;
        bool m_roadAuthoringShowcase = false;
        bool m_cityTerrainRoads = false;
        s32 m_cityBlocks = 8, m_cityRoute = 0;
        f32 m_cityExtent = 0, m_citySpacing = 0;
        f32 m_cityOverviewExtent = 0;
        Vector3F m_cityOverviewCentre{}, m_cityStreetCentre{};
        Vector3F m_cityJunctionCentre{};
        Vector3F m_cityBoulevardCentre{};
        Vector3F m_generatedSpawnPosition{0,.42f,0};
        bool m_cityHasJunction = false;
        Vector3F m_cityStreetForward{ 0, 0, -1 };
        s32 m_forcedLOD = -1;
        s32 m_forcedVehicleLOD = -1;
        f64 m_benchmarkSeconds = 0;
        u32 m_reviewWidth = 1280, m_reviewHeight = 720;
        advanced::SceneAssets m_assets;
        SmartPtr<scene::ProceduralRaceScene> m_raceScene;
        u64 m_profileFrames = 0;
        u64 m_reviewFrames = 0;
        f64 m_profileStart = 0;
        f64 m_reviewStart = -1;
        float m_surfaceGrip = 1.f;
        u32 m_seed = 7;
        procedural::VehicleAppearanceQuality m_quality = procedural::VehicleAppearanceQuality::High;
        size_t m_lastTrackIndex = 0;
        u32 m_nextCheckpoint = 1, m_lap = 1;
        f64 m_lapStart = 0, m_lastLapTime = 0, m_bestLapTime = 0;
        SmartPtr<InputListener> m_inputListener;

        SmartPtr<scene::IGameActor> m_boxGround;
        SmartPtr<scene::IGameActor> m_cameraActor;
        SmartPtr<scene::IGameActor> m_vehicleActor;
        SmartPtr<scene::IGameActor> m_chassisMeshActor;
        std::array<SmartPtr<scene::IGameActor>, 4> m_wheelActors;
        SmartPtr<scene::VehicleCameraController> m_cameraController;
        std::atomic<bool> m_resetRequested{ false };
        f64 m_nextDebugUpdate = 0.0;
        f64 m_nextDebugLog = 0.0;
        bool m_trackSmokeTest = false;
        void updateCollisionSmokeTest();
        void prepareCityRetentionSmoke();
        bool m_collisionSmokeTest = false, m_collisionProbeStarted = false;
        bool m_cityRetentionSmokePrepared = false;
        SourceBudgetText m_cityRetentionSmokeRoadId;
        f32 m_cityRetentionSmokeWidth = 0;
        f64 m_collisionProbeTime = 0;
        bool m_smokeTest = false;
        bool m_smokeTestPassed = false;
        u32 m_smokePhase = 0;
        f64 m_smokeTime = 0.0;
        Vector3<real_Num> m_smokeStartPosition;
        Quaternion<real_Num> m_smokeStartOrientation;
        real_Num m_smokeDriveSpeed = 0.0f;
        real_Num m_smokeMinHeight = std::numeric_limits<real_Num>::max();
        real_Num m_smokeMaxHeight = std::numeric_limits<real_Num>::lowest();
        real_Num m_smokePeakVerticalSpeed = 0.0f;
        real_Num m_smokePeakBodySlip = 0.0f;
        real_Num m_smokePeakYawRate = 0.0f;
    };
}  // namespace workphone

#endif  // SampleVehicleAdvanced_h__
