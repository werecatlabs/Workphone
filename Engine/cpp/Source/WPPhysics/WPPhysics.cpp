#include "WPPhysics/WPPhysicsPCH.hpp"
#include "WPPhysics/WPPhysics.hpp"
#include "WPPhysicsAutoLink.hpp"
#include "WPPhysics/CPhysicsManager2D.hpp"
#include "WPPhysics/CPhysicsManager2.hpp"
#include "WPPhysics/CPhysicsScene2.hpp"
#include "WPPhysics/CPhysicsRigidBody2.hpp"
#include "WPPhysics/CPhysicsShape2.hpp"
#include "WPPhysics/CPhysicsBoxShape2.hpp"
#include "WPPhysics/CPhysicsSphereShape2.hpp"
#include "WPPhysics/CPhysicsManager3.hpp"
#include "WPPhysics/CPhysicsScene3.hpp"
#include "WPPhysics/CPhysicsMaterial3.hpp"
#include "WPPhysics/CBoxShape3.hpp"
#include "WPPhysics/CMeshShape3.hpp"
#include "WPPhysics/CPlaneShape3.hpp"
#include "WPPhysics/CSphereShape3.hpp"
#include "WPPhysics/CTerrainShape3.hpp"
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
            FactoryUtil::addFactory<CPhysicsManager2>();
            FactoryUtil::addFactory<CPhysicsScene2>();
            FactoryUtil::addFactory<CPhysicsRigidBody2>();
            FactoryUtil::addFactory<CPhysicsBoxShape2>();
            FactoryUtil::addFactory<CPhysicsSphereShape2>();

            FactoryUtil::addFactory<CPhysicsManager3>();
            FactoryUtil::addFactory<CPhysicsScene3>();
            FactoryUtil::addFactory<CPhysicsMaterial3>();
            FactoryUtil::addFactory<CBoxShape3>();
            FactoryUtil::addFactory<CSphereShape3>();
            FactoryUtil::addFactory<CPlaneShape3>();
            FactoryUtil::addFactory<CMeshShape3>();
            FactoryUtil::addFactory<CTerrainShape3>();
        }

        void WPPhysics::unload( SmartPtr<ISharedObject> data )
        {
            FactoryUtil::removeFactory<CTerrainShape3>();
            FactoryUtil::removeFactory<CMeshShape3>();
            FactoryUtil::removeFactory<CPlaneShape3>();
            FactoryUtil::removeFactory<CSphereShape3>();
            FactoryUtil::removeFactory<CBoxShape3>();
            FactoryUtil::removeFactory<CPhysicsMaterial3>();
            FactoryUtil::removeFactory<CPhysicsScene3>();
            FactoryUtil::removeFactory<CPhysicsManager3>();

            FactoryUtil::removeFactory<CPhysicsSphereShape2>();
            FactoryUtil::removeFactory<CPhysicsBoxShape2>();
            FactoryUtil::removeFactory<CPhysicsRigidBody2>();
            FactoryUtil::removeFactory<CPhysicsScene2>();
            FactoryUtil::removeFactory<CPhysicsManager2>();
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
