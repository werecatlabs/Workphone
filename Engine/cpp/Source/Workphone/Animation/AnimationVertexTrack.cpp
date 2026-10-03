#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Animation/AnimationVertexTrack.hpp>
#include <Workphone/Interface/Animation/IAnimationTimeIndex.hpp>
#include <Workphone/Interface/Mesh/IVertexBuffer.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, AnimationVertexTrack, IAnimationVertexTrack );

    namespace
    {
        template <class T>
        void sortKeyFrames( Array<SmartPtr<T>> &keyFrames )
        {
            keyFrames.erase( std::remove( keyFrames.begin(), keyFrames.end(), nullptr ),
                             keyFrames.end() );
            std::sort( keyFrames.begin(), keyFrames.end(),
                       []( const SmartPtr<T> &a, const SmartPtr<T> &b ) {
                           return a->getTime() < b->getTime();
                       } );
        }

        template <class T>
        f32 findKeyFramesAtTime( const Array<SmartPtr<T>> &keyFrames,
                                 const SmartPtr<IAnimationTimeIndex> &timeIndex,
                                 SmartPtr<IAnimationKeyFrame> &keyFrame1,
                                 SmartPtr<IAnimationKeyFrame> &keyFrame2, u16 *firstKeyIndex )
        {
            const auto time = timeIndex ? timeIndex->getTimePos() : 0.0f;
            if( keyFrames.empty() )
            {
                keyFrame1 = nullptr;
                keyFrame2 = nullptr;
                if( firstKeyIndex )
                {
                    *firstKeyIndex = 0;
                }
                return 0.0f;
            }

            u16 idx1 = 0;
            u16 idx2 = 0;
            if( time <= keyFrames.front()->getTime() )
            {
                idx1 = idx2 = 0;
            }
            else if( time >= keyFrames.back()->getTime() )
            {
                idx1 = idx2 = static_cast<u16>( keyFrames.size() - 1 );
            }
            else
            {
                auto it = std::upper_bound( keyFrames.begin(), keyFrames.end(), time,
                                            []( f32 value, const SmartPtr<T> &keyFrame ) {
                                                return keyFrame && value < keyFrame->getTime();
                                            } );

                idx2 = static_cast<u16>( std::distance( keyFrames.begin(), it ) );
                idx1 = idx2 > 0 ? idx2 - 1 : 0;
            }

            if( firstKeyIndex )
            {
                *firstKeyIndex = idx1;
            }

            keyFrame1 = keyFrames[idx1];
            keyFrame2 = keyFrames[idx2];

            const auto t1 = keyFrame1->getTime();
            const auto t2 = keyFrame2->getTime();
            if( std::abs( t2 - t1 ) <= std::numeric_limits<f32>::epsilon() )
            {
                return 0.0f;
            }

            return std::clamp( ( time - t1 ) / ( t2 - t1 ), 0.0f, 1.0f );
        }
    }  // namespace

    AnimationVertexTrack::AnimationVertexTrack() : AnimationTrack( nullptr )
    {
    }

    AnimationVertexTrack::AnimationVertexTrack( VertexAnimationType type, IAnimation *parent ) :
        AnimationTrack( parent ),
        m_animationType( type )
    {
        // Always create a keyframe at time 0.0 for the appropriate type
        if( m_animationType == VertexAnimationType::VAT_MORPH )
        {
            auto kf = workphone::make_ptr<AnimationMorphKeyFrame>();
            kf->setTime( 0.0f );
            m_morphKeyFrames.push_back( kf );
        }
        else if( m_animationType == VertexAnimationType::VAT_POSE )
        {
            auto kf = workphone::make_ptr<AnimationPoseKeyFrame>();
            kf->setTime( 0.0f );
            m_poseKeyFrames.push_back( kf );
        }
    }

    AnimationVertexTrack::~AnimationVertexTrack() = default;

    VertexAnimationType AnimationVertexTrack::getAnimationType() const
    {
        return m_animationType;
    }

    void AnimationVertexTrack::setAnimationType( VertexAnimationType type )
    {
        if( m_animationType == type )
        {
            return;
        }

        m_animationType = type;
        removeAllKeyFrames();
    }

    TargetMode AnimationVertexTrack::getTargetMode() const
    {
        return m_targetMode;
    }

    void AnimationVertexTrack::setTargetMode( TargetMode mode )
    {
        m_targetMode = mode;
    }

    IAnimationMorphKeyFrame *AnimationVertexTrack::getVertexMorphKeyFrame( u16 index ) const
    {
        if( m_animationType != VertexAnimationType::VAT_MORPH )
        {
            return nullptr;
        }

        if( index >= m_morphKeyFrames.size() )
        {
            return nullptr;
        }

        return m_morphKeyFrames[index].get();
    }

    IAnimationPoseKeyFrame *AnimationVertexTrack::getVertexPoseKeyFrame( u16 index ) const
    {
        if( m_animationType != VertexAnimationType::VAT_POSE )
        {
            return nullptr;
        }

        if( index >= m_poseKeyFrames.size() )
        {
            return nullptr;
        }

        return m_poseKeyFrames[index].get();
    }

    void AnimationVertexTrack::setAssociatedVertexData( IVertexBuffer *data )
    {
        m_vertexData = data ? SmartPtr<IVertexBuffer>( data ) : nullptr;

        // Store the vertex buffer reference in the appropriate keyframes
        if( m_animationType == VertexAnimationType::VAT_MORPH )
        {
            for( auto &keyFrame : m_morphKeyFrames )
            {
                if( keyFrame )
                {
                    keyFrame->setVertexBuffer( SmartPtr<IVertexBuffer>( data ) );
                }
            }
        }
        // For pose animation, vertex data is typically stored differently
        // as it references existing poses rather than direct vertex data
    }

    IVertexBuffer *AnimationVertexTrack::getAssociatedVertexData() const
    {
        if( m_vertexData )
        {
            return m_vertexData.get();
        }

        if( m_animationType == VertexAnimationType::VAT_MORPH && !m_morphKeyFrames.empty() )
        {
            auto keyFrame = m_morphKeyFrames[0];
            if( keyFrame )
            {
                auto vertexBuffer = keyFrame->getVertexBuffer();
                return vertexBuffer.get();
            }
        }

        return nullptr;
    }

    IAnimationMorphKeyFrame *AnimationVertexTrack::createVertexMorphKeyFrame( f32 timePos )
    {
        if( m_animationType != VertexAnimationType::VAT_MORPH )
        {
            return nullptr;
        }

        auto keyFrame = workphone::make_ptr<AnimationMorphKeyFrame>();
        keyFrame->setTime( timePos );
        keyFrame->setVertexBuffer( m_vertexData );

        // Insert the keyframe in the correct position based on time
        auto insertPos = m_morphKeyFrames.begin();
        for( auto it = m_morphKeyFrames.begin(); it != m_morphKeyFrames.end(); ++it )
        {
            if( ( *it )->getTime() > timePos )
            {
                insertPos = it;
                break;
            }
            insertPos = it + 1;
        }

        m_morphKeyFrames.insert( insertPos, keyFrame );
        _keyFrameDataChanged();
        return keyFrame.get();
    }

    IAnimationPoseKeyFrame *AnimationVertexTrack::createVertexPoseKeyFrame( f32 timePos )
    {
        if( m_animationType != VertexAnimationType::VAT_POSE )
        {
            return nullptr;
        }

        auto keyFrame = workphone::make_ptr<AnimationPoseKeyFrame>();
        keyFrame->setTime( timePos );

        // Insert the keyframe in the correct position based on time
        auto insertPos = m_poseKeyFrames.begin();
        for( auto it = m_poseKeyFrames.begin(); it != m_poseKeyFrames.end(); ++it )
        {
            if( ( *it )->getTime() > timePos )
            {
                insertPos = it;
                break;
            }
            insertPos = it + 1;
        }

        m_poseKeyFrames.insert( insertPos, keyFrame );
        _keyFrameDataChanged();
        return keyFrame.get();
    }

    u16 AnimationVertexTrack::getNumKeyFrames() const
    {
        if( m_animationType == VertexAnimationType::VAT_MORPH )
        {
            return static_cast<u16>( m_morphKeyFrames.size() );
        }

        if( m_animationType == VertexAnimationType::VAT_POSE )
        {
            return static_cast<u16>( m_poseKeyFrames.size() );
        }

        return 0;
    }

    SmartPtr<IAnimationKeyFrame> AnimationVertexTrack::getKeyFrame( u16 index ) const
    {
        if( m_animationType == VertexAnimationType::VAT_MORPH && index < m_morphKeyFrames.size() )
        {
            return workphone::static_pointer_cast<IAnimationKeyFrame>( m_morphKeyFrames[index] );
        }

        if( m_animationType == VertexAnimationType::VAT_POSE && index < m_poseKeyFrames.size() )
        {
            return workphone::static_pointer_cast<IAnimationKeyFrame>( m_poseKeyFrames[index] );
        }

        return nullptr;
    }

    SmartPtr<IAnimationKeyFrame> AnimationVertexTrack::createKeyFrame( f32 timePos )
    {
        if( m_animationType == VertexAnimationType::VAT_MORPH )
        {
            return SmartPtr<IAnimationKeyFrame>( createVertexMorphKeyFrame( timePos ) );
        }

        if( m_animationType == VertexAnimationType::VAT_POSE )
        {
            return SmartPtr<IAnimationKeyFrame>( createVertexPoseKeyFrame( timePos ) );
        }

        return nullptr;
    }

    void AnimationVertexTrack::removeKeyFrame( u16 index )
    {
        if( m_animationType == VertexAnimationType::VAT_MORPH && index < m_morphKeyFrames.size() )
        {
            m_morphKeyFrames.erase( m_morphKeyFrames.begin() + index );
            _keyFrameDataChanged();
            return;
        }

        if( m_animationType == VertexAnimationType::VAT_POSE && index < m_poseKeyFrames.size() )
        {
            m_poseKeyFrames.erase( m_poseKeyFrames.begin() + index );
            _keyFrameDataChanged();
        }
    }

    void AnimationVertexTrack::removeAllKeyFrames()
    {
        m_morphKeyFrames.clear();
        m_poseKeyFrames.clear();
        _keyFrameDataChanged();
    }

    f32 AnimationVertexTrack::getKeyFramesAtTime( const SmartPtr<IAnimationTimeIndex> &timeIndex,
                                                  SmartPtr<IAnimationKeyFrame> &keyFrame1,
                                                  SmartPtr<IAnimationKeyFrame> &keyFrame2,
                                                  u16 *firstKeyIndex ) const
    {
        if( m_animationType == VertexAnimationType::VAT_MORPH )
        {
            return findKeyFramesAtTime( m_morphKeyFrames, timeIndex, keyFrame1, keyFrame2,
                                        firstKeyIndex );
        }

        if( m_animationType == VertexAnimationType::VAT_POSE )
        {
            return findKeyFramesAtTime( m_poseKeyFrames, timeIndex, keyFrame1, keyFrame2,
                                        firstKeyIndex );
        }

        keyFrame1 = nullptr;
        keyFrame2 = nullptr;
        if( firstKeyIndex )
        {
            *firstKeyIndex = 0;
        }
        return 0.0f;
    }

    void AnimationVertexTrack::getInterpolatedKeyFrame( const SmartPtr<IAnimationTimeIndex> &timeIndex,
                                                        SmartPtr<IAnimationKeyFrame> &kf ) const
    {
        SmartPtr<IAnimationKeyFrame> kf1;
        SmartPtr<IAnimationKeyFrame> kf2;
        const auto t = getKeyFramesAtTime( timeIndex, kf1, kf2 );

        if( !kf1 || !kf2 )
        {
            kf = nullptr;
            return;
        }

        kf = t < 0.5f ? kf1 : kf2;
    }

    bool AnimationVertexTrack::hasNonZeroKeyFrames() const
    {
        return getNumKeyFrames() > 1;
    }

    void AnimationVertexTrack::optimise()
    {
        sortKeyFrames( m_morphKeyFrames );
        sortKeyFrames( m_poseKeyFrames );
        _keyFrameDataChanged();
    }

    void AnimationVertexTrack::_collectKeyFrameTimes( Array<f32> &keyFrameTimes )
    {
        for( const auto &keyFrame : m_morphKeyFrames )
        {
            if( keyFrame )
            {
                keyFrameTimes.push_back( keyFrame->getTime() );
            }
        }

        for( const auto &keyFrame : m_poseKeyFrames )
        {
            if( keyFrame )
            {
                keyFrameTimes.push_back( keyFrame->getTime() );
            }
        }
    }

    void AnimationVertexTrack::_applyBaseKeyFrame( const SmartPtr<IAnimationKeyFrame> &base )
    {
        if( !base )
        {
            return;
        }

        if( m_animationType == VertexAnimationType::VAT_MORPH )
        {
            if( auto morphBase = workphone::dynamic_pointer_cast<IAnimationMorphKeyFrame>( base ) )
            {
                m_morphKeyFrames.push_back( morphBase );
                optimise();
            }
            return;
        }

        if( m_animationType == VertexAnimationType::VAT_POSE )
        {
            if( auto poseBase = workphone::dynamic_pointer_cast<IAnimationPoseKeyFrame>( base ) )
            {
                m_poseKeyFrames.push_back( poseBase );
                optimise();
            }
        }
    }

    Array<SmartPtr<IAnimationMorphKeyFrame>> AnimationVertexTrack::getMorphKeyFrames() const
    {
        return m_morphKeyFrames;
    }

    void AnimationVertexTrack::setMorphKeyFrames(
        const Array<SmartPtr<IAnimationMorphKeyFrame>> &keyFrames )
    {
        m_morphKeyFrames = keyFrames;
        m_animationType = VertexAnimationType::VAT_MORPH;
        optimise();
    }

    Array<SmartPtr<IAnimationPoseKeyFrame>> AnimationVertexTrack::getPoseKeyFrames() const
    {
        return m_poseKeyFrames;
    }

    void AnimationVertexTrack::setPoseKeyFrames(
        const Array<SmartPtr<IAnimationPoseKeyFrame>> &keyFrames )
    {
        m_poseKeyFrames = keyFrames;
        m_animationType = VertexAnimationType::VAT_POSE;
        optimise();
    }
}  // namespace workphone
