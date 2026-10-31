#ifndef _ScaleAnimator3_H
#define _ScaleAnimator3_H

#include <Workphone/Animation/Animator.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <algorithm>

namespace workphone
{
    /**
     * @class ScaleAnimator3
     * @brief Handles linear interpolation of the scale for a target object over a specified duration.
     * @tparam T The type of the object to animate. Must implement setScale().
     */
    template <class T>
    class ScaleAnimator3 : public Animator
    {
    public:
        /** @brief Default constructor. */
        ScaleAnimator3();

        /**
         * @brief Constructs a scale animator for a specific object.
         * @param object The target object to animate.
         * @param animationLength The duration of the animation in seconds.
         * @param startScale The starting scale vector.
         * @param endScale The ending scale vector.
         */
        ScaleAnimator3( T *object, f32 animationLength,
                        const Vector3<real_Num> &startScale = Vector3<real_Num>::unit(),
                        const Vector3<real_Num> &endScale = Vector3<real_Num>::unit() );

        ~ScaleAnimator3() override;

        /** @brief Updates the animation progress and applies the calculated scale to the object. */
        void update() override;

        /** @brief Resets the animation timer and starts playback. */
        void start() override;

        /** @brief Checks if the animation has reached its end point. */
        bool isFinished() const override;

    protected:
        void onComplete() override;
        void applyValue();
        Vector3<real_Num> m_start;  ///< Starting scale of the animation.
        Vector3<real_Num> m_end;    ///< Ending scale of the animation.

        T *m_object;  ///< The object being animated.
        bool *m_completionMarker = nullptr;
    };

    template <class T>
    ScaleAnimator3<T>::ScaleAnimator3() :
        m_start( Vector3<real_Num>::unit() ),
        m_end( Vector3<real_Num>::unit() ),
        m_object( nullptr )
    {
    }

    template <class T>
    ScaleAnimator3<T>::ScaleAnimator3( T *object, f32 animationLength,
                                       const Vector3<real_Num> &startScale,
                                       const Vector3<real_Num> &endScale ) :
        m_start( startScale ),
        m_end( endScale ),
        m_object( object )
    {
        setAnimationLength( animationLength );
        setAnimationTime( 0.0f );
    }

    template <class T>
    ScaleAnimator3<T>::~ScaleAnimator3()
    {
    }

    template <class T>
    void ScaleAnimator3<T>::update()
    {
        if( !isPlaying() )
            return;
        bool completed = false;
        m_completionMarker = &completed;
        try
        {
            Animator::update();
        }
        catch( ... )
        {
            if( !completed )
                m_completionMarker = nullptr;
            throw;
        }
        // A completion callback may release the target or this animator.
        if( completed )
            return;
        m_completionMarker = nullptr;
        applyValue();
    }

    template <class T>
    void ScaleAnimator3<T>::onComplete()
    {
        auto *marker = m_completionMarker;
        m_completionMarker = nullptr;
        if( marker )
            *marker = true;
        applyValue();
        Animator::onComplete();
    }

    template <class T>
    void ScaleAnimator3<T>::applyValue()
    {
        if( !m_object )
            return;
        const auto length = getAnimationLength();
        const auto progress = length > 0.0f ? std::clamp( getAnimationTime() / length, 0.0f, 1.0f ) :
                                               ( isReverse() ? 0.0f : 1.0f );
        m_object->setScale( m_start + ( m_end - m_start ) * real_Num( progress ) );
    }

    template <class T>
    void ScaleAnimator3<T>::start()
    {
        Animator::start();
    }

    template <class T>
    bool ScaleAnimator3<T>::isFinished() const
    {
        return Animator::isFinished();
    }
}  // namespace workphone

#endif
