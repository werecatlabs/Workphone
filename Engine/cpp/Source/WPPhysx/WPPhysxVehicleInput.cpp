#include <WPPhysx/WPPhysxPCH.hpp>
#include <WPPhysx/WPPhysxVehicleInput.hpp>
#include <Workphone/Memory/Memory.hpp>
#include <PxPhysicsAPI.h>

namespace workphone::physics
{
    PhysxVehicleInput::PhysxVehicleInput()
    {
        m_inputs = new physx::PxVehicleDrive4WRawInputData;
    }

    PhysxVehicleInput::~PhysxVehicleInput()
    {
        WP_SAFE_DELETE( m_inputs );
    }

    void PhysxVehicleInput::setDigitalAccel( const bool accelKeyPressed )
    {
        m_inputs->setDigitalAccel( accelKeyPressed );
    }

    void PhysxVehicleInput::setDigitalBrake( const bool brakeKeyPressed )
    {
        m_inputs->setDigitalBrake( brakeKeyPressed );
    }

    void PhysxVehicleInput::setDigitalHandbrake( const bool handbrakeKeyPressed )
    {
        m_inputs->setDigitalHandbrake( handbrakeKeyPressed );
    }

    void PhysxVehicleInput::setDigitalSteerLeft( const bool steerLeftKeyPressed )
    {
        // m_inputs->setDigitalSteerLeft(steerLeftKeyPressed);
        m_inputs->setDigitalSteerRight( steerLeftKeyPressed );
    }

    void PhysxVehicleInput::setDigitalSteerRight( const bool steerRightKeyPressed )
    {
        // m_inputs->setDigitalSteerRight(steerRightKeyPressed);
        m_inputs->setDigitalSteerLeft( steerRightKeyPressed );
    }

    auto PhysxVehicleInput::getDigitalAccel() const -> bool
    {
        return m_inputs->getDigitalAccel();
    }

    auto PhysxVehicleInput::getDigitalBrake() const -> bool
    {
        return m_inputs->getDigitalBrake();
    }

    auto PhysxVehicleInput::getDigitalHandbrake() const -> bool
    {
        return m_inputs->getDigitalHandbrake();
    }

    auto PhysxVehicleInput::getDigitalSteerLeft() const -> bool
    {
        return m_inputs->getDigitalSteerLeft();
    }

    auto PhysxVehicleInput::getDigitalSteerRight() const -> bool
    {
        return m_inputs->getDigitalSteerRight();
    }

    void PhysxVehicleInput::setAnalogAccel( const physics_Num accel )
    {
        m_inputs->setAnalogAccel( accel );
    }

    void PhysxVehicleInput::setAnalogBrake( const physics_Num brake )
    {
        m_inputs->setAnalogBrake( brake );
    }

    void PhysxVehicleInput::setAnalogHandbrake( const physics_Num handbrake )
    {
        m_inputs->setAnalogHandbrake( handbrake );
    }

    void PhysxVehicleInput::setAnalogSteer( const physics_Num steer )
    {
        m_inputs->setAnalogSteer( steer );
    }

    auto PhysxVehicleInput::getAnalogAccel() const -> physics_Num
    {
        return m_inputs->getAnalogAccel();
    }

    auto PhysxVehicleInput::getAnalogBrake() const -> physics_Num
    {
        return static_cast<physics_Num>( 0.0 );
    }

    auto PhysxVehicleInput::getAnalogHandbrake() const -> physics_Num
    {
        return static_cast<physics_Num>( 0.0 );
    }

    auto PhysxVehicleInput::getAnalogSteer() const -> physics_Num
    {
        return static_cast<physics_Num>( 0.0 );
    }

    void PhysxVehicleInput::setGearUp( const bool gearUpKeyPressed )
    {
        m_inputs->setGearUp( gearUpKeyPressed );
    }

    void PhysxVehicleInput::setGearDown( const bool gearDownKeyPressed )
    {
        m_inputs->setGearDown( gearDownKeyPressed );
    }

    auto PhysxVehicleInput::getGearUp() const -> bool
    {
        return false;
    }

    auto PhysxVehicleInput::getGearDown() const -> bool
    {
        return false;
    }

    auto PhysxVehicleInput::getInputs() const -> physx::PxVehicleDrive4WRawInputData *
    {
        return m_inputs;
    }

    void PhysxVehicleInput::setInputs( physx::PxVehicleDrive4WRawInputData *input )
    {
        m_inputs = input;
    }

} // namespace workphone::physics
