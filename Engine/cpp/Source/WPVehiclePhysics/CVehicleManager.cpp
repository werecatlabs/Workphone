#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include <WPVehiclePhysics/CVehicleManager.hpp>
#include <WPVehiclePhysics/CTruckController.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    CVehicleManager::CVehicleManager()
    {
        auto pNewVehicles = workphone::make_shared<Array<SmartPtr<IVehicle>>>();
        setVehicles(pNewVehicles);
    }

    CVehicleManager::~CVehicleManager() = default;

    void CVehicleManager::preUpdate()
    {
        auto pVehicles = getVehicles();
        auto &vehicles = *pVehicles;
        for(auto &vehicle : vehicles)
        {
            vehicle->preUpdate();
        }
    }

    void CVehicleManager::update()
    {
        auto pVehicles = getVehicles();
        auto &vehicles = *pVehicles;
        for(auto &vehicle : vehicles)
        {
            vehicle->update();
        }
    }

    void CVehicleManager::postUpdate()
    {
        auto pVehicles = getVehicles();
        auto &vehicles = *pVehicles;
        for(auto &vehicle : vehicles)
        {
            vehicle->postUpdate();
        }
    }

    void CVehicleManager::addVehicle(SmartPtr<IVehicle> vehicle)
    {
        auto pVehicles = getVehicles();
        auto &vehicles = *pVehicles;

        auto pNewVehicles = workphone::make_shared<Array<SmartPtr<IVehicle>>>();
        *pNewVehicles = Array<SmartPtr<IVehicle>>(vehicles.begin(), vehicles.end());
        pNewVehicles->push_back(vehicle);
        setVehicles(pNewVehicles);
    }

    void CVehicleManager::removeVehicle(SmartPtr<IVehicle> vehicle)
    {
        auto pVehicles = getVehicles();
        auto &vehicles = *pVehicles;
        auto vehiclesArray = Array<SmartPtr<IVehicle>>(vehicles.begin(), vehicles.end());

        auto it = std::find(vehiclesArray.begin(), vehiclesArray.end(), vehicle);
        if(it != vehiclesArray.end())
        {
            vehiclesArray.erase(it);

            auto pNewVehicles = workphone::make_shared<Array<SmartPtr<IVehicle>>>();
            *pNewVehicles = Array<SmartPtr<IVehicle>>(vehiclesArray.begin(), vehiclesArray.end());
            setVehicles(pNewVehicles);
        }
    }

    SmartPtr<IVehicle> CVehicleManager::createVehicle(hash64 type)
    {
        // auto typeManager = TypeManager::instance();
        // WP_ASSERT( typeManager );

        // const auto truckTypeInfo = ITruckController::typeInfo();

        // const auto truckType = typeManager->getHash( truckTypeInfo );

        // if( type == truckType )
        //{
        //     return fb::make_ptr<CTruckController>();
        // }

        return nullptr;
    }

    void CVehicleManager::destroyVehicle(SmartPtr<IVehicle> vehicle)
    {
    }

    SharedPtr<Array<SmartPtr<IVehicle>>> CVehicleManager::getVehicles() const
    {
        return m_vehicles;
    }

    void CVehicleManager::setVehicles(SharedPtr<Array<SmartPtr<IVehicle>>> ptr)
    {
        m_vehicles = ptr;
    }
} // namespace workphone
