#include <WPTheoraVideo/WPTheoraVideo.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Math/Math.hpp>

#if __has_include( "TheoraVideoClip.hpp" ) && __has_include( "TheoraVideoFrame.hpp" )
#    include "TheoraVideoClip.hpp"
#    include "TheoraVideoFrame.hpp"
#    define WP_THEORA_HAS_VIDEO_CLIP 1
#elif __has_include( "TheoraVideoManager.hpp" ) && __has_include( "TheoraVideoFrame.hpp" )
#    include "TheoraVideoManager.hpp"
#    include "TheoraVideoFrame.hpp"
#    define WP_THEORA_HAS_VIDEO_CLIP 1
#else
#    define WP_THEORA_HAS_VIDEO_CLIP 0
#endif

namespace workphone
{
    const String TheoraVideo::IdStr = "id";
    const String TheoraVideo::FileNameStr = "fileName";
    const String TheoraVideo::SizeStr = "size";
    const String TheoraVideo::AutoUpdateStr = "autoUpdate";
    const String TheoraVideo::LoopStr = "loop";
    const String TheoraVideo::RestartOnPlayStr = "restartOnPlay";
    const String TheoraVideo::PopFramesAfterUploadStr = "popFramesAfterUpload";
    const String TheoraVideo::UseFrameStrideStr = "useFrameStride";
    const String TheoraVideo::BytesPerPixelStr = "bytesPerPixel";
    const String TheoraVideo::LastTaskIdStr = "lastTaskId";
    const String TheoraVideo::LastUpdateTimeStr = "lastUpdateTime";
    const String TheoraVideo::LastDeltaTimeStr = "lastDeltaTime";
    const String TheoraVideo::FrameUploadCountStr = "frameUploadCount";

    u32 TheoraVideo::m_idExt = 0;

    TheoraVideo::TheoraVideo() : m_id( ++m_idExt )
    {
    }

    TheoraVideo::TheoraVideo( hash32 id ) : m_id( id )
    {
    }

    TheoraVideo::~TheoraVideo() = default;

    void TheoraVideo::update( u32 taskId, f64 time, f64 deltaTime )
    {
        m_lastTaskId = taskId;
        m_lastUpdateTime = time;
        m_lastDeltaTime = deltaTime;

        if( !getAutoUpdate() || !m_videoTexture || !m_clip )
        {
            return;
        }

#if WP_THEORA_HAS_VIDEO_CLIP
        if( auto frame = m_clip->getNextFrame() )
        {
            auto width = getUseFrameStride() ? frame->getStride() : frame->getWidth();
            auto height = frame->getHeight();
            auto videoData = frame->getBuffer();

            m_size = Vector2I( width, height );
            m_videoTexture->copyData( videoData, m_size );
            ++m_frameUploadCount;

            if( getPopFramesAfterUpload() )
            {
                m_clip->popFrame();
            }
        }
#endif
    }

    void TheoraVideo::play()
    {
#if WP_THEORA_HAS_VIDEO_CLIP
        if( m_clip )
        {
            if( getRestartOnPlay() )
            {
                m_clip->restart();
            }

            m_clip->play();
        }
#endif
    }

    void TheoraVideo::stop()
    {
#if WP_THEORA_HAS_VIDEO_CLIP
        if( m_clip )
        {
            m_clip->pause();
        }
#endif
    }

    Vector2I TheoraVideo::getSize() const
    {
#if WP_THEORA_HAS_VIDEO_CLIP
        if( m_clip )
        {
            return Vector2I( m_clip->getStride(), m_clip->getHeight() );
        }
#endif

        return m_size;
    }

    void TheoraVideo::setSize( const Vector2I &size )
    {
        m_size.x = Math<s32>::max( 0, size.x );
        m_size.y = Math<s32>::max( 0, size.y );
    }

    void *TheoraVideo::getCurrentFrameBuffer() const
    {
#if WP_THEORA_HAS_VIDEO_CLIP
        if( m_clip )
        {
            if( auto frame = m_clip->getNextFrame() )
            {
                return frame->getBuffer();
            }
        }
#endif

        return nullptr;
    }

    void TheoraVideo::setLoop( bool loop )
    {
        m_loop = loop;

#if WP_THEORA_HAS_VIDEO_CLIP
        if( m_clip )
        {
            m_clip->setAutoRestart( loop );
        }
#endif
    }

    bool TheoraVideo::getLoop() const
    {
        return m_loop;
    }

    void TheoraVideo::setAutoUpdate( bool autoUpdate )
    {
        m_autoUpdate = autoUpdate;
    }

    bool TheoraVideo::getAutoUpdate() const
    {
        return m_autoUpdate;
    }

    SmartPtr<render::IVideoTexture> TheoraVideo::getVideoTexture() const
    {
        return m_videoTexture;
    }

    void TheoraVideo::setVideoTexture( SmartPtr<render::IVideoTexture> videoTexture )
    {
        m_videoTexture = videoTexture;
    }

    void TheoraVideo::_getObject( void **object )
    {
        *object = m_clip;
    }

    SmartPtr<Properties> TheoraVideo::getProperties() const
    {
        auto properties = ISharedObject::getProperties();
        properties->setProperty( IdStr, getId() );
        properties->setProperty( FileNameStr, getFileName() );
        properties->setProperty( SizeStr, getSize() );
        properties->setProperty( AutoUpdateStr, getAutoUpdate() );
        properties->setProperty( LoopStr, getLoop() );
        properties->setProperty( RestartOnPlayStr, getRestartOnPlay() );
        properties->setProperty( PopFramesAfterUploadStr, getPopFramesAfterUpload() );
        properties->setProperty( UseFrameStrideStr, getUseFrameStride() );
        properties->setProperty( BytesPerPixelStr, getBytesPerPixel() );
        properties->setProperty( LastTaskIdStr, getLastTaskId(), true );
        properties->setProperty( LastUpdateTimeStr, getLastUpdateTime(), true );
        properties->setProperty( LastDeltaTimeStr, getLastDeltaTime(), true );
        properties->setProperty( FrameUploadCountStr, getFrameUploadCount(), true );

        return properties;
    }

    void TheoraVideo::setProperties( SmartPtr<Properties> properties )
    {
        ISharedObject::setProperties( properties );

        auto id = getId();
        auto fileName = getFileName();
        auto size = getSize();
        auto autoUpdate = getAutoUpdate();
        auto loop = getLoop();
        auto restartOnPlay = getRestartOnPlay();
        auto popFramesAfterUpload = getPopFramesAfterUpload();
        auto useFrameStride = getUseFrameStride();
        auto bytesPerPixel = getBytesPerPixel();

        properties->getPropertyValue( IdStr, id );
        properties->getPropertyValue( FileNameStr, fileName );
        properties->getPropertyValue( SizeStr, size );
        properties->getPropertyValue( AutoUpdateStr, autoUpdate );
        properties->getPropertyValue( LoopStr, loop );
        properties->getPropertyValue( RestartOnPlayStr, restartOnPlay );
        properties->getPropertyValue( PopFramesAfterUploadStr, popFramesAfterUpload );
        properties->getPropertyValue( UseFrameStrideStr, useFrameStride );
        properties->getPropertyValue( BytesPerPixelStr, bytesPerPixel );

        setId( id );
        setFileName( fileName );
        setSize( size );
        setAutoUpdate( autoUpdate );
        setLoop( loop );
        setRestartOnPlay( restartOnPlay );
        setPopFramesAfterUpload( popFramesAfterUpload );
        setUseFrameStride( useFrameStride );
        setBytesPerPixel( bytesPerPixel );
    }

    TheoraVideoClip *TheoraVideo::getClip() const
    {
        return m_clip;
    }

    void TheoraVideo::setClip( TheoraVideoClip *clip )
    {
        m_clip = clip;

#if WP_THEORA_HAS_VIDEO_CLIP
        if( m_clip )
        {
            m_size = Vector2I( m_clip->getStride(), m_clip->getHeight() );
            m_clip->setAutoRestart( getLoop() );
        }
#endif
    }

    hash32 TheoraVideo::getId() const
    {
        return m_id;
    }

    void TheoraVideo::setId( hash32 id )
    {
        m_id = id;
    }

    String TheoraVideo::getFileName() const
    {
        return m_fileName;
    }

    void TheoraVideo::setFileName( const String &fileName )
    {
        m_fileName = fileName;
    }

    bool TheoraVideo::getRestartOnPlay() const
    {
        return m_restartOnPlay;
    }

    void TheoraVideo::setRestartOnPlay( bool restartOnPlay )
    {
        m_restartOnPlay = restartOnPlay;
    }

    bool TheoraVideo::getPopFramesAfterUpload() const
    {
        return m_popFramesAfterUpload;
    }

    void TheoraVideo::setPopFramesAfterUpload( bool popFramesAfterUpload )
    {
        m_popFramesAfterUpload = popFramesAfterUpload;
    }

    bool TheoraVideo::getUseFrameStride() const
    {
        return m_useFrameStride;
    }

    void TheoraVideo::setUseFrameStride( bool useFrameStride )
    {
        m_useFrameStride = useFrameStride;
    }

    s32 TheoraVideo::getBytesPerPixel() const
    {
        return m_bytesPerPixel;
    }

    void TheoraVideo::setBytesPerPixel( s32 bytesPerPixel )
    {
        m_bytesPerPixel = Math<s32>::max( 1, bytesPerPixel );
    }

    u32 TheoraVideo::getLastTaskId() const
    {
        return m_lastTaskId;
    }

    f64 TheoraVideo::getLastUpdateTime() const
    {
        return m_lastUpdateTime;
    }

    f64 TheoraVideo::getLastDeltaTime() const
    {
        return m_lastDeltaTime;
    }

    u32 TheoraVideo::getFrameUploadCount() const
    {
        return m_frameUploadCount;
    }
}  // namespace workphone
