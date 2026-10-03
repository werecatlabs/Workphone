#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/WPGraphicsOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CGraphicsSceneOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CGraphicsSystemOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CMaterialOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CMaterialPassOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CMaterialTechniqueOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CSceneNodeOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CTextureOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CTextureOgreStateListener.hpp>
#include <WPGraphicsOgre/Wrapper/CCubemap.hpp>
#include <WPGraphicsOgre/Wrapper/CDynamicMesh.hpp>
#include <WPGraphicsOgre/Wrapper/CFont.hpp>
#include <WPGraphicsOgre/Wrapper/CGraphicsMeshOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CLightOgre.hpp>
#include <WPGraphicsOgre/Wrapper/COverlayElementContainer.hpp>
#include <WPGraphicsOgre/Wrapper/COverlayElementText.hpp>
#include <WPGraphicsOgre/Wrapper/COverlayOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CWindowOgre.hpp>
#include <WPGraphicsOgre/Wrapper/CTerrainOgre.hpp>
#include <Workphone/Workphone.hpp>

#if defined WP_PLATFORM_WIN32
#    include <windows.h>
#endif

#if defined WP_PLATFORM_WIN32

int WINAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID )
{
    return 1;
}

#endif

namespace workphone::render
{

    SmartPtr<WPGraphicsOgre> WPGraphicsOgre::m_sPlugin;
    SmartPtr<IFactoryManager> WPGraphicsOgre::m_factoryManager;

    WPGraphicsOgre::WPGraphicsOgre() = default;
    WPGraphicsOgre::~WPGraphicsOgre() = default;

    void WPGraphicsOgre::load( SmartPtr<ISharedObject> data )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto appFactoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( appFactoryManager );

        auto factoryManager = workphone::make_ptr<FactoryManager>();
        factoryManager->load( nullptr );
        setFactoryManager( factoryManager );

        FactoryUtil::addFactory<CGraphicsSystemOgre>();

        FactoryUtil::addFactory<CCubemap>( factoryManager );
        FactoryUtil::addFactory<CDynamicMesh>( factoryManager );
        FactoryUtil::addFactory<CFont>( factoryManager );

        FactoryUtil::addFactory<CGraphicsSceneOgre>( factoryManager );
        FactoryUtil::addFactory<CGraphicsMeshOgre>( factoryManager );
        FactoryUtil::addFactory<CLightOgre>( factoryManager );

        FactoryUtil::addFactory<COverlayOgre>( factoryManager );
        FactoryUtil::addFactory<COverlayElementContainer>( factoryManager );
        FactoryUtil::addFactory<COverlayElementText>( factoryManager );

        FactoryUtil::addFactory<CMaterialOgre>( factoryManager );
        FactoryUtil::addFactory<CMaterialOgre::MaterialOgreListener>( factoryManager );

        FactoryUtil::addFactory<CMaterialPassOgre>( factoryManager );
        FactoryUtil::addFactory<CMaterialTechniqueOgre>( factoryManager );

        FactoryUtil::addFactory<CTextureOgre>( factoryManager );
        FactoryUtil::addFactory<CTextureOgreStateListener>( factoryManager );

        FactoryUtil::addFactory<CSceneNodeOgre>( factoryManager );
        FactoryUtil::addFactory<CTerrainOgre>( factoryManager );
        FactoryUtil::addFactory<CWindowOgre>( factoryManager );

        appFactoryManager->setPoolSizeByType<CGraphicsMeshOgre>( 32 );
        appFactoryManager->setPoolSizeByType<CGraphicsSceneOgre>( 1 );
        appFactoryManager->setPoolSizeByType<CLightOgre>( 32 );
        appFactoryManager->setPoolSizeByType<COverlayOgre>( 32 );
        appFactoryManager->setPoolSizeByType<COverlayElementContainer>( 32 );
        appFactoryManager->setPoolSizeByType<COverlayElementText>( 32 );
        appFactoryManager->setPoolSizeByType<CMaterialOgre>( 32 );
        appFactoryManager->setPoolSizeByType<CMaterialPassOgre>( 32 );
        appFactoryManager->setPoolSizeByType<CMaterialTechniqueOgre>( 32 );
        appFactoryManager->setPoolSizeByType<CSceneNodeOgre>( 32 );
        appFactoryManager->setPoolSizeByType<CTextureOgre>( 32 );
        appFactoryManager->setPoolSizeByType<CTextureOgreStateListener>( 32 );
    }

    void WPGraphicsOgre::unload( SmartPtr<ISharedObject> data )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        FactoryUtil::removeFactory<CGraphicsSystemOgre>();

        if( auto pluginFactoryManager = getFactoryManager() )
        {
            pluginFactoryManager->unload( nullptr );
            setFactoryManager( nullptr );
        }
    }

    SmartPtr<IGraphicsSystem> WPGraphicsOgre::createGraphicsOgre()
    {
        return workphone::make_ptr<CGraphicsSystemOgre>();
    }

    SmartPtr<WPGraphicsOgre> WPGraphicsOgre::instance()
    {
        return m_sPlugin;
    }

    void WPGraphicsOgre::setInstance( SmartPtr<WPGraphicsOgre> plugin )
    {
        m_sPlugin = plugin;
    }

    SmartPtr<IFactoryManager> WPGraphicsOgre::getFactoryManager()
    {
        return m_factoryManager;
    }

    void WPGraphicsOgre::setFactoryManager( SmartPtr<IFactoryManager> factoryManager )
    {
        m_factoryManager = factoryManager;
    }

}  // namespace workphone::render

#ifndef _WP_STATIC_LIB_
extern "C" {

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
    using namespace workphone::render;

    core::IApplicationManager::setInstance( applicationManager );

    auto plugin = workphone::make_ptr<WPGraphicsOgre>();
    plugin->load( nullptr );
    WPGraphicsOgre::setInstance( plugin );
}

WP_INTERFACE_EXPORT void WP_INTERFACE_API
unloadPlugin( workphone::core::ApplicationManager *applicationManager )
{
    using namespace workphone;
    using namespace workphone::render;

    if( auto plugin = WPGraphicsOgre::instance() )
    {
        plugin->unload( nullptr );
        WPGraphicsOgre::setInstance( nullptr );
    }

    core::IApplicationManager::setInstance( nullptr );
}
}
#endif
