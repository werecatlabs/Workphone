#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CAnimationControllerOgre.hpp>
#include <Workphone/Workphone.hpp>
#include <WPGraphicsOgre/WPGraphicsOgre.hpp>
#include <OgreAnimationState.h>
#include <OgreEntity.h>

namespace workphone
{
    namespace render
    {
        const hash_type CAnimationController::SET_ANIMATION_ENABLED_HASH =
            StringUtil::getHash( "setAnimationEnabled" );
        const hash_type CAnimationController::SET_ANIMATION_ENABLED_TIME_HASH =
            StringUtil::getHash( "setAnimationEnabledTime" );
        const hash_type CAnimationController::IS_ANIMATION_ENABLED_HASH =
            StringUtil::getHash( "isAnimationEnabled" );
        const hash_type CAnimationController::SET_ANIMATION_LOOP_HASH =
            StringUtil::getHash( "setAnimationLoop" );

        const hash_type CAnimationController::STOP_ALL_ANIMATIONS_HASH =
            StringUtil::getHash( "stopAllAnimations" );

        const hash_type CAnimationController::FRAME_EVENT_RENDER_QUEUED =
            StringUtil::getHash( "frameEventRenderQueued" );

        CAnimationController::CAnimationController()
        {
            // setReceiver(SmartPtr<IScriptReceiver>(new ScriptReceiver(this)));

            auto engine = core::IApplicationManager::instance();
            auto graphicsSystem = engine->getGraphicsSystem();

            // m_stateContext = platformMgr->createStateObject();

            m_stateListener = SmartPtr<IStateListener>( new AnimationControllerStateListener( this ) );
            m_stateContext->addStateListener( m_stateListener );
        }

        CAnimationController::~CAnimationController()
        {
            auto engine = core::IApplicationManager::instance();
            auto graphicsSystem = engine->getGraphicsSystem();
            //graphicsSystem->removeFrameListener( m_frameListener );
        }

        void CAnimationController::Initialize( Ogre::Entity *entity )
        {
            m_entity = entity;
        }

        void CAnimationController::update()
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto timer = applicationManager->getTimer();

            auto task = Thread::getCurrentTask();
            auto t = timer->getTime();
            auto dt = timer->getDeltaTime();

            auto currentTaskId = Thread::getCurrentTask();
            if( currentTaskId == TaskId::Render )
            {
                _update( dt );
            }
        }

        bool CAnimationController::setAnimationEnabled( const String &animationName, bool enabled )
        {
            auto currentTaskId = Thread::getCurrentTask();

            if( currentTaskId != TaskId::Render )
            {
                SmartPtr<StateMessageAnimationEnable> message( new StateMessageAnimationEnable );
                message->setType( SET_ANIMATION_ENABLED_HASH );
                message->setName( animationName );
                message->setEnabled( enabled );
                m_stateContext->addMessage( TaskId::Render, message );
            }
            else
            {
                return _setAnimationEnabled( animationName, enabled );
            }

            return false;
        }

        bool CAnimationController::setAnimationEnabled( const String &animationName, bool enabled,
                                                        f32 timePosition )
        {
            auto currentTaskId = Thread::getCurrentTask();

            if( currentTaskId != TaskId::Render )
            {
                SmartPtr<StateMessageAnimationEnable> message( new StateMessageAnimationEnable );
                message->setType( SET_ANIMATION_ENABLED_TIME_HASH );
                message->setName( animationName );
                message->setEnabled( enabled );
                message->setTime( timePosition );
                m_stateContext->addMessage( TaskId::Render, message );
            }
            else
            {
                return _setAnimationEnabled( animationName, enabled, timePosition );
            }

            return false;
        }

        bool CAnimationController::isAnimationEnabled( const String &animationName )
        {
            auto animation = findAnimation( animationName );
            if( animation )
            {
                //			return animation->isEnabled();
            }

            return false;
        }

        void CAnimationController::stopAllAnimations()
        {
            auto currentTaskId = Thread::getCurrentTask();

            if( currentTaskId != TaskId::Render )
            {
                SmartPtr<StateMessageStop> message( new StateMessageStop );
                message->setType( STOP_ALL_ANIMATIONS_HASH );
                m_stateContext->addMessage( TaskId::Render, message );
            }
            else
            {
                Ogre::AnimationStateSet *animationStateSet = m_entity->getAllAnimationStates();
                Ogre::AnimationStateIterator it = animationStateSet->getAnimationStateIterator();
                Ogre::AnimationState *pAnimState = nullptr;
                while( it.hasMoreElements() )
                {
                    pAnimState = it.getNext();
                    pAnimState->setEnabled( false );
                }
            }
        }

        bool CAnimationController::hasAnimationEnded( const String &animationName )
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );

            Ogre::AnimationStateSet *animationStateSet = m_entity->getAllAnimationStates();
            if( !animationStateSet->hasAnimationState( animationName.c_str() ) )
            {
                String msg =
                    String(
                        "CAnimationController::hasAnimationEnded: Animation not found. animation: " ) +
                    animationName;
                WP_LOG_INFO( msg.c_str() );
                return false;
            }

            Ogre::AnimationState *pAnimState =
                animationStateSet->getAnimationState( animationName.c_str() );
            if( pAnimState && pAnimState->hasEnded() )
            {
                return true;
            }

            return false;
        }

        bool CAnimationController::hasAnimation( const String &animationName ) const
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );

            Ogre::AnimationStateSet *animationStateSet = m_entity->getAllAnimationStates();
            if( !animationStateSet )
            {
                return false;
            }

            if( !animationStateSet->hasAnimationState( animationName.c_str() ) )
            {
                return false;
            }

            return true;
        }

        void CAnimationController::setAnimationLoop( const String &animationName, bool loop )
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );

            Ogre::AnimationStateSet *animationStateSet = m_entity->getAllAnimationStates();
            if( !animationStateSet->hasAnimationState( animationName.c_str() ) )
            {
                String msg =
                    String(
                        "CAnimationController::setAnimationLoop: Animation not found. animation: " ) +
                    animationName;
                WP_LOG_INFO( msg.c_str() );
                return;
            }

            Ogre::AnimationState *pAnimState =
                animationStateSet->getAnimationState( animationName.c_str() );
            if( pAnimState )
            {
                pAnimState->setLoop( loop );
            }
        }

        bool CAnimationController::isAnimationLooping( const String &animationName )
        {
            auto animation = findAnimation( animationName );
            if( animation )
            {
                // animation->isLooping();
            }

            return false;
        }

        void CAnimationController::setAnimationReversed( const String &animationName, bool reversed )
        {
        }

        bool CAnimationController::isAnimationReversed( const String &animationName )
        {
            return false;
        }

        bool CAnimationController::setTimePosition( const String &animationName, f32 timePosition )
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );

            Ogre::AnimationStateSet *animationStateSet = m_entity->getAllAnimationStates();
            if( !animationStateSet->hasAnimationState( animationName.c_str() ) )
            {
                String msg =
                    String( "CAnimationController::setTimePosition: Animation not found. animation: " ) +
                    animationName;
                WP_LOG_INFO( msg.c_str() );
                return false;
            }

            Ogre::AnimationState *pAnimState =
                animationStateSet->getAnimationState( animationName.c_str() );
            if( pAnimState )
            {
                pAnimState->setTimePosition( timePosition );
                return true;
            }

            return false;
        }

        f32 CAnimationController::getTimePosition( const String &animationName )
        {
            ScopedLock lock( core::IApplicationManager::instance()->getGraphicsSystem() );

            Ogre::AnimationStateSet *animationStateSet = m_entity->getAllAnimationStates();
            if( !animationStateSet->hasAnimationState( animationName.c_str() ) )
            {
                String msg =
                    String( "CAnimationController::setTimePosition: Animation not found. animation: " ) +
                    animationName;
                WP_LOG_INFO( msg.c_str() );
                return false;
            }

            Ogre::AnimationState *pAnimState =
                animationStateSet->getAnimationState( animationName.c_str() );
            if( pAnimState )
            {
                return pAnimState->getTimePosition();
            }

            return 0.0f;
        }

        f32 CAnimationController::getAnimationLength( const String &animationName ) const
        {
            SmartPtr<IAnimation> animation = findAnimation( animationName );
            if( animation )
                animation->getLength();

            return 0.0f;
        }

        SmartPtr<IStateContext> &CAnimationController::getStateContext()
        {
            return m_stateContext;
        }

        const SmartPtr<IStateContext> &CAnimationController::getStateContext() const
        {
            return m_stateContext;
        }

        void CAnimationController::setStateContext( SmartPtr<IStateContext> subject )
        {
            m_stateContext = subject;
        }

        void CAnimationController::addListener( IAnimationControllerListener *listener )
        {
            m_listeners.push_back( listener );
        }

        void CAnimationController::removeListener( IAnimationControllerListener *listener )
        {
            // m_listeners.erase_element(listener);
        }

        void CAnimationController::_update( f64 dt )
        {
            // Ogre::AnimationStateSet* animationStateSet = m_entity->getAllAnimationStates();
            // if(!animationStateSet)
            //{
            //	return;
            // }

            // Ogre::AnimationStateIterator it = animationStateSet->getAnimationStateIterator();
            // Ogre::AnimationState* pAnimState = NULL;
            // while(it.hasMoreElements())
            //{
            //	pAnimState = it.getNext();

            //	if(pAnimState->getEnabled() && pAnimState->hasEnded() && !pAnimState->getLoop())
            //	{
            //		/*StateManagerPtr& stateMgr = IApplicationManager::instance()->getStateManager();

            //		static const u32 STATE_ON_ANIMATION_END =
            // StringUtil::getHashMakeLower("OnAnimationEnd");

            //		String animationName = pAnimState->getAnimationName().c_str();
            //		u32 animationHash = StringUtil::getHashMakeLower("animationName");

            //		StateMessageDefaultPtr stateMessage(new StateMessageDefault, true);
            //		stateMessage->setSubjectId(m_stateContext->getSubjectId());
            //		stateMessage->setType(STATE_ON_ANIMATION_END);
            //		stateMessage->setProperty(animationHash, animationName);
            //		stateMgr->notifyStateChanged(stateMessage);
            //		*/

            //		pAnimState->setEnabled(false);
            //	}

            //	if(pAnimState->getEnabled())
            //	{
            //		pAnimState->addTime( dt );
            //	}
            //}

            // Array<SmartPtr<IAnimation>> animations;
            // animations.reserve(m_animations.size());

            // Animations::iterator animationsIt = m_animations.begin();
            // for(; animationsIt != m_animations.end(); ++animationsIt)
            //{
            //	animations.push_back(animationsIt->second);
            // }

            // for(u32 i=0; i<animations.size(); ++i)
            //{
            //	SmartPtr<IAnimation> animation = animations[i];
            //	Ogre::AnimationState* animationState = animation->getAnimationState();

            //	animation->setEnabled(animationState->getEnabled());
            //	animation->setLength(animationState->getLength());
            //	animation->setLooping(animationState->getLoop());
            //}
        }

        bool CAnimationController::_setAnimationEnabled( const String &animationName, bool enabled )
        {
            if( !m_entity->hasSkeleton() && !m_entity->hasVertexAnimation() )
                return false;

            Ogre::AnimationStateSet *animationStateSet = m_entity->getAllAnimationStates();
            if( !animationStateSet->hasAnimationState( animationName.c_str() ) )
            {
                String msg =
                    String(
                        "CAnimationController::setAnimationEnabled: Animation not found. animation: " ) +
                    animationName;
                WP_LOG_INFO( msg.c_str() );
                return false;
            }

            Ogre::AnimationState *pAnimState =
                animationStateSet->getAnimationState( animationName.c_str() );
            if( pAnimState )
            {
                pAnimState->setEnabled( enabled );

                addAnimation( pAnimState );

                return true;
            }

            return false;
        }

        bool CAnimationController::_setAnimationEnabled( const String &animationName, bool enabled,
                                                         f32 timePosition )
        {
            if( !m_entity->hasSkeleton() && !m_entity->hasVertexAnimation() )
                return false;

            Ogre::AnimationStateSet *animationStateSet = m_entity->getAllAnimationStates();
            if( !animationStateSet->hasAnimationState( animationName.c_str() ) )
            {
                String msg =
                    String(
                        "CAnimationController::setAnimationEnabled: Animation not found. animation: " ) +
                    animationName;
                WP_LOG_INFO( msg.c_str() );
                return false;
            }

            Ogre::AnimationState *pAnimState =
                animationStateSet->getAnimationState( animationName.c_str() );
            if( pAnimState )
            {
                pAnimState->setEnabled( enabled );
                pAnimState->setTimePosition( timePosition );

                addAnimation( pAnimState );

                return true;
            }

            return false;
        }

        void CAnimationController::addAnimation( Ogre::AnimationState *pAnimState )
        {
            SpinRWMutex::ScopedLock lock( Mutex );

            // SmartPtr<IAnimation> animation(new Animation(this));
            // animation->setAnimationState(pAnimState);
            // animation->setEnabled(pAnimState->getEnabled());
            // animation->setLength(pAnimState->getLength());
            // animation->setLooping(pAnimState->getLoop());

            // hash32 hash = StringUtil::getHash(pAnimState->getAnimationName().c_str());
            // m_animations[hash] = animation;

            // if(!m_frameListener)
            //{
            //	auto engine = core::IApplicationManager::instance();
            //	SmartPtr<IGraphicsSystem> graphicsSystem = engine->getGraphicsSystem();

            //	m_frameListener = SmartPtr<IFrameListener>(new AnimationControllerFrameListener(this));
            //	graphicsSystem->addFrameListener(m_frameListener);
            //}
        }

        SmartPtr<IAnimation> CAnimationController::findAnimation( const String &animationName ) const
        {
            //SpinRWMutex::ScopedLock lock( Mutex );

            auto hash = StringUtil::getHash( animationName );
            auto it = m_animations.find( hash );
            if( it != m_animations.end() )
            {
                SmartPtr<IAnimation> animation = it->second;
                return animation;
            }

            return nullptr;
        }

        CAnimationController::ScriptReceiver::ScriptReceiver( IAnimationController *animCtrl ) :
            m_animCtrl( animCtrl )
        {
        }

        s32 CAnimationController::ScriptReceiver::callFunction( hash_type hashId,
                                                                const Parameters &params,
                                                                Parameters &results )
        {
            if( hashId == SET_ANIMATION_ENABLED_HASH )
            {
                m_animCtrl->setAnimationEnabled( static_cast<char *>( params[0].data.pData ),
                                                 params[1].data.bData );
            }
            else if( hashId == IS_ANIMATION_ENABLED_HASH )
            {
                bool value =
                    m_animCtrl->isAnimationEnabled( static_cast<char *>( params[0].data.pData ) );
                // results.(1);
                results[0].setBool( value );
            }
            else if( hashId == SET_ANIMATION_LOOP_HASH )
            {
                m_animCtrl->setAnimationLoop( static_cast<char *>( params[0].data.pData ),
                                              params[1].data.bData );
            }
            // else if( hashId == StringUtil::UPDATE_HASH )
            //{
            //	SmartPtr<ITimer>& timer = IApplicationManager::instance()->getTimer();
            //	f64 t = (f64)timer->getTimeMilliseconds() / 1000.0;
            //	f64 dt = (f64)timer->getTimeIntervalMilliseconds() / 1000.0;
            //	m_animCtrl->update(0, t, dt);
            // }
            else if( hashId == StringUtil::getHash( "stopAllAnimations" ) )
            {
                m_animCtrl->stopAllAnimations();
            }

            return 0;
        }

        CAnimationController::AnimationControllerStateListener::AnimationControllerStateListener(
            CAnimationController *owner ) :
            m_owner( owner )
        {
        }

        CAnimationController::AnimationControllerStateListener::~AnimationControllerStateListener()
        {
        }

        bool CAnimationController::AnimationControllerStateListener::handleStateMessage(
            const SmartPtr<IStateMessage> &message )
        {
            // if(message->isExactly(StateMessageStop::TYPE_INFO))
            //{
            //	StateMessageStopPtr messageStop = message;
            //	u32 type = messageStop->getType();

            //	if(type == CAnimationController::STOP_ALL_ANIMATIONS_HASH)
            //	{
            //		m_owner->stopAllAnimations();
            //	}
            //}
            // else if(message->isExactly(StateMessageAnimationEnable::TYPE_INFO))
            //{
            //	StateMessageAnimationEnablePtr animationMessage = message;
            //	u32 type = animationMessage->getType();

            //	if(type == CAnimationController::SET_ANIMATION_ENABLED_HASH)
            //	{
            //		m_owner->setAnimationEnabled(animationMessage->getName(),
            // animationMessage->getEnabled());
            //	}
            //	else if(type == CAnimationController::SET_ANIMATION_ENABLED_TIME_HASH)
            //	{
            //		m_owner->setAnimationEnabled(animationMessage->getName(),
            // animationMessage->getEnabled(), animationMessage->getTime());
            //	}
            //}

            return false;
        }

        bool CAnimationController::AnimationControllerStateListener::handleStateChanged(
            SmartPtr<IState> &state )
        {
            return false;
        }

        CAnimationController::Animation::Animation( CAnimationController *controller ) :
            m_controller( controller )
        {
        }

        Ogre::AnimationState *CAnimationController::Animation::getAnimationState() const
        {
            return m_animationState;
        }

        void CAnimationController::Animation::setAnimationState( Ogre::AnimationState *animationState )
        {
            m_animationState = animationState;
        }

        f32 CAnimationController::Animation::getLength() const
        {
            return m_length;
        }

        void CAnimationController::Animation::setLength( f32 length )
        {
            m_length = length;
        }

        bool CAnimationController::Animation::isLooping() const
        {
            return m_isLooping;
        }

        void CAnimationController::Animation::setLooping( bool looping )
        {
            m_isLooping = looping;
        }

        bool CAnimationController::Animation::isEnabled() const
        {
            return m_isEnabled;
        }

        void CAnimationController::Animation::setEnabled( bool enabled )
        {
            m_isEnabled = enabled;
        }
    }  // namespace render
}  // namespace workphone
