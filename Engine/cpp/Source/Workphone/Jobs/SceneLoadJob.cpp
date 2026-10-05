#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Jobs/SceneLoadJob.hpp>
#include <Workphone/Jobs/ActorLoadJob.hpp>
#include <Workphone/ApplicationUtil.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Path.hpp>
#include <Workphone/Core/PropertiesBinarySerializer.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Interface/IO/IStream.hpp>
#include <Workphone/Interface/Scene/ICameraManager.hpp>
#include <Workphone/Interface/Scene/IGamePrefabManager.hpp>
#include <Workphone/Interface/Scene/IGameScene.hpp>
#include <Workphone/Interface/System/IJobQueue.hpp>
#include <Workphone/Interface/System/IThreadPool.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Scene/Directors/LightingDirector.hpp>
#include <Workphone/Scene/GameScene.hpp>
#include <Workphone/Scene/GameActorUtil.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, SceneLoadJob, Job );

    SceneLoadJob::SceneLoadJob() { setPrimary( true ); }

    SceneLoadJob::~SceneLoadJob() = default;

    void SceneLoadJob::execute()
    {
        try
        {
            auto scene = getScene();
            if( !scene ) return;
            
            ScopedLock sceneLock( scene.get() );

            auto concreteScene = workphone::dynamic_pointer_cast<scene::GameScene>(scene);
            if( concreteScene && concreteScene->getLoadGeneration() != m_loadGeneration ) return;
            scene->setSceneLoadingState( scene::IGameScene::SceneLoadingState::None );

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            auto path = getFilePath();
            auto sceneFilePath = StringUtil::cleanupPath( path );

            auto projectPath = applicationManager->getProjectPath();
            auto scenePath = projectPath.empty() ? sceneFilePath
                                                 : Path::lexically_normal( projectPath, sceneFilePath );

            scene->setFilePath( scenePath );

            auto jobQueue = applicationManager->getJobQueuePtr();
            WP_ASSERT( jobQueue );

            auto timer = applicationManager->getTimerPtr();
            WP_ASSERT( timer );

            auto taskManager = applicationManager->getTaskManagerPtr();
            WP_ASSERT( taskManager );

            auto fileSystem = applicationManager->getFileSystemPtr();
            WP_ASSERT( fileSystem );

            auto prefabManager = applicationManager->getPrefabManager();

            auto sceneFileExt = Path::getFileExtension( scenePath );
            if( StringUtil::isNullOrEmpty( sceneFileExt ) )
            {
                scenePath += ApplicationUtil::builtinXmlSceneExt;
                sceneFileExt = ApplicationUtil::builtinXmlSceneExt;
            }

            const auto isBinaryScene = sceneFileExt == ApplicationUtil::builtinBinarySceneExt;

            auto format = DataFormat::JSON;
            if( sceneFileExt == ApplicationUtil::builtinXmlSceneExt )
            {
                format = DataFormat::XML;
            }
            else if( sceneFileExt == ApplicationUtil::builtinUsdSceneExt )
            {
                format = DataFormat::USD;
            }

            auto inlineData = getDataStr();
            auto stream = inlineData.empty()
                ? fileSystem->open( scenePath, true, isBinaryScene, false, false, false )
                : SmartPtr<IStream>();
            if( !stream && inlineData.empty() )
            {
                stream = fileSystem->open( scenePath, true, isBinaryScene, false, true, true );
            }

            if( !stream && inlineData.empty() )
            {
                WP_LOG_ERROR( "Failed to open scene file: " + scenePath );
                return;
            }

            if( stream || !inlineData.empty() )
            {
                auto name = Path::getFileNameWithoutExtension( path );
                scene->setLabel( name );

                auto dataString = getDataStr();
                auto sceneData = factoryManager->make_ptr<Properties>();
                if( isBinaryScene && inlineData.empty() )
                {
                    const auto streamSize = stream->size();
                    auto binaryData = Array<u8>( streamSize );
                    size_Num totalRead = 0;
                    while( totalRead < streamSize )
                    {
                        const auto bytesRead =
                            stream->read( binaryData.data() + totalRead, streamSize - totalRead );
                        if( bytesRead == 0 )
                        {
                            break;
                        }
                        totalRead += bytesRead;
                    }

                    String parseError;
                    if( totalRead != streamSize ||
                        !PropertiesBinarySerializer::deserialize( binaryData, *sceneData, &parseError ) )
                    {
                        WP_LOG_ERROR( "Failed to parse binary scene '" + scenePath + "': " +
                                      ( totalRead == streamSize
                                            ? parseError
                                            : String( "Unexpected end of file." ) ) );
                        return;
                    }
                }
                else
                {
                    auto dataStr = dataString.empty() ? stream->getAsString() : dataString;
                    DataUtil::parse( dataStr, sceneData.get(), format );
                }

                auto lightingDirector = getLightingDirector();

                auto cameraManager = applicationManager->getCameraManager();
                auto editorCamera = cameraManager ? cameraManager->getEditorCamera() : nullptr;
                auto editorCameraData = sceneData->getChild( "editorCamera" );
                auto actorsData = sceneData->getChildrenByName( ApplicationUtil::actorsStr );
                Array<SmartPtr<Properties>> orderedData;
                for( const auto &actorData : actorsData ) {
                    if( scene::GameActorUtil::isEditorCameraData(editorCamera, actorData) ) {
                        if( !editorCameraData ) editorCameraData = actorData;
                    } else orderedData.push_back(actorData);
                }
                auto actors = scene::GameActorUtil::loadSceneActors(orderedData);
                for( const auto &actor : actors ) scene->addActor(actor);
                sceneData->getPropertyAsType(ApplicationUtil::lightingStr, lightingDirector);
                scene->setLightingDirector(lightingDirector);

                if( editorCamera && editorCameraData )
                {
                    scene::GameActorUtil::restoreEditorCameraData( editorCamera, editorCameraData );
                }

            }

            auto nowTime = timer->now();
            timer->setSceneLoadTime( nowTime );

            scene->setSceneLoadingState( scene::IGameScene::SceneLoadingState::Loaded );
            Array<Parameter> args;
            applicationManager->triggerEvent( EventType::Loading, scene::IGameManager::sceneLoadedHash,
                                              args, scene, scene, nullptr, false,
                                              Thread::Application_Flag );

        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<scene::IGameScene> SceneLoadJob::getScene() const
    {
        return m_scene;
    }

    void SceneLoadJob::setScene( SmartPtr<scene::IGameScene> scene )
    {
        m_scene = scene;
        auto concreteScene = workphone::dynamic_pointer_cast<scene::GameScene>(scene);
        m_loadGeneration = concreteScene ? concreteScene->getLoadGeneration() : 0;
    }

    String SceneLoadJob::getFilePath() const
    {
        return m_filePath;
    }

    void SceneLoadJob::setFilePath( const String &filePath )
    {
        m_filePath = filePath;
    }

    SmartPtr<scene::LightingDirector> SceneLoadJob::getLightingDirector() const
    {
        return m_lightingDirector;
    }

    void SceneLoadJob::setLightingDirector( SmartPtr<scene::LightingDirector> lightingDirector )
    {
        m_lightingDirector = lightingDirector;
    }

    bool SceneLoadJob::getCreateActorJobs() const
    {
        return m_createActorJobs;
    }

    void SceneLoadJob::setCreateActorJobs( bool createActorJobs )
    {
        m_createActorJobs = createActorJobs;
    }

    String SceneLoadJob::getDataStr() const
    {
        return m_dataStr;
    }

    void SceneLoadJob::setDataStr( const String &dataStr )
    {
        m_dataStr = dataStr;
    }

}  //  namespace workphone
