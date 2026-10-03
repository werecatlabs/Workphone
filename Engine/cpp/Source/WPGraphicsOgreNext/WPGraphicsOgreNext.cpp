#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/WPGraphicsOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CCameraOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CDebugLine.hpp>
#include <WPGraphicsOgreNext/Wrapper/CDebugTextOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CDynamicMesh.hpp>
#include <WPGraphicsOgreNext/Wrapper/CFontManagerOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CFontOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsMeshOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsSceneOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CGraphicsSystemOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CLightOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CMaterialManagerOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CMaterialOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CMaterialPassOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CMaterialTechniqueOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CMaterialTextureOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/COverlayElementContainer.hpp>
#include <WPGraphicsOgreNext/Wrapper/COverlayElementText.hpp>
#include <WPGraphicsOgreNext/Wrapper/COverlayManagerOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/COverlayOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CParticle.hpp>
#include <WPGraphicsOgreNext/Wrapper/CParticleAffector.hpp>
#include <WPGraphicsOgreNext/Wrapper/CParticleEmitterOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CParticleTechnique.hpp>
#include <WPGraphicsOgreNext/Wrapper/CParticleSystemOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CRenderer.hpp>
#include <WPGraphicsOgreNext/Wrapper/CRenderTextureOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CResourceGroupManager.hpp>
#include <WPGraphicsOgreNext/Wrapper/CSceneNodeOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CSkyboxCubeOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CSkybox6SidedOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CTerrainOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CTextureManagerOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CTextureOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CViewportOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/CWindowOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/GraphicsObjectListenerOgreNext.hpp>
#include <WPGraphicsOgreNext/Compositor.hpp>
#include <WPGraphicsOgreNext/UI/Colibri/UIImageColibri.hpp>
#include <WPGraphicsOgreNext/UI/Colibri/UIButtonColibri.hpp>
#include <WPGraphicsOgreNext/UI/Colibri/UIManagerColibri.hpp>
#include <WPGraphicsOgreNext/UI/Core/UIManagerCore.hpp>
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

namespace workphone::render
{

    SmartPtr<WPGraphicsOgreNext> WPGraphicsOgreNext::m_sPlugin;
    SmartPtr<IFactoryManager> WPGraphicsOgreNext::m_factoryManager;

    WPGraphicsOgreNext::WPGraphicsOgreNext() = default;
    WPGraphicsOgreNext::~WPGraphicsOgreNext() = default;

    void WPGraphicsOgreNext::load( SmartPtr<ISharedObject> data )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto applicationFactory = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( applicationFactory );

        auto factoryManager = workphone::make_ptr<FactoryManager>();
        factoryManager->load( nullptr );
        setFactoryManager( factoryManager );

        FactoryUtil::addFactory<CGraphicsSystemOgreNext>( applicationFactory );

        //FactoryUtil::addFactory<GraphicsSystemState>( factoryManager );
        FactoryUtil::addFactory<Compositor>( factoryManager );

        FactoryUtil::addFactory<CCameraOgreNext>( factoryManager );
        FactoryUtil::addFactory<CDebugLineOgreNext>( factoryManager );
        FactoryUtil::addFactory<CDebugTextOgreNext>( factoryManager );
        FactoryUtil::addFactory<CDynamicMesh>( factoryManager );
        FactoryUtil::addFactory<CFontOgreNext>( factoryManager );
        FactoryUtil::addFactory<CFontManagerOgreNext>( factoryManager );
        FactoryUtil::addFactory<CGraphicsMeshOgreNext>( factoryManager );
        FactoryUtil::addFactory<CGraphicsSceneOgreNext>( factoryManager );
        FactoryUtil::addFactory<CGraphicsMeshOgreNext::MeshMaterialEventListener>( factoryManager );
        FactoryUtil::addFactory<CGraphicsSystemOgreNext::GraphicsSystemEventListener>( factoryManager );
        FactoryUtil::addFactory<CLightOgreNext>( factoryManager );
        FactoryUtil::addFactory<CMaterialManagerOgreNext>( factoryManager );
        FactoryUtil::addFactory<CMaterialOgreNext>( factoryManager );
        FactoryUtil::addFactory<CMaterialPassOgreNext>( factoryManager );
        FactoryUtil::addFactory<CMaterialTechniqueOgreNext>( factoryManager );
        FactoryUtil::addFactory<CMaterialTextureOgreNext>( factoryManager );
        FactoryUtil::addFactory<CMaterialTextureOgreNext::TextureListener>( factoryManager );
        FactoryUtil::addFactory<COverlayElementContainer>( factoryManager );
        FactoryUtil::addFactory<COverlayElementText>( factoryManager );
        FactoryUtil::addFactory<COverlayManagerOgreNext>( factoryManager );
        FactoryUtil::addFactory<COverlayOgreNext>( factoryManager );
        FactoryUtil::addFactory<COverlayOgreNext::OverlayStateListener>( factoryManager );
        FactoryUtil::addFactory<CParticle>( factoryManager );
        FactoryUtil::addFactory<CParticleAffector>( factoryManager );
        FactoryUtil::addFactory<CTextureOgreNext>( factoryManager );
        FactoryUtil::addFactory<CParticleEmitterOgreNext>( factoryManager );
        FactoryUtil::addFactory<CParticleTechnique>( factoryManager );
        FactoryUtil::addFactory<CParticleSystemOgreNext>( factoryManager );
        FactoryUtil::addFactory<CRenderer>( factoryManager );
        FactoryUtil::addFactory<CRenderTextureOgreNext>( factoryManager );
        FactoryUtil::addFactory<CResourceGroupManager>( factoryManager );
        FactoryUtil::addFactory<CResourceGroupManager::ResourceLoadJob>( factoryManager );
        FactoryUtil::addFactory<CSceneNodeOgreNext>( factoryManager );
        FactoryUtil::addFactory<CSkyboxCubeOgreNext>( factoryManager );
        FactoryUtil::addFactory<CSkybox6SidedOgreNext>( factoryManager );
        FactoryUtil::addFactory<CTerrainOgreNext>( factoryManager );
        FactoryUtil::addFactory<CTerrainOgreNext::StateListener>( factoryManager );
        FactoryUtil::addFactory<CTextureManagerOgreNext>( factoryManager );
        FactoryUtil::addFactory<CTextureManagerOgreNext::TextureListener>( factoryManager );
        FactoryUtil::addFactory<CViewportOgreNext>( factoryManager );
        FactoryUtil::addFactory<CWindowOgreNext>( factoryManager );
        FactoryUtil::addFactory<CWindowOgreNext::WindowTexture>( factoryManager );
        FactoryUtil::addFactory<GraphicsObjectListenerOgreNext>( factoryManager );

        FactoryUtil::addFactory<ui::UIManagerColibri>( factoryManager );
        FactoryUtil::addFactory<ui::UIManagerCore>( factoryManager );

        applicationFactory->setPoolSizeByType<CGraphicsSystemOgreNext>( 1 );

        factoryManager->setPoolSizeByType<Compositor>( 4 );

        const auto numCameras = 4;
        const auto numLights = 4;
        const auto numMeshes = 32;
        const auto numMaterials = 32;
        const auto numTextures = 32;
        const auto numObjects = 32;
        const auto numOverlays = 32;
        const auto numParticles = 32;
        const auto numManagers = 1;

        factoryManager->setPoolSizeByType<CCameraOgreNext>( numCameras );
        factoryManager->setPoolSizeByType<CDebugLineOgreNext>( numObjects );
        factoryManager->setPoolSizeByType<CDebugTextOgreNext>( numObjects );
        factoryManager->setPoolSizeByType<CDynamicMesh>( numMeshes );
        factoryManager->setPoolSizeByType<CFontOgreNext>( numObjects );
        factoryManager->setPoolSizeByType<CFontManagerOgreNext>( numManagers );
        factoryManager->setPoolSizeByType<CGraphicsMeshOgreNext>( numMeshes );
        factoryManager->setPoolSizeByType<CGraphicsSceneOgreNext>( 1 );
        factoryManager->setPoolSizeByType<CGraphicsMeshOgreNext::MeshMaterialEventListener>( numMeshes );
        factoryManager->setPoolSizeByType<CGraphicsSystemOgreNext::GraphicsSystemEventListener>(
            numManagers );
        factoryManager->setPoolSizeByType<CLightOgreNext>( numLights );
        factoryManager->setPoolSizeByType<CMaterialManagerOgreNext>( numManagers );
        factoryManager->setPoolSizeByType<CMaterialOgreNext>( numMaterials );
        factoryManager->setPoolSizeByType<CMaterialPassOgreNext>( numMaterials );
        factoryManager->setPoolSizeByType<CMaterialTechniqueOgreNext>( numMaterials );
        factoryManager->setPoolSizeByType<CMaterialTextureOgreNext>( numMaterials );
        factoryManager->setPoolSizeByType<CMaterialTextureOgreNext::TextureListener>( numMaterials );
        factoryManager->setPoolSizeByType<COverlayElementContainer>( numOverlays );
        factoryManager->setPoolSizeByType<COverlayElementText>( numOverlays );
        factoryManager->setPoolSizeByType<COverlayManagerOgreNext>( numManagers );
        factoryManager->setPoolSizeByType<COverlayOgreNext>( numOverlays );
        factoryManager->setPoolSizeByType<COverlayOgreNext::OverlayStateListener>( numOverlays );
        factoryManager->setPoolSizeByType<CParticle>( numParticles );
        factoryManager->setPoolSizeByType<CParticleAffector>( numParticles );
        factoryManager->setPoolSizeByType<CParticleEmitterOgreNext>( numParticles );
        factoryManager->setPoolSizeByType<CParticleTechnique>( numParticles );
        factoryManager->setPoolSizeByType<CParticleSystemOgreNext>( numParticles );
        factoryManager->setPoolSizeByType<CRenderer>( numManagers );
        factoryManager->setPoolSizeByType<CRenderTextureOgreNext>( numTextures );
        factoryManager->setPoolSizeByType<CResourceGroupManager>( numManagers );
        factoryManager->setPoolSizeByType<CResourceGroupManager::ResourceLoadJob>( numObjects );
        factoryManager->setPoolSizeByType<CTextureOgreNext>( numTextures );
        factoryManager->setPoolSizeByType<CSceneNodeOgreNext>( numObjects );
        factoryManager->setPoolSizeByType<CSkyboxCubeOgreNext>( numObjects );
        factoryManager->setPoolSizeByType<CSkybox6SidedOgreNext>( numObjects );
        factoryManager->setPoolSizeByType<CTerrainOgreNext>( numObjects );
        factoryManager->setPoolSizeByType<CTerrainOgreNext::StateListener>( numObjects );
        factoryManager->setPoolSizeByType<CTextureManagerOgreNext>( numManagers );
        factoryManager->setPoolSizeByType<CTextureManagerOgreNext::TextureListener>( numTextures );
        factoryManager->setPoolSizeByType<CViewportOgreNext>( numCameras );
        factoryManager->setPoolSizeByType<CWindowOgreNext>( 1 );
        factoryManager->setPoolSizeByType<CWindowOgreNext::WindowTexture>( numTextures );
        factoryManager->setPoolSizeByType<GraphicsObjectListenerOgreNext>( numObjects );
    }

    void WPGraphicsOgreNext::unload( SmartPtr<ISharedObject> data )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        if( auto applicationFactory = applicationManager->getFactoryManager() )
        {
            FactoryUtil::removeFactory<CGraphicsSystemOgreNext>( applicationFactory );
        }

        if( auto factoryManager = getFactoryManager() )
        {
            factoryManager->unload( nullptr );
            setFactoryManager( nullptr );
        }
    }

    auto WPGraphicsOgreNext::instance() -> SmartPtr<WPGraphicsOgreNext>
    {
        return m_sPlugin;
    }

    void WPGraphicsOgreNext::setInstance( SmartPtr<WPGraphicsOgreNext> plugin )
    {
        m_sPlugin = plugin;
    }

    SmartPtr<IFactoryManager> WPGraphicsOgreNext::getFactoryManager()
    {
        return m_factoryManager;
    }

    void WPGraphicsOgreNext::setFactoryManager( SmartPtr<IFactoryManager> factoryManager )
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
loadPlugin( workphone::core::IApplicationManager *applicationManager )
{
    using namespace workphone;
    using namespace workphone::render;

    auto plugin = workphone::make_ptr<WPGraphicsOgreNext>();
    plugin->load( nullptr );
    WPGraphicsOgreNext::setInstance( plugin );
}

WP_INTERFACE_EXPORT void WP_INTERFACE_API
unloadPlugin( workphone::core::IApplicationManager *applicationManager )
{
    using namespace workphone;
    using namespace workphone::render;

    if( auto plugin = WPGraphicsOgreNext::instance() )
    {
        plugin->unload( nullptr );
        WPGraphicsOgreNext::setInstance( nullptr );
    }
}
}
#endif
