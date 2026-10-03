#ifndef VideoManager_h__
#define VideoManager_h__

#include <Workphone/Interface/Graphics/IVideoManager.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/HashMap.hpp>

namespace workphone
{

    class VideoStream;

    class VideoManager : public render::IVideoManager
    {
    public:
        VideoManager();
        ~VideoManager();

        void update();

        SmartPtr<Properties> getProperties() const override;
        void setProperties( SmartPtr<Properties> properties ) override;

        SmartPtr<render::IVideo> getVideoById( hash32 id ) const;

        SmartPtr<render::IVideo> addVideo( const String &fileName );
        SmartPtr<render::IVideo> addVideo( hash32 id, const String &fileName );

        SmartPtr<render::IVideoTexture> createVideoTexture( const String &textureName );
        bool removeVideoTexture( SmartPtr<render::IVideoTexture> videoTexture );
        bool removeVideoTexture( const String &textureName );

        SmartPtr<render::IVideoStream> createVideoStream() const;

        void startCapture();
        void stopCapture();
        bool isCapturing() const;

        void setOutputFilePath( const String &filePath );
        String getOutputFilePath() const;

        bool getUpdateVideos() const;
        void setUpdateVideos( bool updateVideos );

        bool getAutoRegisterFormats() const;
        void setAutoRegisterFormats( bool autoRegisterFormats );

        static const String IsCapturingStr;
        static const String OutputFilePathStr;
        static const String UpdateVideosStr;
        static const String AutoRegisterFormatsStr;

    protected:
        typedef HashMap<u32, SmartPtr<render::IVideo>> Videos;
        Videos m_videos;

        bool m_isCapturing;
        bool m_updateVideos = false;
        bool m_autoRegisterFormats = true;
        String m_outputFilePath;
        VideoStream *m_videoStream;
    };

    typedef SmartPtr<VideoManager> FFMpegVideoManagerPtr;

}  // namespace workphone

#endif  // VideoManager_h__
