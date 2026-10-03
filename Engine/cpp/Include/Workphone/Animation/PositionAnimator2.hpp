#ifndef _PositionAnimator2_H
#define _PositionAnimator2_H

#include <Workphone/Animation/Animator.hpp>
#include <Workphone/Math/Vector2.hpp>

namespace workphone
{
    /**
     * @class PositionAnimator2
     * @brief Handles linear interpolation of the position for a target object in 2D space over a
     * specified duration.
     * @tparam T The type of the object to animate. Must implement setPosition().
     */
    template <class T>
    class PositionAnimator2 : public Animator
    {
    public:
        /** @brief Default constructor. */
        PositionAnimator2();

        /**
         * @brief Constructs a position animator for a specific object.
         * @param object The target object to animate.
         * @param animationLength The duration of the animation in seconds.
         * @param start The starting position vector.
         * @param end The ending position vector.
         */
        PositionAnimator2( T *object, f32 animationLength,
                           const Vector2<real_Num> &start = Vector2<real_Num>::UNIT,
                           const Vector2<real_Num> &end = Vector2<real_Num>::UNIT );

        ~PositionAnimator2() override;

        /** @brief Updates the animation progress and applies the calculated position to the object. */
        void update() override;

        /** @brief Resets the animation timer and starts playback. */
        void start() override;

        /** @brief Checks if the animation has reached its end point. */
        bool isFinished() const override;

        /** @brief Sets the starting position of the animation. */
        void setStart( const Vector2<real_Num> &start );

        /** @brief Gets the starting position of the animation. */
        Vector2<real_Num> getStart() const;

        /** @brief Sets the ending position of the animation. */
        void setEnd( const Vector2<real_Num> &end );

        /** @brief Gets the ending position of the animation. */
        Vector2<real_Num> getEnd() const;

    protected:
        Vector2<real_Num> m_start;  ///< Starting position of the animation.
        Vector2<real_Num> m_end;    ///< Ending position of the animation.
        T *m_object;                ///< The object being animated.
    };

    template <class T>
    PositionAnimator2<T>::PositionAnimator2() :
        m_animationLength( 1.0f ),
        m_animationTime( 0.0f ),
        m_start( Vector2<real_Num>::UNIT ),
        m_end( Vector2<real_Num>::UNIT ),
        m_object( nullptr )
    {
    }

    template <class T>
    PositionAnimator2<T>::PositionAnimator2( T *object, f32 animationLength,
                                             const Vector2<real_Num> &start = Vector2<real_Num>::UNIT,
                                             const Vector2<real_Num> &end = Vector2<real_Num>::UNIT ) :
        m_start( start ),
        m_end( end ),
        m_object( object )
    {
        setAnimationTime( 0.f );
        setAnimationLength( animationLength );
    }

    template <class T>
    PositionAnimator2<T>::~PositionAnimator2()
    {
    }

    template <class T>
    void PositionAnimator2<T>::update()
    {
        if( m_isPlaying )
        {
            m_animationTime += dt * ( 1.0f / m_animationLength );
            m_animationTime = MathF::clamp( m_animationTime, 0.0f, 1.0f );
            if( m_loop && m_animationTime == 1.0f )
                m_animationTime = 0.0f;

            Vector2<real_Num> position;
            if( !m_reverse )
                position = m_start + ( m_end - m_start ) * m_animationTime;
            else
                position = m_end + ( m_start - m_end ) * m_animationTime;

            m_object->setPosition( position );
        }
    }

    template <class T>
    void PositionAnimator2<T>::start()
    {
        Animator::start();
        m_animationTime = 0.0f;
    }

    template <class T>
    bool PositionAnimator2<T>::isFinished() const
    {
        return m_animationTime >= ( 1.0 - MathF::epsilon() );
    }

    template <class T>
    void PositionAnimator2<T>::setStart( const Vector2<real_Num> &start )
    {
        m_start = start;
    }

    template <class T> /** */
    Vector2<real_Num> PositionAnimator2<T>::getStart() const
    {
        return m_start;
    }

    template <class T>
    void PositionAnimator2<T>::setEnd( const Vector2<real_Num> &end )
    {
        m_end = end;
    }

    template <class T>
    Vector2<real_Num> PositionAnimator2<T>::getEnd() const
    {
        return m_end;
    }
}  // namespace workphone

#endif
