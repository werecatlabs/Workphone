#ifndef VideoManager_h__
#define VideoManager_h__

#include <Workphone/Interface/Graphics/IVideoManager.hpp>
#include <Workphone/Interface/Graphics/IVideo.hpp>
#include <Workphone/Interface/Graphics/IVideoTexture.hpp>
#include <Workphone/Interface/Graphics/IVideoStream.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/WorkphoneTypes.hpp>
#include <unordered_map>
#include <string>

namespace workphone
{
    namespace render
    {
        /**
         * @brief VideoManager handles video playback and video texture management across multiple
         * platforms.
         *
         * This class provides a unified interface for video playback, video texture creation, and video
         * capture functionality. It uses platform-specific backends for optimal performance and
         * compatibility:
         *   - Windows: Uses Windows Media Foundation (WMF) or FFmpeg depending on configuration.
         *   - Android: Uses MediaCodec.
         *   - iOS: Uses AVFoundation.
         *   - Linux: Uses an FFmpeg-based backend (optionally GStreamer if configured).
         *
         * VideoManager is responsible for managing the lifecycle of video objects and video textures, as
         * well as handling video capture operations.
         */
        class WPCore_API VideoManager : public IVideoManager
        {
        public:
            /**
             * @brief Constructs a new VideoManager instance.
             */
            VideoManager();

            /**
             * @brief Destroys the VideoManager instance and releases all managed resources.
             */
            ~VideoManager() override;

            /**
             * @brief Adds a video for playback from the specified file.
             * @param fileName The path to the video file.
             * @return A smart pointer to the created IVideo instance.
             */
            SmartPtr<IVideo> addVideo( const String &fileName ) override;

            /**
             * @brief Adds a video for playback with a specific identifier.
             * @param id The unique hash identifier for the video.
             * @param fileName The path to the video file.
             * @return A smart pointer to the created IVideo instance.
             */
            SmartPtr<IVideo> addVideo( hash32 id, const String &fileName ) override;

            /**
             * @brief Retrieves a video by its unique identifier.
             * @param id The hash identifier of the video.
             * @return A smart pointer to the IVideo instance, or nullptr if not found.
             */
            SmartPtr<IVideo> getVideoById( hash32 id ) const override;

            /**
             * @brief Creates a video texture with the specified name.
             * @param textureName The name for the video texture.
             * @return A smart pointer to the created IVideoTexture instance.
             */
            SmartPtr<IVideoTexture> createVideoTexture( const String &textureName ) override;

            /**
             * @brief Removes a video texture by pointer.
             * @param videoTexture The smart pointer to the video texture to remove.
             * @return True if the texture was removed, false otherwise.
             */
            bool removeVideoTexture( SmartPtr<IVideoTexture> videoTexture ) override;

            /**
             * @brief Removes a video texture by name.
             * @param textureName The name of the video texture to remove.
             * @return True if the texture was removed, false otherwise.
             */
            bool removeVideoTexture( const String &textureName ) override;

            /**
             * @brief Creates a new video stream instance.
             * @return A smart pointer to the created IVideoStream instance.
             */
            SmartPtr<IVideoStream> createVideoStream() const override;

            /**
             * @brief Starts video capture to the output file path.
             */
            void startCapture() override;

            /**
             * @brief Stops the ongoing video capture.
             */
            void stopCapture() override;

            /**
             * @brief Checks if video capture is currently active.
             * @return True if capturing, false otherwise.
             */
            bool isCapturing() const override;

            /**
             * @brief Gets the current output file path for video capture.
             * @return The output file path as a string.
             */
            String getOutputFilePath() const override;

            /**
             * @brief Sets the output file path for video capture.
             * @param filePath The new output file path.
             */
            void setOutputFilePath( const String &filePath ) override;

        private:
            /**
             * @brief Map of video hash IDs to their corresponding video objects.
             */
            std::unordered_map<hash32, SmartPtr<IVideo>> m_videos;

            /**
             * @brief Map of video texture names to their corresponding video texture objects.
             */
            std::unordered_map<String, SmartPtr<IVideoTexture>> m_videoTextures;

            /**
             * @brief Indicates whether video capture is currently active.
             */
            bool m_isCapturing = false;

            /**
             * @brief The output file path for video capture.
             */
            String m_outputFilePath;
        };

    }  // namespace render
}  // namespace workphone

#endif  // VideoManager_h__
