#ifndef CAnimationTrack_h__
#define CAnimationTrack_h__

#include <Workphone/Interface/Animation/IAnimationTrack.hpp>
#include <Workphone/Animation/AnimationKeyFrame.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Interface/Animation/IAnimation.hpp>
#include <Workphone/Interface/Animation/IAnimationTimeIndex.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

namespace workphone
{
    /**
     * @class AnimationTrack
     * @brief A template-based animation track that manages a collection of keyframes and handles
     * interpolation.
     * @tparam T The base class type that this animation track extends.
     */
    template <class T>
    class AnimationTrack : public T
    {
    public:
        /** @brief Default constructor. */
        AnimationTrack() = default;

        /**
         * @brief Constructs an animation track with an optional parent animation.
         * @param parent Pointer to the parent IAnimation object.
         */
        AnimationTrack( IAnimation *parent = nullptr );

        ~AnimationTrack() override;

        /** @brief Gets the unique handle associated with the animation. */
        u16 getAnimationHandle() const override;

        /** @brief Sets the unique handle associated with the animation. */
        void setAnimationHandle( u16 handle ) override;

        /** @brief Returns the total number of keyframes in the track. */
        u16 getNumKeyFrames() const override;

        /** @brief Retrieves a keyframe at the specified index. */
        SmartPtr<IAnimationKeyFrame> getKeyFrame( u16 index ) const override;

        /**
         * @brief Calculates the interpolation factor between two keyframes at a given time index.
         * @param timeIndex The index containing current animation time.
         * @param keyFrame1 Output: The first keyframe involved in interpolation.
         * @param keyFrame2 Output: The second keyframe involved in interpolation.
         * @param firstKeyIndex Output: The index of the first keyframe.
         * @return The interpolation factor (0.0 to 1.0) between the two keyframes.
         */
        f32 getKeyFramesAtTime( const SmartPtr<IAnimationTimeIndex> &timeIndex,
                                SmartPtr<IAnimationKeyFrame> &keyFrame1,
                                SmartPtr<IAnimationKeyFrame> &keyFrame2,
                                u16 *firstKeyIndex = nullptr ) const override;

        /** @brief Creates a new keyframe at the specified time position. */
        SmartPtr<IAnimationKeyFrame> createKeyFrame( f32 timePos ) override;

        /** @brief Removes a keyframe at the specified index. */
        void removeKeyFrame( u16 index ) override;

        /** @brief Removes all keyframes from the track. */
        void removeAllKeyFrames() override;

        /**
         * @brief Gets an interpolated keyframe based on the current time index.
         * @param timeIndex The index containing current animation time.
         * @param kf Output: The resulting interpolated keyframe.
         */
        void getInterpolatedKeyFrame( const SmartPtr<IAnimationTimeIndex> &timeIndex,
                                      SmartPtr<IAnimationKeyFrame> &kf ) const override;

        /**
         * @brief Applies the animation state to the target object.
         * @param timeIndex The index containing current animation time.
         * @param weight The blend weight of this track.
         * @param scale The overall scale factor.
         */
        void apply( const SmartPtr<IAnimationTimeIndex> &timeIndex, f32 weight = 1.0,
                    f32 scale = 1.0f ) override;

        /** @brief Notifies the system that keyframe data has changed. */
        void _keyFrameDataChanged() const override;

        /** @brief Checks if the track contains any keyframes with non-zero values. */
        bool hasNonZeroKeyFrames() const override;

        /** @brief Optimizes the track by removing redundant keyframes. */
        void optimise() override;

        /** @brief Collects the timestamps of all keyframes in the track. */
        void _collectKeyFrameTimes( Array<f32> &keyFrameTimes ) override;

        /** @brief Builds a map to accelerate keyframe lookup based on time. */
        void _buildKeyFrameIndexMap( const Array<f32> &keyFrameTimes ) override;

        /** @brief Applies a base keyframe's data to the track's state. */
        void _applyBaseKeyFrame( const SmartPtr<IAnimationKeyFrame> &base ) override;

        /** @brief Sets the event listener for this animation track. */
        void setListener( IEventListener *l ) override;

        /** @brief Gets the parent animation. */
        IAnimation *getParent() const override;

        /** @brief Sets the parent animation. */
        void setParent( IAnimation *parent );

        /** @brief Gets the list of all keyframes. */
        const Array<SmartPtr<IAnimationKeyFrame>> &getKeyFrames() const;

        /** @brief Sets the list of keyframes for the track. */
        void setKeyFrames( const Array<SmartPtr<IAnimationKeyFrame>> &keyFrames );

        WP_CLASS_REGISTER_TEMPLATE_DECL( AnimationTrack, T );

    private:
        Array<SmartPtr<IAnimationKeyFrame>> m_keyFrames;  ///< List of keyframes in this track.
        IEventListener *m_listener = nullptr;             ///< Listener for animation events.
        IAnimation *m_parent = nullptr;                   ///< Parent animation.
        u16 m_animationHandle = 0;                        ///< Unique animation handle.
    };

    WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone, AnimationTrack, T, T );

    // Template method definitions
    template <class T>
    AnimationTrack<T>::AnimationTrack( IAnimation *parent ) : T(), m_parent( parent )
    {
        // Always create a keyframe at time 0.0
        auto kf = workphone::make_ptr<AnimationKeyFrame>();
        kf->setTime( 0.0f );
        m_keyFrames.push_back( kf );
    }

    template <class T>
    AnimationTrack<T>::~AnimationTrack() = default;

    template <class T>
    u16 AnimationTrack<T>::getAnimationHandle() const
    {
        return m_animationHandle;
    }

    template <class T>
    void AnimationTrack<T>::setAnimationHandle( u16 handle )
    {
        m_animationHandle = handle;
    }

    template <class T>
    u16 AnimationTrack<T>::getNumKeyFrames() const
    {
        return static_cast<u16>( m_keyFrames.size() );
    }

    template <class T>
    SmartPtr<IAnimationKeyFrame> AnimationTrack<T>::getKeyFrame( u16 index ) const
    {
        if( index < m_keyFrames.size() )
            return m_keyFrames[index];
        return nullptr;
    }

    template <class T>
    f32 AnimationTrack<T>::getKeyFramesAtTime( const SmartPtr<IAnimationTimeIndex> &timeIndex,
                                               SmartPtr<IAnimationKeyFrame> &keyFrame1,
                                               SmartPtr<IAnimationKeyFrame> &keyFrame2,
                                               u16 *firstKeyIndex ) const
    {
        f32 time = timeIndex ? timeIndex->getTimePos() : 0.0f;
        if( m_keyFrames.empty() )
        {
            keyFrame1 = nullptr;
            keyFrame2 = nullptr;
            if( firstKeyIndex )
                *firstKeyIndex = 0;
            return 0.0f;
        }
        u16 idx1 = 0;
        u16 idx2 = 0;

        if( timeIndex && timeIndex->hasKeyIndex() && timeIndex->getKeyIndex() < m_keyFrames.size() )
        {
            idx1 = static_cast<u16>( timeIndex->getKeyIndex() );
            idx2 = std::min<u16>( idx1 + 1, static_cast<u16>( m_keyFrames.size() - 1 ) );
        }
        else if( time <= m_keyFrames.front()->getTime() )
        {
            idx1 = idx2 = 0;
        }
        else if( time >= m_keyFrames.back()->getTime() )
        {
            idx1 = idx2 = static_cast<u16>( m_keyFrames.size() - 1 );
        }
        else
        {
            auto it = std::upper_bound( m_keyFrames.begin(), m_keyFrames.end(), time,
                                        []( f32 value, const SmartPtr<IAnimationKeyFrame> &keyFrame ) {
                                            return keyFrame && value < keyFrame->getTime();
                                        } );

            idx2 = static_cast<u16>( std::distance( m_keyFrames.begin(), it ) );
            idx1 = idx2 > 0 ? idx2 - 1 : 0;
        }

        if( firstKeyIndex )
        {
            *firstKeyIndex = idx1;
        }

        keyFrame1 = m_keyFrames[idx1];
        keyFrame2 = m_keyFrames[idx2];

        if( !keyFrame1 || !keyFrame2 )
        {
            return 0.0f;
        }

        const auto t1 = keyFrame1->getTime();
        const auto t2 = keyFrame2->getTime();
        if( std::abs( t2 - t1 ) <= std::numeric_limits<f32>::epsilon() )
        {
            return 0.0f;
        }

        return std::clamp( ( time - t1 ) / ( t2 - t1 ), 0.0f, 1.0f );
    }

    template <class T>
    SmartPtr<IAnimationKeyFrame> AnimationTrack<T>::createKeyFrame( f32 timePos )
    {
        timePos = std::max( 0.0f, timePos );

        auto existing = std::find_if(
            m_keyFrames.begin(), m_keyFrames.end(), [timePos]( const SmartPtr<IAnimationKeyFrame> &kf ) {
                return kf && std::abs( kf->getTime() - timePos ) <= std::numeric_limits<f32>::epsilon();
            } );

        if( existing != m_keyFrames.end() )
        {
            return *existing;
        }

        auto kf = workphone::make_ptr<AnimationKeyFrame>();
        kf->setTime( timePos );
        m_keyFrames.push_back( kf );
        std::sort( m_keyFrames.begin(), m_keyFrames.end(),
                   []( const SmartPtr<IAnimationKeyFrame> &a, const SmartPtr<IAnimationKeyFrame> &b ) {
                       if( !a )
                       {
                           return false;
                       }
                       if( !b )
                       {
                           return true;
                       }
                       return a->getTime() < b->getTime();
                   } );
        _keyFrameDataChanged();
        return kf;
    }

    template <class T>
    void AnimationTrack<T>::removeKeyFrame( u16 index )
    {
        if( index < m_keyFrames.size() )
        {
            m_keyFrames.erase( m_keyFrames.begin() + index );
            _keyFrameDataChanged();
        }
    }

    template <class T>
    void AnimationTrack<T>::removeAllKeyFrames()
    {
        m_keyFrames.clear();
        _keyFrameDataChanged();
    }

    template <class T>
    void AnimationTrack<T>::getInterpolatedKeyFrame( const SmartPtr<IAnimationTimeIndex> &timeIndex,
                                                     SmartPtr<IAnimationKeyFrame> &kf ) const
    {
        SmartPtr<IAnimationKeyFrame> kf1, kf2;
        f32 t = getKeyFramesAtTime( timeIndex, kf1, kf2 );
        if( !kf1 || !kf2 )
        {
            kf = nullptr;
            return;
        }

        kf = t < 0.5f ? kf1 : kf2;
    }

    template <class T>
    void AnimationTrack<T>::apply( const SmartPtr<IAnimationTimeIndex> &timeIndex, f32 weight,
                                   f32 scale )
    {
    }

    template <class T>
    void AnimationTrack<T>::_keyFrameDataChanged() const
    {
        if( m_parent )
        {
            m_parent->_keyFrameListChanged();
        }
    }

    template <class T>
    bool AnimationTrack<T>::hasNonZeroKeyFrames() const
    {
        return std::any_of(
            m_keyFrames.begin(), m_keyFrames.end(), []( const SmartPtr<IAnimationKeyFrame> &kf ) {
                return kf && std::abs( kf->getTime() ) > std::numeric_limits<f32>::epsilon();
            } );
    }

    template <class T>
    void AnimationTrack<T>::optimise()
    {
        m_keyFrames.erase( std::remove( m_keyFrames.begin(), m_keyFrames.end(), nullptr ),
                           m_keyFrames.end() );
        std::sort( m_keyFrames.begin(), m_keyFrames.end(),
                   []( const SmartPtr<IAnimationKeyFrame> &a, const SmartPtr<IAnimationKeyFrame> &b ) {
                       return a->getTime() < b->getTime();
                   } );
        auto last = std::unique(
            m_keyFrames.begin(), m_keyFrames.end(),
            []( const SmartPtr<IAnimationKeyFrame> &a, const SmartPtr<IAnimationKeyFrame> &b ) {
                return std::abs( a->getTime() - b->getTime() ) <= std::numeric_limits<f32>::epsilon();
            } );
        m_keyFrames.erase( last, m_keyFrames.end() );
        _keyFrameDataChanged();
    }

    template <class T>
    void AnimationTrack<T>::_collectKeyFrameTimes( Array<f32> &keyFrameTimes )
    {
        for( const auto &kf : m_keyFrames )
        {
            if( kf )
            {
                keyFrameTimes.push_back( kf->getTime() );
            }
        }
        std::sort( keyFrameTimes.begin(), keyFrameTimes.end() );
        keyFrameTimes.erase( std::unique( keyFrameTimes.begin(), keyFrameTimes.end() ),
                             keyFrameTimes.end() );
    }

    template <class T>
    void AnimationTrack<T>::_buildKeyFrameIndexMap( const Array<f32> & /*keyFrameTimes*/ )
    {
        // Stub: implement if needed for optimization
    }

    template <class T>
    void AnimationTrack<T>::_applyBaseKeyFrame( const SmartPtr<IAnimationKeyFrame> &base )
    {
        if( !base )
        {
            return;
        }

        for( auto &kf : m_keyFrames )
        {
            if( kf && std::abs( kf->getTime() ) <= std::numeric_limits<f32>::epsilon() )
            {
                kf = base;
                _keyFrameDataChanged();
                return;
            }
        }

        m_keyFrames.push_back( base );
        optimise();
    }

    template <class T>
    void AnimationTrack<T>::setListener( IEventListener *l )
    {
        m_listener = l;
    }

    template <class T>
    IAnimation *AnimationTrack<T>::getParent() const
    {
        return m_parent;
    }

    template <class T>
    void AnimationTrack<T>::setParent( IAnimation *parent )
    {
        m_parent = parent;
    }

    template <class T>
    const Array<SmartPtr<IAnimationKeyFrame>> &AnimationTrack<T>::getKeyFrames() const
    {
        return m_keyFrames;
    }

    template <class T>
    void AnimationTrack<T>::setKeyFrames( const Array<SmartPtr<IAnimationKeyFrame>> &keyFrames )
    {
        m_keyFrames = keyFrames;
        optimise();
    }

}  // namespace workphone

#endif  // CAnimationTrack_h__
