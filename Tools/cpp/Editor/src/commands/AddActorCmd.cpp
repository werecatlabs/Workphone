#include <EditorPCH.hpp>
#include <commands/AddActorCmd.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <ui/UIManager.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    using namespace scene;

    WP_CLASS_REGISTER_DERIVED( workphone::editor, AddActorCmd, Command );

    const String AddActorCmd::prefabExtStr = ".prefab";

    AddActorCmd::AddActorCmd()
    {
        setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_RECEIVE_EVENTS, true );
    }

    AddActorCmd::~AddActorCmd() = default;

    void AddActorCmd::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loaded );
    }

    void AddActorCmd::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloaded );
    }

    void AddActorCmd::undo()
    {
        RecursiveMutex::ScopedLock lock( m_mutex );

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto editorManager = EditorManager::getSingletonPtr();
        auto projectManager = editorManager->getProjectManager();
        auto uiManager = editorManager->getUI();

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        if( auto actor = getActor() )
        {
            if( auto parent = actor->getParent() )
            {
                parent->removeChild( actor );
            }

            scene->unregisterAll( actor );
            scene->removeActor( actor );
        }

        uiManager->rebuildSceneTree();
    }

    void AddActorCmd::redo()
    {
        RecursiveMutex::ScopedLock lock( m_mutex );

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto editorManager = EditorManager::getSingletonPtr();
        auto projectManager = editorManager->getProjectManager();
        auto uiManager = editorManager->getUI();
        auto project = editorManager->getProject();

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        if( auto actor = getActor() )
        {
            if( auto parent = getParent() )
            {
                parent->addChild( actor );
            }
            else if( !actor->getParent() )
            {
                scene->addActor( actor );
            }

            scene->registerAllUpdates( actor );
            uiManager->rebuildSceneTree();

            return;
        }

        if( auto actor = createActor() )
        {
            scene->addActor( actor );
            scene->registerAllUpdates( actor );

            setActor( actor );
        }

        uiManager->rebuildSceneTree();
    }

    void AddActorCmd::execute()
    {
        RecursiveMutex::ScopedLock lock( m_mutex );

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto editorManager = EditorManager::getSingletonPtr();
        WP_ASSERT( editorManager );

        auto projectManager = editorManager->getProjectManager();
        WP_ASSERT( projectManager );

        auto uiManager = editorManager->getUI();
        WP_ASSERT( uiManager );

        auto project = editorManager->getProject();
        WP_ASSERT( project );

        auto selectionManager = applicationManager->getSelectionManager();
        WP_ASSERT( selectionManager );

        auto selection = selectionManager->getSelection();

        SmartPtr<IGameActor> parent;

        if( !selection.empty() )
        {
            auto selected = selection.back();

            if( selected->isDerived<IGameActor>() )
            {
                parent = workphone::dynamic_pointer_cast<IGameActor>( selected );
            }
        }

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        WP_ASSERT( scene );

        auto actor = createActor();
        //WP_ASSERT( actor );

        if( !parent )
        {
            auto actorType = getActorType();
            if( actorType == ActorType::Button || actorType == ActorType::Checkbox ||
                actorType == ActorType::Dropdown || actorType == ActorType::Panel ||
                actorType == ActorType::Slider || actorType == ActorType::SliderVertical ||
                actorType == ActorType::Text )
            {
                auto layout = scene->getComponent<scene::Layout>();
                if( !layout )
                {
                    auto layoutActor = sceneManager->createActor();
                    layoutActor->setName( "Layout" );

                    layoutActor->addComponent<LayoutTransform>();

                    layout = layoutActor->addComponent<scene::Layout>();

                    scene->addActor( layoutActor );

                    parent = layoutActor;
                }
            }
        }

        if( actor )
        {
            if( parent )
            {
                setParent( parent );
                parent->addChild( actor );
            }
            else
            {
                auto s = actor->getScene();
                if( !s )
                {
                    scene->addActor( actor );
                }
            }

            // Keep initial creation consistent with redo(). Components need to
            // receive scene updates after the actor has been inserted.
            scene->registerAllUpdates( actor );

            setActor( actor );

            if( applicationManager->isPlaying() )
            {
                actor->setState( IGameActor::State::Play );
            }
            else
            {
                actor->setState( IGameActor::State::Edit );
            }

            // Particle presets are configured before insertion, but must only
            // become visible and start once their actor belongs to the scene.
            if( auto particleSystem = actor->getComponent<ParticleSystem>() )
            {
                particleSystem->updateTransform();
                particleSystem->updateVisibility();
                particleSystem->play();
            }

            applicationManager->triggerEvent( EventType::Actor, IComponent::hierarchyChanged,
                                              Array<Parameter>(), this, actor, nullptr );
        }

        uiManager->rebuildSceneTree();
    }

    SmartPtr<IGameActor> AddActorCmd::createActor()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto application = applicationManager->getApplication();

        auto editorManager = EditorManager::getSingletonPtr();
        WP_ASSERT( editorManager );

        auto projectManager = editorManager->getProjectManager();
        WP_ASSERT( projectManager );

        auto uiManager = editorManager->getUI();
        WP_ASSERT( uiManager );

        auto gameManager = applicationManager->getGameManagerPtr();
        WP_ASSERT( gameManager );

        auto gameScene = gameManager->getCurrentScene();
        WP_ASSERT( gameScene );

        auto createParticleSystemActor = [&gameManager]( const String &name, ActorType preset ) {
            auto actor = gameManager->createActor();
            if( !actor )
            {
                return SmartPtr<IGameActor>();
            }

            actor->setName( name );

            auto particleSystem = actor->addComponent<ParticleSystem>();
            if( !particleSystem )
            {
                return actor;
            }

            // Configure the complete preset before allowing it to emit. This
            // keeps creation deterministic even when components are loaded
            // immediately by the current scene.
            particleSystem->stop();
            particleSystem->setPlayOnLoad( true );
            particleSystem->setLooping( true );
            particleSystem->setFastForwardInterval( 0.05f );

            switch( preset )
            {
            case ActorType::ParticleSystemSmoke:
                particleSystem->setFastForwardTime( 0.75f );
                particleSystem->setLifetime( 6.0f );
                particleSystem->setDuration( 8.0f );
                particleSystem->setStartLifetime( Vector2<real_Num>( 3.0f, 6.0f ) );
                particleSystem->setStartSize( Vector2<real_Num>( 0.45f, 1.1f ) );
                particleSystem->setRate( 18.0f );
                particleSystem->setRateVariance( 3.0f );
                particleSystem->setAngle( 10.0f );
                particleSystem->setAngleVariance( 18.0f );
                particleSystem->setShapeSize( 0.65f );
                particleSystem->setShapeSizeVariance( 0.2f );
                break;
            case ActorType::ParticleSystemSand:
                particleSystem->setFastForwardTime( 0.2f );
                particleSystem->setLifetime( 2.0f );
                particleSystem->setDuration( 5.0f );
                particleSystem->setStartLifetime( Vector2<real_Num>( 0.8f, 2.0f ) );
                particleSystem->setStartSize( Vector2<real_Num>( 0.06f, 0.14f ) );
                particleSystem->setRate( 45.0f );
                particleSystem->setRateVariance( 8.0f );
                particleSystem->setAngle( 5.0f );
                particleSystem->setAngleVariance( 10.0f );
                particleSystem->setShapeSize( 2.5f );
                particleSystem->setShapeSizeVariance( 0.6f );
                break;
            case ActorType::ParticleSystem:
            default:
                particleSystem->setFastForwardTime( 0.35f );
                particleSystem->setLifetime( 3.0f );
                particleSystem->setDuration( 5.0f );
                particleSystem->setStartLifetime( Vector2<real_Num>( 1.5f, 3.0f ) );
                particleSystem->setStartSize( Vector2<real_Num>( 0.2f, 0.4f ) );
                particleSystem->setRate( 12.0f );
                particleSystem->setRateVariance( 2.0f );
                particleSystem->setAngle( 15.0f );
                particleSystem->setAngleVariance( 10.0f );
                particleSystem->setShapeSize( 1.5f );
                particleSystem->setShapeSizeVariance( 0.35f );
                break;
            }

            // AddActorCmd starts the system after the actor belongs to the scene.
            // Starting here would cache "not in scene" visibility on the renderer.
            return actor;
        };

        auto createAircraftActor = [&gameManager]( const String &name,
                                                   const Vector3<real_Num> &scale, f32 mass,
                                                   real_Num sectionMultiplier,
                                                   real_Num rollwiseDamping ) {
            auto actor = gameManager->createActor();
            if( !actor )
            {
                return SmartPtr<IGameActor>();
            }

            actor->setName( name );
            actor->setLocalPosition( Vector3<real_Num>::unitY() * static_cast<real_Num>( 5.0 ) );
            actor->setLocalScale( scale );

            auto collision = actor->addComponent<CollisionBox>();
            WP_ASSERT( collision );

            auto rigidbody = actor->addComponent<Rigidbody>();
            WP_ASSERT( rigidbody );

            auto vehicle = actor->addComponent<VehicleController>();
            WP_ASSERT( vehicle );
            if( vehicle )
            {
                vehicle->setVehicleType(
                    static_cast<s32>( VehicleController::VehicleType::Aircraft ) );
                vehicle->setMass( mass );
                vehicle->setAerodynamicSectionMultiplier( sectionMultiplier );
                vehicle->setRollwiseDamping( rollwiseDamping );
            }

            auto mesh = actor->addComponent<scene::Mesh>();
            WP_ASSERT( mesh );
            if( mesh )
            {
                mesh->setMeshPath( "cube_internal.fbmeshbin" );
            }

            auto meshRenderer = actor->addComponent<MeshRenderer>();
            WP_ASSERT( meshRenderer );

            auto material = actor->addComponent<Material>();
            WP_ASSERT( material );
            if( material )
            {
                material->setMaterialPath( "Standard.mat" );
                material->updateMaterial();
            }

            actor->updateTransform();
            return actor;
        };

        switch( auto actorType = getActorType() )
        {
        case ActorType::Actor:
        {
            auto filePath = getFilePath();
            auto fileExt = Path::getFileExtension( filePath );
            auto fileExtLower = StringUtil::make_lower( fileExt );

            if( ApplicationUtil::isSupportedMesh( filePath ) || fileExtLower == prefabExtStr )
            {
                auto prefabManager = applicationManager->getPrefabManager();

                auto selectionManager = applicationManager->getSelectionManager();

                auto prefabResource = prefabManager->loadResource( filePath );
                if( prefabResource )
                {
                    auto prefab = workphone::static_pointer_cast<IGamePrefab>( prefabResource );
                    if( prefab )
                    {
                        if( auto actor = prefab->createActor() )
                        {
                            auto name = Path::getFileNameWithoutExtension( filePath );
                            actor->setName( name );

                            if( applicationManager->isPlaying() )
                            {
                                actor->setState( IGameActor::State::Play );
                            }
                            else
                            {
                                actor->setState( IGameActor::State::Edit );
                            }

                            return actor;
                        }
                    }
                }
            }
            else
            {
                return gameManager->createActor();
            }
        }
        break;
        case ActorType::Button:
        {
            auto actor = application->createButton( "Button", nullptr, "", false );

            return actor;
        }
        break;
        case ActorType::Camera:
        {
            auto actor = application->createDefaultCamera( false );
            return actor;
        }
        break;
        case ActorType::RenderTarget:
        {
            return ApplicationUtil::createRenderTarget( 512, 512, false );
        }
        break;
        case ActorType::Car:
        {
            auto actor = application->createDefaultVehicle( false );
            return actor;
        }
        break;
        case ActorType::Canvas:
        {
            if( auto actor = gameManager->createActor() )
            {
                static const auto name = String( "Canvas" );
                actor->setName( name );

                actor->addComponent<LayoutTransform>();
                actor->addComponent<Layout>();

                return actor;
            }
        }
        break;
        case ActorType::Checkbox:
        {
            auto actor = gameManager->createActor();

            auto name = String( "Checkbox" );
            actor->setName( name );

            auto size = Vector2F( 300, 100 );

            auto canvasTransform = actor->addComponent<LayoutTransform>();
            if( canvasTransform )
            {
                canvasTransform->setSize( size );
            }

            //actor->addComponent<scene::ImageComponent>();
            auto checkbox = actor->addComponent<Toggle>();
            checkbox->setCascadeInput( false );
            //checkbox->setCaption( "Checkbox" );

            auto material = actor->addComponent<Material>();
            if( material )
            {
                // material->setMainTexturePath("Rounded Filled 256px.png");
                // material->setTint(ColourF(0.0f, 0.5f, 0.0f, 1.0f));
            }

            return actor;
        }
        break;
        case ActorType::Constraint:
        {
            auto actor = gameManager->createActor();
            actor->setName( "Constraint" );
            actor->addComponent<Constraint>();

            return actor;
        }
        break;
        case ActorType::Cube:
        {
            auto actor = application->createDefaultCube( false );
            return actor;
        }
        break;
        case ActorType::CubeMesh:
        {
            auto actor = application->createDefaultCubeMesh( false );
            return actor;
        }
        break;
        case ActorType::Cubemap:
        {
            auto actor = application->createDefaultCubemap( false );
            return actor;
        }
        break;
        case ActorType::CubeGround:
        {
            auto actor = application->createDefaultCube( false );
            return actor;
        }
        break;
        case ActorType::DirectionalLight:
        {
            return application->createDirectionalLight( false );
        }
        break;
        case ActorType::Dropdown:
        {
            auto actor = UiUtil::createDropdown( "Dropdown", false );

            return actor;
        }
        break;
        case ActorType::Helicopter:
        {
            return createAircraftActor( "Helicopter", Vector3<real_Num>( 1.5f, 1.0f, 3.0f ),
                                        1200.0f, 1.25f, 2.0f );
        }
        break;
        case ActorType::Panel:
        {
            if( auto actor = gameManager->createActor() )
            {
                static const auto name = String( "Panel" );
                actor->setName( name );

                if( auto canvasTransform = actor->addComponent<LayoutTransform>() )
                {
                    auto size = Vector2F( 1920, 1080 );
                    canvasTransform->setSize( size );
                }

                if( auto image = actor->addComponent<scene::Image>() )
                {
                    auto panelColour = ColourF( 0.5f, 0.5f, 0.5f, 1.0f );
                    image->setColour( panelColour );
                }

                return actor;
            }
        }
        break;
        case ActorType::ParticleSystem:
            return createParticleSystemActor( "Particle System", actorType );
        case ActorType::ParticleSystemSmoke:
            return createParticleSystemActor( "Smoke Particle System", actorType );
        case ActorType::ParticleSystemSand:
            return createParticleSystemActor( "Sand Particle System", actorType );
        case ActorType::Plane:
        {
            return createAircraftActor( "Plane", Vector3<real_Num>( 1.25f, 0.75f, 4.0f ),
                                        1000.0f, 1.0f, 0.5f );
        }
        break;
        case ActorType::PlaneMesh:
        {
            return application->createDefaultPlane( false );
        }
        break;
        case ActorType::ProceduralScene:
        {
            auto actor = gameManager->createActor();

            auto name = String( "ProceduralScene" );
            actor->setName( name );

            //auto proceduralScene = actor->addComponent<scene::ProceduralScene>();
            //WP_ASSERT( proceduralScene );

            return actor;
        }
        break;
        case ActorType::PointLight:
        {
            return application->createPointLight( false );
        }
        break;
        case ActorType::PhysicsCube:
        {
            return application->createDefaultGround( false );
        }
        break;
        case ActorType::Skybox:
        {
            return application->createDefaultSky( false );
        }
        break;
        case ActorType::SimpleButton:
        {
            auto actor = gameManager->createActor();

            auto name = String( "Button" );
            actor->setName( name );

            auto size = Vector2F( 300, 100 );

            auto canvasTransform = actor->addComponent<LayoutTransform>();
            if( canvasTransform )
            {
                canvasTransform->setSize( size );
            }

            auto button = actor->addComponent<Button>();
            button->setCascadeInput( false );
            //button->setSimpleButton( true );
            //button->setCaption( "Button" );

            return actor;
        }
        break;
        case ActorType::Scrollbar:
        {
            return UiUtil::createScrollbar( "Scrollbar", nullptr, "", false );
        }
        break;
        case ActorType::ScrollbarVertical:
        {
            auto scrollbarActor =
                UiUtil::createScrollbarVertical( "Scrollbar Vertical", nullptr, "", false );
            if( auto scrollbar = scrollbarActor->getComponent<scene::ScrollBar>() )
            {
                scrollbar->setDirection( Direction::Vertical );
            }

            return scrollbarActor;
        }
        break;
        case ActorType::Scrollview:
        {
            return UiUtil::createScrollview( "Scrollview", nullptr, "", false );
        }
        break;
        case ActorType::Slider:
        {
            auto actor = application->createSlider( "Slider", nullptr, "", false );

            return actor;
        }
        break;
        case ActorType::SliderVertical:
        {
            auto sliderActor = UiUtil::createSliderVertical( "Slider Vertical", nullptr, "", false );
            if( auto slider = sliderActor->getComponent<scene::Slider>() )
            {
                slider->setDirection( Direction::Vertical );
            }

            return sliderActor;
        }
        break;
        case ActorType::TableLayout:
        {
            auto actor = gameManager->createActor();

            auto name = String( "TableLayout" );
            actor->setName( name );

            auto tableLayout = actor->addComponent<TableLayout>();
            WP_ASSERT( tableLayout );

            return actor;
        }
        break;
        case ActorType::TabView:
        {
            auto actor = gameManager->createActor();

            auto name = String( "TabView" );
            actor->setName( name );

            auto tabView = actor->addComponent<TabView>();
            WP_ASSERT( tabView );

            return actor;
        }
        break;
        case ActorType::Text:
        {
            auto actor = application->createText( "Text", nullptr, "", false );

            return actor;
        }
        break;
        case ActorType::ToggleButton:
        case ActorType::ToggleWithText:
        {
            auto actor = application->createToggle( "Toggle", nullptr, "", false );
            return actor;
        }
        break;
        case ActorType::Terrain:
        {
            auto actor = gameManager->createActor();
            actor->setStatic( true );

            auto name = String( "Terrain" );
            actor->setName( name );

            //auto terrainDirector = fb::make_ptr<Director>();
            //terrainDirector->saveToFile( "Terrain.resource" );

            auto terrain = actor->addComponent<TerrainSystem>();
            WP_ASSERT( terrain );

            auto collisionTerrain = actor->addComponent<CollisionTerrain>();
            WP_ASSERT( collisionTerrain );

            auto rigidbody = actor->addComponent<Rigidbody>();
            WP_ASSERT( rigidbody );

            auto material = actor->addComponent<Material>();
            if( material )
            {
                material->setMaterialPath( "DefaultTerrain.mat" );
            }

            return actor;
        }
        break;
        case ActorType::Vehicle:
        {
            auto actor = application->createDefaultVehicle( false );
            return actor;
        }
        break;
        default:
        {
            auto actor = gameManager->createActor();

            auto name = String( "Actor" );
            actor->setName( name );
            return actor;
        }
        }

        return nullptr;
    }

    SmartPtr<IGameActor> AddActorCmd::getActor() const
    {
        RecursiveMutex::ScopedLock lock( m_mutex );
        return m_actor;
    }

    void AddActorCmd::setActor( SmartPtr<IGameActor> actor )
    {
        RecursiveMutex::ScopedLock lock( m_mutex );
        m_actor = actor;
    }

    SmartPtr<IGameActor> AddActorCmd::getParent() const
    {
        RecursiveMutex::ScopedLock lock( m_mutex );
        return m_parent;
    }

    void AddActorCmd::setParent( SmartPtr<IGameActor> parent )
    {
        RecursiveMutex::ScopedLock lock( m_mutex );
        m_parent = parent;
    }

    AddActorCmd::ActorType AddActorCmd::getActorType() const
    {
        RecursiveMutex::ScopedLock lock( m_mutex );
        return m_actorType;
    }

    void AddActorCmd::setActorType( ActorType actorType )
    {
        RecursiveMutex::ScopedLock lock( m_mutex );
        m_actorType = actorType;
    }

    String AddActorCmd::getFilePath() const
    {
        RecursiveMutex::ScopedLock lock( m_mutex );
        return m_filePath;
    }

    void AddActorCmd::setFilePath( const String &filePath )
    {
        RecursiveMutex::ScopedLock lock( m_mutex );
        WP_ASSERT( !Path::isPathAbsolute( filePath ) );
        m_filePath = filePath;
    }
}  // namespace workphone::editor
