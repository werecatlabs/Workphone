#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Graphics/VideoManager.hpp>
#include <Workphone/Core/StringUtil.hpp>

namespace workphone
{
    namespace render
    {
        VideoManager::VideoManager() = default;
        VideoManager::~VideoManager() = default;

        SmartPtr<IVideo> VideoManager::addVideo( const String &fileName )
        {
            auto id = StringUtil::getHash( fileName );
            return addVideo( (hash32)id, fileName );
        }

        SmartPtr<IVideo> VideoManager::addVideo( hash32 id, const String &fileName )
        {
            // Stub: In a real implementation, create a platform-specific IVideo instance
            SmartPtr<IVideo> video;  // = SmartPtr<IVideo>(new PlatformVideo(fileName));
            m_videos[id] = video;
            return video;
        }

        SmartPtr<IVideo> VideoManager::getVideoById( hash32 id ) const
        {
            auto it = m_videos.find( id );
            if( it != m_videos.end() )
                return it->second;
            return nullptr;
        }

        SmartPtr<IVideoTexture> VideoManager::createVideoTexture( const String &textureName )
        {
            // Stub: In a real implementation, create a platform-specific IVideoTexture instance
            SmartPtr<IVideoTexture>
                texture;  // = SmartPtr<IVideoTexture>(new PlatformVideoTexture(textureName));
            m_videoTextures[textureName] = texture;
            return texture;
        }

        bool VideoManager::removeVideoTexture( SmartPtr<IVideoTexture> videoTexture )
        {
            for( auto it = m_videoTextures.begin(); it != m_videoTextures.end(); ++it )
            {
                if( it->second == videoTexture )
                {
                    m_videoTextures.erase( it );
                    return true;
                }
            }
            return false;
        }

        bool VideoManager::removeVideoTexture( const String &textureName )
        {
            return m_videoTextures.erase( textureName ) > 0;
        }

        SmartPtr<IVideoStream> VideoManager::createVideoStream() const
        {
            // Stub: In a real implementation, create a platform-specific IVideoStream instance
            SmartPtr<IVideoStream> stream;  // = SmartPtr<IVideoStream>(new PlatformVideoStream());
            return stream;
        }

        void VideoManager::startCapture()
        {
            m_isCapturing = true;
        }

        void VideoManager::stopCapture()
        {
            m_isCapturing = false;
        }

        bool VideoManager::isCapturing() const
        {
            return m_isCapturing;
        }

        String VideoManager::getOutputFilePath() const
        {
            return m_outputFilePath;
        }

        void VideoManager::setOutputFilePath( const String &filePath )
        {
            m_outputFilePath = filePath;
        }
    }  // namespace render
}  // namespace workphone
