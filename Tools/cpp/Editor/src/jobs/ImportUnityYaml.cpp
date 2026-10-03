#include <EditorPCH.hpp>
#include "jobs/ImportUnityYaml.hpp"
#include "editor/EditorManager.hpp"
#include <ui/UIManager.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    ImportUnityYaml::ImportUnityYaml() = default;

    ImportUnityYaml::~ImportUnityYaml() = default;

    void ImportUnityYaml::execute()
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto fileSystem = applicationManager->getFileSystem();

        auto editorManager = EditorManager::getSingletonPtr();
        auto uiManager = editorManager->getUI();

        auto resourceDatabase = applicationManager->getResourceDatabase();
        auto factoryManager = applicationManager->getFactoryManager();

        if( auto fileDialog = fileSystem->openFileDialog() )
        {
            auto projectPath = Path::getWorkingDirectory();
            if( !fileSystem->isExistingFolder( projectPath ) )
            {
                projectPath = "";
            }

            fileDialog->setDialogMode( INativeFileDialog::DialogMode::Open );
            fileDialog->setFileExtension( ".unity" );
            fileDialog->setFilePath( projectPath );

            auto result = fileDialog->openDialog();
            if( result == INativeFileDialog::Result::Dialog_Okay )
            {
                auto filePath = fileDialog->getFilePath();
                if( !StringUtil::isNullOrEmpty( filePath ) )
                {
                    auto sceneManager = applicationManager->getGameManager();
                    auto scene = sceneManager->getCurrentScene();
                    auto fileSystem = applicationManager->getFileSystem();

                    auto root = sceneManager->createActor();
                    root->setName( "Root" );
                    scene->addActor( root );

                    char buffer[4096];
                    std::ostringstream stream;
                    auto data = fileSystem->open( filePath );
                    while( !data->eof() )
                    {
                        if( data->readLine( buffer, 4096 ) > 0 )
                        {
                            auto s = std::string( buffer );
                            if( s.find( "--- " ) != String::npos )
                            {
                                if( data->readLine( buffer, 4096 ) > 0 )
                                {
                                    stream << buffer << std::endl;

                                    stream << "  obj_def:  " << s << std::endl;
                                }
                            }
                            else
                            {
                                stream << buffer << std::endl;
                            }
                        }
                    }

                    auto properties = workphone::make_ptr<Properties>();
                    DataUtil::parse( stream.str().c_str(), properties.get(), DataFormat::YAML );

                    auto gameObjects = properties->getChildrenByName( "GameObject" );
                    for( auto gameObject : gameObjects )
                    {
                        auto definition = gameObject->getProperty( "obj_def" );
                        auto definitions = StringUtil::split( definition );

                        auto actor = sceneManager->createActor();

                        auto name = gameObject->getProperty( "m_Name" );
                        actor->setName( name );

                        root->addChild( actor );

                        auto handle = actor->getHandle();

                        if( handle )
                        {
                            auto classUUID = StringUtil::parseUUID( definitions[1] );
                            handle->setClassId( classUUID );

                            auto fileID = StringUtil::replaceAll( definitions[2], "&", "" );

                            auto fileUUID = StringUtil::parseUUID( fileID );
                            handle->setFileId( fileUUID );
                        }
                    }

                    resourceDatabase->refresh();

                    auto actors = sceneManager->getActors();
                    for( auto actor : actors )
                    {
                        auto handle = actor->getHandle();

                        auto transforms = properties->getChildrenByName( "Transform" );
                        for( auto transform : transforms )
                        {
                            auto transformDefinition = transform->getProperty( "obj_def" );
                            auto transformDefinitions = StringUtil::split( transformDefinition );

                            if( auto gameObject = transform->getChild( "m_GameObject" ) )
                            {
                                auto fileID = gameObject->getProperty( "fileID" );
                                auto fileUUID = StringUtil::parseUUID( fileID );

                                if( handle->getFileId() == fileUUID )
                                {
                                    if( auto t = actor->getTransform() )
                                    {
                                        if( auto transformHandle = t->getHandle() )
                                        {
                                            auto classUUD =
                                                StringUtil::parseUUID( transformDefinitions[1] );
                                            transformHandle->setClassId( classUUD );

                                            auto transformFileID = StringUtil::replaceAll(
                                                transformDefinitions[2], "&", "" );

                                            auto fileUUID = StringUtil::parseUUID( transformFileID );
                                            transformHandle->setFileId( fileUUID );
                                        }

                                        auto localRotationProperties =
                                            transform->getChild( "m_LocalRotation" );
                                        auto localPositionProperties =
                                            transform->getChild( "m_LocalPosition" );
                                        auto localScaleProperties =
                                            transform->getChild( "m_LocalScale" );

                                        auto localRotation = QuaternionF::identity();
                                        auto localPosition = Vector3F::zero();
                                        auto localScale = Vector3F::unit();

                                        DataUtil::parse( localRotationProperties, localRotation );
                                        DataUtil::parse( localPositionProperties, localPosition );
                                        DataUtil::parse( localScaleProperties, localScale );

                                        t->setLocalOrientation( localRotation );
                                        t->setLocalPosition( localPosition );
                                        t->setScale( localScale );
                                    }
                                }
                            }
                        }

                        auto rectTransforms = properties->getChildrenByName( "RectTransform" );
                        for( auto transform : rectTransforms )
                        {
                            auto transformDefinition = transform->getProperty( "obj_def" );
                            auto transformDefinitions = StringUtil::split( transformDefinition );

                            if( auto gameObject = transform->getChild( "m_GameObject" ) )
                            {
                                auto fileID = gameObject->getProperty( "fileID" );
                                auto fileUUID = StringUtil::parseUUID( fileID );

                                if( handle->getFileId() == fileUUID )
                                {
                                    if( auto t = actor->getTransform() )
                                    {
                                        if( auto transformHandle = t->getHandle() )
                                        {
                                            auto classUUID =
                                                StringUtil::parseUUID( transformDefinitions[1] );
                                            transformHandle->setClassId( classUUID );

                                            auto transformFileID = StringUtil::replaceAll(
                                                transformDefinitions[2], "&", "" );

                                            auto fileUUID = StringUtil::parseUUID( transformFileID );
                                            transformHandle->setFileId( fileUUID );
                                        }

                                        auto rectTransform =
                                            actor->getComponent<scene::LayoutTransform>();
                                        if( !rectTransform )
                                        {
                                            rectTransform =
                                                actor->addComponent<scene::LayoutTransform>();

                                            auto anchoredPositionProperties =
                                                transform->getChild( "m_AnchoredPosition" );
                                            auto sizeDeltaProperties =
                                                transform->getChild( "m_SizeDelta" );
                                            auto anchorProperties = transform->getChild( "m_Pivot" );
                                            auto anchorMinProperties =
                                                transform->getChild( "m_AnchorMin" );
                                            auto anchorMaxProperties =
                                                transform->getChild( "m_AnchorMax" );

                                            auto anchoredPosition = Vector2F::zero();
                                            auto sizeDelta = Vector2F::unit();
                                            auto anchor = Vector2F::zero();
                                            auto anchorMin = Vector2F::zero();
                                            auto anchorMax = Vector2F::zero();

                                            DataUtil::parse( anchoredPositionProperties,
                                                             anchoredPosition );
                                            DataUtil::parse( sizeDeltaProperties, sizeDelta );

                                            DataUtil::parse( anchorProperties, anchor );
                                            DataUtil::parse( anchorMinProperties, anchorMin );
                                            DataUtil::parse( anchorMaxProperties, anchorMax );

                                            if( anchorMin.Y() > anchor.Y() )
                                            {
                                                anchoredPosition.Y() =
                                                    anchoredPosition.Y() - sizeDelta.Y();
                                            }
                                            else
                                            {
                                                anchoredPosition.Y() = -anchoredPosition.Y();
                                            }

                                            anchor.Y() = 1.0f - anchor.Y();
                                            anchorMin.Y() = 1.0f - anchorMin.Y();
                                            anchorMax.Y() = 1.0f - anchorMax.Y();

                                            rectTransform->setPosition( anchoredPosition );
                                            rectTransform->setSize( sizeDelta );
                                            rectTransform->setAnchor( anchor );
                                            rectTransform->setAnchorMin( anchorMin );
                                            rectTransform->setAnchorMax( anchorMax );
                                        }
                                    }
                                }
                            }
                        }
                    }

                    auto monoBehaviours = properties->getChildrenByName( "MonoBehaviour" );
                    for( auto monoBehaviour : monoBehaviours )
                    {
                        auto scriptProperties = monoBehaviour->getChild( "m_Script" );
                        auto scriptFileID = scriptProperties->getProperty( "fileID" );
                        auto scriptGuid = scriptProperties->getProperty( "guid" );

                        auto gameObjectProperties = monoBehaviour->getChild( "m_GameObject" );
                        if( gameObjectProperties )
                        {
                            auto parentGameObjectId = gameObjectProperties->getProperty( "fileID" );

                            auto parentActor = resourceDatabase->getObjectTypeByFileId<scene::IGameActor>(
                                parentGameObjectId );
                            if( parentActor )
                            {
                                auto pComponent =
                                    factoryManager->createObjectFromType<scene::IComponent>(
                                        scriptGuid );
                                if( !pComponent )
                                {
                                    auto componentTypeClean =
                                        sceneManager->getComponentFactoryType( scriptGuid );
                                    pComponent = factoryManager->createObjectFromType<scene::IComponent>(
                                        componentTypeClean );
                                }

                                if( pComponent )
                                {
                                    if( pComponent->isDerived<scene::Text>() )
                                    {
                                        auto textComponent =
                                            workphone::static_pointer_cast<scene::Text>( pComponent );
                                        auto text = monoBehaviour->getProperty( "m_Text" );
                                        textComponent->setText( text );
                                    }

                                    parentActor->addComponentInstance( pComponent );

                                    pComponent->setActor( parentActor );
                                    pComponent->load( nullptr );
                                }
                            }
                        }
                    }

                    auto transforms = properties->getChildrenByName( "Transform" );
                    for( auto transform : transforms )
                    {
                        auto gameObjectProperties = transform->getChild( "m_GameObject" );
                        if( gameObjectProperties )
                        {
                            auto parentGameObjectId = gameObjectProperties->getProperty( "fileID" );
                            auto parentActor = resourceDatabase->getObjectTypeByFileId<scene::IGameActor>(
                                parentGameObjectId );
                            if( parentActor )
                            {
                                auto children = transform->getChildrenByName( "m_Children" );
                                for( auto child : children )
                                {
                                    auto childrenProperties = child->getChildrenByName( "m_Children" );
                                    for( auto childProperties : childrenProperties )
                                    {
                                        auto fileID = childProperties->getProperty( "fileID" );
                                        if( !StringUtil::isNullOrEmpty( fileID ) )
                                        {
                                            auto actor =
                                                resourceDatabase
                                                    ->getObjectTypeByFileId<scene::IGameActor>(
                                                    fileID );
                                            if( actor )
                                            {
                                                parentActor->addChild( actor );
                                            }

                                            auto transform =
                                                resourceDatabase
                                                    ->getObjectTypeByFileId<scene::ITransform>( fileID );
                                            if( transform )
                                            {
                                                if( auto a = transform->getActor() )
                                                {
                                                    if( auto p = a->getParent() )
                                                    {
                                                        p->removeChild( a );
                                                    }

                                                    parentActor->addChild( a );
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    auto rectTransforms = properties->getChildrenByName( "RectTransform" );
                    for( auto transform : rectTransforms )
                    {
                        auto gameObjectProperties = transform->getChild( "m_GameObject" );
                        if( gameObjectProperties )
                        {
                            auto parentGameObjectId = gameObjectProperties->getProperty( "fileID" );
                            auto parentActor = resourceDatabase->getObjectTypeByFileId<scene::IGameActor>(
                                parentGameObjectId );
                            if( parentActor )
                            {
                                auto children = transform->getChildrenByName( "m_Children" );
                                for( auto child : children )
                                {
                                    auto childrenProperties = child->getChildrenByName( "m_Children" );
                                    for( auto childProperties : childrenProperties )
                                    {
                                        auto fileID = childProperties->getProperty( "fileID" );
                                        if( !StringUtil::isNullOrEmpty( fileID ) )
                                        {
                                            auto actor =
                                                resourceDatabase->getObjectTypeByFileId<scene::IGameActor>(
                                                    fileID );
                                            if( actor )
                                            {
                                                parentActor->addChild( actor );
                                            }

                                            auto transform =
                                                resourceDatabase
                                                    ->getObjectTypeByFileId<scene::ITransform>( fileID );
                                            if( transform )
                                            {
                                                if( auto a = transform->getActor() )
                                                {
                                                    if( auto p = a->getParent() )
                                                    {
                                                        p->removeChild( a );
                                                    }

                                                    parentActor->addChild( a );
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    uiManager->rebuildSceneTree();

                    //root->edit();
                }
            }
        }
    }
}  // namespace workphone::editor
