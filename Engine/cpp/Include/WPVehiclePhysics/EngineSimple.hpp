#ifndef Engine_h__
#define Engine_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector3.hpp>
#include "Workphone/Math/LinearSpline1.hpp"
#include "Workphone/Math/Transform3.hpp"
#include "WPVehiclePhysics/CAircraftAttachment.hpp"
#include <Workphone/Interface/Vehicle/IVehicleComponent.hpp>

namespace workphone
{
    namespace vehicle
    {
        class WPVehiclePhysics_API EngineSimple : public CAircraftAttachment<IVehicleComponent>
        {
        public:
            enum class EngineState
            {
                Off,
                Starting,
                Running
            };

            EngineSimple();
            ~EngineSimple() override;

            float getCurrentRpm() const;
            void  setCurrentRpm( float currentRpm );

            void update( const double &time, const double &deltaTime ) override;

            void updateThrust();

            Vector3<real_Num> getThrust() const;
            void              setThrust( Vector3<real_Num> thrust );

        private:
            void updateOff();
            void updateStarting();
            void updateRunning();

            // Pointer<Transformf> AnimatedPropellerPivot;
            Vector3<real_Num> m_animatedPropellerPivotRotateAxis;

            // ActorPtr SlowPropeller;
            // ActorPtr FastPropeller;
            // RigidbodyPtr Parent;

            EngineState m_currentEngineState;

            Vector3<real_Num> m_thrust;

            float m_rpmToUseFastProp;

            float m_idleRpm;
            float m_maxRpm;
            float m_forceAtMaxRpm;
            // AnimationCurve PercentageForceAppliedVSAirspeedKTS = null;
            float m_rpmToAddPerKtOfSpeed;

            float m_rpmLerpSpeed;

            // AudioClip EngineStartClip = null;
            // AudioClip EngineRunClip = null;
            float m_pitchAtIdleRpm;
            float m_pitchAtMaxRpm;

            // InputController ThrottleController = new InputController();
            // InputController EngineStartController = new InputController();

            float m_desiredRpm;

            // AudioSource EngineStart;
            // AudioSource EngineRun;
            float m_engineRunVolume;

            float m_currentRpm;

            SmartPtr<LinearSpline1<real_Num>> m_percentageForceAppliedVsAirspeedKts;
        };
    } // namespace vehicle
} // namespace workphone

#endif // Engine_h__
