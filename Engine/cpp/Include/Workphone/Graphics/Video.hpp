#ifndef Video_h__
#define Video_h__

#include <Workphone/Interface/Graphics/IVideo.hpp>
#include <Workphone/Memory/SmartPtr.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Implementation of the IVideo interface for video playback and rendering.
         *
         * This class provides a cross-platform video playback solution, abstracting platform-specific
         * video player implementations behind a unified interface. It supports video playback, frame
         * retrieval, looping, auto-update, and integration with video textures for rendering.
         *
         * Platform-specific backends:
         *   - Windows: Uses WMF (Windows Media Foundation) or FFmpeg depending on configuration.
         *   - Android: Uses MediaCodec.
         *   - iOS: Uses AVFoundation.
         *   - Linux: Uses FFmpeg-based backend (optionally GStreamer if configured).
         *
         * @note The actual video decoding and playback is delegated to a platform-specific
         * implementation held in m_impl.
         */
        class WPCore_API Video : public IVideo
        {
        public:
            /**
             * @brief Default constructor.
             *
             * Initializes the video object without loading any file.
             */
            Video();

            /**
             * @brief Constructs a Video object and loads the specified file.
             * @param fileName The path to the video file to load.
             */
            Video( const String &fileName );

            /**
             * @brief Destructor. Cleans up resources used by the video object.
             */
            ~Video() override;

            /**
             * @brief Starts or resumes video playback.
             */
            void play() override;

            /**
             * @brief Stops video playback.
             */
            void stop() override;

            /**
             * @brief Gets the dimensions of the video in pixels.
             * @return The size of the video as a Vector2I (width, height).
             */
            Vector2I getSize() const override;

            /**
             * @brief Sets the dimensions for video rendering.
             * @param size The desired size as a Vector2I (width, height).
             */
            void setSize( const Vector2I &size ) override;

            /**
             * @brief Retrieves a pointer to the current frame's pixel buffer.
             * @return Pointer to the frame buffer data.
             */
            void *getCurrentFrameBuffer() const override;

            /**
             * @brief Enables or disables looping of the video playback.
             * @param loop True to enable looping, false to disable.
             */
            void setLoop( bool loop ) override;

            /**
             * @brief Checks if video playback is set to loop.
             * @return True if looping is enabled, false otherwise.
             */
            bool getLoop() const override;

            /**
             * @brief Enables or disables automatic updating of the video frame.
             * @param autoUpdate True to enable auto-update, false to disable.
             */
            void setAutoUpdate( bool autoUpdate ) override;

            /**
             * @brief Checks if automatic frame updating is enabled.
             * @return True if auto-update is enabled, false otherwise.
             */
            bool getAutoUpdate() const override;

            /**
             * @brief Gets the video texture associated with this video.
             * @return Smart pointer to the IVideoTexture object.
             */
            SmartPtr<IVideoTexture> getVideoTexture() const override;

            /**
             * @brief Sets the video texture to be used for rendering this video.
             * @param videoTexture Smart pointer to the IVideoTexture object.
             */
            void setVideoTexture( SmartPtr<IVideoTexture> videoTexture ) override;

            /**
             * @brief Retrieves the underlying platform-specific video object.
             * @param object Output pointer to receive the platform-specific object.
             */
            void _getObject( void **object ) override;

        private:
            /// Platform-specific implementation of the IVideo interface.
            SmartPtr<IVideo> m_impl;
        };

    }  // namespace render
}  // namespace workphone

#endif  // Video_h__
