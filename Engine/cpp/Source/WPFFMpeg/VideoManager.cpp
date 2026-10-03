#include "WPFFMpeg/VideoManager.hpp"
#include "WPFFMpeg/Video.hpp"
#include <Workphone/WorkphoneHeaders.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
};

namespace workphone
{
    const String VideoManager::IsCapturingStr = "isCapturing";
    const String VideoManager::OutputFilePathStr = "outputFilePath";
    const String VideoManager::UpdateVideosStr = "updateVideos";
    const String VideoManager::AutoRegisterFormatsStr = "autoRegisterFormats";

    VideoManager::VideoManager() : m_isCapturing( false ), m_videoStream( nullptr )
    {
        // Register all formats and codecs
        if( getAutoRegisterFormats() )
        {
            av_register_all();
        }
    }

    VideoManager::~VideoManager()
    {
    }

    void VideoManager::update()
    {
        if( getUpdateVideos() )
        {
            for( auto &video : m_videos )
            {
                auto ffmpegVideo = workphone::static_pointer_cast<Video>( video.second );
                if( ffmpegVideo )
                {
                    ffmpegVideo->update();
                }
            }
        }
    }

    SmartPtr<render::IVideo> VideoManager::getVideoById( hash32 id ) const
    {
        Videos::const_iterator it = m_videos.find( id );
        if( it != m_videos.end() )
        {
            return it->second;
        }

        return nullptr;
    }

    SmartPtr<render::IVideo> VideoManager::addVideo( hash32 id, const String &fileName )
    {
        try
        {
            FFMpegVideoPtr video( new Video( id ) );
            video->initialise( fileName );
            m_videos[video->getId()] = video;

            return video;
        }
        catch( Exception &e )
        {
            WP_LOG_INFO( e.what() );
        }
        catch( std::exception &e )
        {
            WP_LOG_INFO( e.what() );
        }
        catch( ... )
        {
            WP_LOG_INFO( "Unknown error." );
        }

        return nullptr;
    }

    SmartPtr<render::IVideo> VideoManager::addVideo( const String &fileName )
    {
        try
        {
            FFMpegVideoPtr video( new Video );
            video->initialise( fileName );
            m_videos[video->getId()] = video;

            return video;
        }
        catch( Exception &e )
        {
            WP_LOG_INFO( e.what() );
        }
        catch( std::exception &e )
        {
            WP_LOG_INFO( e.what() );
        }
        catch( ... )
        {
            WP_LOG_INFO( "Unknown error." );
        }

        return nullptr;
    }

    SmartPtr<render::IVideoTexture> VideoManager::createVideoTexture( const String &textureName )
    {
        auto applicationManager = core::ApplicationManager::instance();
        auto graphicsSystem = applicationManager->getGraphicsSystem();
        auto textureManager = graphicsSystem->getTextureManager();

        auto videoTexture = textureManager->createVideoTexture( textureName );
        return videoTexture;
    }

    SmartPtr<render::IVideoStream> VideoManager::createVideoStream() const
    {
        return nullptr;
    }

    void VideoManager::startCapture()
    {
        m_isCapturing = true;
    }

    void VideoManager::stopCapture()
    {
        m_isCapturing = false;
    }

    bool VideoManager::isCapturing() const
    {
        return m_isCapturing;
    }

    String VideoManager::getOutputFilePath() const
    {
        return m_outputFilePath;
    }

    void VideoManager::setOutputFilePath( const String &filePath )
    {
        m_outputFilePath = filePath;
    }

    bool VideoManager::getUpdateVideos() const
    {
        return m_updateVideos;
    }

    void VideoManager::setUpdateVideos( bool updateVideos )
    {
        m_updateVideos = updateVideos;
    }

    bool VideoManager::getAutoRegisterFormats() const
    {
        return m_autoRegisterFormats;
    }

    void VideoManager::setAutoRegisterFormats( bool autoRegisterFormats )
    {
        m_autoRegisterFormats = autoRegisterFormats;
    }

    SmartPtr<Properties> VideoManager::getProperties() const
    {
        auto properties = ISharedObject::getProperties();
        properties->setProperty( IsCapturingStr, isCapturing() );
        properties->setProperty( OutputFilePathStr, getOutputFilePath() );
        properties->setProperty( UpdateVideosStr, getUpdateVideos() );
        properties->setProperty( AutoRegisterFormatsStr, getAutoRegisterFormats() );

        return properties;
    }

    void VideoManager::setProperties( SmartPtr<Properties> properties )
    {
        ISharedObject::setProperties( properties );

        auto capturing = isCapturing();
        auto outputFilePath = getOutputFilePath();
        auto updateVideos = getUpdateVideos();
        auto autoRegisterFormats = getAutoRegisterFormats();

        properties->getPropertyValue( IsCapturingStr, capturing );
        properties->getPropertyValue( OutputFilePathStr, outputFilePath );
        properties->getPropertyValue( UpdateVideosStr, updateVideos );
        properties->getPropertyValue( AutoRegisterFormatsStr, autoRegisterFormats );

        if( capturing )
        {
            startCapture();
        }
        else
        {
            stopCapture();
        }

        setOutputFilePath( outputFilePath );
        setUpdateVideos( updateVideos );
        setAutoRegisterFormats( autoRegisterFormats );
    }

    bool VideoManager::removeVideoTexture( SmartPtr<render::IVideoTexture> videoTexture )
    {
        return false;
    }

    bool VideoManager::removeVideoTexture( const String &textureName )
    {
        return false;
    }

}  // namespace workphone
