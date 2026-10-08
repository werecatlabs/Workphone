#ifndef WheelControllerBrush_h__
#define WheelControllerBrush_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>
#include <Workphone/Interface/Vehicle/IWheelComponent.hpp>
#include <WPVehiclePhysics/CVehicleComponent.hpp>

namespace workphone
{
    class WPVehiclePhysics_API WheelControllerBrush : public CVehicleComponent<IWheelComponent>
    {
    public:
        WheelControllerBrush();
        ~WheelControllerBrush() override;

        void update() override;
        void reset() override;

        void        addTorque( physics_Num torque ) override;
        void        setTorque( physics_Num torque ) override;
        physics_Num getTorque() const override;

        physics_Num getMass() const override;
        void        setMass( physics_Num mass ) override;

        physics_Num getSpringRate() const override;
        void        setSpringRate( physics_Num springRate ) override;

        physics_Num getRadius() const override;
        void        setRadius( physics_Num radius ) override;

        physics_Num getSuspensionTravel() const override;
        void        setSuspensionTravel( physics_Num suspensionTravel ) override;

        physics_Num getDamping() const override;
        void        setDamping( physics_Num damping ) override;

        physics_Num getSuspensionDistance() const override;
        void        setSuspensionDistance( physics_Num suspensionDistance ) override;

        physics_Num getSteeringAngle() const override;
        void        setSteeringAngle( physics_Num steeringAngle ) override;

        bool isSteeringWheel() const override;
        void setSteeringWheel( bool steeringWheel ) override;

        bool isPoweredWheel() const override;
        void setPoweredWheel( bool poweredWheel ) override;

        physics_Num getAngularVelocity() const override;
        void        setAngularVelocity( physics_Num angularVelocity ) override;

        physics_Num getBrake() const override;
        void        setBrake( physics_Num brake ) override;

        TireModel getTireModel() const override;
        void      setTireModel( TireModel tireModel ) override;

        physics_Num getMassFraction() const;
        void        setMassFraction( physics_Num massFraction );

        physics_Num getInertia() const;
        void        setInertia( physics_Num inertia );

        physics_Num getGrip() const;
        void        setGrip( physics_Num grip ) override;
        void        setContactAcceleration( physics_Num acceleration ) override;

        physics_Num getStaticFrictionCoefficient() const;
        void        setStaticFrictionCoefficient( physics_Num coefficient );

        physics_Num getSlidingFrictionCoefficient() const;
        void        setSlidingFrictionCoefficient( physics_Num coefficient );

        physics_Num getLongitudinalStiffness() const;
        void        setLongitudinalStiffness( physics_Num stiffness );

        physics_Num getLateralStiffness() const;
        void        setLateralStiffness( physics_Num stiffness );

        physics_Num getBrakeFrictionTorque() const;
        void        setBrakeFrictionTorque( physics_Num torque );

        physics_Num getHandbrake() const;
        void        setHandbrake( physics_Num handbrake );

        physics_Num getHandbrakeFrictionTorque() const;
        void        setHandbrakeFrictionTorque( physics_Num torque );

        physics_Num getRollingResistanceTorque() const;
        void        setRollingResistanceTorque( physics_Num torque );

        physics_Num getCompression() const override;
        physics_Num getNormalForce() const;
        physics_Num getSlipRatio() const;
        physics_Num getSlipAngle() const;
        physics_Num getSlipVelo() const;
        bool        isOnGround() const;
        bool        isGrounded() const;

        Vector3<physics_Num> getWheelVelo() const;
        Vector3<physics_Num> getLocalVelo() const;
        Vector3<physics_Num> getSuspensionForceVector() const;
        Vector3<physics_Num> getRoadForceVector() const;

        SmartPtr<Properties> getProperties() const override;
        void                 setProperties( SmartPtr<Properties> properties ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        void                 updateWheel( physics_Num dt );
        Vector3<physics_Num> calculateBrushForce( physics_Num normalForce, physics_Num dt, physics_Num cornerMass );
        void                 integrateFreeSpin( physics_Num dt );
        void                 applyAngularFriction( physics_Num torque, physics_Num dt );

        SmartPtr<physics::IRaycastHit> m_hit;

        physics_Num m_radius = static_cast<physics_Num>( 0.35 );
        physics_Num m_suspensionTravel = static_cast<physics_Num>( 0.52 );
        physics_Num m_suspensionDistance = static_cast<physics_Num>( 0.52 );
        bool m_implicitSuspension = false;
        bool m_tractionControl = false;
        bool m_antiLockBrakes = false;
        physics_Num m_contactAcceleration = 9.81;
        physics_Num m_contactEffectiveMass = 0;
        physics_Num m_springRate = static_cast<physics_Num>( 5000.0 );
        physics_Num m_damping = static_cast<physics_Num>( 1000.0 );
        physics_Num m_massFraction = static_cast<physics_Num>( 0.25 );
        physics_Num m_chassisMass = static_cast<physics_Num>( 0.0 );

        physics_Num m_inertia = static_cast<physics_Num>( 2.2 );
        physics_Num m_grip = static_cast<physics_Num>( 1.0 );
        physics_Num m_staticFrictionCoefficient = static_cast<physics_Num>( 1.1 );
        physics_Num m_slidingFrictionCoefficient = static_cast<physics_Num>( 0.9 );
        physics_Num m_longitudinalStiffness = static_cast<physics_Num>( 80000.0 );
        physics_Num m_lateralStiffness = static_cast<physics_Num>( 60000.0 );
        physics_Num m_brakeFrictionTorque = static_cast<physics_Num>( 8000.0 );
        physics_Num m_handbrakeFrictionTorque = static_cast<physics_Num>( 8000.0 );
        physics_Num m_rollingResistanceTorque = static_cast<physics_Num>( 10.0 );

        physics_Num m_driveTorque = static_cast<physics_Num>( 0.0 );
        physics_Num m_brake = static_cast<physics_Num>( 0.0 );
        physics_Num m_handbrake = static_cast<physics_Num>( 0.0 );
        physics_Num m_steeringAngle = static_cast<physics_Num>( 0.0 );

        physics_Num m_angularVelocity = static_cast<physics_Num>( 0.0 );
        physics_Num m_compression = static_cast<physics_Num>( 0.0 );
        physics_Num m_normalForce = static_cast<physics_Num>( 0.0 );
        physics_Num m_slipRatio = static_cast<physics_Num>( 0.0 );
        physics_Num m_slipAngle = static_cast<physics_Num>( 0.0 );
        physics_Num m_slipVelo = static_cast<physics_Num>( 0.0 );

        Vector3<physics_Num> m_wheelVelo;
        Vector3<physics_Num> m_localVelo;
        Vector3<physics_Num> m_suspensionForce;
        Vector3<physics_Num> m_roadForce;

        bool m_onGround = false;
        bool m_isSteeringWheel = false;
        bool m_isPoweredWheel = false;
    };

} // namespace workphone

#endif // WheelControllerBrush_h__
