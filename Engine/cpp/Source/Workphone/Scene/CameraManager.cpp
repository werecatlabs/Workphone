#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/CameraManager.hpp>
#include <Workphone/Scene/Components/Camera.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/IGameScene.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/System/ITask.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/Jobs/CameraManagerReset.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, CameraManager, ICameraManager );

    const String CameraManager::editorTextureStr = String( "EditorTexture" );
    const String CameraManager::editorCameraStr = String( "EditorCamera" );
    const String CameraManager::camerasStr = String( "Cameras" );
    const String CameraManager::resetStr = String( "Reset" );

    CameraManager::CameraManager()
    {
        static const auto name = String( "CameraManager" );
        setName( name );
    }

    CameraManager::~CameraManager() = default;

    void CameraManager::load( SmartPtr<ISharedObject> data )
    {
        if( isLoaded() )
            return;

        setLoadingState( LoadingState::Loading );

        m_cameras.reserve( 12 );

        setLoadingState( LoadingState::Loaded );
    }

    void CameraManager::unload( SmartPtr<ISharedObject> data )
    {
        if( !isLoaded() )
            return;

        setLoadingState( LoadingState::Unloading );

        m_editorCamera = nullptr;
        m_cameras.clear();

        setLoadingState( LoadingState::Unloaded );
    }

    void CameraManager::update()
    {
        if( !isEnabled() )
            return;

        auto task = Thread::getCurrentTask();
        switch( task )
        {
        case TaskId::Application:
        {
            if( isLoaded() )
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                WP_ASSERT( applicationManager );

                switch( m_state )
                {
                case State::Edit:
                {
                    if( auto editorCamera = getEditorCameraPtr() )
                    {
                        editorCamera->update();
                    }
                }
                break;
                case State::Play:
                {
                    // The editor camera lives outside the game scene. Continue
                    // processing its component transitions while it is selected.
                    if( applicationManager->isEditorCamera() )
                    {
                        if( auto editorCamera = getEditorCameraPtr() )
                            editorCamera->update();
                    }

                    auto sceneManager = applicationManager->getGameManagerPtr();
                    if( !sceneManager )
                        break;

                    auto scene = sceneManager->getCurrentScenePtr();
                    if( !scene )
                        break;

                    auto cameras = scene->getComponents<Camera>();
                    for( auto &camera : cameras )
                    {
                        if( auto cameraActor = camera->getActorPtr() )
                        {
                            cameraActor->update();
                        }
                    }
                }
                break;
                default:
                {
                    if( applicationManager->isEditorCamera() )
                    {
                        if( auto editorCamera = getEditorCameraPtr() )
                        {
                            editorCamera->update();
                        }
                    }

                    auto sceneManager = applicationManager->getGameManagerPtr();
                    if( !sceneManager )
                        break;

                    auto scene = sceneManager->getCurrentScenePtr();
                    if( !scene )
                        break;

                    auto cameras = scene->getComponents<Camera>();
                    for( auto &camera : cameras )
                    {
                        if( auto cameraActor = camera->getActorPtr() )
                        {
                            cameraActor->update();
                        }
                    }
                }
                break;
                }
            }
        }
        break;
        default:
        {
        }
        }
    }

    void CameraManager::addCamera( SmartPtr<IGameActor> camera )
    {
        // Restoring camera components can register the same actor again before
        // its previous component finishes unloading.
        if( camera && std::find( m_cameras.begin(), m_cameras.end(), camera ) == m_cameras.end() )
        {
            m_cameras.push_back( camera );
        }
    }

    auto CameraManager::removeCamera( SmartPtr<IGameActor> camera ) -> bool
    {
        if( camera )
        {
            m_cameras.erase( std::remove( m_cameras.begin(), m_cameras.end(), camera ),
                             m_cameras.end() );

            return true;
        }

        return false;
    }

    auto CameraManager::findCamera( const String &name ) const -> SmartPtr<IGameActor>
    {
        auto cameras = getCameras();
        for( auto &camera : cameras )
        {
            if( camera->getName() == name )
            {
                return camera;
            }
        }

        return nullptr;
    }

    auto CameraManager::getCameras() const -> Array<SmartPtr<IGameActor>>
    {
        return m_cameras.snapshot();
    }

    void CameraManager::reset()
    {
        if( !isEnabled() )
            return;

        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto gameManager = applicationManager->getGameManagerPtr();
        if( !gameManager )
            return;

        auto scene = gameManager->getCurrentScenePtr();
        if( !scene )
            return;

        auto cameras = scene->getComponents<Camera>();

        if( applicationManager->isEditorCamera() )
        {
            for( auto &camera : cameras )
            {
                camera->setActive( false );
            }

            if( auto editorCamera = getEditorCamera() )
            {
                auto editorCameras = editorCamera->getAllComponentsAndInChildren<Camera>();
                for( auto &camera : editorCameras )
                {
                    camera->setActive( camera == editorCameras.front() );
                }

                if( applicationManager->isEditor() )
                {
                    auto state = applicationManager->isPlaying() ? IGameActor::State::Play
                                                                 : IGameActor::State::Edit;
                    editorCamera->setState( state );
                }
            }
        }
        else
        {
            if( auto editorCamera = getEditorCamera() )
            {
                // Older scene restoration could leave multiple camera components
                // on the editor actor. None may remain visible during game play.
                for( auto &camera : editorCamera->getAllComponentsAndInChildren<Camera>() )
                {
                    camera->setActive( false );
                }
            }

            if( !cameras.empty() )
            {
                for( auto &camera : cameras )
                {
                    camera->setActive( false );
                }

                for( auto &camera : cameras )
                {
                    auto cameraActor = camera->getActor();
                    if( !cameraActor->getFlag( IGameActor::ActorFlagIsEditor ) )
                    {
                        auto enable = camera->isEnabled() && cameraActor->isEnabledInScene();
                        camera->setActive( enable );

                        auto rtt = getEditorRTT();
                        camera->setTargetTexture( rtt );
                    }
                }
            }
        }

        applicationManager->triggerEvent( EventType::Scene, IEvent::cameraManagerReset,
                                          Array<Parameter>(), this, this, nullptr );
    }

    IGameActor *CameraManager::getEditorCameraPtr() const
    {
        return m_editorCamera.get();
    }

    auto CameraManager::getEditorCamera() const -> SmartPtr<IGameActor>
    {
        return m_editorCamera;
    }

    void CameraManager::setEditorCamera( SmartPtr<IGameActor> editorCamera )
    {
        m_editorCamera = editorCamera;
    }

    auto CameraManager::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = ICameraManager::getProperties();
        auto editorCamera = getEditorCamera();

        properties->setProperty( editorCameraStr, editorCamera );
        properties->setProperty( camerasStr, m_cameras.snapshot() );
        properties->setButtonPressed( resetStr );

        return properties;
    }

    void CameraManager::setProperties( SmartPtr<Properties> properties )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto taskManager = applicationManager->getTaskManager();
        auto editorCamera = getEditorCamera();

        if( properties )
        {
            properties->getPropertyValue( editorCameraStr, editorCamera );
            setEditorCamera( editorCamera );

            auto cameras = m_cameras.snapshot();
            properties->getPropertyValue( camerasStr, cameras );

            m_cameras = { cameras.begin(), cameras.end() };

            if( properties->isButtonPressed( resetStr ) )
            {
                auto applicationTask = taskManager->getTask( TaskId::Application );
                auto cameraManagerResetJob = workphone::make_ptr<CameraManagerReset>();
                applicationTask->addJob( cameraManagerResetJob );
            }
        }
    }

    auto CameraManager::getChildObjects() const -> Array<SmartPtr<ISharedObject>>
    {
        Array<SmartPtr<ISharedObject>> objects;
        //objects.emplace_back( getEditorCamera() );

        auto cameras = getCameras();
        for( auto camera : cameras )
        {
            objects.emplace_back( camera );
        }

        return objects;
    }

    bool CameraManager::isEditorCameraEnabled() const
    {
        if( auto editorCamera = getEditorCamera() )
        {
            auto camera = editorCamera->getComponent<Camera>();
            if( camera )
            {
                return camera->isActive();
            }
        }

        return false;
    }

    void CameraManager::setState( State state )
    {
        if( m_state == state )
            return;

        m_state = state;

        if( state == State::Edit || state == State::Play || state == State::Reset )
        {
            reset();
        }
    }

    ICameraManager::State CameraManager::getState() const
    {
        return m_state;
    }

    void CameraManager::setEnabled( bool enabled )
    {
        m_enabled = enabled;
    }

    bool CameraManager::isEnabled() const
    {
        return m_enabled;
    }

    void CameraManager::lock()
    {
        m_mutex.lock();
    }

    bool CameraManager::try_lock()
    {
        return m_mutex.try_lock();
    }

    void CameraManager::unlock()
    {
        m_mutex.unlock();
    }

    void CameraManager::setEditorRTT( SmartPtr<render::ITexture> editorRTT )
    {
        m_editorRTT = editorRTT;
    }

    SmartPtr<render::ITexture> CameraManager::getEditorRTT() const
    {
        return m_editorRTT;
    }

}  // namespace workphone::scene
