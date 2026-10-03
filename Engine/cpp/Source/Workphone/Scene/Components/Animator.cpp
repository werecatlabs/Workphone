#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/Animator.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/Animation/IAnimation.hpp>
#include <Workphone/Interface/Mesh/ISkeleton.hpp>
#include <Workphone/Interface/System/IFSM.hpp>
#include <Workphone/Interface/System/IFSMManager.hpp>
#include <Workphone/Interface/System/IFSMListener.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <algorithm>
#include <cmath>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, Animator, Component );

    // Animation states
    const u32 Animator::STATE_STOPPED = 0;
    const u32 Animator::STATE_PLAYING = 1;
    const u32 Animator::STATE_PAUSED = 2;

    // Animation events
    const u32 Animator::EVENT_PLAY = 0;
    const u32 Animator::EVENT_PAUSE = 1;
    const u32 Animator::EVENT_STOP = 2;
    const u32 Animator::EVENT_UPDATE = 3;

    const String Animator::animationTimeStr = String( "animationTime" );
    const String Animator::animationSpeedStr = String( "animationSpeed" );
    const String Animator::animationWeightStr = String( "animationWeight" );
    const String Animator::animationScaleStr = String( "animationScale" );
    const String Animator::loopingStr = String( "looping" );
    const String Animator::selectedAnimationIndexStr = String( "selectedAnimationIndex" );
    const String Animator::selectedAnimationNameStr = String( "selectedAnimationName" );
    const String Animator::selectedAnimationLengthStr = String( "selectedAnimationLength" );
    const String Animator::playbackStateStr = String( "playbackState" );
    const String Animator::runtimeClipCountStr = String( "runtimeClipCount" );
    const String Animator::editableClipCountStr = String( "editableClipCount" );
    const String Animator::autoPlayStr = String( "autoPlay" );
    const String Animator::resetTimeOnPlayStr = String( "resetTimeOnPlay" );
    const String Animator::resetTimeOnStopStr = String( "resetTimeOnStop" );
    const String Animator::clampTimeToClipStr = String( "clampTimeToClip" );
    const String Animator::applyOnPropertyChangeStr = String( "applyOnPropertyChange" );
    const String Animator::playButtonStr = String( "Play" );
    const String Animator::pauseButtonStr = String( "Pause" );
    const String Animator::stopButtonStr = String( "Stop" );
    const String Animator::rewindButtonStr = String( "Rewind" );
    const String Animator::clipsStr = String( "clips" );
    const String Animator::clipStr = String( "clip" );
    const String Animator::ikConstraintsStr = String( "ikConstraints" );

    namespace
    {
        Array<String> getPlaybackStateNames()
        {
            return { String( "Stopped" ), String( "Playing" ), String( "Paused" ) };
        }

        u32 normalisePlaybackState( u32 state )
        {
            switch( state )
            {
            case Animator::STATE_PLAYING:
            case Animator::STATE_PAUSED:
            case Animator::STATE_STOPPED:
                return state;
            default:
                return Animator::STATE_STOPPED;
            }
        }
    }  // namespace

    class AnimatorFSMListener : public IFSMListener
    {
    public:
        AnimatorFSMListener( Animator *animator ) : m_animator( animator )
        {
        }

        FSMReturnType handleEvent( u32 state, FSMEvent eventType ) override
        {
            //if( !m_animator )
            //    return FSMReturnType::Error;

            //switch( state )
            //{
            //case STATE_STOPPED:
            //    return handleStoppedState( eventType );
            //case STATE_PLAYING:
            //    return handlePlayingState( eventType );
            //case STATE_PAUSED:
            //    return handlePausedState( eventType );
            //default:
            //    return FSMReturnType::Error;

            return {};
        }

    private:
        FSMReturnType handleStoppedState( FSMEvent eventType )
        {
            //switch( eventType )
            //{
            //case EVENT_PLAY:
            //    m_animator->onPlay();
            //    return FSMReturnType::Success;
            //case EVENT_UPDATE:
            //    m_animator->onUpdate();
            //    return FSMReturnType::Success;
            //default:
            //    return FSMReturnType::Error;
            //}

            return {};
        }

        FSMReturnType handlePlayingState( FSMEvent eventType )
        {
            //switch( eventType )
            //{
            //case EVENT_PAUSE:
            //    m_animator->onPause();
            //    return FSMReturnType::Success;
            //case EVENT_STOP:
            //    m_animator->onStop();
            //    return FSMReturnType::Success;
            //case EVENT_UPDATE:
            //    m_animator->onUpdate();
            //    return FSMReturnType::Success;
            //default:
            //    return FSMReturnType::Error;
            //}

            return {};
        }

        FSMReturnType handlePausedState( FSMEvent eventType )
        {
            //switch( eventType )
            //{
            //case EVENT_PLAY:
            //    m_animator->onResume();
            //    return FSMReturnType::Success;
            //case EVENT_STOP:
            //    m_animator->onStop();
            //    return FSMReturnType::Success;
            //case EVENT_UPDATE:
            //    m_animator->onUpdate();
            //    return FSMReturnType::Success;
            //default:
            //    return FSMReturnType::Error;
            //}

            return {};
        }

        WeakPtr<Animator> m_animator;
    };

    Animator::~Animator() = default;

    Animator::Animator() = default;

    void Animator::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        Component::load( data );

        if( auto properties = workphone::dynamic_pointer_cast<Properties>( data ) )
        {
            setProperties( properties );
        }

        if( m_autoPlay )
        {
            play();
        }

        setLoadingState( LoadingState::Loaded );
    }

    void Animator::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );

        // Cleanup FSM
        if( m_fsm )
        {
            //m_fsm->setListener( nullptr );
            m_fsmListener = nullptr;

            //auto fsmManager = IFSMManager::getInstance();
            //if( fsmManager )
            //{
            //    fsmManager->destroyFSM( m_fsm );
            //    m_fsm = nullptr;
            //}
        }

        Component::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    void Animator::update()
    {
        if( !isEnabled() || m_state != STATE_PLAYING )
            return;

        auto applicationManager = core::IApplicationManager::instance();
        SmartPtr<ITimer> timer;
        if( applicationManager )
        {
            timer = applicationManager->getTimer();
        }
        const auto deltaTime = timer ? static_cast<f32>( timer->getDeltaTime() ) : 0.0f;

        if( m_animationGraph.isValid() )
        {
            m_animationGraph.update( std::max( 0.0f, deltaTime ) * std::max( 0.0f, m_animationSpeed ) );
            onUpdate();
            return;
        }

        const auto length = getSelectedAnimationLength();
        const auto selectedClip = getSelectedAnimationClip();
        const auto effectiveSpeed = std::max( 0.0f, m_animationSpeed ) *
                                    ( selectedClip ? std::max( 0.0f, selectedClip->getSpeed() ) : 1.0f );
        const auto looping = selectedClip ? selectedClip->isLooping() : m_looping;

        m_animationTime += std::max( 0.0f, deltaTime ) * effectiveSpeed;

        if( length > 0.0f && m_animationTime >= length )
        {
            if( looping )
            {
                m_animationTime = std::fmod( m_animationTime, length );
            }
            else
            {
                m_animationTime = length;
                m_state = STATE_STOPPED;
                onStop();
            }
        }

        onUpdate();
    }

    void Animator::play()
    {
        if( !isEnabled() )
            return;

        const auto wasPaused = m_state == STATE_PAUSED;
        if( !wasPaused && m_resetTimeOnPlay )
        {
            if( m_animationGraph.isValid() )
            {
                m_animationGraph.reset();
            }
            else
            {
                setAnimationTime( 0.0f );
            }
        }

        m_state = STATE_PLAYING;

        if( wasPaused )
        {
            onResume();
        }
        else
        {
            onPlay();
        }
    }

    void Animator::pause()
    {
        if( !isEnabled() || m_state != STATE_PLAYING )
            return;

        m_state = STATE_PAUSED;
        onPause();
    }

    void Animator::stop()
    {
        if( !isEnabled() )
            return;

        m_state = STATE_STOPPED;
        if( m_resetTimeOnStop )
        {
            m_animationTime = 0.0f;
            if( m_animationGraph.isValid() )
            {
                m_animationGraph.reset();
            }
        }

        onStop();
        if( m_applyOnPropertyChange )
        {
            onUpdate();
        }
    }

    bool Animator::isPlaying() const
    {
        return m_state == STATE_PLAYING;
    }

    bool Animator::isPaused() const
    {
        return m_state == STATE_PAUSED;
    }

    bool Animator::isStopped() const
    {
        return m_state == STATE_STOPPED;
    }

    void Animator::onPlay()
    {
        // Implement animation start logic
    }

    void Animator::onPause()
    {
        // Implement animation pause logic
    }

    void Animator::onResume()
    {
        // Implement animation resume logic
    }

    void Animator::onStop()
    {
        // Implement animation stop logic
    }

    void Animator::onUpdate()
    {
        if( m_animationGraph.isValid() )
        {
            m_animationGraph.apply( m_skeleton.get(), m_animationScale, m_animationWeight );
        }
        else if( auto animation = getSelectedAnimation() )
        {
            if( auto skeleton = m_skeleton )
            {
                animation->apply( skeleton.get(), m_animationTime, m_animationWeight, m_animationScale );
            }
            else
            {
                animation->apply( m_animationTime, m_animationWeight, m_animationScale );
            }
        }

        if( auto skeleton = m_skeleton )
        {
            m_lastIKSolveResults = m_ikSystem.solve( skeleton.get() );
        }
        else
        {
            m_lastIKSolveResults.clear();
        }
    }

    f32 Animator::getAnimationTime() const
    {
        return m_animationTime;
    }

    void Animator::setAnimationTime( f32 animationTime )
    {
        const auto length = getSelectedAnimationLength();
        if( m_clampTimeToClip && length > 0.0f )
        {
            m_animationTime = std::clamp( animationTime, 0.0f, length );
        }
        else
        {
            m_animationTime = std::max( 0.0f, animationTime );
        }

        onUpdate();
    }

    f32 Animator::getAnimationSpeed() const
    {
        return m_animationSpeed;
    }

    void Animator::setAnimationSpeed( f32 animationSpeed )
    {
        m_animationSpeed = std::max( 0.0f, animationSpeed );
    }

    f32 Animator::getAnimationWeight() const
    {
        return m_animationWeight;
    }

    void Animator::setAnimationWeight( f32 animationWeight )
    {
        m_animationWeight = std::clamp( animationWeight, 0.0f, 1.0f );
        if( m_applyOnPropertyChange )
        {
            onUpdate();
        }
    }

    f32 Animator::getAnimationScale() const
    {
        return m_animationScale;
    }

    void Animator::setAnimationScale( f32 animationScale )
    {
        m_animationScale = std::max( 0.0f, animationScale );
        if( m_applyOnPropertyChange )
        {
            onUpdate();
        }
    }

    bool Animator::isLooping() const
    {
        return m_looping;
    }

    void Animator::setLooping( bool looping )
    {
        m_looping = looping;
    }

    bool Animator::getAutoPlay() const
    {
        return m_autoPlay;
    }

    void Animator::setAutoPlay( bool autoPlay )
    {
        m_autoPlay = autoPlay;
    }

    bool Animator::getResetTimeOnPlay() const
    {
        return m_resetTimeOnPlay;
    }

    void Animator::setResetTimeOnPlay( bool resetTimeOnPlay )
    {
        m_resetTimeOnPlay = resetTimeOnPlay;
    }

    bool Animator::getResetTimeOnStop() const
    {
        return m_resetTimeOnStop;
    }

    void Animator::setResetTimeOnStop( bool resetTimeOnStop )
    {
        m_resetTimeOnStop = resetTimeOnStop;
    }

    bool Animator::getClampTimeToClip() const
    {
        return m_clampTimeToClip;
    }

    void Animator::setClampTimeToClip( bool clampTimeToClip )
    {
        m_clampTimeToClip = clampTimeToClip;
        if( m_clampTimeToClip )
        {
            setAnimationTime( m_animationTime );
        }
    }

    bool Animator::getApplyOnPropertyChange() const
    {
        return m_applyOnPropertyChange;
    }

    void Animator::setApplyOnPropertyChange( bool applyOnPropertyChange )
    {
        m_applyOnPropertyChange = applyOnPropertyChange;
    }

    u32 Animator::getPlaybackState() const
    {
        return m_state;
    }

    void Animator::setPlaybackState( u32 playbackState )
    {
        const auto state = normalisePlaybackState( playbackState );
        switch( state )
        {
        case STATE_PLAYING:
            play();
            break;
        case STATE_PAUSED:
            if( m_state == STATE_PLAYING )
            {
                pause();
            }
            else
            {
                m_state = STATE_PAUSED;
            }
            break;
        case STATE_STOPPED:
        default:
            stop();
            break;
        }
    }

    u32 Animator::getSelectedAnimationIndex() const
    {
        return m_selectedAnimationIndex;
    }

    void Animator::setSelectedAnimationIndex( u32 selectedAnimationIndex )
    {
        const auto clipCount = std::max( m_animations.size(), m_animationClips.size() );
        m_selectedAnimationIndex =
            clipCount == 0 ? 0
                           : std::min<u32>( selectedAnimationIndex, static_cast<u32>( clipCount - 1 ) );
        setAnimationTime( m_animationTime );
    }

    String Animator::getSelectedAnimationName() const
    {
        if( auto animation = getSelectedAnimation() )
        {
            return animation->getName();
        }

        if( auto clip = getSelectedAnimationClip() )
        {
            return clip->getName();
        }

        return StringUtil::EmptyString;
    }

    f32 Animator::getSelectedAnimationLength() const
    {
        if( !m_animations.empty() && m_selectedAnimationIndex < m_animations.size() )
        {
            auto animation = m_animations[m_selectedAnimationIndex];
            return animation ? animation->getLength() : 0.0f;
        }

        if( !m_animationClips.empty() && m_selectedAnimationIndex < m_animationClips.size() )
        {
            auto animation = m_animationClips[m_selectedAnimationIndex];
            return animation ? animation->getLength() : 0.0f;
        }

        return 0.0f;
    }

    SmartPtr<IAnimation> Animator::getSelectedAnimation() const
    {
        if( !m_animations.empty() && m_selectedAnimationIndex < m_animations.size() )
        {
            return m_animations[m_selectedAnimationIndex];
        }

        return nullptr;
    }

    SmartPtr<Animation> Animator::getSelectedAnimationClip() const
    {
        if( !m_animationClips.empty() && m_selectedAnimationIndex < m_animationClips.size() )
        {
            return m_animationClips[m_selectedAnimationIndex];
        }

        return nullptr;
    }

    void Animator::addAnimation( SmartPtr<IAnimation> animation )
    {
        if( animation &&
            std::find( m_animations.begin(), m_animations.end(), animation ) == m_animations.end() )
        {
            m_animations.push_back( animation );
        }
    }

    void Animator::addAnimation( SmartPtr<Animation> animation )
    {
        if( animation && std::find( m_animationClips.begin(), m_animationClips.end(), animation ) ==
                             m_animationClips.end() )
        {
            m_animationClips.push_back( animation );
        }
    }

    Array<SmartPtr<Animation>> Animator::getAnimationClips() const
    {
        return m_animationClips;
    }

    void Animator::setAnimationClips( const Array<SmartPtr<Animation>> &animationClips )
    {
        m_animationClips = animationClips;
        setSelectedAnimationIndex( m_selectedAnimationIndex );
    }

    u32 Animator::getNumAnimationClips() const
    {
        return static_cast<u32>( std::max( m_animations.size(), m_animationClips.size() ) );
    }

    void Animator::clearAnimations()
    {
        m_animations.clear();
        m_animationClips.clear();
        m_selectedAnimationIndex = 0;
        m_animationTime = 0.0f;
        m_state = STATE_STOPPED;
    }

    void Animator::removeAnimation( SmartPtr<IAnimation> animation )
    {
        m_animations.erase( std::remove( m_animations.begin(), m_animations.end(), animation ),
                            m_animations.end() );
    }

    Array<SmartPtr<IAnimation>> Animator::getAnimations() const
    {
        return m_animations;
    }

    void Animator::setAnimations( const Array<SmartPtr<IAnimation>> &animations )
    {
        m_animations = animations;
        setSelectedAnimationIndex( m_selectedAnimationIndex );
    }

    SmartPtr<Properties> Animator::getProperties() const
    {
        auto properties = Component::getProperties();
        if( !properties )
        {
            return nullptr;
        }

        properties->setProperty( animationTimeStr, m_animationTime );
        properties->setProperty( animationSpeedStr, m_animationSpeed );
        properties->setProperty( animationWeightStr, m_animationWeight );
        properties->setProperty( animationScaleStr, m_animationScale );
        properties->setProperty( loopingStr, m_looping );
        properties->setProperty( selectedAnimationIndexStr, m_selectedAnimationIndex );
        properties->setProperty( selectedAnimationNameStr, getSelectedAnimationName(), true );
        properties->setProperty( selectedAnimationLengthStr, getSelectedAnimationLength(), true );
        properties->setPropertyAsEnum( playbackStateStr, static_cast<s32>( m_state ),
                                       getPlaybackStateNames() );
        properties->setProperty( runtimeClipCountStr, static_cast<u32>( m_animations.size() ), true );
        properties->setProperty( editableClipCountStr, static_cast<u32>( m_animationClips.size() ),
                                 true );
        properties->setProperty( autoPlayStr, m_autoPlay );
        properties->setProperty( resetTimeOnPlayStr, m_resetTimeOnPlay );
        properties->setProperty( resetTimeOnStopStr, m_resetTimeOnStop );
        properties->setProperty( clampTimeToClipStr, m_clampTimeToClip );
        properties->setProperty( applyOnPropertyChangeStr, m_applyOnPropertyChange );
        properties->setButtonPressed( playButtonStr );
        properties->setButtonPressed( pauseButtonStr );
        properties->setButtonPressed( stopButtonStr );
        properties->setButtonPressed( rewindButtonStr );

        auto clipsNode = workphone::make_ptr<Properties>();
        clipsNode->setName( clipsStr );
        clipsNode->setProperty( editableClipCountStr, static_cast<u32>( m_animationClips.size() ),
                                true );
        for( u32 i = 0; i < m_animationClips.size(); ++i )
        {
            const auto &clip = m_animationClips[i];
            if( clip )
            {
                auto clipProperties = clip->getProperties();
                if( clipProperties )
                {
                    clipProperties->setName( clipStr );
                    clipProperties->setProperty( selectedAnimationIndexStr, i, true );
                    clipsNode->addChild( clipProperties );
                }
            }
        }
        properties->addChild( clipsNode );
        properties->addChild( m_ikSystem.getProperties() );

        return properties;
    }

    void Animator::setProperties( SmartPtr<Properties> properties )
    {
        if( !properties )
        {
            return;
        }

        const auto previousApplyOnPropertyChange = m_applyOnPropertyChange;
        m_applyOnPropertyChange = false;

        u32 selectedIndex = m_selectedAnimationIndex;
        f32 animationTime = m_animationTime;
        f32 animationSpeed = m_animationSpeed;
        f32 animationWeight = m_animationWeight;
        f32 animationScale = m_animationScale;
        bool looping = m_looping;
        bool autoPlay = m_autoPlay;
        bool resetTimeOnPlay = m_resetTimeOnPlay;
        bool resetTimeOnStop = m_resetTimeOnStop;
        bool clampTimeToClip = m_clampTimeToClip;
        bool applyOnPropertyChange = previousApplyOnPropertyChange;
        u32 playbackState = m_state;

        properties->getPropertyValue( selectedAnimationIndexStr, selectedIndex );
        properties->getPropertyValue( animationSpeedStr, animationSpeed );
        properties->getPropertyValue( animationWeightStr, animationWeight );
        properties->getPropertyValue( animationScaleStr, animationScale );
        properties->getPropertyValue( loopingStr, looping );
        properties->getPropertyValue( autoPlayStr, autoPlay );
        properties->getPropertyValue( resetTimeOnPlayStr, resetTimeOnPlay );
        properties->getPropertyValue( resetTimeOnStopStr, resetTimeOnStop );
        properties->getPropertyValue( clampTimeToClipStr, clampTimeToClip );
        properties->getPropertyValue( applyOnPropertyChangeStr, applyOnPropertyChange );
        properties->getPropertyValue( animationTimeStr, animationTime );
        const auto hasPlaybackState = properties->getPropertyValue( playbackStateStr, playbackState );

        if( auto clipsNode = properties->getChild( clipsStr ) )
        {
            Array<SmartPtr<Animation>> clips;
            for( auto &clipProperties : clipsNode->getChildrenByName( clipStr ) )
            {
                if( clipProperties )
                {
                    auto clip = workphone::make_ptr<Animation>();
                    clip->setProperties( clipProperties );
                    clips.push_back( clip );
                }
            }

            if( !clips.empty() || clipsNode->hasProperty( editableClipCountStr ) )
            {
                setAnimationClips( clips );
            }
        }

        if( auto ikConstraintsNode = properties->getChild( ikConstraintsStr ) )
        {
            m_ikSystem.setProperties( ikConstraintsNode );
        }

        setSelectedAnimationIndex( selectedIndex );
        setAnimationSpeed( animationSpeed );
        setAnimationWeight( animationWeight );
        setAnimationScale( animationScale );
        setLooping( looping );
        setAutoPlay( autoPlay );
        setResetTimeOnPlay( resetTimeOnPlay );
        setResetTimeOnStop( resetTimeOnStop );
        setClampTimeToClip( clampTimeToClip );
        setAnimationTime( animationTime );

        m_applyOnPropertyChange = applyOnPropertyChange;

        if( properties->isButtonPressed( rewindButtonStr ) )
        {
            setAnimationTime( 0.0f );
        }

        if( properties->isButtonPressed( playButtonStr ) )
        {
            play();
        }
        else if( properties->isButtonPressed( pauseButtonStr ) )
        {
            pause();
        }
        else if( properties->isButtonPressed( stopButtonStr ) )
        {
            stop();
        }
        else if( hasPlaybackState && normalisePlaybackState( playbackState ) != m_state )
        {
            setPlaybackState( playbackState );
        }

        if( m_applyOnPropertyChange )
        {
            onUpdate();
        }

        Component::setProperties( properties );
    }

    Array<SmartPtr<ISharedObject>> Animator::getChildObjects() const
    {
        auto children = Component::getChildObjects();
        children.insert( children.end(), m_animations.begin(), m_animations.end() );
        children.insert( children.end(), m_animationClips.begin(), m_animationClips.end() );
        return children;
    }

    void Animator::setSkeleton( SmartPtr<ISkeleton> skeleton )
    {
        m_skeleton = skeleton;
    }

    SmartPtr<ISkeleton> Animator::getSkeleton() const
    {
        return m_skeleton;
    }

    bool Animator::setAnimationGraph( const animation::AnimationGraphDefinition &definition )
    {
        return m_animationGraph.setDefinition( definition );
    }

    void Animator::clearAnimationGraph()
    {
        m_animationGraph = animation::AnimationGraphInstance();
    }

    bool Animator::hasAnimationGraph() const
    {
        return m_animationGraph.isValid();
    }

    animation::AnimationGraphInstance *Animator::getAnimationGraph()
    {
        return hasAnimationGraph() ? &m_animationGraph : nullptr;
    }

    const animation::AnimationGraphInstance *Animator::getAnimationGraph() const
    {
        return hasAnimationGraph() ? &m_animationGraph : nullptr;
    }

    animation::AnimationIKSystem &Animator::getIKSystem()
    {
        return m_ikSystem;
    }

    const animation::AnimationIKSystem &Animator::getIKSystem() const
    {
        return m_ikSystem;
    }

    const Array<animation::AnimationIKSolveResult> &Animator::getLastIKSolveResults() const
    {
        return m_lastIKSolveResults;
    }
}  // namespace workphone::scene
