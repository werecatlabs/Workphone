#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Vehicle/VehicleManager.hpp>
#include <Workphone/Vehicle/Vehicle.hpp>
#include <Workphone/Interface/Vehicle/IVehicle.hpp>
#include <Workphone/Interface/Vehicle/IVehicleBody.hpp>
#include <Workphone/Interface/Vehicle/IVehicleCallback.hpp>
#include <Workphone/Interface/Vehicle/IWheelComponent.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/System/ITask.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>

namespace workphone
{
    namespace vehicle
    {
        WP_CLASS_REGISTER_DERIVED( workphone::vehicle, VehicleManager, IVehicleManager );

        VehicleManager::VehicleManager() = default;

        VehicleManager::~VehicleManager() = default;

        void VehicleManager::load( SmartPtr<ISharedObject> data )
        {
            setLoadingState( LoadingState::Loading );
            m_vehicles.reserve( 1024 );
            setLoadingState( LoadingState::Loaded );
        }

        void VehicleManager::reload( SmartPtr<ISharedObject> data )
        {
            unload( data );
            load( data );
        }

        void VehicleManager::unload( SmartPtr<ISharedObject> data )
        {
            setLoadingState( LoadingState::Unloading );
            m_vehicles.clear();
            setLoadingState( LoadingState::Unloaded );
        }

        void VehicleManager::preUpdate()
        {
            auto vehicles = m_vehicles.snapshot();
            for( auto &vehicle : vehicles )
            {
                vehicle->preUpdate();
            }
        }

        void VehicleManager::update()
        {
            auto vehicles = m_vehicles.snapshot();
            for( auto &vehicle : vehicles )
            {
                vehicle->update();
            }
        }

        void VehicleManager::postUpdate()
        {
            auto vehicles = m_vehicles.snapshot();
            for( auto &vehicle : vehicles )
            {
                vehicle->postUpdate();
            }
        }

        SmartPtr<IVehicle> VehicleManager::createVehicle( hash64 type )
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto factoryManager = applicationManager->getFactoryManagerPtr();

            auto vehicleController = factoryManager->make_ptr<Vehicle>();

            m_vehicles.emplace_back( vehicleController );

            std::sort( m_vehicles.begin(), m_vehicles.end(),
                       []( SmartPtr<IVehicle> &a, SmartPtr<IVehicle> &b ) {
                           return a < b;  // compare by memory address
                       } );

            return vehicleController;
        }

        void VehicleManager::destroyVehicle( SmartPtr<IVehicle> vehicle )
        {
            m_vehicles.erase( std::remove( m_vehicles.begin(), m_vehicles.end(), vehicle ) );
        }

        void VehicleManager::addVehicle( SmartPtr<IVehicle> vehicle )
        {
            m_vehicles.push_back( vehicle );
        }

        void VehicleManager::removeVehicle( SmartPtr<IVehicle> vehicle )
        {
            m_vehicles.erase( std::remove( m_vehicles.begin(), m_vehicles.end(), vehicle ) );
        }

        void VehicleManager::lock()
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            if( auto taskManager = applicationManager->getTaskManager() )
            {
                if( auto task = taskManager->getTask( TaskId::Physics ) )
                {
                    task->lock();
                }
            }
        }

        bool VehicleManager::try_lock()
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            if( auto taskManager = applicationManager->getTaskManagerPtr() )
            {
                if( auto task = taskManager->getTask( TaskId::Physics ) )
                {
                    return task->try_lock();
                }
            }

            return false;
        }

        void VehicleManager::unlock()
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            if( auto taskManager = applicationManager->getTaskManager() )
            {
                if( auto task = taskManager->getTask( TaskId::Physics ) )
                {
                    task->unlock();
                }
            }
        }

    }  // namespace vehicle
}  // namespace workphone
