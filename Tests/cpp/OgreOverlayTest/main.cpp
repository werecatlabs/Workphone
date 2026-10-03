#include <Workphone/Workphone.hpp>
#include <iostream>

#ifdef _FB_STATIC_LIB_
#if FB_GRAPHICS_SYSTEM_OGRENEXT
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

#    include <OgreHlmsUnlit.h>
#    include <OgreHlmsPbs.h>
#    include <OgreHlmsManager.h>

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
#            ifdef OGRE_BUILD_RENDERSYSTEM_D3D11
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

#    include <OgreMaterial.h>
#    include <OgreTechnique.h>
#    include <OgrePass.h>
#    include <OgreRoot.h>
#    include <OgreResourceGroupManager.h>
#    include <OgreGpuProgramParams.h>
#    include <OgreHlmsPbsDatablock.h>
#    include <OgreHlms.h>
#    include <OgreHlmsPbs.h>
#    include <OgreHlmsUnlitDatablock.h>
#    include <OgreHlmsManager.h>

using namespace lioncat;
using namespace Ogre;

constexpr int testIndex = 1;

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

    auto window = root->initialise( false );
    window = root->createRenderWindow( "default", 1280, 720, false );

    auto overlaySystem = new Ogre::v1::OverlaySystem;
    // OgreBites::TrayManager* mTrayMgr = nullptr;

    try
    {
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

        baseRegisterHlms();
    }
    catch( std::exception &e )
    {
        Ogre::LogManager::getSingletonPtr()->logMessage( e.what() );
    }

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
        camera->setAutoAspectRatio( true );

        // auto vp = window->addViewport(camera);
        // vp->setBackgroundColour(Ogre::ColourValue(0.5, 0.5, 0.5, 1));
        // camera->setAspectRatio(vp->getActualWidth() / vp->getActualHeight());
        // vp->setClearEveryFrame(true);
        // vp->setOverlaysEnabled(true);

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
        }
        break;
        case 3:
        {
            auto overlay = overlayManager->create( "Test" );

            auto panel =
                (Ogre::v1::OverlayContainer *)overlayManager->createOverlayElement( "Panel", "Panel" );
            // panel->setVisible(true);
            // panel->setMaterialName("DebugCube");

            auto hlmsManager = root->getHlmsManager();

            auto hlmsPbs = static_cast<Ogre::HlmsPbs *>( hlmsManager->getHlms( Ogre::HLMS_UNLIT ) );

            Ogre::HlmsMacroblock macroblock;
            macroblock.mDepthCheck = false;

            Ogre::HlmsBlendblock blendblock;
            Ogre::HlmsParamVec paramVec;

            auto datablockName = "DebugPanelTest";
            auto datablock = static_cast<Ogre::HlmsUnlitDatablock *>( hlmsPbs->createDatablock(
                datablockName, datablockName, macroblock, blendblock, paramVec ) );

            datablock->setTexture( 0, "checker.png" );

            // panel->setDatablock(datablock);
            panel->setMaterialName( datablockName );
            panel->setMetricsMode( Ogre::v1::GuiMetricsMode::GMM_PIXELS );
            panel->setLeft( 0 );
            panel->setTop( 0 );
            panel->setWidth( 1280 );
            panel->setHeight( 720 );
            panel->show();
            overlay->add2D( panel );

            /*
            auto text =
            (Ogre::v1::TextAreaOverlayElement*)overlayManager->createOverlayElement("TextArea",
            "TextArea"); text->setMetricsMode(Ogre::v1::GuiMetricsMode::GMM_PIXELS);
            text->setHorizontalAlignment(Ogre::v1::GuiHorizontalAlignment::GHA_CENTER);
            text->setVerticalAlignment(Ogre::v1::GuiVerticalAlignment::GVA_CENTER);
            text->setFontName("DebugFont");
            text->setCharHeight(18);
            text->setCaption("Test");
            text->setSpaceWidth(9);
            text->setColour(Ogre::ColourValue::White);
            panel->addChild(text);
            */

            // auto textName = String("test");
            // auto mElement =
            // Ogre::v1::OverlayManager::getSingleton().createOverlayElementFromTemplate("SdkTrays/Label",
            // "BorderPanel", textName); auto mTextArea =
            // (Ogre::v1::TextAreaOverlayElement*)((Ogre::v1::OverlayContainer*)mElement)->getChild(textName
            // + "/LabelCaption"); mTextArea->setCaption("Test2"); panel->addChild(mTextArea);

            overlay->show();
        }
        break;
        default:
        {
        }
        }

        // auto graphicsObject = sceneManager->createEntity("athene.mesh");
        // auto meshSceneNode = sceneManager->getRootSceneNode()->createChildSceneNode();
        // meshSceneNode->attachObject(graphicsObject);

        auto light = sceneManager->createLight();
        auto lightSceneNode = sceneManager->getRootSceneNode()->createChildSceneNode();
        lightSceneNode->attachObject( light );
        lightSceneNode->setPosition( Ogre::Vector3( 0, 100, 0 ) );

        // Setup a basic compositor with a blue clear colour
        CompositorManager2 *compositorManager = root->getCompositorManager2();
        const String workspaceName( "Demo Workspace" );
        const ColourValue backgroundColour( 0.2f, 0.4f, 0.6f );
        compositorManager->createBasicWorkspaceDef( workspaceName, backgroundColour, IdString() );
        compositorManager->addWorkspace( sceneManager, window->getTexture(), camera, workspaceName,
                                         true );
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

        running = root->renderOneFrame();
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

#    if FB_BUILD_RENDERER_GL
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
