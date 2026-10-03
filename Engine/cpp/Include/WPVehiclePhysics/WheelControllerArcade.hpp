#ifndef CWheelControllerSimple_h__
#define CWheelControllerSimple_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>
#include <Workphone/Interface/Vehicle/IWheelComponent.hpp>
#include <WPVehiclePhysics/CVehicleComponent.hpp>

namespace workphone
{
    /** Simple wheel controller implementation. */
    class WPVehiclePhysics_API WheelControllerArcade : public CVehicleComponent<IWheelComponent>
    {
    public:
        /** Constructor. */
        WheelControllerArcade();

        /** Destructor. */
        ~WheelControllerArcade() override;

        void update() override;

        void addTorque( physics_Num torque ) override;

        void setTorque( physics_Num torque ) override;

        physics_Num getTorque() const override;

        physics_Num getMass() const override;

        void setMass( physics_Num mass ) override;

        physics_Num getSpringRate() const override;

        void setSpringRate( physics_Num springRate ) override;

        physics_Num getRadius() const override;

        void setRadius( physics_Num radius ) override;

        physics_Num getSuspensionTravel() const override;

        void setSuspensionTravel( physics_Num suspensionTravel ) override;

        physics_Num getDamping() const override;

        void setDamping( physics_Num damping ) override;

        physics_Num getSuspensionDistance() const override;

        void setSuspensionDistance( physics_Num suspensionDistance ) override;

        physics_Num getSteeringAngle() const override;

        void setSteeringAngle( physics_Num steeringAngle ) override;

        /** @copydoc IWheelController::isSteeringWheel */
        bool isSteeringWheel() const override;

        /** @copydoc IWheelController::setSteeringWheel */
        void setSteeringWheel( bool steeringWheel ) override;

        bool isPoweredWheel() const override;

        void setPoweredWheel( bool poweredWheel ) override;

        physics_Num getAngularVelocity() const override;

        void setAngularVelocity( physics_Num angularVelocity ) override;

        physics_Num getBrake() const override;

        void setBrake( physics_Num brake ) override;

        TireModel getTireModel() const override;

        void setTireModel( TireModel tireModel ) override;

        physics_Num getSpringForce() const;
        void        setSpringForce( physics_Num springForce );

        physics_Num getMassFraction() const;
        void        setMassFraction( physics_Num massFraction );

        Vector3<physics_Num> getWheelVelocity() const;
        void                 setWheelVelocity( const Vector3<physics_Num> &wheelVelocity );

        SmartPtr<Properties> getProperties() const override;
        void                 setProperties( SmartPtr<Properties> properties ) override;

        physics_Num getLateralFrictionCoefficient() const;
        void        setLateralFrictionCoefficient( physics_Num coefficient );
        physics_Num getRollingResistanceCoefficient() const;
        void        setRollingResistanceCoefficient( physics_Num coefficient );
        bool        isGrounded() const;

        WP_CLASS_REGISTER_DECL;

    protected:
        void updateWheel();

        // The ray cast hit of the wheel.
        SmartPtr<physics::IRaycastHit> m_hit;

        // The radius of the wheel.
        physics_Num m_radius = static_cast<physics_Num>( 0.35 );

        // The suspension distance.
        physics_Num m_suspensionDistance = static_cast<physics_Num>( 0.52 );

        // The suspension spring force.
        physics_Num m_springForce = static_cast<physics_Num>( 5000.0 );

        // The suspension damping.
        physics_Num m_damping = static_cast<physics_Num>( 1000.0 );

        // The mass fraction of the wheel.
        physics_Num m_massFraction = static_cast<physics_Num>( 0.25 );

        // The angular velocity of the wheel.
        physics_Num m_angularVelocity = static_cast<physics_Num>( 0.0 );

        // The steering angle of the wheel.
        physics_Num m_steeringAngle = static_cast<physics_Num>( 0.0 );

        // The linear velocity of the wheel.
        Vector3<physics_Num> m_wheelVelocity;

        physics_Num m_driveTorque = static_cast<physics_Num>( 0.0 );
        physics_Num m_lateralFrictionCoefficient = static_cast<physics_Num>( 5.0 );
        physics_Num m_rollingResistanceCoefficient = static_cast<physics_Num>( 0.01 );
        bool        m_isGrounded = false;

        // To know if the wheel is a steering wheel.
        bool m_isSteeringWheel = false;

        bool m_isPoweredWheel = false;
    };
} // namespace workphone

#endif // CWheelControllerSimple_h__
