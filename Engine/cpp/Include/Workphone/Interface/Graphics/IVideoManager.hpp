#ifndef IVideoManager_h__
#define IVideoManager_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief An interface for a video manager class.
         */
        class WPCore_API IVideoManager : public ISharedObject
        {
        public:
            /**
             * @brief Destructor for IVideoManager.
             */
            ~IVideoManager() override;

            /**
             * @brief Adds a video to the manager from a file.
             * @param fileName The name of the video file.
             * @return A shared pointer to the newly added video object.
             */
            virtual SmartPtr<IVideo> addVideo( const String &fileName ) = 0;

            /**
             * @brief Adds a video to the manager with a specific ID from a file.
             * @param id The ID of the video.
             * @param fileName The name of the video file.
             * @return A shared pointer to the newly added video object.
             */
            virtual SmartPtr<IVideo> addVideo( hash32 id, const String &fileName ) = 0;

            /**
             * @brief Gets a video from the manager by its ID.
             * @param id The ID of the video.
             * @return A shared pointer to the video object.
             */
            virtual SmartPtr<IVideo> getVideoById( hash32 id ) const = 0;

            /**
             * @brief Creates a new video texture object.
             * @param textureName The name of the new texture.
             * @return A shared pointer to the new video texture object.
             */
            virtual SmartPtr<IVideoTexture> createVideoTexture( const String &textureName ) = 0;

            /**
             * @brief Removes a video texture object from the manager.
             * @param videoTexture A shared pointer to the video texture object to remove.
             * @return True if the object was successfully removed, false otherwise.
             */
            virtual bool removeVideoTexture( SmartPtr<IVideoTexture> videoTexture ) = 0;

            /**
             * @brief Removes a video texture object from the manager by its name.
             * @param textureName The name of the video texture object to remove.
             * @return True if the object was successfully removed, false otherwise.
             */
            virtual bool removeVideoTexture( const String &textureName ) = 0;

            /**
             * @brief Creates a new video stream object.
             * @return A shared pointer to the new video stream object.
             */
            virtual SmartPtr<IVideoStream> createVideoStream() const = 0;

            /**
             * @brief Starts capturing video.
             */
            virtual void startCapture() = 0;

            /**
             * @brief Stops capturing video.
             */
            virtual void stopCapture() = 0;

            /**
             * @brief Returns whether video is currently being captured.
             * @return True if video is being captured, false otherwise.
             */
            virtual bool isCapturing() const = 0;

            /**
             * @brief Gets the output file path for captured video.
             * @return The output file path for captured video.
             */
            virtual String getOutputFilePath() const = 0;

            /**
             * @brief Sets the output file path for captured video.
             * @param filePath The new output file path for captured video.
             */
            virtual void setOutputFilePath( const String &filePath ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace render
}  // namespace workphone

#endif  // IVideoManager_h__
