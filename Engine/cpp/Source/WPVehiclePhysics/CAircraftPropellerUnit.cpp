#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include "WPVehiclePhysics/CAircraftPropellerUnit.hpp"
#include "WPVehiclePhysics/CAircraft.hpp"
#include "WPVehiclePhysics/CAircraftEngine.hpp"
#include "WPVehiclePhysics/CAircraftMotor.hpp"
#include "WPVehiclePhysics/CAircraftPropeller.hpp"
#include "WPVehiclePhysics/CAircraftBody.hpp"
#include "WPVehiclePhysics/CAircraft.hpp"
#include <Workphone/Workphone.hpp>
#include <iostream>

namespace workphone::vehicle
{
    u32 CAircraftPropellerUnit::m_idExt = 0;

    CAircraftPropellerUnit::CAircraftPropellerUnit()
    {
        setFlowSettlingTime(0.1);

        m_downWashCoef = -0.01f;
        m_torqueMultiplier = 10.0f;
        m_modelIsElectric = true;
        m_inFlowTC = 4.0;

        m_clSlopes.resize(1024);
        m_cdSlopes.resize(1024);

        m_gfValue = 1.0f;
        m_inflowFollowTC = 10.0f;
        m_inflowDotProduct = 1.0f;
        m_propPowerCoef = 1.0f;
        m_fixedVRSModifier = 45.0f;
        m_stallPoint = 0.3f;
        m_stallThrust = 0.4f;

        m_modelIsElectric = true;
        m_vrs = true;

        m_id = StringUtil::parseInt("PropellerUnit" + StringUtil::toString(m_idExt++));
    }

    CAircraftPropellerUnit::~CAircraftPropellerUnit()
    {
    }

    Vector3<real_Num> CAircraftPropellerUnit::getThrust() const
    {
        return m_thrust;
    }

    void CAircraftPropellerUnit::setThrust(const Vector3<real_Num> &thrust)
    {
        m_thrust = thrust;
    }

    Vector3<real_Num> CAircraftPropellerUnit::getPropwash() const
    {
        return m_propwash;
    }

    bool CAircraftPropellerUnit::isValid() const
    {
        return m_parent != nullptr && m_parentAircraft != nullptr && m_esc != nullptr &&
               m_batteryPack != nullptr && m_propeller != nullptr && m_pu != nullptr;
    }

    float CAircraftPropellerUnit::lookupSlope(float angle)
    {
        SmartPtr<CAircraftPropeller> pPropeller =
            workphone::static_pointer_cast<CAircraftPropeller>(m_propeller);
        CAircraftPropeller &prop = *pPropeller;

        if(angle != angle)
        {
            angle = 10.0f;
        }

        int num =
            static_cast<int>(Math<real_Num>::Floor(Math<real_Num>::Abs(angle) * 572.95f));
        num = ((!(static_cast<float>(num) > 899.0f)) ? num : 899);
        num = ((!(static_cast<float>(num) < 0.0f)) ? num : 0);
        return 0.7f * m_clSlopes[num] * prop.propClCoef();
    }

    float CAircraftPropellerUnit::lookupCdSlope(float angle)
    {
        SmartPtr<CAircraftPropeller> pPropeller =
            workphone::static_pointer_cast<CAircraftPropeller>(m_propeller);
        CAircraftPropeller &prop = *pPropeller;

        if(angle != angle)
        {
            angle = 0.0f;
        }
        int num =
            static_cast<int>(Math<real_Num>::Floor(Math<real_Num>::Abs(angle) * 572.95f));
        num = ((!(static_cast<float>(num) > 899.0f)) ? num : 899);
        num = ((!(static_cast<float>(num) < 0.0f)) ? num : 0);
        return 0.25f * m_cdSlopes[num];
    }

    void CAircraftPropellerUnit::ePropellerSimple(const double &t, const double &dt)
    {
        auto aircraftBody = getParent();
        auto aircraft = getParentAircraft();
        auto aircraftTransform = aircraft->getBodyTransform();
        auto worldTransform = getWorldTransform();
        auto localTransform = getLocalTransform();

        auto pPropeller = workphone::static_pointer_cast<CAircraftPropeller>(m_propeller);
        auto &prop = *pPropeller;

        auto velocity = aircraft->getLinearVelocity();

        auto engineRps = m_pu->getRPM();

        auto a = 1.0;
        auto b = 1.0;
        auto enginePower = m_pu->getPeakPowerW();
        auto factor = 1.0;

        //  Compute thrust
        if(engineRps > 0.0)
        {
            auto advanceRatio = velocity.length() / (engineRps * pPropeller->getDiameter());
            auto thrust = m_pu->getThrottle() * factor * enginePower *
                          (a + b * advanceRatio * advanceRatio) /
                          (engineRps * pPropeller->getDiameter());

            prop.propThrustValue(thrust);
        }
        else
        {
            prop.propThrustValue(0.0);
        }
    }

    void CAircraftPropellerUnit::ePropellerOld(const double &time, const double &deltaTime)
    {
        WP_ASSERT(Math<real_Num>::isFinite( time ));
        WP_ASSERT(Math<real_Num>::isFinite( deltaTime ));

        auto aircraftBody = getParent();
        auto aircraft = getParentAircraft();

        WP_ASSERT(aircraftBody);
        WP_ASSERT(aircraft);

        SmartPtr<CAircraftPropeller> pPropeller =
            workphone::static_pointer_cast<CAircraftPropeller>(m_propeller);
        CAircraftPropeller &prop = *pPropeller;

        // SmartPtr<CESController> pESC = fb::static_pointer_cast<CESController>(m_esc);
        // CESController& esc = *pESC;

        auto aircraftTransform = aircraft->getBodyTransform();

        // auto& aircraftTransform = aircraft->getWorldTransform();
        auto engineTransform = m_pu->getLocalTransform();
        auto worldTransform = getWorldTransform();
        auto localTransform = getLocalTransform();

        // WP_ASSERT(worldTransform);
        // WP_ASSERT(localTransform);

        auto localPosition = engineTransform.getPosition();
        auto worldPosition = aircraftTransform.transformPoint(localPosition);
        auto worldVelocity = aircraft->getPointVelocity(worldPosition);
        // auto worldVelocity = aircraft->getLinearVelocity();
        auto localVelocity = aircraftTransform.inverseTransformVector(worldVelocity);

        auto cfFlow = vecFromYFrame(localVelocity);

        auto roAir = aircraft->getAirDensity();
        auto effectivePitch = static_cast<real_Num>(0.0);
        // geometric pitch angle of working part of blade
        auto pDrag = static_cast<real_Num>(0.0);
        auto pInd = static_cast<real_Num>(0.0); // Drag and induced powers for prop
        auto bladeSpeed = static_cast<real_Num>(0.0); // Effective blade speed

        auto deltaW = static_cast<real_Num>(0.0); // change in omega in this timestep
        auto nn = 0;
        auto k = static_cast<real_Num>(0.0);
        auto temp = static_cast<real_Num>(0.0);
        auto tv1 = Vector3<real_Num>::zero();
        auto tv2 = Vector3<real_Num>::zero();

        // find the axial flow component
        prop.m_discFlow.X() = prop.m_propFrame.m_xAxis.dotProduct(cfFlow);
        prop.m_discFlow.Y() = prop.m_propFrame.m_yAxis.dotProduct(cfFlow);
        prop.m_discFlow.Z() = prop.m_propFrame.m_zAxis.dotProduct(cfFlow);

        // for slip when calculating the effective pitch
        prop.m_discFlow = prop.m_discFlow * 1.0;

        // auto engineRPM = m_pu->getRPM();
        // prop.m_wProp = engineRPM / (real_Num(30.0) / Math<real_Num>::pi());

        // prop.m_discFlow = VecFromSaracen(cfFlow);

        // prop.m_discFlow = Vector3<real_Num>::zero();

        bladeSpeed = static_cast<real_Num>(0.4) * prop.m_propDia * prop.m_wProp;
        //{effective blade speed}

        if(bladeSpeed > std::numeric_limits<real_Num>::epsilon())
        {
            effectivePitch =
                prop.m_geomerticPitch - (prop.m_discFlow.X() + prop.m_propWash) / bladeSpeed;
            // changed to use axial component
        }

        prop.m_propCl = effectivePitch * prop.m_dClByAlpha;
        if(prop.m_propCl > prop.m_propClMax)
        {
            prop.m_propCl = prop.m_propClMax;
        }

        if(prop.m_propCl < prop.m_propClMin)
        {
            prop.m_propCl = prop.m_propClMin;
        }

        // note the minimum is stored as a negative number
        // now calculate a cd for current cl allowing for a more rapid Cd rise for negative Cl (prop
        // operating against blade camber)
        if(prop.m_propCl > static_cast<real_Num>(0.0))
        {
            prop.m_propCd = prop.m_propCd0 +
                            prop.m_dCdBydCl2 *
                            Math<real_Num>::Pow(prop.m_propCl, 2.0);
        }
        else
        {
            prop.m_propCd =
                prop.m_propCd0 +
                prop.m_dCdBydCl2 * Math<real_Num>::Pow(static_cast<real_Num>(1.4) * prop.m_propCl,
                                                       2.0);
        }

        prop.m_propThrustValue = static_cast<real_Num>(0.5) * roAir *
                                 Math<real_Num>::Pow(bladeSpeed, 2.0) *
                                 prop.m_propCl * prop.m_bladeArea;
        WP_ASSERT(prop.m_propThrustValue < 1e10);

        // now calulate the P-factor torque components
        prop.m_yawPFac = static_cast<real_Num>(-0.20) * prop.m_propDia * roAir * prop.m_propCl *
                         prop.m_bladeArea * bladeSpeed * prop.m_discFlow.Z();
        // this assumes thrust of blades is effectively generated in an annulus 80% out in the disc
        prop.m_pitchPFac = static_cast<real_Num>(-0.20) * prop.m_propDia * roAir * prop.m_propCl *
                           prop.m_bladeArea * bladeSpeed * prop.m_discFlow.Y();

        if(prop.m_reversed)
        {
            prop.m_yawPFac = -prop.m_yawPFac; // reverse the sign of these of prop is reverse
            // rotation
            prop.m_pitchPFac = -prop.m_pitchPFac;
        }

        // now calculate the drag imbalance forces (yaw and pitch) due to in-plane flow components
        prop.m_yawDrag = static_cast<real_Num>(-0.5) * roAir * prop.m_propCd * prop.m_bladeArea *
                         bladeSpeed * prop.m_discFlow.Y();
        prop.m_pitchDrag = static_cast<real_Num>(-0.5) * roAir * prop.m_propCd * prop.m_bladeArea *
                           bladeSpeed * prop.m_discFlow.Z();

        auto propThrust = (prop.m_propFrame.m_xAxis * prop.m_propThrustValue) +
                          (prop.m_propFrame.m_yAxis * prop.m_yawDrag) +
                          (prop.m_propFrame.m_zAxis * prop.m_pitchDrag);
        // sum the thrust and drag components into thrust vector

        auto thrustLineRotation = MathUtil<real_Num>::getOrientationFromDirection(
            prop.getThrustLine(), -Vector3<real_Num>::unitZ(), false, Vector3<real_Num>::unitY());

        auto downThrust = m_propeller->getDownThrust();
        auto sideThrust = m_propeller->getSideThrust();

        auto thrustLine = Vector3<real_Num>::zero();
        thrustLine.X() = Math<real_Num>::Cos(downThrust) * Math<real_Num>::Cos(sideThrust);
        thrustLine.Y() = Math<real_Num>::Sin(sideThrust);
        thrustLine.Z() = Math<real_Num>::Sin(downThrust);
        thrustLine.normalise();
        thrustLine = vecToYFrame(thrustLine);

        prop.m_propThrust =
            Vector3<real_Num>(propThrust.X() * thrustLine.X(), propThrust.Y() * thrustLine.Y(),
                              propThrust.Z() * thrustLine.Z());

        auto propThrustLineRotate = MathUtil<real_Num>::getOrientationFromDirection(
            thrustLine, Vector3<real_Num>::unitZ(), false, Vector3<real_Num>::unitY());
        prop.m_propThrust = propThrustLineRotate * propThrust;

        // now do the quadratic solution for the propwash speed
        // this is the  K needed to apply the timeconstant TC
        k = deltaTime / getFlowSettlingTime();

        // intermediate sum that needs testing for positive
        temp = Math<real_Num>::Pow(prop.m_discFlow.X(), 2.0) +
               static_cast<real_Num>(4.0) * prop.m_propThrustValue /
               (static_cast<real_Num>(2.0) * roAir * prop.m_propArea);

        if(temp >= static_cast<real_Num>(0.0))
        {
            prop.m_propWash = (static_cast<real_Num>(1.0) - k) * prop.m_propWash +
                              k * static_cast<real_Num>(0.5) *
                              (Math<real_Num>::Sqrt(temp) - prop.m_discFlow.X());
        }

        // auto A = Math<real_Num>::pi() * bladeSpeed * prop.m_propSolRatio;
        // auto B = A + prop.m_discFlow.X();
        // auto discriminant = B * B + real_Num(4.0) * (prop.m_geomerticPitch * bladeSpeed -
        // prop.m_discFlow.X()) * A; if (discriminant > std::numeric_limits<real_Num>::epsilon())
        //{
        //	prop.m_propWash = real_Num(0.5) * (Math<real_Num>::Sqrt(discriminant) - B);
        // }

        pDrag = prop.m_propDragPowerFactor * prop.m_propCd *
                Math<real_Num>::Pow(prop.m_wProp, 3.0);
        // this sum checked 10/12/2014
        WP_ASSERT(pDrag < 1e10);

        pInd = prop.m_propThrustValue * prop.m_propWash;
        // induced power = thrust x airspeed through the prop
        // prop.m_massFlow = roAir * prop.m_propArea * (prop.m_discFlow.X() + prop.m_propWash);
        // //mass flow through the prop prop.m_wakeMoI = prop.m_massFlow * prop.m_propDia *
        // prop.m_propDia / real_Num(8.0);

        // calculate the wake moment of inertia as if a simple cylinger (MoI = 0.5*Mass*Radius
        // squared)
        if(Math<real_Num>::Abs(prop.m_wProp) > std::numeric_limits<real_Num>::epsilon())
        {
            auto propTorque = (pInd + pDrag) / prop.m_wProp;
            prop.propTorque(propTorque);
            WP_ASSERT(Math<real_Num>::isFinite( propTorque ));
        }
        else
        {
            auto propTorque =
                static_cast<real_Num>(0.0); // trap the div by zero if the prop stopped
            prop.propTorque(propTorque);
            WP_ASSERT(Math<real_Num>::isFinite( propTorque ));
        }

        ////NOTE*** In the following line the factor 0.6 has been added to compensate for the wake
            /// being subject to the (linear wash speed) PropWashFactors which are between 1 and 2
        // if (prop.m_wakeMoI > std::numeric_limits<real_Num>::epsilon())
        //{
        //	//apply the prop torque to the Wakes MoI to get rotation speed (Trapping Zero MoI)
        //	prop.m_wakeRotation = real_Num(0.6) * prop.m_propTorque / prop.m_wakeMoI;
        // }
        // else
        //{
        //	prop.m_wakeRotation = real_Num(0.0);
        // }

        // if (prop.m_reversed)
        //{
        //	prop.m_wakeRotationVector = prop.m_propFrame.m_xAxis * -prop.m_wakeRotation; //this for a
        // reverse rotation prop
        // }
        // else
        //{
        //	prop.m_wakeRotationVector = prop.m_propFrame.m_xAxis * prop.m_wakeRotation; //this for a
        // conventional prop rotation
        // }

        prop.m_inputTorque = m_pu->getTorque(); // pass the motor torque out to the prop

        if(prop.m_totMoI > std::numeric_limits<real_Num>::epsilon())
        {
            auto propTorque = prop.propTorque();
            deltaW = deltaTime * (prop.m_inputTorque - propTorque) / prop.m_totMoI;
            // calc the change in RPM in this timestep
        }

        WP_ASSERT(deltaW < 1e10);
        WP_ASSERT(prop.m_wProp < 1e10);

        // limit acceleration to 4000 radians/s (about 40,000 rpm/s)
        if(deltaW > static_cast<real_Num>(4000.0) * deltaTime)
        {
            deltaW = static_cast<real_Num>(4000.0) * deltaTime;
        }

        if(deltaW < static_cast<real_Num>(-4000.0) * deltaTime)
        {
            deltaW = static_cast<real_Num>(-4000.0) * deltaTime;
        }

        WP_ASSERT(Math<real_Num>::isFinite( deltaW ));

        prop.m_wProp = prop.m_wProp + deltaW;

        WP_ASSERT(prop.m_wProp < 1e10);
        WP_ASSERT(prop.isValid());

        if(prop.m_wProp < std::numeric_limits<real_Num>::epsilon())
        {
            prop.m_wProp = static_cast<real_Num>(0.0); // stop prop running backwards
        }

        // if (prop.m_wProp < real_Num(10.0) && prop.m_folding == false) //if prop is basically
        // stopped and is not folding
        //{
        //	prop.m_reaction = real_Num(0.0); //added to make sure the reaction is zeroed when motor
        // stops 	prop.m_propThrustValue = real_Num(-0.5) * roAir *
        // Math<real_Num>::Pow(prop.m_discFlow.X(), real_Num(2.0)) * 		real_Num(2.1) *
        // prop.m_bladeArea;

        //	//tread the stopped prop as a drag area with a Cd of about 2.1 (possibly slightly high)
        //	//thisProp.m_propThrust:=VScale(thisProp.m_propFrame.m_xAxis ,thisProp.m_propThrustValue) ;
        ////the thrust in vector form
        //	//now sum the thrust and the inplane drag inbalances into the thrust vector
        //	prop.m_propThrust = (prop.m_propFrame.m_xAxis * prop.m_propThrustValue) +
        //		(prop.m_propFrame.m_yAxis * prop.m_yawDrag),
        //		(prop.m_propFrame.m_zAxis * prop.m_pitchDrag); //sum the thrust and drag components
        // into thrust vector } //end of deadstick drag case

        // prop.m_angMomentum = prop.m_wProp * prop.m_totMoI; //calc ang momentum of
        // prop/engine/motor system if (prop.m_reversed)
        //{
        //	prop.m_angMomentum = -prop.m_angMomentum; //apply reverser to the angular momentum
        // }

        // prop.m_angMomVector = (prop.m_propFrame.m_xAxis * prop.m_angMomentum);

        // prop.m_reactionVector = (prop.m_propFrame.m_xAxis * prop.m_reaction);
        ////make the reaction vector for use in the OutputForces proc.
        // prop.m_reactionVector = prop.m_reactionVector + (prop.m_propFrame.m_yAxis *
        // prop.m_pitchPFac) + (prop.m_propFrame.m_zAxis * 	prop.m_yawPFac); //add the P-Factor
        // torques
        // in to the reaction vector

        if(m_pu->isElectric())
        {
            SmartPtr<CAircraftMotor> pEngine =
                workphone::static_pointer_cast<CAircraftMotor>(m_pu);
            CAircraftMotor &motor = *pEngine;

            prop.m_totMoI =
                prop.m_propMoI + motor.getMoi(); // make sure the MoI total includes the motor
            motor.setMotorOmega(prop.m_wProp); // pass the props current rotation rate to the Motor

            auto engineRPM = prop.m_wProp * static_cast<real_Num>(30.0) / Math<real_Num>::pi();
            motor.setRPM(engineRPM);
        }
        else
        {
            SmartPtr<CAircraftEngine> pEngine =
                workphone::static_pointer_cast<CAircraftEngine>(m_pu);
            CAircraftEngine &thisEngine = *pEngine;

            prop.m_totMoI = prop.m_propMoI + thisEngine.getMoi();

            if(prop.m_wProp < 0.01)
            {
                prop.m_wProp = 0.01;
            }

            if(Math<real_Num>::Abs(prop.m_wProp) > std::numeric_limits<real_Num>::epsilon())
            {
                auto engineTorque = thisEngine.getEnginePower() / prop.m_wProp;
                thisEngine.setTorque(engineTorque);
            }
            else
            {
                thisEngine.setTorque(0.0);
            }

            // thisEngine.setMotorOmega(prop.m_wProp); //pass the props current rotation rate to the
            // Motor

            auto engineRPM = prop.m_wProp * static_cast<real_Num>(30.0) / Math<real_Num>::pi();
            m_pu->setRPM(engineRPM);
        }
    }

    void CAircraftPropellerUnit::ePropellerNew(const double &time, const double &deltaTime)
    {
        WP_ASSERT(m_pu);
        WP_ASSERT(m_propeller);
        // WP_ASSERT(m_batteryPack);
        // WP_ASSERT(m_esc);

        SmartPtr<CAircraftPropeller> pPropeller =
            workphone::static_pointer_cast<CAircraftPropeller>(m_propeller);
        CAircraftPropeller &prop = *pPropeller;

        // SmartPtr<CBatteryPack> pBatteryPack =
        // boost::static_pointer_cast<CBatteryPack>(m_batteryPack); CBatteryPack& pack =
        // *pBatteryPack;

        // SmartPtr<CESController> pESC = boost::static_pointer_cast<CESController>(m_esc);
        // CESController& esc = *pESC;

        auto aircraftBody = getParent();
        auto aircraft = getParentAircraft();
        auto aircraftTransform = aircraft->getBodyTransform();
        auto worldTransform = getWorldTransform();

        auto localTransform = getLocalTransform();
        m_flag = 0;
        Vector3<real_Num> velocity = aircraftBody->getVelocity();
        auto cfFlow = worldTransform.inverseTransformVector(velocity);

        float airDensity = 1.225f;
        float timeConstant = m_inFlowTC;
        prop.m_discFlow.X() = cfFlow.Y();
        prop.m_discFlow.Y() = cfFlow.X();
        prop.m_discFlow.Z() = cfFlow.Z();
        m_propDiskFlowX = prop.m_discFlow.X();
        float bladeSpeed = 0.4f * prop.m_propDia * prop.m_wProp;
        m_propGA = 0.0f;

        if(bladeSpeed != 0.0f)
        {
            m_propGA = prop.m_geomerticPitch - (prop.m_discFlow.X() + m_vortex +
                                                prop.m_propWash * m_inflowDotProduct * m_gfValue *
                                                m_thrustAccelerationModifier) /
                       bladeSpeed;
        }

        cfFlow.X() = 0.0f;
        cfFlow.Z() = 0.0f;
        prop.m_propCl = ((!(m_propGA < 0.0f))
                             ? lookupSlope(m_propGA)
                             : (0.0f - lookupSlope(m_propGA)));

        if(prop.m_propCl > 0.0f)
        {
            prop.m_propCd =
                prop.m_propCd0 + prop.m_dCdBydCl2 * Math<real_Num>::Pow(prop.m_propCl, 2.0f);
        }
        else
        {
            prop.m_propCd = prop.m_propCd0 +
                            prop.m_dCdBydCl2 * Math<real_Num>::Pow(1.4f * prop.m_propCl, 2.0f);
        }

        prop.m_propThrustValue = 0.5f * airDensity * Math<real_Num>::Pow(bladeSpeed, 2.0f) *
                                 prop.m_propCl * prop.m_bladeArea;
        prop.m_proprThrustKg = prop.m_propThrustValue / 9.81f;
        m_thisPW = 0.0f;
        m_vh = 0.0f;
        m_vc = 0.0f;
        m_vh = Math<real_Num>::Sqrt(Math<real_Num>::Abs(prop.m_propThrustValue) /
                                    (2.0f * airDensity * prop.m_propArea));

        if(prop.m_propThrustValue < 0.0f)
        {
            m_vh = 0.0f - m_vh;
        }

        if(m_vh == 0.0f)
        {
            m_thisPW = 0.0f;
        }
        else if(!m_vrs)
        {
            m_vc = prop.m_discFlow.X() / m_vh;
            if(m_vc > 0.0f)
            {
                m_thisPW = m_vh * Math<real_Num>::Pow(1.414f, 0.0f - m_vc);
            }
            else if(m_vc < -2.0f)
            {
                m_thisPW = m_vh * Math<real_Num>::Pow(2.0f, 2.0f + m_vc);
            }
            else
            {
                m_flag = 10;
                m_thisPW = m_vh;
            }
        }
        else
        {
            m_vc = prop.m_discFlow.X() / m_vh;
            if(m_vc > 0.0f)
            {
                m_thisPW = m_vh * Math<real_Num>::Pow(1.414f, 0.0f - m_vc);
            }
            else if(m_vc < -2.0f)
            {
                m_thisPW = m_vh * Math<real_Num>::Pow(1.414f, m_vc);
            }
            else
            {
                m_flag = 10;
                m_thisPW = m_vh * Math<real_Num>::Pow(1.414f, m_vc);
            }
        }

        float flowFactor =
            Math<real_Num>::Abs(prop.m_discFlow.Y()) + Math<real_Num>::Abs(prop.m_discFlow.Z());
        flowFactor = Math<real_Num>::Pow(flowFactor, 2.0f) / 80.0f;
        m_thisPW /= 1.0f + flowFactor;
        float deltaFactor = deltaTime * timeConstant;

        if(Math<real_Num>::isFinite(m_thisPW))
        {
            prop.m_propWash = (1.0f - deltaFactor) * prop.m_propWash + deltaFactor * m_thisPW;
        }

        m_vortex = 0.0f;

        if(prop.m_discFlow.X() < 0.0f && m_vrs)
        {
            float translationFactor =
                Math<real_Num>::Sqrt(prop.m_discFlow.Y() * prop.m_discFlow.Y() +
                                     prop.m_discFlow.Z() * prop.m_discFlow.Z());
            translationFactor = 1.0f - translationFactor / (1.0f + translationFactor);
            m_translation = (1.0f - deltaFactor) * m_translation + deltaFactor * translationFactor;
            m_vortex = m_thisPW * prop.m_discFlow.X() * m_translation * (m_vrsModifier / 100.0f);
            m_vortex = ((!(m_vortex < -15.0f)) ? m_vortex : (-15.0f));
            m_vortex = ((!(m_vortex > 15.0f)) ? m_vortex : 15.0f);
        }

        m_inflowQuat = Quaternion<real_Num>::slerp(deltaTime * m_inflowFollowTC, m_inflowQuat,
                                                   worldTransform.getOrientation());
        Vector3<real_Num> worldDown = worldTransform.getOrientation() * Vector3<real_Num>::down();
        m_inflowVec = m_inflowQuat * Vector3<real_Num>::down();
        m_inflowDotProduct = m_inflowVec.normaliseCopy().dotProduct(worldDown.normaliseCopy());
        m_vortex *= m_inflowDotProduct;
        m_pdrag =
            prop.m_propDragPowerFactor * prop.m_propCd * Math<real_Num>::Pow(prop.m_wProp, 3.0f);
        m_pind = prop.m_propThrustValue * prop.m_propWash;
        prop.m_massFlow = airDensity * prop.m_propArea * (prop.m_discFlow.X() + prop.m_propWash);
        prop.m_wakeMoI = prop.m_massFlow * prop.m_propDia * prop.m_propDia / 8.0f;

        if(Math<real_Num>::Abs(prop.m_wProp) > std::numeric_limits<real_Num>::epsilon())
        {
            auto propTorque = (m_pind + m_pdrag) / prop.m_wProp;
            prop.propTorque(propTorque);
        }
        else
        {
            auto propTorque = static_cast<real_Num>(0.0);
            prop.propTorque(propTorque);
        }

        if(prop.propTorque() < static_cast<real_Num>(0.0))
        {
            auto propTorque = static_cast<real_Num>(0.0);
            prop.propTorque(propTorque);
        }

        if(m_pu->isElectric())
        {
            SmartPtr<CAircraftMotor> pEngine =
                workphone::static_pointer_cast<CAircraftMotor>(m_pu);
            CAircraftMotor &motor = *pEngine;

            prop.m_totMoI =
                prop.m_propMoI + motor.getMoi(); // make sure the MoI total includes the motor
            motor.setMotorOmega(prop.m_wProp); // pass the props current rotation rate to the Motor
            motor.setRPM(prop.m_wProp * static_cast<real_Num>(30.0) / Math<real_Num>::pi());
            prop.m_inputTorque = motor.getTorque(); // pass the motor torque out to the prop
        }
        else
        {
            SmartPtr<CAircraftEngine> pEngine =
                workphone::static_pointer_cast<CAircraftEngine>(m_pu);
            CAircraftEngine &thisEngine = *pEngine;

            if(prop.m_wProp < 0.1)
            {
                prop.m_wProp = 0.1;
            }

            if(prop.m_wProp > 0.0)
            {
                auto engineTorque = thisEngine.getEnginePower() / prop.m_wProp;
                thisEngine.setTorque(engineTorque);
            }

            // thisEngine.setMotorOmega(prop.m_wProp); //pass the props current rotation rate to the
            // Motor
            thisEngine.setRPM(prop.m_wProp * static_cast<real_Num>(30.0) / Math<real_Num>::pi());

            prop.m_inputTorque = thisEngine.getTorque(); // pass the motor torque out to the prop
        }

        float deltaOmega = deltaTime * (prop.m_inputTorque - prop.propTorque()) / prop.m_totMoI;
        prop.m_wProp += deltaOmega;
        if(prop.m_wProp < 0.0f)
        {
            prop.m_wProp = 0.0f;
        }
    }

    void CAircraftPropellerUnit::propellerNitro(const double &deltaTime)
    {
        auto aircraft = m_parentAircraft;

        // SmartPtr<CAircraftEngine> pEngine = boost::static_pointer_cast<CAircraftEngine>(m_pu);
        // CAircraftEngine& thisEngine = *pEngine;

        SmartPtr<CAircraftPropeller> pPropeller =
            workphone::static_pointer_cast<CAircraftPropeller>(m_propeller);
        CAircraftPropeller &prop = *pPropeller;

        auto aircraftBody = getParent();
        auto aircraftRef = getParentAircraft();
        auto aircraftTransform = aircraftRef->getBodyTransform();
        auto worldTransform = getWorldTransform();
        auto localTransform = getLocalTransform();

        auto engineTransform = m_pu->getLocalTransform();

        auto localPosition = engineTransform.getPosition();
        auto worldPosition = worldTransform.transformPoint(localPosition);

        auto cfFlow = aircraftRef->getPointVelocity(worldPosition);
        cfFlow = vecFromYFrame(worldTransform.inverseTransformVector(cfFlow));

        // WP_ASSERT(Math<real_Num>::isFinite(throttlePos));
        WP_ASSERT(Math<real_Num>::isFinite( deltaTime ));
        WP_ASSERT(deltaTime > static_cast<real_Num>( 0.0 ) &&
            deltaTime < static_cast<real_Num>( 1.0 ));

        constexpr auto propCd = static_cast<real_Num>(0.03);
        constexpr auto propClLim = static_cast<real_Num>(1.2);
        const auto rec2Pi =
            static_cast<real_Num>(1.0) / (static_cast<real_Num>(2.0) * Math<real_Num>::pi());
        auto propGA =
            static_cast<real_Num>(0.0); /*geometric pitch angle of working part of blade*/
        auto pDrag = static_cast<real_Num>(0.0);
        auto pInd = static_cast<real_Num>(0.0); /*Drag and induced powers for prop*/
        auto propCl = static_cast<real_Num>(0.0);
        auto bladeSpeed = static_cast<real_Num>(0.0); /*Effective blade speed*/
        auto propTorque = static_cast<real_Num>(0.0);
        auto deltaW = static_cast<real_Num>(0.0);
        auto fuelFlow = static_cast<real_Num>(0.0);
        auto ths = 0;
        auto factorA = static_cast<real_Num>(0.0);
        /* = Math<real_Num>::pi()*bladeSpeed*PropSolidityRatio*/
        auto factorB = static_cast<real_Num>(0.0);
        auto airDensity = aircraft->getAirDensity();

        WP_ASSERT(Math<real_Num>::isFinite( prop.m_wProp ));

        if(prop.m_wProp < 0.1)
        {
            prop.m_wProp = 0.1;
        }

        // calc the peak power omega
        bladeSpeed = static_cast<real_Num>(0.4) * prop.m_propDia * prop.m_wProp;
        /*effective blade speed*/

        if(bladeSpeed > static_cast<real_Num>(0.0))
        {
            propGA = prop.m_geomerticPitch - (cfFlow.X() + prop.m_propWash) / bladeSpeed;
        }
        else
        {
            propGA = static_cast<real_Num>(0.0);
        }

        propCl = propGA * static_cast<real_Num>(2.0) * Math<real_Num>::pi();
        if(propCl > propClLim)
        {
            propCl = propClLim;
        }

        if(propCl < -propClLim)
        {
            propCl = -propClLim;
        }

        prop.m_propThrustValue = static_cast<real_Num>(0.5) * airDensity *
                                 Math<real_Num>::Pow(bladeSpeed, 2.0) *
                                 propCl * prop.m_bladeArea;
        WP_ASSERT(prop.m_propThrustValue < 1e10);

        // thisProp.m_propThrustValue = real_Num(0.5) * aircraft->getAirDensity() * bladeSpeed *
        // bladeSpeed * 	propCl * thisProp.m_bladeArea; WP_ASSERT(thisProp.m_propThrustValue <
        // 1e4);

        prop.setThrust(prop.m_thrustLine * prop.m_propThrustValue);
        /*  PropThrust:=Ro*PropDia*PropDia*WProp*WProp*PropBlades*PropChord*Math<real_Num>::pi()*propGA*0.2*PropDia;*/
        /*  PropWash:=0.5*(SQRT(CGFlow.x*CGFlow.x+0.5*PropDia*WProp*WProp*PropBlades*PropChord*propGA)-CGFlow.x);*/

        /*  PropWash:=0.75*PropWash+0.125*PropThrust/(PropArea*Ro*(CGFlow.x+PropWash));*/
        factorA = Math<real_Num>::pi() * bladeSpeed * prop.m_propSolRatio;
        factorB = factorA; // +aircraft.CFFlow.x;
        auto discriminant =
            factorB * factorB + static_cast<real_Num>(4.0) *
            (prop.m_geomerticPitch * bladeSpeed - cfFlow.X()) * factorA;
        if(discriminant > std::numeric_limits<real_Num>::epsilon())
        {
            prop.m_propWash =
                static_cast<real_Num>(0.5) * (Math<real_Num>::Sqrt(discriminant) - factorB);
        }

        pDrag = prop.m_propDragPowerFactor * prop.m_wProp * prop.m_wProp * prop.m_wProp;
        pInd = prop.m_propThrustValue * (cfFlow.X() + prop.m_propWash);

        if(prop.m_wProp > std::numeric_limits<real_Num>::epsilon())
        {
            propTorque = (pInd + pDrag) / prop.m_wProp;
        }

        if(m_pu->isElectric())
        {
            SmartPtr<CAircraftMotor> pEngine =
                workphone::static_pointer_cast<CAircraftMotor>(m_pu);
            CAircraftMotor &motor = *pEngine;

            prop.m_totMoI =
                prop.m_propMoI + motor.getMoi(); // make sure the MoI total includes the motor
            motor.setMotorOmega(prop.m_wProp); // pass the props current rotation rate to the Motor

            auto engineRPM = prop.m_wProp * static_cast<real_Num>(30.0) / Math<real_Num>::pi();
            motor.setRPM(engineRPM);

            prop.m_inputTorque = motor.getTorque(); // pass the motor torque out to the prop
        }
        else
        {
            SmartPtr<CAircraftEngine> pEngine =
                workphone::static_pointer_cast<CAircraftEngine>(m_pu);
            CAircraftEngine &thisEngine = *pEngine;

            if(prop.m_wProp < 0.01)
            {
                prop.m_wProp = 0.01;
            }

            if(Math<real_Num>::Abs(prop.m_wProp) > std::numeric_limits<real_Num>::epsilon())
            {
                auto engineTorque = thisEngine.getEnginePower() / prop.m_wProp;
                thisEngine.setTorque(engineTorque);
            }
            else
            {
                thisEngine.setTorque(0.0);
            }

            // thisEngine.setMotorOmega(prop.m_wProp); //pass the props current rotation rate to the
            // Motor
            thisEngine.setRPM(prop.m_wProp * static_cast<real_Num>(30.0) / Math<real_Num>::pi());

            prop.m_inputTorque = thisEngine.getTorque(); // pass the motor torque out to the prop
        }

        // calc the instant torque of the engine
        if(prop.m_propMoI > std::numeric_limits<real_Num>::epsilon())
        {
            deltaW = deltaTime * (prop.m_inputTorque - prop.propTorque()) / prop.m_propMoI;
        }

        // limit acceleration to 4000 radians/s (about 40,000 rpm/s)
        if(deltaW > static_cast<real_Num>(4000.0) * deltaTime)
        {
            deltaW = static_cast<real_Num>(4000.0) * deltaTime;
        }

        if(deltaW < static_cast<real_Num>(-4000.0) * deltaTime)
        {
            deltaW = static_cast<real_Num>(-4000.0) * deltaTime;
        }

        WP_ASSERT(Math<real_Num>::isFinite( deltaW ));
        prop.m_wProp = prop.m_wProp + deltaW;

        WP_ASSERT(prop.isValid());
    }

    void CAircraftPropellerUnit::update(const double &t, const double &dt)
    {
        WP_ASSERT(m_pu);

        // ePropellerNew(t, dt);
        ePropellerOld(t, dt);
        // propellerNitro(dt);
        // ePropellerSimple(t, dt);

        // WP_LOG(std::string("Thrust: ") + StringUtil::toString(m_propeller->getThrustValue()));

        auto transform = m_pu->getLocalTransform();
        auto localPosition = transform.getPosition();

        auto thrust = vecToYFrame(m_propeller->getThrust()) * m_pu->getThrustMultiplier();
        setThrust(thrust);

        // auto propwash = VecToSaracen(m_propeller->getThrustLine()).normaliseCopy() *
        // m_propeller->getPropwash(); setPropwash(propwash);
        setPropwash(-thrust);

        // auto throttlePos = real_Num(0.8) -
        // (real_Num)m_parentAircraft->getChannel(CAircraft::m_thrChannel); auto throttle =
        // throttlePos / (0.8 * 2.0); auto thrust = throttlePos * 200.0 * Vector3<real_Num>::UNIT_Z *
        // m_pu->getThrustMultiplier(); m_thrust = thrust;

        // auto thrust = m_propeller->getThrustValue() * Vector3<real_Num>::UNIT_Z *
        // m_pu->getThrustMultiplier(); m_thrust = thrust;

        auto localTorque = -thrust.normaliseCopy() * m_pu->getTorque() * m_pu->getTorqueMultiplier();

        if(m_parentAircraft->getEnablePowerUnit())
        {
            m_parent->addLocalForceAtLocalPosition(thrust, localPosition);
            m_parent->addLocalTorque(localTorque);
        }
    }

    void CAircraftPropellerUnit::escCutoutControl(SmartPtr<IESController> pESC, float dt)
    {
        SmartPtr<CESController> pCESC = workphone::static_pointer_cast<CESController>(pESC);
        CESController &ESC = *pCESC;

        SmartPtr<CBatteryPackStandard> pBatteryPack =
            workphone::static_pointer_cast<CBatteryPackStandard>(m_batteryPack);
        CBatteryPackStandard &pack = *pBatteryPack;

        if(pack.getVoltage() < (0.93f * ESC.m_cutoffV * pack.getNumCells()))
            ESC.m_cutoffActive = true;
            // if pack less than 93% of the cutoff voltage then hard cut
        else if(pack.getVoltage() < ESC.m_cutoffV * pack.getNumCells())
        {
            ESC.m_cutoffTimer = ESC.m_cutoffTimer + dt;
            if((Math<real_Num>::FloorToInt(ESC.m_cutoffTimer) % 5) == 0.0f)
                ESC.m_cutoffActive = true;
            else
                ESC.m_cutoffActive = false; // if below cutoff but  >93% cutoff pulse power on and off
        }
        else // battery volts ate above the cuttoff level
        {
            ESC.m_cutoffActive = false;
            ESC.m_cutoffTimer = 0.0f;
        }
    }

    SmartPtr<IBatteryPack> &CAircraftPropellerUnit::getBatteryPack()
    {
        return m_batteryPack;
    }

    const SmartPtr<IBatteryPack> &CAircraftPropellerUnit::getBatteryPack() const
    {
        return m_batteryPack;
    }

    void CAircraftPropellerUnit::setBatteryPack(SmartPtr<IBatteryPack> batteryPack)
    {
        m_batteryPack = batteryPack;
    }

    SmartPtr<IESController> &CAircraftPropellerUnit::getESC()
    {
        return m_esc;
    }

    const SmartPtr<IESController> &CAircraftPropellerUnit::getESC() const
    {
        return m_esc;
    }

    void CAircraftPropellerUnit::setESC(SmartPtr<IESController> esc)
    {
        m_esc = esc;
    }

    SmartPtr<IAircraftPowerUnit> &CAircraftPropellerUnit::getPowerUnit()
    {
        return m_pu;
    }

    const SmartPtr<IAircraftPowerUnit> &CAircraftPropellerUnit::getPowerUnit() const
    {
        return m_pu;
    }

    void CAircraftPropellerUnit::setPowerUnit(SmartPtr<IAircraftPowerUnit> powerUnit)
    {
        m_pu = powerUnit;
    }

    SmartPtr<IAircraftPropeller> &CAircraftPropellerUnit::getPropeller()
    {
        return m_propeller;
    }

    const SmartPtr<IAircraftPropeller> &CAircraftPropellerUnit::getPropeller() const
    {
        return m_propeller;
    }

    void CAircraftPropellerUnit::setPropeller(SmartPtr<IAircraftPropeller> propeller)
    {
        m_propeller = propeller;
    }

    real_Num CAircraftPropellerUnit::getFlowSettlingTime() const
    {
        return m_inFlowSettlingTime;
    }

    void CAircraftPropellerUnit::setFlowSettlingTime(real_Num flowSettlingTime)
    {
        m_inFlowSettlingTime = flowSettlingTime;
    }

    void CAircraftPropellerUnit::setPropwash(const Vector3<real_Num> &propwash)
    {
        m_propwash = propwash;
    }
}
