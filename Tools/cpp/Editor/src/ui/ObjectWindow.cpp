#include <EditorPCH.hpp>
#include <ui/ObjectWindow.hpp>
#include <ui/ActorWindow.hpp>
#include <ui/MaterialWindow.hpp>
#include <ui/PropertiesWindow.hpp>
#include <ui/FileViewWindow.hpp>
#include <ui/ResourceWindow.hpp>
#include <ui/TerrainWindow.hpp>
#include <ui/UIManager.hpp>
#include <editor/EditorManager.hpp>
#include <Workphone/Workphone.hpp>
#include <EditorTypes.hpp>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone, ObjectWindow, EditorWindow );

    ObjectWindow::ObjectWindow() = default;

    ObjectWindow::ObjectWindow( SmartPtr<ui::IUIWindow> parent )
    {
        setParent( parent );
    }

    ObjectWindow::~ObjectWindow()
    {
        unload( nullptr );
    }

    void ObjectWindow::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto ui = applicationManager->getUI();
            WP_ASSERT( ui );

            auto editorManager = EditorManager::getSingletonPtr();
            auto editorUI = editorManager->getUI();

            auto parent = getParent();

            auto parentWindow = ui->addElementByType<ui::IUIWindow>();
            if( parentWindow )
            {
#if 0
                parentWindow->setLabel( "Object" );
                parentWindow->setHasBorder( true );
                setParentWindow( parentWindow );
                setWindow( parentWindow );

                if( parent )
                {
                    parent->addChild( parentWindow );
                }

                // Production inspector chrome: quick actions, lock/follow concepts, and a dedicated
                // content container for the specialist inspectors below. The buttons are deliberately
                // lightweight UI affordances here; existing specialist windows still own their data logic.
                auto inspectorToolbar = ui->addElementByType<ui::IUIWindow>();
                inspectorToolbar->setLabel( "Inspector Toolbar | Selection, Lock, Prefab, Overrides" );
                inspectorToolbar->setHasBorder( true );
                parentWindow->addChild( inspectorToolbar );

                auto addInspectorButton = [&]( const String &label, bool sameLine ) {
                    auto button = ui->addElementByType<ui::IUIButton>();
                    button->setLabel( label );
                    button->setSameLine( sameLine );
                    inspectorToolbar->addChild( button );
                    return button;
                };

                addInspectorButton( "Lock", false );
                addInspectorButton( "Follow Selection", true );
                addInspectorButton( "Prefab", true );
                addInspectorButton( "Overrides", true );
                addInspectorButton( "Add Component", true );
                addInspectorButton( "Reset", true );

                auto inspectorBody = ui->addElementByType<ui::IUIWindow>();
                inspectorBody->setLabel( "Inspector Details" );
                inspectorBody->setHasBorder( true );
                parentWindow->addChild( inspectorBody );

                auto inspectorParent = inspectorBody ? inspectorBody : parentWindow;

                auto actorWindow = workphone::make_ptr<ActorWindow>( inspectorParent );
                m_actorWindow = actorWindow;
                editorUI->setActorWindow( actorWindow );
                actorWindow->load( data );

                auto materialWindow = factoryManager->make_ptr<ScriptWindow>();
                m_materialWindow = materialWindow;
                materialWindow->setClassName( "MaterialEditor" );
                materialWindow->setParent( inspectorParent );
                materialWindow->load( nullptr );
                materialWindow->setWindowVisible( false );

                auto fileViewWindow = workphone::make_ptr<FileViewWindow>( inspectorParent );
                m_fileViewWindow = fileViewWindow;
                fileViewWindow->load( data );

                auto resourceWindow = workphone::make_ptr<ResourceWindow>( inspectorParent );
                m_resourceWindow = resourceWindow;
                resourceWindow->load( data );

                auto propertiesWindow = workphone::make_ptr<PropertiesWindow>();
                m_propertiesWindow = propertiesWindow;
                propertiesWindow->setParent( inspectorParent );
                propertiesWindow->load( data );
#else
                parentWindow->setLabel( "Object" );
                setParentWindow( parentWindow );

                if( parent )
                {
                    parent->addChild( parentWindow );
                }

                auto actorWindow = workphone::make_ptr<ActorWindow>( parentWindow );
                m_actorWindow = actorWindow;
                editorUI->setActorWindow( actorWindow );
                actorWindow->load( data );

                auto materialWindow = factoryManager->make_ptr<ScriptWindow>();
                m_materialWindow = materialWindow;
                materialWindow->setClassName( "MaterialEditor" );
                materialWindow->setParent( parentWindow );
                materialWindow->load( nullptr );
                materialWindow->setWindowVisible( false );

                auto fileViewWindow = workphone::make_ptr<FileViewWindow>( parentWindow );
                m_fileViewWindow = fileViewWindow;
                fileViewWindow->load( data );

                auto resourceWindow = workphone::make_ptr<ResourceWindow>( parentWindow );
                m_resourceWindow = resourceWindow;
                resourceWindow->load( data );

                auto propertiesWindow = workphone::make_ptr<PropertiesWindow>();
                m_propertiesWindow = propertiesWindow;
                propertiesWindow->setParent( parentWindow );
                propertiesWindow->load( data );
#endif

                //terrainWindow->setWindowVisible( true );

                m_actorWindow->setWindowVisible( false );
                m_materialWindow->setWindowVisible( false );
                m_fileViewWindow->setWindowVisible( false );
                m_resourceWindow->setWindowVisible( false );
                m_propertiesWindow->setWindowVisible( false );
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ObjectWindow::reload( SmartPtr<ISharedObject> data )
    {
        if( m_actorWindow )
        {
            m_actorWindow->reload( data );
        }

        if( m_materialWindow )
        {
            m_materialWindow->reload( data );
        }

        if( m_fileViewWindow )
        {
            m_fileViewWindow->reload( data );
        }

        if( m_resourceWindow )
        {
            m_resourceWindow->reload( data );
        }

        EditorWindow::reload( data );
    }

    void ObjectWindow::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            auto editorManager = EditorManager::getSingletonPtr();
            auto editorUI = editorManager ? editorManager->getUI() : nullptr;

            if( editorUI && m_actorWindow && editorUI->getActorWindow() == m_actorWindow )
            {
                editorUI->setActorWindow( nullptr );
            }

            if( m_actorWindow )
            {
                m_actorWindow->unload( nullptr );
                m_actorWindow = nullptr;
            }

            if( m_materialWindow )
            {
                m_materialWindow->unload( nullptr );
                m_materialWindow = nullptr;
            }

            if( m_fileViewWindow )
            {
                m_fileViewWindow->unload( nullptr );
                m_fileViewWindow = nullptr;
            }

            if( m_resourceWindow )
            {
                m_resourceWindow->unload( nullptr );
                m_resourceWindow = nullptr;
            }

            if( m_propertiesWindow )
            {
                m_propertiesWindow->unload( nullptr );
                m_propertiesWindow = nullptr;
            }

            if( auto parentWindow = getParentWindow() )
            {
                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                if( auto ui = applicationManager->getUI() )
                {
                    ui->removeElement( parentWindow );
                }

                setParentWindow( nullptr );
            }

            m_window = nullptr;

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

    SmartPtr<ui::IUIWindow> ObjectWindow::getWindow() const
    {
        return m_window;
    }

    void ObjectWindow::setWindow( SmartPtr<ui::IUIWindow> window )
    {
        m_window = window;
    }

    void ObjectWindow::updateSelection()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto selectionManager = applicationManager->getSelectionManager();
        WP_ASSERT( selectionManager );

        auto selection = selectionManager->getSelection();
        if( !selection.empty() )
        {
            auto selected = selection.front();

            if( selected->isDerived<scene::IGameActor>() )
            {
                m_objectType = ObjectType::Actor;
            }
            else if( selected->isDerived<scene::IComponent>() )
            {
                // Components use the actor inspector's specialist panes, including Lua materials.
                m_objectType = ObjectType::Actor;
            }
            else if( selected->isDerived<FileSelection>() )
            {
                auto fileSelection = workphone::static_pointer_cast<FileSelection>( selected );
                auto filePath = fileSelection->getFilePath();
                auto fileExt = Path::getFileExtension( filePath );
                fileExt = StringUtil::make_lower( fileExt );

                m_objectType = ObjectType::Resource;

                if( fileExt == ".mat" )
                {
                    m_resourceType = ObjectType::Material;
                    //m_resourceType = ObjectWindow::ObjectType::Resource;
                }
                else if( ApplicationUtil::isSupportedMesh( filePath ) )
                {
                    m_resourceType = ObjectType::Mesh;
                }
                else if( ApplicationUtil::isSupportedTexture( filePath ) )
                {
                    m_resourceType = ObjectType::Texture;
                }
                else if( ApplicationUtil::isSupportedSound( filePath ) )
                {
                    m_resourceType = ObjectType::Sound;
                }
                else if( fileExt == ".resource" )
                {
                    m_resourceType = ObjectType::Resource;
                }
                else if( fileExt == ".lightingpreset" )
                {
                    m_resourceType = ObjectType::Resource;
                }
                else
                {
                    m_resourceType = ObjectType::FileUnknown;
                    m_objectType = ObjectType::FileUnknown;
                }
            }
            else if( selected->isDerived<render::IMaterial>() )
            {
                m_resourceType = ObjectType::Material;
                m_objectType = ObjectType::Resource;
            }
            else if( selected->isDerived<IResource>() )
            {
                m_resourceType = ObjectType::Resource;
                m_objectType = ObjectType::Resource;
            }
            else if( selected->isDerived<ISharedObject>() )
            {
                m_resourceType = ObjectType::SharedObject;
                m_objectType = ObjectType::SharedObject;
            }
        }

        switch( m_objectType )
        {
        case ObjectType::None:
        {
        }
        break;
        case ObjectType::Actor:
        {
            if( m_actorWindow )
            {
                m_actorWindow->setWindowVisible( true );
            }

            if( m_materialWindow )
            {
                m_materialWindow->setWindowVisible( false );
            }

            if( m_fileViewWindow )
            {
                m_fileViewWindow->setWindowVisible( false );
            }

            if( m_resourceWindow )
            {
                m_resourceWindow->setWindowVisible( false );
            }

            if( m_actorWindow )
            {
                m_actorWindow->updateSelection();
            }
        }
        break;
        case ObjectType::Resource:
        {
            switch( m_resourceType )
            {
            case ObjectType::None:
            {
            }
            break;
            case ObjectType::Actor:
            {
            }
            break;
            case ObjectType::SharedObject:
            {
                if( m_propertiesWindow )
                {
                    m_propertiesWindow->setWindowVisible( true );
                }

                if( m_actorWindow )
                {
                    m_actorWindow->setWindowVisible( false );
                }

                if( m_materialWindow )
                {
                    m_materialWindow->setWindowVisible( false );
                }

                if( m_fileViewWindow )
                {
                    m_fileViewWindow->setWindowVisible( false );
                }

                if( m_resourceWindow )
                {
                    m_resourceWindow->setWindowVisible( false );
                }

                if( m_propertiesWindow )
                {
                    m_propertiesWindow->updateSelection();
                }
            }
            break;
            case ObjectType::Resource:
            {
                if( m_propertiesWindow )
                {
                    m_propertiesWindow->setWindowVisible( false );
                }

                if( m_actorWindow )
                {
                    m_actorWindow->setWindowVisible( false );
                }

                if( m_materialWindow )
                {
                    m_materialWindow->setWindowVisible( false );
                }

                if( m_fileViewWindow )
                {
                    m_fileViewWindow->setWindowVisible( false );
                }

                if( m_resourceWindow )
                {
                    m_resourceWindow->setWindowVisible( true );
                }

                if( m_resourceWindow )
                {
                    m_resourceWindow->updateSelection();
                }
            }
            break;
            case ObjectType::FileUnknown:
            {
                if( m_propertiesWindow )
                {
                    m_propertiesWindow->setWindowVisible( false );
                }

                if( m_actorWindow )
                {
                    m_actorWindow->setWindowVisible( false );
                }

                if( m_materialWindow )
                {
                    m_materialWindow->setWindowVisible( false );
                }

                if( m_fileViewWindow )
                {
                    m_fileViewWindow->setWindowVisible( true );
                }

                if( m_resourceWindow )
                {
                    m_resourceWindow->setWindowVisible( false );
                }

                if( m_fileViewWindow )
                {
                    m_fileViewWindow->updateSelection();
                }
            }
            break;
            case ObjectType::Mesh:
            {
                if( m_propertiesWindow )
                {
                    m_propertiesWindow->setWindowVisible( false );
                }

                if( m_actorWindow )
                {
                    m_actorWindow->setWindowVisible( false );
                }

                if( m_materialWindow )
                {
                    m_materialWindow->setWindowVisible( false );
                }

                if( m_fileViewWindow )
                {
                    m_fileViewWindow->setWindowVisible( false );
                }

                if( m_resourceWindow )
                {
                    m_resourceWindow->setWindowVisible( true );
                }

                if( m_actorWindow )
                {
                    m_resourceWindow->updateSelection();
                }
            }
            break;
            case ObjectType::Material:
            {
                if( m_propertiesWindow )
                {
                    m_propertiesWindow->setWindowVisible( false );
                }

                if( m_actorWindow )
                {
                    m_actorWindow->setWindowVisible( false );
                }

                if( m_materialWindow )
                {
                    m_materialWindow->setWindowVisible( true );
                }

                if( m_fileViewWindow )
                {
                    m_fileViewWindow->setWindowVisible( false );
                }

                if( m_resourceWindow )
                {
                    m_resourceWindow->setWindowVisible( false );
                }

                if( m_materialWindow )
                {
                    m_materialWindow->updateSelection();
                }
            }
            break;
            case ObjectType::MaterialNode:
            {
            }
            break;
            case ObjectType::Terrain:
            {
                if( m_propertiesWindow )
                {
                    m_propertiesWindow->setWindowVisible( false );
                }

                if( m_actorWindow )
                {
                    m_actorWindow->setWindowVisible( false );
                }

                if( m_materialWindow )
                {
                    m_materialWindow->setWindowVisible( false );
                }

                if( m_fileViewWindow )
                {
                    m_fileViewWindow->setWindowVisible( false );
                }

                if( m_resourceWindow )
                {
                    m_resourceWindow->setWindowVisible( false );
                }

                if( m_materialWindow )
                {
                    m_materialWindow->updateSelection();
                }
            }
            break;
            case ObjectType::Sound:
            case ObjectType::Texture:
            {
                if( m_propertiesWindow )
                {
                    m_propertiesWindow->setWindowVisible( false );
                }

                if( m_actorWindow )
                {
                    m_actorWindow->setWindowVisible( false );
                }

                if( m_materialWindow )
                {
                    m_materialWindow->setWindowVisible( false );
                }

                if( m_fileViewWindow )
                {
                    m_fileViewWindow->setWindowVisible( false );
                }

                if( m_resourceWindow )
                {
                    m_resourceWindow->setWindowVisible( true );
                }

                if( m_actorWindow )
                {
                    m_resourceWindow->updateSelection();
                }
            }
            break;
            default:
            {
            }
            break;
            }
        }
        break;
        case ObjectType::Mesh:
        {
        }
        break;
        case ObjectType::Material:
        {
        }
        break;
        case ObjectType::MaterialNode:
        {
        }
        break;
        case ObjectType::SharedObject:
        {
            if( m_propertiesWindow )
            {
                m_propertiesWindow->setWindowVisible( true );
            }

            if( m_actorWindow )
            {
                m_actorWindow->setWindowVisible( false );
            }

            if( m_materialWindow )
            {
                m_materialWindow->setWindowVisible( false );
            }

            if( m_fileViewWindow )
            {
                m_fileViewWindow->setWindowVisible( false );
            }

            if( m_resourceWindow )
            {
                m_resourceWindow->setWindowVisible( false );
            }

            if( m_propertiesWindow )
            {
                m_propertiesWindow->updateSelection();
            }
        }
        break;
        default:
        {
        }
        break;
        }
    }
}  // namespace workphone::editor
