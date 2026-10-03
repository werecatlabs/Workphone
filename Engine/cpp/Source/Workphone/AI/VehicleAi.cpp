#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/AI/VehicleAi.hpp>
#include <Workphone/AI/VehicleAiManager.hpp>
#include <Workphone/AI/Learning.hpp>
#include <Workphone/Interface/Vehicle/IVehicle.hpp>

namespace workphone
{
    VehicleAi::VehicleAi() = default;

    VehicleAi::~VehicleAi() = default;

    void VehicleAi::update( f64, f64 )
    {
    }

    void VehicleAi::load( SmartPtr<ISharedObject> )
    {
    }

    void VehicleAi::reload( SmartPtr<ISharedObject> )
    {
    }

    void VehicleAi::unload( SmartPtr<ISharedObject> )
    {
    }

    void VehicleAi::reset()
    {
    }

    auto VehicleAi::getVehicleController() const -> SmartPtr<vehicle::IVehicle>
    {
        return m_vehicleController;
    }

    void VehicleAi::setVehicleController( SmartPtr<vehicle::IVehicle> vehicleController )
    {
        m_vehicleController = vehicleController;
    }

    auto VehicleAi::getVehicleManager() const -> SmartPtr<IVehicleAiManager>
    {
        return m_vehicleManager;
    }

    void VehicleAi::setVehicleManager( SmartPtr<IVehicleAiManager> vehicleManager )
    {
        m_vehicleManager = vehicleManager;
    }
}  // namespace workphone
