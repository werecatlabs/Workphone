#include <WPTheoraVideo/WPTheoraVideoManager.hpp>
#include <WPTheoraVideo/WPTheoraVideo.hpp>
#include <WPTheoraVideo/WPTheoraDataSource.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/ITextureManager.hpp>
#include <Workphone/Math/Math.hpp>
#include <Workphone/System/ApplicationManager.hpp>

#if __has_include( "TheoraVideoManager.hpp" )
#    include "TheoraVideoManager.hpp"
#    define WP_THEORA_HAS_MANAGER 1
#else
#    define WP_THEORA_HAS_MANAGER 0
#endif

#ifndef TH_BGRA
#    define TH_BGRA 0
#endif

namespace workphone
{
    const String TheoraVideoMgr::IsCapturingStr = "isCapturing";
    const String TheoraVideoMgr::OutputFilePathStr = "outputFilePath";
    const String TheoraVideoMgr::WorkerThreadsStr = "workerThreads";
    const String TheoraVideoMgr::OutputPixelFormatStr = "outputPixelFormat";
    const String TheoraVideoMgr::PrecacheStrategyStr = "precacheStrategy";
    const String TheoraVideoMgr::MaxPrecachedFramesStr = "maxPrecachedFrames";
    const String TheoraVideoMgr::AutoRestartStr = "autoRestart";
    const String TheoraVideoMgr::PrecacheAllFramesStr = "precacheAllFrames";
    const String TheoraVideoMgr::UpdateVideosStr = "updateVideos";
    const String TheoraVideoMgr::TheoraAvailableStr = "theoraAvailable";

    TheoraVideoMgr::TheoraVideoMgr()
    {
        m_outputPixelFormat = TH_BGRA;

#if WP_THEORA_HAS_MANAGER
        m_mgr = new TheoraVideoManager( getWorkerThreads() );
#endif
    }

    TheoraVideoMgr::~TheoraVideoMgr()
    {
#if WP_THEORA_HAS_MANAGER
        delete m_mgr;
#endif
        m_mgr = nullptr;
    }

    void TheoraVideoMgr::update()
    {
#if WP_THEORA_HAS_MANAGER
        if( m_mgr )
        {
            m_mgr->update( 0.0f );
        }
#endif

        if( getUpdateVideos() )
        {
            for( auto &entry : m_videos )
            {
                auto video = workphone::static_pointer_cast<TheoraVideo>( entry.second );
                if( video )
                {
                    video->update( 0, 0.0, 0.0 );
                }
            }
        }
    }

    SmartPtr<Properties> TheoraVideoMgr::getProperties() const
    {
        auto properties = ISharedObject::getProperties();
        properties->setProperty( IsCapturingStr, isCapturing() );
        properties->setProperty( OutputFilePathStr, getOutputFilePath() );
        properties->setProperty( WorkerThreadsStr, getWorkerThreads() );
        properties->setProperty( OutputPixelFormatStr, getOutputPixelFormat() );
        properties->setProperty( PrecacheStrategyStr, getPrecacheStrategy() );
        properties->setProperty( MaxPrecachedFramesStr, getMaxPrecachedFrames() );
        properties->setProperty( AutoRestartStr, getAutoRestart() );
        properties->setProperty( PrecacheAllFramesStr, getPrecacheAllFrames() );
        properties->setProperty( UpdateVideosStr, getUpdateVideos() );
        properties->setProperty( TheoraAvailableStr, getTheoraAvailable(), true );

        return properties;
    }

    void TheoraVideoMgr::setProperties( SmartPtr<Properties> properties )
    {
        ISharedObject::setProperties( properties );

        auto capturing = isCapturing();
        auto outputFilePath = getOutputFilePath();
        auto workerThreads = getWorkerThreads();
        auto outputPixelFormat = getOutputPixelFormat();
        auto precacheStrategy = getPrecacheStrategy();
        auto maxPrecachedFrames = getMaxPrecachedFrames();
        auto autoRestart = getAutoRestart();
        auto precacheAllFrames = getPrecacheAllFrames();
        auto updateVideos = getUpdateVideos();

        properties->getPropertyValue( IsCapturingStr, capturing );
        properties->getPropertyValue( OutputFilePathStr, outputFilePath );
        properties->getPropertyValue( WorkerThreadsStr, workerThreads );
        properties->getPropertyValue( OutputPixelFormatStr, outputPixelFormat );
        properties->getPropertyValue( PrecacheStrategyStr, precacheStrategy );
        properties->getPropertyValue( MaxPrecachedFramesStr, maxPrecachedFrames );
        properties->getPropertyValue( AutoRestartStr, autoRestart );
        properties->getPropertyValue( PrecacheAllFramesStr, precacheAllFrames );
        properties->getPropertyValue( UpdateVideosStr, updateVideos );

        capturing ? startCapture() : stopCapture();
        setOutputFilePath( outputFilePath );
        setWorkerThreads( workerThreads );
        setOutputPixelFormat( outputPixelFormat );
        setPrecacheStrategy( precacheStrategy );
        setMaxPrecachedFrames( maxPrecachedFrames );
        setAutoRestart( autoRestart );
        setPrecacheAllFrames( precacheAllFrames );
        setUpdateVideos( updateVideos );
    }

    SmartPtr<render::IVideo> TheoraVideoMgr::addVideo( const String &fileName )
    {
        return createVideo( 0, fileName );
    }

    SmartPtr<render::IVideo> TheoraVideoMgr::addVideo( hash32 id, const String &fileName )
    {
        return createVideo( id, fileName );
    }

    SmartPtr<render::IVideo> TheoraVideoMgr::createVideo( hash32 id, const String &fileName )
    {
        auto video = id == 0 ? workphone::make_ptr<TheoraVideo>() : workphone::make_ptr<TheoraVideo>( id );
        video->setFileName( fileName );
        video->setLoop( getAutoRestart() );

#if WP_THEORA_HAS_MANAGER
        if( m_mgr )
        {
            auto clip = m_mgr->createVideoClip( new WPTheoraDataSource( fileName ), getOutputPixelFormat(),
                                                getPrecacheStrategy(), getWorkerThreads() );
            video->setClip( clip );
            applyClipSettings( video );
        }
#else
        WP_LOG_WARNING( "Theora video backend is not available; created metadata-only video." );
#endif

        m_videos[video->getId()] = video;
        return video;
    }

    void TheoraVideoMgr::applyClipSettings( SmartPtr<render::IVideo> video )
    {
#if WP_THEORA_HAS_MANAGER
        auto theoraVideo = workphone::static_pointer_cast<TheoraVideo>( video );
        if( theoraVideo )
        {
            if( auto clip = theoraVideo->getClip() )
            {
                auto frames = getPrecacheAllFrames() ? clip->getNumFrames() :
                              Math<u32>::min( static_cast<u32>( clip->getNumFrames() ),
                                               getMaxPrecachedFrames() );
                clip->setNumPrecachedFrames( frames );
                clip->setAutoRestart( getAutoRestart() );
            }
        }
#endif
    }

    SmartPtr<render::IVideo> TheoraVideoMgr::getVideoById( hash32 id ) const
    {
        auto it = m_videos.find( id );
        return it != m_videos.end() ? it->second : nullptr;
    }

    SmartPtr<render::IVideoTexture> TheoraVideoMgr::createVideoTexture( const String &textureName )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( auto graphicsSystem = applicationManager->getGraphicsSystemPtr() )
        {
            if( auto textureManager = graphicsSystem->getTextureManager() )
            {
                return textureManager->createVideoTexture( textureName );
            }
        }

        return nullptr;
    }

    bool TheoraVideoMgr::removeVideoTexture( SmartPtr<render::IVideoTexture> videoTexture )
    {
        return videoTexture != nullptr;
    }

    bool TheoraVideoMgr::removeVideoTexture( const String &textureName )
    {
        return !textureName.empty();
    }

    SmartPtr<render::IVideoStream> TheoraVideoMgr::createVideoStream() const
    {
        return nullptr;
    }

    void TheoraVideoMgr::startCapture()
    {
        m_isCapturing = true;
    }

    void TheoraVideoMgr::stopCapture()
    {
        m_isCapturing = false;
    }

    bool TheoraVideoMgr::isCapturing() const
    {
        return m_isCapturing;
    }

    void TheoraVideoMgr::setOutputFilePath( const String &filePath )
    {
        m_outputFilePath = filePath;
    }

    String TheoraVideoMgr::getOutputFilePath() const
    {
        return m_outputFilePath;
    }

    u32 TheoraVideoMgr::getWorkerThreads() const
    {
        return m_workerThreads;
    }

    void TheoraVideoMgr::setWorkerThreads( u32 workerThreads )
    {
        m_workerThreads = Math<u32>::max( 1u, workerThreads );
    }

    s32 TheoraVideoMgr::getOutputPixelFormat() const
    {
        return m_outputPixelFormat;
    }

    void TheoraVideoMgr::setOutputPixelFormat( s32 outputPixelFormat )
    {
        m_outputPixelFormat = outputPixelFormat;
    }

    s32 TheoraVideoMgr::getPrecacheStrategy() const
    {
        return m_precacheStrategy;
    }

    void TheoraVideoMgr::setPrecacheStrategy( s32 precacheStrategy )
    {
        m_precacheStrategy = Math<s32>::max( 0, precacheStrategy );
    }

    u32 TheoraVideoMgr::getMaxPrecachedFrames() const
    {
        return m_maxPrecachedFrames;
    }

    void TheoraVideoMgr::setMaxPrecachedFrames( u32 maxPrecachedFrames )
    {
        m_maxPrecachedFrames = maxPrecachedFrames;
    }

    bool TheoraVideoMgr::getAutoRestart() const
    {
        return m_autoRestart;
    }

    void TheoraVideoMgr::setAutoRestart( bool autoRestart )
    {
        m_autoRestart = autoRestart;
    }

    bool TheoraVideoMgr::getPrecacheAllFrames() const
    {
        return m_precacheAllFrames;
    }

    void TheoraVideoMgr::setPrecacheAllFrames( bool precacheAllFrames )
    {
        m_precacheAllFrames = precacheAllFrames;
    }

    bool TheoraVideoMgr::getUpdateVideos() const
    {
        return m_updateVideos;
    }

    void TheoraVideoMgr::setUpdateVideos( bool updateVideos )
    {
        m_updateVideos = updateVideos;
    }

    bool TheoraVideoMgr::getTheoraAvailable() const
    {
        return WP_THEORA_HAS_MANAGER != 0;
    }
}  // namespace workphone
