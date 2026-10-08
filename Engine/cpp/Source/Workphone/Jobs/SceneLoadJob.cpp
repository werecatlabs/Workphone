#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Jobs/SceneLoadJob.hpp>
#include <Workphone/System/JobFunction.hpp>
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
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <Workphone/Core/XmlUtil.hpp>
#include <tinyxml.h>

namespace workphone
{
    namespace
    {
        // Older editor copies retained actor/component UUIDs. Repair collisions between
        // root trees in memory, keeping references inside each copied tree together.
        void repairCopiedRootIdentities( const SmartPtr<Properties> &sceneData )
        {
            std::unordered_set<String> seen;
            std::function<void( SmartPtr<Properties>,
                                const std::function<void( SmartPtr<Properties> )> & )>
                visit = [&]( SmartPtr<Properties> data, const auto &action ) {
                    action( data );
                    for( const auto &child : data->getChildren() )
                        visit( child, action );
                };
            // Reserve every serialized identity so generated IDs cannot collide with later roots.
            std::unordered_set<String> reserved;
            visit( sceneData, [&]( const auto &data ) {
                if( data->hasProperty( scene::GameActorUtil::uuidStr ) )
                    reserved.insert(
                        data->getPropertyObject( scene::GameActorUtil::uuidStr ).getValue() );
            } );

            for( const auto &root : sceneData->getChildrenByName( ApplicationUtil::actorsStr ) )
            {
                std::unordered_map<String, String> replacements;
                std::unordered_set<String> local;
                visit( root, [&]( const auto &data ) {
                    if( !data->hasProperty( scene::GameActorUtil::uuidStr ) )
                        return;
                    auto uuid = data->getPropertyObject( scene::GameActorUtil::uuidStr ).getValue();
                    if( uuid.empty() )
                        return;
                    if( !local.insert( uuid ).second )
                        throw std::runtime_error( "Duplicate UUID inside scene actor tree: " + uuid );
                    if( !seen.insert( uuid ).second )
                    {
                        String replacement;
                        do
                        {
                            replacement = StringUtil::getUUID();
                        } while( !reserved.insert( replacement ).second );
                        replacements.emplace( uuid, replacement );
                    }
                } );
                if( replacements.empty() )
                    continue;
                visit( root, [&]( auto data ) {
                    for( auto property : data->getPropertiesAsArray() )
                    {
                        auto found = replacements.find( property.getValue() );
                        if( found != replacements.end() )
                        {
                            property.setValue( found->second );
                            data->setProperty( property );
                        }
                    }
                } );
                WP_LOG( "Repaired copied scene identities in " +
                        root->getPropertyObject( scene::GameActorUtil::labelStr ).getValue() );
            }
        }
    }  // namespace

    WP_CLASS_REGISTER_DERIVED( workphone, SceneLoadJob, Job );

    SceneLoadJob::SceneLoadJob()
    {
        setPrimary( true );
    }

    SceneLoadJob::~SceneLoadJob() = default;

    void SceneLoadJob::queuePrepare()
    {
        auto app = core::IApplicationManager::instancePtr();
        auto prepareJob = app->getFactoryManagerPtr()->make_ptr<JobFunction>();

        SmartPtr<SceneLoadJob> commit( this );
        std::function<void()> preparation = [commit]() mutable {
            if( commit->isInterrupted() )
            {
                commit->setState( IJob::State::Finish );
                return;
            }
            commit->prepare();
            core::IApplicationManager::instancePtr()->getJobQueue()->addJob( commit );
        };

        prepareJob->setFunction( preparation );
        app->getJobQueue()->addJob( prepareJob );
    }

    void SceneLoadJob::prepare()
    {
        if( m_prepared )
            return;

        m_prepared = true;

        try
        {
            auto app = core::IApplicationManager::instancePtr();
            auto factory = app->getFactoryManagerPtr();
            auto path = getFilePath();
            auto inlineData = getDataStr();
            auto projectPath = app->getProjectPath();
            m_preparedPath =
                projectPath.empty() || Path::isPathAbsolute( path )
                    ? StringUtil::cleanupPath( path )
                    : Path::lexically_normal( projectPath, StringUtil::cleanupPath( path ) );
            if( path.empty() && !inlineData.empty() )
                m_preparedPath = String();
            m_preparedLabel = Path::getFileNameWithoutExtension( path );
            auto extension = Path::getFileExtension( m_preparedPath );
            if( inlineData.empty() && extension.empty() )
            {
                m_preparedPath += ApplicationUtil::builtinXmlSceneExt;
                extension = ApplicationUtil::builtinXmlSceneExt;
            }
            const bool binary = extension == ApplicationUtil::builtinBinarySceneExt;
            auto format = extension == ApplicationUtil::builtinXmlSceneExt   ? DataFormat::XML
                          : extension == ApplicationUtil::builtinUsdSceneExt ? DataFormat::USD
                                                                             : DataFormat::JSON;
            if( !inlineData.empty() )
            {
                auto first = inlineData.find_first_not_of( " \t\r\n" );
                format = first != String::npos && inlineData[first] == '<' ? DataFormat::XML
                                                                           : DataFormat::JSON;
            }
            SmartPtr<IStream> stream;
            if( inlineData.empty() )
            {
                auto files = app->getFileSystemPtr();
                stream = files->open( m_preparedPath, true, binary, false, false, false );
                if( !stream )
                    stream = files->open( m_preparedPath, true, binary, false, true, true );
                if( !stream )
                    throw std::runtime_error( "Failed to open scene: " + m_preparedPath );
            }
            auto data = factory->make_ptr<Properties>();
            if( binary && inlineData.empty() )
            {
                const auto size = stream->size();
                Array<u8> bytes( size );
                size_Num total = 0;
                while( total < size )
                {
                    auto count = stream->read( bytes.data() + total, size - total );
                    if( count == 0 )
                        break;
                    total += count;
                }
                String error;
                if( total != size || !PropertiesBinarySerializer::deserialize( bytes, *data, &error ) )
                    throw std::runtime_error( "Invalid binary scene: " + error );
            }
            else
            {
                auto text = inlineData.empty() ? stream->getAsString() : inlineData;
                if( format == DataFormat::JSON )
                {
                    if( !DataUtil::isValidData( text, format ) )
                        throw std::runtime_error( "Invalid JSON scene" );
                }
                else if( format == DataFormat::XML )
                {
                    TiXmlDocument document;
                    document.Parse( text.c_str() );
                    auto root = document.RootElement();
                    if( document.Error() || !root || String( root->Value() ) != XmlUtil::ROOT_ELEMENT ||
                        !root->FirstChildElement( XmlUtil::PROPERTIES_ELEMENT.c_str() ) )
                        throw std::runtime_error( "Invalid XML scene" );
                }
                else
                    throw std::runtime_error( "Unsupported scene format" );
                DataUtil::parse( text, data.get(), format );
            }
            if( inlineData.empty() )
                repairCopiedRootIdentities( data );
            m_preparedData = data;
        }
        catch( const std::exception &e )
        {
            m_prepareError = e.what();
        }
        catch( ... )
        {
            m_prepareError = "Unknown error preparing scene";
        }
    }

    void SceneLoadJob::execute()
    {
        auto target = getScene();
        if( !target )
            return;

        // Preparation is safe off-thread; direct synchronous execution also uses it.
        prepare();

        struct TaskScope
        {
            TaskId previous = Thread::getCurrentTask();
            TaskScope()
            {
                Thread::setCurrentTask( TaskId::None );
            }
            ~TaskScope()
            {
                Thread::setCurrentTask( previous );
            }
        } taskScope;

        auto concrete = workphone::dynamic_pointer_cast<scene::GameScene>( target );
        auto current = [&] { return !concrete || concrete->getLoadGeneration() == m_loadGeneration; };
        if( !current() || !target->isLoaded() )
            return;
        auto app = core::IApplicationManager::instancePtr();
        auto manager = app->getGameManager();

        Array<SmartPtr<scene::IGameActor>> actors;
        auto rollback = [&] {
            for( const auto &actor : actors )
            {
                if( actor && actor->getLoadingState() != LoadingState::Unloaded )
                    manager->destroyActor( actor );
            }
            if( current() )
                target->setSceneLoadingState( scene::IGameScene::SceneLoadingState::Failed );
        };
        try
        {
            if( !m_prepareError.empty() )
                throw std::runtime_error( m_prepareError );
            target->setSceneLoadingState( scene::IGameScene::SceneLoadingState::Loading );
            auto cameraManager = app->getCameraManager();
            auto editorCamera = cameraManager ? cameraManager->getEditorCamera() : nullptr;
            auto editorData = m_preparedData->getChild( "editorCamera" );
            Array<SmartPtr<Properties>> ordered;
            for( const auto &data : m_preparedData->getChildrenByName( ApplicationUtil::actorsStr ) )
            {
                if( scene::GameActorUtil::isEditorCameraData( editorCamera, data ) )
                {
                    if( !editorData )
                        editorData = data;
                }
                else
                    ordered.push_back( data );
            }
            actors = scene::GameActorUtil::loadSceneActors( ordered, target );
            for( const auto &actor : actors )
            {
                if( !current() )
                    throw std::runtime_error( "Scene load superseded during commit" );
                target->addActor( actor );
            }
            if( !current() )
                throw std::runtime_error( "Scene load superseded during commit" );
            auto lighting = getLightingDirector();
            m_preparedData->getPropertyAsType( ApplicationUtil::lightingStr, lighting );
            target->setLightingDirector( lighting );
            if( editorCamera && editorData &&
                ( editorCamera->getScene() == target || manager->getCurrentScene() == target ) )
                scene::GameActorUtil::restoreEditorCameraData( editorCamera, editorData );
            if( !m_preparedPath.empty() )
                target->setFilePath( m_preparedPath );
            if( !m_preparedLabel.empty() )
                target->setLabel( m_preparedLabel );
            auto timer = app->getTimerPtr();
            timer->setSceneLoadTime( timer->now() );
            target->setSceneLoadingState( scene::IGameScene::SceneLoadingState::Loaded );
            if( !current() )
                return;

            app->triggerEvent( EventType::Loading, scene::IGameManager::sceneLoadedHash,
                               Array<Parameter>(), target, target, nullptr, false,
                               Thread::Application_Flag );
        }
        catch( const std::exception &e )
        {
            rollback();
            WP_LOG_EXCEPTION( e );
        }
        catch( ... )
        {
            rollback();
            WP_LOG_ERROR( "Unknown error committing scene" );
        }
    }

    SmartPtr<scene::IGameScene> SceneLoadJob::getScene() const
    {
        return m_scene;
    }

    void SceneLoadJob::setScene( SmartPtr<scene::IGameScene> scene )
    {
        m_scene = scene;
        auto concreteScene = workphone::dynamic_pointer_cast<scene::GameScene>( scene );
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
