#ifndef IVideo_h__
#define IVideo_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @class IVideo
         * @brief Interface for a video playback and rendering object.
         *
         * This interface provides methods for controlling video playback, managing video properties such
         * as size and looping, accessing the current frame buffer, and handling the associated video
         * texture. Implementations of this interface are responsible for providing the actual video
         * decoding and rendering functionality.
         *
         * @note This class inherits from ISharedObject for shared ownership semantics.
         */
        class WPCore_API IVideo : public ISharedObject
        {
        public:
            /**
             * @brief Virtual destructor for IVideo.
             *
             * Ensures proper cleanup of derived video objects.
             */
            ~IVideo() override;

            /**
             * @brief Starts or resumes video playback.
             *
             * Implementations should begin decoding and displaying video frames.
             */
            virtual void play() = 0;

            /**
             * @brief Stops video playback.
             *
             * Implementations should halt decoding and displaying video frames, and may reset playback
             * position.
             */
            virtual void stop() = 0;

            /**
             * @brief Retrieves the current size (width and height) of the video in pixels.
             * @return The size of the video as a Vector2I (width, height).
             */
            virtual Vector2I getSize() const = 0;

            /**
             * @brief Sets the size (width and height) of the video.
             * @param size The new size of the video as a Vector2I (width, height).
             *
             * Implementations may scale or resize the video output accordingly.
             */
            virtual void setSize( const Vector2I &size ) = 0;

            /**
             * @brief Gets a pointer to the current frame buffer data.
             * @return Pointer to the frame buffer containing the current video frame.
             *
             * The format and ownership of the buffer are implementation-defined.
             */
            virtual void *getCurrentFrameBuffer() const = 0;

            /**
             * @brief Enables or disables looping of the video playback.
             * @param loop If true, the video will loop when it reaches the end; otherwise, it will stop.
             */
            virtual void setLoop( bool loop ) = 0;

            /**
             * @brief Checks whether video playback is set to loop.
             * @return True if looping is enabled, false otherwise.
             */
            virtual bool getLoop() const = 0;

            /**
             * @brief Enables or disables automatic updating of the video frames.
             * @param autoUpdate If true, the video will update frames automatically; otherwise, manual
             * update may be required.
             */
            virtual void setAutoUpdate( bool autoUpdate ) = 0;

            /**
             * @brief Checks whether automatic frame updating is enabled.
             * @return True if auto-update is enabled, false otherwise.
             */
            virtual bool getAutoUpdate() const = 0;

            /**
             * @brief Retrieves the smart pointer to the associated video texture object.
             * @return SmartPtr to the IVideoTexture used for rendering the video.
             */
            virtual SmartPtr<IVideoTexture> getVideoTexture() const = 0;

            /**
             * @brief Sets the video texture object to be used for rendering.
             * @param videoTexture SmartPtr to the new IVideoTexture.
             */
            virtual void setVideoTexture( SmartPtr<IVideoTexture> videoTexture ) = 0;

            /**
             * @brief Retrieves the underlying implementation object pointer.
             * @param object Output pointer to receive the implementation-specific object.
             *
             * This is typically used for integration with platform-specific or third-party APIs.
             */
            virtual void _getObject( void **object ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace render
}  // namespace workphone

#endif  // IVideo_h__
