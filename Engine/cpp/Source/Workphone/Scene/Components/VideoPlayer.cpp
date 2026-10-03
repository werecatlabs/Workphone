#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/VideoPlayer.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Interface/Graphics/IVideoManager.hpp>
#include <Workphone/Interface/Graphics/IVideo.hpp>
#include <Workphone/Interface/Graphics/IVideoTexture.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Core/Parameter.hpp>

namespace workphone::scene
{

    WP_CLASS_REGISTER_DERIVED( workphone::scene, VideoPlayer, Component );

    const String VideoPlayer::videoPathStr = "videoPath";
    const String VideoPlayer::autoPlayStr = "autoPlay";
    const String VideoPlayer::loopStr = "loop";
    const String VideoPlayer::autoUpdateStr = "autoUpdate";
    const String VideoPlayer::playStr = "Play";
    const String VideoPlayer::stopStr = "Stop";
    const String VideoPlayer::pauseStr = "Pause";

    VideoPlayer::VideoPlayer()
    {
    }

    VideoPlayer::~VideoPlayer()
    {
    }

    void VideoPlayer::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );
            Component::load( data );

            // Initialize video player resources here
            createVideo();

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void VideoPlayer::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            // Cleanup video resources
            destroyVideo();

            Component::unload( data );
            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void VideoPlayer::play()
    {
        if( m_video )
        {
            m_video->play();
            m_isPlaying = true;
        }
    }

    void VideoPlayer::stop()
    {
        if( m_video )
        {
            m_video->stop();
            m_isPlaying = false;
        }
    }

    void VideoPlayer::pause()
    {
        if( m_video )
        {
            m_video->stop();
            m_isPlaying = false;
        }
    }

    bool VideoPlayer::isPlaying() const
    {
        return m_isPlaying;
    }

    String VideoPlayer::getVideoPath() const
    {
        return m_videoPath;
    }

    void VideoPlayer::setVideoPath( const String &videoPath )
    {
        if( m_videoPath != videoPath )
        {
            m_videoPath = videoPath;

            // Recreate video if component is loaded
            if( getLoadingState() == LoadingState::Loaded )
            {
                destroyVideo();
                createVideo();
            }
        }
    }

    SmartPtr<render::IVideoTexture> VideoPlayer::getVideoTexture() const
    {
        if( m_video )
        {
            return m_video->getVideoTexture();
        }

        return nullptr;
    }

    void VideoPlayer::setVideoTexture( SmartPtr<render::IVideoTexture> videoTexture )
    {
        if( m_video )
        {
            m_video->setVideoTexture( videoTexture );
        }
    }

    bool VideoPlayer::getAutoPlay() const
    {
        return m_autoPlay;
    }

    void VideoPlayer::setAutoPlay( bool autoPlay )
    {
        m_autoPlay = autoPlay;

        if( m_autoPlay && m_video && !m_isPlaying )
        {
            play();
        }
    }

    bool VideoPlayer::getLoop() const
    {
        return m_loop;
    }

    void VideoPlayer::setLoop( bool loop )
    {
        m_loop = loop;

        if( m_video )
        {
            m_video->setLoop( m_loop );
        }
    }

    bool VideoPlayer::getAutoUpdate() const
    {
        return m_autoUpdate;
    }

    void VideoPlayer::setAutoUpdate( bool autoUpdate )
    {
        m_autoUpdate = autoUpdate;

        if( m_video )
        {
            m_video->setAutoUpdate( m_autoUpdate );
        }
    }

    SmartPtr<Properties> VideoPlayer::getProperties() const
    {
        auto properties = Component::getProperties();
        if( properties )
        {
            properties->setProperty( videoPathStr, m_videoPath );
            properties->setProperty( autoPlayStr, m_autoPlay );
            properties->setProperty( loopStr, m_loop );
            properties->setProperty( autoUpdateStr, m_autoUpdate );

            properties->setButtonPressed( playStr );
            properties->setButtonPressed( stopStr );
            properties->setButtonPressed( pauseStr );
        }

        return properties;
    }

    void VideoPlayer::setProperties( SmartPtr<Properties> properties )
    {
        Component::setProperties( properties );

        if( properties )
        {
            String videoPath;
            if( properties->getPropertyValue( videoPathStr, videoPath ) )
            {
                setVideoPath( videoPath );
            }

            bool autoPlay = false;
            if( properties->getPropertyValue( autoPlayStr, autoPlay ) )
            {
                setAutoPlay( autoPlay );
            }

            bool loop = false;
            if( properties->getPropertyValue( loopStr, loop ) )
            {
                setLoop( loop );
            }

            bool autoUpdate = true;
            if( properties->getPropertyValue( autoUpdateStr, autoUpdate ) )
            {
                setAutoUpdate( autoUpdate );
            }

            // Handle button presses
            if( properties->isButtonPressed( playStr ) )
            {
                play();
            }

            if( properties->isButtonPressed( stopStr ) )
            {
                stop();
            }

            if( properties->isButtonPressed( pauseStr ) )
            {
                pause();
            }
        }
    }

    Array<SmartPtr<ISharedObject>> VideoPlayer::getChildObjects() const
    {
        auto objects = Component::getChildObjects();
        objects.reserve( objects.size() + 2 );

        if( m_video )
        {
            objects.emplace_back( m_video );
        }

        if( auto videoTexture = getVideoTexture() )
        {
            objects.emplace_back( videoTexture );
        }

        return objects;
    }

    void VideoPlayer::createVideo()
    {
        if( !m_videoPath.empty() )
        {
            try
            {
                auto applicationManager = core::IApplicationManager::instance();
                if( applicationManager )
                {
                    auto graphicsSystem = applicationManager->getGraphicsSystem();
                    if( graphicsSystem )
                    {
                        auto videoManager = applicationManager->getVideoManager();
                        if( videoManager )
                        {
                            m_video = videoManager->addVideo( m_videoPath );
                            if( m_video )
                            {
                                m_video->setLoop( m_loop );
                                m_video->setAutoUpdate( m_autoUpdate );

                                if( m_autoPlay )
                                {
                                    play();
                                }
                            }
                        }
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }
    }

    void VideoPlayer::destroyVideo()
    {
        if( m_video )
        {
            if( m_isPlaying )
            {
                stop();
            }

            m_video = nullptr;
        }
    }

}  // namespace workphone::scene
