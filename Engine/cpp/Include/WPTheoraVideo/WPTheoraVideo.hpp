#ifndef WPTheoraVideo_h__
#define WPTheoraVideo_h__

#include <Workphone/Interface/Graphics/IVideo.hpp>
#include <Workphone/Interface/Graphics/IVideoTexture.hpp>
#include <Workphone/Core/StringTypes.hpp>

class TheoraVideoClip;

namespace workphone
{
    class TheoraVideo : public render::IVideo
    {
    public:
        TheoraVideo();
        explicit TheoraVideo( hash32 id );
        ~TheoraVideo() override;

        void update( u32 taskId, f64 time, f64 deltaTime );

        void play() override;
        void stop() override;

        Vector2I getSize() const override;
        void setSize( const Vector2I &size ) override;

        void *getCurrentFrameBuffer() const override;

        void setLoop( bool loop ) override;
        bool getLoop() const override;

        void setAutoUpdate( bool autoUpdate ) override;
        bool getAutoUpdate() const override;

        SmartPtr<render::IVideoTexture> getVideoTexture() const override;
        void setVideoTexture( SmartPtr<render::IVideoTexture> videoTexture ) override;

        void _getObject( void **object ) override;

        SmartPtr<Properties> getProperties() const override;
        void setProperties( SmartPtr<Properties> properties ) override;

        TheoraVideoClip *getClip() const;
        void setClip( TheoraVideoClip *clip );

        hash32 getId() const;
        void setId( hash32 id );

        String getFileName() const;
        void setFileName( const String &fileName );

        bool getRestartOnPlay() const;
        void setRestartOnPlay( bool restartOnPlay );

        bool getPopFramesAfterUpload() const;
        void setPopFramesAfterUpload( bool popFramesAfterUpload );

        bool getUseFrameStride() const;
        void setUseFrameStride( bool useFrameStride );

        s32 getBytesPerPixel() const;
        void setBytesPerPixel( s32 bytesPerPixel );

        u32 getLastTaskId() const;
        f64 getLastUpdateTime() const;
        f64 getLastDeltaTime() const;
        u32 getFrameUploadCount() const;

        static const String IdStr;
        static const String FileNameStr;
        static const String SizeStr;
        static const String AutoUpdateStr;
        static const String LoopStr;
        static const String RestartOnPlayStr;
        static const String PopFramesAfterUploadStr;
        static const String UseFrameStrideStr;
        static const String BytesPerPixelStr;
        static const String LastTaskIdStr;
        static const String LastUpdateTimeStr;
        static const String LastDeltaTimeStr;
        static const String FrameUploadCountStr;

    protected:
        TheoraVideoClip *m_clip = nullptr;
        SmartPtr<render::IVideoTexture> m_videoTexture;
        Vector2I m_size = Vector2I::zero();
        hash32 m_id = 0;
        String m_fileName;
        bool m_autoUpdate = true;
        bool m_loop = false;
        bool m_restartOnPlay = true;
        bool m_popFramesAfterUpload = true;
        bool m_useFrameStride = true;
        s32 m_bytesPerPixel = 4;
        u32 m_lastTaskId = 0;
        f64 m_lastUpdateTime = 0.0;
        f64 m_lastDeltaTime = 0.0;
        u32 m_frameUploadCount = 0;

        static u32 m_idExt;
    };

    using TheoraVideoPtr = SmartPtr<TheoraVideo>;
}  // namespace workphone

#endif  // WPTheoraVideo_h__
