#ifndef _CAnimationController_H_
#define _CAnimationController_H_

#include <WPGraphicsOgreNext/WPGraphicsOgreNextPrerequisites.hpp>
#include <Workphone/Interface/Graphics/IAnimationController.hpp>
#include <Workphone/Interface/Script/IScriptReceiver.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Interface/System/IState.hpp>
#include <Workphone/Core/HashMap.hpp>

namespace workphone
{
    namespace render
    {
        class CAnimationController : public IAnimationController
        {
        public:
            static const hash_type SET_ANIMATION_ENABLED_HASH;
            static const hash_type SET_ANIMATION_ENABLED_TIME_HASH;
            static const hash_type IS_ANIMATION_ENABLED_HASH;
            static const hash_type SET_ANIMATION_LOOP_HASH;
            static const hash_type STOP_ALL_ANIMATIONS_HASH;

            CAnimationController();
            ~CAnimationController() override;

            void Initialize( Ogre::v1::Entity *entity );

            void update( const s32 &task, const time_interval &t, const time_interval &dt );

            bool setAnimationEnabled( const String &animationName, bool enabled ) override;
            bool setAnimationEnabled( const String &animationName, bool enabled,
                                      f32 timePosition ) override;

            bool _setAnimationEnabled( const String &animationName, bool enabled, f32 timePosition );

            bool isAnimationEnabled( const String &animationName ) override;
            void stopAllAnimations() override;

            bool hasAnimationEnded( const String &animationName ) override;

            bool hasAnimation( const String &animationName ) const override;

            void setAnimationLoop( const String &animationName, bool loop ) override;
            bool isAnimationLooping( const String &animationName ) override;

            void setAnimationReversed( const String &animationName, bool reversed ) override;
            bool isAnimationReversed( const String &animationName ) override;

            bool setTimePosition( const String &animationName, f32 timePosition ) override;
            f32 getTimePosition( const String &animationName ) override;

            f32 getAnimationLength( const String &animationName ) const override;

            void addListener( IAnimationControllerListener *listener );
            void removeListener( IAnimationControllerListener *listener );

            SmartPtr<IStateContext> &getStateContext();
            const SmartPtr<IStateContext> &getStateContext() const;
            void setStateContext( SmartPtr<IStateContext> subject );

        private:
            class ScriptReceiver : public IScriptReceiver
            {
            public:
                ScriptReceiver( IAnimationController *animCtrl );

                s32 callFunction( u32 hashId, const Parameters &params, Parameters &results );

            protected:
                IAnimationController *m_animCtrl;
            };

            class AnimationControllerStateListener : public IStateListener
            {
            public:
                AnimationControllerStateListener( CAnimationController *owner );
                ~AnimationControllerStateListener() override;

                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;
                bool handleStateChanged( SmartPtr<IState> &state ) override;

            protected:
                CAnimationController *m_owner;
            };

            class Animation : public ISharedObject
            {
            public:
                Animation( CAnimationController *controller );

                Ogre::AnimationState *getAnimationState() const;
                void setAnimationState( Ogre::AnimationState *animationState );

                f32 getLength() const;
                void setLength( f32 length );

                bool isLooping() const;
                void setLooping( bool looping );

                bool isEnabled() const;
                void setEnabled( bool enabled );

            protected:
                CAnimationController *m_controller;
                Ogre::AnimationState *m_animationState;
                f32 m_length;
                bool m_isLooping;
                bool m_isEnabled;
            };

            void _update( f64 dt );
            bool _setAnimationEnabled( const String &animationName, bool enabled );

            void addAnimation( Ogre::v1::AnimationState *pAnimState );
            SmartPtr<IAnimation> findAnimation( const String &animationName ) const;

            SmartPtr<IStateContext> m_stateContext;
            SmartPtr<IStateListener> m_stateListener;
            Ogre::v1::Entity *m_entity;

            using Animations = HashMap<hash_type, SmartPtr<IAnimation>>;
            Animations m_animations;

            Array<IAnimationControllerListener *> m_listeners;

            mutable SpinRWMutex Mutex;
        };

        using CAnimationControllerPtr = SmartPtr<CAnimationController>;
    }  // namespace render
}  // namespace workphone

#endif
