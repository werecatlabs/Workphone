#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include "WPVehiclePhysics/EngineSimple.hpp"
#include "WPVehiclePhysics/CAircraftBody.hpp"
#include "WPVehiclePhysics/CAircraft.hpp"
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/WorkphoneHeaders.hpp>

namespace workphone
{
    namespace vehicle
    {
        EngineSimple::EngineSimple()
        {
            // AnimatedPropellerPivot = nullptr;
            m_animatedPropellerPivotRotateAxis = Vector3<real_Num>::forward();
            // SlowPropeller = nullptr;
            // FastPropeller = nullptr;
            m_rpmToUseFastProp = 300.0f;

            m_idleRpm = 400.0f;
            m_maxRpm = 2800.0f;
            m_forceAtMaxRpm = 250.0f;
            m_percentageForceAppliedVsAirspeedKts = workphone::make_ptr<LinearSpline1<real_Num>>();
            m_rpmToAddPerKtOfSpeed = 10.0f;

            m_rpmLerpSpeed = 1.5f;

            // AudioClip EngineStartClip = null;
            // AudioClip EngineRunClip = null;
            m_pitchAtIdleRpm = 0.5f;
            m_pitchAtMaxRpm = 1.0f;

            m_currentEngineState = EngineState::Running;

            m_thrust = Vector3<real_Num>::zero();

            // InputController ThrottleController = new InputController();
            // InputController EngineStartController = new InputController();

            setCurrentRpm( 0.0f );
            // Parent = nullptr;
            m_desiredRpm = 0.0f;

            // AudioSource EngineStart = null;
            // AudioSource EngineRun = null;
            m_engineRunVolume = 0.0f;
        }

        EngineSimple::~EngineSimple()
        {
        }

        float EngineSimple::getCurrentRpm() const
        {
            return m_currentRpm;
        }

        void EngineSimple::setCurrentRpm( float currentRpm )
        {
            m_currentRpm = currentRpm;
        }

        void EngineSimple::update( const double &time, const double &deltaTime )
        {
            (void)time;

            if( !m_parent )
            {
                m_parent = m_parentAircraft->getBody();
            }

            if( m_parent )
            {
                switch( m_currentEngineState )
                {
                case EngineState::Off:
                {
                    updateOff();
                }
                break;

                case EngineState::Starting:
                {
                    updateStarting();
                }
                break;

                case EngineState::Running:
                {
                    updateRunning();
                }
                break;
                }

                // Lerp current rpm to desired rpm.
                m_currentRpm = Math<real_Num>::lerp( m_currentRpm, m_desiredRpm,
                                                     m_rpmLerpSpeed * static_cast<real_Num>( deltaTime ) );

                // Update audio based on RPM. (Doesn't matter if it's not playing i.e engine off )
                // if (null != EngineRun)
                //{
                //	float velocity = Parent.velocity.magnitude;
                //	float velocityKTS = velocity * 1.943844492f;

                //	float CurrentPitchRPM = CurrentRPM + (velocityKTS * RPMToAddPerKTOfSpeed);

                //	float rpmOffset = (CurrentPitchRPM - IdleRPM) / (MaxRPM - IdleRPM);

                //	float enginePitch = PitchAtIdleRPM + ((PitchAtMaxRPM - PitchAtIdleRPM) * rpmOffset);
                //	EngineRun.pitch = enginePitch;

                //	//If below idle fade out engine run sound..
                //	if (CurrentRPM < IdleRPM)
                //	{
                //		EngineRun.volume = EngineRunVolume * (CurrentRPM / IdleRPM);

                //		if (CurrentRPM < (IdleRPM * 0.1f))
                //		{
                //			EngineRun.volume = 0.0f;
                //		}
                //	}
                //	else
                //	{
                //		EngineRun.volume = EngineRunVolume;
                //	}

                //}

                // Set the correct propeller visibility.
                // if (SlowPropeller && FastPropeller)
                //{
                //	if (CurrentRpm > m_rpmToUseFastProp)
                //	{
                //		SlowPropeller.GetComponent<Renderer>().enabled = false;
                //		FastPropeller.GetComponent<Renderer>().enabled = true;
                //	}
                //	else
                //	{
                //		SlowPropeller.GetComponent<Renderer>().enabled = true;
                //		FastPropeller.GetComponent<Renderer>().enabled = false;
                //	}
                // }

                // Rotate the propeller hub.
                // if (null != AnimatedPropellerPivot)
                //{
                //	float rotationThisFrame = ((CurrentRpm * 360.0f) / 60.0f) * Time.smoothDeltaTime;
                //	AnimatedPropellerPivot.transform.Rotate(m_animatedPropellerPivotRotateAxis,
                // rotationThisFrame);
                // }
            }

            updateThrust();
        }

        void EngineSimple::updateThrust()
        {
            if( m_parent )
            {
                float forceMultiplier = ( m_currentRpm - m_idleRpm ) / ( m_maxRpm - m_idleRpm );
                forceMultiplier = Math<real_Num>::clamp( forceMultiplier, 0.0, 1.0 );

                float velocity = m_parent->getVelocity().length();
                float velocityKTS = velocity * 1.943844492f;

                WP_ASSERT( m_percentageForceAppliedVsAirspeedKts );
                float thrustPercent = m_percentageForceAppliedVsAirspeedKts->interpolate( velocityKTS ) *
                                      0.01f; // Convert to zero to one.
                thrustPercent = 0.2f;

                auto transform = getLocalTransform();
                m_thrust =
                    ( Vector3<real_Num>::UNIT_Z * ( m_forceAtMaxRpm * forceMultiplier ) ) * thrustPercent;
                m_parent->addForceAtPosition( m_thrust, transform.getPosition() );
            }
        }

        void EngineSimple::updateOff()
        {
            m_desiredRpm = 0.0f;
        }

        void EngineSimple::updateStarting()
        {
            // Spin up blades to idle.
            m_desiredRpm = m_idleRpm;
        }

        void EngineSimple::updateRunning()
        {
            float input = 0.8f - m_parentAircraft->getChannel( CAircraft::m_thrChannel );
            input = MathF::clamp( input, 0.0f, 1.0f );
            m_desiredRpm = m_idleRpm + ( ( m_maxRpm - m_idleRpm ) * input );
        }

        Vector3<real_Num> EngineSimple::getThrust() const
        {
            return m_thrust;
        }

        void EngineSimple::setThrust( Vector3<real_Num> thrust )
        {
            m_thrust = thrust;
        }
    } // namespace vehicle
} // namespace workphone
