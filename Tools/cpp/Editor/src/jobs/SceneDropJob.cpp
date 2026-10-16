#include <EditorPCH.hpp>
#include <jobs/SceneDropJob.hpp>
#include <commands/AddActorCmd.hpp>
#include <commands/DragDropActorCmd.hpp>
#include <editor/EditorManager.hpp>
#include <ui/ProjectTreeData.hpp>
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
            auto destination = getDestination();

            auto applicationManager = core::IApplicationManager::instancePtr();
            if( !applicationManager )
            {
                return;
            }

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            if( !factoryManager )
            {
                return;
            }

            auto commandManager = applicationManager->getCommandManagerPtr();

            auto editorManager = EditorManager::getSingletonPtr();
            auto uiManager = editorManager ? editorManager->getUI() : nullptr;

            auto resourceDatabase = applicationManager->getResourceDatabase();

            auto sceneManager = applicationManager->getGameManager();
            auto scene = sceneManager ? sceneManager->getCurrentScene() : nullptr;
            if( !commandManager || !scene || !uiManager || StringUtil::isNullOrEmpty( text ) )
            {
                return;
            }

            auto properties = factoryManager->make_ptr<Properties>();
            auto dataStr = String( text.c_str() );

            DataUtil::parse( dataStr, properties.get() );

            SmartPtr<scene::IGameActor> destinationActor;
            if( auto destinationNode = dynamic_pointer_cast<ui::IUITreeNode>( destination ) )
            {
                auto nodeData =
                    dynamic_pointer_cast<ProjectTreeData>( destinationNode->getNodeUserData() );
                auto target = nodeData ? nodeData->getObjectData() : nullptr;
                destinationActor = dynamic_pointer_cast<scene::IGameActor>( target );
                if( !destinationActor && target.get() != scene.get() )
                {
                    return;
                }
            }

            if( properties->hasProperty( resourceUUIDStr ) )
            {
                auto resourceId = properties->getProperty( resourceUUIDStr );
                auto iResourceId = StringUtil::parseUUID( resourceId );

                auto prefabResource = resourceDatabase ? resourceDatabase->loadResource( iResourceId )
                                                       : nullptr;
                auto prefab = workphone::dynamic_pointer_cast<scene::IGamePrefab>( prefabResource );
                if( prefab )
                {
                    auto actor = prefab->createActor();
                    if( actor )
                    {
                        auto cmd = factoryManager->make_ptr<AddActorCmd>();
                        cmd->setActorType( AddActorCmd::ActorType::Actor );
                        cmd->setActor( actor );
                        cmd->setParent( destinationActor );
                        cmd->setUseSelectionAsParent( false );
                        cmd->setExpectedScene( scene );
                        commandManager->addCommand( cmd );
                    }
                }
            }
            else if( !StringUtil::isNullOrEmpty( dataStr ) )
            {
                auto filePath = properties->getProperty( filePathStr );

                if( !StringUtil::isNullOrEmpty( filePath ) )
                {
                    auto ext = Path::getFileExtension( filePath );
                    if( ApplicationUtil::isSupportedMesh( filePath ) || ext == prefabExtStr )
                    {
                        auto cmd = factoryManager->make_ptr<AddActorCmd>();
                        cmd->setActorType( AddActorCmd::ActorType::Actor );
                        cmd->setFilePath( filePath );
                        cmd->setUseSelectionAsParent( false );
                        cmd->setParent( destinationActor );
                        cmd->setExpectedScene( scene );
                        commandManager->addCommand( cmd );
                    }
                }
                else if( properties->hasProperty( "actorUUID" ) )
                {
                    auto actorUUID = properties->getProperty( "actorUUID" );
                    SmartPtr<scene::IGameActor> actor;
                    auto pending = scene->getActors();
                    while( !pending.empty() && !actor )
                    {
                        auto candidate = pending.back();
                        pending.pop_back();
                        if( !candidate )
                        {
                            continue;
                        }

                        auto handle = candidate->getHandle();
                        if( handle && handle->getUUIDAsString() == actorUUID )
                        {
                            actor = candidate;
                            break;
                        }

                        auto children = candidate->getChildren();
                        pending.insert( pending.end(), children.begin(), children.end() );
                    }
                    if( actor )
                    {
                        for( auto ancestor = destinationActor; ancestor;
                             ancestor = ancestor->getParent() )
                        {
                            if( ancestor == actor )
                            {
                                return;
                            }
                        }
                        if( actor->getParent() == destinationActor && getSiblingIndex() < 0 )
                        {
                            return;
                        }

                        auto cmd = factoryManager->make_ptr<DragDropActorCmd>();
                        cmd->setActor( actor );
                        cmd->setDestinationActor( destinationActor );
                        cmd->setSiblingIndex( getSiblingIndex() );
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

    SmartPtr<ui::IUIElement> SceneDropJob::getDestination() const
    {
        return m_destination;
    }

    void SceneDropJob::setDestination( SmartPtr<ui::IUIElement> destination )
    {
        m_destination = destination;
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
