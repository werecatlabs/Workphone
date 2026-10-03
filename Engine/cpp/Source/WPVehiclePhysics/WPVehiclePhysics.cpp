#include <WPVehiclePhysics/WPVehiclePhysicsPCH.hpp>
#include <WPVehiclePhysics/WPVehiclePhysics.hpp>
#include <WPVehiclePhysics/CCarController.hpp>
#include <WPVehiclePhysics/CAircraft.hpp>
#include <WPVehiclePhysics/CDriveTrain.hpp>
#include <WPVehiclePhysics/WheelControllerArcade.hpp>
#include <WPVehiclePhysics/WheelControllerBrush.hpp>
#include <WPVehiclePhysics/WheelControllerPacejka.hpp>
#include <Workphone/Workphone.hpp>

#if defined WP_PLATFORM_WIN32
#    include <windows.h>
#endif

#if defined WP_PLATFORM_WIN32
#    ifndef _WP_STATIC_LIB_
int WINAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID )
{
    return 1;
}
#    endif
#endif

namespace workphone
{
    SmartPtr<WPVehiclePhysics> WPVehiclePhysics::m_sPlugin;

    WPVehiclePhysics::WPVehiclePhysics() = default;
    WPVehiclePhysics::~WPVehiclePhysics() = default;

    void WPVehiclePhysics::load( SmartPtr<ISharedObject> data )
    {
        FactoryUtil::addFactory<CCarController>();
        FactoryUtil::addFactory<vehicle::CAircraft>();
        FactoryUtil::addFactory<CDriveTrain>();
        FactoryUtil::addFactory<WheelControllerArcade>();
        FactoryUtil::addFactory<WheelControllerBrush>();
        FactoryUtil::addFactory<WheelControllerPacejka>();
    }

    void WPVehiclePhysics::unload( SmartPtr<ISharedObject> data )
    {
        FactoryUtil::removeFactory<CCarController>();
        FactoryUtil::removeFactory<vehicle::CAircraft>();
        FactoryUtil::removeFactory<CDriveTrain>();
        FactoryUtil::removeFactory<WheelControllerArcade>();
        FactoryUtil::removeFactory<WheelControllerBrush>();
        FactoryUtil::removeFactory<WheelControllerPacejka>();
    }

    SmartPtr<WPVehiclePhysics> WPVehiclePhysics::instance()
    {
        return m_sPlugin;
    }

    void WPVehiclePhysics::setInstance( SmartPtr<WPVehiclePhysics> plugin )
    {
        m_sPlugin = plugin;
    }
} // namespace workphone

#ifndef _WP_STATIC_LIB_
extern "C"
{
    WP_INTERFACE_EXPORT void WP_INTERFACE_API workphone_get_version( int *major, int *minor, int *patch )
    {
        *major = WP_VERSION_MAJOR;
        *minor = WP_VERSION_MINOR;
        *patch = WP_VERSION_PATCH;
    }

    WP_INTERFACE_EXPORT void WP_INTERFACE_API
    loadPlugin( workphone::core::IApplicationManager *applicationManager )
    {
        using namespace workphone;
        using namespace physics;

        core::IApplicationManager::setInstance( applicationManager );

        auto plugin = workphone::make_ptr<WPVehiclePhysics>();
        plugin->load( nullptr );
        WPVehiclePhysics::setInstance( plugin );
    }

    WP_INTERFACE_EXPORT void WP_INTERFACE_API
    unloadPlugin( workphone::core::IApplicationManager *applicationManager )
    {
        using namespace workphone;
        using namespace physics;

        if( auto plugin = WPVehiclePhysics::instance() )
        {
            plugin->unload( nullptr );
            WPVehiclePhysics::setInstance( nullptr );
        }
    }
}
#endif
