#ifndef Video_h__
#define Video_h__

#include "Workphone/Interface/Graphics/IVideo.hpp"
#include "Workphone/Core/ConcurrentQueue.hpp"
#include <Workphone/Thread/RecursiveSpinMutex.hpp>

namespace workphone
{

    class Decoder;

    class Video : public render::IVideo
    {
    public:
        Video();
        Video( u32 id );
        ~Video();

        void initialise( const String &fileName );

        void update();

        SmartPtr<Properties> getProperties() const override;
        void setProperties( SmartPtr<Properties> properties ) override;

        u32 getId() const;
        void setId( u32 id );

        void play();
        void stop();

        Vector2I getSize() const;
        void setSize( const Vector2I &size );

        void *getCurrentFrameBuffer() const;

        bool getAutoUpdate() const;
        void setAutoUpdate( bool autoUpdate );

        bool getLoop() const;
        void setLoop( bool loop );

        String getFileName() const;
        void setFileName( const String &fileName );

        u32 getLastUpdate() const;
        void setLastUpdate( u32 lastUpdate );

        u32 getNextUpdateTime() const;
        void setNextUpdateTime( u32 nextUpdateTime );

        bool getUsePowerOfTwoTextureSize() const;
        void setUsePowerOfTwoTextureSize( bool usePowerOfTwoTextureSize );

        s32 getBytesPerPixel() const;
        void setBytesPerPixel( s32 bytesPerPixel );

        SmartPtr<render::IVideoTexture> getVideoTexture() const;
        void setVideoTexture( SmartPtr<render::IVideoTexture> videoTexture );

        void _getObject( void **object );

        SmartPtr<IStateContext> &getStateContext();

        const SmartPtr<IStateContext> &getStateContext() const;

        static const String IdStr;
        static const String FileNameStr;
        static const String SizeStr;
        static const String LoopStr;
        static const String AutoUpdateStr;
        static const String LastUpdateStr;
        static const String NextUpdateTimeStr;
        static const String UsePowerOfTwoTextureSizeStr;
        static const String BytesPerPixelStr;

    protected:
        SmartPtr<render::IVideoTexture> m_videoTexture;
        SmartPtr<IStateContext> m_stateContext;

        Vector2I m_size;
        u32 m_id = 0;
        bool m_loop = false;
        bool m_autoUpdate = true;
        u32 m_nextUpdateTime = 0;
        u32 m_lastUpdate = 0;
        bool m_usePowerOfTwoTextureSize = true;
        s32 m_bytesPerPixel = 4;
        String m_fileName;

        Decoder *m_decoder;

        ConcurrentQueue<SmartPtr<IStateMessage>> m_frames;
        Array<SmartPtr<IStateMessage>> m_displayQueue;

        mutable RecursiveSpinMutex MUTEX;

        static u32 m_idExt;
    };

    typedef SmartPtr<Video> FFMpegVideoPtr;

}  // namespace workphone

#endif  // Video_h__
