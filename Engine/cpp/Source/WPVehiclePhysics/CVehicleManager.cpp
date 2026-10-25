#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include <WPVehiclePhysics/CVehicleManager.hpp>
#include <Workphone/Workphone.hpp>
#include <algorithm>
#include <limits>
#include <mutex>

namespace workphone
{
    namespace
    {
        using VehicleList = Array<SmartPtr<IVehicle>>;

        // SharedPtr's typed control block retains this actual derived type when
        // exposed as SharedPtr<Array<...>>, whose base destructor is nonvirtual.
        // Both this body and that control block use the Source object policy.
        struct VehicleListOwner final : VehicleList, SourceBudgetObject
        {
            using SourceBudgetObject::operator new;
            using SourceBudgetObject::operator delete;
            VehicleListOwner() : VehicleList( Allocator<SmartPtr<IVehicle>>( sourceBudget ) ) {}
            explicit VehicleListOwner( const VehicleList &input, size_t skipped ) :
                VehicleList( Allocator<SmartPtr<IVehicle>>( sourceBudget ) )
            {
                reserve( input.capacity() );
                for( size_t i = 0; i < input.size(); ++i )
                    if( i != skipped ) push_back( input[i] );
            }
        };

        SharedPtr<VehicleList> copyVehicleList( const VehicleList *input = nullptr,
                                              size_t skipped = std::numeric_limits<size_t>::max() )
        {
            SharedPtr<VehicleListOwner> owner( input ? new VehicleListOwner( *input, skipped ) :
                                                     new VehicleListOwner );
            return SharedPtr<VehicleList>( std::move( owner ) );
        }
    }

    CVehicleManager::CVehicleManager()
    {
        m_vehicles = copyVehicleList();
    }

    CVehicleManager::~CVehicleManager()
    {
        setVehicles( nullptr );
    }

    void CVehicleManager::unload( SmartPtr<ISharedObject> data )
    {
        // Detach before optional base notifications, retaining any caller snapshots.
        setVehicles( nullptr );
        IVehicleManager::unload( data );
    }

    void CVehicleManager::preUpdate()
    {
        auto pVehicles = getVehicles();
        if( !pVehicles ) return;
        auto &vehicles = *pVehicles;
        for( auto &vehicle : vehicles )
        {
            vehicle->preUpdate();
        }
    }

    void CVehicleManager::update()
    {
        auto pVehicles = getVehicles();
        if( !pVehicles ) return;
        auto &vehicles = *pVehicles;
        for( auto &vehicle : vehicles )
        {
            vehicle->update();
        }
    }

    void CVehicleManager::postUpdate()
    {
        auto pVehicles = getVehicles();
        if( !pVehicles ) return;
        auto &vehicles = *pVehicles;
        for( auto &vehicle : vehicles )
        {
            vehicle->postUpdate();
        }
    }

    void CVehicleManager::addVehicle( SmartPtr<IVehicle> vehicle )
    {
        SharedPtr<VehicleList> retired;
        {
            std::lock_guard<std::mutex> mutation( m_vehicleMutationMutex );
            auto previous = getVehicles();
            auto candidate = copyVehicleList( previous.get() );
            candidate->push_back( vehicle );
            retired = m_vehicles.exchange( candidate );
        }
        // Last-owner callbacks must run after releasing the mutation lock.
    }

    void CVehicleManager::removeVehicle( SmartPtr<IVehicle> vehicle )
    {
        SharedPtr<VehicleList> retired;
        {
            std::lock_guard<std::mutex> mutation( m_vehicleMutationMutex );
            auto previous = getVehicles();
            if( !previous ) return;
            auto found = std::find( previous->begin(), previous->end(), vehicle );
            if( found == previous->end() ) return;
            const auto index = size_t( found - previous->begin() );
            // Copy directly around the removed slot; Array::erase would stage
            // another complete buffer even though this candidate is unpublished.
            auto candidate = copyVehicleList( previous.get(), index );
            retired = m_vehicles.exchange( candidate );
        }
    }

    SmartPtr<IVehicle> CVehicleManager::createVehicle( hash64 type )
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

    void CVehicleManager::destroyVehicle( SmartPtr<IVehicle> vehicle )
    {
    }

    SharedPtr<Array<SmartPtr<IVehicle>>> CVehicleManager::getVehicles() const
    {
        return m_vehicles;
    }

    void CVehicleManager::setVehicles( SharedPtr<Array<SmartPtr<IVehicle>>> ptr )
    {
        SharedPtr<VehicleList> retired;
        {
            std::lock_guard<std::mutex> mutation( m_vehicleMutationMutex );
            auto current = getVehicles();
            if( ptr == current && ( !ptr || ptr->get_allocator().isSourceBudgeted() ) ) return;
            auto candidate = ptr ? copyVehicleList( ptr.get() ) : SharedPtr<VehicleList>();
            retired = m_vehicles.exchange( candidate );
        }
    }
}  // namespace workphone
