#include "WPPhysics/WPPhysicsPCH.hpp"
#include "WPPhysics/WPPhysics.hpp"
#include "WPPhysicsAutoLink.hpp"
#include "WPPhysics/WPPhysicsManager2D.hpp"
#include "WPPhysics/WPPhysicsManager2.hpp"
#include "WPPhysics/WPPhysicsScene2.hpp"
#include "WPPhysics/WPPhysicsNativeRigidBody2.hpp"
#include "WPPhysics/WPPhysicsShape2.hpp"
#include "WPPhysics/WPPhysicsNativeBoxShape2.hpp"
#include "WPPhysics/WPPhysicsNativeSphereShape2.hpp"
#include "WPPhysics/WPPhysicsManager3.hpp"
#include "WPPhysics/WPPhysicsScene3.hpp"
#include "WPPhysics/WPPhysicsMaterial3.hpp"
#include "WPPhysics/WPPhysicsBoxShape3.hpp"
#include "WPPhysics/WPPhysicsMeshShape3.hpp"
#include "WPPhysics/WPPhysicsPlaneShape3.hpp"
#include "WPPhysics/WPPhysicsSphereShape3.hpp"
#include "WPPhysics/WPPhysicsTerrainShape3.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace physics
    {
        SmartPtr<WPPhysics> WPPhysics::m_sPlugin;

        WPPhysics::WPPhysics() = default;

        WPPhysics::~WPPhysics() = default;

        void WPPhysics::load( SmartPtr<ISharedObject> data )
        {
            FactoryUtil::addFactory<WPPhysicsManager2>();
            FactoryUtil::addFactory<WPPhysicsScene2>();
            FactoryUtil::addFactory<WPPhysicsNativeRigidBody2>();
            FactoryUtil::addFactory<WPPhysicsNativeBoxShape2>();
            FactoryUtil::addFactory<WPPhysicsNativeSphereShape2>();

            FactoryUtil::addFactory<WPPhysicsManager3>();
            FactoryUtil::addFactory<WPPhysicsScene3>();
            FactoryUtil::addFactory<WPPhysicsMaterial3>();
            FactoryUtil::addFactory<WPPhysicsBoxShape3>();
            FactoryUtil::addFactory<WPPhysicsSphereShape3>();
            FactoryUtil::addFactory<WPPhysicsPlaneShape3>();
            FactoryUtil::addFactory<WPPhysicsMeshShape3>();
            FactoryUtil::addFactory<WPPhysicsTerrainShape3>();
        }

        void WPPhysics::unload( SmartPtr<ISharedObject> data )
        {
            FactoryUtil::removeFactory<WPPhysicsTerrainShape3>();
            FactoryUtil::removeFactory<WPPhysicsMeshShape3>();
            FactoryUtil::removeFactory<WPPhysicsPlaneShape3>();
            FactoryUtil::removeFactory<WPPhysicsSphereShape3>();
            FactoryUtil::removeFactory<WPPhysicsBoxShape3>();
            FactoryUtil::removeFactory<WPPhysicsMaterial3>();
            FactoryUtil::removeFactory<WPPhysicsScene3>();
            FactoryUtil::removeFactory<WPPhysicsManager3>();

            FactoryUtil::removeFactory<WPPhysicsNativeSphereShape2>();
            FactoryUtil::removeFactory<WPPhysicsNativeBoxShape2>();
            FactoryUtil::removeFactory<WPPhysicsNativeRigidBody2>();
            FactoryUtil::removeFactory<WPPhysicsScene2>();
            FactoryUtil::removeFactory<WPPhysicsManager2>();
        }

        SmartPtr<WPPhysics> WPPhysics::instance()
        {
            return m_sPlugin;
        }

        void WPPhysics::setInstance( SmartPtr<WPPhysics> plugin )
        {
            m_sPlugin = plugin;
        }

    } // namespace physics
} // namespace workphone

extern "C"
{
    WP_INTERFACE_EXPORT void WP_INTERFACE_API workphone_get_version( int *major, int *minor, int *patch )
    {
        *major = WP_VERSION_MAJOR;
        *minor = WP_VERSION_MINOR;
        *patch = WP_VERSION_PATCH;
    }

    WP_INTERFACE_EXPORT void WP_INTERFACE_API
    loadPlugin( workphone::core::ApplicationManager *applicationManager )
    {
        using namespace workphone;
        using namespace workphone::physics;

        auto plugin = workphone::make_ptr<WPPhysics>();
        plugin->load( nullptr );
        WPPhysics::setInstance( plugin );
    }

    WP_INTERFACE_EXPORT void WP_INTERFACE_API
    unloadPlugin( workphone::core::ApplicationManager *applicationManager )
    {
        using namespace workphone;
        using namespace workphone::physics;

        if( auto plugin = WPPhysics::instance() )
        {
            plugin->unload( nullptr );
            WPPhysics::setInstance( nullptr );
        }
    }
}
