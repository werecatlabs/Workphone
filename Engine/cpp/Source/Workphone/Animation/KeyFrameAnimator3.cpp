#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Animation/KeyFrameAnimator3.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <algorithm>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, KeyFrameAnimator3, Animator );

    KeyFrameAnimator3::KeyFrameAnimator3()
    {
        setAnimationTime( 0.f );
        setAnimationLength( 0.f );

        m_currentKeyFrame = make_ptr<KeyFrameTransform3>();
    }

    KeyFrameAnimator3::~KeyFrameAnimator3()
    {
        unload( nullptr );
    }

    void KeyFrameAnimator3::unload( SmartPtr<ISharedObject> data )
    {
        auto keys = makeSourceBudgetSnapshot<SmartPtr<KeyFrameTransform3>>();
        keys.swap( m_keyFrames );
        auto current = m_currentKeyFrame;
        m_currentKeyFrame = nullptr;
        m_positionSpline.unload( nullptr );
        m_orientationSpline.unload( nullptr );
        Animator::unload( data );
    }

    void KeyFrameAnimator3::update()
    {
        Animator::update();

        if( !m_currentKeyFrame || m_keyFrames.empty() )
        {
            return;
        }

        if( m_keyFrames.size() == 1 )
        {
            m_currentKeyFrame = m_keyFrames.front();
            return;
        }

        auto upper = std::upper_bound( m_keyFrames.begin(), m_keyFrames.end(), m_animationTime,
                                       []( f32 value, const SmartPtr<KeyFrameTransform3> &keyFrame ) {
                                           return keyFrame && value < keyFrame->getTime();
                                       } );

        size_t idx1 = 0;
        size_t idx2 = 0;
        if( upper == m_keyFrames.begin() )
        {
            idx1 = idx2 = 0;
        }
        else if( upper == m_keyFrames.end() )
        {
            idx1 = idx2 = m_keyFrames.size() - 1;
        }
        else
        {
            idx2 = static_cast<size_t>( std::distance( m_keyFrames.begin(), upper ) );
            idx1 = idx2 - 1;
        }

        const auto time1 = m_keyFrames[idx1]->getTime();
        const auto time2 = m_keyFrames[idx2]->getTime();
        const auto localT =
            time1 == time2 ? 0.0f
                           : MathF::clamp( ( m_animationTime - time1 ) / ( time2 - time1 ), 0.0f, 1.0f );

        m_currentKeyFrame->setTime( m_animationTime );
        m_currentKeyFrame->setPosition(
            m_keyFrames[idx1]->getPosition() +
            ( m_keyFrames[idx2]->getPosition() - m_keyFrames[idx1]->getPosition() ) * localT );
        m_currentKeyFrame->setScale( m_keyFrames[idx1]->getScale() +
                                     ( m_keyFrames[idx2]->getScale() - m_keyFrames[idx1]->getScale() ) *
                                         localT );
        m_currentKeyFrame->setOrientation( m_keyFrames[idx2]->getOrientation() );
    }

    void KeyFrameAnimator3::addKeyFrame( const SmartPtr<KeyFrameTransform3> &keyFrame )
    {
        if( !keyFrame ) return;
        auto current = m_currentKeyFrame;
        if( !current ) current = make_ptr<KeyFrameTransform3>();
        m_keyFrames.push_back( keyFrame );
        std::sort( m_keyFrames.begin(), m_keyFrames.end(),
                   []( const SmartPtr<KeyFrameTransform3> &a, const SmartPtr<KeyFrameTransform3> &b ) {
                       return a && b ? a->getTime() < b->getTime() : static_cast<bool>( a );
                   } );
        m_currentKeyFrame = std::move( current );
    }

    void KeyFrameAnimator3::addKeyFrames( const Array<SmartPtr<KeyFrameTransform3>> &keyFrames )
    {
        setKeyFrames( keyFrames );
    }

    void KeyFrameAnimator3::setAnimationLength( f32 animationLength )
    {
        Animator::setAnimationLength( animationLength );
    }

    auto KeyFrameAnimator3::getCurrentKeyFrame() const -> SmartPtr<KeyFrameTransform3>
    {
        return m_currentKeyFrame;
    }

    const Array<SmartPtr<KeyFrameTransform3>> &KeyFrameAnimator3::getKeyFrames() const
    {
        return m_keyFrames;
    }

    void KeyFrameAnimator3::setKeyFrames( const Array<SmartPtr<KeyFrameTransform3>> &keyFrames )
    {
        auto candidate = makeSourceBudgetSnapshot( keyFrames );
        candidate.erase( std::remove( candidate.begin(), candidate.end(), nullptr ), candidate.end() );
        std::sort( candidate.begin(), candidate.end(),
                   []( const SmartPtr<KeyFrameTransform3> &a, const SmartPtr<KeyFrameTransform3> &b ) {
                       return a->getTime() < b->getTime();
                   } );
        auto current = m_currentKeyFrame;
        if( !current && !candidate.empty() ) current = make_ptr<KeyFrameTransform3>();
        m_keyFrames.swap( candidate );
        m_currentKeyFrame = std::move( current );
    }

}  // namespace workphone
