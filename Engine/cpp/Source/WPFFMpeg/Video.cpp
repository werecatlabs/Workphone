#include "WPFFMpeg/Video.hpp"
#include "WPFFMpeg/Decoder.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    const String Video::IdStr = "id";
    const String Video::FileNameStr = "fileName";
    const String Video::SizeStr = "size";
    const String Video::LoopStr = "loop";
    const String Video::AutoUpdateStr = "autoUpdate";
    const String Video::LastUpdateStr = "lastUpdate";
    const String Video::NextUpdateTimeStr = "nextUpdateTime";
    const String Video::UsePowerOfTwoTextureSizeStr = "usePowerOfTwoTextureSize";
    const String Video::BytesPerPixelStr = "bytesPerPixel";

    u32 Video::m_idExt = 0;

    Video::Video() : m_id( ++m_idExt ), m_decoder( nullptr )
    {
    }

    Video::Video( u32 id ) : m_id( id ), m_decoder( nullptr )
    {
    }

    Video::~Video()
    {
        WP_SAFE_DELETE( m_decoder );
    }

    void Video::initialise( const String &fileName )
    {      
        m_fileName = fileName;
        m_decoder = new Decoder;
        m_decoder->setLoop( m_loop );
        m_decoder->setBytesPerPixel( m_bytesPerPixel );
        m_decoder->initialise( fileName );

        m_size = m_decoder->getSize();
    }

    void Video::update()
    {
        auto applicationManager = core::ApplicationManager::instance();
        auto timer = applicationManager->getTimer();

        if( !getAutoUpdate() )
        {
            return;
        }

        auto task = Thread::getCurrentTask();
        auto t = timer->getTime();
        auto dt = timer->getDeltaTime();

        switch( task )
        {
        case TaskId::Render:
        {
            if( m_decoder->isRunning() == true && m_decoder->isPlaying() == true )
            {
                if( m_videoTexture )
                {
                    while( !m_frames.empty() )
                    {
                        SmartPtr<IStateMessage> message;
                        if( m_frames.try_pop( message ) )
                        {
                            m_displayQueue.push_back( message );
                        }
                    }

                    if( !m_displayQueue.empty() )
                    {
                        auto frameData = workphone::static_pointer_cast<StateFrameData>(
                            m_displayQueue[m_displayQueue.size() - 1] );
                        auto buffer = frameData ? frameData->getVideoBuffer() : nullptr;

                        auto bufferSize = getUsePowerOfTwoTextureSize()
                                              ? Vector2I( Util::calculateNearest2Pow( m_size.X() ),
                                                          Util::calculateNearest2Pow( m_size.Y() ) )
                                              : m_size;

                        if( buffer )
                        {
                            m_videoTexture->copyData( buffer, bufferSize );
                            m_lastUpdate = m_decoder->getUpdateCount();
                        }

                        m_displayQueue.clear();
                    }
                }
                else
                {
                    while( !m_frames.empty() )
                    {
                        SmartPtr<IStateMessage> message;
                        if( m_frames.try_pop( message ) )
                        {
                        }
                    }
                }
            }
        }
        break;
        //case TaskId::Video:
        //	{
        //		if( m_decoder->isRunning() == true &&
        //			m_decoder->isPlaying() == true)
        //		{
        //			if(m_frames.empty())
        //			{
        //				RecursiveMutex::ScopedLock lock(MUTEX);
        //				auto frameData = m_decoder->getNextFrame();
        //				if(frameData)
        //				{
        //					m_frames.push(frameData);
        //				}
        //			}
        //		}
        //	}
        //	break;
        default:
        {
        }
        };
    }

    void Video::play()
    {
        if( m_decoder )
        {
            m_decoder->setPlaying( true );
        }
    }

    void Video::stop()
    {
        if( m_decoder )
        {
            m_decoder->setPlaying( false );
        }
    }

    SmartPtr<render::IVideoTexture> Video::getVideoTexture() const
    {
        return m_videoTexture;
    }

    void Video::setVideoTexture( SmartPtr<render::IVideoTexture> videoTexture )
    {
        m_videoTexture = videoTexture;
    }

    bool Video::getAutoUpdate() const
    {
        return m_autoUpdate;
    }

    void Video::setLoop( bool loop )
    {
        m_loop = loop;

        if( m_decoder )
        {
            m_decoder->setLoop( loop );
        }
    }

    u32 Video::getId() const
    {
        return m_id;
    }

    void Video::setId( u32 id )
    {
        m_id = id;
    }

    void Video::setSize( const Vector2I &size )
    {
        m_size = size;
    }

    Vector2I Video::getSize() const
    {
        return m_size;
    }

    void Video::setAutoUpdate( bool autoUpdate )
    {
        m_autoUpdate = autoUpdate;
    }

    bool Video::getLoop() const
    {
        return m_decoder ? m_decoder->getLoop() == true : m_loop;
    }

    void Video::_getObject( void **object )
    {
    }

    const workphone::SmartPtr<workphone::IStateContext> &Video::getStateContext() const
    {
        return m_stateContext;
    }

    workphone::SmartPtr<workphone::IStateContext> &Video::getStateContext()
    {
        return m_stateContext;
    }

    void *Video::getCurrentFrameBuffer() const
    {
        return nullptr;
    }

    SmartPtr<Properties> Video::getProperties() const
    {
        auto properties = ISharedObject::getProperties();
        properties->setProperty( IdStr, getId() );
        properties->setProperty( FileNameStr, getFileName() );
        properties->setProperty( SizeStr, getSize() );
        properties->setProperty( LoopStr, getLoop() );
        properties->setProperty( AutoUpdateStr, getAutoUpdate() );
        properties->setProperty( LastUpdateStr, getLastUpdate(), true );
        properties->setProperty( NextUpdateTimeStr, getNextUpdateTime() );
        properties->setProperty( UsePowerOfTwoTextureSizeStr, getUsePowerOfTwoTextureSize() );
        properties->setProperty( BytesPerPixelStr, getBytesPerPixel() );

        if( m_decoder )
        {
            auto decoderProperties = m_decoder->getProperties();
            decoderProperties->setName( "Decoder" );
            properties->addChild( decoderProperties );
        }

        return properties;
    }

    void Video::setProperties( SmartPtr<Properties> properties )
    {
        ISharedObject::setProperties( properties );

        auto id = getId();
        auto fileName = getFileName();
        auto size = getSize();
        auto loop = getLoop();
        auto autoUpdate = getAutoUpdate();
        auto nextUpdateTime = getNextUpdateTime();
        auto usePowerOfTwoTextureSize = getUsePowerOfTwoTextureSize();
        auto bytesPerPixel = getBytesPerPixel();

        properties->getPropertyValue( IdStr, id );
        properties->getPropertyValue( FileNameStr, fileName );
        properties->getPropertyValue( SizeStr, size );
        properties->getPropertyValue( LoopStr, loop );
        properties->getPropertyValue( AutoUpdateStr, autoUpdate );
        properties->getPropertyValue( NextUpdateTimeStr, nextUpdateTime );
        properties->getPropertyValue( UsePowerOfTwoTextureSizeStr, usePowerOfTwoTextureSize );
        properties->getPropertyValue( BytesPerPixelStr, bytesPerPixel );

        setId( id );
        setSize( size );
        setLoop( loop );
        setAutoUpdate( autoUpdate );
        setNextUpdateTime( nextUpdateTime );
        setUsePowerOfTwoTextureSize( usePowerOfTwoTextureSize );
        setBytesPerPixel( bytesPerPixel );

        if( fileName != getFileName() )
        {
            setFileName( fileName );
            WP_SAFE_DELETE( m_decoder );
            initialise( m_fileName );
        }
        else
        {
            setFileName( fileName );
        }

        if( m_decoder )
        {
            if( auto decoderProperties = properties->getChild( "Decoder" ) )
            {
                m_decoder->setProperties( decoderProperties );
            }
        }
    }

    String Video::getFileName() const
    {
        return m_fileName;
    }

    void Video::setFileName( const String &fileName )
    {
        m_fileName = fileName;
    }

    u32 Video::getLastUpdate() const
    {
        return m_lastUpdate;
    }

    void Video::setLastUpdate( u32 lastUpdate )
    {
        m_lastUpdate = lastUpdate;
    }

    u32 Video::getNextUpdateTime() const
    {
        return m_nextUpdateTime;
    }

    void Video::setNextUpdateTime( u32 nextUpdateTime )
    {
        m_nextUpdateTime = nextUpdateTime;
    }

    bool Video::getUsePowerOfTwoTextureSize() const
    {
        return m_usePowerOfTwoTextureSize;
    }

    void Video::setUsePowerOfTwoTextureSize( bool usePowerOfTwoTextureSize )
    {
        m_usePowerOfTwoTextureSize = usePowerOfTwoTextureSize;
    }

    s32 Video::getBytesPerPixel() const
    {
        return m_bytesPerPixel;
    }

    void Video::setBytesPerPixel( s32 bytesPerPixel )
    {
        m_bytesPerPixel = Math<s32>::max( 1, bytesPerPixel );

        if( m_decoder )
        {
            m_decoder->setBytesPerPixel( m_bytesPerPixel );
        }
    }

}  // namespace workphone
