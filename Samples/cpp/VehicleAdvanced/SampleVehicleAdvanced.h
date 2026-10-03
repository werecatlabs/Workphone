#ifndef SampleVehicleAdvanced_h__
#define SampleVehicleAdvanced_h__

#include <Workphone/Application.hpp>
#include "Workphone/Core/Parameter.hpp"
#include "Workphone/Interface/System/IEventListener.hpp"
#include "ProceduralScene.h"
#include <atomic>
#include <array>
#include <limits>

namespace workphone
{

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
        void reset();
        void setSmokeTest( bool enabled );
        void setTrackSmokeTest( bool enabled )
        {
            m_trackSmokeTest = enabled;
        }
        bool smokeTestPassed() const;

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

        void createScene() override;

        void updateRenderCamera();
        void updateControls();
        void updateWheelVisuals();
        void performReset();
        void updateDebugText();
        void updateSmokeTest();

        std::string m_capturePath, m_captureView;
        bool m_capturePassed = false, m_captureAttempted = false, m_physicsConfigured = false;
        advanced::SceneAssets m_assets;
        u64 m_profileFrames = 0;
        f64 m_profileStart = 0;
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
    };
}  // namespace workphone

#endif  // SampleVehicleAdvanced_h__
