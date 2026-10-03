#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Animation/Animator.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <algorithm>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, Animator, IAnimator );

    u32 Animator::m_idExt = 0;

    const hash_type Animator::START_HASH = StringUtil::getHash( "start" );
    const hash_type Animator::STOP_HASH = StringUtil::getHash( "stop" );
    const hash_type Animator::PAUSE_HASH = StringUtil::getHash( "pause" );
    const hash_type Animator::RESUME_HASH = StringUtil::getHash( "resume" );
    const hash_type Animator::COMPLETE_HASH = StringUtil::getHash( "complete" );

    Animator::Animator() :
        m_animationLength( 0.0f ),
        m_animationTime( 0.0f ),
        m_animationSpeed( 1.0f ),
        m_isPlaying( false ),
        m_isPaused( false ),
        m_loop( false ),
        m_reverse( false ),
        m_onCompleteCallback( nullptr )
    {
        //m_id = m_idExt++;
    }

    Animator::~Animator()
    {
        stop();
        m_onCompleteCallback = nullptr;
    }

    void Animator::setLoop( bool loop )
    {
        m_loop = loop;
    }

    auto Animator::isLoop() const -> bool
    {
        return m_loop;
    }

    void Animator::setReverse( bool reverse )
    {
        m_reverse = reverse;
    }

    auto Animator::isReverse() const -> bool
    {
        return m_reverse;
    }

    void Animator::setAnimationSpeed( f32 speed )
    {
        m_animationSpeed = std::max( 0.0f, speed );
    }

    auto Animator::getAnimationSpeed() const -> f32
    {
        return m_animationSpeed;
    }

    void Animator::start()
    {
        if( !m_isPlaying )
        {
            m_isPlaying = true;
            m_isPaused = false;
            m_animationTime = m_reverse ? m_animationLength : 0.0f;
            onStart();
        }
    }

    void Animator::stop()
    {
        if( m_isPlaying )
        {
            m_isPlaying = false;
            m_isPaused = false;
            m_animationTime = 0.0f;
            onStop();
        }
    }

    void Animator::pause()
    {
        if( m_isPlaying && !m_isPaused )
        {
            m_isPaused = true;
            onPause();
        }
    }

    void Animator::resume()
    {
        if( m_isPlaying && m_isPaused )
        {
            m_isPaused = false;
            onResume();
        }
    }

    auto Animator::isPlaying() const -> bool
    {
        return m_isPlaying && !m_isPaused;
    }

    auto Animator::isPaused() const -> bool
    {
        return m_isPaused;
    }

    auto Animator::isFinished() const -> bool
    {
        if( m_loop )
            return false;

        if( m_reverse )
            return m_animationTime <= 0.0f;

        return m_animationTime >= m_animationLength;
    }

    void Animator::setAnimationLength( f32 animationLength )
    {
        m_animationLength = std::max( 0.0f, animationLength );
        setAnimationTime( m_animationTime );
    }

    auto Animator::getAnimationLength() const -> f32
    {
        return m_animationLength;
    }

    auto Animator::getAnimationTime() const -> f32
    {
        return m_animationTime;
    }

    void Animator::setAnimationTime( f32 animationTime )
    {
        m_animationTime = std::clamp( animationTime, 0.0f, m_animationLength );
    }

    void Animator::setOnCompleteCallback( OnCompleteCallback callback )
    {
        m_onCompleteCallback = callback;
    }

    void Animator::update()
    {
        auto applicationManager = core::IApplicationManager::instance();
        if( !applicationManager )
        {
            return;
        }

        auto timer = applicationManager->getTimer();
        if( !timer )
        {
            return;
        }

        auto deltaTime = timer->getDeltaTime();

        if( !m_isPlaying || m_isPaused )
            return;

        // Update animation time based on speed and direction
        if( m_reverse )
        {
            m_animationTime -= (real_Num)deltaTime * m_animationSpeed;
            if( m_animationTime <= real_Num( 0.0 ) )
            {
                if( m_loop )
                {
                    m_animationTime = m_animationLength;
                }
                else
                {
                    m_animationTime = 0.0f;
                    m_isPlaying = false;
                    onComplete();
                }
            }
        }
        else
        {
            m_animationTime += (real_Num)deltaTime * m_animationSpeed;
            if( m_animationTime >= m_animationLength )
            {
                if( m_loop )
                {
                    m_animationTime = 0.0f;
                }
                else
                {
                    m_animationTime = m_animationLength;
                    m_isPlaying = false;
                    onComplete();
                }
            }
        }
    }

    void Animator::onStart()
    {
        // Override in derived classes
    }

    void Animator::onStop()
    {
        // Override in derived classes
    }

    void Animator::onPause()
    {
        // Override in derived classes
    }

    void Animator::onResume()
    {
        // Override in derived classes
    }

    void Animator::onComplete()
    {
        if( m_onCompleteCallback )
        {
            m_onCompleteCallback();
        }
    }

}  // namespace workphone
