#ifndef Animator_h__
#define Animator_h__

#include <Workphone/Scene/Components/Component.hpp>
#include <Workphone/Scene/Components/Animation.hpp>
#include <Workphone/Animation/AnimationGraph.hpp>
#include <Workphone/Animation/IK/AnimationIKSystem.hpp>
#include <Workphone/Interface/System/IFSM.hpp>
#include <Workphone/Interface/System/IFSMListener.hpp>

/**
 * @file Animator.hpp
 * @brief Animator component used to manage and drive animations on an actor.
 */
namespace workphone
{
    namespace scene
    {

        /**
         * @brief Component responsible for managing animations and animation state.
         *
         * The Animator owns a collection of animations and an internal finite state
         * machine (FSM) that tracks whether the animations are playing, paused or
         * stopped. It provides lifecycle hooks that are invoked when animation
         * events occur and exposes methods to query and control the playback.
         */
        class WPCore_API Animator : public Component
        {
        public:
            /// Animation state: stopped.
            static const u32 STATE_STOPPED;
            /// Animation state: playing.
            static const u32 STATE_PLAYING;
            /// Animation state: paused.
            static const u32 STATE_PAUSED;

            /// Event dispatched when playback starts.
            static const u32 EVENT_PLAY;
            /// Event dispatched when playback is paused.
            static const u32 EVENT_PAUSE;
            /// Event dispatched when playback stops.
            static const u32 EVENT_STOP;
            /// Event dispatched on each animation update/tick.
            static const u32 EVENT_UPDATE;

            static const String animationTimeStr;
            static const String animationSpeedStr;
            static const String animationWeightStr;
            static const String animationScaleStr;
            static const String loopingStr;
            static const String selectedAnimationIndexStr;
            static const String selectedAnimationNameStr;
            static const String selectedAnimationLengthStr;
            static const String playbackStateStr;
            static const String runtimeClipCountStr;
            static const String editableClipCountStr;
            static const String autoPlayStr;
            static const String resetTimeOnPlayStr;
            static const String resetTimeOnStopStr;
            static const String clampTimeToClipStr;
            static const String applyOnPropertyChangeStr;
            static const String playButtonStr;
            static const String pauseButtonStr;
            static const String stopButtonStr;
            static const String rewindButtonStr;
            static const String clipsStr;
            static const String clipStr;
            static const String ikConstraintsStr;

            /**
             * @brief Construct a new Animator component.
             *
             * The constructor initializes internal state but does not start playback.
             */
            Animator();

            /**
             * @brief Destroy the Animator component.
             */
            ~Animator() override;

            /**
             * @copydoc Component::load
             *
             * Load component-specific data (animations, skeleton, properties) from
             * the provided shared object.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc Component::unload
             *
             * Unload and release resources held by this animator.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Advance the animator and its active animations.
             *
             * Called each frame (or tick) to update the FSM and drive animation
             * time progression. This will typically dispatch the @c EVENT_UPDATE
             * event on the internal FSM.
             */
            void update() override;

            /**
             * @brief Start or resume playback of animations.
             *
             * Transitions the internal FSM to @c STATE_PLAYING and triggers
             * @c onPlay / @c onResume callbacks as appropriate.
             */
            void play();

            /**
             * @brief Pause playback of animations.
             *
             * Transitions the internal FSM to @c STATE_PAUSED and triggers
             * @c onPause.
             */
            void pause();

            /**
             * @brief Stop playback and reset animation time where appropriate.
             *
             * Transitions the internal FSM to @c STATE_STOPPED and triggers
             * @c onStop.
             */
            void stop();

            /**
             * @brief Return true if animator is currently playing animations.
             */
            bool isPlaying() const;

            /**
             * @brief Return true if animator playback is currently paused.
             */
            bool isPaused() const;

            /**
             * @brief Return true if animator is currently stopped.
             */
            bool isStopped() const;

            /**
             * @brief Callback invoked when playback starts from stopped state.
             *
             * Implementers may override or bind to this to perform start-up
             * logic (e.g. resetting time, enabling animation-driven nodes).
             */
            void onPlay();

            /**
             * @brief Callback invoked when playback is paused.
             */
            void onPause();

            /**
             * @brief Callback invoked when playback is resumed from pause.
             */
            void onResume();

            /**
             * @brief Callback invoked when playback is stopped.
             */
            void onStop();

            /**
             * @brief Callback invoked on each update/tick while playing.
             */
            void onUpdate();

            /**
             * @brief Gets the current playback position in seconds.
             * @return The current animation time.
             */
            f32 getAnimationTime() const;

            /**
             * @brief Sets the playback position in seconds.
             * @param animationTime The target time to seek to.
             */
            void setAnimationTime( f32 animationTime );

            /**
             * @brief Gets the playback speed multiplier.
             * @return The current speed (1.0 is normal speed).
             */
            f32 getAnimationSpeed() const;

            /**
             * @brief Sets the playback speed multiplier.
             * @param animationSpeed The speed multiplier (e.g., 2.0 for double speed).
             */
            void setAnimationSpeed( f32 animationSpeed );

            /**
             * @brief Gets the influence weight of the animation.
             * @return The weight value, typically between 0.0 and 1.0.
             */
            f32 getAnimationWeight() const;

            /**
             * @brief Sets the influence weight of the animation.
             * @param animationWeight The desired weight.
             */
            void setAnimationWeight( f32 animationWeight );

            /**
             * @brief Gets the animation scale factor.
             * @return The current scale.
             */
            f32 getAnimationScale() const;

            /**
             * @brief Sets the animation scale factor.
             * @param animationScale The desired scale.
             */
            void setAnimationScale( f32 animationScale );

            /**
             * @brief Checks if the animation is configured to loop upon reaching the end.
             * @return True if looping is enabled, false otherwise.
             */
            bool isLooping() const;
            void setLooping( bool looping );

            bool getAutoPlay() const;
            void setAutoPlay( bool autoPlay );

            bool getResetTimeOnPlay() const;
            void setResetTimeOnPlay( bool resetTimeOnPlay );

            bool getResetTimeOnStop() const;
            void setResetTimeOnStop( bool resetTimeOnStop );

            bool getClampTimeToClip() const;
            void setClampTimeToClip( bool clampTimeToClip );

            bool getApplyOnPropertyChange() const;
            void setApplyOnPropertyChange( bool applyOnPropertyChange );

            u32 getPlaybackState() const;
            void setPlaybackState( u32 playbackState );

            u32 getSelectedAnimationIndex() const;
            void setSelectedAnimationIndex( u32 selectedAnimationIndex );

            String getSelectedAnimationName() const;
            f32 getSelectedAnimationLength() const;
            SmartPtr<IAnimation> getSelectedAnimation() const;
            SmartPtr<Animation> getSelectedAnimationClip() const;

            /**
             * @brief Add an animation to the animator's collection.
             * @param animation Animation to add. If null or already present this is a no-op.
             */
            void addAnimation( SmartPtr<IAnimation> animation );

            /**
             * @brief Add an animation descriptor to the animator's editable clip list.
             */
            void addAnimation( SmartPtr<Animation> animation );

            /**
             * @brief Remove an animation from the animator's collection.
             * @param animation Animation to remove.
             */
            void removeAnimation( SmartPtr<IAnimation> animation );

            /**
             * @brief Get a copy of the current list of animations.
             * @return Array of animations currently owned by the animator.
             */
            Array<SmartPtr<IAnimation>> getAnimations() const;

            /**
             * @brief Get editable scene animation descriptors.
             */
            Array<SmartPtr<Animation>> getAnimationClips() const;
            void setAnimationClips( const Array<SmartPtr<Animation>> &animationClips );

            /**
             * @brief Get the number of runtime or descriptor clips.
             */
            u32 getNumAnimationClips() const;

            void clearAnimations();

            /**
             * @brief Replace the animator's animations with the provided list.
             * @param animations New list of animations to use.
             */
            void setAnimations( const Array<SmartPtr<IAnimation>> &animations );

            /**
             * @brief Retrieve serialized properties for this component.
             * @return Component properties instance.
             * @see Component::getProperties
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Set this component's properties from a serialized object.
             * @param properties Properties to apply to this animator.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Get child objects that should be serialized with this component.
             * @return Array of child shared objects.
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @brief Get the skeleton associated with this animator.
             * @return Skeleton used by animations, or null if none set.
             */
            SmartPtr<ISkeleton> getSkeleton() const;

            /**
             * @brief Set the skeleton that animations will drive.
             * @param skeleton Skeleton to associate with this animator.
             */
            void setSkeleton( SmartPtr<ISkeleton> skeleton );

            /** Installs a tools-authored animation graph and resets it to its initial state. */
            bool setAnimationGraph( const animation::AnimationGraphDefinition &definition );

            /** Removes the graph and returns the component to selected-clip playback. */
            void clearAnimationGraph();

            bool hasAnimationGraph() const;
            animation::AnimationGraphInstance *getAnimationGraph();
            const animation::AnimationGraphInstance *getAnimationGraph() const;

            /** Gets the post-animation IK pass evaluated after clip/graph sampling. */
            animation::AnimationIKSystem &getIKSystem();
            const animation::AnimationIKSystem &getIKSystem() const;

            /** Results from the most recent post-animation IK pass, for tools/debugging. */
            const Array<animation::AnimationIKSolveResult> &getLastIKSolveResults() const;

            WP_CLASS_REGISTER_DECL;

        private:
            /**
             * @brief Finite state machine instance that manages animation playback
             * states and dispatches events.
             */
            SmartPtr<IFSM> m_fsm;

            /**
             * @brief Listener registered on the FSM to receive state change events.
             */
            SmartPtr<IFSMListener> m_fsmListener;

            /**
             * @brief Skeleton resource that animations will be applied to.
             */
            SmartPtr<ISkeleton> m_skeleton;

            /**
             * @brief Owned animations for this animator.
             *
             * The array contains smart pointers to animation resources that will
             * be advanced when playback is active.
             */
            Array<SmartPtr<IAnimation>> m_animations;
            Array<SmartPtr<Animation>> m_animationClips;

            animation::AnimationGraphInstance m_animationGraph;
            animation::AnimationIKSystem m_ikSystem;
            Array<animation::AnimationIKSolveResult> m_lastIKSolveResults;

            u32 m_state = STATE_STOPPED;
            u32 m_selectedAnimationIndex = 0;
            f32 m_animationTime = 0.0f;
            f32 m_animationSpeed = 1.0f;
            f32 m_animationWeight = 1.0f;
            f32 m_animationScale = 1.0f;
            bool m_looping = true;
            bool m_autoPlay = false;
            bool m_resetTimeOnPlay = false;
            bool m_resetTimeOnStop = true;
            bool m_clampTimeToClip = true;
            bool m_applyOnPropertyChange = true;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // Animator_h__
