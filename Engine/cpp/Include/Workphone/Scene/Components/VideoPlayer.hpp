#ifndef VideoPlayer_h__
#define VideoPlayer_h__

#include <Workphone/Scene/Components/Component.hpp>
#include <Workphone/Interface/Graphics/IVideo.hpp>
#include <Workphone/Interface/Graphics/IVideoTexture.hpp>

namespace workphone
{
    namespace scene
    {

        /**
         * @class VideoPlayer
         * @brief Component for playing video files in the scene.
         *
         * This component provides functionality to load, play, pause, and stop video files.
         * It integrates with the graphics system's video manager and can be configured
         * through properties for auto-play, looping, and other video playback settings.
         */
        class WPCore_API VideoPlayer : public Component
        {
        public:
            /** String identifier for the video path property. */
            static const String videoPathStr;

            /** String identifier for the auto play property. */
            static const String autoPlayStr;

            /** String identifier for the loop property. */
            static const String loopStr;

            /** String identifier for the auto update property. */
            static const String autoUpdateStr;

            /** String identifier for the play button. */
            static const String playStr;

            /** String identifier for the stop button. */
            static const String stopStr;

            /** String identifier for the pause button. */
            static const String pauseStr;

            /**
             * @brief Default constructor.
             */
            VideoPlayer();

            /**
             * @brief Destructor.
             */
            ~VideoPlayer() override;

            /**
             * @copydoc Component::load
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc Component::unload
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Starts video playback.
             */
            void play();

            /**
             * @brief Stops video playback.
             */
            void stop();

            /**
             * @brief Pauses video playback.
             */
            void pause();

            /**
             * @brief Checks if the video is currently playing.
             * @return True if the video is playing, false otherwise.
             */
            bool isPlaying() const;

            /**
             * @brief Gets the path to the video file.
             * @return The video file path.
             */
            String getVideoPath() const;

            /**
             * @brief Sets the path to the video file.
             * @param videoPath The path to the video file.
             */
            void setVideoPath( const String &videoPath );

            /**
             * @brief Gets the video texture used for rendering.
             * @return Smart pointer to the video texture.
             */
            SmartPtr<render::IVideoTexture> getVideoTexture() const;

            /**
             * @brief Sets the video texture used for rendering.
             * @param videoTexture Smart pointer to the video texture.
             */
            void setVideoTexture( SmartPtr<render::IVideoTexture> videoTexture );

            /**
             * @brief Gets whether the video auto-plays when loaded.
             * @return True if auto-play is enabled, false otherwise.
             */
            bool getAutoPlay() const;

            /**
             * @brief Sets whether the video auto-plays when loaded.
             * @param autoPlay True to enable auto-play, false to disable.
             */
            void setAutoPlay( bool autoPlay );

            /**
             * @brief Gets whether the video loops when it reaches the end.
             * @return True if looping is enabled, false otherwise.
             */
            bool getLoop() const;

            /**
             * @brief Sets whether the video loops when it reaches the end.
             * @param loop True to enable looping, false to disable.
             */
            void setLoop( bool loop );

            /**
             * @brief Gets whether the video automatically updates frames.
             * @return True if auto-update is enabled, false otherwise.
             */
            bool getAutoUpdate() const;

            /**
             * @brief Sets whether the video automatically updates frames.
             * @param autoUpdate True to enable auto-update, false to disable.
             */
            void setAutoUpdate( bool autoUpdate );

            /**
             * @copydoc Component::getProperties
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @copydoc Component::setProperties
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @copydoc Component::getChildObjects
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Creates the video object from the video path.
             */
            void createVideo();

            /**
             * @brief Destroys the video object and cleans up resources.
             */
            void destroyVideo();

            /** Smart pointer to the video object. */
            SmartPtr<render::IVideo> m_video;

            /** Path to the video file. */
            String m_videoPath;

            /** Whether the video should auto-play when loaded. */
            bool m_autoPlay = false;

            /** Whether the video should loop when it reaches the end. */
            bool m_loop = false;

            /** Whether the video should automatically update frames. */
            bool m_autoUpdate = true;

            /** Whether the video is currently playing. */
            bool m_isPlaying = false;
        };

    }  // namespace scene
}  // namespace workphone

#endif  // VideoPlayer_h__
