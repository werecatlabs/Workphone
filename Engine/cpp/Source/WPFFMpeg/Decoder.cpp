#include "WPFFMpeg/Decoder.hpp"
#include <boost/thread/thread.hpp>
#include <Workphone/Workphone.hpp>

extern "C" {
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
};

namespace workphone
{
    const String Decoder::FileNameStr = "fileName";
    const String Decoder::SizeStr = "size";
    const String Decoder::PixelFormatStr = "pixelFormat";
    const String Decoder::LoopStr = "loop";
    const String Decoder::PlayingStr = "playing";
    const String Decoder::RunningStr = "running";
    const String Decoder::UpdateCountStr = "updateCount";
    const String Decoder::MaxPictureBufferSizeStr = "maxPictureBufferSize";
    const String Decoder::IoBufferSizeStr = "ioBufferSize";
    const String Decoder::InputStreamNameStr = "inputStreamName";
    const String Decoder::InputProbeSizeStr = "inputProbeSize";
    const String Decoder::InputMaxProbeSizeStr = "inputMaxProbeSize";
    const String Decoder::CodecThreadCountStr = "codecThreadCount";
    const String Decoder::ScaleFlagsStr = "scaleFlags";
    const String Decoder::BytesPerPixelStr = "bytesPerPixel";

    static int read_buffer( void *opaque, uint8_t *buf, int buf_size )
    {
        workphone::Decoder *decoder = static_cast<workphone::Decoder *>( opaque );
        SmartPtr<IStream> &stream = decoder->getStream();
        if( stream )
        {
            return stream->read( buf, buf_size );
        }
    }

    static int64_t seek( void *opaque, int64_t offset, int whence )
    {
        workphone::Decoder *decoder = static_cast<workphone::Decoder *>( opaque );
        SmartPtr<IStream> &stream = decoder->getStream();

        int force = whence & AVSEEK_FORCE;
        whence &= ~AVSEEK_FORCE;

        switch( whence )
        {
        case SEEK_SET:
            stream->seek( offset );
            break;
        case SEEK_CUR:
            stream->seek( offset );
            break;
        case SEEK_END:
            stream->seek( stream->size() - offset );
            break;
        case AVSEEK_SIZE:
            return stream->size();
            break;
        default:
        {
        }
        };

        return stream->tell();
    }

    Decoder::Decoder() :
        m_convertCtx( nullptr ),
        m_formatCtx( nullptr ),
        m_codecCtx( nullptr ),
        m_codec( nullptr ),
        m_frame( nullptr ),
        m_frameRGB( nullptr ),
        m_buffer( nullptr ),

        m_numBytes( 0 ),
        m_videoStream( 0 ),

        m_pixelFormat( 0 )
    {
        m_pixelFormat = PIX_FMT_BGRA;
        m_scaleFlags = SWS_BICUBIC;

        m_isReady = false;
        m_isRunning = true;
        m_isPlaying = false;
        m_loop = false;

        m_updateCount = 0;
    }

    Decoder::~Decoder()
    {
        m_isRunning = false;
    }

    int Decoder::initialise( const String &fileName )
    {
        /*
        m_fileName = fileName;

        auto applicationManager = core::ApplicationManager::instance();
        auto fileSystem = applicationManager->getFileSystem();
        m_stream = fileSystem->open( m_fileName );

        AVDictionary *options = nullptr;

        m_formatCtx = avformat_alloc_context();
        m_formatCtx->max_picture_buffer = getMaxPictureBufferSize();

        int bufLen = getIoBufferSize();
        u8 *pBuffer = (unsigned char *)av_malloc( bufLen );
        m_formatCtx->pb = avio_alloc_context( pBuffer, bufLen, 0, this, read_buffer, NULL, seek );
        if( !m_formatCtx->pb )
            return -1;

        AVInputFormat *fmt = NULL;
        if( av_probe_input_buffer( m_formatCtx->pb, &fmt, m_inputStreamName.c_str(), NULL,
                                   getInputProbeSize(), getInputMaxProbeSize() ) != 0 )
        {
            WP_EXCEPTION( "Error: Could not determine video format. " );
        }

        // Open video file
        if( avformat_open_input( &m_formatCtx, m_inputStreamName.c_str(), fmt, NULL ) != 0 )
        {
            WP_EXCEPTION( "Error: Couldn't open file. " );
        }

        // Retrieve stream information
        if( av_find_stream_info( m_formatCtx ) < 0 )
        {
            WP_EXCEPTION( "Error: Couldn't find stream information. " );
        }

        // Dump information about file onto standard error
        av_dump_format( m_formatCtx, 0, m_fileName.c_str(), false );

        // Find the first video stream
        m_videoStream = -1;
        for( int i = 0; i < m_formatCtx->nb_streams; ++i )
        {
            AVCodecContext *codecCtx = m_formatCtx->streams[i]->codec;
            AVMediaType mediaType = codecCtx->codec_type;
            if( mediaType == AVMEDIA_TYPE_VIDEO )
            {
                m_videoStream = i;
                break;
            }
        }

        if( m_videoStream == -1 )
        {
            WP_EXCEPTION( "Error: Didn't find a video stream. " );
        }

        // Get a pointer to the codec context for the video stream
        m_codecCtx = m_formatCtx->streams[m_videoStream]->codec;

        // Find the decoder for the video stream
        m_codec = avcodec_find_decoder( m_codecCtx->codec_id );
        if( !m_codec )
        {
            WP_EXCEPTION( "Error: Codec not found. " );
        }

        m_codecCtx->thread_count =
            getCodecThreadCount() > 0 ? getCodecThreadCount() : boost::thread::hardware_concurrency();

        // Inform the codec that we can handle truncated bitstreams -- i.e.,
        // bitstreams where frame boundaries can fall in the middle of packets
        if( m_codec->capabilities & CODEC_CAP_TRUNCATED )
            m_codecCtx->flags |= CODEC_FLAG_TRUNCATED;

        // Open codec
        if( avcodec_open2( m_codecCtx, m_codec, NULL ) < 0 )
        {
            WP_EXCEPTION( "Error: Could not open codec. " );
        }

        // Hack to correct wrong frame rates that seem to be generated by some
        // codecs
        //if(pCodecCtx->frame_rate>1000 && pCodecCtx->frame_rate_base==1)
        //	pCodecCtx->frame_rate_base=1000;

        // Allocate video frame
        m_frame = avcodec_alloc_frame();

        // Allocate an AVFrame structure
        m_frameRGB = avcodec_alloc_frame();
        if( !m_frameRGB )
        {
            WP_EXCEPTION( "Error: Could not allocate an AVFrame structure. " );
        }

        m_width = m_codecCtx->width;
        m_height = m_codecCtx->height;
        m_size = Vector2I( m_width, m_height );

        // Determine required buffer size and allocate buffer
        m_numBytes =
            avpicture_get_size( (PixelFormat)m_pixelFormat, m_codecCtx->width, m_codecCtx->height );
        m_buffer = new uint8_t[m_numBytes];

        // Assign appropriate parts of buffer to image planes in pFrameRGB
        avpicture_fill( (AVPicture *)m_frameRGB, m_buffer, (PixelFormat)m_pixelFormat, m_codecCtx->width,
                        m_codecCtx->height );

        m_convertCtx = sws_getContext( m_codecCtx->width, m_codecCtx->height, m_codecCtx->pix_fmt,
                                       m_codecCtx->width, m_codecCtx->height, (PixelFormat)m_pixelFormat,
                                       getScaleFlags(), NULL, NULL, NULL );

        m_packet = (AVPacket *)av_malloc( sizeof( AVPacket ) );
        av_init_packet( m_packet );

        m_isRunning = true;
        */

        return 0;
    }

    SmartPtr<Properties> Decoder::getProperties() const
    {
        auto properties = ISharedObject::getProperties();
        properties->setProperty( FileNameStr, getFileName() );
        properties->setProperty( SizeStr, getSize() );
        properties->setProperty( PixelFormatStr, getPixelFormat() );
        properties->setProperty( LoopStr, getLoop() );
        properties->setProperty( PlayingStr, isPlaying() );
        properties->setProperty( RunningStr, isRunning() );
        properties->setProperty( UpdateCountStr, getUpdateCount(), true );
        properties->setProperty( MaxPictureBufferSizeStr, getMaxPictureBufferSize() );
        properties->setProperty( IoBufferSizeStr, getIoBufferSize() );
        properties->setProperty( InputStreamNameStr, getInputStreamName() );
        properties->setProperty( InputProbeSizeStr, getInputProbeSize() );
        properties->setProperty( InputMaxProbeSizeStr, getInputMaxProbeSize() );
        properties->setProperty( CodecThreadCountStr, getCodecThreadCount() );
        properties->setProperty( ScaleFlagsStr, getScaleFlags() );
        properties->setProperty( BytesPerPixelStr, getBytesPerPixel() );

        return properties;
    }

    void Decoder::setProperties( SmartPtr<Properties> properties )
    {
        ISharedObject::setProperties( properties );

        auto fileName = getFileName();
        auto size = getSize();
        auto pixelFormat = getPixelFormat();
        auto loop = getLoop();
        auto playing = isPlaying();
        auto running = isRunning();
        auto maxPictureBufferSize = getMaxPictureBufferSize();
        auto ioBufferSize = getIoBufferSize();
        auto inputStreamName = getInputStreamName();
        auto inputProbeSize = getInputProbeSize();
        auto inputMaxProbeSize = getInputMaxProbeSize();
        auto codecThreadCount = getCodecThreadCount();
        auto scaleFlags = getScaleFlags();
        auto bytesPerPixel = getBytesPerPixel();

        properties->getPropertyValue( FileNameStr, fileName );
        properties->getPropertyValue( SizeStr, size );
        properties->getPropertyValue( PixelFormatStr, pixelFormat );
        properties->getPropertyValue( LoopStr, loop );
        properties->getPropertyValue( PlayingStr, playing );
        properties->getPropertyValue( RunningStr, running );
        properties->getPropertyValue( MaxPictureBufferSizeStr, maxPictureBufferSize );
        properties->getPropertyValue( IoBufferSizeStr, ioBufferSize );
        properties->getPropertyValue( InputStreamNameStr, inputStreamName );
        properties->getPropertyValue( InputProbeSizeStr, inputProbeSize );
        properties->getPropertyValue( InputMaxProbeSizeStr, inputMaxProbeSize );
        properties->getPropertyValue( CodecThreadCountStr, codecThreadCount );
        properties->getPropertyValue( ScaleFlagsStr, scaleFlags );
        properties->getPropertyValue( BytesPerPixelStr, bytesPerPixel );

        setFileName( fileName );
        setSize( size );
        setPixelFormat( pixelFormat );
        setLoop( loop );
        setPlaying( playing );
        setRunning( running );
        setMaxPictureBufferSize( maxPictureBufferSize );
        setIoBufferSize( ioBufferSize );
        setInputStreamName( inputStreamName );
        setInputProbeSize( inputProbeSize );
        setInputMaxProbeSize( inputMaxProbeSize );
        setCodecThreadCount( codecThreadCount );
        setScaleFlags( scaleFlags );
        setBytesPerPixel( bytesPerPixel );
    }

    bool Decoder::decodeFrame()
    {
        bool retValue = false;

        int len;
        int got_picture = 0;

        if( av_read_frame( m_formatCtx, m_packet ) >= 0 )
        {
            if( m_codecCtx->codec_id == CODEC_ID_RAWVIDEO )
            {
                retValue = true;
            }
            else
            {
                u32 size = m_packet->size;
                u8 *data_ptr = m_packet->data;

                while( m_packet->size > 0 )
                {
                    len = avcodec_decode_video2( m_codecCtx, m_frame, &got_picture, m_packet );
                    if( len < 0 )
                    {
                        retValue = false;
                        break;
                    }

                    if( got_picture )
                    {
                        retValue = true;
                    }

                    m_packet->size -= len;
                    m_packet->data += len;
                }

                m_packet->size = size;
                m_packet->data = data_ptr;
            }
        }
        else if( getLoop() == true )
        {
            int retVal = av_seek_frame( m_formatCtx, 0, 0, SEEK_SET );
            if( retVal < 0 )
            {
                WP_LOG_INFO( "Seeking error." );
            }
        }

        return retValue;
    }

    void Decoder::destroy()
    {
        // Free the RGB image
        delete[] m_buffer;
        av_free( m_frameRGB );

        // Free the YUV frame
        av_free( m_frame );

        // Close the codec
        avcodec_close( m_codecCtx );

        // Close the video file
        av_close_input_file( m_formatCtx );
    }

    SmartPtr<IStateMessage> Decoder::getNextFrame()
    {
        try
        {
            if( isRunning() )
            {
                if( isPlaying() == true )
                {
                    if( decodeFrame() )
                    {
                        auto frameData = workphone::make_ptr<StateFrameData>();
                        frameData->setVideoBufferSize( Util::calculateNearest2Pow( m_size.X() ) *
                                                       Util::calculateNearest2Pow( m_size.Y() ) *
                                                       getBytesPerPixel() );

                        if( m_codecCtx->pix_fmt != m_pixelFormat )
                        {
                            sws_scale( m_convertCtx, m_frame->data, m_frame->linesize, 0,
                                       m_codecCtx->height, m_frameRGB->data, m_frameRGB->linesize );
                            memcpy( frameData->getVideoBuffer(), m_frameRGB->data[0], m_numBytes );
                        }
                        else
                        {
                            if( m_frame->linesize[0] > 0 && m_frame->linesize[0] < m_numBytes )
                            {
                                memcpy( frameData->getVideoBuffer(), m_frame->data[0], m_numBytes );
                            }
                            else
                            {
                                if( m_packet->size >= m_numBytes )
                                {
                                    s32 biSourcePitch = m_size.X() * getBytesPerPixel();
                                    s32 biTargetWidth = Util::calculateNearest2Pow( m_size.X() );
                                    s32 biTargetHeight = Util::calculateNearest2Pow( m_size.Y() );
                                    s32 biTargetPitch = biTargetWidth * getBytesPerPixel();
                                    double vScale = m_size.X() * 1.0 / biTargetHeight;
                                    double hScale = m_size.Y() * 1.0 / biTargetWidth;

                                    u8 *pTargetLine = frameData->getVideoBuffer();

                                    for( int i = m_size.Y() - 1; i > 0; --i )
                                    {
                                        pTargetLine = pTargetLine + biTargetPitch;
                                        u8 *pSourceLine = &m_packet->data[i * biSourcePitch];
                                        Memory::Memcpy( pTargetLine, pSourceLine, biSourcePitch );
                                    }
                                }
                            }
                        }

                        av_free_packet( m_packet );
                        ++m_updateCount;
                        return frameData;
                    }
                }
            }
        }
        catch( ... )
        {
        }

        return nullptr;
    }

    String Decoder::getFileName() const
    {
        return m_fileName;
    }

    void Decoder::setFileName( const String &fileName )
    {
        m_fileName = fileName;
    }

    s32 Decoder::getMaxPictureBufferSize() const
    {
        return m_maxPictureBufferSize;
    }

    void Decoder::setMaxPictureBufferSize( s32 maxPictureBufferSize )
    {
        m_maxPictureBufferSize = Math<s32>::max( 1, maxPictureBufferSize );
    }

    s32 Decoder::getIoBufferSize() const
    {
        return m_ioBufferSize;
    }

    void Decoder::setIoBufferSize( s32 ioBufferSize )
    {
        m_ioBufferSize = Math<s32>::max( 1, ioBufferSize );
    }

    String Decoder::getInputStreamName() const
    {
        return m_inputStreamName;
    }

    void Decoder::setInputStreamName( const String &inputStreamName )
    {
        m_inputStreamName = inputStreamName.empty() ? String( "stream" ) : inputStreamName;
    }

    s32 Decoder::getInputProbeSize() const
    {
        return m_inputProbeSize;
    }

    void Decoder::setInputProbeSize( s32 inputProbeSize )
    {
        m_inputProbeSize = Math<s32>::max( 0, inputProbeSize );
    }

    s32 Decoder::getInputMaxProbeSize() const
    {
        return m_inputMaxProbeSize;
    }

    void Decoder::setInputMaxProbeSize( s32 inputMaxProbeSize )
    {
        m_inputMaxProbeSize = Math<s32>::max( 0, inputMaxProbeSize );
    }

    s32 Decoder::getCodecThreadCount() const
    {
        return m_codecThreadCount;
    }

    void Decoder::setCodecThreadCount( s32 codecThreadCount )
    {
        m_codecThreadCount = Math<s32>::max( 0, codecThreadCount );
    }

    s32 Decoder::getScaleFlags() const
    {
        return m_scaleFlags;
    }

    void Decoder::setScaleFlags( s32 scaleFlags )
    {
        m_scaleFlags = scaleFlags;
    }

    s32 Decoder::getBytesPerPixel() const
    {
        return m_bytesPerPixel;
    }

    void Decoder::setBytesPerPixel( s32 bytesPerPixel )
    {
        m_bytesPerPixel = Math<s32>::max( 1, bytesPerPixel );
    }

}  // namespace workphone
