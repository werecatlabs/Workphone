#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/ApplicationUtil.hpp>
#include <Workphone/WorkphoneHeaders.hpp>
#include <Workphone/System/DebugUtil.hpp>

namespace workphone
{
    const String ApplicationUtil::meshbinExt = ".fbmeshbin";
    const String ApplicationUtil::materialExt = ".mat";
    const String ApplicationUtil::lightingStr = "lighting";
    const String ApplicationUtil::actorsStr = "actors";
    const String ApplicationUtil::builtinSceneExt = String( ".fbscene" );
    const String ApplicationUtil::builtinXmlSceneExt = String( ".fbscenexml" );
    const String ApplicationUtil::builtinBinarySceneExt = String( ".fbscenebin" );
    const String ApplicationUtil::builtinUsdSceneExt = String( ".usda" );

    Array<ISharedObject *> ApplicationUtil::getArray( ISharedObject *object )
    {
        Array<ISharedObject *> objects;
        objects.reserve( 32 );

        while( object )
        {
            if( object )
            {
                objects.push_back( object );
            }

            object = object->next.get();
        }

        return objects;
    }

    List<ISharedObject *> ApplicationUtil::getList( ISharedObject *object )
    {
        List<ISharedObject *> objects;

        while( object )
        {
            if( object )
            {
                objects.push_back( object );
            }

            object = object->next.get();
        }

        return objects;
    }

    Set<ISharedObject *> ApplicationUtil::getSet( ISharedObject *object )
    {
        Set<ISharedObject *> objects;

        while( object )
        {
            if( object )
            {
                objects.insert( object );
            }

            object = object->next.get();
        }

        return objects;
    }

    Deque<ISharedObject *> ApplicationUtil::getDeque( ISharedObject *object )
    {
        Deque<ISharedObject *> objects;

        while( object )
        {
            if( object )
            {
                objects.push_back( object );
            }

            object = object->next.get();
        }

        return objects;
    }

    void ApplicationUtil::push_back( ISharedObject *object, ISharedObject *item )
    {
        if( object && item )
        {
            while( object->next.get() )
            {
                object = object->next.get();
            }

            object->next = item;
        }
    }

    bool ApplicationUtil::erase( ISharedObject *object, ISharedObject *item )
    {
        ISharedObject *prev = nullptr;
        if( object && item )
        {
            if( object == item )
            {
                prev->next = nullptr;
                return true;
            }

            prev = object;
            object = object->next.get();
        }

        return false;
    }

    String ApplicationUtil::getObjectTypeName( SmartPtr<ISharedObject> object )
    {
        if( object )
        {
            return getObjectTypeName( object.get() );
        }

        return {};
    }

    String ApplicationUtil::getObjectTypeName( ISharedObject *object )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto typeManager = TypeManager::instance();
        WP_ASSERT( typeManager );

        if( object )
        {
            auto typeInfo = object->getTypeInfo();
            auto type = typeManager->getName( typeInfo );
            return type;
        }

        return {};
    }

    String ApplicationUtil::getPrefabPath( const String &filePath )
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto cachePath = applicationManager->getCachePath();

        static const auto prefabExt = String( ".prefab" );
        auto hash = StringUtil::getUUID( filePath );
        auto uuidStr = StringUtil::toString( hash );
        auto prefabFilePath = cachePath + Path::getFileName( filePath ) + "." + uuidStr + prefabExt;

        return prefabFilePath;
    }

    SmartPtr<scene::IGameActor> ApplicationUtil::loadMesh( const String &filePath )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto prefabManager = applicationManager->getPrefabManager();
        WP_ASSERT( prefabManager );

        if( prefabManager )
        {
            auto prefab = prefabManager->loadPrefab( filePath );
            if( prefab )
            {
                return prefab->createActor();
            }
        }

        return nullptr;
    }

    SmartPtr<ui::IUIMenuItem> ApplicationUtil::addMenuItem( SmartPtr<ui::IUIMenu> menu, s32 itemid,
                                                            const String &text, const String &help,
                                                            ui::IUIMenuItem::Type type )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto ui = applicationManager->getUI();
        WP_ASSERT( ui );

        auto menuItem = ui->addElementByType<ui::IUIMenuItem>();
        WP_ASSERT( menuItem );

        menuItem->setElementId( itemid );
        menuItem->setText( text );
        menuItem->setHelp( help );
        menuItem->setMenuItemType( type );

        menu->addMenuItem( menuItem );

        return menuItem;
    }

    SmartPtr<ui::IUIMenuItem> ApplicationUtil::addMenuSeparator( SmartPtr<ui::IUIMenu> menu )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto ui = applicationManager->getUI();
        WP_ASSERT( ui );

        auto menuItem = ui->addElementByType<ui::IUIMenuItem>();
        WP_ASSERT( menuItem );

        auto type = ui::IUIMenuItem::Type::Separator;
        menuItem->setMenuItemType( type );

        menu->addMenuItem( menuItem );

        return menuItem;
    }

    SmartPtr<ui::IUIElement> ApplicationUtil::setText( SmartPtr<ui::IUITreeNode> node,
                                                       const String &text )
    {
        if( node )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            if( auto ui = applicationManager->getUI() )
            {
                if( auto textElement = ui->addElementByType<ui::IUIText>() )
                {
                    textElement->setText( text );
                    node->addChild( textElement );

                    return textElement;
                }
            }
        }

        return nullptr;
    }

    SmartPtr<ui::IUIElement> ApplicationUtil::setImage( SmartPtr<ui::IUITreeNode> node,
                                                        const String &imagePath )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto ui = applicationManager->getUI();
        WP_ASSERT( ui );

        auto imageElement = ui->addElementByType<ui::IUIImage>();
        WP_ASSERT( imageElement );

        //textElement->setText( text );
        node->setNodeData( imageElement );

        return imageElement;
    }

    SmartPtr<ui::IUIElement> ApplicationUtil::getFirstChild( SmartPtr<ui::IUIElement> element )
    {
        if( element )
        {
            auto children = element->getChildren();

            if( !children.empty() )
            {
                return children.front();
            }
        }

        return nullptr;
    }

    String ApplicationUtil::getText( SmartPtr<ui::IUITreeNode> node )
    {
        auto children = node->getChildren();

        for( auto &child : children )
        {
            if( child->isDerived<ui::IUIText>() )
            {
                auto text = workphone::static_pointer_cast<ui::IUIText>( child );
                return text->getText();
            }
        }

        return {};
    }

    String ApplicationUtil::getComponentFactoryType( const String &factoryType )
    {
        Array<String> ignoreList;
        ignoreList.emplace_back( "UnityEngine.Transform" );

        if( std::find( ignoreList.begin(), ignoreList.end(), factoryType ) != ignoreList.end() )
        {
            return {};
        }

        Map<String, String> componentMap;
        componentMap["saracen.ApplicationManager"] = "SimulatorApplication";
        componentMap["FB.FBFiniteStateMachine"] = "FiniteStateMachine";
        componentMap["UnityEngine.Canvas"] = "CanvasComponent";

        componentMap["UnityEngine.RectTransform"] = "CanvasTransform";

        componentMap["UnityEngine.UI.Text"] = "TextComponent";
        componentMap["UnityEngine.UI.Image"] = "ImageComponent";
        componentMap["UnityEngine.UI.Button"] = "ButtonComponent";

        componentMap["Unitycoding.UIWidgets.TooltipTrigger"] = "Tooltip";
        componentMap["UI.Tables.TableLayout"] = "TableLayout";
        componentMap["UnityEngine.UI.VerticalLayoutGroup"] = "TableLayout";

        auto it = componentMap.find( factoryType );
        if( it != componentMap.end() )
        {
            return it->second;
        }

        return {};
    }

    void ApplicationUtil::convertCSharp( const String &srcPath, const String &dstPath )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto fileSystem = applicationManager->getFileSystem();
        WP_ASSERT( fileSystem );

        ScriptGenerator scriptGenerator;

        fileSystem->addFolder( srcPath, true );

        // if (fileSystem->isExistingFolder(csharpPath))
        {
            if( !fileSystem->isExistingFolder( dstPath ) )
            {
                fileSystem->createDirectories( dstPath );
            }

            fileSystem->addFolder( dstPath, true );

            scriptGenerator.setProjectPath( "WPApplication" );
            scriptGenerator.setReplaceFileName( "" );
            scriptGenerator.setReplacementFileName( "" );

            auto namespaceNames = Array<String>();
            namespaceNames.emplace_back( "fb" );
            scriptGenerator.setNamespaceNames( namespaceNames );

            scriptGenerator.convertCSharp( srcPath, dstPath );
        }
    }

    u32 ApplicationUtil::getEventPriority( SmartPtr<scene::IComponent> component )
    {
        if( component->isDerived<scene::Mesh>() )
        {
            return 500;
        }
        if( component->isDerived<scene::CollisionMesh>() )
        {
            return 400;
        }
        if( component->isDerived<scene::Rigidbody>() )
        {
            return 300;
        }
        if( component->isDerived<scene::MeshRenderer>() )
        {
            return 200;
        }
        if( component->isDerived<scene::Material>() )
        {
            return 100;
        }

        return 1000;
    }

    SmartPtr<scene::IGameActor> ApplicationUtil::createOverlayPanelTest()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        WP_ASSERT( scene );

        auto actor = sceneManager->createActor();
        scene->addActor( actor );
        scene->registerAllUpdates( actor );
        actor->setName( "Canvas" );

        actor->setLocalPosition( Vector3<real_Num>( 0, 0, 0 ) );
        actor->setPosition( Vector3<real_Num>( 0, 0, 0 ) );

        auto canvas = actor->addComponent<scene::Layout>();
        canvas->setReferenceSize( Vector2I( 1920, 1080 ) );

        auto actorImageParent = sceneManager->createActor();
        actor->addChild( actorImageParent );
        actorImageParent->setName( "Transform" );

        auto actorImageParentCanvasTransform = actorImageParent->addComponent<scene::LayoutTransform>();
        actorImageParentCanvasTransform->setSize( Vector2<real_Num>( 1920.0f, 1080.0f ) );

        auto actorImage = sceneManager->createActor();
        actorImageParent->addChild( actorImage );
        actorImage->setName( "Image" );

        auto actorImageCanvasTransform = actorImage->addComponent<scene::LayoutTransform>();
        actorImageCanvasTransform->setSize( Vector2<real_Num>( 1280.0f, 720.0f ) * 1.0f );

        auto image = actorImage->addComponent<scene::Image>();
        // image->setImagePath(mediaPath + "/checker.png");
        //image->setImagePath( "f40_hd.jpg" );

        auto materialComponent = actorImage->addComponent<scene::Material>();
        materialComponent->setMaterialPath( "DefaultUI.mat" );

        // auto material = materialComponent->getMaterial();
        // if (material)
        //{
        //	material->setMaterialType(MaterialType::UI);
        // }

        return actor;
    }

    SmartPtr<scene::IGameActor> ApplicationUtil::createOverlayTextTest()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        WP_ASSERT( scene );

        auto actor = sceneManager->createActor();
        scene->addActor( actor );
        scene->registerAllUpdates( actor );
        actor->setName( "Canvas" );

        actor->setLocalPosition( Vector3<real_Num>( 0, 0, 0 ) );
        actor->setPosition( Vector3<real_Num>( 0, 0, 0 ) );

        auto canvas = actor->addComponent<scene::Layout>();
        canvas->setReferenceSize( Vector2I( 1920, 1080 ) );

        auto actorImageParent = sceneManager->createActor();
        actor->addChild( actorImageParent );
        actorImageParent->setName( "Transform" );

        auto actorImageParentCanvasTransform = actorImageParent->addComponent<scene::LayoutTransform>();
        actorImageParentCanvasTransform->setSize( Vector2<real_Num>( 1920.0f, 1080.0f ) );

        auto actorImage = sceneManager->createActor();
        actorImageParent->addChild( actorImage );
        actorImage->setName( "Image" );

        auto actorImageCanvasTransform = actorImage->addComponent<scene::LayoutTransform>();
        actorImageCanvasTransform->setSize( Vector2<real_Num>( 1280.0f, 720.0f ) * 1.0f );

        auto actorText = sceneManager->createActor();
        actorImage->addChild( actorText );

        auto actorTextCanvasTransform = actorText->addComponent<scene::LayoutTransform>();
        if( actorTextCanvasTransform )
        {
            actorTextCanvasTransform->setSize( Vector2<real_Num>( 300, 200 ) );
        }

        auto textComponent = actorText->addComponent<scene::Text>();
        if( textComponent )
        {
            textComponent->setText( "test" );
        }

        // auto image = actorImage->addComponent<scene::ImageComponent>();
        ////image->setImagePath(mediaPath + "/checker.png");
        // image->setImagePath("f40_hd.jpg");

        auto materialComponent = actorImage->addComponent<scene::Material>();
        materialComponent->setMaterialPath( "DefaultUI.mat" );

        // auto material = materialComponent->getMaterial();
        // if (material)
        //{
        //	material->setMaterialType(MaterialType::UI);
        // }

        return actor;
    }

    SmartPtr<scene::IGameActor> ApplicationUtil::createOverlayButtonTest()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto actor = sceneManager->createActor();
        scene->addActor( actor );
        scene->registerAllUpdates( actor );
        actor->setName( "Canvas" );

        actor->setLocalPosition( Vector3<real_Num>( 0, 0, 0 ) );
        actor->setPosition( Vector3<real_Num>( 0, 0, 0 ) );

        auto canvas = actor->addComponent<scene::Layout>();
        canvas->setReferenceSize( Vector2I( 1920, 1080 ) );

        auto actorImageParent = sceneManager->createActor();
        actor->addChild( actorImageParent );
        actorImageParent->setName( "Transform" );

        auto actorImageParentCanvasTransform = actorImageParent->addComponent<scene::LayoutTransform>();
        actorImageParentCanvasTransform->setSize( Vector2<real_Num>( 1920.0f, 1080.0f ) );

        auto actorButton = sceneManager->createActor();
        actorImageParent->addChild( actorButton );
        actorButton->setName( "Button" );

        actorButton->setLocalPosition( Vector3<real_Num>( 300, 200, 0 ) );

        auto actorImageCanvasTransform = actorButton->addComponent<scene::LayoutTransform>();
        if( actorImageCanvasTransform )
        {
            actorImageCanvasTransform->setSize( Vector2<real_Num>( 300, 100 ) );
        }

        auto actorButtonImageComponent = actorButton->addComponent<scene::Image>();
        if( actorButtonImageComponent )
        {
            //actorButtonImageComponent->setImagePath( "checker.png" );
        }

        actorButton->addComponent<scene::Button>();

        auto material = actorButton->addComponent<scene::Material>();
        if( material )
        {
            // material->setMainTexturePath("Rounded Filled 256px.png");
            // material->setTint(ColourF(0.0f, 0.5f, 0.0f, 1.0f));
        }

        auto actorText = sceneManager->createActor();
        actorButton->addChild( actorText );

        actorText->addComponent<scene::LayoutTransform>();
        auto text = actorText->addComponent<scene::Text>();
        text->setText( "Button" );

        auto textName = String( "Text" );
        actorText->setName( textName );

        //auto image = actorImage->addComponent<scene::ImageComponent>();
        ////image->setImagePath(mediaPath + "/checker.png");
        // image->setImagePath("f40_hd.jpg");

        auto materialComponent = actorButton->addComponent<scene::Material>();
        materialComponent->setMaterialPath( "DefaultUI.mat" );

        // auto material = materialComponent->getMaterial();
        // if (material)
        //{
        //	material->setMaterialType(MaterialType::UI);
        // }

        return actor;
    }

    void ApplicationUtil::createFactories()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        FactoryUtil::addFactory<Properties>();

        FactoryUtil::addFactory<scene::GameActor>();
        FactoryUtil::addFactory<Director>();

        FactoryUtil::addFactory<scene::SphericalCameraController>();
        FactoryUtil::addFactory<scene::ThirdPersonCameraController>();
        FactoryUtil::addFactory<scene::VehicleCameraController>();

        FactoryUtil::addFactory<scene::Light>();

        FactoryUtil::addFactory<scene::Camera>();
        FactoryUtil::addFactory<scene::CarController>();
        FactoryUtil::addFactory<scene::Constraint>();

        FactoryUtil::addFactory<scene::CollisionBox>();
        FactoryUtil::addFactory<scene::CollisionMesh>();
        FactoryUtil::addFactory<scene::CollisionPlane>();
        FactoryUtil::addFactory<scene::CollisionSphere>();
        FactoryUtil::addFactory<scene::CollisionTerrain>();

        FactoryUtil::addFactory<scene::FiniteStateMachine>();

        FactoryUtil::addFactory<scene::Material>();
        FactoryUtil::addFactory<scene::Mesh>();
        FactoryUtil::addFactory<scene::MeshRenderer>();

        FactoryUtil::addFactory<scene::ParticleSystem>();

        FactoryUtil::addFactory<scene::Rigidbody>();
        FactoryUtil::addFactory<scene::Skybox>();

        FactoryUtil::addFactory<scene::TerrainBlendMap>();
        FactoryUtil::addFactory<scene::TerrainLayer>();
        FactoryUtil::addFactory<scene::TerrainSystem>();

        FactoryUtil::addFactory<scene::Script>();

        FactoryUtil::addFactory<scene::WheelController>();

        //FactoryUtil::addFactory<ui::StartMenu>();
        //FactoryUtil::addFactory<ui::SystemSettings>();

        FactoryUtil::addFactory<scene::Button>();
        FactoryUtil::addFactory<scene::Layout>();
        FactoryUtil::addFactory<scene::LayoutTransform>();
        FactoryUtil::addFactory<scene::Image>();
        FactoryUtil::addFactory<scene::InputField>();
        FactoryUtil::addFactory<scene::Text>();

        FactoryUtil::addFactory<scene::Transform>();

        FactoryUtil::addFactory<StateMessageVector3>();
        FactoryUtil::addFactory<StateMessageVector4>();
        FactoryUtil::addFactory<StateMessageUIntValue>();
        FactoryUtil::addFactory<StateMessageIntValue>();
        FactoryUtil::addFactory<StateMessageVisible>();

        FactoryUtil::addFactory<MaterialPassStateData>();
        FactoryUtil::addFactory<PhysicsSceneState>();

        const auto size = 32;

        factoryManager->setPoolSizeByType<Properties>( size );

        factoryManager->setPoolSizeByType<scene::GameActor>( size );
        factoryManager->setPoolSizeByType<scene::Transform>( size );

        factoryManager->setPoolSizeByType<scene::Material>( size );
        factoryManager->setPoolSizeByType<scene::Mesh>( size );
        factoryManager->setPoolSizeByType<scene::MeshRenderer>( size );
        factoryManager->setPoolSizeByType<scene::CollisionBox>( size );
        factoryManager->setPoolSizeByType<scene::Rigidbody>( size );

        const auto messagePoolSize = 12;

        factoryManager->setPoolSizeByType<StateMessageVector3>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessageVector4>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessageUIntValue>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessageIntValue>( messagePoolSize );
        factoryManager->setPoolSizeByType<StateMessageVisible>( messagePoolSize );

        auto factories = factoryManager->getFactories();
        for( auto factory : factories )
        {
            factory->allocatePoolData();
        }
    }

    void ApplicationUtil::createDefaultMaterials()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();

        auto resourceDatabase = applicationManager->getResourceDatabase();
        if( resourceDatabase )
        {
            auto filePath = String( "default" );
            auto result = resourceDatabase->createOrRetrieveByType<render::IMaterial>( filePath );
            if( result.first && result.second )
            {
                auto material = result.first;
                graphicsSystem->loadObject( material );
            }
        }
        else
        {
            WP_LOG_ERROR( "Resource database is null." );
        }
    }

    void ApplicationUtil::createDefaultFont()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto fileSystem = applicationManager->getFileSystem();
        WP_ASSERT( fileSystem );

        auto resourceDatabase = applicationManager->getResourceDatabase();
        WP_ASSERT( resourceDatabase );

        auto director = workphone::make_ptr<Director>();
        director->load( nullptr );

        auto fontFileName = String( "cuckoo.ttf" );

        auto directorProperties = director->getProperties();
        directorProperties->setProperty( "font_type", String( "arial" ) );
        directorProperties->setProperty( "font_source", fontFileName );
        directorProperties->setProperty( "font_size", 12 );
        directorProperties->setProperty( "font_resolution", 96 );

        const auto fontName = String( "default" );

        auto result =
            resourceDatabase->createOrRetrieveFromDirector<render::IFont>( fontName, director );
        if( result.first )
        {
            auto font = result.first;
            font->setProperties( directorProperties );
            font->load( nullptr );
        }
    }

    bool ApplicationUtil::isSupportedFont( const String &filePath )
    {
        auto fileTypes = Array<String>();
        fileTypes.reserve( 4 );

        fileTypes.emplace_back( ".ttf" );
        fileTypes.emplace_back( ".otf" );
        fileTypes.emplace_back( ".woff" );
        fileTypes.emplace_back( ".woff2" );

        auto ext = Path::getFileExtension( filePath );
        ext = StringUtil::make_lower( ext );

        for( auto &type : fileTypes )
        {
            if( ext == type )
            {
                return true;
            }
        }

        return false;
    }

    bool ApplicationUtil::isSupportedMesh( const String &filePath )
    {
        auto fileTypes = Array<String>();
        fileTypes.reserve( 12 );

        fileTypes.emplace_back( ".fbx" );
        fileTypes.emplace_back( ".obj" );
        fileTypes.emplace_back( ".dae" );
        fileTypes.emplace_back( ".3ds" );
        fileTypes.emplace_back( ".fbmeshbin" );
        fileTypes.emplace_back( ".gltf" );
        fileTypes.emplace_back( ".glb" );
        fileTypes.emplace_back( ".ngp" );

        auto ext = Path::getFileExtension( filePath );
        auto extLower = StringUtil::make_lower( ext );

        auto it = std::find( fileTypes.begin(), fileTypes.end(), extLower );
        return it != fileTypes.end();
    }

    bool ApplicationUtil::isSupportedTexture( const String &filePath )
    {
        auto fileTypes = Array<String>();
        fileTypes.reserve( 5 );

        fileTypes.emplace_back( ".png" );
        fileTypes.emplace_back( ".jpg" );
        fileTypes.emplace_back( ".jpeg" );
        fileTypes.emplace_back( ".tga" );
        fileTypes.emplace_back( ".bmp" );

        auto ext = Path::getFileExtension( filePath );
        ext = StringUtil::make_lower( ext );

        for( auto type : fileTypes )
        {
            if( ext == type )
            {
                return true;
            }
        }

        return false;
    }

    bool ApplicationUtil::isSupportedSound( const String &filePath )
    {
        auto fileTypes = Array<String>();
        fileTypes.reserve( 5 );

        fileTypes.emplace_back( ".wav" );
        fileTypes.emplace_back( ".ogg" );
        fileTypes.emplace_back( ".mp3" );

        auto ext = Path::getFileExtension( filePath );
        ext = StringUtil::make_lower( ext );

        for( auto type : fileTypes )
        {
            if( ext == type )
            {
                return true;
            }
        }

        return false;
    }

    Array<String> ApplicationUtil::getSupportedMeshFormats()
    {
        return { ".fbx", ".obj", ".dae", ".gltf", ".glb", ".blend", ".ngp" };
    }

    Array<String> ApplicationUtil::getSupportedTextureFormats()
    {
        return { ".png", ".jpg", ".jpeg", ".tga", ".bmp", ".dds", ".hdr", ".tif", ".tiff" };
    }

    Array<String> ApplicationUtil::getSupportedVideoFormats()
    {
        return { ".mp4", ".avi", ".mov", ".mkv", ".webm" };
    }

    Array<String> ApplicationUtil::getSupportedFontFormats()
    {
        return { ".ttf", ".otf", ".woff", ".woff2" };
    }

    Array<String> ApplicationUtil::getSupportedPrefabFormats()
    {
        return { ".prefab", ".json" };
    }

    Array<String> ApplicationUtil::getSupportedSceneFormats()
    {
        return { ".scene", ".json", ".usda" };
    }

    Array<String> ApplicationUtil::getSupportedScriptFormats()
    {
        return { ".cs", ".lua", ".py" };
    }

    Array<String> ApplicationUtil::getSupportedShaderFormats()
    {
        return { ".glsl", ".hlsl", ".shader", ".compute" };
    }

    Array<String> ApplicationUtil::getSupportedDatabaseFormats()
    {
        return { ".db", ".sqlite", ".json" };
    }

    Array<String> ApplicationUtil::getSupportedAudioFormats()
    {
        return { ".wav", ".ogg", ".mp3", ".flac", ".aiff" };
    }

    SmartPtr<scene::IGameActor> ApplicationUtil::createDefaultSky( bool addToScene )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto actor = sceneManager->createActor();

        auto name = String( "Skybox" );
        actor->setName( name );

        auto skybox = actor->addComponent<scene::Skybox>();

        auto material = actor->addComponent<scene::Material>();
        if( material )
        {
            material->setMaterialPath( "DefaultSkybox.mat" );
        }

        if( addToScene )
        {
            scene->addActor( actor );
        }

        auto uniqueId = 0;  // StringUtil::getHash(uuid);
        return actor;
    }

    SmartPtr<scene::IGameActor> ApplicationUtil::createCamera( bool addToScene )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        auto scene = sceneManager->getCurrentScene();

        auto actor = sceneManager->createActor();

        auto name = String( "Camera" );
        actor->setName( name );

        // auto id = StringUtil::getHash(name);
        // actor->setId(id);

        // auto uuid = StringUtil::getUUID();
        // actor->setUUID(uuid);

        auto c = actor->addComponent<scene::Camera>();
        WP_ASSERT( c );

        if( addToScene )
        {
            scene->addActor( actor );
            scene->registerAllUpdates( actor );
        }

        auto uniqueId = 0;  // StringUtil::getHash(uuid);
        return actor;
    }

    SmartPtr<scene::IGameActor> ApplicationUtil::createRenderTarget( u32 width, u32 height,
                                                                     bool addToScene )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManagerPtr();
        WP_ASSERT( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        auto actor = sceneManager->createActor();
        actor->setName( "Render Target" );

        auto renderTexture = actor->addComponent<scene::RenderTexture>();
        WP_ASSERT( renderTexture );
        renderTexture->setWidth( std::max( width, 1u ) );
        renderTexture->setHeight( std::max( height, 1u ) );
        renderTexture->setTextureName( "RenderTarget_" + StringUtil::getUUID() );

        if( addToScene && scene )
        {
            scene->addActor( actor );
            scene->registerAllUpdates( actor );
        }

        return actor;
    }

    SmartPtr<scene::IGameActor> ApplicationUtil::createDefaultCubemap( bool addToScene )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );
            WP_ASSERT( applicationManager->isValid() );

            auto sceneManager = applicationManager->getGameManager();
            WP_ASSERT( sceneManager );
            WP_ASSERT( sceneManager->isValid() );

            auto scene = sceneManager->getCurrentScene();
            WP_ASSERT( scene );
            WP_ASSERT( scene->isValid() );

            auto actor = sceneManager->createActor();
            WP_ASSERT( actor );

            auto name = String( "Cubemap" );
            actor->setName( name );

            auto c = actor->addComponent<scene::CollisionBox>();
            WP_ASSERT( c );

            auto cubemap = actor->addComponent<scene::Cubemap>();
            WP_ASSERT( cubemap );

            auto meshComponent = actor->addComponent<scene::Mesh>();
            WP_ASSERT( meshComponent );
            meshComponent->setMeshPath( "cube_internal.fbmeshbin" );

            auto meshRenderer = actor->addComponent<scene::MeshRenderer>();
            WP_ASSERT( meshRenderer );

            auto material = actor->addComponent<scene::Material>();
            WP_ASSERT( material );
            WP_ASSERT( material->isValid() );

            if( material )
            {
                material->setMaterialPath( "Standard.mat" );
            }

            if( addToScene )
            {
                scene->addActor( actor );
                WP_ASSERT( scene->isValid() );
            }

            return actor;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    SmartPtr<scene::IGameActor> ApplicationUtil::createDefaultCube( bool addToScene )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );
            WP_ASSERT( applicationManager->isValid() );

            auto sceneManager = applicationManager->getGameManager();
            WP_ASSERT( sceneManager );
            WP_ASSERT( sceneManager->isValid() );

            auto scene = sceneManager->getCurrentScene();
            WP_ASSERT( scene );
            WP_ASSERT( scene->isValid() );

            auto actor = sceneManager->createActor();
            WP_ASSERT( actor );

            const auto name = String( "Cube" );
            actor->setName( name );

            auto c = actor->addComponent<scene::CollisionBox>();
            WP_ASSERT( c );

            auto rigidbody = actor->addComponent<scene::Rigidbody>();
            WP_ASSERT( rigidbody );

            auto meshComponent = actor->addComponent<scene::Mesh>();
            WP_ASSERT( meshComponent );
            meshComponent->setMeshPath( "cube_internal.fbmeshbin" );

            auto meshRenderer = actor->addComponent<scene::MeshRenderer>();
            WP_ASSERT( meshRenderer );

            auto material = actor->addComponent<scene::Material>();
            WP_ASSERT( material );
            WP_ASSERT( material->isValid() );

            if( material )
            {
                material->setMaterialPath( "Standard.mat" );
            }

            if( addToScene )
            {
                scene->registerAllUpdates( actor );
                scene->addActor( actor );
                WP_ASSERT( scene->isValid() );
            }

            return actor;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    SmartPtr<scene::IGameActor> ApplicationUtil::createDefaultCubeMesh( bool addToScene )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );
            WP_ASSERT( applicationManager->isValid() );

            auto sceneManager = applicationManager->getGameManager();
            WP_ASSERT( sceneManager );
            WP_ASSERT( sceneManager->isValid() );

            auto scene = sceneManager->getCurrentScene();
            WP_ASSERT( scene );
            WP_ASSERT( scene->isValid() );

            auto actor = sceneManager->createActor();
            WP_ASSERT( actor );

            const auto name = String( "Cube Mesh" );
            actor->setName( name );

            auto c = actor->addComponent<scene::CollisionMesh>();
            WP_ASSERT( c );

            auto rigidbody = actor->addComponent<scene::Rigidbody>();
            WP_ASSERT( rigidbody );

            auto meshComponent = actor->addComponent<scene::Mesh>();
            WP_ASSERT( meshComponent );
            meshComponent->setMeshPath( "cube_internal.fbmeshbin" );

            auto meshRenderer = actor->addComponent<scene::MeshRenderer>();
            WP_ASSERT( meshRenderer );

            auto material = actor->addComponent<scene::Material>();
            WP_ASSERT( material );
            WP_ASSERT( material->isValid() );

            if( material )
            {
                material->setMaterialPath( "Standard.mat" );
            }

            if( addToScene )
            {
                scene->addActor( actor );
                WP_ASSERT( scene->isValid() );
            }

            return actor;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    SmartPtr<scene::IGameActor> ApplicationUtil::createDefaultGround( bool addToScene )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        WP_ASSERT( scene );

        auto actor = sceneManager->createActor();
        actor->setStatic( true );

        auto name = String( "Ground" );
        actor->setName( name );

        auto collisionBox = actor->addComponent<scene::CollisionBox>();
        WP_ASSERT( collisionBox );
        if( collisionBox )
        {
            collisionBox->setExtents( Vector3<real_Num>::unit() * 1.0f );
        }

        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        WP_ASSERT( rigidbody );

        auto meshComponent = actor->addComponent<scene::Mesh>();
        WP_ASSERT( meshComponent );
        meshComponent->setMeshPath( "cube_internal.fbmeshbin" );

        auto meshRenderer = actor->addComponent<scene::MeshRenderer>();
        WP_ASSERT( meshRenderer );

        //auto material = actor->addComponent<scene::Material>();
        //WP_ASSERT( material );

        //if( material )
        //{
        //    material->setMaterialPath( "default" );
        //    material->updateMaterial();
        //}

        auto groundPosition = Vector3<real_Num>::unitY() * static_cast<real_Num>( -250.0 );
        // groundPosition += Vector3<real_Num>::unitZ() * -250.0f;

        auto groundScale = Vector3<real_Num>::unit() * static_cast<real_Num>( 500.0 );

        auto transform = actor->getTransform();
        if( transform )
        {
            transform->setLocalPosition( groundPosition );
            transform->setLocalScale( groundScale );
        }

        if( addToScene )
        {
            scene->addActor( actor );
            WP_ASSERT( scene->isValid() );
        }

        return actor;
    }

    SmartPtr<scene::IGameActor> ApplicationUtil::createDefaultTerrain( bool addToScene )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        WP_ASSERT( scene );

        auto actor = sceneManager->createActor();
        actor->setStatic( true );

        auto name = String( "Terrain" );
        actor->setName( name );

        // auto id = StringUtil::getHash(name);
        // actor->setId(id);

        // auto uuid = StringUtil::getUUID();
        // actor->setUUID(uuid);

        // todo remove
        // for test the renderer
        auto terrain = actor->addComponent<scene::TerrainSystem>();

        auto terrainCollision = actor->addComponent<scene::CollisionTerrain>();

        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        WP_ASSERT( rigidbody );

        if( addToScene )
        {
            scene->registerAllUpdates( actor );
            scene->addActor( actor );
        }

        return actor;
    }

    SmartPtr<scene::IGameActor> ApplicationUtil::createDefaultConstraint()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        WP_ASSERT( scene );

        auto actor = sceneManager->createActor();
        actor->setStatic( false );

        auto name = String( "Constraint" );
        actor->setName( name );

        // auto id = StringUtil::getHash(name);
        // actor->setId(id);

        // auto uuid = StringUtil::getUUID();
        // actor->setUUID(uuid);

        auto terrain = actor->addComponent<scene::Constraint>();

        scene->addActor( actor );
        scene->registerAllUpdates( actor );

        return actor;
    }

    SmartPtr<scene::IGameActor> ApplicationUtil::createDirectionalLight( bool addToScene )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        WP_ASSERT( scene );

        auto actor = sceneManager->createActor();

        auto name = String( "Directional Light" );
        actor->setName( name );

        // auto id = StringUtil::getHash(name);
        // actor->setId(id);

        // auto uuid = StringUtil::getUUID();
        // actor->setUUID(uuid);

        // todo remove
        // for test the renderer
        auto light = actor->addComponent<scene::Light>();
        if( light )
        {
            light->setLightType( LightTypes::LT_DIRECTIONAL );
        }

        if( addToScene )
        {
            scene->addActor( actor );
            scene->registerAllUpdates( actor );
        }

        auto uniqueId = 0;  // StringUtil::getHash(uuid);
        return actor;
    }

    SmartPtr<scene::IGameActor> ApplicationUtil::createPointLight( bool addToScene )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        WP_ASSERT( scene );

        auto actor = sceneManager->createActor();

        auto name = String( "Main Light" );
        actor->setName( name );

        // auto id = StringUtil::getHash(name);
        // actor->setId(id);

        // auto uuid = StringUtil::getUUID();
        // actor->setUUID(uuid);

        // todo remove
        // for test the renderer
        auto light = actor->addComponent<scene::Light>();
        if( light )
        {
            light->setLightType( LightTypes::LT_POINT );
        }

        if( addToScene )
        {
            scene->addActor( actor );
            scene->registerAllUpdates( actor );
        }

        auto uniqueId = 0;  // StringUtil::getHash(uuid);
        return actor;
    }

    SmartPtr<scene::IGameActor> ApplicationUtil::createDefaultPlane( bool addToScene )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        WP_ASSERT( scene );

        auto actor = sceneManager->createActor();

        auto name = String( "Plane" );
        actor->setName( name );

        auto localOrientation = Quaternion<real_Num>::angleAxis(
            Math<real_Num>::DegToRad( static_cast<real_Num>( -90.0 ) ), Vector3<real_Num>::unitX() );
        actor->setLocalOrientation( localOrientation );
        actor->setLocalScale( Vector3<real_Num>::unit() * static_cast<real_Num>( 100.0 ) );

        // auto id = StringUtil::getHash(name);
        // actor->setId(id);

        // auto uuid = StringUtil::getUUID();
        // actor->setUUID(uuid);

        auto c = actor->addComponent<scene::CollisionBox>();
        WP_ASSERT( c );

        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        WP_ASSERT( rigidbody );

        auto meshComponent = actor->addComponent<scene::Mesh>();
        WP_ASSERT( meshComponent );
        meshComponent->setMeshPath( "plane.fbmeshbin" );

        auto meshRenderer = actor->addComponent<scene::MeshRenderer>();

        auto material = actor->addComponent<scene::Material>();
        WP_ASSERT( material );

        if( material )
        {
            material->setMaterialPath( "Standard.mat" );
            material->updateMaterial();
        }

        if( addToScene )
        {
            scene->addActor( actor );
            scene->registerAllUpdates( actor );
        }

        auto uniqueId = 0;  // StringUtil::getHash(uuid);
        return actor;
    }

    SmartPtr<scene::IGameActor> ApplicationUtil::createDefaultVehicle( bool addToScene )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto prefabManager = applicationManager->getPrefabManager();
        WP_ASSERT( prefabManager );

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        WP_ASSERT( scene );

        f32 wheelBase = 2.530f;
        f32 length = 4.405f;
        f32 width = 1.810f;
        f32 height = 1.170f;
        f32 mass = 1370.0f;

        auto actor = sceneManager->createActor();

        auto name = String( "Vehicle" );
        actor->setName( name );

        // auto id = StringUtil::getHash(name);
        // actor->setId(id);

        // auto uuid = StringUtil::getUUID();
        // actor->setUUID(uuid);

        auto c = actor->addComponent<scene::CollisionBox>();
        WP_ASSERT( c );

        auto bodyDimensions = Vector3<real_Num>( length, height, width ) * 0.5f;
        c->setExtents( bodyDimensions );

        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        WP_ASSERT( rigidbody );

        auto vehicle = actor->addComponent<scene::CarController>();
        WP_ASSERT( vehicle );

        auto cubeMeshPath = String( "cube_internal.fbmeshbin" );

        auto meshComponent = actor->addComponent<scene::Mesh>();
        WP_ASSERT( meshComponent );
        meshComponent->setMeshPath( cubeMeshPath );

        auto meshRenderer = actor->addComponent<scene::MeshRenderer>();
        WP_ASSERT( meshRenderer );

        auto material = actor->addComponent<scene::Material>();
        WP_ASSERT( material );

        if( material )
        {
            material->setMaterialPath( "Standard.mat" );
            material->updateMaterial();
        }

        auto meshActor = sceneManager->createActor();

        auto meshActorName = String( "mesh" );
        meshActor->setName( meshActorName );

        actor->addChild( meshActor );

        Array<Vector3<real_Num>> wheelPositions;
        wheelPositions.resize( 4 );

        auto wheelOffset = -1.0f;
        wheelPositions[static_cast<u32>( scene::CarController::Wheels::FRONT_LEFT )] =
            Vector3<real_Num>( -width / 2.0f, wheelOffset, wheelBase / 2.0f );
        wheelPositions[static_cast<u32>( scene::CarController::Wheels::FRONT_RIGHT )] =
            Vector3<real_Num>( width / 2.0f, wheelOffset, wheelBase / 2.0f );
        wheelPositions[static_cast<u32>( scene::CarController::Wheels::REAR_LEFT )] =
            Vector3<real_Num>( -width / 2.0f, wheelOffset, -wheelBase / 2.0f );
        wheelPositions[static_cast<u32>( scene::CarController::Wheels::REAR_RIGHT )] =
            Vector3<real_Num>( width / 2.0f, wheelOffset, -wheelBase / 2.0f );

        auto wheelMeshSize = Vector3<real_Num>( 0.1f, 0.3f, 0.3f );

        for( u32 i = 0; i < 4; ++i )
        {
            auto wheelMeshActor = sceneManager->createActor();

            auto wheelMeshActorName = String( "wheel_" ) + StringUtil::toString( i );
            wheelMeshActor->setName( wheelMeshActorName );

            auto wheelMeshComponent = wheelMeshActor->addComponent<scene::Mesh>();
            WP_ASSERT( wheelMeshComponent );
            wheelMeshComponent->setMeshPath( cubeMeshPath );

            auto meshRenderer = wheelMeshActor->addComponent<scene::MeshRenderer>();
            WP_ASSERT( meshRenderer );

            auto material = wheelMeshActor->addComponent<scene::Material>();
            WP_ASSERT( material );

            meshActor->addChild( wheelMeshActor );

            wheelMeshActor->setLocalPosition( wheelPositions[i] );
            wheelMeshActor->setLocalScale( wheelMeshSize );
        }

        auto dynamicsActor = sceneManager->createActor();

        auto dynamicsName = String( "dynamics" );
        dynamicsActor->setName( dynamicsName );

        // auto dynamicsId = StringUtil::getHash(dynamicsName);
        // dynamicsActor->setId(dynamicsId);

        // auto dynamicsUuid = StringUtil::getUUID();
        // dynamicsActor->setUUID(dynamicsUuid);

        actor->addChild( dynamicsActor );

        for( u32 i = 0; i < 4; ++i )
        {
            auto wheelActor = sceneManager->createActor();
            dynamicsActor->addChild( wheelActor );

            auto wheelName = String( "Wheel" );
            wheelActor->setName( wheelName );

            auto wheel = wheelActor->addComponent<scene::WheelController>();
            WP_ASSERT( wheel );
        }

        // auto prefab = prefabManager->loadPrefab("f40.fbx");
        // if (prefab)
        //{
        //	auto vehicleMesh = prefab->createActor();
        //
        //	//auto vehicleMeshTransform = vehicleMesh->getLocalTransform();
        //	//if (vehicleMeshTransform)
        //	//{
        //	//	vehicleMeshTransform->setScale(Vector3<real_Num>::unit() * 0.0254f);
        //	//}

        //	vehicleMesh->setName("F40");
        //	actor->addChild(vehicleMesh);
        //}

        auto groundPosition = Vector3<real_Num>::unitY() * static_cast<real_Num>( 5.0 );
        auto groundScale = Vector3<real_Num>::unit() * static_cast<real_Num>( 1.0 );

        auto transform = actor->getTransform();
        if( transform )
        {
            transform->setLocalPosition( groundPosition );
            transform->setLocalScale( groundScale );
        }

        actor->updateTransform();

        if( addToScene )
        {
            scene->addActor( actor );
            scene->registerAllUpdates( actor );
        }

        return actor;
    }

    SmartPtr<scene::IGameActor> ApplicationUtil::createDefaultCar( bool addToScene )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto prefabManager = applicationManager->getPrefabManager();
        WP_ASSERT( prefabManager );

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        WP_ASSERT( scene );

        auto actor = sceneManager->createActor();

        auto name = String( "Car" );
        actor->setName( name );

        // auto id = StringUtil::getHash(name);
        // actor->setId(id);

        // auto uuid = StringUtil::getUUID();
        // actor->setUUID(uuid);

        auto c = actor->addComponent<scene::CollisionBox>();
        WP_ASSERT( c );

        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        WP_ASSERT( rigidbody );

        auto vehicle = actor->addComponent<scene::CarController>();
        WP_ASSERT( vehicle );

        auto cubeMeshPath = String( "cube_internal.fbmeshbin" );

        auto meshComponent = actor->addComponent<scene::Mesh>();
        WP_ASSERT( meshComponent );
        meshComponent->setMeshPath( cubeMeshPath );

        auto meshRenderer = actor->addComponent<scene::MeshRenderer>();
        WP_ASSERT( meshRenderer );

        auto material = actor->addComponent<scene::Material>();
        WP_ASSERT( material );

        if( material )
        {
            material->setMaterialPath( "Standard.mat" );
            material->updateMaterial();
        }

        auto dynamicsActor = sceneManager->createActor();

        auto dynamicsName = String( "dynamics" );
        dynamicsActor->setName( dynamicsName );

        // auto dynamicsId = StringUtil::getHash(dynamicsName);
        // dynamicsActor->setId(dynamicsId);

        // auto dynamicsUuid = StringUtil::getUUID();
        // dynamicsActor->setUUID(dynamicsUuid);

        actor->addChild( dynamicsActor );

        for( u32 i = 0; i < 4; ++i )
        {
            auto wheelActor = sceneManager->createActor();
            dynamicsActor->addChild( wheelActor );

            auto wheelName = String( "Wheel" );
            wheelActor->setName( wheelName );

            auto wheel = wheelActor->addComponent<scene::WheelController>();
            WP_ASSERT( wheel );
        }

        auto rigActor = sceneManager->createActor();

        auto rigName = String( "rig" );
        rigActor->setName( rigName );
        actor->addChild( rigActor );

        //auto prefab = prefabManager->loadPrefab( "f40.fbx" );
        //if( prefab )
        //{
        //    auto vehicleMesh = prefab->createActor();

        //    // auto vehicleMeshTransform = vehicleMesh->getLocalTransform();
        //    // if (vehicleMeshTransform)
        //    //{
        //    //	vehicleMeshTransform->setScale(Vector3<real_Num>::unit() * 0.0254f);
        //    // }

        //    vehicleMesh->setName( "F40" );
        //    rigActor->addChild( vehicleMesh );
        //}

        auto groundPosition = Vector3<real_Num>::unitY() * 5.0f;
        auto groundScale = Vector3<real_Num>::unit() * 1.0f;

        auto transform = actor->getTransform();
        if( transform )
        {
            transform->setLocalPosition( groundPosition );
            transform->setLocalScale( groundScale );
        }

        actor->updateTransform();

        if( addToScene )
        {
            scene->addActor( actor );
            scene->registerAllUpdates( actor );
        }

        return actor;
    }

    SmartPtr<scene::IGameActor> ApplicationUtil::createDefaultTruck( bool addToScene )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto prefabManager = applicationManager->getPrefabManager();
        WP_ASSERT( prefabManager );

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        WP_ASSERT( scene );

        auto actor = sceneManager->createActor();

        auto name = String( "Truck" );
        actor->setName( name );

        // auto id = StringUtil::getHash(name);
        // actor->setId(id);

        // auto uuid = StringUtil::getUUID();
        // actor->setUUID(uuid);

        auto c = actor->addComponent<scene::CollisionBox>();
        WP_ASSERT( c );

        auto rigidbody = actor->addComponent<scene::Rigidbody>();
        WP_ASSERT( rigidbody );

        auto vehicle = actor->addComponent<scene::CarController>();
        WP_ASSERT( vehicle );

        auto meshComponent = actor->addComponent<scene::Mesh>();
        WP_ASSERT( meshComponent );
        meshComponent->setMeshPath( "cube_internal.fbmeshbin" );

        auto meshRenderer = actor->addComponent<scene::MeshRenderer>();
        WP_ASSERT( meshRenderer );

        auto material = actor->addComponent<scene::Material>();
        WP_ASSERT( material );

        if( material )
        {
            material->setMaterialPath( "Standard.mat" );
            material->updateMaterial();
        }

        auto dynamicsActor = sceneManager->createActor();

        auto dynamicsName = String( "dynamics" );
        dynamicsActor->setName( dynamicsName );

        // auto dynamicsId = StringUtil::getHash(dynamicsName);
        // dynamicsActor->setId(dynamicsId);

        // auto dynamicsUuid = StringUtil::getUUID();
        // dynamicsActor->setUUID(dynamicsUuid);

        actor->addChild( dynamicsActor );

        for( u32 i = 0; i < 4; ++i )
        {
            auto wheelActor = sceneManager->createActor();
            dynamicsActor->addChild( wheelActor );

            auto wheelName = String( "Wheel" );
            wheelActor->setName( wheelName );

            auto wheel = wheelActor->addComponent<scene::WheelController>();
            WP_ASSERT( wheel );
        }

        auto rigActor = sceneManager->createActor();

        auto rigName = String( "rig" );
        rigActor->setName( rigName );
        actor->addChild( rigActor );

        auto prefab = prefabManager->loadPrefab( "f40.fbx" );
        if( prefab )
        {
            auto vehicleMesh = prefab->createActor();

            // auto vehicleMeshTransform = vehicleMesh->getLocalTransform();
            // if (vehicleMeshTransform)
            //{
            //	vehicleMeshTransform->setScale(Vector3<real_Num>::unit() * 0.0254f);
            // }

            vehicleMesh->setName( "F40" );
            rigActor->addChild( vehicleMesh );
        }

        auto groundPosition = Vector3<real_Num>::unitY() * 5.0f;
        auto groundScale = Vector3<real_Num>::unit() * 1.0f;

        auto transform = actor->getTransform();
        if( transform )
        {
            transform->setLocalPosition( groundPosition );
            transform->setLocalScale( groundScale );
        }

        actor->updateTransform();

        if( addToScene )
        {
            scene->addActor( actor );
            scene->registerAllUpdates( actor );
        }

        return actor;
    }

    SmartPtr<scene::IGameActor> ApplicationUtil::createDefaultParticleSystem( bool addToScene )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        WP_ASSERT( scene );

        auto actor = sceneManager->createActor();
        WP_ASSERT( actor );

        auto particleSystem = actor->addComponent<scene::ParticleSystem>();
        WP_ASSERT( particleSystem );

        return actor;
    }

    SmartPtr<render::IMaterial> ApplicationUtil::createDefaultMaterialUI()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( !graphicsSystem )
        {
            WP_LOG_ERROR( "No graphics system found" );
            return nullptr;
        }

        auto materialManager = graphicsSystem->getMaterialManager();
        WP_ASSERT( materialManager );

        auto materialName = String( "DefaultUI" );
        SmartPtr<render::IMaterial> defaultMat = materialManager->loadFromFile( materialName + ".mat" );
        if( !defaultMat )
        {
            static const auto uuid =
                String( "2f261ebe-5db8-11ed-9b6a-0242ac120002" );  //StringUtil::getUUID();

            auto resource = materialManager->createOrRetrieve( uuid, materialName, "material" );
            auto material = workphone::static_pointer_cast<render::IMaterial>( resource.first );
            if( material )
            {
                material->setMaterialType( MaterialType::UI );
                graphicsSystem->loadObject( material );

                auto techniques = material->getTechniques();
                SmartPtr<render::IMaterialTechnique> technique;

                if( !techniques.empty() )
                {
                    technique = techniques[0];
                }

                if( !technique )
                {
                    technique = material->createTechnique();
                }

                if( technique )
                {
                    auto passes = technique->getPasses();
                    SmartPtr<render::IMaterialPass> pass;

                    if( !passes.empty() )
                    {
                        pass = passes[0];
                    }

                    if( !pass )
                    {
                        pass = technique->createPass();
                    }

                    if( pass )
                    {
                        auto textures = pass->getTextureUnits();
                        if( !textures.empty() )
                        {
                            //textures[0];
                        }
                    }
                }
            }

            return material;
        }

        return defaultMat;
    }

    SmartPtr<render::IMaterial> ApplicationUtil::createDefaultMaterial()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        auto materialManager = graphicsSystem->getMaterialManager();
        WP_ASSERT( materialManager );

        auto resource = materialManager->create( StringUtil::getUUID() );
        auto material = workphone::static_pointer_cast<render::IMaterial>( resource );
        if( material )
        {
            auto techniques = material->getTechniques();
            SmartPtr<render::IMaterialTechnique> technique;

            if( !techniques.empty() )
            {
                technique = techniques[0];
            }

            if( !technique )
            {
                technique = material->createTechnique();
            }

            if( technique )
            {
                auto passes = technique->getPasses();
                SmartPtr<render::IMaterialPass> pass;

                if( !passes.empty() )
                {
                    pass = passes[0];
                }

                if( !pass )
                {
                    pass = technique->createPass();
                }

                if( pass )
                {
                }
            }
        }

        return material;
    }

    SmartPtr<scene::IGameActor> ApplicationUtil::createProceduralTest()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        WP_ASSERT( scene );

        auto actor = sceneManager->createActor();

        auto name = String( "ProceduralScene" );
        actor->setName( name );

        // auto id = StringUtil::getHash(name);
        // actor->setId(id);

        // auto uuid = StringUtil::getUUID();
        // actor->setUUID(uuid);

        //auto proceduralScene = actor->addComponent<scene::ProceduralScene>();
        //if( proceduralScene )
        //{
        //    proceduralScene->loadMapData( "bullsmoor_small.osm" );
        //}

        scene->addActor( actor );
        scene->registerUpdate( TaskId::Primary, Thread::UpdateState::Update, actor );

        auto uniqueId = 0;  // StringUtil::getHash(uuid);
        return actor;
    }

    Vector2<real_Num> ApplicationUtil::getRelativeMousePos( Vector2<real_Num> relativeMousePosition )
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto ui = applicationManager->getUI();
        if( ui )
        {
            if( auto uiWindow = ui->getMainWindow() )
            {
                if( auto mainWindow = applicationManager->getWindow() )
                {
                    auto mainWindowSize = mainWindow->getSize();
                    auto mainWindowSizeF = Vector2<real_Num>( static_cast<f32>( mainWindowSize.x ),
                                                              static_cast<f32>( mainWindowSize.y ) );

                    auto pos = uiWindow->getPosition() / mainWindowSizeF;
                    auto size = uiWindow->getSize() / mainWindowSizeF;

                    auto aabb = AABB2<real_Num>( pos, size, true );
                    if( aabb.isInside( relativeMousePosition ) )
                    {
                        return ( relativeMousePosition - pos ) / size;
                    }
                }
            }
        }

        return Vector2<real_Num>::zero();
    }

    void ApplicationUtil::createDefaultScene()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );

        auto scene = sceneManager->getCurrentScene();
        WP_ASSERT( scene );

        createDefaultSky();
        createDirectionalLight();
    }

    /**
     * @brief Determines the creation order priority for a shared object.
     * @param object The shared object to evaluate.
     * @return Priority value where higher numbers indicate earlier creation order.
     *         Components that others depend on should have higher values.
     */
    u32 ApplicationUtil::getCreationOrder( SmartPtr<ISharedObject> object )
    {
        if( !object )
        {
            return 0;
        }

        // Material should be created early for rendering dependencies
        if( object->isDerived<scene::Material>() )
        {
            return 600000;
        }

        // Transform must be created first as all components depend on it
        if( object->isDerived<scene::Transform>() )
        {
            return 500000;
        }

        // Layout components for UI hierarchy
        if( object->isDerived<scene::Layout>() )
        {
            return 450000;
        }

        if( object->isDerived<scene::LayoutTransform>() )
        {
            return 440000;
        }

        // Mesh data before renderer
        if( object->isDerived<scene::Mesh>() )
        {
            return 200000;
        }

        // Collision shapes before rigidbody
        if( object->isDerived<scene::Collision>() )
        {
            return 100000;
        }

        // Rigidbody after collision shapes
        if( object->isDerived<scene::Rigidbody>() )
        {
            return 50000;
        }

        // Camera and light components
        if( object->isDerived<scene::Camera>() )
        {
            return 10000;
        }

        if( object->isDerived<scene::Light>() )
        {
            return 9000;
        }

        // Vehicle-related components
        if( object->isDerived<scene::CarController>() )
        {
            return 5000;
        }

        if( object->isDerived<scene::WheelController>() )
        {
            return 4000;
        }

        // Terrain system
        if( object->isDerived<scene::TerrainSystem>() )
        {
            return 3000;
        }

        // UI components
        if( object->isDerived<scene::Image>() )
        {
            return 500;
        }

        if( object->isDerived<scene::Text>() )
        {
            return 400;
        }

        if( object->isDerived<scene::Button>() )
        {
            return 300;
        }

        // Mesh renderer after mesh and material
        if( object->isDerived<scene::MeshRenderer>() )
        {
            return 200;
        }

        if( object->isDerived<scene::Skybox>() )
        {
            return 180;
        }

        // Generic component fallback
        if( object->isDerived<scene::Component>() )
        {
            return 100;
        }

        return 0;
    }

    bool ApplicationUtil::hasAnyTexture( const Array<SmartPtr<render::ITexture>> &textures )
    {
        for( auto &texture : textures )
        {
            if( texture )
            {
                return true;
            }
        }

        return false;
    }

}  // namespace workphone
