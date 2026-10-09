#ifndef WPVehiclePhysics_h__
#define WPVehiclePhysics_h__

#include "WPVehiclePhysics/WPVehiclePhysicsPrerequisites.hpp"
#include "WPVehiclePhysics/WPVehiclePhysicsHeaders.hpp"

namespace workphone
{
    class WPVehiclePhysics_API WPVehiclePhysics : public ISharedObject
    {
    public:
        WPVehiclePhysics();
        ~WPVehiclePhysics() override;

        void load(SmartPtr<ISharedObject> data) override;
        void unload(SmartPtr<ISharedObject> data) override;

        static SmartPtr<WPVehiclePhysics> instance();
        static void setInstance(SmartPtr<WPVehiclePhysics> plugin);

    protected:
        static SmartPtr<WPVehiclePhysics> m_sPlugin;
    };
} // namespace workphone

#endif // WPVehiclePhysics_h__
