#include <EditorPCH.hpp>
#include <jobs/SceneDropJob.hpp>
#include <commands/AddActorCmd.hpp>
#include <commands/DragDropActorCmd.hpp>
#include <editor/EditorManager.hpp>
#include <ui/SceneWindow.hpp>
#include <ui/UIManager.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    const String SceneDropJob::resourceUUIDStr = "resourceUUID";
    const String SceneDropJob::filePathStr = "filePath";
    const String SceneDropJob::prefabExtStr = ".prefab";

    WP_CLASS_REGISTER_DERIVED( workphone, SceneDropJob, Job );

    SceneDropJob::SceneDropJob() = default;

    SceneDropJob::~SceneDropJob() = default;

    void SceneDropJob::execute()
    {
        try
        {
            auto text = getData();
            auto sender = getSender();
            auto tree = getTree();
            auto owner = getOwner();

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            auto commandManager = applicationManager->getCommandManagerPtr();
            WP_ASSERT( commandManager );

            auto fileSystem = applicationManager->getFileSystemPtr();
            WP_ASSERT( fileSystem );

            auto editorManager = EditorManager::getSingletonPtr();

            auto uiManager = editorManager->getUI();

            auto resourceDatabase = applicationManager->getResourceDatabase();

            auto sceneManager = applicationManager->getGameManager();
            auto scene = sceneManager->getCurrentScene();

            auto properties = factoryManager->make_ptr<Properties>();
            auto dataStr = String( text.c_str() );

            DataUtil::parse( dataStr, properties.get() );

            if( properties->hasProperty( resourceUUIDStr ) )
            {
                auto resourceId = properties->getProperty( resourceUUIDStr );
                auto iResourceId = StringUtil::parseUUID( resourceId );

                auto prefabResource = resourceDatabase->loadResource( iResourceId );
                auto prefab = workphone::static_pointer_cast<scene::IGamePrefab>( prefabResource );
                if( prefab )
                {
                    auto actor = prefab->createActor();
                    scene->addActor( actor );
                }

                uiManager->rebuildSceneTree();
            }
            else if( !StringUtil::isNullOrEmpty( dataStr ) )
            {
                auto filePath = properties->getProperty( filePathStr );

                auto dragSrc = tree->getDragSourceElement();
                auto dropDst = tree->getDropDestinationElement();

                if( sender && sender->isDerived<ui::IUIWindow>() )
                {
                    if( !StringUtil::isNullOrEmpty( filePath ) )
                    {
                        auto cmd = factoryManager->make_ptr<AddActorCmd>();
                        cmd->setActorType( AddActorCmd::ActorType::Actor );
                        cmd->setFilePath( filePath );
                        commandManager->addCommand( cmd );
                    }
                    else
                    {
                        if( !owner->getDragDropActorCmd() )
                        {
                            auto cmd = factoryManager->make_ptr<DragDropActorCmd>();
                            cmd->setPosition( Vector2I::zero() );
                            cmd->setSrc( sender );
                            cmd->setDst( nullptr );
                            cmd->setData( text );

                            commandManager->addCommand( cmd );
                        }

                        owner->setDragDropActorCmd( nullptr );
                    }
                }
                else if( dragSrc )
                {
                    if( !commandManager->hasCommand( owner->getDragDropActorCmd() ) )
                    {
                        owner->setDragDropActorCmd( nullptr );
                    }

                    if( !owner->getDragDropActorCmd() )
                    {
                        auto cmd = factoryManager->make_ptr<DragDropActorCmd>();
                        cmd->setPosition( Vector2I::zero() );
                        cmd->setSrc( dragSrc );
                        cmd->setDst( dropDst );
                        cmd->setData( text );
                        cmd->setSiblingIndex( getSiblingIndex() );
                        owner->setDragDropActorCmd( cmd );

                        commandManager->addCommand( cmd );
                    }
                }
                else if( !StringUtil::isNullOrEmpty( filePath ) )
                {
                    auto ext = Path::getFileExtension( filePath );
                    if( ApplicationUtil::isSupportedMesh( filePath ) || ext == prefabExtStr )
                    {
                        auto cmd = factoryManager->make_ptr<AddActorCmd>();
                        cmd->setActorType( AddActorCmd::ActorType::Actor );
                        cmd->setFilePath( filePath );
                        commandManager->addCommand( cmd );
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    String SceneDropJob::getData() const
    {
        return m_data;
    }

    void SceneDropJob::setData( const String &data )
    {
        m_data = data;
    }

    String SceneDropJob::getFilePath() const
    {
        return m_filePath;
    }

    void SceneDropJob::setFilePath( const String &filePath )
    {
        m_filePath = filePath;
    }

    SmartPtr<ui::IUIElement> SceneDropJob::getSender() const
    {
        return m_sender;
    }

    void SceneDropJob::setSender( SmartPtr<ui::IUIElement> sender )
    {
        m_sender = sender;
    }

    SmartPtr<ICommand> SceneDropJob::getDragDropActorCmd() const
    {
        return m_dragDropActorCmd;
    }

    void SceneDropJob::setDragDropActorCmd( SmartPtr<ICommand> dragDropActorCmd )
    {
        m_dragDropActorCmd = dragDropActorCmd;
    }

    SmartPtr<ui::IUITreeCtrl> SceneDropJob::getTree() const
    {
        return m_tree;
    }

    void SceneDropJob::setTree( SmartPtr<ui::IUITreeCtrl> tree )
    {
        m_tree = tree;
    }

    SmartPtr<SceneWindow> SceneDropJob::getOwner() const
    {
        return m_owner;
    }

    void SceneDropJob::setOwner( SmartPtr<SceneWindow> owner )
    {
        m_owner = owner;
    }

    s32 SceneDropJob::getSiblingIndex() const
    {
        return m_siblingIndex;
    }

    void SceneDropJob::setSiblingIndex( s32 siblingIndex )
    {
        m_siblingIndex = siblingIndex;
    }
}  // namespace workphone::editor
