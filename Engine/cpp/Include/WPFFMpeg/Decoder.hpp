#ifndef Decoder_h__
#define Decoder_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>
#include <Workphone/Interface/IO/IStream.hpp>

struct AVPacket;
struct AVCodec;
struct AVCodecContext;
struct AVFrame;
struct AVOutputFormat;
struct AVFormatContext;
struct AVStream;
struct SwsContext;

namespace workphone
{

    //--------------------------------------------
    class Decoder : public ISharedObject
    {
    public:
        Decoder();
        ~Decoder();

        int initialise( const String &fileName );
        void destroy();

        SmartPtr<IStateMessage> getNextFrame();

        SmartPtr<Properties> getProperties() const override;
        void setProperties( SmartPtr<Properties> properties ) override;

        bool isRunning() const
        {
            return m_isRunning;
        }
        void setRunning( bool running )
        {
            m_isRunning = running;
        }

        bool isPlaying() const
        {
            return m_isPlaying;
        }
        void setPlaying( bool playing )
        {
            m_isPlaying = playing;
        }

        bool getLoop() const
        {
            return m_loop;
        }
        void setLoop( bool loop )
        {
            m_loop = loop;
        }

        u32 getUpdateCount() const
        {
            return m_updateCount;
        }
        void setUpdateCount( u32 updateCount )
        {
            m_updateCount = updateCount;
        }

        Vector2I getSize() const
        {
            return m_size;
        }
        void setSize( const Vector2I &size )
        {
            m_size = size;
        }

        int getPixelFormat() const
        {
            return m_pixelFormat;
        }
        void setPixelFormat( int pixelFormat )
        {
            m_pixelFormat = pixelFormat;
        }

        SmartPtr<IStream> &getStream()
        {
            return m_stream;
        }

        String getFileName() const;
        void setFileName( const String &fileName );

        s32 getMaxPictureBufferSize() const;
        void setMaxPictureBufferSize( s32 maxPictureBufferSize );

        s32 getIoBufferSize() const;
        void setIoBufferSize( s32 ioBufferSize );

        String getInputStreamName() const;
        void setInputStreamName( const String &inputStreamName );

        s32 getInputProbeSize() const;
        void setInputProbeSize( s32 inputProbeSize );

        s32 getInputMaxProbeSize() const;
        void setInputMaxProbeSize( s32 inputMaxProbeSize );

        s32 getCodecThreadCount() const;
        void setCodecThreadCount( s32 codecThreadCount );

        s32 getScaleFlags() const;
        void setScaleFlags( s32 scaleFlags );

        s32 getBytesPerPixel() const;
        void setBytesPerPixel( s32 bytesPerPixel );

        static const String FileNameStr;
        static const String SizeStr;
        static const String PixelFormatStr;
        static const String LoopStr;
        static const String PlayingStr;
        static const String RunningStr;
        static const String UpdateCountStr;
        static const String MaxPictureBufferSizeStr;
        static const String IoBufferSizeStr;
        static const String InputStreamNameStr;
        static const String InputProbeSizeStr;
        static const String InputMaxProbeSizeStr;
        static const String CodecThreadCountStr;
        static const String ScaleFlagsStr;
        static const String BytesPerPixelStr;

    protected:
        bool decodeFrame();

        SmartPtr<IStream> m_stream;

        AVPacket *m_packet;

        SwsContext *m_convertCtx;

        AVFormatContext *m_formatCtx;
        AVCodecContext *m_codecCtx;
        AVCodec *m_codec;
        AVFrame *m_frame;
        AVFrame *m_frameRGB;
        u8 *m_buffer;

        Vector2I m_size;

        s32 m_width;   //to remove
        s32 m_height;  //to remove

        s32 m_numBytes;
        s32 m_videoStream;

        s32 m_pixelFormat;
        s32 m_maxPictureBufferSize = 2048 * 2048 * 4;
        s32 m_ioBufferSize = 2048 * 2048 * 4;
        s32 m_inputProbeSize = 0;
        s32 m_inputMaxProbeSize = 0;
        s32 m_codecThreadCount = 0;
        s32 m_scaleFlags = 0;
        s32 m_bytesPerPixel = 4;

        atomic_u32 m_updateCount;
        atomic_bool m_isRunning;
        atomic_bool m_isReady;
        atomic_bool m_isPlaying;
        atomic_bool m_loop;

        String m_fileName;
        String m_inputStreamName = "stream";
    };

    typedef SmartPtr<Decoder> DecoderPtr;

}  // namespace workphone

#endif  // Decoder_h__
