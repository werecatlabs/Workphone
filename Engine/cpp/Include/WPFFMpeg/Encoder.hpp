#ifndef Encoder_h__
#define Encoder_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include "Workphone/Interface/System/IStateMessage.hpp"
#include "Workphone/Core/ConcurrentQueue.hpp"
#include <stdint.h>

struct AVCodec;
struct AVCodecContext;
struct AVFrame;
struct AVOutputFormat;
struct AVFormatContext;
struct AVStream;
struct SwsContext;

namespace boost
{
    class thread;
}

namespace workphone
{

    //--------------------------------------------
    class FFMpegEncoder : public ISharedObject
    {
    public:
        FFMpegEncoder( int width, int height, int fps, const String &fileName, int codec_id );
        ~FFMpegEncoder();

        void create( const char *filename );

        void addFrame( SmartPtr<IStateMessage> message );
        void addFrame( int w, int h, u8 *data );
        void _addFrame( int w, int h, u8 *data );

        void addSoundFrame( SmartPtr<IStateMessage> message );
        void _addSoundFrame( SmartPtr<IStateMessage> message );

        SmartPtr<Properties> getProperties() const override;
        void setProperties( SmartPtr<Properties> properties ) override;

        bool32 isRunning() const
        {
            return m_isRunning;
        }
        void setRunning( bool32 running )
        {
            m_isRunning = running;
        }

        int getWidth() const;
        void setWidth( int width );
        int getHeight() const;
        void setHeight( int height );
        int getFps() const;
        void setFps( int fps );
        String getFileName() const;
        void setFileName( const String &fileName );
        int getAudioBitRate() const;
        void setAudioBitRate( int audioBitRate );
        int getAudioSampleRate() const;
        void setAudioSampleRate( int audioSampleRate );
        int getAudioChannels() const;
        void setAudioChannels( int audioChannels );
        int getAudioSampleFormat() const;
        void setAudioSampleFormat( int audioSampleFormat );
        int getAudioOutputBufferSize() const;
        void setAudioOutputBufferSize( int audioOutputBufferSize );
        int getVideoBitRate() const;
        void setVideoBitRate( int videoBitRate );
        int getStreamPixelFormat() const;
        void setStreamPixelFormat( int streamPixelFormat );
        int getInputPixelFormat() const;
        void setInputPixelFormat( int inputPixelFormat );
        int getScaleFlags() const;
        void setScaleFlags( int scaleFlags );
        int getVideoOutputBufferSize() const;
        void setVideoOutputBufferSize( int videoOutputBufferSize );
        int getGopSize() const;
        void setGopSize( int gopSize );
        int getKeyintMin() const;
        void setKeyintMin( int keyintMin );
        int getMaxBFrames() const;
        void setMaxBFrames( int maxBFrames );
        int getMpeg2MaxBFrames() const;
        void setMpeg2MaxBFrames( int mpeg2MaxBFrames );
        int getMpeg1MbDecision() const;
        void setMpeg1MbDecision( int mpeg1MbDecision );
        float getOutputThreadSleepSeconds() const;
        void setOutputThreadSleepSeconds( float outputThreadSleepSeconds );

        static const String WidthStr;
        static const String HeightStr;
        static const String FpsStr;
        static const String FileNameStr;
        static const String RunningStr;
        static const String AudioBitRateStr;
        static const String AudioSampleRateStr;
        static const String AudioChannelsStr;
        static const String AudioSampleFormatStr;
        static const String AudioOutputBufferSizeStr;
        static const String VideoBitRateStr;
        static const String StreamPixelFormatStr;
        static const String InputPixelFormatStr;
        static const String ScaleFlagsStr;
        static const String VideoOutputBufferSizeStr;
        static const String GopSizeStr;
        static const String KeyintMinStr;
        static const String MaxBFramesStr;
        static const String Mpeg2MaxBFramesStr;
        static const String Mpeg1MbDecisionStr;
        static const String OutputThreadSleepSecondsStr;

    protected:
        struct OutputThread
        {
            OutputThread( FFMpegEncoder *encoder ) : m_encoder( encoder )
            {
            }

            void operator()();

            FFMpegEncoder *m_encoder;
        };

        void update();

        void openFile();
        void closeFile();

        void close_video( AVFormatContext *oc, AVStream *st );
        void write_video_frame( AVFormatContext *oc, AVStream *st, int w, int h, u8 *data );
        void open_video( AVFormatContext *oc, AVStream *st );
        AVFrame *alloc_picture( int pix_fmt, int width, int height );
        AVStream *add_video_stream( AVFormatContext *oc, int codec_id );
        void close_audio( AVFormatContext *oc, AVStream *st );
        void write_audio_frame( AVFormatContext *oc, AVStream *st, int16_t *samples, int frame_size );
        void open_audio( AVFormatContext *oc, AVStream *st );
        AVStream *add_audio_stream( AVFormatContext *oc, int codec_id );

        boost::thread *m_thread;
        atomic_bool m_isRunning;
        atomic_bool m_isReady;

        double audio_pts, video_pts;

        AVOutputFormat *fmt;
        AVFormatContext *oc;
        AVStream *audio_st, *video_st;

        AVFormatContext *format;
        AVCodec *codec;
        AVCodecContext *c;
        int out_size, x, y, outbuf_size;

        /* video output */
        AVFrame *picture;
        AVFrame *tmp_picture;
        uint8_t *video_outbuf;
        int video_outbuf_size;
        int m_width;
        int m_height;
        int frame_number;
        int fps;
        int video_frame_number;
        int m_audioBitRate = 64000;
        int m_audioSampleRate = 44100;
        int m_audioChannels = 2;
        int m_audioSampleFormat = 0;
        int m_audioOutputBufferSize = 10000;
        int m_videoBitRate = 800000;
        int m_streamPixelFormat = 0;
        int m_inputPixelFormat = 0;
        int m_scaleFlags = 0;
        int m_videoOutputBufferSize = 200000;
        int m_gopSize = 250;
        int m_keyintMin = 25;
        int m_maxBFrames = 16;
        int m_mpeg2MaxBFrames = 2;
        int m_mpeg1MbDecision = 2;
        float m_outputThreadSleepSeconds = 1.0f;

        /* audio output */
        AVCodecContext *audio_codec_ctx;
        AVCodec *audio_codec;
        uint8_t *audio_outbuf;
        int audio_outbuf_size;
        int audio_input_frame_size;
        int audio_frame_number;

        String m_fileName;

        ConcurrentQueue<SmartPtr<IStateMessage>> m_frames;
    };

}  // namespace workphone

#endif  // Encoder_h__
