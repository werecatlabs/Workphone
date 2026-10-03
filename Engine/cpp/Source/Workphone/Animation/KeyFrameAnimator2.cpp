#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Animation/KeyFrameAnimator2.hpp>
#include <algorithm>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, KeyFrameAnimator2, Animator );

    KeyFrameAnimator2::KeyFrameAnimator2() = default;

    KeyFrameAnimator2::~KeyFrameAnimator2() = default;

    void KeyFrameAnimator2::update()
    {
        Animator::update();

        if( !m_isPlaying || m_keyFrames.empty() )
            return;

        if( m_keyFrames.size() == 1 )
        {
            m_currentKeyFrame = m_keyFrames[0];
            return;
        }

        auto upper = std::upper_bound( m_keyFrames.begin(), m_keyFrames.end(), m_animationTime,
                                       []( f32 value, const SmartPtr<KeyFrameTransform2> &keyFrame ) {
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

        // Interpolate position
        Vector2<real_Num> pos1 = m_keyFrames[idx1]->getPosition();
        Vector2<real_Num> pos2 = m_keyFrames[idx2]->getPosition();
        Vector2<real_Num> interpPos = pos1 + ( pos2 - pos1 ) * localT;

        // Interpolate scale
        Vector2<real_Num> scale1 = m_keyFrames[idx1]->getScale();
        Vector2<real_Num> scale2 = m_keyFrames[idx2]->getScale();
        Vector2<real_Num> interpScale = scale1 + ( scale2 - scale1 ) * localT;

        // Interpolate rotation (simple lerp, could be improved for angles)
        f32 rot1 = m_keyFrames[idx1]->getRotation();
        f32 rot2 = m_keyFrames[idx2]->getRotation();
        f32 interpRot = rot1 + ( rot2 - rot1 ) * localT;

        // Set current keyframe
        if( !m_currentKeyFrame )
            m_currentKeyFrame = workphone::make_ptr<KeyFrameTransform2>();
        m_currentKeyFrame->setPosition( interpPos );
        m_currentKeyFrame->setScale( interpScale );
        m_currentKeyFrame->setRotation( interpRot );
    }

    void KeyFrameAnimator2::addKeyFrame( const SmartPtr<KeyFrameTransform2> &keyFrame )
    {
        m_keyFrames.push_back( keyFrame );
        std::sort( m_keyFrames.begin(), m_keyFrames.end(),
                   []( const SmartPtr<KeyFrameTransform2> &a, const SmartPtr<KeyFrameTransform2> &b ) {
                       return a && b ? a->getTime() < b->getTime() : static_cast<bool>( a );
                   } );
    }

    void KeyFrameAnimator2::addKeyFrames( const Array<SmartPtr<KeyFrameTransform2>> &keyFrames )
    {
        for( const auto &kf : keyFrames )
            addKeyFrame( kf );
    }

    void KeyFrameAnimator2::setAnimationLength( f32 animationLength )
    {
        Animator::setAnimationLength( animationLength );
    }

    SmartPtr<KeyFrameTransform2> KeyFrameAnimator2::getCurrentKeyFrame() const
    {
        return m_currentKeyFrame;
    }

    const Array<SmartPtr<KeyFrameTransform2>> &KeyFrameAnimator2::getKeyFrames() const
    {
        return m_keyFrames;
    }

    void KeyFrameAnimator2::setKeyFrames( const Array<SmartPtr<KeyFrameTransform2>> &keyFrames )
    {
        m_keyFrames.clear();
        addKeyFrames( keyFrames );
    }

}  // namespace workphone
