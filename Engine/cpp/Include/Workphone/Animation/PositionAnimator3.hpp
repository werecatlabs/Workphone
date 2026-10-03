#ifndef _PositionAnimator3_H
#define _PositionAnimator3_H

#include "Workphone/Animation/Animator.hpp"
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{
    /**
     * @class PositionAnimator3
     * @brief Handles linear interpolation of the position for a target object over a specified duration.
     * @tparam T The type of the object to animate. Must implement setPosition().
     */
    template <class T>
    class PositionAnimator3 : public Animator
    {
    public:
        /** @brief Default constructor. */
        PositionAnimator3();

        /**
         * @brief Constructs a position animator for a specific object.
         * @param object The target object to animate.
         * @param animationLength The duration of the animation in seconds.
         * @param startScale The starting position vector.
         * @param endScale The ending position vector.
         */
        PositionAnimator3( T *object, f32 animationLength,
                           const Vector3<real_Num> &startScale = Vector3<real_Num>::unit(),
                           const Vector3<real_Num> &endScale = Vector3<real_Num>::unit() );

        ~PositionAnimator3() override;

        /** @brief Updates the animation progress and applies the calculated position to the object. */
        void update() override;

        /** @brief Resets the animation timer and starts playback. */
        void start() override;

        /** @brief Checks if the animation has reached its end point. */
        bool isFinished() const override;

    protected:
        Vector3<real_Num> m_start;  ///< Starting position of the animation.
        Vector3<real_Num> m_end;    ///< Ending position of the animation.

        T *m_object;  ///< The object being animated.
    };

    template <class T>
    PositionAnimator3<T>::PositionAnimator3() :
        m_start( Vector3<real_Num>::unit() ),
        m_end( Vector3<real_Num>::unit() ),
        m_object( nullptr )
    {
    }

    template <class T>
    PositionAnimator3<T>::PositionAnimator3(
        T *object, f32 animationLength, const Vector3<real_Num> &startScale = Vector3<real_Num>::unit(),
        const Vector3<real_Num> &endScale = Vector3<real_Num>::unit() ) :
        m_animationLength( animationLength ),
        m_animationTime( 0.0f ),
        m_start( startScale ),
        m_end( endScale ),
        m_object( object )
    {
    }

    template <class T>
    PositionAnimator3<T>::~PositionAnimator3()
    {
    }

    template <class T>
    void PositionAnimator3<T>::update()
    {
        if( m_isPlaying )
        {
            m_animationTime += dt * ( 1.0f / m_animationLength );
            m_animationTime = MathF::clamp( m_animationTime, 0.0f, 1.0f );
            Vector3<real_Num> position = m_start + ( m_end - m_start ) * m_animationTime;
            m_object->setPosition( position );
        }
    }

    template <class T>
    void PositionAnimator3<T>::start()
    {
        Animator::start();
        m_animationTime = 0.0f;
    }

    template <class T>
    bool PositionAnimator3<T>::isFinished() const
    {
        return m_animationTime >= ( 1.0 - MathF::epsilon() );
    }
}  // namespace workphone

#endif
