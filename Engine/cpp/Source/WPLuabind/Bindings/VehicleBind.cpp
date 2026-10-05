#include "WPLuabind/WPLuabindPCH.hpp"
#include "WPLuabind/Bindings/VehicleBind.hpp"
#include <luabind/luabind.hpp>
#include "WPLuabind/SmartPtrConverter.hpp"
#include "WPLuabind/ParamConverter.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Interface/Vehicle/IESController.hpp>
#include "luabind/operator.hpp"

namespace workphone
{
    void _setVehicleState( vehicle::IVehicle *vehicle, lua_Integer state )
    {
        vehicle->setState( static_cast<vehicle::IVehicle::State>( state ) );
    }

    lua_Integer _getVehicleState( const vehicle::IVehicle *vehicle )
    {
        return static_cast<lua_Integer>( vehicle->getState() );
    }

    void _setVehicleComponentState( vehicle::IVehicleComponent *component, lua_Integer state )
    {
        component->setState( static_cast<vehicle::IVehicleComponent::State>( state ) );
    }

    lua_Integer _getVehicleComponentState( const vehicle::IVehicleComponent *component )
    {
        return static_cast<lua_Integer>( component->getState() );
    }

    void _setVehicleDriveType( vehicle::IVehicle *vehicle, lua_Integer driveType )
    {
        vehicle->setDriveType( static_cast<VehicleDriveType>( driveType ) );
    }

    lua_Integer _getVehicleDriveType( const vehicle::IVehicle *vehicle )
    {
        return static_cast<lua_Integer>( vehicle->getDriveType() );
    }

    void _setTireModel( vehicle::IWheelComponent *wheel, lua_Integer tireModel )
    {
        wheel->setTireModel( static_cast<TireModel>( tireModel ) );
    }

    lua_Integer _getTireModel( const vehicle::IWheelComponent *wheel )
    {
        return static_cast<lua_Integer>( wheel->getTireModel() );
    }

    void bindVehicle( lua_State *L )
    {
        using namespace luabind;
        using namespace vehicle;

        module( L )[class_<IVehicleComponent, ISharedObject, SmartPtr<IVehicleComponent>>(
                        "IVehicleComponent" )
                        .def( "reset", &IVehicleComponent::reset )
                        .def( "getOwnerPtr", &IVehicleComponent::getOwnerPtr )
                        .def( "getOwner", &IVehicleComponent::getOwner )
                        .def( "setOwner", &IVehicleComponent::setOwner )
                        .def( "updateTransform", &IVehicleComponent::updateTransform )
                        .def( "updateBodyTransform", &IVehicleComponent::updateBodyTransform )
                        .def( "updateGeometry", &IVehicleComponent::updateGeometry )
                        .def( "getWorldTransform", &IVehicleComponent::getWorldTransform )
                        .def( "setWorldTransform", &IVehicleComponent::setWorldTransform )
                        .def( "getLocalTransform", &IVehicleComponent::getLocalTransform )
                        .def( "setLocalTransform", &IVehicleComponent::setLocalTransform )
                        .def( "setState", _setVehicleComponentState )
                        .def( "getState", _getVehicleComponentState )
                        .def( "getData", &IVehicleComponent::getData )
                        .def( "setData", &IVehicleComponent::setData )
                        .scope[def( "typeInfo", IVehicleComponent::typeInfo )]];

        module(
            L )[class_<IVehicleCallback, ISharedObject, SmartPtr<IVehicleCallback>>( "IVehicleCallback" )
                    .def( "getData", &IVehicleCallback::getData )
                    .def( "addForce", &IVehicleCallback::addForce )
                    .def( "addTorque", &IVehicleCallback::addTorque )
                    .def( "addLocalForce", &IVehicleCallback::addLocalForce )
                    .def( "addLocalTorque", &IVehicleCallback::addLocalTorque )
                    .def( "getAngularVelocity", &IVehicleCallback::getAngularVelocity )
                    .def( "getLinearVelocity", &IVehicleCallback::getLinearVelocity )
                    .def( "getLocalAngularVelocity", &IVehicleCallback::getLocalAngularVelocity )
                    .def( "getLocalLinearVelocity", &IVehicleCallback::getLocalLinearVelocity )
                    .def( "getPosition", &IVehicleCallback::getPosition )
                    .def( "getScale", &IVehicleCallback::getScale )
                    .def( "getOrientation", &IVehicleCallback::getOrientation )
                    .def( "displayLocalVector",
                          static_cast<void ( IVehicleCallback::* )(
                              s32, const Vector3<real_Num> &, const Vector3<real_Num> &, s32 ) const>(
                              &IVehicleCallback::displayLocalVector ) )
                    .def( "displayVector", &IVehicleCallback::displayVector )
                    .def( "displayLocalVector",
                          static_cast<void ( IVehicleCallback::* )( s32, s32, const Vector3<real_Num> &,
                                                                    const Vector3<real_Num> &, s32 )
                                          const>( &IVehicleCallback::displayLocalVector ) )
                    .def( "getCallbackFunction", &IVehicleCallback::getCallbackFunction )
                    .def( "setCallbackFunction", &IVehicleCallback::setCallbackFunction )
                    .def( "getCallbackDataFunction", &IVehicleCallback::getCallbackDataFunction )
                    .def( "setCallbackDataFunction", &IVehicleCallback::setCallbackDataFunction )
                    .def( "getInputData", &IVehicleCallback::getInputData )
                    .def( "getControlAngles", &IVehicleCallback::getControlAngles )
                    .def( "getPointVelocity", &IVehicleCallback::getPointVelocity )
                    .def( "castLocalRay", &IVehicleCallback::castLocalRay )
                    .def( "castWorldRay", &IVehicleCallback::castWorldRay )
                    .scope[def( "typeInfo", IVehicleCallback::typeInfo )]];

        module( L )[class_<IVehicle, ISharedObject, SmartPtr<IVehicle>>( "IVehicle" )
                        .def( "updateTransform", &IVehicle::updateTransform )
                        .def( "reset", &IVehicle::reset )
                        .def( "getChannel", &IVehicle::getChannel )
                        .def( "setChannel", &IVehicle::setChannel )
                        .def( "getPosition", &IVehicle::getPosition )
                        .def( "setPosition", &IVehicle::setPosition )
                        .def( "getOrientation", &IVehicle::getOrientation )
                        .def( "setOrientation", &IVehicle::setOrientation )
                        .def( "getScale", &IVehicle::getScale )
                        .def( "isUserControlled", &IVehicle::isUserControlled )
                        .def( "setUserControlled", &IVehicle::setUserControlled )
                        .def( "getMass", &IVehicle::getMass )
                        .def( "setMass", &IVehicle::setMass )
                        .def( "getBody", &IVehicle::getBody )
                        .def( "setBody", &IVehicle::setBody )
                        .def( "getWorldTransform", &IVehicle::getWorldTransform )
                        .def( "setWorldTransform", &IVehicle::setWorldTransform )
                        .def( "getLocalTransform", &IVehicle::getLocalTransform )
                        .def( "setLocalTransform", &IVehicle::setLocalTransform )
                        .def( "drawPoint", &IVehicle::drawPoint )
                        .def( "drawLocalPoint", &IVehicle::drawLocalPoint )
                        .def( "displayLocalVector",
                              static_cast<void ( IVehicle::* )( s32, const Vector3<real_Num> &,
                                                                const Vector3<real_Num> &, u32 )>(
                                  &IVehicle::displayLocalVector ) )
                        .def( "displayVector", &IVehicle::displayVector )
                        .def( "displayLocalVector",
                              static_cast<void ( IVehicle::* )( s32, s32, const Vector3<real_Num> &,
                                                                const Vector3<real_Num> &, u32 )>(
                                  &IVehicle::displayLocalVector ) )
                        .def( "getDisplayDebugData", &IVehicle::getDisplayDebugData )
                        .def( "setDisplayDebugData", &IVehicle::setDisplayDebugData )
                        .def( "addForce", &IVehicle::addForce )
                        .def( "addTorque", &IVehicle::addTorque )
                        .def( "addLocalForce", &IVehicle::addLocalForce )
                        .def( "addLocalTorque", &IVehicle::addLocalTorque )
                        .def( "getPointVelocity", &IVehicle::getPointVelocity )
                        .def( "getAngularVelocity", &IVehicle::getAngularVelocity )
                        .def( "getLinearVelocity", &IVehicle::getLinearVelocity )
                        .def( "getLocalAngularVelocity", &IVehicle::getLocalAngularVelocity )
                        .def( "getLocalLinearVelocity", &IVehicle::getLocalLinearVelocity )
                        .def( "getVehicleCallback", &IVehicle::getVehicleCallback )
                        .def( "setVehicleCallback", &IVehicle::setVehicleCallback )
                        .def( "getCG", &IVehicle::getCG )
                        .def( "setState", _setVehicleState )
                        .def( "getState", _getVehicleState )
                        .def( "getWheelController", &IVehicle::getWheelController )
                        .def( "getDriveTrain", &IVehicle::getDriveTrain )
                        .def( "setDriveTrain", &IVehicle::setDriveTrain )
                        .def( "getDriveType", _getVehicleDriveType )
                        .def( "setDriveType", _setVehicleDriveType )
                        .def( "isElectric", &IVehicle::isElectric )
                        .def( "setElectric", &IVehicle::setElectric )
                        .def( "getEnablePowerUnit", &IVehicle::getEnablePowerUnit )
                        .def( "setEnablePowerUnit", &IVehicle::setEnablePowerUnit )
                        .def( "getEmulateBattery", &IVehicle::getEmulateBattery )
                        .def( "setEmulateBattery", &IVehicle::setEmulateBattery )
                        .scope[def( "typeInfo", IVehicle::typeInfo )]];

        module(
            L )[class_<IVehicleBody, IVehicleComponent, SmartPtr<IVehicleBody>>( "IVehicleBody" )
                    .def( "getVelocity", &IVehicleBody::getVelocity )
                    .def( "setVelocity", &IVehicleBody::setVelocity )
                    .def( "getAngularVelocity", &IVehicleBody::getAngularVelocity )
                    .def( "setAngularVelocity", &IVehicleBody::setAngularVelocity )
                    .def( "getLocalVelocity", &IVehicleBody::getLocalVelocity )
                    .def( "setLocalVelocity", &IVehicleBody::setLocalVelocity )
                    .def( "getLocalAngularVelocity", &IVehicleBody::getLocalAngularVelocity )
                    .def( "setLocalAngularVelocity", &IVehicleBody::setLocalAngularVelocity )
                    .def( "getWorldCenterOfMass", &IVehicleBody::getWorldCenterOfMass )
                    .def( "setWorldCenterOfMass", &IVehicleBody::setWorldCenterOfMass )
                    .def( "getPointVelocity", &IVehicleBody::getPointVelocity )
                    .def( "addLocalForceAtLocalPosition", &IVehicleBody::addLocalForceAtLocalPosition )
                    .def( "addForceAtPosition", &IVehicleBody::addForceAtPosition )
                    .def( "addTorque", &IVehicleBody::addTorque )
                    .def( "addLocalTorque", &IVehicleBody::addLocalTorque )
                    .def( "castLocalRay", &IVehicleBody::castLocalRay )
                    .def( "castWorldRay", &IVehicleBody::castWorldRay )
                    .def( "getMass", &IVehicleBody::getMass )
                    .def( "setMass", &IVehicleBody::setMass )
                    .scope[def( "typeInfo", IVehicleBody::typeInfo )]];

        module(
            L )[class_<IVehicleManager, ISharedObject, SmartPtr<IVehicleManager>>( "IVehicleManager" )
                    .def( "createVehicle", &IVehicleManager::createVehicle )
                    .def( "destroyVehicle", &IVehicleManager::destroyVehicle )
                    .def( "addVehicle", &IVehicleManager::addVehicle )
                    .def( "removeVehicle", &IVehicleManager::removeVehicle )
                    .scope[def( "typeInfo", IVehicleManager::typeInfo )]];

        module( L )[class_<IVehiclePowerUnit, IVehicleComponent, SmartPtr<IVehiclePowerUnit>>(
                        "IVehiclePowerUnit" )
                        .def( "setRPM", &IVehiclePowerUnit::setRPM )
                        .def( "getRPM", &IVehiclePowerUnit::getRPM )
                        .def( "getMaxRPM", &IVehiclePowerUnit::getMaxRPM )
                        .def( "getTorque", static_cast<real_Num ( IVehiclePowerUnit::* )() const>(
                                               &IVehiclePowerUnit::getTorque ) )
                        .def( "getTorque", static_cast<f32 ( IVehiclePowerUnit::* )( f32 ) const>(
                                               &IVehiclePowerUnit::getTorque ) )
                        .def( "getMaxTorque", &IVehiclePowerUnit::getMaxTorque )
                        .def( "getMinTorque", &IVehiclePowerUnit::getMinTorque )
                        .def( "getThrottle", &IVehiclePowerUnit::getThrottle )
                        .def( "setThrottle", &IVehiclePowerUnit::setThrottle )
                        .scope[def( "typeInfo", IVehiclePowerUnit::typeInfo )]];

        module( L )[class_<IWheelComponent, IVehicleComponent, SmartPtr<IWheelComponent>>(
                        "IWheelComponent" )
                        .def( "addTorque", &IWheelComponent::addTorque )
                        .def( "setTorque", &IWheelComponent::setTorque )
                        .def( "getTorque", &IWheelComponent::getTorque )
                        .def( "getMass", &IWheelComponent::getMass )
                        .def( "setMass", &IWheelComponent::setMass )
                        .def( "getSpringRate", &IWheelComponent::getSpringRate )
                        .def( "setSpringRate", &IWheelComponent::setSpringRate )
                        .def( "getRadius", &IWheelComponent::getRadius )
                        .def( "setRadius", &IWheelComponent::setRadius )
                        .def( "getSuspensionTravel", &IWheelComponent::getSuspensionTravel )
                        .def( "setSuspensionTravel", &IWheelComponent::setSuspensionTravel )
                        .def( "getDamping", &IWheelComponent::getDamping )
                        .def( "setDamping", &IWheelComponent::setDamping )
                        .def( "getSuspensionDistance", &IWheelComponent::getSuspensionDistance )
                        .def( "setSuspensionDistance", &IWheelComponent::setSuspensionDistance )
                        .def( "getSteeringAngle", &IWheelComponent::getSteeringAngle )
                        .def( "setSteeringAngle", &IWheelComponent::setSteeringAngle )
                        .def( "isSteeringWheel", &IWheelComponent::isSteeringWheel )
                        .def( "setSteeringWheel", &IWheelComponent::setSteeringWheel )
                        .def( "isPoweredWheel", &IWheelComponent::isPoweredWheel )
                        .def( "setPoweredWheel", &IWheelComponent::setPoweredWheel )
                        .def( "getAngularVelocity", &IWheelComponent::getAngularVelocity )
                        .def( "setAngularVelocity", &IWheelComponent::setAngularVelocity )
                        .def( "getBrake", &IWheelComponent::getBrake )
                        .def( "setBrake", &IWheelComponent::setBrake )
                        .def( "getTireModel", _getTireModel )
                        .def( "setTireModel", _setTireModel )
                        .scope[def( "typeInfo", IWheelComponent::typeInfo )]];

        module( L )[class_<IBatteryPack, IVehicleComponent, SmartPtr<IBatteryPack>>( "IBatteryPack" )
                        .def( "charge", &IBatteryPack::charge )
                        .def( "discharge", &IBatteryPack::discharge )
                        .def( "getDischargeRate", &IBatteryPack::getDischargeRate )
                        .def( "setDischargeRate", &IBatteryPack::setDischargeRate )
                        .def( "getVolts", &IBatteryPack::getVolts )
                        .def( "setVolts", &IBatteryPack::setVolts )
                        .def( "getCharge", &IBatteryPack::getCharge )
                        .def( "setCharge", &IBatteryPack::setCharge )
                        .def( "getNumCells", &IBatteryPack::getNumCells )
                        .def( "setNumCells", &IBatteryPack::setNumCells )
                        .def( "getVoltage", &IBatteryPack::getVoltage )
                        .def( "setVoltage", &IBatteryPack::setVoltage )
                        .def( "getResistance", &IBatteryPack::getResistance )
                        .def( "setResistance", &IBatteryPack::setResistance )
                        .def( "getTerminalVoltage", &IBatteryPack::getTerminalVoltage )
                        .def( "setTerminalVoltage", &IBatteryPack::setTerminalVoltage )
                        .def( "getEmulateBattery", &IBatteryPack::getEmulateBattery )
                        .def( "setEmulateBattery", &IBatteryPack::setEmulateBattery )
                        .scope[def( "typeInfo", IBatteryPack::typeInfo )]];

        module( L )[class_<IGearBox, IVehicleComponent, SmartPtr<IGearBox>>( "IGearBox" )
                        .def( "setRatios", &IGearBox::setRatios )
                        .def( "getRatios", &IGearBox::getRatios )
                        .def( "getRatio", &IGearBox::getRatio )
                        .def( "getNumGears", &IGearBox::getNumGears )
                        .def( "decreamentSelectedGear", &IGearBox::decreamentSelectedGear )
                        .def( "increamentSelectedGear", &IGearBox::increamentSelectedGear )
                        .def( "getCurrentGear", &IGearBox::getCurrentGear )
                        .scope[def( "typeInfo", IGearBox::typeInfo )]];

        module( L )[class_<IDifferential, IVehicleComponent, SmartPtr<IDifferential>>( "IDifferential" )
                        .def( "setRatio", &IDifferential::setRatio )
                        .def( "getRatio", &IDifferential::getRatio )
                        .def( "setWheel", &IDifferential::setWheel )
                        .def( "getWheel", &IDifferential::getWheel )
                        .def( "setWheelTorque", &IDifferential::setWheelTorque )
                        .def( "getLockCoefficient", &IDifferential::getLockCoefficient )
                        .def( "setLockCoefficient", &IDifferential::setLockCoefficient )
                        .scope[def( "typeInfo", IDifferential::typeInfo )]];

        module( L )[class_<IDriveTrain, IVehicleComponent, SmartPtr<IDriveTrain>>( "IDriveTrain" )
                        .def( "getWheels", &IDriveTrain::getWheels )
                        .def( "setWheels", &IDriveTrain::setWheels )
                        .def( "getGearBox", &IDriveTrain::getGearBox )
                        .def( "setGearBox", &IDriveTrain::setGearBox )
                        .def( "getDifferential", &IDriveTrain::getDifferential )
                        .def( "setDifferential", &IDriveTrain::setDifferential )
                        .def( "getThrottle", &IDriveTrain::getThrottle )
                        .def( "setThrottle", &IDriveTrain::setThrottle )
                        .def( "getThrottleInput", &IDriveTrain::getThrottleInput )
                        .def( "setThrottleInput", &IDriveTrain::setThrottleInput )
                        .def( "getBrake", &IDriveTrain::getBrake )
                        .def( "setBrake", &IDriveTrain::setBrake )
                        .def( "getBrakeInput", &IDriveTrain::getBrakeInput )
                        .def( "setBrakeInput", &IDriveTrain::setBrakeInput )
                        .scope[def( "typeInfo", IDriveTrain::typeInfo )]];

        module( L )[class_<IESController, IVehicleComponent, SmartPtr<IESController>>( "IESController" )
                        .def( "getBatteryPack", &IESController::getBatteryPack )
                        .def( "setBatteryPack", &IESController::setBatteryPack )
                        .def( "getEsc", &IESController::getEsc )
                        .def( "setEsc", &IESController::setEsc )
                        .def( "getMotor", &IESController::getMotor )
                        .def( "setMotor", &IESController::setMotor )
                        .scope[def( "typeInfo", IESController::typeInfo )]];

        module(
            L )[class_<IElectricMotor, IVehicleComponent, SmartPtr<IElectricMotor>>( "IElectricMotor" )
                    .scope[def( "typeInfo", IElectricMotor::typeInfo )]];

        module( L )[class_<IGroundEffect, IVehicleComponent, SmartPtr<IGroundEffect>>( "IGroundEffect" )
                        .scope[def( "typeInfo", IGroundEffect::typeInfo )]];

        // These marker interfaces do not export their own typeInfo from Workphone.dll.
        module(
            L )[class_<IFourWheelVehicle, IVehicle, SmartPtr<IFourWheelVehicle>>( "IFourWheelVehicle" )];
        module( L )[class_<ITruckVehicle, IVehicle, SmartPtr<ITruckVehicle>>( "ITruckVehicle" )];
        module( L )[class_<IDrone, IVehicle, SmartPtr<IDrone>>( "IDrone" )
                        .def( "getGroundEffectMultiplier", &IDrone::getGroundEffectMultiplier )
                        .def( "setGroundEffectMultiplier", &IDrone::setGroundEffectMultiplier )
                        .def( "getDrag", &IDrone::getDrag )
                        .def( "setDrag", &IDrone::setDrag )
                        .scope[def( "typeInfo", IDrone::typeInfo )]];

        module( L )[class_<IAerodymanicsWind, IVehicleComponent, SmartPtr<IAerodymanicsWind>>(
                        "IAerodymanicsWind" )
                        .def( "setWind", &IAerodymanicsWind::setWind )
                        .def( "getWind", &IAerodymanicsWind::getWind )
                        .def( "getAirDensity", &IAerodymanicsWind::getAirDensity )
                        .def( "setAirDensity", &IAerodymanicsWind::setAirDensity )
                        .scope[def( "typeInfo", IAerodymanicsWind::typeInfo )]];

        module( L )[class_<IAerofoil, ISharedObject, SmartPtr<IAerofoil>>( "IAerofoil" )
                        .def( "getCLPtr", &IAerofoil::getCLPtr )
                        .def( "getCL", &IAerofoil::getCL )
                        .def( "setCL", &IAerofoil::setCL )
                        .def( "getCDPtr", &IAerofoil::getCDPtr )
                        .def( "getCD", &IAerofoil::getCD )
                        .def( "setCD", &IAerofoil::setCD )
                        .def( "getCMPtr", &IAerofoil::getCMPtr )
                        .def( "getCM", &IAerofoil::getCM )
                        .def( "setCM", &IAerofoil::setCM )
                        .def( "getAircraftPtr", &IAerofoil::getAircraftPtr )
                        .def( "getAircraft", &IAerofoil::getAircraft )
                        .def( "setAircraft", &IAerofoil::setAircraft )
                        .def( "getReverseValues", &IAerofoil::getReverseValues )
                        .def( "setReverseValues", &IAerofoil::setReverseValues )
                        .scope[def( "typeInfo", IAerofoil::typeInfo )]];

        module(
            L )[class_<IAircraftCallback, IVehicleCallback, SmartPtr<IAircraftCallback>>(
                    "IAircraftCallback" )
                    .def( "addForce",
                          static_cast<void ( IAircraftCallback::* )( s32, const Vector3<real_Num> & )>(
                              &IAircraftCallback::addForce ) )
                    .scope[def( "typeInfo", IAircraftCallback::typeInfo )]];

        module( L )[class_<IAircraftBody, IVehicleBody, SmartPtr<IAircraftBody>>( "IAircraftBody" )
                        .scope[def( "typeInfo", IAircraftBody::typeInfo )]];

        module( L )[class_<IAircraft, IVehicle, SmartPtr<IAircraft>>( "IAircraft" )
                        .def( "getAirDensity", &IAircraft::getAirDensity )
                        .def( "setAirDensity", &IAircraft::setAirDensity )
                        .def( "getBatteryPack", &IAircraft::getBatteryPack )
                        .def( "setBatteryPack", &IAircraft::setBatteryPack )
                        .def( "getCallback", &IAircraft::getCallback )
                        .def( "setCallback", &IAircraft::setCallback )
                        .def( "getWind", &IAircraft::getWind )
                        .def( "setWind", &IAircraft::setWind )
                        .def( "addPropellerUnit", &IAircraft::addPropellerUnit )
                        .def( "removePropellerUnit", &IAircraft::removePropellerUnit )
                        .def( "getPropellerUnits", &IAircraft::getPropellerUnits )
                        .def( "setPropellerUnits", &IAircraft::setPropellerUnits )
                        .def( "addWheel", &IAircraft::addWheel )
                        .def( "removeWheel", &IAircraft::removeWheel )
                        .def( "getWheels", &IAircraft::getWheels )
                        .def( "setWheels", &IAircraft::setWheels )
                        .def( "getEngineRPM", &IAircraft::getEngineRPM )
                        .def( "getThrust", &IAircraft::getThrust )
                        .def( "setControlAngle", &IAircraft::setControlAngle )
                        .def( "getSectionMultiplier", &IAircraft::getSectionMultiplier )
                        .def( "setSectionMultiplier", &IAircraft::setSectionMultiplier )
                        .def( "getModelDataFilePath", &IAircraft::getModelDataFilePath )
                        .def( "setModelDataFilePath", &IAircraft::setModelDataFilePath )
                        .def( "getBodyTransform", &IAircraft::getBodyTransform )
                        .def( "setBodyTransform", &IAircraft::setBodyTransform )
                        .def( "getRollwiseDamping", &IAircraft::getRollwiseDamping )
                        .def( "setRollwiseDamping", &IAircraft::setRollwiseDamping )
                        .scope[def( "typeInfo", IAircraft::typeInfo )]];

        module( L )[class_<IAircraftPlane, IAircraft, SmartPtr<IAircraftPlane>>( "IAircraftPlane" )];
        module( L )[class_<IHelicopter, IAircraft, SmartPtr<IHelicopter>>( "IHelicopter" )
                        .scope[def( "typeInfo", IHelicopter::typeInfo )]];

        module( L )[class_<IAircraftPowerUnit, IVehiclePowerUnit, SmartPtr<IAircraftPowerUnit>>(
                        "IAircraftPowerUnit" )
                        .def( "getPropellerPtr", &IAircraftPowerUnit::getPropellerPtr )
                        .def( "getPropeller", &IAircraftPowerUnit::getPropeller )
                        .def( "setPropeller", &IAircraftPowerUnit::setPropeller )
                        .def( "isElectric", &IAircraftPowerUnit::isElectric )
                        .def( "setElectric", &IAircraftPowerUnit::setElectric )
                        .def( "getThrustMultiplier", &IAircraftPowerUnit::getThrustMultiplier )
                        .def( "setThrustMultiplier", &IAircraftPowerUnit::setThrustMultiplier )
                        .def( "getTorqueMultiplier", &IAircraftPowerUnit::getTorqueMultiplier )
                        .def( "setTorqueMultiplier", &IAircraftPowerUnit::setTorqueMultiplier )
                        .def( "getPeakPowerW", &IAircraftPowerUnit::getPeakPowerW )
                        .def( "setPeakPowerW", &IAircraftPowerUnit::setPeakPowerW )
                        .def( "getMoi", &IAircraftPowerUnit::getMoi )
                        .def( "setMoi", &IAircraftPowerUnit::setMoi )
                        .scope[def( "typeInfo", IAircraftPowerUnit::typeInfo )]];

        module( L )[class_<IAircraftPropeller, IVehicleComponent, SmartPtr<IAircraftPropeller>>(
                        "IAircraftPropeller" )
                        .def( "getDiameter", &IAircraftPropeller::getDiameter )
                        .def( "setDiameter", &IAircraftPropeller::setDiameter )
                        .def( "getThrust", &IAircraftPropeller::getThrust )
                        .def( "setThrust", &IAircraftPropeller::setThrust )
                        .def( "getThrustValue", &IAircraftPropeller::getThrustValue )
                        .def( "setThrustValue", &IAircraftPropeller::setThrustValue )
                        .def( "getPropwash", &IAircraftPropeller::getPropwash )
                        .def( "setPropwash", &IAircraftPropeller::setPropwash )
                        .def( "getThrustLine", &IAircraftPropeller::getThrustLine )
                        .def( "setThrustLine", &IAircraftPropeller::setThrustLine )
                        .def( "getDownThrust", &IAircraftPropeller::getDownThrust )
                        .def( "setDownThrust", &IAircraftPropeller::setDownThrust )
                        .def( "getSideThrust", &IAircraftPropeller::getSideThrust )
                        .def( "setSideThrust", &IAircraftPropeller::setSideThrust )
                        .scope[def( "typeInfo", IAircraftPropeller::typeInfo )]];

        module(
            L )[class_<IAircraftPropellerUnit, IVehicleComponent, SmartPtr<IAircraftPropellerUnit>>(
                    "IAircraftPropellerUnit" )
                    .def( "getBatteryPack",
                          static_cast<const SmartPtr<IBatteryPack> &(IAircraftPropellerUnit::*)() const>(
                              &IAircraftPropellerUnit::getBatteryPack ) )
                    .def( "setBatteryPack", &IAircraftPropellerUnit::setBatteryPack )
                    .def( "getESC",
                          static_cast<const SmartPtr<IESController> &(IAircraftPropellerUnit::*)()
                                          const>( &IAircraftPropellerUnit::getESC ) )
                    .def( "setESC", &IAircraftPropellerUnit::setESC )
                    .def( "getPowerUnit",
                          static_cast<const SmartPtr<IAircraftPowerUnit> &(IAircraftPropellerUnit::*)()
                                          const>( &IAircraftPropellerUnit::getPowerUnit ) )
                    .def( "setPowerUnit", &IAircraftPropellerUnit::setPowerUnit )
                    .def( "getPropeller",
                          static_cast<const SmartPtr<IAircraftPropeller> &(IAircraftPropellerUnit::*)()
                                          const>( &IAircraftPropellerUnit::getPropeller ) )
                    .def( "setPropeller", &IAircraftPropellerUnit::setPropeller )
                    .def( "getThrust", &IAircraftPropellerUnit::getThrust )
                    .def( "setThrust", &IAircraftPropellerUnit::setThrust )
                    .def( "getPropwash", &IAircraftPropellerUnit::getPropwash )
                    .def( "setPropwash", &IAircraftPropellerUnit::setPropwash )
                    .scope[def( "typeInfo", IAircraftPropellerUnit::typeInfo )]];

        module(
            L )[class_<IAircraftTurbineUnit, IVehicleComponent, SmartPtr<IAircraftTurbineUnit>>(
                    "IAircraftTurbineUnit" )
                    .def( "getBatteryPack",
                          static_cast<const SmartPtr<IBatteryPack> &(IAircraftTurbineUnit::*)() const>(
                              &IAircraftTurbineUnit::getBatteryPack ) )
                    .def( "setBatteryPack", &IAircraftTurbineUnit::setBatteryPack )
                    .def( "getESC",
                          static_cast<const SmartPtr<IESController> &(IAircraftTurbineUnit::*)() const>(
                              &IAircraftTurbineUnit::getESC ) )
                    .def( "setESC", &IAircraftTurbineUnit::setESC )
                    .def( "getPowerUnit",
                          static_cast<const SmartPtr<IAircraftPowerUnit> &(IAircraftTurbineUnit::*)()
                                          const>( &IAircraftTurbineUnit::getPowerUnit ) )
                    .def( "setPowerUnit", &IAircraftTurbineUnit::setPowerUnit )
                    .def( "getPropeller",
                          static_cast<const SmartPtr<IAircraftPropeller> &(IAircraftTurbineUnit::*)()
                                          const>( &IAircraftTurbineUnit::getPropeller ) )
                    .def( "setPropeller", &IAircraftTurbineUnit::setPropeller )
                    .def( "getThrust", &IAircraftTurbineUnit::getThrust )
                    .scope[def( "typeInfo", IAircraftTurbineUnit::typeInfo )]];

        module(
            L )[class_<IAircraftPropWash, IVehicleComponent, SmartPtr<IAircraftPropWash>>(
                    "IAircraftPropWash" )
                    .def( "getPropWash", static_cast<Vector3<real_Num> ( IAircraftPropWash::* )()>(
                                             &IAircraftPropWash::getPropWash ) )
                    .def( "getPropWash", static_cast<Vector3<real_Num> ( IAircraftPropWash::* )( s32 )>(
                                             &IAircraftPropWash::getPropWash ) )
                    .def( "getPropellerUnit", &IAircraftPropWash::getPropellerUnit )
                    .def( "setPropellerUnit", &IAircraftPropWash::setPropellerUnit )
                    .def( "getAffectedSections",
                          static_cast<const Array<bool> &(IAircraftPropWash::*)() const>(
                              &IAircraftPropWash::getAffectedSections ) )
                    .def( "setAffectedSections", &IAircraftPropWash::setAffectedSections )
                    .def( "getSectionMultipliers",
                          static_cast<const Array<float> &(IAircraftPropWash::*)() const>(
                              &IAircraftPropWash::getSectionMultipliers ) )
                    .def( "setSectionMultipliers", &IAircraftPropWash::setSectionMultipliers )
                    .def( "getStrength", &IAircraftPropWash::getStrength )
                    .def( "setStrength", &IAircraftPropWash::setStrength )
                    .scope[def( "typeInfo", IAircraftPropWash::typeInfo )]];

        module( L )[class_<IAircraftWing, IVehicleComponent, SmartPtr<IAircraftWing>>( "IAircraftWing" )
                        .def( "getAttachedControlSurface", &IAircraftWing::getAttachedControlSurface )
                        .def( "setAttachedControlSurface", &IAircraftWing::setAttachedControlSurface )
                        .def( "getAttachedPropWash", &IAircraftWing::getAttachedPropWash )
                        .def( "setAttachedPropWash", &IAircraftWing::setAttachedPropWash )
                        .def( "isControlSurface", &IAircraftWing::isControlSurface )
                        .def( "setControlSurface", &IAircraftWing::setControlSurface )
                        .def( "useCombinedControlSurface", &IAircraftWing::useCombinedControlSurface )
                        .def( "setUserCombinedControlSurface",
                              &IAircraftWing::setUserCombinedControlSurface )
                        .scope[def( "typeInfo", IAircraftWing::typeInfo )]];

        module(
            L )[class_<IAircraftControlSurface, IVehicleComponent, SmartPtr<IAircraftControlSurface>>(
                    "IAircraftControlSurface" )
                    .def( "getSurfaceId", &IAircraftControlSurface::getSurfaceId )
                    .def( "setSurfaceId", &IAircraftControlSurface::setSurfaceId )
                    .def( "getCurrentDeflection", &IAircraftControlSurface::getCurrentDeflection )
                    .def( "setCurrentDeflection", &IAircraftControlSurface::setCurrentDeflection )
                    .def( "isReversed", &IAircraftControlSurface::isReversed )
                    .def( "setReversed", &IAircraftControlSurface::setReversed )
                    .def( "getAffectedSections",
                          static_cast<const Array<bool> &(IAircraftControlSurface::*)() const>(
                              &IAircraftControlSurface::getAffectedSections ) )
                    .def( "setAffectedSections", &IAircraftControlSurface::setAffectedSections )
                    .def( "isAffectedSection", &IAircraftControlSurface::isAffectedSection )
                    .def( "modifyWingGeometry",
                          static_cast<void ( IAircraftControlSurface::* )(
                              s32, Vector3<real_Num> &, Vector3<real_Num> &, Vector3<real_Num> &,
                              Vector3<real_Num> & )>( &IAircraftControlSurface::modifyWingGeometry ) )
                    .def( "modifyWingGeometry",
                          static_cast<void ( IAircraftControlSurface::* )(
                              s32, Vector3<real_Num> &, Vector3<real_Num> &, Vector3<real_Num> &,
                              Vector3<real_Num> &, real_Num )>(
                              &IAircraftControlSurface::modifyWingGeometry ) )
                    .def( "modifyMainWingGeometry", &IAircraftControlSurface::modifyMainWingGeometry )
                    .def( "getClLookup", &IAircraftControlSurface::getClLookup )
                    .def( "setClLookup", &IAircraftControlSurface::setClLookup )
                    .def( "getCdLookup", &IAircraftControlSurface::getCdLookup )
                    .def( "setCdLookup", &IAircraftControlSurface::setCdLookup )
                    .def( "getCmLookup", &IAircraftControlSurface::getCmLookup )
                    .def( "setCmLookup", &IAircraftControlSurface::setCmLookup )
                    .def( "getClMultiplier", &IAircraftControlSurface::getClMultiplier )
                    .def( "setClMultiplier", &IAircraftControlSurface::setClMultiplier )
                    .def( "getCdMultiplier", &IAircraftControlSurface::getCdMultiplier )
                    .def( "setCdMultiplier", &IAircraftControlSurface::setCdMultiplier )
                    .def( "getCmMultiplier", &IAircraftControlSurface::getCmMultiplier )
                    .def( "setCmMultiplier", &IAircraftControlSurface::setCmMultiplier )
                    .def( "getStallControlCL", &IAircraftControlSurface::getStallControlCL )
                    .def( "setStallControlCL", &IAircraftControlSurface::setStallControlCL )
                    .def( "getStallControlCD", &IAircraftControlSurface::getStallControlCD )
                    .def( "setStallControlCD", &IAircraftControlSurface::setStallControlCD )
                    .def( "getStallControlCM", &IAircraftControlSurface::getStallControlCM )
                    .def( "setStallControlCM", &IAircraftControlSurface::setStallControlCM )
                    .def( "getStallThreshold", &IAircraftControlSurface::getStallThreshold )
                    .def( "setStallThreshold", &IAircraftControlSurface::setStallThreshold )
                    .def( "getMinDeflectionDegrees", &IAircraftControlSurface::getMinDeflectionDegrees )
                    .def( "setMinDeflectionDegrees", &IAircraftControlSurface::setMinDeflectionDegrees )
                    .def( "getMaxDeflectionDegrees", &IAircraftControlSurface::getMaxDeflectionDegrees )
                    .def( "setMaxDeflectionDegrees", &IAircraftControlSurface::setMaxDeflectionDegrees )
                    .def( "getAoAMultiplier", &IAircraftControlSurface::getAoAMultiplier )
                    .def( "setAoAMultiplier", &IAircraftControlSurface::setAoAMultiplier )
                    .def( "getMainWing", &IAircraftControlSurface::getMainWing )
                    .def( "setMainWing", &IAircraftControlSurface::setMainWing )
                    .def( "getControlWing", &IAircraftControlSurface::getControlWing )
                    .def( "setControlWing", &IAircraftControlSurface::setControlWing )
                    .def( "getRootHingeDistanceFromTrailingEdge",
                          &IAircraftControlSurface::getRootHingeDistanceFromTrailingEdge )
                    .def( "setRootHingeDistanceFromTrailingEdge",
                          &IAircraftControlSurface::setRootHingeDistanceFromTrailingEdge )
                    .def( "getTipHingeDistanceFromTrailingEdge",
                          &IAircraftControlSurface::getTipHingeDistanceFromTrailingEdge )
                    .def( "setTipHingeDistanceFromTrailingEdge",
                          &IAircraftControlSurface::setTipHingeDistanceFromTrailingEdge )
                    .scope[def( "typeInfo", IAircraftControlSurface::typeInfo )]];
    }
} // namespace workphone
