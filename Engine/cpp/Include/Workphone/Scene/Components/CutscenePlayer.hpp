#ifndef __CutscenePlayer_h__
#define __CutscenePlayer_h__

#include <Workphone/Scene/Components/Component.hpp>
#include <Workphone/Scene/Cutscene.hpp>

namespace workphone
{
    namespace scene
    {
        /**
         * @brief Component that plays a Cutscene resource.
         *
         * The CutscenePlayer drives a Cutscene asset over time and applies the
         * evaluated values to the target actors in the current scene. Playback can
         * be controlled through play/pause/stop and can optionally loop.
         */
        class WPCore_API CutscenePlayer : public Component
        {
        public:
            static const String cutsceneStr;
            static const String playOnStartStr;
            static const String loopingStr;
            static const String speedStr;
            static const String currentTimeStr;
            static const String playingStr;
            static const String playButtonStr;
            static const String pauseButtonStr;
            static const String stopButtonStr;

            /** @brief Constructor. */
            CutscenePlayer();

            /** @brief Destructor. */
            ~CutscenePlayer() override;

            /** @copydoc Component::load */
            void load( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Component::unload */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** @copydoc Component::update */
            void update() override;

            /** @copydoc Component::getProperties */
            SmartPtr<Properties> getProperties() const override;

            /** @copydoc Component::setProperties */
            void setProperties( SmartPtr<Properties> properties ) override;

            /** @brief Sets the cutscene asset to play. */
            void setCutscene( SmartPtr<Cutscene> cutscene );

            /** @brief Gets the cutscene asset. */
            SmartPtr<Cutscene> getCutscene() const;

            /** @brief Starts or resumes playback. */
            void play();

            /** @brief Pauses playback without resetting time. */
            void pause();

            /** @brief Stops playback and resets time to zero. */
            void stop();

            /** @brief Returns true if the cutscene is currently playing. */
            bool isPlaying() const;

            /** @brief Sets the playback speed multiplier. */
            void setSpeed( f32 speed );

            /** @brief Gets the playback speed multiplier. */
            f32 getSpeed() const;

            /** @brief Sets whether the cutscene loops. */
            void setLooping( bool looping );

            /** @brief Returns true if the cutscene loops. */
            bool isLooping() const;

            /** @brief Sets whether playback starts automatically when loaded. */
            void setPlayOnStart( bool playOnStart );

            /** @brief Returns true if playback starts automatically. */
            bool getPlayOnStart() const;

            /** @brief Sets the current playback time in seconds. */
            void setCurrentTime( f32 time );

            /** @brief Gets the current playback time in seconds. */
            f32 getCurrentTime() const;

            WP_CLASS_REGISTER_DECL;

        public:
            /** @brief Applies one evaluated keyframe to a target actor. */
            void applyValue( SmartPtr<IGameActor> actor, Cutscene::TrackType type,
                             const Cutscene::Keyframe &value );

            /** @brief Advances time and applies the cutscene this frame. */
            void tick( f32 deltaTime );

            SmartPtr<Cutscene> m_cutscene;
            bool m_playing = false;
            bool m_looping = false;
            bool m_playOnStart = false;
            f32 m_speed = 1.0f;
            f32 m_currentTime = 0.0f;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // __CutscenePlayer_h__
