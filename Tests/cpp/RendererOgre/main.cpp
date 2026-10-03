#include <Workphone/Workphone.hpp>
#include <iostream>

#ifdef _FB_STATIC_LIB_
#if FB_GRAPHICS_SYSTEM_OGRENEXT
#    include "FBGraphicsOgreNext/Terra/Terra.h"
#    include "FBGraphicsOgreNext/Terra/Hlms/OgreHlmsTerra.h"
#    include "FBGraphicsOgreNext/Terra/Hlms/PbsListener/OgreHlmsPbsTerraShadows.h"

#    define OGRE_DEPRECATED_2_2

#    include <Ogre.h>
#    include <OgreWindow.h>
//#include <OgreStaticPluginLoader.hpp>
#    include <OgreWindowEventUtilities.h>
#    include <OgreOverlaySystem.h>
#    include <OgreOverlayManager.h>
#    include <OgreOverlay.h>
#    include <OgreOverlayContainer.h>
#    include <OgreTextAreaOverlayElement.h>
#    include <OgreTextureGpuManager.h>
#    include <OgreHlmsUnlit.h>
#    include <OgreHlmsPbs.h>
#    include <OgreHlmsManager.h>
#    include <OgrePixelFormatGpuUtils.h>
#    include <OgreHlmsUnlitDatablock.h>
#    include "Compositor/OgreCompositorManager2.h"
#    include "Compositor/OgreCompositorNodeDef.h"
#    include "Compositor/OgreCompositorWorkspaceDef.h"
#    include "OgrePixelFormatGpuUtils.h"
#    include "OgreTextureGpuManager.h"

#    include "OgreMeshManager.h"
#    include "OgreMesh.h"
#    include "OgreMeshManager2.h"
#    include "OgreMesh2.h"

#    include "Compositor/Pass/PassIblSpecular/OgreCompositorPassIblSpecularDef.h"

#    include <Compositor/OgreCompositorManager2.h>
#    include <Compositor/OgreCompositorWorkspace.h>

#    ifdef OGRE_STATIC_LIB
#        ifdef OGRE_BUILD_RENDERSYSTEM_GL3PLUS
#            if FB_BUILD_RENDERER_GL3PLUS
#                include <OgreGL3PlusPlugin.h>
#            endif
#        endif
#        ifdef OGRE_BUILD_RENDERSYSTEM_GLES2
#            include "OgreGLES2Plugin.h"
#        endif
#        ifdef OGRE_BUILD_RENDERSYSTEM_D3D11
#            if FB_BUILD_RENDERER_DX11
#                include <OgreD3D11Plugin.h>
#                include <OgreD3D11RenderSystem.h>
#            endif
#        endif
#        ifdef OGRE_BUILD_RENDERSYSTEM_METAL
#            if FB_BUILD_RENDERER_METAL

#                include <OgreMetalPlugin.h>

#            endif
#        endif
#    endif

using namespace lioncat;
using namespace Ogre;

constexpr int testIndex = 7;

void importV1Mesh( const Ogre::String &meshName, const Ogre::String &groupName )
{
    Ogre::v1::MeshPtr v1Mesh;
    Ogre::MeshPtr v2Mesh;

    v1Mesh = Ogre::v1::MeshManager::getSingleton().load( meshName, groupName,
                                                         Ogre::v1::HardwareBuffer::HBU_STATIC,
                                                         Ogre::v1::HardwareBuffer::HBU_STATIC );

    //Create a v2 mesh to import to, with the same name (arbitrary).
    v2Mesh = Ogre::MeshManager::getSingleton().createByImportingV1( meshName, groupName, v1Mesh.get(),
                                                                    true, true, true );

    //Free memory
    v1Mesh->unload();

    // Do not destroy mesh, it could be used to recover from lost device.
    // Ogre::v1::MeshManager::getSingleton().remove( meshName );
}

void baseRegisterHlms()
{
    using namespace Ogre;

#    if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
    // Note:  macBundlePath works for iOS too. It's misnamed.
    const String resourcePath = Path::macBundlePath() + "/Contents/Resources/";
#    elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
    const String resourcePath = Ogre::macBundlePath() + "/";
#    else
    String resourcePath = "";
#    endif

    ConfigFile cf;
    cf.load( resourcePath + "resources2.cfg" );

#    if OGRE_PLATFORM == OGRE_PLATFORM_APPLE || OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
    String rootHlmsFolder =
        Path::macBundlePath() + '/' + cf.getSetting( "DoNotUseAsResource", "Hlms", "" );
#    else
    String rootHlmsFolder = resourcePath + cf.getSetting( "DoNotUseAsResource", "Hlms", "" );
#    endif

    if( rootHlmsFolder.empty() )
        rootHlmsFolder = "./";
    else if( *( rootHlmsFolder.end() - 1 ) != '/' )
        rootHlmsFolder += "/";

    // At this point rootHlmsFolder should be a valid path to the Hlms data folder

    HlmsUnlit *hlmsUnlit = 0;
    HlmsPbs *hlmsPbs = 0;

    // For retrieval of the paths to the different folders needed
    String mainFolderPath;
    StringVector libraryFoldersPaths;
    StringVector::const_iterator libraryFolderPathIt;
    StringVector::const_iterator libraryFolderPathEn;

    ArchiveManager &archiveManager = ArchiveManager::getSingleton();

    {
        // Create & Register HlmsUnlit
        // Get the path to all the subdirectories used by HlmsUnlit
        HlmsUnlit::getDefaultPaths( mainFolderPath, libraryFoldersPaths );
        Archive *archiveUnlit =
            archiveManager.load( rootHlmsFolder + mainFolderPath, "FileSystem", true );
        ArchiveVec archiveUnlitLibraryFolders;
        libraryFolderPathIt = libraryFoldersPaths.begin();
        libraryFolderPathEn = libraryFoldersPaths.end();
        while( libraryFolderPathIt != libraryFolderPathEn )
        {
            Archive *archiveLibrary =
                archiveManager.load( rootHlmsFolder + *libraryFolderPathIt, "FileSystem", true );
            archiveUnlitLibraryFolders.push_back( archiveLibrary );
            ++libraryFolderPathIt;
        }

        // Create and register the unlit Hlms
        hlmsUnlit = new HlmsUnlit( archiveUnlit, &archiveUnlitLibraryFolders );
        Root::getSingleton().getHlmsManager()->registerHlms( hlmsUnlit );
    }

    {
        // Create & Register HlmsPbs
        // Do the same for HlmsPbs:
        HlmsPbs::getDefaultPaths( mainFolderPath, libraryFoldersPaths );
        Archive *archivePbs = archiveManager.load( rootHlmsFolder + mainFolderPath, "FileSystem", true );

        // Get the library archive(s)
        ArchiveVec archivePbsLibraryFolders;
        libraryFolderPathIt = libraryFoldersPaths.begin();
        libraryFolderPathEn = libraryFoldersPaths.end();
        while( libraryFolderPathIt != libraryFolderPathEn )
        {
            Archive *archiveLibrary =
                archiveManager.load( rootHlmsFolder + *libraryFolderPathIt, "FileSystem", true );
            archivePbsLibraryFolders.push_back( archiveLibrary );
            ++libraryFolderPathIt;
        }

        // Create and register
        hlmsPbs = new HlmsPbs( archivePbs, &archivePbsLibraryFolders );
        Root::getSingleton().getHlmsManager()->registerHlms( hlmsPbs );
    }

    RenderSystem *renderSystem = Root::getSingletonPtr()->getRenderSystem();
    if( renderSystem->getName() == "Direct3D11 Rendering Subsystem" )
    {
        // Set lower limits 512kb instead of the default 4MB per Hlms in D3D 11.0
        // and below to avoid saturating AMD's discard limit (8MB) or
        // saturate the PCIE bus in some low end machines.
        bool supportsNoOverwriteOnTextureBuffers;
        renderSystem->getCustomAttribute( "MapNoOverwriteOnDynamicBufferSRV",
                                          &supportsNoOverwriteOnTextureBuffers );

        if( !supportsNoOverwriteOnTextureBuffers )
        {
            hlmsPbs->setTextureBufferDefaultSize( 512 * 1024 );
            hlmsUnlit->setTextureBufferDefaultSize( 512 * 1024 );
        }
    }
}

enum IblQuality
{
    MipmapsLowest,
    IblLow,
    IblHigh
};

IblQuality mIblQuality;
Ogre::Camera *mCubeCamera;
Ogre::TextureGpu *mDynamicCubemap;
Ogre::CompositorWorkspace *mDynamicCubemapWorkspace;

bool isAndroid()
{
    return false;
}

const char *getMediaReadArchiveType()
{
#    if OGRE_PLATFORM != OGRE_PLATFORM_ANDROID
    return "FileSystem";
#    else
    return "APKFileSystem";
#    endif
}

void registerHlms()
{
    auto mRoot = Ogre::Root::getSingletonPtr();
    Ogre::String mResourcePath;

    Ogre::ConfigFile cf;
    cf.load( "resources2.cfg" );

#    if OGRE_PLATFORM == OGRE_PLATFORM_APPLE || OGRE_PLATFORM == OGRE_PLATFORM_APPLE_IOS
    Ogre::String rootHlmsFolder =
        Ogre::macBundlePath() + '/' + cf.getSetting( "DoNotUseAsResource", "Hlms", "" );
#    else
    Ogre::String rootHlmsFolder = mResourcePath + cf.getSetting( "DoNotUseAsResource", "Hlms", "" );
#    endif
    if( rootHlmsFolder.empty() )
        rootHlmsFolder = isAndroid() ? "/" : "./";
    else if( *( rootHlmsFolder.end() - 1 ) != '/' )
        rootHlmsFolder += "/";

    Ogre::RenderSystem *renderSystem = mRoot->getRenderSystem();

    Ogre::String shaderSyntax = "GLSL";
    if( renderSystem->getName() == "OpenGL ES 2.x Rendering Subsystem" )
        shaderSyntax = "GLSLES";
    if( renderSystem->getName() == "Direct3D11 Rendering Subsystem" )
        shaderSyntax = "HLSL";
    else if( renderSystem->getName() == "Metal Rendering Subsystem" )
        shaderSyntax = "Metal";

    Ogre::String mainFolderPath;
    Ogre::StringVector libraryFoldersPaths;
    Ogre::StringVector::const_iterator libraryFolderPathIt;
    Ogre::StringVector::const_iterator libraryFolderPathEn;

    Ogre::ArchiveManager &archiveManager = Ogre::ArchiveManager::getSingleton();

    Ogre::HlmsTerra *hlmsTerra = 0;
    Ogre::HlmsManager *hlmsManager = mRoot->getHlmsManager();

    {
        // Create & Register HlmsTerra
        // Get the path to all the subdirectories used by HlmsTerra
        Ogre::HlmsTerra::getDefaultPaths( mainFolderPath, libraryFoldersPaths );
        Ogre::Archive *archiveTerra =
            archiveManager.load( rootHlmsFolder + mainFolderPath, getMediaReadArchiveType(), true );
        Ogre::ArchiveVec archiveTerraLibraryFolders;
        libraryFolderPathIt = libraryFoldersPaths.begin();
        libraryFolderPathEn = libraryFoldersPaths.end();
        while( libraryFolderPathIt != libraryFolderPathEn )
        {
            Ogre::Archive *archiveLibrary = archiveManager.load( rootHlmsFolder + *libraryFolderPathIt,
                                                                 getMediaReadArchiveType(), true );
            archiveTerraLibraryFolders.push_back( archiveLibrary );
            ++libraryFolderPathIt;
        }

        // Create and register the terra Hlms
        hlmsTerra = OGRE_NEW Ogre::HlmsTerra( archiveTerra, &archiveTerraLibraryFolders );
        hlmsManager->registerHlms( hlmsTerra );
    }

    // Add Terra's piece files that customize the PBS implementation.
    // These pieces are coded so that they will be activated when
    // we set the HlmsPbsTerraShadows listener and there's an active Terra
    //(see Tutorial_TerrainGameState::createScene01)
    Ogre::Hlms *hlmsPbs = hlmsManager->getHlms( Ogre::HLMS_PBS );
    Ogre::Archive *archivePbs = hlmsPbs->getDataFolder();
    Ogre::ArchiveVec libraryPbs = hlmsPbs->getPiecesLibraryAsArchiveVec();
    libraryPbs.push_back( Ogre::ArchiveManager::getSingletonPtr()->load(
        rootHlmsFolder + "Hlms/Terra/" + shaderSyntax + "/PbsTerraShadows", getMediaReadArchiveType(),
        true ) );
    hlmsPbs->reloadFrom( archivePbs, &libraryPbs );
}

int main()
{
    Ogre::String mResourcePath = "";
    Ogre::String pluginsPath;
    Ogre::String resourcesPath = "resources2.cfg";

    // only use plugins.cfg if not static
#    ifndef OGRE_STATIC_LIB
    pluginsPath = mResourcePath + "plugins.cfg";
#    endif

    auto root = new Ogre::Root( pluginsPath, mResourcePath + "ogre.cfg", mResourcePath + "Ogre.log" );

    Ogre::ConfigFile cf;
    cf.load( resourcesPath );

#    ifdef OGRE_STATIC_LIB
#        ifdef OGRE_BUILD_RENDERSYSTEM_D3D11
#            if FB_BUILD_RENDERER_DX11
    auto d3d11Plugin = new Ogre::D3D11Plugin;
    root->installPlugin( d3d11Plugin, nullptr );
#            endif
#        endif

#        ifdef OGRE_BUILD_RENDERSYSTEM_METAL
    auto metalPlugin = OGRE_NEW Ogre::MetalPlugin();
    root->installPlugin( metalPlugin, nullptr );
#        endif

#        ifdef OGRE_BUILD_RENDERSYSTEM_GL3PLUS
#            if FB_BUILD_RENDERER_GL3PLUS
    auto glPlugin = OGRE_NEW Ogre::GL3PlusPlugin();
    root->installPlugin( glPlugin, nullptr );
#            endif
#        endif
#    endif

    // auto staticPluginLoader = new Ogre::StaticPluginLoader;
    // staticPluginLoader->load(nullptr);

    const Ogre::String rsDX9Name = "Direct3D9 Rendering Subsystem";
    const Ogre::String rsDX11Name = "Direct3D11 Rendering Subsystem";
    const auto rsGL3Name = String( "OpenGL 3+ Rendering Subsystem" );
    const Ogre::String rsGLName = "OpenGL Rendering Subsystem";

    Ogre::RenderSystem *selectedRenderSystem = nullptr;

#    if OGRE_PLATFORM == OGRE_PLATFORM_WIN32

    auto rsDX9 = root->getRenderSystemByName( rsDX9Name );
    auto rsDX11 = root->getRenderSystemByName( rsDX11Name );
    auto rsGL3 = root->getRenderSystemByName( rsGL3Name );
    auto rsGL = root->getRenderSystemByName( rsGLName );

    if( rsGL )
    {
        selectedRenderSystem = rsGL;
    }
    else if( rsDX11 )
    {
        selectedRenderSystem = rsDX11;
    }
    else if( rsDX9 )
    {
        selectedRenderSystem = rsDX9;
    }
    else if( rsGL3 )
    {
        selectedRenderSystem = rsGL3;
    }

#    elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE

    auto renderers = root->getAvailableRenderers();
    for( auto renderer : renderers )
    {
        selectedRenderSystem = renderer;
        break;
    }

#    else
#        pragma error "Unsupported platform!"
#    endif

    if( selectedRenderSystem )
    {
        root->setRenderSystem( selectedRenderSystem );
    }

    // if (!root->showConfigDialog())
    //{
    //	return -1;
    // }

    auto window = root->initialise( false );

    window = root->createRenderWindow( "default", 1280, 720, false );

    auto overlaySystem = new Ogre::v1::OverlaySystem;
    // OgreBites::TrayManager* mTrayMgr = nullptr;

    try
    {
        baseRegisterHlms();
        registerHlms();

        auto resourceGroupManager = Ogre::ResourceGroupManager::getSingletonPtr();

        Ogre::ConfigFile cf;
        cf.load( "resources2.cfg" );

        // Go through all sections & settings in the file
        Ogre::ConfigFile::SectionIterator seci = cf.getSectionIterator();

        String secName, typeName, archName;
        while( seci.hasMoreElements() )
        {
            secName = seci.peekNextKey();
            Ogre::ConfigFile::SettingsMultiMap *settings = seci.getNext();
            Ogre::ConfigFile::SettingsMultiMap::iterator i;
            for( i = settings->begin(); i != settings->end(); ++i )
            {
                try
                {
                    typeName = i->first;
                    archName = i->second;

                    resourceGroupManager->addResourceLocation( archName, typeName, secName, false );
                }
                catch( std::exception &e )
                {
                    std::cout << e.what() << std::endl;
                }
            }
        }

        resourceGroupManager->initialiseAllResourceGroups( true );
    }
    catch( std::exception &e )
    {
        Ogre::LogManager::getSingletonPtr()->logMessage( e.what() );
    }

    Ogre::Terra *mTerra = nullptr;
    Ogre::Light *mSunLight = nullptr;

    try
    {
        auto numThreads = 0;
        auto sceneManager = root->createSceneManager( Ogre::ST_GENERIC, numThreads, "GameSceneManager" );

        sceneManager->addRenderQueueListener( overlaySystem );

        auto sceneManagerRenderQueue = sceneManager->getRenderQueue();
        WP_ASSERT( sceneManagerRenderQueue );

        auto overlayManager = Ogre::v1::OverlayManager::getSingletonPtr();
        sceneManagerRenderQueue->setSortRenderQueue( overlayManager->mDefaultRenderQueueId,
                                                     Ogre::RenderQueue::StableSort );

        auto camera = sceneManager->createCamera( "DefaultCamera" );

        camera->setPosition( Ogre::Vector3( 0, 5, 15 ) );
        camera->lookAt( Ogre::Vector3( 0, 0, 0 ) );
        camera->setNearClipDistance( 0.2f );
        camera->setFarClipDistance( 1000.0f );
        camera->setAutoAspectRatio( true );

        // auto vp = window->addViewport(camera);
        // vp->setBackgroundColour(Ogre::ColourValue(0.5, 0.5, 0.5, 1));
        // camera->setAspectRatio(vp->getActualWidth() / vp->getActualHeight());
        // vp->setClearEveryFrame(true);
        // vp->setOverlaysEnabled(true);

        auto compositorManager = root->getCompositorManager2();

        switch( testIndex )
        {
        case 0:
        {
            // mTrayMgr = new OgreBites::TrayManager("BrowserControls", window, 0);
            // mTrayMgr->showBackdrop("SdkTrays/Bands");

            // mTrayMgr->destroyAllWidgets();
            // int gui_width = 250;
            //// create main navigation tray
            // mTrayMgr->showLogo(OgreBites::TL_LEFT);
            // mTrayMgr->createButton(OgreBites::TL_NONE, "Apply", "Apply Changes");
            // mTrayMgr->createCheckBox(OgreBites::TL_TOPLEFT, "ibl", "Image Based Lighting",
            // gui_width)->setChecked(true, false); mTrayMgr->showFrameStats(OgreBites::TL_BOTTOMLEFT);
            // mTrayMgr->showAll();
            // mTrayMgr->showCursor();
        }
        break;
        case 1:
        {
            auto overlay = overlayManager->create( "Test" );

            auto panel =
                (Ogre::v1::OverlayContainer *)overlayManager->createOverlayElement( "Panel", "Panel" );
            // panel->setVisible(true);
            // panel->setMaterialName("DebugCube");
            overlay->add2D( panel );

            auto text = (Ogre::v1::TextAreaOverlayElement *)overlayManager->createOverlayElement(
                "TextArea", "TextArea" );
            text->setMetricsMode( Ogre::v1::GuiMetricsMode::GMM_PIXELS );
            text->setHorizontalAlignment( Ogre::v1::GuiHorizontalAlignment::GHA_CENTER );
            text->setVerticalAlignment( Ogre::v1::GuiVerticalAlignment::GVA_CENTER );
            text->setFontName( "DebugFont" );
            text->setCharHeight( 18 );
            text->setCaption( "Test" );
            text->setSpaceWidth( 9 );
            text->setColour( Ogre::ColourValue::White );
            panel->addChild( text );

            // auto textName = String("test");
            // auto mElement =
            // Ogre::v1::OverlayManager::getSingleton().createOverlayElementFromTemplate("SdkTrays/Label",
            // "BorderPanel", textName); auto mTextArea =
            // (Ogre::v1::TextAreaOverlayElement*)((Ogre::v1::OverlayContainer*)mElement)->getChild(textName
            // + "/LabelCaption"); mTextArea->setCaption("Test2"); panel->addChild(mTextArea);

            overlay->show();

            // Setup a basic compositor with a blue clear colour
            const String workspaceName( "Demo Workspace" );
            const ColourValue backgroundColour( 0.2f, 0.4f, 0.6f );
            compositorManager->createBasicWorkspaceDef( workspaceName, backgroundColour, IdString() );
            compositorManager->addWorkspace( sceneManager, window->getTexture(), camera, workspaceName,
                                             true );
        }
        break;
        case 2:
        {
            auto overlayManager = Ogre::v1::OverlayManager::getSingletonPtr();
            auto overlay = overlayManager->getByName( "Core/DebugOverlay" );
            if( overlay )
            {
                overlay->show();
                WP_ASSERT( overlay->isInitialised() );
            }

            // Setup a basic compositor with a blue clear colour
            const String workspaceName( "Demo Workspace" );
            const ColourValue backgroundColour( 0.2f, 0.4f, 0.6f );
            compositorManager->createBasicWorkspaceDef( workspaceName, backgroundColour, IdString() );
            compositorManager->addWorkspace( sceneManager, window->getTexture(), camera, workspaceName,
                                             true );
        }
        break;
        case 3:
        {
            sceneManager->setSky( true, Ogre::SceneManager::SkyCubemap, "SaintPetersBasilica.dds",
                                  Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );

            auto light = sceneManager->createLight();
            auto lightSceneNode = sceneManager->getRootSceneNode()->createChildSceneNode();
            lightSceneNode->attachObject( light );
            lightSceneNode->setPosition( Ogre::Vector3( 0, 100, 0 ) );

            // Setup a basic compositor with a blue clear colour
            const String workspaceName( "Demo Workspace" );
            const ColourValue backgroundColour( 0.2f, 0.4f, 0.6f );
            compositorManager->createBasicWorkspaceDef( workspaceName, backgroundColour, IdString() );
            compositorManager->addWorkspace( sceneManager, window->getTexture(), camera, workspaceName,
                                             true );
        }
        break;
        case 4:
        {
            sceneManager->setSky( true, Ogre::SceneManager::SkyCubemap, "SaintPetersBasilica.dds",
                                  Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );

            if( mDynamicCubemapWorkspace )
            {
                compositorManager->removeWorkspace( mDynamicCubemapWorkspace );
                mDynamicCubemapWorkspace = 0;
            }

            uint32 iblSpecularFlag = 0;
            if( root->getRenderSystem()->getCapabilities()->hasCapability( RSC_COMPUTE_PROGRAM ) &&
                mIblQuality != MipmapsLowest )
            {
                iblSpecularFlag = TextureFlags::Uav | TextureFlags::Reinterpretable;
            }

            // A RenderTarget created with AllowAutomipmaps means the compositor still needs to
            // explicitly generate the mipmaps by calling generate_mipmaps. It's just an API
            // hint to tell the GPU we will be using the mipmaps auto generation routines.
            TextureGpuManager *textureManager = root->getRenderSystem()->getTextureGpuManager();
            mDynamicCubemap =
                textureManager->createOrRetrieveTexture( "DynamicCubemap",
                                                         GpuPageOutStrategy::Discard,          //
                                                         TextureFlags::RenderToTexture |       //
                                                             TextureFlags::AllowAutomipmaps |  //
                                                             iblSpecularFlag,                  //
                                                         TextureTypes::TypeCube );
            mDynamicCubemap->scheduleTransitionTo( GpuResidency::OnStorage );
            uint32 resolution = 512u;
            if( mIblQuality == MipmapsLowest )
                resolution = 1024u;
            else if( mIblQuality == IblLow )
                resolution = 256u;
            else
                resolution = 512u;

            mDynamicCubemap->setResolution( resolution, resolution );
            mDynamicCubemap->setNumMipmaps( Ogre::PixelFormatGpuUtils::getMaxMipmapCount( resolution ) );
            if( mIblQuality != MipmapsLowest )
            {
                // Limit max mipmap to 16x16
                mDynamicCubemap->setNumMipmaps( mDynamicCubemap->getNumMipmaps() - 4u );
            }
            mDynamicCubemap->setPixelFormat( PFG_RGBA8_UNORM_SRGB );
            mDynamicCubemap->scheduleTransitionTo( GpuResidency::Resident );

            Ogre::HlmsManager *hlmsManager = root->getHlmsManager();
            assert( dynamic_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) ) );
            Ogre::HlmsPbs *hlmsPbs =
                static_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_PBS ) );
            hlmsPbs->resetIblSpecMipmap( 0u );

            // Create the camera used to render to our cubemap
            if( !mCubeCamera )
            {
                mCubeCamera = sceneManager->createCamera( "CubeMapCamera", true, true );
                mCubeCamera->setFOVy( Degree( 90 ) );
                mCubeCamera->setAspectRatio( 1 );
                mCubeCamera->setFixedYawAxis( false );
                mCubeCamera->setNearClipDistance( 0.5 );
                // The default far clip distance is way too big for a cubemap-capable camera,
                // hich prevents Ogre from better culling.
                mCubeCamera->setFarClipDistance( 10000 );
                mCubeCamera->setPosition( 0, 1.0, 0 );
            }

            // Note: You don't necessarily have to tie RenderWindow's use of MSAA with cubemap's MSAA
            // You could always use MSAA for the cubemap, or never use MSAA for the cubemap.
            // That's up to you. This sample is tying them together in order to showcase them. That's
            // all.
            const IdString cubemapRendererNode = window->getSampleDescription().isMultisample()
                                                     ? "CubemapRendererNodeMsaa"
                                                     : "CubemapRendererNode";

            {
                CompositorNodeDef *nodeDef =
                    compositorManager->getNodeDefinitionNonConst( cubemapRendererNode );
                const CompositorPassDefVec &passes =
                    nodeDef->getTargetPass( nodeDef->getNumTargetPasses() - 1u )->getCompositorPasses();

                WP_ASSERT( dynamic_cast<CompositorPassIblSpecularDef *>( passes.back() ) );
                CompositorPassIblSpecularDef *iblSpecPassDef =
                    static_cast<CompositorPassIblSpecularDef *>( passes.back() );
                iblSpecPassDef->mForceMipmapFallback = mIblQuality == MipmapsLowest;
                iblSpecPassDef->mSamplesPerIteration = mIblQuality == IblLow ? 32.0f : 128.0f;
                iblSpecPassDef->mSamplesSingleIterationFallback = iblSpecPassDef->mSamplesPerIteration;
            }

            auto overlay = overlayManager->create( "Test" );

            auto panel =
                (Ogre::v1::OverlayContainer *)overlayManager->createOverlayElement( "Panel", "Panel" );
            panel->show();
            // panel->setMaterialName("DebugCube");
            overlay->add2D( panel );

            auto text = (Ogre::v1::TextAreaOverlayElement *)overlayManager->createOverlayElement(
                "TextArea", "TextArea" );
            text->setMetricsMode( Ogre::v1::GuiMetricsMode::GMM_PIXELS );
            text->setHorizontalAlignment( Ogre::v1::GuiHorizontalAlignment::GHA_CENTER );
            text->setVerticalAlignment( Ogre::v1::GuiVerticalAlignment::GVA_CENTER );
            text->setFontName( "DebugFont" );
            text->setCharHeight( 18 );
            text->setCaption( "Test" );
            text->setSpaceWidth( 9 );
            text->setColour( Ogre::ColourValue::White );
            panel->addChild( text );

            // auto textName = String("test");
            // auto mElement =
            // Ogre::v1::OverlayManager::getSingleton().createOverlayElementFromTemplate("SdkTrays/Label",
            // "BorderPanel", textName); auto mTextArea =
            // (Ogre::v1::TextAreaOverlayElement*)((Ogre::v1::OverlayContainer*)mElement)->getChild(textName
            // + "/LabelCaption"); mTextArea->setCaption("Test2"); panel->addChild(mTextArea);

            overlay->show();

            //// Setup the cubemap's compositor.
            // CompositorChannelVec cubemapExternalChannels(1);
            //// Any of the cubemap's render targets will do
            // cubemapExternalChannels[0] = mDynamicCubemap;

            // const Ogre::String workspaceName("Tutorial_DynamicCubemap_cubemap");
            // if (!compositorManager->hasWorkspaceDefinition(workspaceName))
            //{
            //	CompositorWorkspaceDef* workspaceDef =
            //		compositorManager->addWorkspaceDefinition(workspaceName);
            //	//"CubemapRendererNode" has been defined in scripts.
            //	// Very handy (as it 99% the same for everything)
            //	workspaceDef->connectExternal(0, cubemapRendererNode, 0);
            // }

            // mDynamicCubemapWorkspace = compositorManager->addWorkspace(
            //	sceneManager, cubemapExternalChannels, mCubeCamera, workspaceName, true);

            ////// Now setup the regular renderer
            CompositorChannelVec externalChannels( 2 );
            // Render window
            externalChannels[0] = window->getTexture();
            externalChannels[1] = mDynamicCubemap;

            // compositorManager->addWorkspace(sceneManager, window->getTexture(), camera,
            //	"Tutorial_DynamicCubemapWorkspace", true);

            const String workspaceNameRT( "Demo RT" );
            const ColourValue backgroundColourRT( 0.2f, 0.4f, 0.6f );
            compositorManager->createBasicWorkspaceDef( workspaceNameRT, backgroundColourRT,
                                                        IdString() );
            compositorManager->addWorkspace( sceneManager, mDynamicCubemap, camera, workspaceNameRT,
                                             true );

            // Setup a basic compositor with a blue clear colour
            const String workspaceName( "Demo Workspace" );
            const ColourValue backgroundColour( 0.2f, 0.4f, 0.6f );
            compositorManager->createBasicWorkspaceDef( workspaceName, backgroundColour, IdString() );
            compositorManager->addWorkspace( sceneManager, window->getTexture(), camera, workspaceName,
                                             true );

            // Ogre::HlmsMacroblock macroblock;
            // macroblock.mDepthCheck = false;
            // Ogre::HlmsBlendblock blendblock;

            // Ogre::Hlms* hlmsUnlit = root->getHlmsManager()->getHlms(Ogre::HLMS_UNLIT);
            // const Ogre::String datablockName("depthShadow");
            // Ogre::HlmsUnlitDatablock* depthShadow =
            //	(Ogre::HlmsUnlitDatablock*)hlmsUnlit->createDatablock(
            //		datablockName, datablockName, macroblock, blendblock, Ogre::HlmsParamVec());

            // depthShadow->setTexture(0, mDynamicCubemap);
            // panel->setMaterialName(datablockName);

            // compositorManager->addWorkspace(sceneManager, externalChannels, camera,
            //	"Tutorial_DynamicCubemapWorkspace", true);
        }
        break;
        case 5:
        {
            sceneManager->setSky( true, Ogre::SceneManager::SkyCubemap, "SaintPetersBasilica.dds",
                                  Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );

            /// Listener to make PBS objects also be affected by terrain's shadows
            Ogre::HlmsPbsTerraShadows *mHlmsPbsTerraShadows;

            // Render terrain after most objects, to improve performance by taking advantage of early Z
            mTerra = new Ogre::Terra( Ogre::Id::generateNewId<Ogre::MovableObject>(),
                                      &sceneManager->_getEntityMemoryManager( Ogre::SCENE_STATIC ),
                                      sceneManager, 11u, root->getCompositorManager2(), camera, false );
            mTerra->setCastShadows( false );

            // mTerra->load( "Heightmap.png", Ogre::Vector3::ZERO, Ogre::Vector3( 256.0f, 1.0f, 256.0f ),
            // false ); mTerra->load( "Heightmap.png", Ogre::Vector3( 64.0f, 0, 64.0f ), Ogre::Vector3(
            // 128.0f, 5.0f, 128.0f ), false ); mTerra->load( "Heightmap.png", Ogre::Vector3( 64.0f, 0, 64.0f
            // ), Ogre::Vector3( 1024.0f, 5.0f, 1024.0f ), false ); mTerra->load( "Heightmap.png",
            // Ogre::Vector3( 64.0f, 0, 64.0f ), Ogre::Vector3( 4096.0f * 4, 15.0f * 64.0f*4, 4096.0f * 4 ),
            // false );
            mTerra->load( "Heightmap.png", Ogre::Vector3( 64.0f, 4096.0f * 0.5f, 64.0f ),
                          Ogre::Vector3( 4096.0f, 4096.0f, 4096.0f ), false, false );
            // mTerra->load( "Heightmap.png", Ogre::Vector3( 64.0f, 4096.0f * 0.5f, 64.0f ), Ogre::Vector3(
            // 14096.0f, 14096.0f, 14096.0f ), false );

            Ogre::SceneNode *rootNode = sceneManager->getRootSceneNode( Ogre::SCENE_STATIC );
            Ogre::SceneNode *sceneNode = rootNode->createChildSceneNode( Ogre::SCENE_STATIC );
            sceneNode->attachObject( mTerra );

            Ogre::HlmsManager *hlmsManager = root->getHlmsManager();
            Ogre::HlmsDatablock *datablock = hlmsManager->getDatablock( "TerraExampleMaterial" );
            //        Ogre::HlmsDatablock *datablock = hlmsManager->getHlms( Ogre::HLMS_USER3
            //        )->getDefaultDatablock(); Ogre::HlmsMacroblock macroblock; macroblock.mPolygonMode =
            //        Ogre::PM_WIREFRAME;
            // datablock->setMacroblock( macroblock );
            mTerra->setDatablock( datablock );

            //{
            //    mHlmsPbsTerraShadows = new Ogre::HlmsPbsTerraShadows();
            //    mHlmsPbsTerraShadows->setTerra( mTerra );
            //    // Set the PBS listener so regular objects also receive terrain shadows
            //    Ogre::Hlms *hlmsPbs = root->getHlmsManager()->getHlms( Ogre::HLMS_PBS );
            //    hlmsPbs->setListener( mHlmsPbsTerraShadows );
            //}

            mSunLight = sceneManager->createLight();
            Ogre::SceneNode *lightNode = rootNode->createChildSceneNode();
            lightNode->attachObject( mSunLight );
            mSunLight->setPowerScale( Ogre::Math::PI );
            mSunLight->setType( Ogre::Light::LT_DIRECTIONAL );
            mSunLight->setDirection( Ogre::Vector3( -1, -1, -1 ).normalisedCopy() );

            sceneManager->setAmbientLight( Ogre::ColourValue( 0.33f, 0.61f, 0.98f ) * 0.01f,
                                           Ogre::ColourValue( 0.02f, 0.53f, 0.96f ) * 0.01f,
                                           Ogre::Vector3::UNIT_Y );

            // Setup a basic compositor with a blue clear colour
            const String workspaceName( "Demo Workspace" );
            const ColourValue backgroundColour( 0.2f, 0.4f, 0.6f );
            compositorManager->createBasicWorkspaceDef( workspaceName, backgroundColour, IdString() );
            compositorManager->addWorkspace( sceneManager, window->getTexture(), camera, workspaceName,
                                             true );

            camera->setPosition( Ogre::Vector3( 0, 150, 150 ) );
            camera->lookAt( Ogre::Vector3( 0, 150, 0 ) );
        }
        break;
        case 6:
        {
            sceneManager->setSky( true, Ogre::SceneManager::SkyCubemap, "SaintPetersBasilica.dds",
                                  Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME );

            // Setup a basic compositor with a blue clear colour
            const String workspaceName( "Demo Workspace" );
            const ColourValue backgroundColour( 0.2f, 0.4f, 0.6f );
            compositorManager->createBasicWorkspaceDef( workspaceName, backgroundColour, IdString() );
            compositorManager->addWorkspace( sceneManager, window->getTexture(), camera, workspaceName,
                                             true );
        }
        break;
        case 7:
        {
        }
        break;
        default:
        {
        }
        }
    }
    catch( std::exception &e )
    {
        Ogre::LogManager::getSingletonPtr()->logMessage( e.what() );
    }

    auto running = true;
    while( running )
    {
        Ogre::WindowEventUtilities::messagePump();

        // Ogre::FrameEvent evt;
        // evt.timeSinceLastEvent = 0.01;
        // evt.timeSinceLastFrame = 0.01;

        // if (mTrayMgr)
        //{
        //	mTrayMgr->frameRendered(evt);
        // }

        if( mTerra )
        {
            mTerra->update( -Ogre::Vector3::UNIT_Y );
        }

        running = root->renderOneFrame();
    }

    if( mDynamicCubemap )
    {
        // mDynamicCubemap;
    }

    delete root;
    return 0;
}

#elif FB_GRAPHICS_SYSTEM_OGRE
#    include <Ogre.h>
//#include <OgreStaticPluginLoader.hpp>
#    include <OgreWindowEventUtilities.h>
#    include <OgreOverlaySystem.h>
#    include <OgreOverlay.h>
#    include <OgreOverlayManager.h>

#    ifdef OGRE_STATIC_LIB
#        ifdef FB_PLATFORM_WIN32

#            if FB_BUILD_RENDERER_DX9
#                include <OgreD3D9Plugin.h>
#                include <OgreD3D9RenderSystem.h>
#                include <OgreD3D9Driver.h>
#                include <OgreD3D9DriverList.h>
#                include <OgreD3D9VideoModeList.h>
#                include <OgreD3D9VideoMode.h>
#            endif

#            if FB_BUILD_RENDERER_DX11
#                include <OgreD3D11Plugin.h>
#                include <OgreD3D11RenderSystem.h>
#            endif

#        endif

#        if FB_BUILD_RENDERER_OPENGL
#            include <OgreGLPlugin.h>
#        endif

#    endif

#    include <OgreTrays.h>
#    include "OgreSTBICodec.h"

using namespace lioncat;

constexpr int testIndex = 1;

int main()
{
    Ogre::String mResourcePath = "";
    Ogre::String pluginsPath;
    Ogre::String resourcesPath = "resources_overlay_test.cfg";

    // only use plugins.cfg if not static
#    ifndef OGRE_STATIC_LIB
    pluginsPath = mResourcePath + "plugins.cfg";
#    endif

    auto root = new Ogre::Root( pluginsPath, mResourcePath + "ogre.cfg", mResourcePath + "Ogre.log" );

    Ogre::ConfigFile cf;
    cf.load( resourcesPath );

#    ifdef FB_PLATFORM_WIN32
#        if FB_BUILD_RENDERER_DX9
    auto d3d9Plugin = new Ogre::D3D9Plugin;
    root->installPlugin( d3d9Plugin );
#        endif

#        if FB_BUILD_RENDERER_DX11
    auto d3d11Plugin = new Ogre::D3D11Plugin;
    root->installPlugin( d3d11Plugin );
#        endif
#    endif

#    if FB_BUILD_RENDERER_OPENGL
    auto glPlugin = new Ogre::GLPlugin;
    root->installPlugin( glPlugin );
#    endif

    // auto staticPluginLoader = new Ogre::StaticPluginLoader;
    // staticPluginLoader->load(nullptr);

    const Ogre::String rsDX9Name = "Direct3D9 Rendering Subsystem";
    const Ogre::String rsDX11Name = "Direct3D11 Rendering Subsystem";
    const Ogre::String rsGLName = "OpenGL Rendering Subsystem";

    Ogre::RenderSystem *selectedRenderSystem = nullptr;

#    if OGRE_PLATFORM == OGRE_PLATFORM_WIN32

    auto rsDX9 = root->getRenderSystemByName( rsDX9Name );
    auto rsDX11 = root->getRenderSystemByName( rsDX11Name );
    auto rsGL = root->getRenderSystemByName( rsGLName );

    if( rsGL )
    {
        selectedRenderSystem = rsGL;
    }
    else if( rsDX11 )
    {
        selectedRenderSystem = rsDX11;
    }
    else if( rsDX9 )
    {
        selectedRenderSystem = rsDX9;
    }

#    elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE

    RenderSystem *rsGL = m_root->getRenderSystemByName( rsGLName );
    if( rsGL )
    {
        selectedRenderSystem = rsGL;
    }

#    else
#        pragma error "Unsupported platform!"
#    endif

    Ogre::STBIImageCodec::startup();

    if( selectedRenderSystem )
    {
        root->setRenderSystem( selectedRenderSystem );
    }

    auto window = root->initialise( true );
    window->setAutoUpdated( true );

    auto overlaySystem = new Ogre::OverlaySystem;
    OgreBites::TrayManager *mTrayMgr = nullptr;

    try
    {
        auto resourceGroupManager = Ogre::ResourceGroupManager::getSingletonPtr();

        Ogre::String sec, type, arch;
        // go through all specified resource groups
        Ogre::ConfigFile::SettingsBySection_::const_iterator seci;
        for( seci = cf.getSettingsBySection().begin(); seci != cf.getSettingsBySection().end(); ++seci )
        {
            sec = seci->first;
            const Ogre::ConfigFile::SettingsMultiMap &settings = seci->second;
            Ogre::ConfigFile::SettingsMultiMap::const_iterator i;

            // go through all resource paths
            for( i = settings.begin(); i != settings.end(); i++ )
            {
                try
                {
                    type = i->first;
                    arch = i->second;

                    Ogre::StringUtil::trim( arch );
                    if( arch.empty() || arch[0] == '.' )
                    {
                        // resolve relative path with regards to configfile
                        Ogre::String baseDir, filename;
                        Ogre::StringUtil::splitFilename( resourcesPath, filename, baseDir );
                        arch = baseDir + arch;
                    }

                    resourceGroupManager->addResourceLocation( arch, type, sec );
                }
                catch( std::exception &e )
                {
                    Ogre::LogManager::getSingletonPtr()->logMessage( e.what() );
                }
            }
        }

        resourceGroupManager->initialiseAllResourceGroups();
    }
    catch( std::exception &e )
    {
        Ogre::LogManager::getSingletonPtr()->logMessage( e.what() );
    }

    try
    {
        auto sceneManager = root->createSceneManager( "DefaultSceneManager", "GameSceneManager" );

        sceneManager->addRenderQueueListener( overlaySystem );

        auto camera = sceneManager->createCamera( "DefaultCamera" );
        camera->setAutoAspectRatio( true );

        auto vp = window->addViewport( camera );
        vp->setBackgroundColour( Ogre::ColourValue( 0.5, 0.5, 0.5, 1 ) );
        camera->setAspectRatio( vp->getActualWidth() / vp->getActualHeight() );
        vp->setClearEveryFrame( true );
        vp->setOverlaysEnabled( true );

        switch( testIndex )
        {
        case 0:
        {
            mTrayMgr = new OgreBites::TrayManager( "BrowserControls", window, 0 );
            mTrayMgr->showBackdrop( "SdkTrays/Bands" );

            mTrayMgr->destroyAllWidgets();
            int gui_width = 250;
            // create main navigation tray
            mTrayMgr->showLogo( OgreBites::TL_LEFT );
            mTrayMgr->createButton( OgreBites::TL_NONE, "Apply", "Apply Changes" );
            mTrayMgr->createCheckBox( OgreBites::TL_TOPLEFT, "ibl", "Image Based Lighting", gui_width )
                ->setChecked( true, false );
            mTrayMgr->showFrameStats( OgreBites::TL_BOTTOMLEFT );
            mTrayMgr->showAll();
            mTrayMgr->showCursor();
        }
        break;
        case 1:
        {
            auto overlayManager = Ogre::OverlayManager::getSingletonPtr();
            auto overlay = overlayManager->create( "Test" );

            auto panel =
                (Ogre::OverlayContainer *)overlayManager->createOverlayElement( "Panel", "Panel" );
            panel->setVisible( true );
            overlay->add2D( panel );

            auto text = (Ogre::TextAreaOverlayElement *)overlayManager->createOverlayElement(
                "TextArea", "TextArea" );
            text->setMetricsMode( Ogre::GuiMetricsMode::GMM_PIXELS );
            text->setHorizontalAlignment( Ogre::GuiHorizontalAlignment::GHA_CENTER );
            text->setVerticalAlignment( Ogre::GuiVerticalAlignment::GVA_CENTER );
            text->setFontName( "SdkTrays/Caption" );
            text->setCharHeight( 18 );
            text->setCaption( "Test" );
            text->setSpaceWidth( 9 );
            text->setColour( Ogre::ColourValue::White );
            panel->addChild( text );

            auto textName = String( "test" );
            auto mElement = Ogre::OverlayManager::getSingleton().createOverlayElementFromTemplate(
                "SdkTrays/Label", "BorderPanel", textName );
            auto mTextArea = (Ogre::TextAreaOverlayElement *)( (Ogre::OverlayContainer *)mElement )
                                 ->getChild( textName + "/LabelCaption" );
            mTextArea->setCaption( "Test2" );
            // panel->addChild(mTextArea);

            overlay->show();
        }
        break;
        case 2:
        {
            auto overlayManager = Ogre::OverlayManager::getSingletonPtr();
            auto overlay = overlayManager->getByName( "Core/DebugOverlay" );
            if( overlay )
            {
                overlay->show();
                WP_ASSERT( overlay->isInitialised() );
            }
        }
        break;
        default:
        {
        }
        }

        // auto graphicsObject = sceneManager->createEntity("athene.mesh");
        // auto meshSceneNode = sceneManager->getRootSceneNode()->createChildSceneNode();
        // meshSceneNode->attachObject(graphicsObject);

        auto light = sceneManager->createLight( "DefaultLight" );
        auto lightSceneNode = sceneManager->getRootSceneNode()->createChildSceneNode();
        lightSceneNode->attachObject( light );
        lightSceneNode->setPosition( Ogre::Vector3( 0, 100, 0 ) );
    }
    catch( std::exception &e )
    {
        Ogre::LogManager::getSingletonPtr()->logMessage( e.what() );
    }

    auto running = true;
    while( running )
    {
        Ogre::WindowEventUtilities::messagePump();

        Ogre::FrameEvent evt;
        evt.timeSinceLastEvent = 0.01;
        evt.timeSinceLastFrame = 0.01;

        if( mTrayMgr )
        {
            mTrayMgr->frameRendered( evt );
        }

        running = root->renderOneFrame();
    }

    delete root;
    return 0;
}
#else
int main()
{
    return 0;
}
#endif
#else
int main()
{
    return 0;
}
#endif
