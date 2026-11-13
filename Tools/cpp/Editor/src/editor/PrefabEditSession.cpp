#include <EditorPCH.hpp>
#include <editor/PrefabEditSession.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Scene/GameScene.hpp>
#include <Workphone/Scene/GameActorUtil.hpp>
#include <Workphone/Scene/PrefabInstanceService.hpp>
#include <Workphone/Scene/Components/Camera/SphericalCameraController.hpp>
#include <Workphone/Database/AssetDocumentSave.hpp>
#include <Workphone/System/CommandManagerMT.hpp>
#include <Workphone/System/TaskLock.hpp>
#include <filesystem>
#include <unordered_set>
#include <stdexcept>

namespace workphone::editor
{
    namespace
    {
        const String stageTag = "__WorkphonePrefabEditing";

        struct EditLocks
        {
            std::unique_ptr<TaskLock> render, physics, application;
            explicit EditLocks( SmartPtr<core::IApplicationManager> app )
            {
                if( auto tasks = app->getTaskManager() )
                {
                    const auto hold = [&]( TaskId id ) {
                        auto task = tasks->getTask( id );
                        return task && task->isLoaded() ? std::make_unique<TaskLock>( task ) : nullptr;
                    };
                    render = hold( TaskId::Render );
                    physics = hold( TaskId::Physics );
                    application = hold( TaskId::Application );
                }
            }
        };

        void collectActors( SmartPtr<scene::IGameActor> actor,
                            Array<SmartPtr<scene::IGameActor>> &actors )
        {
            actors.push_back( actor );
            for( auto child : actor->getChildren() ) collectActors( child, actors );
        }

        void removeStageTag( SmartPtr<Properties> data )
        {
            Array<String> tags;
            data->getPropertyValue( "tags", tags );
            tags.erase( std::remove( tags.begin(), tags.end(), stageTag ), tags.end() );
            data->setProperty( "tags", tags );
            for( auto child : data->getChildren() )
                if( child->getName() == "child" || child->getName() == "children" )
                    removeStageTag( child );
        }

        void normalizeEditingState( SmartPtr<Properties> data )
        {
            // Lifecycle state is transient. Serialized Play must not call onPlay
            // while an author is merely opening a prefab in the Inspector.
            if( data->hasProperty( "componentType" ) )
                data->setProperty( "state", static_cast<s32>( scene::IComponent::State::Edit ) );
            for( auto child : data->getChildren() ) normalizeEditingState( child );
        }
    }

    struct PrefabEditSession::Data
    {
        struct ActorState
        {
            SmartPtr<scene::IGameActor> actor;
            u32 flags = 0, previousFlags = 0;
        };
        struct Registration
        {
            SmartPtr<scene::IComponent> component;
            TaskId task;
            Thread::UpdateState phase;
        };
        SmartPtr<scene::IGameScene> original, stage;
        SmartPtr<scene::IGameActor> edited, editorCamera;
        SmartPtr<Properties> cameraData;
        SmartPtr<ISharedObject> recoveryData;
        SmartPtr<ICommandManager> originalCommands, stageCommands;
        Array<SmartPtr<ISharedObject>> selection;
        Array<ActorState> actorStates;
        Array<Registration> registrations;
        String sourcePath, sourceHash;
        DataFormat format = DataFormat::JSON;
        bool suspended = false, editorCameraEnabled = true;

        void suspend( SmartPtr<scene::IGameManager> manager )
        {
            if( suspended ) return;
            actorStates.clear();
            registrations.clear();
            Array<SmartPtr<scene::IGameActor>> actors;
            for( auto actor : original->getActors() ) collectActors( actor, actors );
            std::unordered_set<scene::IComponent *> components;
            for( auto actor : actors )
            {
                if( actor == editorCamera ) continue;
                actorStates.push_back( { actor, actor->getFlags(), actor->getPreviousFlags() } );
                for( auto component : actor->getComponents() ) components.insert( component.get() );
            }
            // Admit the complete restoration inventory before changing any scene state.
            for( u32 p = 0; p < static_cast<u32>( Thread::UpdateState::Count ); ++p )
                for( u32 t = 0; t < static_cast<u32>( TaskId::Count ); ++t )
                {
                    auto phase = static_cast<Thread::UpdateState>( p );
                    auto task = static_cast<TaskId>( t );
                    for( auto component : manager->getRegisteredComponents( phase, task ).snapshot() )
                        if( components.count( component.get() ) )
                            registrations.push_back( { component, task, phase } );
                }
            suspended = true;
            for( auto &state : actorStates ) state.actor->setEnabled( false, false );
            for( auto &state : actorStates ) state.actor->updateVisibility();
            for( const auto &entry : registrations )
                manager->unregisterComponentUpdate( entry.task, entry.phase, entry.component );
        }

        void resume( SmartPtr<scene::IGameManager> manager )
        {
            if( !suspended ) return;
            for( auto &state : actorStates )
            {
                state.actor->setFlags( state.flags );
                state.actor->setPreviousFlags( state.previousFlags );
            }
            for( auto &state : actorStates ) state.actor->updateVisibility();
            for( const auto &entry : registrations )
                manager->registerComponentUpdate( entry.task, entry.phase, entry.component );
            suspended = false;
        }

        void captureRecovery()
        {
            auto data = dynamic_pointer_cast<Properties>( original->toData() );
            if( !data ) throw std::runtime_error( "Unable to preserve the authoring scene" );
            // The editor camera is shared with the viewport. Its prefab-view orbit
            // must not leak into the original scene's recovery document.
            if( cameraData )
            {
                data->removeChild( "editorCamera" );
                data->addChild( cameraData );
            }
            recoveryData = data;
        }

        void destroyStage( SmartPtr<scene::IGameManager> manager )
        {
            if( !stage ) return;
            const auto roots = stage->getActors();
            if( edited && std::find( roots.begin(), roots.end(), edited ) == roots.end() && !edited->getParent() )
                manager->destroyActor( edited );
            for( auto actor : roots )
                if( actor != editorCamera ) manager->destroyActor( actor );
            stage->unload( nullptr );
            edited = nullptr;
            stage = nullptr;
        }
    };

    PrefabEditSession::PrefabEditSession() = default;
    PrefabEditSession::~PrefabEditSession()
    {
        String error;
        if( m_data ) close( error );
    }

    bool PrefabEditSession::open( const String &path, String &error )
    {
        error.clear();
        if( m_data ) { error = "Return from the current prefab before opening another asset."; return false; }
        auto app = core::IApplicationManager::instance();
        if( !app || !app->isEditor() || app->isPlaying() )
        { error = "Stop Play before editing a prefab."; return false; }
        EditLocks locks( app );
        auto manager = app->getGameManager();
        auto files = app->getFileSystem();
        auto prefabs = app->getPrefabManager();
        auto original = manager ? manager->getCurrentScene() : nullptr;
        if( !manager || !files || !prefabs || !original || !original->isLoaded() )
        { error = "Load an editor scene before opening a prefab."; return false; }
        ScopedLock managerLock( manager );
        ScopedLock graphicsLock( app->getGraphicsSystem() );
        const auto sceneState = original->getSceneLoadingState();
        if( sceneState != scene::IGameScene::SceneLoadingState::Loaded &&
            !( sceneState == scene::IGameScene::SceneLoadingState::None && original->getFilePath().empty() ) )
        { error = "Wait for the current scene to finish loading before editing a prefab."; return false; }
        auto candidate = std::make_unique<Data>();
        try
        {
            const auto fullPath = Path::isPathAbsolute( path ) ? Path::lexically_normal( path )
                : Path::lexically_normal( app->getProjectPath(), path );
            const auto extension = StringUtil::make_lower( Path::getFileExtension( fullPath ) );
            if( extension != ".prefab" && extension != ".fbprefab" )
                throw std::runtime_error( "Select a prefab asset to edit." );
            const auto initialHash = files->getFileHash( fullPath );
            const auto text = files->readAllText( fullPath );
            if( text.empty() ) throw std::runtime_error( "The prefab asset is empty or unavailable." );
            candidate->sourcePath = fullPath;
            candidate->sourceHash = files->getFileHash( fullPath );
            if( candidate->sourceHash.empty() ) throw std::runtime_error( "Cannot verify the prefab asset." );
            if( initialHash != candidate->sourceHash )
                throw std::runtime_error( "The prefab changed while it was opening. Open it again." );
            const auto first = text.find_first_not_of( " \t\r\n" );
            candidate->format = first != String::npos && text[first] == '<' ? DataFormat::XML : DataFormat::JSON;
            auto source = make_ptr<Properties>();
            DataUtil::parse( text, source.get(), candidate->format );
            if( !source->hasProperty( "label" ) && !source->hasProperty( "name" ) )
                throw std::runtime_error( "The selected document is not an actor prefab." );
            candidate->original = original;
            candidate->originalCommands = app->getCommandManager();
            candidate->stageCommands = make_ptr<CommandManagerMT>();
            if( auto selected = app->getSelectionManager() ) candidate->selection = selected->getSelection();
            candidate->editorCameraEnabled = app->isEditorCamera();
            if( auto cameras = app->getCameraManager() ) candidate->editorCamera = cameras->getEditorCamera();
            if( candidate->editorCamera )
            {
                candidate->cameraData = dynamic_pointer_cast<Properties>( candidate->editorCamera->toData() );
                candidate->cameraData->setName( "editorCamera" );
            }
            candidate->captureRecovery();
            auto stage = make_ptr<scene::GameScene>();
            stage->setEditorPrefabScene( true );
            stage->load( nullptr );
            if( !stage->isLoaded() ) throw std::runtime_error( "Unable to allocate the prefab editing scene." );
            candidate->stage = stage;
            stage->setLabel( "Prefab: " + Path::getFileName( fullPath ) );
            auto instance = scene::PrefabInstanceService::createLinkedInstanceData( source, fullPath );
            normalizeEditingState( instance );
            auto actors = scene::GameActorUtil::loadSceneActors( { instance }, stage );
            if( actors.size() != 1 ) throw std::runtime_error( "The prefab has no editable root actor." );
            candidate->edited = actors.front();
            stage->addActor( candidate->edited );
            candidate->suspend( manager );
            manager->setCurrentScene( stage );
            app->setCommandManager( candidate->stageCommands );
            app->setEditorCamera( true );
            stage->setSceneLoadingState( scene::IGameScene::SceneLoadingState::Loaded );
            stage->setState( scene::IGameScene::State::Edit );
            if( candidate->editorCamera )
            {
                const auto bounds = scene::GameActorUtil::getActorAABB( candidate->edited );
                const auto center = bounds.isValid() && bounds.isFinite() ? bounds.getCenter() : candidate->edited->getPosition();
                const auto distance = bounds.isValid() && bounds.isFinite()
                    ? std::max( static_cast<real_Num>( 3 ), bounds.getExtent().length() * static_cast<real_Num>( 1.5 ) )
                    : static_cast<real_Num>( 6 );
                const auto position = center + Vector3<real_Num>( distance, distance * static_cast<real_Num>( 0.65 ), distance );
                if( auto orbit = candidate->editorCamera->getComponent<scene::SphericalCameraController>() )
                {
                    orbit->setTargetPosition( center );
                    orbit->setPosition( position );
                }
                candidate->editorCamera->setPosition( position );
                candidate->editorCamera->lookAt( center );
                candidate->editorCamera->updateTransform();
            }
            if( auto selected = app->getSelectionManager() )
            {
                selected->clearSelection();
                selected->addSelectedObject( candidate->edited );
            }
            m_data = std::move( candidate );
            return true;
        }
        catch( const std::exception &exception )
        {
            error = exception.what();
            const auto recover = [&]( auto action ) {
                try { action(); }
                catch( const std::exception &failure ) { error += " Recovery: " + String( failure.what() ); }
            };
            if( candidate->original )
            {
                manager->setCurrentScene( candidate->original );
                recover( [&] { candidate->resume( manager ); } );
                app->setCommandManager( candidate->originalCommands );
                app->setEditorCamera( candidate->editorCameraEnabled );
                recover( [&] {
                    if( candidate->editorCamera && candidate->cameraData )
                        scene::GameActorUtil::restoreEditorCameraData( candidate->editorCamera, candidate->cameraData );
                    if( auto selected = app->getSelectionManager() )
                    {
                        selected->clearSelection();
                        for( auto object : candidate->selection ) selected->addSelectedObject( object );
                    }
                } );
            }
            recover( [&] { candidate->destroyStage( manager ); } );
            return false;
        }
    }

    bool PrefabEditSession::save( String &error, size_t *updatedInstances )
    {
        error.clear();
        if( updatedInstances ) *updatedInstances = 0;
        if( !m_data ) { error = "No prefab is being edited."; return false; }
        auto app = core::IApplicationManager::instance();
        if( !app || app->isPlaying() ) { error = "Stop Play before saving a prefab."; return false; }
        EditLocks locks( app );
        auto manager = app->getGameManager();
        auto files = app->getFileSystem();
        ScopedLock managerLock( manager );
        ScopedLock graphicsLock( app->getGraphicsSystem() );
        auto &data = *m_data;
        std::unique_ptr<scene::PrefabUpdateTransaction> transaction;
        bool published = false;
        const auto previousRecovery = data.recoveryData;
        try
        {
            auto roots = data.stage->getActors();
            if( roots.size() != 1 || roots.front() != data.edited || data.edited->getParent() )
                throw std::runtime_error( "Keep one prefab root. Move added actors under the prefab before saving." );
            if( files->getFileHash( data.sourcePath ) != data.sourceHash )
                throw std::runtime_error( "The prefab changed on disk. Return and reopen it before saving." );
            auto source = scene::PrefabInstanceService::makeTemplateData( data.edited );
            removeStageTag( source );
            const auto encoded = DataUtil::toString( source.get(), true, data.format );
            data.resume( manager );
            manager->setCurrentScene( data.original );
            transaction = scene::PrefabInstanceService::prepareUpdates( data.original, data.sourcePath, source );
            transaction->commit();
            // Prepare recovery while rollback is still available, before publishing bytes.
            data.captureRecovery();
            data.suspend( manager );
            manager->setCurrentScene( data.stage );
            if( files->getFileHash( data.sourcePath ) != data.sourceHash )
                throw std::runtime_error( "The prefab changed on disk while saving. Return and reopen it before saving." );
            if( !saveAssetDocumentText( data.sourcePath, source, encoded, error ) )
                throw std::runtime_error( error.c_str() );
            published = true;
            data.sourceHash = files->getFileHash( data.sourcePath );
            if( updatedInstances ) *updatedInstances = transaction->changedCount();
            transaction->finish();
            return true;
        }
        catch( const std::exception &exception )
        {
            error = exception.what();
            const auto recover = [&]( auto action ) {
                try { action(); }
                catch( const std::exception &failure ) { error += " Recovery: " + String( failure.what() ); }
            };
            if( transaction && !published )
            {
                recover( [&] { data.resume( manager ); } );
                manager->setCurrentScene( data.original );
                recover( [&] { transaction->rollback(); } );
                data.recoveryData = previousRecovery;
            }
            recover( [&] { data.suspend( manager ); } );
            manager->setCurrentScene( data.stage );
            // A cleanup error after publication cannot undo an already saved asset.
            if( published ) { error = "Prefab saved; cleanup warning: " + error; return true; }
            return false;
        }
    }

    bool PrefabEditSession::close( String &error )
    {
        error.clear();
        if( !m_data ) return true;
        auto app = core::IApplicationManager::instance();
        if( !app || !app->getGameManager() )
        { error = "The editor scene manager is unavailable."; return false; }
        EditLocks locks( app );
        auto manager = app->getGameManager();
        ScopedLock managerLock( manager );
        ScopedLock graphicsLock( app->getGraphicsSystem() );
        try
        {
            auto &data = *m_data;
            if( auto selected = app->getSelectionManager() ) selected->clearSelection();
            manager->setCurrentScene( data.original );
            data.destroyStage( manager );
            data.resume( manager );
            app->setCommandManager( data.originalCommands );
            app->setEditorCamera( data.editorCameraEnabled );
            if( data.editorCamera && data.cameraData )
                scene::GameActorUtil::restoreEditorCameraData( data.editorCamera, data.cameraData );
            if( auto selected = app->getSelectionManager() )
                for( auto object : data.selection )
                    if( object )
                    {
                        // Project browser selections need not have a loaded resource.
                        // Removed instance actors/components, however, must not be selected.
                        const bool sceneObject = dynamic_pointer_cast<scene::IGameActor>( object ) ||
                            dynamic_pointer_cast<scene::IComponent>( object );
                        if( !sceneObject || object->isLoaded() ) selected->addSelectedObject( object );
                    }
            m_data.reset();
            return true;
        }
        catch( const std::exception &exception ) { error = exception.what(); return false; }
    }

    bool PrefabEditSession::active() const { return m_data != nullptr; }
    String PrefabEditSession::path() const { return m_data ? m_data->sourcePath : String(); }
    SmartPtr<scene::IGameActor> PrefabEditSession::root() const { return m_data ? m_data->edited : nullptr; }
    SmartPtr<scene::IGameScene> PrefabEditSession::authoringScene() const { return m_data ? m_data->original : nullptr; }
    SmartPtr<ISharedObject> PrefabEditSession::authoringData() const { return m_data ? m_data->recoveryData : nullptr; }
}
