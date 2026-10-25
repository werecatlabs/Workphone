#include <EditorPCH.hpp>
#include <Workphone/Database/AssetReferenceChange.hpp>
#include <Workphone/Core/PropertyPath.hpp>
#include <ui/PropertiesWindow.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <ui/UIManager.hpp>
#include <ui/ResourceDatabaseDialog.hpp>
#include <ui/ActorWindow.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Scene/Components/Cubemap.hpp>
#include <Workphone/Interface/Graphics/IGraphicsCubemap.hpp>

namespace workphone::editor
{
    namespace
    {
        SmartPtr<ISharedObject> propertyTarget( SmartPtr<ISharedObject> object )
        {
            auto file = dynamic_pointer_cast<FileSelection>( object );
            if( !file ) return object;
            auto app = core::IApplicationManager::instancePtr();
            auto resources = app ? app->getResourceDatabase() : nullptr;
            if( !resources ) return nullptr;
            const auto path = file->getFilePath();
            const auto extension = StringUtil::make_lower( Path::getFileExtension( path ) );
            if( extension == ".mat" )
            {
                auto graphics = app->getGraphicsSystem();
                auto materials = graphics ? graphics->getMaterialManager() : nullptr;
                return materials ? materials->loadFromFile( path ) : nullptr;
            }
            if( ApplicationUtil::isSupportedMesh( path ) )
                return resources->loadDirectorFromResourcePath( path, scene::MeshResourceDirector::typeInfo() );
            if( ApplicationUtil::isSupportedTexture( path ) )
                return resources->loadDirectorFromResourcePath( path, scene::TextureResourceDirector::typeInfo() );
            if( ApplicationUtil::isSupportedSound( path ) )
                return resources->loadDirectorFromResourcePath( path, scene::SoundResourceDirector::typeInfo() );
            return resources->loadResource( path );
        }
    }

    WP_CLASS_REGISTER_DERIVED( workphone, PropertiesWindow, EditorWindow );
    WP_CLASS_REGISTER_DERIVED( workphone, PropertiesWindow::PropertiesDropTarget, ui::IUIDropTarget );
    WP_CLASS_REGISTER_DERIVED( workphone, PropertiesWindow::UpdateSelectionJob, Job );
    WP_CLASS_REGISTER_DERIVED( workphone, PropertiesWindow::PropertiesListener, IEventListener );

    PropertiesWindow::PropertiesWindow() = default;

    PropertiesWindow::~PropertiesWindow() = default;

    void PropertiesWindow::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto ui = applicationManager->getUI();
            WP_ASSERT( ui );

            auto parent = getParent();

            auto parentWindow = ui->addElementByType<ui::IUIWindow>();
            WP_ASSERT( parentWindow );

            setParentWindow( parentWindow );
            parentWindow->setLabel( "PropertiesWindowChild" );
            parentWindow->setSize( Vector2F( 0.0f, 300.0f ) );
            parentWindow->setHasBorder( true );

            if( parent )
            {
                parent->addChild( parentWindow );
            }

            m_propertyGrid = ui->addElementByType<ui::IUIPropertyGrid>();
            parentWindow->addChild( m_propertyGrid );

            auto propertiesListener = workphone::make_ptr<PropertiesListener>();
            propertiesListener->setOwner( this );
            m_propertyGrid->addObjectListener( propertiesListener );
            setEventListener( propertiesListener );

            auto dropTarget = workphone::make_ptr<PropertiesDropTarget>();
            dropTarget->setOwner( this );
            m_propertyGrid->setDropTarget( dropTarget );
            m_dropTarget = dropTarget;

            m_cubemapPreview = ui->addElementByType<ui::IUIWindow>();
            m_cubemapPreview->setLabel( "Cubemap Texture Preview" );
            m_cubemapPreview->setSize( Vector2F( 0.0f, 300.0f ) );
            parentWindow->addChild( m_cubemapPreview );
            m_cubemapPreviewStatus = ui->addElementByType<ui::IUIText>();
            m_cubemapPreview->addChild( m_cubemapPreviewStatus );

            const char *faceNames[] = { "Front (-Z)", "Back (+Z)", "Left (-X)",
                                        "Right (+X)", "Up (+Y)",   "Down (-Y)" };
            for( size_t i = 0; i < 6; ++i )
            {
                auto panel = ui->addElementByType<ui::IUIWindow>();
                panel->setLabel( String( "CubemapFace##" ) + StringUtil::toString( i ) );
                // Keep faces in one column so the preview fits the narrow default inspector dock.
                panel->setSize( Vector2F( 136.0f, 155.0f ) );
                panel->setHasBorder( false );
                m_cubemapPreview->addChild( panel );
                m_cubemapFacePanels.push_back( panel );

                auto label = ui->addElementByType<ui::IUIText>();
                label->setText( faceNames[i] );
                panel->addChild( label );
                auto image = ui->addElementByType<ui::IUIImage>();
                image->setSize( Vector2F( 120.0f, 120.0f ) );
                panel->addChild( image );
                m_cubemapFaceImages.push_back( image );
            }
            m_cubemapPreview->setVisible( false, false );

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void PropertiesWindow::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto ui = applicationManager->getUI();
            WP_ASSERT( ui );

            if( m_propertyGrid )
            {
                if( auto listener = getEventListener() )
                {
                    m_propertyGrid->removeObjectListener( listener );
                    setEventListener( nullptr );
                }

                m_propertyGrid->setDropTarget( nullptr );

                ui->removeElement( m_propertyGrid );
                m_propertyGrid = nullptr;
            }

            m_dropTarget = nullptr;
            m_selected = nullptr;
            m_selection.clear();

            // The parent window recursively removes the preview controls from the UI manager.
            for( auto image : m_cubemapFaceImages )
            {
                image->setTexture( nullptr );
            }
            m_cubemapFaceImages.clear();
            m_cubemapFacePanels.clear();
            m_cubemapPreviewStatus = nullptr;
            m_cubemapPreview = nullptr;

            if( auto parentWindow = getParentWindow() )
            {
                ui->removeElement( parentWindow );
                setParentWindow( nullptr );
            }

            EditorWindow::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void PropertiesWindow::update()
    {
        auto object = getSelected();
        if( !object )
        {
            auto app = core::IApplicationManager::instancePtr();
            auto selectionManager = app ? app->getSelectionManagerPtr() : nullptr;
            auto selection =
                selectionManager ? selectionManager->getSelection() : Array<SmartPtr<ISharedObject>>();
            if( !selection.empty() )
            {
                object = selection.front();
            }
        }
        updateCubemapPreview( object );

        if( isDirty() )
        {
            updateSelection();
            setDirty( false );
        }
    }

    void PropertiesWindow::updateCubemapPreview( SmartPtr<ISharedObject> object )
    {
        if( !m_cubemapPreview )
        {
            return;
        }

        auto cubemap = workphone::dynamic_pointer_cast<scene::Cubemap>( object );
        if( !cubemap )
        {
            if( auto actor = workphone::dynamic_pointer_cast<scene::IGameActor>( object ) )
            {
                cubemap = actor->getComponent<scene::Cubemap>();
            }
        }

        auto probe = cubemap ? cubemap->getRenderCubemap() : nullptr;
        auto texture = probe ? probe->getTexture() : nullptr;
        auto faces = texture ? texture->getCubemapFaces() : Array<SmartPtr<render::ITexture>>();
        auto hasFaces = faces.size() == 6;
        for( size_t i = 0; i < m_cubemapFaceImages.size(); ++i )
        {
            auto face = hasFaces ? faces[i] : nullptr;
            if( m_cubemapFaceImages[i]->getTexture() != face )
            {
                m_cubemapFaceImages[i]->setTexture( face );
            }
            m_cubemapFacePanels[i]->setVisible( cubemap && face != nullptr );
        }

        m_cubemapPreview->setVisible( cubemap != nullptr, false );
        m_cubemapPreview->setSize( Vector2F( 0.0f, hasFaces ? 300.0f : 100.0f ) );
        auto status = String( "Cubemap Texture\nPreview\n" );
        if( !texture )
        {
            status += "No cubemap texture.\nAssign or generate one.";
        }
        else if( !hasFaces )
        {
            status += "Preview unavailable\nfor this texture\nor graphics backend.";
        }
        else
        {
            status += texture->getName() + "\nSource faces\n(before roughness filtering)";
        }
        if( m_cubemapPreviewStatus->getText() != status )
        {
            m_cubemapPreviewStatus->setText( status );
        }
    }

    void PropertiesWindow::updateSelection()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto selectionManager = applicationManager->getSelectionManager();
            auto jobQueue = applicationManager->getJobQueue();

            auto previewObject = getSelected();
            if( !previewObject )
            {
                auto selection = selectionManager->getSelection();
                if( !selection.empty() )
                {
                    previewObject = selection.front();
                }
            }
            updateCubemapPreview( previewObject );

            if( auto selected = getSelected() )
            {
                auto job = workphone::make_ptr<UpdateSelectionJob>();
                job->setOwner( this );
                job->setObject( selected );
                jobQueue->addJob( job );
            }
            else
            {
                auto selection = selectionManager->getSelection();
                for( auto object : selection )
                {
                    auto job = workphone::make_ptr<UpdateSelectionJob>();
                    job->setOwner( this );
                    job->setObject( object );
                    jobQueue->addJob( job );
                }

                setSelection( selection );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void PropertiesWindow::updateSelection( SmartPtr<ISharedObject> object )
    {
        updateCubemapPreview( object );
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        auto materialManager = graphicsSystem->getMaterialManager();

        auto resourceDatabase = applicationManager->getResourceDatabase();

        auto meshManager = applicationManager->getMeshManager();

        if( object )
        {
            if( object->isDerived<scene::IGameActor>() )
            {
                auto actor = workphone::dynamic_pointer_cast<scene::IGameActor>( object );

                auto properties = actor->getProperties();
                if( properties )
                {
                    m_propertyGrid->setProperties( properties );
                }
            }
            else if( object->isDerived<physics::IPhysicsMaterial3>() )
            {
                auto physicsMaterial =
                    workphone::dynamic_pointer_cast<physics::IPhysicsMaterial3>( object );

                auto properties = physicsMaterial->getProperties();
                if( properties )
                {
                    m_propertyGrid->setProperties( properties );
                }
            }
            else if( object->isDerived<physics::IPhysicsShape>() )
            {
                auto physicsShape = workphone::dynamic_pointer_cast<physics::IPhysicsShape>( object );

                auto properties = physicsShape->getProperties();
                if( properties )
                {
                    m_propertyGrid->setProperties( properties );
                }
            }
            else if( object->isDerived<physics::IPhysicsBody3>() )
            {
                auto physicsBody = workphone::dynamic_pointer_cast<physics::IPhysicsBody3>( object );

                auto properties = physicsBody->getProperties();
                if( properties )
                {
                    m_propertyGrid->setProperties( properties );
                }
            }
            else if( object->isDerived<scene::ITransform>() )
            {
                auto transform = workphone::dynamic_pointer_cast<scene::ITransform>( object );

                auto properties = transform->getProperties();
                if( properties )
                {
                    m_propertyGrid->setProperties( properties );
                }
            }
            else if( object->isDerived<render::IGraphicsSceneNode>() )
            {
                auto sceneNode = workphone::dynamic_pointer_cast<render::IGraphicsSceneNode>( object );

                auto properties = sceneNode->getProperties();
                if( properties )
                {
                    m_propertyGrid->setProperties( properties );
                }
            }
            else if( object->isDerived<render::IGraphicsObject>() )
            {
                auto graphicsObject = workphone::dynamic_pointer_cast<render::IGraphicsObject>( object );

                auto properties = graphicsObject->getProperties();
                if( properties )
                {
                    m_propertyGrid->setProperties( properties );
                }
            }
            else if( object->isDerived<IStateContext>() )
            {
                auto stateContext = workphone::dynamic_pointer_cast<IStateContext>( object );

                auto properties = stateContext->getProperties();
                if( properties )
                {
                    m_propertyGrid->setProperties( properties );
                }
            }
            else if( object->isDerived<IState>() )
            {
                auto state = workphone::dynamic_pointer_cast<IState>( object );

                auto properties = state->getProperties();
                if( properties )
                {
                    m_propertyGrid->setProperties( properties );
                }
            }
            else if( object->isDerived<IMeshResource>() )
            {
                auto meshResource = workphone::static_pointer_cast<IMeshResource>( object );

                auto properties = meshResource->getProperties();
                if( properties )
                {
                    m_propertyGrid->setProperties( properties );
                }
            }
            else if( object->isDerived<render::IMaterial>() )
            {
                auto material = workphone::static_pointer_cast<render::IMaterial>( object );

                auto properties = material->getProperties();
                if( properties )
                {
                    m_propertyGrid->setProperties( properties );
                }
            }
            else if( object->isDerived<render::IMaterialNode>() )
            {
                auto materialNode = workphone::static_pointer_cast<render::IMaterialNode>( object );

                auto properties = materialNode->getProperties();
                if( properties )
                {
                    m_propertyGrid->setProperties( properties );
                }
            }
            else if( object->isDerived<scene::IComponent>() )
            {
                auto derived = workphone::dynamic_pointer_cast<scene::IComponent>( object );

                if( auto properties = derived->getProperties() )
                {
                    m_propertyGrid->setProperties( properties );
                }
            }
            else if( object->isDerived<scene::IComponentEventListener>() )
            {
                auto derived = workphone::dynamic_pointer_cast<scene::IComponentEventListener>( object );

                if( auto properties = derived->getProperties() )
                {
                    m_propertyGrid->setProperties( properties );
                }
            }
            else if( object->isDerived<IBuildDirector>() )
            {
                auto derived = workphone::dynamic_pointer_cast<IBuildDirector>( object );

                if( auto properties = derived->getProperties() )
                {
                    m_propertyGrid->setProperties( properties );
                }
            }
            else if( object->isDerived<ui::IUIElement>() )
            {
                auto element = workphone::dynamic_pointer_cast<ui::IUIElement>( object );

                if( auto properties = element->getProperties() )
                {
                    m_propertyGrid->setProperties( properties );
                }
            }
            else if( object->isDerived<render::IOverlay>() )
            {
                auto element = workphone::dynamic_pointer_cast<render::IOverlay>( object );
                if( auto properties = element->getProperties() )
                {
                    m_propertyGrid->setProperties( properties );
                }
            }
            else if( object->isDerived<render::IOverlayElement>() )
            {
                auto element = workphone::dynamic_pointer_cast<render::IOverlayElement>( object );
                if( auto properties = element->getProperties() )
                {
                    m_propertyGrid->setProperties( properties );
                }
            }
            else if( object->isDerived<core::IPrototype>() )
            {
                auto prototype = workphone::dynamic_pointer_cast<core::IPrototype>( object );
                if( auto properties = prototype->getProperties() )
                {
                    m_propertyGrid->setProperties( properties );
                }
            }
            else if( object->isDerived<FileSelection>() )
            {
                auto fileSelection = workphone::dynamic_pointer_cast<FileSelection>( object );

                auto filePath = fileSelection->getFilePath();
                auto ext = Path::getFileExtension( filePath );
                ext = StringUtil::make_lower( ext );

                static const auto materialExt = String( ".mat" );
                static const auto fbxExt = String( ".fbx" );
                static const auto resourceExt = String( ".resource" );

                if( ext == materialExt )
                {
                    auto material = materialManager->loadFromFile( filePath );
                    if( material )
                    {
                        auto properties = material->getProperties();
                        if( properties )
                        {
                            m_propertyGrid->setProperties( properties );
                        }
                    }
                }
                else if( ApplicationUtil::isSupportedMesh( filePath ) )
                {
                    auto mesh = meshManager->loadFromFile( filePath );
                    if( mesh )
                    {
                        auto properties = mesh->getProperties();
                        if( properties )
                        {
                            m_propertyGrid->setProperties( properties );
                        }
                    }
                }
                else if( ApplicationUtil::isSupportedTexture( filePath ) )
                {
                    auto texture = graphicsSystem->getTextureManager()->loadFromFile( filePath );
                    if( texture )
                    {
                        auto properties = texture->getProperties();
                        if( properties )
                        {
                            m_propertyGrid->setProperties( properties );
                        }
                    }
                }
                else if( ApplicationUtil::isSupportedSound( filePath ) )
                {
                    auto soundManager = applicationManager->getSoundManager();
                    auto sound = soundManager->loadFromFile( filePath );
                    if( sound )
                    {
                        auto properties = sound->getProperties();
                        if( properties )
                        {
                            m_propertyGrid->setProperties( properties );
                        }
                    }
                }
                else if( ext == resourceExt )
                {
                    auto resource = resourceDatabase->loadResource( filePath );
                    if( resource )
                    {
                        auto properties = resource->getProperties();
                        if( properties )
                        {
                            m_propertyGrid->setProperties( properties );
                        }
                    }
                }
            }
        }
    }

    void PropertiesWindow::updateSelectionMT( SmartPtr<ISharedObject> object )
    {
        if( auto propertyGrid = getPropertyGrid() )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            auto materialManager = graphicsSystem->getMaterialManager();

            auto resourceDatabase = applicationManager->getResourceDatabase();

            auto meshManager = applicationManager->getMeshManager();

            if( object )
            {
                if( object->isDerived<scene::IGameActor>() )
                {
                    auto actor = workphone::dynamic_pointer_cast<scene::IGameActor>( object );

                    auto properties = actor->getProperties();
                    if( properties )
                    {
                        propertyGrid->setProperties( properties );
                    }
                }
                else if( object->isDerived<physics::IPhysicsMaterial3>() )
                {
                    auto physicsMaterial =
                        workphone::dynamic_pointer_cast<physics::IPhysicsMaterial3>( object );

                    auto properties = physicsMaterial->getProperties();
                    if( properties )
                    {
                        propertyGrid->setProperties( properties );
                    }
                }
                else if( object->isDerived<physics::IPhysicsShape>() )
                {
                    auto physicsShape =
                        workphone::dynamic_pointer_cast<physics::IPhysicsShape>( object );

                    auto properties = physicsShape->getProperties();
                    if( properties )
                    {
                        propertyGrid->setProperties( properties );
                    }
                }
                else if( object->isDerived<physics::IPhysicsBody3>() )
                {
                    auto physicsBody = workphone::dynamic_pointer_cast<physics::IPhysicsBody3>( object );

                    auto properties = physicsBody->getProperties();
                    if( properties )
                    {
                        propertyGrid->setProperties( properties );
                    }
                }
                else if( object->isDerived<scene::ITransform>() )
                {
                    auto transform = workphone::dynamic_pointer_cast<scene::ITransform>( object );

                    auto properties = transform->getProperties();
                    if( properties )
                    {
                        propertyGrid->setProperties( properties );
                    }
                }
                else if( object->isDerived<render::IGraphicsSceneNode>() )
                {
                    auto sceneNode =
                        workphone::dynamic_pointer_cast<render::IGraphicsSceneNode>( object );

                    auto properties = sceneNode->getProperties();
                    if( properties )
                    {
                        propertyGrid->setProperties( properties );
                    }
                }
                else if( object->isDerived<render::IGraphicsObject>() )
                {
                    auto graphicsObject =
                        workphone::dynamic_pointer_cast<render::IGraphicsObject>( object );

                    auto properties = graphicsObject->getProperties();
                    if( properties )
                    {
                        propertyGrid->setProperties( properties );
                    }
                }
                else if( object->isDerived<IStateContext>() )
                {
                    auto stateContext = workphone::dynamic_pointer_cast<IStateContext>( object );

                    auto properties = stateContext->getProperties();
                    if( properties )
                    {
                        propertyGrid->setProperties( properties );
                    }
                }
                else if( object->isDerived<IState>() )
                {
                    auto state = workphone::dynamic_pointer_cast<IState>( object );

                    auto properties = state->getProperties();
                    if( properties )
                    {
                        propertyGrid->setProperties( properties );
                    }
                }
                else if( object->isDerived<IMeshResource>() )
                {
                    auto meshResource = workphone::static_pointer_cast<IMeshResource>( object );

                    auto properties = meshResource->getProperties();
                    if( properties )
                    {
                        propertyGrid->setProperties( properties );
                    }
                }
                else if( object->isDerived<render::IMaterial>() )
                {
                    auto material = workphone::static_pointer_cast<render::IMaterial>( object );

                    auto properties = material->getProperties();
                    if( properties )
                    {
                        propertyGrid->setProperties( properties );
                    }
                }
                else if( object->isDerived<render::IMaterialNode>() )
                {
                    auto materialNode = workphone::static_pointer_cast<render::IMaterialNode>( object );

                    auto properties = materialNode->getProperties();
                    if( properties )
                    {
                        propertyGrid->setProperties( properties );
                    }
                }
                else if( object->isDerived<scene::IComponent>() )
                {
                    auto derived = workphone::dynamic_pointer_cast<scene::IComponent>( object );

                    if( auto properties = derived->getProperties() )
                    {
                        propertyGrid->setProperties( properties );
                    }
                }
                else if( object->isDerived<scene::IComponentEventListener>() )
                {
                    auto derived =
                        workphone::dynamic_pointer_cast<scene::IComponentEventListener>( object );

                    if( auto properties = derived->getProperties() )
                    {
                        propertyGrid->setProperties( properties );
                    }
                }
                else if( object->isDerived<IBuildDirector>() )
                {
                    auto derived = workphone::dynamic_pointer_cast<IBuildDirector>( object );

                    if( auto properties = derived->getProperties() )
                    {
                        propertyGrid->setProperties( properties );
                    }
                }
                else if( object->isDerived<ui::IUIElement>() )
                {
                    auto element = workphone::dynamic_pointer_cast<ui::IUIElement>( object );

                    if( auto properties = element->getProperties() )
                    {
                        propertyGrid->setProperties( properties );
                    }
                }
                else if( object->isDerived<render::IOverlay>() )
                {
                    auto element = workphone::dynamic_pointer_cast<render::IOverlay>( object );
                    if( auto properties = element->getProperties() )
                    {
                        propertyGrid->setProperties( properties );
                    }
                }
                else if( object->isDerived<render::IOverlayElement>() )
                {
                    auto element = workphone::dynamic_pointer_cast<render::IOverlayElement>( object );
                    if( auto properties = element->getProperties() )
                    {
                        propertyGrid->setProperties( properties );
                    }
                }
                else if( object->isDerived<core::IPrototype>() )
                {
                    auto prototype = workphone::dynamic_pointer_cast<core::IPrototype>( object );
                    if( auto properties = prototype->getProperties() )
                    {
                        propertyGrid->setProperties( properties );
                    }
                }
                else if( object->isDerived<FileSelection>() )
                {
                    auto fileSelection = workphone::dynamic_pointer_cast<FileSelection>( object );

                    auto filePath = fileSelection->getFilePath();
                    if( StringUtil::isNullOrEmpty( filePath ) )
                    {
                        return;
                    }

                    auto ext = Path::getFileExtension( filePath );
                    ext = StringUtil::make_lower( ext );

                    static const auto materialExt = String( ".mat" );
                    static const auto fbxExt = String( ".fbx" );
                    static const auto resourceExt = String( ".resource" );

                    if( ext == materialExt )
                    {
                        auto material = materialManager->loadFromFile( filePath );
                        if( material )
                        {
                            auto properties = material->getProperties();
                            if( properties )
                            {
                                propertyGrid->setProperties( properties );
                            }
                        }
                    }
                    else if( ApplicationUtil::isSupportedMesh( filePath ) )
                    {
                        auto director = resourceDatabase->loadDirectorFromResourcePath(
                            filePath, scene::MeshResourceDirector::typeInfo() );
                        if( director )
                        {
                            auto properties = director->getProperties();
                            if( properties )
                            {
                                propertyGrid->setProperties( properties );
                            }
                        }
                    }
                    else if( ApplicationUtil::isSupportedTexture( filePath ) )
                    {
                        auto director = resourceDatabase->loadDirectorFromResourcePath(
                            filePath, scene::TextureResourceDirector::typeInfo() );
                        if( director )
                        {
                            auto properties = director->getProperties();
                            if( properties )
                            {
                                propertyGrid->setProperties( properties );
                            }
                        }
                    }
                    else if( ApplicationUtil::isSupportedSound( filePath ) )
                    {
                        auto director = resourceDatabase->loadDirectorFromResourcePath(
                            filePath, scene::SoundResourceDirector::typeInfo() );
                        if( director )
                        {
                            auto properties = director->getProperties();
                            if( properties )
                            {
                                propertyGrid->setProperties( properties );
                            }
                        }
                    }
                    else
                    {
                        auto resource = resourceDatabase->loadResource( filePath );
                        if( resource )
                        {
                            auto properties = resource->getProperties();
                            if( properties )
                            {
                                propertyGrid->setProperties( properties );
                            }
                        }
                    }
                }
                else
                {
                    if( auto properties = object->getProperties() )
                    {
                        propertyGrid->setProperties( properties );
                    }
                }
            }
        }
    }

    bool PropertiesWindow::isDirty() const
    {
        return m_isDirty;
    }

    void PropertiesWindow::setDirty( bool dirty )
    {
        if( m_isDirty != dirty )
        {
            m_isDirty = dirty;

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto jobQueue = applicationManager->getJobQueue();
            WP_ASSERT( jobQueue );

            auto job = workphone::make_ptr<ObjectUpdateJob>();
            job->setOwner( this );
            jobQueue->addJob( job );
        }
    }

    SmartPtr<ui::IUIPropertyGrid> PropertiesWindow::getPropertyGrid() const
    {
        return m_propertyGrid;
    }

    void PropertiesWindow::setPropertyGrid( SmartPtr<ui::IUIPropertyGrid> val )
    {
        m_propertyGrid = val;
    }

    SmartPtr<ISharedObject> PropertiesWindow::getSelected() const
    {
        return m_selected;
    }

    void PropertiesWindow::setSelected( SmartPtr<ISharedObject> selected )
    {
        m_selected = selected;
    }

    Array<SmartPtr<ISharedObject>> PropertiesWindow::getSelection() const
    {
        return m_selection;
    }

    void PropertiesWindow::setSelection( Array<SmartPtr<ISharedObject>> selection )
    {
        m_selection = selection;
    }

    void PropertiesWindow::propertyChange( SmartPtr<Properties> properties, const String &name,
                                           const String &value, bool isButton )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto selectionManager = applicationManager->getSelectionManager();
            WP_ASSERT( selectionManager );

            auto editorManager = EditorManager::getSingletonPtr();
            WP_ASSERT( editorManager );

            auto ui = editorManager->getUI();
            WP_ASSERT( ui );

            auto resourceType = (hash_type)ISharedObject::typeInfo();
            PropertyGroupPath propertyPath;
            if( auto root = m_propertyGrid ? m_propertyGrid->getProperties() : nullptr )
            {
                if( properties && !findPropertyGroupPath( root, properties, propertyPath ) )
                {
                    WP_LOG_ERROR( "Ignored an edit from a stale property group." );
                    return;
                }
            }

            if( auto selected = getSelected() )
            {
                resourceType = propertyChange( properties, selected, name, value, isButton );
            }
            else
            {
                auto selection = getSelection();
                for( auto object : selection )
                {
                    resourceType = propertyChange( properties, object, name, value, isButton );
                }
            }

            if( resourceType != 0 && resourceType != ISharedObject::typeInfo() )
            {
                if( isButton )
                {
                    if( ui )
                    {
                        if( auto resourceDatabaseDialog = ui->getResourceDatabaseDialog() )
                        {
                            resourceDatabaseDialog->setResourceType( resourceType );

                            if( auto selected = getSelected() )
                            {
                                auto target = propertyTarget( selected );
                                if( !target )
                                {
                                    WP_LOG_ERROR( "The property target is no longer available." );
                                    return;
                                }
                                resourceDatabaseDialog->setCurrentObject( target );
                            }
                            else
                            {
                                resourceDatabaseDialog->setCurrentObject( nullptr );
                            }

                            resourceDatabaseDialog->setPropertyName( name );
                            resourceDatabaseDialog->setPropertyPath( propertyPath );
                            resourceDatabaseDialog->setWindowVisible( true );
                            resourceDatabaseDialog->updateSelection();
                        }
                    }
                }
            }

            setDirty( true );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    hash_type PropertiesWindow::propertyChange( SmartPtr<Properties> properties,
                                                SmartPtr<ISharedObject> object, const String &name,
                                                const String &value, bool isButton )
    {
        if( !object ) return 0;

        // Events identify the owning group in the displayed snapshot. Resolve
        // its path before loading a fresh director or another selected object.
        auto root = m_propertyGrid ? m_propertyGrid->getProperties() : nullptr;
        if( !root ) root = properties;
        if( !root ) root = object->getProperties();
        if( !properties ) properties = root;
        PropertyGroupPath path;
        if( !findPropertyGroupPath( root, properties, path ) || !properties->hasProperty( name ) )
            return 0;

        auto target = propertyTarget( object );
        if( !target ) return 0;
        auto fileResource = dynamic_pointer_cast<FileSelection>( object ) ?
                                dynamic_pointer_cast<IResource>( target ) : nullptr;

        auto targetRoot = target->getProperties();
        auto group = resolvePropertyGroup( targetRoot, path );
        if( !group || !group->hasProperty( name ) ) return 0;
        const auto &descriptor = group->getPropertyObject( name );
        if( descriptor.isReadOnly() ) return 0;
        const auto resourceType = AssetReferenceChange::referenceType( descriptor.getAttribute( "resourceType" ) );
        if( isButton && resourceType != 0 ) return resourceType;

        // Never replace a target with the displayed group's snapshot: another
        // selected object can have different sibling settings. Preserve types,
        // attributes and siblings, and apply the supplied value to this leaf.
        auto replacement = clonePropertyTree( targetRoot );
        auto edited = resolvePropertyGroup( replacement, path );
        auto &property = edited->getPropertyObject( name );
        if( isButton ) property.setValueAsButton( true );
        else property.setValue( value );
        target->setProperties( replacement );
        // A director's Save action is dispatched by setProperties itself.
        const bool dispatchedSave = isButton && path.empty() && name == scene::ResourceDirector::saveStr &&
                                    dynamic_pointer_cast<scene::ResourceDirector>( target );
        if( fileResource && !dispatchedSave ) fileResource->save();

        if( auto editor = EditorManager::getSingletonPtr() )
            if( auto project = editor->getProject() )
                if( target == project->getGraphicsSettingsDirector() ) project->setDirty( true );
        return resourceType;
    }

    Parameter PropertiesWindow::PropertiesListener::handleEvent(
        EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        if( auto owner = getOwner() )
        {
            if( arguments.size() < 3 ) return {};
            auto name = arguments[0].str;
            auto value = arguments[1].str;
            auto properties = workphone::dynamic_pointer_cast<Properties>( arguments[2].object );
            if( !properties ) return {};

            if( eventValue == IEvent::handlePropertyButtonClick )
            {
                owner->propertyChange( properties, name, value, true );
            }
            else if( eventValue == IEvent::handlePropertyChanged )
            {
                owner->propertyChange( properties, name, value, false );
            }
        }

        return {};
    }

    SmartPtr<PropertiesWindow> PropertiesWindow::PropertiesListener::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void PropertiesWindow::PropertiesListener::setOwner( SmartPtr<PropertiesWindow> owner )
    {
        m_owner = owner;
    }

    PropertiesWindow::PropertiesListener::PropertiesListener() = default;

    PropertiesWindow::PropertiesListener::~PropertiesListener() = default;

    PropertiesWindow::UpdateSelectionJob::UpdateSelectionJob() = default;

    PropertiesWindow::UpdateSelectionJob::~UpdateSelectionJob() = default;

    void PropertiesWindow::UpdateSelectionJob::execute()
    {
        auto owner = getOwner();
        WP_ASSERT( owner );

        auto object = getObject();
        owner->updateSelectionMT( object );
    }

    SmartPtr<PropertiesWindow> PropertiesWindow::UpdateSelectionJob::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void PropertiesWindow::UpdateSelectionJob::setOwner( SmartPtr<PropertiesWindow> owner )
    {
        m_owner = owner;
    }

    SmartPtr<ISharedObject> PropertiesWindow::UpdateSelectionJob::getObject() const
    {
        auto p = m_object.load();
        return p.lock();
    }

    void PropertiesWindow::UpdateSelectionJob::setObject( SmartPtr<ISharedObject> object )
    {
        m_object = object;
    }

    PropertiesWindow::PropertiesDropTarget::PropertiesDropTarget() = default;

    PropertiesWindow::PropertiesDropTarget::~PropertiesDropTarget() = default;

    Parameter PropertiesWindow::PropertiesDropTarget::handleEvent(
        EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        if( auto owner = getOwner() )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto resourceDatabase = applicationManager->getResourceDatabase();

            if( eventValue == IEvent::handleDrop )
            {
                if( arguments.size() < 4 ) return {};
                auto properties = dynamic_pointer_cast<Properties>( arguments[3].object );
                if( !properties ) return {};
                auto dragData = workphone::make_ptr<Properties>();
                auto dataStr = arguments[0].getStr();
                auto name = arguments[1].getStr();
                DataUtil::parse( dataStr, dragData.get() );

                auto filePath = dragData->getProperty( "filePath" );
                if( !StringUtil::isNullOrEmpty( filePath ) )
                {
                    if( !resourceDatabase ) return {};
                    auto resource = resourceDatabase->loadResource( filePath );
                    if( !resource )
                    {
                        WP_LOG( "Failed to load resource from file: " + filePath );
                    }

                    if( resource )
                    {
                        auto handle = resource->getHandle();
                        auto uuid = handle->getUUIDAsString();

                        owner->propertyChange( properties, name, uuid, false );
                    }
                }
                else
                {
                    auto uuid = dragData->getProperty( "actorUUID" );
                    if( !uuid.empty() ) owner->propertyChange( properties, name, uuid, false );
                }
            }
        }

        return {};
    }

    SmartPtr<PropertiesWindow> PropertiesWindow::PropertiesDropTarget::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void PropertiesWindow::PropertiesDropTarget::setOwner( SmartPtr<PropertiesWindow> owner )
    {
        m_owner = owner;
    }
}  // namespace workphone::editor
