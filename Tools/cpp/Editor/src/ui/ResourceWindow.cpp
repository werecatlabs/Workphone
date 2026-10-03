#include <EditorPCH.hpp>
#include "ui/ResourceWindow.hpp"
#include <Workphone/Workphone.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <editor/EditorManager.hpp>
#include <ui/ProjectTreeData.hpp>
#include <ui/SceneWindow.hpp>
#include <ui/PropertiesWindow.hpp>
#include <ui/UIManager.hpp>
#include <EditorTypes.hpp>
#include <ui/ObjectBrowserDialog.hpp>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone::editor, ResourceWindow, EditorWindow );
    WP_CLASS_REGISTER_DERIVED( workphone::editor, ResourceWindow::TreeCtrlListener, IEventListener );

    ResourceWindow::ResourceWindow( SmartPtr<ui::IUIWindow> parent )
    {
        setParent( parent );
    }

    ResourceWindow::~ResourceWindow()
    {
        unload( nullptr );
    }

    void ResourceWindow::load( SmartPtr<ISharedObject> data )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto ui = applicationManager->getUI();
        WP_ASSERT( ui );

        auto parent = getParent();

        auto parentWindow = ui->addElementByType<ui::IUIWindow>();
        setParentWindow( parentWindow );

        if( parent )
        {
            parent->addChild( parentWindow );
        }

        // m_dropdown = ui->addElementByType<ui::IUIDropdown>();
        // WP_ASSERT(m_dropdown);

        // auto materialTypes = render::GraphicsUtil::getMaterialTypes();
        // m_dropdown->setOptions(materialTypes);

        auto treeCtrl = ui->addElementByType<ui::IUITreeCtrl>();
        parentWindow->addChild( treeCtrl );
        m_tree = treeCtrl;

        auto treeListener = workphone::make_ptr<TreeCtrlListener>();
        treeListener->setOwner( this );
        m_treeListener = treeListener;

        m_tree->addObjectListener( treeListener );

        m_propertiesWindow = workphone::make_ptr<PropertiesWindow>();
        m_propertiesWindow->setParent( parentWindow );
        m_propertiesWindow->load( data );

        // parentWindow->addChild(m_dropdown);
    }

    void ResourceWindow::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto ui = applicationManager->getUI();
            WP_ASSERT( ui );

            if( m_tree )
            {
                m_tree->removeObjectListener( m_treeListener );

                ui->removeElement( m_tree );
                m_tree = nullptr;
            }

            if( m_treeListener )
            {
                m_treeListener->unload( nullptr );
                m_treeListener = nullptr;
            }

            if( m_propertiesWindow )
            {
                m_propertiesWindow->unload( nullptr );
                m_propertiesWindow = nullptr;
            }

            if( auto parentWindow = getParentWindow() )
            {
                ui->removeElement( parentWindow );
                setParentWindow( nullptr );
            }

            for( auto data : m_dataArray )
            {
                data->unload( nullptr );
            }

            m_dataArray.clear();

            EditorWindow::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ResourceWindow::updateSelection()
    {
        buildTree();

        if( m_propertiesWindow )
        {
            m_propertiesWindow->updateSelection();
        }
    }

    void ResourceWindow::buildTree()
    {
        try
        {
            WP_ASSERT( m_tree );

            if( m_tree )
            {
                m_tree->clear();
            }

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );
            WP_ASSERT( graphicsSystem->isValid() );

            auto materialManager = graphicsSystem->getMaterialManager();
            auto meshManager = applicationManager->getMeshManager();

            WP_ASSERT( materialManager );
            WP_ASSERT( meshManager );

            auto editorManager = EditorManager::getSingletonPtr();
            auto selectionManager = applicationManager->getSelectionManager();
            WP_ASSERT( selectionManager );

            auto sceneManager = applicationManager->getGameManager();
            WP_ASSERT( sceneManager );

            auto currentScene = sceneManager->getCurrentScene();
            WP_ASSERT( currentScene );

            /*
            if( currentScene )
            {
                auto project = editorManager->getProject();
                WP_ASSERT( project );

                auto appTemplate = project->getApplicationTemplate();

                auto sceneName = currentScene->getName();

                auto selection = selectionManager->getSelection();
                if( selection.size() == 1 )
                {
                    auto selectedObject = selection.front();

                    if( selectedObject->isDerived<FileSelection>() )
                    {
                        auto fileSelection =
                            fb::dynamic_pointer_cast<FileSelection>( selectedObject );

                        auto filePath = fileSelection->getFilePath();
                        auto ext = Path::getFileExtension( filePath );

                        static const auto materialExt = String( ".mat" );

                        if( ext == materialExt )
                        {
                            auto graphicsSystem = applicationManager->getGraphicsSystem();
                            auto materialManager = graphicsSystem->getMaterialManager();
                            auto material = materialManager->loadFromFile( filePath );
                            if( material )
                            {
                                m_material = material;

                                auto rootNode = m_tree->addRoot();
                                WP_ASSERT( rootNode );
                                rootNode->setExpanded( true );

                                addMaterialToTree( material, rootNode );

                                // auto properties = material->getProperties();
                                // if (properties)
                                //{
                                //	m_propertyGrid->setProperties(properties);
                                // }
                            }
                        }
                    }

                    // addMaterialToTree(material, node);
                }
            }
             */
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ResourceWindow::addMaterialToTree( SmartPtr<render::IMaterial> material,
                                            SmartPtr<ui::IUITreeNode> node )
    {
        try
        {
            auto editorManager = EditorManager::getSingletonPtr();

            auto project = editorManager->getProject();
            WP_ASSERT( project );

            auto data = SmartPtr<ProjectTreeData>(
                new ProjectTreeData( "material", "material", material, material ) );

            auto actorName = material->getName();
            if( StringUtil::isNullOrEmpty( actorName ) )
            {
                actorName = "Untitled";
            }

            auto treeNode = m_tree->addNode();

            WP_ASSERT( treeNode );
            Util::setText( treeNode, actorName );

            treeNode->setNodeUserData( data );

            if( node )
            {
                node->addChild( treeNode );
            }

            addObjectToTree( material, treeNode );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ResourceWindow::addObjectToTree( SmartPtr<ISharedObject> object,
                                          SmartPtr<ui::IUITreeNode> node )
    {
        try
        {
            if( object )
            {
                auto editorManager = EditorManager::getSingletonPtr();

                auto project = editorManager->getProject();
                WP_ASSERT( project );

                auto data = SmartPtr<ProjectTreeData>(
                    new ProjectTreeData( "object", "object", object, object ) );

                auto typeinfo = object->getTypeInfo();
                WP_ASSERT( typeinfo != 0 );

                auto typeManager = TypeManager::instance();
                WP_ASSERT( typeManager );

                auto className = String( typeManager->getName( typeinfo ) );
                if( StringUtil::isNullOrEmpty( className ) )
                {
                    className = "Untitled";
                }

                /*if (object->isDerived<IStateObject>())
                {
                    className = String("StateObject");
                }
                else
                */
                if( object->isDerived<IStateContext>() )
                {
                    className = String( "StateObject" );
                }
                else if( object->isDerived<IStateListener>() )
                {
                    className = String( "StateListener" );
                }
                else if( object->isDerived<render::IGraphicsScene>() )
                {
                    className = String( "SceneManager" );
                }
                else if( object->isDerived<render::IGraphicsSceneNode>() )
                {
                    className = String( "SceneNode" );
                }
                else if( object->isDerived<render::IGraphicsMesh>() )
                {
                    className = String( "Mesh" );
                }
                else if( object->isDerived<render::IMaterial>() )
                {
                    className = String( "Material" );
                }
                else if( object->isDerived<render::IMaterialTechnique>() )
                {
                    className = String( "Technique" );
                }
                else if( object->isDerived<render::IMaterialPass>() )
                {
                    className = String( "Pass" );
                }
                else if( object->isDerived<render::IMaterialTexture>() )
                {
                    className = object->getName();
                }

                auto treeNode = m_tree->addNode();

                WP_ASSERT( treeNode );
                Util::setText( treeNode, className );

                treeNode->setNodeUserData( data );

                if( node )
                {
                    node->addChild( treeNode );
                }

                auto children = object->getChildObjects();
                for( auto child : children )
                {
                    if( child )
                    {
                        addObjectToTree( child, treeNode );
                    }
                }

                // if (!children.empty())
                //{
                //	m_tree->Expand(treeId);
                // }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    ResourceWindow::TreeCtrlListener::TreeCtrlListener() = default;

    ResourceWindow::TreeCtrlListener::~TreeCtrlListener() = default;

    Parameter ResourceWindow::TreeCtrlListener::handleEvent(
        EventType eventType, hash_type eventValue, const Array<Parameter> &arguments,
        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
    {
        return {};
    }

    void ResourceWindow::TreeCtrlListener::handleTreeSelectionChanged( SmartPtr<ui::IUITreeNode> node )
    {
    }

    void ResourceWindow::TreeCtrlListener::handleTreeSelectionActivated( SmartPtr<ui::IUITreeNode> node )
    {
        auto data = workphone::static_pointer_cast<ProjectTreeData>( node->getNodeUserData() );
        if( data )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto selectionManager = applicationManager->getSelectionManager();
            WP_ASSERT( selectionManager );

            selectionManager->clearSelection();

            auto component = data->getObjectData();
            selectionManager->addSelectedObject( component );

            m_owner->m_propertiesWindow->updateSelection();
        }
    }

    void ResourceWindow::TreeCtrlListener::handleTreeDragStart( SmartPtr<ui::IUITreeNode> node )
    {
    }

    ResourceWindow *ResourceWindow::TreeCtrlListener::getOwner() const
    {
        return m_owner;
    }

    void ResourceWindow::TreeCtrlListener::setOwner( ResourceWindow *owner )
    {
        m_owner = owner;
    }
}  // namespace workphone::editor
