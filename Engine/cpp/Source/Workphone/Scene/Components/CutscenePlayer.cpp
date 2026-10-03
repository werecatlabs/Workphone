#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/CutscenePlayer.hpp>
#include <Workphone/Scene/Components/Camera.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/Scene/IGameScene.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Math/Euler.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{
    const String CutscenePlayer::cutsceneStr = String( "cutscene" );
    const String CutscenePlayer::playOnStartStr = String( "playOnStart" );
    const String CutscenePlayer::loopingStr = String( "looping" );
    const String CutscenePlayer::speedStr = String( "speed" );
    const String CutscenePlayer::currentTimeStr = String( "currentTime" );
    const String CutscenePlayer::playingStr = String( "playing" );
    const String CutscenePlayer::playButtonStr = String( "Play" );
    const String CutscenePlayer::pauseButtonStr = String( "Pause" );
    const String CutscenePlayer::stopButtonStr = String( "Stop" );

    WP_CLASS_REGISTER_DERIVED( workphone::scene, CutscenePlayer, Component );

    CutscenePlayer::CutscenePlayer()
    {
        setObjectFlag( OBJECT_FLAG_GARBAGE_COLLECTED, true );
    }

    CutscenePlayer::~CutscenePlayer() = default;

    void CutscenePlayer::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            Component::load( data );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto gameManager = applicationManager->getGameManager();
            if( gameManager )
            {
                gameManager->registerComponentUpdate( TaskId::Application, Thread::UpdateState::Update,
                                                      this );
            }

            if( m_playOnStart )
            {
                play();
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CutscenePlayer::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( getLoadingState() == LoadingState::Loaded )
            {
                setLoadingState( LoadingState::Unloading );

                auto applicationManager = core::IApplicationManager::instance();
                if( applicationManager )
                {
                    auto sceneManager = applicationManager->getGameManager();
                    if( sceneManager )
                    {
                        sceneManager->unregisterAllComponent( this );
                    }
                }

                m_cutscene = nullptr;

                Component::unload( data );

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CutscenePlayer::update()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            if( !applicationManager )
            {
                return;
            }

            auto timer = applicationManager->getTimerPtr();
            if( !timer )
            {
                return;
            }

            tick( static_cast<f32>( timer->getDeltaTime() ) );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CutscenePlayer::tick( f32 deltaTime )
    {
        if( !m_playing || !m_cutscene )
        {
            return;
        }

        auto length = m_cutscene->getLength();
        if( length <= 0.0f )
        {
            return;
        }

        m_currentTime += deltaTime * m_speed;

        if( m_currentTime > length )
        {
            if( m_cutscene->isLooping() || m_looping )
            {
                m_currentTime = std::fmod( m_currentTime, length );
            }
            else
            {
                m_currentTime = length;
                m_playing = false;
            }
        }

        Map<Pair<String, Cutscene::TrackType>, Cutscene::Keyframe> values;
        m_cutscene->evaluate( m_currentTime, values );

        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager )
        {
            return;
        }

        auto sceneManager = applicationManager->getGameManagerPtr();
        if( !sceneManager )
        {
            return;
        }

        for( const auto &entry : values )
        {
            auto actor = sceneManager->getActorByName( entry.first.first );
            if( actor )
            {
                applyValue( actor, entry.first.second, entry.second );
            }
        }
    }

    void CutscenePlayer::applyValue( SmartPtr<IGameActor> actor, Cutscene::TrackType type,
                                     const Cutscene::Keyframe &value )
    {
        if( !actor )
        {
            return;
        }

        auto transform = actor->getTransformPtr();
        if( !transform )
        {
            return;
        }

        switch( type )
        {
        case Cutscene::TrackType::Position:
        {
            transform->setPosition( value.vectorValue );
            transform->setDirty( true );
        }
        break;

        case Cutscene::TrackType::Rotation:
        {
            constexpr real_Num degToRad = static_cast<real_Num>( 3.14159265358979323846 / 180.0 );
            Euler<real_Num> euler( value.vectorValue.y * degToRad, value.vectorValue.x * degToRad,
                                   value.vectorValue.z * degToRad );
            transform->setOrientation( euler.toQuaternion() );
            transform->setDirty( true );
        }
        break;

        case Cutscene::TrackType::Scale:
        {
            transform->setScale( value.vectorValue );
            transform->setDirty( true );
        }
        break;

        case Cutscene::TrackType::CameraFOV:
        {
            auto cameraComponents = actor->getComponentsByType<Camera>();
            for( auto &camera : cameraComponents )
            {
                if( camera )
                {
                    camera->setFOV( value.scalarValue );
                }
            }
        }
        break;
        }
    }

    SmartPtr<Properties> CutscenePlayer::getProperties() const
    {
        auto properties = Component::getProperties();
        if( !properties )
        {
            return nullptr;
        }

        properties->setProperty( playOnStartStr, m_playOnStart );
        properties->setProperty( loopingStr, m_looping );
        properties->setProperty( speedStr, m_speed );
        properties->setProperty( currentTimeStr, m_currentTime );
        properties->setProperty( playingStr, m_playing );

        if( m_cutscene )
        {
            auto cutsceneNode = m_cutscene->getProperties();
            if( cutsceneNode )
            {
                cutsceneNode->setName( cutsceneStr );
                properties->addChild( cutsceneNode );
            }
        }

        return properties;
    }

    void CutscenePlayer::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
        {
            return;
        }

        Component::setProperties( properties );

        properties->getPropertyValue( playOnStartStr, m_playOnStart );
        properties->getPropertyValue( loopingStr, m_looping );
        properties->getPropertyValue( speedStr, m_speed );
        properties->getPropertyValue( currentTimeStr, m_currentTime );
        properties->getPropertyValue( playingStr, m_playing );

        if( auto cutsceneNode = properties->getChild( cutsceneStr ) )
        {
            auto cutscene = workphone::make_ptr<Cutscene>();
            cutscene->setProperties( cutsceneNode );
            m_cutscene = cutscene;
        }

        if( properties->isButtonPressed( playButtonStr ) )
        {
            play();
        }
        else if( properties->isButtonPressed( pauseButtonStr ) )
        {
            pause();
        }
        else if( properties->isButtonPressed( stopButtonStr ) )
        {
            stop();
        }
    }

    void CutscenePlayer::setCutscene( SmartPtr<Cutscene> cutscene )
    {
        m_cutscene = cutscene;
    }

    SmartPtr<Cutscene> CutscenePlayer::getCutscene() const
    {
        return m_cutscene;
    }

    void CutscenePlayer::play()
    {
        m_playing = true;
    }

    void CutscenePlayer::pause()
    {
        m_playing = false;
    }

    void CutscenePlayer::stop()
    {
        m_playing = false;
        m_currentTime = 0.0f;
    }

    bool CutscenePlayer::isPlaying() const
    {
        return m_playing;
    }

    void CutscenePlayer::setSpeed( f32 speed )
    {
        m_speed = speed;
    }

    f32 CutscenePlayer::getSpeed() const
    {
        return m_speed;
    }

    void CutscenePlayer::setLooping( bool looping )
    {
        m_looping = looping;
    }

    bool CutscenePlayer::isLooping() const
    {
        return m_looping;
    }

    void CutscenePlayer::setPlayOnStart( bool playOnStart )
    {
        m_playOnStart = playOnStart;
    }

    bool CutscenePlayer::getPlayOnStart() const
    {
        return m_playOnStart;
    }

    void CutscenePlayer::setCurrentTime( f32 time )
    {
        m_currentTime = time;
    }

    f32 CutscenePlayer::getCurrentTime() const
    {
        return m_currentTime;
    }
}  // namespace workphone::scene
