#include <Workphone/Workphone.hpp>

constexpr int testStart = 0;
constexpr int testEnd = 15;

int main()
{
    using namespace workphone;

    for( u32 i = testStart; i < testEnd; ++i )
    {
        auto testIndex = i;
        // auto testIndex = 11;

        auto currentThread = Thread::ThreadId::Primary;
        Thread::setCurrentThreadId( currentThread );

        auto currentTask = TaskId::Primary;
        Thread::setCurrentTask( currentTask );

        auto applicationManager = new core::ApplicationManager;
        core::ApplicationManager::setInstance( applicationManager );

        try
        {
            auto factoryManager = workphone::make_ptr<FactoryManager>();
            applicationManager->setFactoryManager( factoryManager );
            WP_ASSERT( applicationManager->getFactoryManager() );

            auto logManager = workphone::make_ptr<LogManagerDefault>();
            applicationManager->setLogManager( logManager );
            logManager->open( "OverlayTest.log" );

#ifdef _WP_STATIC_LIB_
            auto databasePlugin = workphone::make_ptr<SQLitePlugin>();
            applicationManager->addPlugin( databasePlugin );
#endif

#ifdef _WP_STATIC_LIB_
#    if WP_GRAPHICS_SYSTEM_OGRENEXT
            auto graphicsPlugin = workphone::make_ptr<render::WPGraphicsOgreNext>();
            applicationManager->addPlugin( graphicsPlugin );
#    elif WP_GRAPHICS_SYSTEM_OGRE
            auto graphicsPlugin = workphone::make_ptr<render::WPGraphicsOgre>();
            applicationManager->addPlugin( graphicsPlugin );
#    endif
#endif

            auto timer = workphone::make_ptr<TimerBoost>();
            applicationManager->setTimer( timer );

            auto stateManager = workphone::make_ptr<StateManager>();
            applicationManager->setStateManager( stateManager );

            auto fsmManager = workphone::make_ptr<FSMManager>();
            fsmManager->load( nullptr );
            applicationManager->setFsmManager( fsmManager );

            auto fileSystem = factoryManager->make_object<IFileSystem>();
            applicationManager->setFileSystem( fileSystem );

            auto taskManager = workphone::make_ptr<TaskManager>();
            applicationManager->setTaskManager( taskManager );

            auto mediaPath = String( "" );

            auto packs = fileSystem->getFiles( mediaPath + "/packs" );
            for( auto &pack : packs )
            {
                fileSystem->addFileArchive( pack, true, true, IFileSystem::ArchiveType::Zip );
            }

            fileSystem->addFolder( mediaPath, true );

            applicationManager->setMediaPath( mediaPath );

            auto resourceDatabase = workphone::make_ptr<ResourceDatabase>();
            resourceDatabase->load( nullptr );
            applicationManager->setResourceDatabase( resourceDatabase );

            WP_ASSERT( resourceDatabase->isLoaded() );

            auto graphicsSystem = factoryManager->make_object<render::IGraphicsSystem>( "OgreNext" );
            applicationManager->setGraphicsSystem( graphicsSystem );
            graphicsSystem->load( nullptr );

            if( !graphicsSystem->configure( nullptr ) )
            {
                return false;
            }

            auto resourceGroupManager = graphicsSystem->getResourceGroupManager();
            WP_ASSERT( resourceGroupManager );

            resourceGroupManager->load( nullptr );

            auto window = graphicsSystem->getDefaultWindow();

            auto renderSceneManager =
                graphicsSystem->addGraphicsScene( "DefaultSceneManager", "GameSceneManager" );

            auto camera = renderSceneManager->addGraphicsObjectByType<render::IGraphicsCamera>();
            camera->setName( "DefaultCamera" );

            auto cameraSceneNode =
                renderSceneManager->getRootSceneNode()->addChildSceneNode( "DefaultCamera" );
            cameraSceneNode->attachObject( camera );

            camera->setNearClipDistance( 0.001f );
            camera->setFarClipDistance( 10000.f );

            camera->setVisible( true );

            auto vp = window->addViewport( 0, camera );
            vp->setBackgroundColour( ColourF( 1, 0, 0, 1 ) );
            // camera->setAspectRatio(f32(vp->getActualWidth()) / f32(vp->getActualHeight()));
            vp->setClearEveryFrame( true );
            vp->setOverlaysEnabled( true );

            ApplicationUtil::createDefaultMaterialUI();
            ApplicationUtil::createDefaultMaterial();
            ApplicationUtil::createDefaultFont();

            auto sceneManager = workphone::make_ptr<scene::GameManager>();
            // sceneManager->setEnableReferenceTracking(true);
            applicationManager->setGameManager( sceneManager );
            sceneManager->load( nullptr );

            auto scene = workphone::make_ptr<scene::GameScene>();
            sceneManager->setCurrentScene( scene );
            scene->load( nullptr );

            //auto renderUI = ui::FBRenderUI::createUIManager();
            //applicationManager->setRenderUI( renderUI );

            auto pOverlay = SmartPtr<render::IOverlay>();

            SmartPtr<IFrameStatistics> frameStatistics;

            resourceDatabase->build();

            switch( testIndex )
            {
            case 0:
            {
                auto overlayManager = graphicsSystem->getOverlayManager();
                WP_ASSERT( overlayManager );

                auto overlay = overlayManager->addOverlay( "Test" );
                WP_ASSERT( overlay );

                auto panel = overlayManager->addElement( "Panel", "Panel" );
                WP_ASSERT( panel );

                panel->setSize( Vector2F( 1920, 1080 ) );
                //panel->setMaterialName( "DefaultUI" );
                panel->setVisible( true );
                overlay->addElement( panel );

                auto text = workphone::static_pointer_cast<render::IOverlayElementText>(
                    overlayManager->addElement( "TextArea", "TextArea" ) );
                // text->setMetricsMode(Ogre::v1::GuiMetricsMode::GMM_PIXELS);
                // text->setHorizontalAlignment(Ogre::v1::GuiHorizontalAlignment::GHA_CENTER);
                // text->setVerticalAlignment(Ogre::v1::GuiVerticalAlignment::GVA_CENTER);
                // text->setFontName("DebugFont");
                // text->setCharHeight(18);
                // text->setCaption("Test");
                // text->setSpaceWidth(9);
                // text->setColour(ColourF::White);

                WP_ASSERT( text->isLoaded() );
                WP_ASSERT( text->isValid() );

                text->setFontName( "SdkTrays/Caption" );
                text->setCaption( "Test" );
                text->setPosition( Vector2F::zero() );
                // text->setSize( Vector2F::unit() * 400.0 );
                text->setVisible( true );
                panel->addChild( text );

                overlay->setVisible( true );
                pOverlay = overlay;
            }
            break;
            case 1:
            {
                auto actor = sceneManager->createActor();
                scene->addActor( actor );
                scene->registerAllUpdates( actor );

                auto canvas = actor->addComponent<scene::Layout>();

                auto actorText = sceneManager->createActor();
                actor->addChild( actorText );

                auto text = actorText->addComponent<scene::Text>();
            }
            break;
            case 2:
            {
                auto actor = sceneManager->createActor();
                scene->addActor( actor );
                scene->registerAllUpdates( actor );

                auto canvas = actor->addComponent<scene::Layout>();

                auto actorButton = sceneManager->createActor();
                actor->addChild( actorButton );
                actorButton->setLocalPosition( Vector3F( 200, 500, 0 ) );

                auto name = String( "Button" );
                actorButton->setName( name );

                actorButton->addComponent<scene::Image>();
                actorButton->addComponent<scene::Button>();

                auto material = actorButton->addComponent<scene::Material>();
                if( material )
                {
                    // material->setMaterialName("TestMat");
                    // material->setMainTexturePath("BumpyMetal.jpg");
                }

                auto actorText = sceneManager->createActor();
                actorButton->addChild( actorText );

                auto canvasTransform = actorText->addComponent<scene::LayoutTransform>();
                auto text = actorText->addComponent<scene::Text>();

                actorText->setLocalPosition( Vector3F( 100, 100, 0 ) );
                text->setText( "Button" );

                auto textName = String( "Text" );
                actorText->setName( textName );
            }
            break;
            case 3:
            {
                auto actor = sceneManager->createActor();
                scene->addActor( actor );
                scene->registerAllUpdates( actor );

                auto canvas = actor->addComponent<scene::Layout>();

                auto actorTextParent = sceneManager->createActor();
                actor->addChild( actorTextParent );

                auto actorText = sceneManager->createActor();
                actorTextParent->addChild( actorText );

                auto text = actorText->addComponent<scene::Text>();
            }
            break;
            case 4:
            {
                auto overlayManager = graphicsSystem->getOverlayManager();
                auto overlay = overlayManager->addOverlay( "Test" );

                auto panel = overlayManager->addElement( "Panel", "Panel" );

                //panel->setMaterialName( "DefaultUI.mat" );
                //panel->setVisible( true );
                overlay->addElement( panel );

                auto text = workphone::static_pointer_cast<render::IOverlayElementText>(
                    overlayManager->addElement( "TextArea", "TextArea" ) );
                panel->addChild( text );

                text->setFontName( "DebugFont" );
                text->setCaption( "Test" );

                auto materialManager = graphicsSystem->getMaterialManager();

                auto materialResource = materialManager->loadFromFile( mediaPath + "/DefaultUI.mat" );
                if( materialResource )
                {
                    auto material =
                        workphone::static_pointer_cast<render::IMaterial>( materialResource );
                    material->setMaterialType( MaterialType::UI );
                    WP_ASSERT( material->getMaterialType() == MaterialType::UI );

                    auto textureName = String( "checker.png" );
                    material->setTexture( textureName );
                    //WP_ASSERT( material->getTexture() == textureName );

                    panel->setMaterial( material );
                    WP_ASSERT( panel->getMaterial() == material );
                    //WP_ASSERT( panel->getMaterialName() == material->getHandle()->getName() );
                }

                //panel->setMaterialName( "DefaultUI.mat" );

                panel->setPosition( Vector2F::zero() );
                panel->setSize( Vector2F( 1920.0f, 1080.0f ) );

                overlay->setVisible( true );
                pOverlay = overlay;
            }
            break;
            case 5:
            {
                auto actor = sceneManager->createActor();
                scene->addActor( actor );
                scene->registerAllUpdates( actor );

                actor->setLocalPosition( Vector3F( 0, 0, 0 ) );
                actor->setPosition( Vector3F( 0, 0, 0 ) );

                auto canvas = actor->addComponent<scene::Layout>();
                canvas->setReferenceSize( Vector2I( 1920, 1080 ) );

                auto actorImageParent = sceneManager->createActor();
                actor->addChild( actorImageParent );

                auto actorImageParentCanvasTransform =
                    actorImageParent->addComponent<scene::LayoutTransform>();
                actorImageParentCanvasTransform->setSize( Vector2F( 1920.0f, 1080.0f ) );

                auto actorImage = sceneManager->createActor();
                actorImageParent->addChild( actorImage );

                auto actorImageCanvasTransform = actorImage->addComponent<scene::LayoutTransform>();
                actorImageCanvasTransform->setSize( Vector2F( 1280.0f, 720.0f ) * 1.0f );

                auto image = actorImage->addComponent<scene::Image>();
                // image->setImagePath(mediaPath + "/checker.png");
                //image->setImagePath( "f40_hd.jpg" );

                auto materialComponent = actorImage->addComponent<scene::Material>();
                materialComponent->setMaterialPath( mediaPath + "/DefaultUI.mat" );

                // auto material = materialComponent->getMaterial();
                // if (material)
                //{
                //	material->setMaterialType(MaterialType::UI);
                // }
            }
            break;
            case 6:
            {
            }
            break;
            case 7:
            {
            }
            break;
            case 8:
            {
                auto actor = sceneManager->createActor();
                scene->addActor( actor );
                scene->registerAllUpdates( actor );

                actor->setLocalPosition( Vector3F( 0, 0, 0 ) );
                actor->setPosition( Vector3F( 0, 0, 0 ) );

                auto canvas = actor->addComponent<scene::Layout>();
                canvas->setReferenceSize( Vector2I( 1920, 1080 ) );

                auto actorImageParent = sceneManager->createActor();
                actor->addChild( actorImageParent );

                auto actorImageParentCanvasTransform =
                    actorImageParent->addComponent<scene::LayoutTransform>();
                actorImageParentCanvasTransform->setSize( Vector2F( 1920.0f, 1080.0f ) );

                auto actorImage = sceneManager->createActor();
                actorImageParent->addChild( actorImage );

                auto actorImageCanvasTransform = actorImage->addComponent<scene::LayoutTransform>();
                actorImageCanvasTransform->setSize( Vector2F( 1280.0f, 720.0f ) * 1.0f );

                auto image = actorImage->addComponent<scene::Image>();
                // image->setImagePath(mediaPath + "/checker.png");
                //image->setImagePath( "f40_hd.jpg" );

                auto materialComponent = actorImage->addComponent<scene::Material>();
                materialComponent->setMaterialPath( mediaPath + "/DefaultUI.mat" );

                auto actorText = sceneManager->createActor();
                actorImage->addChild( actorText );

                auto text = actorText->addComponent<scene::Text>();

                // auto material = materialComponent->getMaterial();
                // if (material)
                //{
                //	material->setMaterialType(MaterialType::UI);
                // }
            }
            break;
            case 9:
            {
                auto actor = sceneManager->createActor();
                scene->addActor( actor );
                scene->registerAllUpdates( actor );

                actor->setLocalPosition( Vector3F( 0, 0, 0 ) );
                actor->setPosition( Vector3F( 0, 0, 0 ) );

                auto canvas = actor->addComponent<scene::Layout>();
                canvas->setReferenceSize( Vector2I( 1920, 1080 ) );

                auto actorImageParent = sceneManager->createActor();
                actor->addChild( actorImageParent );

                auto actorImageParentCanvasTransform =
                    actorImageParent->addComponent<scene::LayoutTransform>();
                actorImageParentCanvasTransform->setSize( Vector2F( 1920.0f, 1080.0f ) );

                auto actorImage = sceneManager->createActor();
                actorImageParent->addChild( actorImage );

                auto actorImageCanvasTransform = actorImage->addComponent<scene::LayoutTransform>();
                actorImageCanvasTransform->setSize( Vector2F( 1280.0f, 720.0f ) * 1.0f );

                auto image = actorImage->addComponent<scene::Image>();
                // image->setImagePath(mediaPath + "/checker.png");
                // image->setImagePath("f40_hd.jpg");

                auto materialComponent = actorImage->addComponent<scene::Material>();
                // materialComponent->setMaterialPath(mediaPath + "/DefaultUI.mat");

                auto actorText = sceneManager->createActor();
                actorImage->addChild( actorText );

                auto text = actorText->addComponent<scene::Text>();

                // auto material = materialComponent->getMaterial();
                // if (material)
                //{
                //	material->setMaterialType(MaterialType::UI);
                // }
            }
            break;
            case 10:
            {
                auto actor = sceneManager->createActor();
                scene->addActor( actor );
                scene->registerAllUpdates( actor );

                actor->setLocalPosition( Vector3F( 0, 0, 0 ) );
                actor->setPosition( Vector3F( 0, 0, 0 ) );

                auto canvas = actor->addComponent<scene::Layout>();
                canvas->setReferenceSize( Vector2I( 1920, 1080 ) );

                auto actorImageParent = sceneManager->createActor();
                actor->addChild( actorImageParent );

                auto actorImageParentCanvasTransform =
                    actorImageParent->addComponent<scene::LayoutTransform>();
                actorImageParentCanvasTransform->setSize( Vector2F( 1920.0f, 1080.0f ) );

                auto actorImage = sceneManager->createActor();
                actorImageParent->addChild( actorImage );

                auto actorImageCanvasTransform = actorImage->addComponent<scene::LayoutTransform>();
                actorImageCanvasTransform->setSize( Vector2F( 1280.0f, 720.0f ) * 1.0f );

                auto image = actorImage->addComponent<scene::Image>();
                // image->setImagePath(mediaPath + "/checker.png");
                //image->setImagePath( "f40_hd.jpg" );

                auto materialComponent = actorImage->addComponent<scene::Material>();

                auto materialPath = mediaPath + "/DefaultUI.mat";
                materialPath = StringUtil::cleanupPath( materialPath );
                materialComponent->setMaterialPath( materialPath );
            }
            break;
            case 11:
            {
                auto canvas1 = ApplicationUtil::createOverlayPanelTest();
                // auto canvas2 = ApplicationUtil::createOverlayPanelTest();
            }
            break;
            case 12:
            {
                auto overlayManager = graphicsSystem->getOverlayManager();
                auto overlay = overlayManager->addOverlay( "Test" );

                auto panel = overlayManager->addElement( "Panel", "Panel" );
                // panel->setVisible(true);
                overlay->addElement( panel );
            }
            break;
            case 13:
            {
                frameStatistics = workphone::make_ptr<FrameStatistics>();
                frameStatistics->load( nullptr );
                WP_ASSERT( frameStatistics->isValid() );
            }
            break;
            case 14:
            {
                auto actor = sceneManager->createActor();
                scene->addActor( actor );
                scene->registerAllUpdates( actor );

                actor->setLocalPosition( Vector3F( 0, 0, 0 ) );
                actor->setPosition( Vector3F( 0, 0, 0 ) );

                auto canvas = actor->addComponent<scene::Layout>();
                canvas->setReferenceSize( Vector2I( 1920, 1080 ) );

                auto actorImageParent = sceneManager->createActor();
                actor->addChild( actorImageParent );

                auto actorImageParentCanvasTransform =
                    actorImageParent->addComponent<scene::LayoutTransform>();
                actorImageParentCanvasTransform->setSize( Vector2F( 1920.0f, 1080.0f ) );

                auto actorImage = sceneManager->createActor();
                actorImageParent->addChild( actorImage );

                auto actorImageCanvasTransform = actorImage->addComponent<scene::LayoutTransform>();
                actorImageCanvasTransform->setSize( Vector2F( 1280.0f, 720.0f ) * 2.0f );

                auto imageTransform = actorImage->addComponent<scene::LayoutTransform>();

                auto image = actorImage->addComponent<scene::Image>();
                image->setTextureName( "checker.png" );
                //image->setTextureName( "f40_hd.jpg" );

                auto materialComponent = actorImage->addComponent<scene::Material>();

                auto materialPath = mediaPath + "/DefaultUI.mat";
                materialPath = StringUtil::cleanupPath( materialPath );
                materialComponent->setMaterialPath( materialPath );

                auto actorTextParent = sceneManager->createActor();
                actorImage->addChild( actorTextParent );

                auto textTransform = actorTextParent->addComponent<scene::LayoutTransform>();

                auto textComponent = actorTextParent->addComponent<scene::Text>();
                textComponent->setText( "Test" );

                textTransform->setVerticalAlignment( VerticalAlignment::CENTER );
                textTransform->setHorizontalAlignment( HorizontalAlignment::CENTER );

                textTransform->setPosition( Vector2F( 1280, 720 ) );
                textTransform->setSize( Vector2F( 300, 100 ) );

                textTransform->updateTransform();
            }
            break;
            default:
            {
            }
            };

            graphicsSystem->setupRenderer( renderSceneManager, window, camera, "TestOverlay", true );

            timer->reset();

            sceneManager->play();

            auto endTime = timer->now() + 5.0;
            while( applicationManager->isRunning() )
            {
                timer->update();

                fsmManager->update();

                if( frameStatistics )
                {
                    WP_ASSERT( frameStatistics->isValid() );
                    frameStatistics->update();
                    WP_ASSERT( frameStatistics->isValid() );
                }

                stateManager->preUpdate();
                stateManager->update();
                stateManager->postUpdate();

                sceneManager->preUpdate();
                sceneManager->update();
                sceneManager->postUpdate();

                graphicsSystem->update();

                graphicsSystem->messagePump();
                if( applicationManager->getQuit() )
                {
                    applicationManager->setRunning( false );
                }

                if( endTime < timer->now() )
                {
                    break;
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        applicationManager->unload( nullptr );
        core::ApplicationManager::setInstance( nullptr );

        delete applicationManager;
        applicationManager = nullptr;
        WP_ASSERT( core::ApplicationManager::instance() == nullptr );

#if WP_ENABLE_MEMORY_TRACKER
        auto &memoryTracker = MemoryTracker::get();
        memoryTracker.reportLeaks();
#endif
    }

    return 0;
}
