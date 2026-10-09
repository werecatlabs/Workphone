#ifndef CVehicleManager_h__
#define CVehicleManager_h__

#include <WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/Vehicle/IVehicleManager.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Memory/AtomicSharedPtr.hpp>

namespace workphone
{
    class WPVehiclePhysics_API CVehicleManager : public IVehicleManager
    {
    public:
        CVehicleManager();
        ~CVehicleManager() override;

        void preUpdate() override;
        void update() override;
        void postUpdate() override;

        void addVehicle(SmartPtr<IVehicle> vehicle) override;
        void removeVehicle(SmartPtr<IVehicle> vehicle) override;

        SmartPtr<IVehicle> createVehicle(hash64 type) override;
        void destroyVehicle(SmartPtr<IVehicle> vehicle) override;

    protected:
        SharedPtr<Array<SmartPtr<IVehicle>>> getVehicles() const;
        void setVehicles(SharedPtr<Array<SmartPtr<IVehicle>>> ptr);

        AtomicSharedPtr<Array<SmartPtr<IVehicle>>> m_vehicles;
    };
} // namespace workphone

#endif // CVehicleManager_h__
