#ifndef WorkphoneSceneHelicopterController_h__
#define WorkphoneSceneHelicopterController_h__

#include <Workphone/Scene/Components/VehicleController.hpp>
#include <atomic>

namespace workphone::scene
{
    /** Scene helicopter with a selectable rotor model.
     * AircraftThrottle is collective [0,1]; EngineThrottle is the governed engine input.
     * Player input is sampled on the scene task. Physics is created on entering Play.
     */
    class WPCore_API HelicopterController : public VehicleController
    {
    public:
        enum class PhysicsSolution : s32 { Arcade = 0, Advanced = 1, Extreme = 2, DCS = 3 };

        HelicopterController();
        ~HelicopterController() override;
        void update() override;
        void unload( SmartPtr<ISharedObject> data ) override;
        SmartPtr<Properties> getProperties() const override;
        void setProperties( SmartPtr<Properties> properties ) override;

        /** Invalid values are ignored; a loaded model changes on the next scene update. */
        s32 getPhysicsSolution() const;
        void setPhysicsSolution( s32 solution );
        f32 getEngineThrottle() const;
        void setEngineThrottle( f32 throttle );
        real_Num getMaximumLift() const;
        void setMaximumLift( real_Num lift );
        WP_CLASS_REGISTER_DECL;

    protected:
        String getAircraftImplementationName() const override;
        bool shouldCreateAircraftController() const override;
        void applyAircraftConfiguration() override;

    private:
        bool isSimulationActive() const;
        void neutralizeControls();
        void samplePlayerControls();
        atomic_s32 m_physicsSolution = 0;
        s32 m_activePhysicsSolution = -1;
        std::atomic<f32> m_engineThrottle{ 1.0f };
        std::atomic<real_Num> m_maximumLift{ static_cast<real_Num>( 22000.0 ) };
        f32 m_keyboardCollective = 0.0f;
        atomic_bool m_resetPhysics = false;
        SmartPtr<Properties> m_rotorTuning;
        real_Num m_appliedMaximumLift = static_cast<real_Num>( -1 );
    };
}
#endif
