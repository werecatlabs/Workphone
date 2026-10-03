#include "WPFFMpeg/Encoder.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/thread/thread.hpp>
#include <math.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include "libavutil/avutil.h"
#include <libswscale/swscale.h>
};

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

namespace workphone
{
    const String FFMpegEncoder::WidthStr = "width";
    const String FFMpegEncoder::HeightStr = "height";
    const String FFMpegEncoder::FpsStr = "fps";
    const String FFMpegEncoder::FileNameStr = "fileName";
    const String FFMpegEncoder::RunningStr = "running";
    const String FFMpegEncoder::AudioBitRateStr = "audioBitRate";
    const String FFMpegEncoder::AudioSampleRateStr = "audioSampleRate";
    const String FFMpegEncoder::AudioChannelsStr = "audioChannels";
    const String FFMpegEncoder::AudioSampleFormatStr = "audioSampleFormat";
    const String FFMpegEncoder::AudioOutputBufferSizeStr = "audioOutputBufferSize";
    const String FFMpegEncoder::VideoBitRateStr = "videoBitRate";
    const String FFMpegEncoder::StreamPixelFormatStr = "streamPixelFormat";
    const String FFMpegEncoder::InputPixelFormatStr = "inputPixelFormat";
    const String FFMpegEncoder::ScaleFlagsStr = "scaleFlags";
    const String FFMpegEncoder::VideoOutputBufferSizeStr = "videoOutputBufferSize";
    const String FFMpegEncoder::GopSizeStr = "gopSize";
    const String FFMpegEncoder::KeyintMinStr = "keyintMin";
    const String FFMpegEncoder::MaxBFramesStr = "maxBFrames";
    const String FFMpegEncoder::Mpeg2MaxBFramesStr = "mpeg2MaxBFrames";
    const String FFMpegEncoder::Mpeg1MbDecisionStr = "mpeg1MbDecision";
    const String FFMpegEncoder::OutputThreadSleepSecondsStr = "outputThreadSleepSeconds";

    FFMpegEncoder::FFMpegEncoder( int width, int height, int fps, const String &fileName,
                                  int codec_id ) :
        m_fileName( fileName ),
        fps( fps )
    {
        m_audioSampleFormat = AV_SAMPLE_FMT_S16;
        m_streamPixelFormat = PIX_FMT_YUV420P;
        m_inputPixelFormat = PIX_FMT_BGR24;
        m_scaleFlags = SWS_BICUBIC;

        m_isReady = false;

        video_frame_number = 0;
        audio_frame_number = 0;

        m_width = width;
        m_height = height;

        m_isRunning = true;
        m_thread = new boost::thread( OutputThread( this ) );
    }

    FFMpegEncoder::~FFMpegEncoder()
    {
        m_isRunning = false;
        m_thread->join();
        WP_SAFE_DELETE( m_thread );
    }

    void FFMpegEncoder::_addFrame( int w, int h, u8 *data )
    {
        write_video_frame( oc, video_st, w, h, data );
    }

    void FFMpegEncoder::openFile()
    {
        create( m_fileName.c_str() );
    }

    void FFMpegEncoder::closeFile()
    {
        /* write the trailer, if any.  the trailer must be written
         * before you close the CodecContexts open when you wrote the
         * header; otherwise write_trailer may try to use memory that
         * was freed on av_codec_close() */
        av_write_trailer( oc );

        /* close each codec */
        if( video_st )
            close_video( oc, video_st );
        if( audio_st )
            close_audio( oc, audio_st );

        /* free the streams */
        for( unsigned int i = 0; i < oc->nb_streams; i++ )
        {
            av_freep( &oc->streams[i]->codec );
            av_freep( &oc->streams[i] );
        }

        if( !( fmt->flags & AVFMT_NOFILE ) )
        {
            /* close the output file */
            avio_close( oc->pb );
        }

        /* free the stream */
        av_free( oc );

        WP_LOG_INFO( String( "Encoded: " ) + m_fileName );
    }

    void FFMpegEncoder::addFrame( SmartPtr<IStateMessage> message )
    {
        m_frames.push( message );
    }

    void FFMpegEncoder::addFrame( int w, int h, u8 *data )
    {
    }

    void FFMpegEncoder::addSoundFrame( SmartPtr<IStateMessage> message )
    {
        //m_audioFrames.push(message);
    }

    void FFMpegEncoder::_addSoundFrame( SmartPtr<IStateMessage> message )
    {
        //StateMessageVideoFramePtr msg = message;
        //write_audio_frame(oc, audio_st, (int16_t*)msg->getSoundBuffer(), msg->getSoundBufferSize()/2);
    }

    void FFMpegEncoder::create( const char *filename )
    {
        fmt = av_guess_format( NULL, filename, NULL );
        if( !fmt )
        {
            WP_LOG_INFO( "Error: Could not deduce output format from file extension: using MPEG.\n" );

            fmt = av_guess_format( "mpeg", NULL, NULL );
        }

        if( !fmt )
        {
            WP_LOG_INFO( "Error: Could not find suitable output format.\n" );
            return;
        }

        /* allocate the output media context */
        oc = avformat_alloc_context();
        if( !oc )
        {
            WP_LOG_INFO( "Error: Could not allocate memory for format context.\n" );
            return;
        }

        oc->oformat = fmt;
        sprintf( oc->filename, "%s", filename );

        /* add the audio and video streams using the default format codecs
        and initialize the codecs */
        video_st = NULL;
        audio_st = NULL;
        if( fmt->video_codec != CODEC_ID_NONE )
        {
            video_st = add_video_stream( oc, fmt->video_codec );
        }
        if( fmt->audio_codec != CODEC_ID_NONE )
        {
            audio_st = add_audio_stream( oc, fmt->audio_codec );
        }

        /* set the output parameters (must be done even if no
        parameters). */
        /*if (av_set_parameters(oc, NULL) < 0) {
        fprintf(stderr, "Invalid output format parameters\n");

        }*/

        av_dump_format( oc, 0, filename, 1 );

        /* now that all the parameters are set, we can open the audio and
        video codecs and allocate the necessary encode buffers */
        if( video_st )
            open_video( oc, video_st );
        if( audio_st )
            open_audio( oc, audio_st );

        /* open the output file, if needed */
        if( !( fmt->flags & AVFMT_NOFILE ) )
        {
            if( avio_open( &oc->pb, filename, AVIO_FLAG_READ_WRITE ) < 0 )
            {
                WP_LOG_INFO( String( "Error: Could not open: " ) + String( filename ) + String( "\n" ) );
                return;
            }
        }

        /* write the stream header, if any */
        AVDictionary *options = nullptr;
        avformat_write_header( oc, &options );

        frame_number = 0;

        m_isReady = true;
    }

    AVStream *FFMpegEncoder::add_audio_stream( AVFormatContext *oc, int codec_id )
    {
        AVCodecContext *c;
        AVStream *st;

        st = av_new_stream( oc, 1 );
        if( !st )
        {
            WP_LOG_INFO( "Error: Could not alloc stream.\n" );
            return nullptr;
        }

        c = st->codec;
        c->codec_id = (CodecID)codec_id;
        c->codec_type = AVMEDIA_TYPE_AUDIO;

        /* put sample parameters */
        c->sample_fmt = static_cast<AVSampleFormat>( getAudioSampleFormat() );
        c->bit_rate = getAudioBitRate();
        c->sample_rate = getAudioSampleRate();
        c->channels = getAudioChannels();

        // some formats want stream headers to be separate
        if( oc->oformat->flags & AVFMT_GLOBALHEADER )
            c->flags |= CODEC_FLAG_GLOBAL_HEADER;

        return st;
    }

    void FFMpegEncoder::open_audio( AVFormatContext *oc, AVStream *st )
    {
        audio_codec_ctx = st->codec;

        /* find the audio encoder */
        audio_codec = avcodec_find_encoder( audio_codec_ctx->codec_id );
        if( !audio_codec )
        {
            fprintf( stderr, "codec not found\n" );
        }

        /* open it */
        if( avcodec_open( audio_codec_ctx, audio_codec ) < 0 )
        {
            fprintf( stderr, "could not open codec\n" );
        }

        audio_outbuf_size = getAudioOutputBufferSize();
        audio_outbuf = (uint8_t *)av_malloc( audio_outbuf_size );

        /* ugly hack for PCM codecs (will be removed ASAP with new PCM
        support to compute the input frame size in samples */
        if( audio_codec_ctx->frame_size <= 1 )
        {
            audio_input_frame_size = audio_outbuf_size / audio_codec_ctx->channels;
            switch( st->codec->codec_id )
            {
            case CODEC_ID_PCM_S16LE:
            case CODEC_ID_PCM_S16BE:
            case CODEC_ID_PCM_U16LE:
            case CODEC_ID_PCM_U16BE:
                audio_input_frame_size >>= 1;
                break;
            default:
                break;
            }
        }
        else
        {
            audio_input_frame_size = audio_codec_ctx->frame_size;
        }
        audio_input_frame_size = audio_codec_ctx->frame_size;
    }

    void FFMpegEncoder::write_audio_frame( AVFormatContext *oc, AVStream *st, int16_t *samples_,
                                           int frame_size )
    {
#if 1
        AVCodecContext *audio_codec_context;
        AVPacket pkt;
        av_init_packet( &pkt );

        audio_codec_context = st->codec;

        pkt.size =
            avcodec_encode_audio( audio_codec_context, audio_outbuf, audio_outbuf_size, samples_ );

        if( audio_codec_context->coded_frame && audio_codec_context->coded_frame->pts != AV_NOPTS_VALUE )
            pkt.pts = av_rescale_q( audio_codec_context->coded_frame->pts,
                                    audio_codec_context->time_base, st->time_base );

        pkt.flags |= AV_PKT_FLAG_KEY;
        pkt.stream_index = st->index;
        pkt.data = audio_outbuf;

        if( av_interleaved_write_frame( oc, &pkt ) != 0 )
        {
            WP_LOG_INFO( "Error: Error while writing audio frame.\n" );
        }

        //printf("FFMpegEncoder::write_audio_frame\n");
#else
#endif
    }

    void FFMpegEncoder::close_audio( AVFormatContext *oc, AVStream *st )
    {
        avcodec_close( st->codec );
        av_free( audio_outbuf );
    }

    AVStream *FFMpegEncoder::add_video_stream( AVFormatContext *oc, int codec_id )
    {
        AVStream *st;

        st = av_new_stream( oc, 0 );
        if( !st )
        {
            WP_LOG_INFO( "Error: Could not alloc video stream.\n" );
            return nullptr;
        }

        c = st->codec;
        c->codec_id = (CodecID)codec_id;
        c->codec_type = AVMEDIA_TYPE_VIDEO;

        /* put sample parameters */
        c->bit_rate = getVideoBitRate();
        /* resolution must be a multiple of two */
        c->width = m_width;
        c->height = m_height;
        /* time base: this is the fundamental unit of time (in seconds) in terms
        of which frame timestamps are represented. for fixed-fps content,
        timebase should be 1/framerate and timestamp increments should be
        identically 1. */
        c->time_base.den = fps;
        c->time_base.num = 1;
        c->pix_fmt = static_cast<::PixelFormat>( getStreamPixelFormat() );
        if( c->codec_id == CODEC_ID_MPEG2VIDEO )
        {
            /* just for testing, we also add B frames */
            c->max_b_frames = getMpeg2MaxBFrames();
        }
        if( c->codec_id == CODEC_ID_MPEG1VIDEO )
        {
            /* Needed to avoid using macroblocks in which some coeffs overflow.
            This does not happen with normal video, it just happens here as
            the motion of the chroma plane does not match the luma plane. */
            c->mb_decision = getMpeg1MbDecision();
        }
        // some formats want stream headers to be separate
        if( oc->oformat->flags & AVFMT_GLOBALHEADER )
            c->flags |= CODEC_FLAG_GLOBAL_HEADER;

        /*
        // Set profile to baseline
        av_opt_set(c->priv_data, "profile", "baseline", AV_OPT_SEARCH_CHILDREN);
        //av_opt_set(c->priv_data, "preset", "slow", 0);
        av_opt_set(c->priv_data, "preset", "slow", 0);
        */

        //preset options
        // libx264-medium.ffpreset preset
        c->coder_type = 1;                   // coder = 1
        c->flags |= CODEC_FLAG_LOOP_FILTER;  // flags=+loop
        c->me_cmp |= 1;                      // cmp=+chroma, where CHROMA = 1
        c->me_method = ME_UMH;               // me_method=hex
        c->me_subpel_quality = 6;            // subq=8
        c->me_range = 16;                    // me_range=16
        c->gop_size = getGopSize();          // g=25
        c->keyint_min = getKeyintMin();      // keyint_min=25
        c->scenechange_threshold = 40;       // sc_threshold=40
        c->i_quant_factor = 0.71;            // i_qfactor=0.71
        c->b_frame_strategy = 1;             // b_strategy=2
        c->qcompress = 0.6;                  // qcomp=0.6
        c->qmin = 10;                        // qmin=10
        c->qmax = 51;                        // qmax=51
        c->max_qdiff = 4;                    // qdiff=4
        c->max_b_frames = getMaxBFrames();   // bf=3
        c->refs = 2;                         // refs=3
        c->trellis = 1;                      // trellis=1

        return st;
    }

    AVFrame *FFMpegEncoder::alloc_picture( int pix_fmt, int width, int height )
    {
        AVFrame *picture;
        uint8_t *picture_buf;
        int size;

        picture = avcodec_alloc_frame();
        if( !picture )
            return nullptr;

        size = avpicture_get_size( static_cast<::PixelFormat>( pix_fmt ), width, height );
        picture_buf = (uint8_t *)av_malloc( size );
        if( !picture_buf )
        {
            av_free( picture );
            return nullptr;
        }

        avpicture_fill( (AVPicture *)picture, picture_buf, static_cast<::PixelFormat>( pix_fmt ), width,
                        height );
        return picture;
    }

    void FFMpegEncoder::open_video( AVFormatContext *oc, AVStream *st )
    {
        AVCodec *codec;
        AVCodecContext *c;

        c = st->codec;

        /* find the video encoder */
        codec = avcodec_find_encoder( c->codec_id );
        if( !codec )
        {
            WP_LOG_INFO( "Error: codec not found.\n" );
            return;
        }

        /* open the codec */
        if( avcodec_open( c, codec ) < 0 )
        {
            WP_LOG_INFO( "Error: could not open codec.\n" );
            return;
        }

        video_outbuf = NULL;
        if( !( oc->oformat->flags & AVFMT_RAWPICTURE ) )
        {
            /* allocate output buffer */
            /* XXX: API change will be done */
            /* buffers passed into lav* can be allocated any way you prefer,
            as long as they're aligned enough for the architecture, and
            they're freed appropriately (such as using av_free for buffers
            allocated with av_malloc) */
            video_outbuf_size = getVideoOutputBufferSize();
            video_outbuf = (uint8_t *)av_malloc( video_outbuf_size );
        }

        /* allocate the encoded raw picture */
        picture = alloc_picture( c->pix_fmt, c->width, c->height );
        if( !picture )
        {
            WP_LOG_INFO( "Error: could not alloc picture.\n" );
            return;
        }

        /* if the output format is not YUV420P, then a temporary YUV420P
        picture is needed too. It is then converted to the required
        output format */
        tmp_picture = NULL;
        if( c->pix_fmt != PIX_FMT_YUV420P )
        {
            tmp_picture = alloc_picture( PIX_FMT_YUV420P, c->width, c->height );
            if( !tmp_picture )
            {
                WP_LOG_INFO( "Error: Could not allocate temporary picture.\n" );
                return;
            }
        }
    }

    void FFMpegEncoder::write_video_frame( AVFormatContext *oc, AVStream *st, int w, int h, u8 *data )
    {
        int out_size, ret;
        AVCodecContext *c;
        static struct SwsContext *img_convert_ctx;

        c = st->codec;

        //if (c->pix_fmt != PIX_FMT_YUV420P)
        {
            /* as we only generate a YUV420P picture, we must convert it
            to the codec pixel format if needed */
            if( img_convert_ctx == NULL )
            {
                img_convert_ctx = sws_getContext(
                    c->width, c->height, static_cast<::PixelFormat>( getInputPixelFormat() ), c->width,
                    c->height, c->pix_fmt, getScaleFlags(), NULL, NULL, NULL );
                if( img_convert_ctx == NULL )
                {
                    fprintf( stderr, "Cannot initialize the conversion context\n" );
                }
            }

            int srcstride = w * 3;
            sws_scale( img_convert_ctx, &data, &srcstride, 0, c->height, picture->data,
                       picture->linesize );
        }

        picture->pts = c->frame_number;

        if( oc->oformat->flags & AVFMT_RAWPICTURE )
        {
            /* raw video case. The API will change slightly in the near
            future for that */
            AVPacket pkt;
            av_init_packet( &pkt );

            pkt.flags |= AV_PKT_FLAG_KEY;
            pkt.stream_index = st->index;
            pkt.data = (uint8_t *)picture;
            pkt.size = sizeof( AVPicture );

            ret = av_interleaved_write_frame( oc, &pkt );
        }
        else
        {
            /* encode the image */
            out_size = avcodec_encode_video( c, video_outbuf, video_outbuf_size, picture );
            /* if zero size, it means the image was buffered */
            if( out_size > 0 )
            {
                AVPacket pkt;
                av_init_packet( &pkt );

                if( c->coded_frame->pts != AV_NOPTS_VALUE )
                    pkt.pts = av_rescale_q( c->coded_frame->pts, c->time_base, st->time_base );
                if( c->coded_frame->key_frame )
                    pkt.flags |= AV_PKT_FLAG_KEY;
                pkt.stream_index = st->index;
                pkt.data = video_outbuf;
                pkt.size = out_size;

                /* write the compressed frame in the media file */
                ret = av_interleaved_write_frame( oc, &pkt );
            }
            else
            {
                ret = 0;
            }
        }

        if( ret != 0 )
        {
            WP_LOG_INFO( "Error: Error while writing video frame.\n" );
        }

        //printf("FFMpegEncoder::write_video_frame\n");
    }

    void FFMpegEncoder::close_video( AVFormatContext *oc, AVStream *st )
    {
        avcodec_close( st->codec );
        av_free( picture->data[0] );
        av_free( picture );
        if( tmp_picture )
        {
            av_free( tmp_picture->data[0] );
            av_free( tmp_picture );
        }
        av_free( video_outbuf );
    }

    void FFMpegEncoder::update()
    {
        if( audio_st )
            audio_pts = c->frame_number;
        else
            audio_pts = 0.0;

        if( video_st )
        {
            //video_pts = (1.0/(double)fps) * (double)(c->sample_rate / 1000) * (double)frame_number;
            video_pts = c->frame_number;
        }
        else
            video_pts = 0.0;

        if( ( !audio_st ) && ( !video_st ) )
            return;

        /* write interleaved audio and video frames */
        /*if (!video_st || (video_st && audio_st && audio_pts < video_pts)) {
        write_audio_frame(oc, audio_st);
        }
        else */
        {
        }

        frame_number++;
    }

    void FFMpegEncoder::OutputThread::operator()()
    {
        m_encoder->openFile();

        while( m_encoder->m_isReady == false )
        {
            Thread::sleep( m_encoder->getOutputThreadSleepSeconds() );
        }

        //while(m_encoder->isRunning() ||
        //	!m_encoder->m_frames.empty() )
        //{
        //	{
        //		SmartPtr<IStateMessage> message;
        //		if(!m_encoder->m_frames.empty())
        //		{
        //			if(m_encoder->m_frames.try_pop(message))
        //			{
        //				StateFrameDataPtr buffer = message;
        //			}
        //		}
        //	}

        //	Thread::yield();
        //}

        m_encoder->closeFile();
    }

    SmartPtr<Properties> FFMpegEncoder::getProperties() const
    {
        auto properties = ISharedObject::getProperties();
        properties->setProperty( WidthStr, getWidth() );
        properties->setProperty( HeightStr, getHeight() );
        properties->setProperty( FpsStr, getFps() );
        properties->setProperty( FileNameStr, getFileName() );
        properties->setProperty( RunningStr, isRunning() != 0 );
        properties->setProperty( AudioBitRateStr, getAudioBitRate() );
        properties->setProperty( AudioSampleRateStr, getAudioSampleRate() );
        properties->setProperty( AudioChannelsStr, getAudioChannels() );
        properties->setProperty( AudioSampleFormatStr, getAudioSampleFormat() );
        properties->setProperty( AudioOutputBufferSizeStr, getAudioOutputBufferSize() );
        properties->setProperty( VideoBitRateStr, getVideoBitRate() );
        properties->setProperty( StreamPixelFormatStr, getStreamPixelFormat() );
        properties->setProperty( InputPixelFormatStr, getInputPixelFormat() );
        properties->setProperty( ScaleFlagsStr, getScaleFlags() );
        properties->setProperty( VideoOutputBufferSizeStr, getVideoOutputBufferSize() );
        properties->setProperty( GopSizeStr, getGopSize() );
        properties->setProperty( KeyintMinStr, getKeyintMin() );
        properties->setProperty( MaxBFramesStr, getMaxBFrames() );
        properties->setProperty( Mpeg2MaxBFramesStr, getMpeg2MaxBFrames() );
        properties->setProperty( Mpeg1MbDecisionStr, getMpeg1MbDecision() );
        properties->setProperty( OutputThreadSleepSecondsStr, getOutputThreadSleepSeconds() );

        return properties;
    }

    void FFMpegEncoder::setProperties( SmartPtr<Properties> properties )
    {
        ISharedObject::setProperties( properties );

        auto width = getWidth();
        auto height = getHeight();
        auto currentFps = getFps();
        auto fileName = getFileName();
        auto running = isRunning() != 0;
        auto audioBitRate = getAudioBitRate();
        auto audioSampleRate = getAudioSampleRate();
        auto audioChannels = getAudioChannels();
        auto audioSampleFormat = getAudioSampleFormat();
        auto audioOutputBufferSize = getAudioOutputBufferSize();
        auto videoBitRate = getVideoBitRate();
        auto streamPixelFormat = getStreamPixelFormat();
        auto inputPixelFormat = getInputPixelFormat();
        auto scaleFlags = getScaleFlags();
        auto videoOutputBufferSize = getVideoOutputBufferSize();
        auto gopSize = getGopSize();
        auto keyintMin = getKeyintMin();
        auto maxBFrames = getMaxBFrames();
        auto mpeg2MaxBFrames = getMpeg2MaxBFrames();
        auto mpeg1MbDecision = getMpeg1MbDecision();
        auto outputThreadSleepSeconds = getOutputThreadSleepSeconds();

        properties->getPropertyValue( WidthStr, width );
        properties->getPropertyValue( HeightStr, height );
        properties->getPropertyValue( FpsStr, currentFps );
        properties->getPropertyValue( FileNameStr, fileName );
        properties->getPropertyValue( RunningStr, running );
        properties->getPropertyValue( AudioBitRateStr, audioBitRate );
        properties->getPropertyValue( AudioSampleRateStr, audioSampleRate );
        properties->getPropertyValue( AudioChannelsStr, audioChannels );
        properties->getPropertyValue( AudioSampleFormatStr, audioSampleFormat );
        properties->getPropertyValue( AudioOutputBufferSizeStr, audioOutputBufferSize );
        properties->getPropertyValue( VideoBitRateStr, videoBitRate );
        properties->getPropertyValue( StreamPixelFormatStr, streamPixelFormat );
        properties->getPropertyValue( InputPixelFormatStr, inputPixelFormat );
        properties->getPropertyValue( ScaleFlagsStr, scaleFlags );
        properties->getPropertyValue( VideoOutputBufferSizeStr, videoOutputBufferSize );
        properties->getPropertyValue( GopSizeStr, gopSize );
        properties->getPropertyValue( KeyintMinStr, keyintMin );
        properties->getPropertyValue( MaxBFramesStr, maxBFrames );
        properties->getPropertyValue( Mpeg2MaxBFramesStr, mpeg2MaxBFrames );
        properties->getPropertyValue( Mpeg1MbDecisionStr, mpeg1MbDecision );
        properties->getPropertyValue( OutputThreadSleepSecondsStr, outputThreadSleepSeconds );

        setWidth( width );
        setHeight( height );
        setFps( currentFps );
        setFileName( fileName );
        setRunning( running );
        setAudioBitRate( audioBitRate );
        setAudioSampleRate( audioSampleRate );
        setAudioChannels( audioChannels );
        setAudioSampleFormat( audioSampleFormat );
        setAudioOutputBufferSize( audioOutputBufferSize );
        setVideoBitRate( videoBitRate );
        setStreamPixelFormat( streamPixelFormat );
        setInputPixelFormat( inputPixelFormat );
        setScaleFlags( scaleFlags );
        setVideoOutputBufferSize( videoOutputBufferSize );
        setGopSize( gopSize );
        setKeyintMin( keyintMin );
        setMaxBFrames( maxBFrames );
        setMpeg2MaxBFrames( mpeg2MaxBFrames );
        setMpeg1MbDecision( mpeg1MbDecision );
        setOutputThreadSleepSeconds( outputThreadSleepSeconds );
    }

    int FFMpegEncoder::getWidth() const
    {
        return m_width;
    }
    void FFMpegEncoder::setWidth( int width )
    {
        m_width = Math<s32>::max( 2, width );
    }
    int FFMpegEncoder::getHeight() const
    {
        return m_height;
    }
    void FFMpegEncoder::setHeight( int height )
    {
        m_height = Math<s32>::max( 2, height );
    }
    int FFMpegEncoder::getFps() const
    {
        return fps;
    }
    void FFMpegEncoder::setFps( int value )
    {
        fps = Math<s32>::max( 1, value );
    }
    String FFMpegEncoder::getFileName() const
    {
        return m_fileName;
    }
    void FFMpegEncoder::setFileName( const String &fileName )
    {
        m_fileName = fileName;
    }
    int FFMpegEncoder::getAudioBitRate() const
    {
        return m_audioBitRate;
    }
    void FFMpegEncoder::setAudioBitRate( int audioBitRate )
    {
        m_audioBitRate = Math<s32>::max( 1, audioBitRate );
    }
    int FFMpegEncoder::getAudioSampleRate() const
    {
        return m_audioSampleRate;
    }
    void FFMpegEncoder::setAudioSampleRate( int audioSampleRate )
    {
        m_audioSampleRate = Math<s32>::max( 1, audioSampleRate );
    }
    int FFMpegEncoder::getAudioChannels() const
    {
        return m_audioChannels;
    }
    void FFMpegEncoder::setAudioChannels( int audioChannels )
    {
        m_audioChannels = Math<s32>::max( 1, audioChannels );
    }
    int FFMpegEncoder::getAudioSampleFormat() const
    {
        return m_audioSampleFormat;
    }
    void FFMpegEncoder::setAudioSampleFormat( int audioSampleFormat )
    {
        m_audioSampleFormat = audioSampleFormat;
    }
    int FFMpegEncoder::getAudioOutputBufferSize() const
    {
        return m_audioOutputBufferSize;
    }
    void FFMpegEncoder::setAudioOutputBufferSize( int audioOutputBufferSize )
    {
        m_audioOutputBufferSize = Math<s32>::max( 1, audioOutputBufferSize );
    }
    int FFMpegEncoder::getVideoBitRate() const
    {
        return m_videoBitRate;
    }
    void FFMpegEncoder::setVideoBitRate( int videoBitRate )
    {
        m_videoBitRate = Math<s32>::max( 1, videoBitRate );
    }
    int FFMpegEncoder::getStreamPixelFormat() const
    {
        return m_streamPixelFormat;
    }
    void FFMpegEncoder::setStreamPixelFormat( int streamPixelFormat )
    {
        m_streamPixelFormat = streamPixelFormat;
    }
    int FFMpegEncoder::getInputPixelFormat() const
    {
        return m_inputPixelFormat;
    }
    void FFMpegEncoder::setInputPixelFormat( int inputPixelFormat )
    {
        m_inputPixelFormat = inputPixelFormat;
    }
    int FFMpegEncoder::getScaleFlags() const
    {
        return m_scaleFlags;
    }
    void FFMpegEncoder::setScaleFlags( int scaleFlags )
    {
        m_scaleFlags = scaleFlags;
    }
    int FFMpegEncoder::getVideoOutputBufferSize() const
    {
        return m_videoOutputBufferSize;
    }
    void FFMpegEncoder::setVideoOutputBufferSize( int videoOutputBufferSize )
    {
        m_videoOutputBufferSize = Math<s32>::max( 1, videoOutputBufferSize );
    }
    int FFMpegEncoder::getGopSize() const
    {
        return m_gopSize;
    }
    void FFMpegEncoder::setGopSize( int gopSize )
    {
        m_gopSize = Math<s32>::max( 1, gopSize );
    }
    int FFMpegEncoder::getKeyintMin() const
    {
        return m_keyintMin;
    }
    void FFMpegEncoder::setKeyintMin( int keyintMin )
    {
        m_keyintMin = Math<s32>::max( 1, keyintMin );
    }
    int FFMpegEncoder::getMaxBFrames() const
    {
        return m_maxBFrames;
    }
    void FFMpegEncoder::setMaxBFrames( int maxBFrames )
    {
        m_maxBFrames = Math<s32>::max( 0, maxBFrames );
    }
    int FFMpegEncoder::getMpeg2MaxBFrames() const
    {
        return m_mpeg2MaxBFrames;
    }
    void FFMpegEncoder::setMpeg2MaxBFrames( int mpeg2MaxBFrames )
    {
        m_mpeg2MaxBFrames = Math<s32>::max( 0, mpeg2MaxBFrames );
    }
    int FFMpegEncoder::getMpeg1MbDecision() const
    {
        return m_mpeg1MbDecision;
    }
    void FFMpegEncoder::setMpeg1MbDecision( int mpeg1MbDecision )
    {
        m_mpeg1MbDecision = mpeg1MbDecision;
    }
    float FFMpegEncoder::getOutputThreadSleepSeconds() const
    {
        return m_outputThreadSleepSeconds;
    }
    void FFMpegEncoder::setOutputThreadSleepSeconds( float outputThreadSleepSeconds )
    {
        m_outputThreadSleepSeconds = Math<f32>::max( 0.0f, outputThreadSleepSeconds );
    }

}  // namespace workphone
