#ifndef WPTheoraVideoManager_h__
#define WPTheoraVideoManager_h__

#include <Workphone/Interface/Graphics/IVideoManager.hpp>
#include <Workphone/Interface/Graphics/IVideoStream.hpp>
#include <Workphone/Core/HashMap.hpp>

class TheoraVideoManager;

namespace workphone
{
    class TheoraVideoMgr : public render::IVideoManager
    {
    public:
        TheoraVideoMgr();
        ~TheoraVideoMgr() override;

        void update();

        SmartPtr<Properties> getProperties() const override;
        void setProperties( SmartPtr<Properties> properties ) override;

        SmartPtr<render::IVideo> addVideo( const String &fileName ) override;
        SmartPtr<render::IVideo> addVideo( hash32 id, const String &fileName ) override;
        SmartPtr<render::IVideo> getVideoById( hash32 id ) const override;

        SmartPtr<render::IVideoTexture> createVideoTexture( const String &textureName ) override;
        bool removeVideoTexture( SmartPtr<render::IVideoTexture> videoTexture ) override;
        bool removeVideoTexture( const String &textureName ) override;

        SmartPtr<render::IVideoStream> createVideoStream() const override;

        void startCapture() override;
        void stopCapture() override;
        bool isCapturing() const override;

        void setOutputFilePath( const String &filePath ) override;
        String getOutputFilePath() const override;

        u32 getWorkerThreads() const;
        void setWorkerThreads( u32 workerThreads );

        s32 getOutputPixelFormat() const;
        void setOutputPixelFormat( s32 outputPixelFormat );

        s32 getPrecacheStrategy() const;
        void setPrecacheStrategy( s32 precacheStrategy );

        u32 getMaxPrecachedFrames() const;
        void setMaxPrecachedFrames( u32 maxPrecachedFrames );

        bool getAutoRestart() const;
        void setAutoRestart( bool autoRestart );

        bool getPrecacheAllFrames() const;
        void setPrecacheAllFrames( bool precacheAllFrames );

        bool getUpdateVideos() const;
        void setUpdateVideos( bool updateVideos );

        bool getTheoraAvailable() const;

        static const String IsCapturingStr;
        static const String OutputFilePathStr;
        static const String WorkerThreadsStr;
        static const String OutputPixelFormatStr;
        static const String PrecacheStrategyStr;
        static const String MaxPrecachedFramesStr;
        static const String AutoRestartStr;
        static const String PrecacheAllFramesStr;
        static const String UpdateVideosStr;
        static const String TheoraAvailableStr;

    protected:
        SmartPtr<render::IVideo> createVideo( hash32 id, const String &fileName );
        void applyClipSettings( SmartPtr<render::IVideo> video );

        TheoraVideoManager *m_mgr = nullptr;

        using Videos = HashMap<hash32, SmartPtr<render::IVideo>>;
        Videos m_videos;

        bool m_isCapturing = false;
        String m_outputFilePath;
        u32 m_workerThreads = 1;
        s32 m_outputPixelFormat = 0;
        s32 m_precacheStrategy = 0;
        u32 m_maxPrecachedFrames = 300;
        bool m_autoRestart = false;
        bool m_precacheAllFrames = false;
        bool m_updateVideos = true;
    };

    using TheoraVideoManagerPtr = SmartPtr<TheoraVideoMgr>;
}  // namespace workphone

#endif  // WPTheoraVideoManager_h__
