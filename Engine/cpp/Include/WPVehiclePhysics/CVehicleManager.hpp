#ifndef CVehicleManager_h__
#define CVehicleManager_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Vehicle/IVehicleManager.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Memory/AtomicSharedPtr.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>
#include <mutex>

namespace workphone
{
    class WPVehiclePhysics_API CVehicleManager : public IVehicleManager, public SourceBudgetObject
    {
    public:
        using SourceBudgetObject::operator new;
        using SourceBudgetObject::operator delete;
        CVehicleManager();
        ~CVehicleManager() override;

        void unload( SmartPtr<ISharedObject> data ) override;

        void preUpdate() override;
        void update() override;
        void postUpdate() override;

        void addVehicle( SmartPtr<IVehicle> vehicle ) override;
        void removeVehicle( SmartPtr<IVehicle> vehicle ) override;

        SmartPtr<IVehicle> createVehicle( hash64 type ) override;
        void destroyVehicle( SmartPtr<IVehicle> vehicle ) override;

    protected:
        SharedPtr<Array<SmartPtr<IVehicle>>> getVehicles() const;
        void setVehicles( SharedPtr<Array<SmartPtr<IVehicle>>> ptr );

        AtomicSharedPtr<Array<SmartPtr<IVehicle>>> m_vehicles;

    private:
        // Snapshot readers do not hold this while invoking vehicle callbacks.
        // Serialize the complete read/copy/publish mutation to avoid lost updates.
        std::mutex m_vehicleMutationMutex;
    };
}  // namespace workphone

#endif  // CVehicleManager_h__
